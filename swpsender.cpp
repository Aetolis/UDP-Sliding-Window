// swpsender.cpp
// SWP sender class implementation

#include "swp.h"

using namespace std;

// Constructor
SWPSender::SWPSender()
{
    // Initialize UDP variables
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket

    packet_num = 0;
    LAR = INIT_SEQ_NUM;
    LFS = (INIT_SEQ_NUM + WINDOW_SIZE - 1) % MAX_SEQ_NUM;
    // windowSize = WINDOW_SIZE;

    // Initialize the send buffer
    for (int i = 0; i < WINDOW_SIZE; i++)
    {
        // send_buf[i].ack_status = false;
        send_buf[i].data_len = 0;
        send_buf[i].timestamp = 0;
    }
}

// Destructor
SWPSender::~SWPSender()
{
    // Close socket
    close(sock_fd);
}

// Connect
int SWPSender::connect(char *hostname)
{
    struct addrinfo *sender_info;

    // Get sender's address info
    if ((status = getaddrinfo(hostname, UDP_PORT, &hints, &sender_info)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(-1);
    }

    // Iterate over linked-list of addrinfo structs returned by getadderinfo
    for (addr_ptr = sender_info; addr_ptr != NULL; addr_ptr = addr_ptr->ai_next)
    {
        sock_fd = socket(addr_ptr->ai_family, addr_ptr->ai_socktype, addr_ptr->ai_protocol);
        if (sock_fd != -1)
        {
            break;
        }
    }

    if (addr_ptr == NULL)
    {
        fprintf(stderr, "[Sender] failed to create socket\n");
        exit(2);
    }

    char recv_ip[INET6_ADDRSTRLEN];
    // get server IP address
    if (inet_ntop(addr_ptr->ai_family, &((struct sockaddr_in *)addr_ptr->ai_addr)->sin_addr, recv_ip, INET6_ADDRSTRLEN) == NULL)
    {
        perror("[Sender] inet_ntop");
        exit(-1);
    }

    // Set up pollfd struct
    pfds[0].fd = sock_fd;
    pfds[0].events = POLLIN;

    freeaddrinfo(sender_info);

    // Establish virtual connection with receiver
    char vcon_packet[HEADER_SIZE];

    // Set initial sequence number
    uint32_t seq_num = htonl((uint32_t)INIT_SEQ_NUM);
    memcpy(vcon_packet, &seq_num, sizeof(uint32_t));

    // Set ACK flag as 0x00
    vcon_packet[4] = 0x00;

    // Set control flag as 0x01
    vcon_packet[5] = 0x01;

    // Set data length as 0
    vcon_packet[6] = 0x00;
    vcon_packet[7] = 0x00;

    // Send connection request
    char recv_buf[HEADER_SIZE];
    uint32_t recv_seq_num;
    bool connected = false;
    fprintf(stdout, "[Sender] establishing connection with receiver...\n");

    // Retry connection MAX_RETRY times
    for (int i = 0; i < MAX_RETRY; i++)
    {
        if (!error(2)) {
            if (sendto(sock_fd, vcon_packet, HEADER_SIZE, 0, addr_ptr->ai_addr, addr_ptr->ai_addrlen) == -1)
            {
                fprintf(stderr, "[Sender] connection attempt #%d: sendto error\n", i);
                continue;
            }
        } else {
            fprintf(stderr, "[Sender] connection attempt #%d: packet lost\n", i);
        }
        

        // Wait for response
        if (poll(pfds, 1, 1000) == -1)
        {
            fprintf(stderr, "[Sender] connection attempt #%d: poll error\n", i);
            continue;
        }

        if (pfds[0].revents & POLLIN)
        {
            if (recvfrom(sock_fd, recv_buf, HEADER_SIZE, 0, NULL, 0) == -1)
            {
                fprintf(stderr, "[Sender] connection attempt #%d: no response\n", i);
                exit(3);
            }

            // Check if initial sequence number is valid
            memcpy(&recv_seq_num, recv_buf, sizeof(uint32_t));
            recv_seq_num = ntohl(recv_seq_num);
            if (recv_seq_num != (u_int32_t)INIT_SEQ_NUM)
            {
                fprintf(stderr, "[Sender] connection attempt #%d: invalid initial sequence number\n", i);
                exit(3);
            }

            // Check if ACK flag is 0x01
            if (recv_buf[4] != 0x01)
            {
                fprintf(stderr, "[Sender] connection attempt #%d: ACK flag invalid\n", i);
                exit(3);
            }

            // Check if control flag is 0x01
            if (recv_buf[5] != 0x01)
            {
                fprintf(stderr, "[Sender] connection attempt #%d: control flag invalid\n", i);
                exit(3);
            }

            // Check if data length is 0x0000
            if (recv_buf[6] != 0x00 || recv_buf[7] != 0x00)
            {
                fprintf(stderr, "[Sender] connection attempt #%d: invalid data length\n", i);
                exit(3);
            }

            // Connection request successful
            connected = true;
            break;
        }
    }

    if (connected)
    {
        fprintf(stdout, "[Sender] virtual connection established with %s!\n", recv_ip);
    }
    else
    {
        fprintf(stderr, "[Sender] failed to establish virtual connection\n");
        exit(-1);
    }

    fprintf(stdout, "[Sender] initial sequence number: %d\n", recv_seq_num);

    return 0;
}

int SWPSender::send_file(char *filename)
{
    // Check file exists
    if (access(filename, F_OK) == -1)
    {
        fprintf(stderr, "[Sender] file %s does not exist\n", filename);
        exit(-1);
    }

    // Open file
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        fprintf(stderr, "[Sender] failed to open \"%s\"\n", filename);
        exit(-1);
    }

    bool EOF_flag = false;
    bool EOT_flag = false;
    int cur_len;
    char read_buf[MAX_DATA_SIZE];
    char recv_buf[HEADER_SIZE];
    uint32_t recv_seq_num;
    uint32_t temp_seq_num;
    uint32_t final_seq_num;
    uint16_t temp_data_len;

    while (!EOT_flag)
    {
        // Check for ACKs
        if (poll(pfds, 1, 0) == -1)
        {
            fprintf(stderr, "[Sender] failed to poll\n");
            continue;
        }

        if (pfds[0].revents & POLLIN)
        {
            // Receive ACK
            if (recvfrom(sock_fd, recv_buf, HEADER_SIZE, 0, NULL, 0) == -1)
            {
                fprintf(stderr, "[Sender] failed to receive ACK\n");
                continue;
            }

            // Check if ACK flag is 0x01
            if (recv_buf[4] != 0x01)
            {
                fprintf(stderr, "[Sender] ACK flag not set\n");
                continue;
            }

            // Check if control flag is 0x00
            if (recv_buf[5] != 0x00)
            {
                fprintf(stderr, "[Sender] control flag not valid\n");
                continue;
            }

            // Check if data length is valid
            if (recv_buf[6] != 0x00 || recv_buf[7] != 0x00)
            {
                fprintf(stderr, "[Sender] invalid data length\n");
                continue;
            }

            // Check if sequence number is valid
            memcpy(&recv_seq_num, recv_buf, sizeof(uint32_t));
            recv_seq_num = ntohl(recv_seq_num);
            if (recv_seq_num > (u_int32_t)MAX_SEQ_NUM)
            {
                fprintf(stderr, "[Sender] invalid sequence number: %d\n", recv_seq_num);
                continue;
            } else if (LAR < LFS){
                if (recv_seq_num < LAR || recv_seq_num > LFS)
                {
                    fprintf(stderr, "[Sender] ACK sequence number not in window: %d\n", recv_seq_num);
                    continue;
                }
            } else {
                if (recv_seq_num < LAR && recv_seq_num > LFS)
                {
                    fprintf(stderr, "[Sender] ACK sequence number not in window: %d\n", recv_seq_num);
                    continue;
                }
            }

            // ACK received
            fprintf(stdout, "[Sender] ACK received for sequence number %d\n", recv_seq_num);
            if (EOF_flag && recv_seq_num == final_seq_num)
            {
                fprintf(stdout, "[Sender] file transfer complete\n");
                EOT_flag = true;
                break;
            }
            if (LAR <= recv_seq_num) {
                // Clear window
                for (int i = LAR; i <= recv_seq_num; i++)
                {
                    send_buf[i % WINDOW_SIZE].data_len = 0;
                    memset(send_buf[i % WINDOW_SIZE].packet, 0, MAX_PACKET_SIZE);
                }
            } else {
                // Clear window
                for (int i = LAR; i <= LAR + recv_seq_num + 1; i++)
                {
                    send_buf[i % WINDOW_SIZE].data_len = 0;
                    memset(send_buf[i % WINDOW_SIZE].packet, 0, MAX_PACKET_SIZE);
                }
            }
            // Update LAR and LFS
            LAR = (recv_seq_num + 1) % MAX_SEQ_NUM;
            LFS = (recv_seq_num + WINDOW_SIZE) % MAX_SEQ_NUM;
        }

        // Populate window
        for (int i = LAR + 1; i <= LAR + WINDOW_SIZE; i++)
        {
            if (!EOF_flag && send_buf[i % WINDOW_SIZE].data_len == 0)
            {
                // Set packet sequence number
                send_buf[i % WINDOW_SIZE].seq_num = packet_num % MAX_SEQ_NUM;
                packet_num = (packet_num + 1) % MAX_SEQ_NUM;

                // Read file into buffer
                cur_len = fread(read_buf, 1, MAX_DATA_SIZE, fp);
                if (cur_len < MAX_DATA_SIZE || cur_len == 0)
                {
                    EOF_flag = true;
                    final_seq_num = send_buf[i % WINDOW_SIZE].seq_num;
                    fprintf(stdout, "[Sender] EOF reached final sequence num is %d\n", final_seq_num);
                }
                send_buf[i % WINDOW_SIZE].data_len = (uint32_t)cur_len;

                // Set packet sequence number
                temp_seq_num = htonl(send_buf[i % WINDOW_SIZE].seq_num);
                memcpy(send_buf[i % WINDOW_SIZE].packet, &temp_seq_num, sizeof(uint32_t));

                // Set ACK flag to 0x00
                send_buf[i % WINDOW_SIZE].packet[4] = 0x00;

                // Set control flag to 0x00
                send_buf[i % WINDOW_SIZE].packet[5] = 0x00;

                // Set length of data
                temp_data_len = htons(send_buf[i % WINDOW_SIZE].data_len);
                memcpy(send_buf[i % WINDOW_SIZE].packet + 6, &temp_data_len, sizeof(uint16_t));

                // Copy data into packet buffer
                memcpy(send_buf[i % WINDOW_SIZE].packet + HEADER_SIZE, read_buf, cur_len);
                // fprintf(stdout, "data: %s\n", send_buf[i].packet + HEADER_SIZE);
            } else if (((float)(clock() - send_buf[i % WINDOW_SIZE].timestamp))/CLOCKS_PER_SEC < 0.5) {
                break;
            }

            send_buf[i % WINDOW_SIZE].timestamp = clock();

            if (!error(4)) {
                if (sendto(sock_fd, send_buf[i % WINDOW_SIZE].packet, HEADER_SIZE + send_buf[i % WINDOW_SIZE].data_len, 0, addr_ptr->ai_addr, addr_ptr->ai_addrlen) == -1)
                {
                    fprintf(stderr, "[Sender] failed to send packet #%d\n", send_buf[i % WINDOW_SIZE].seq_num);
                }
                fprintf(stdout, "[Sender] sent packet #%d at %f\n", send_buf[i % WINDOW_SIZE].seq_num, (float)send_buf[i % WINDOW_SIZE].timestamp / CLOCKS_PER_SEC);
            } else {
                fprintf(stderr, "[Sender] packet #%d dropped\n", send_buf[i % WINDOW_SIZE].seq_num);
            }
        }
    }

    return final_seq_num;
} // End_send file

int SWPSender::disconnect(uint32_t final_seq_num)
{
    char disc_packet[HEADER_SIZE];

    // Set final sequence number
    uint32_t seq_num = htonl(final_seq_num);
    memcpy(disc_packet, &seq_num, sizeof(uint32_t));

    // Set ACK flag as 0
    disc_packet[4] = 0x00;

    // Set control flag as 0x02
    disc_packet[5] = 0x02;

    // Set length as 0
    disc_packet[6] = 0x00;
    disc_packet[7] = 0x00;

    // Send disconnect request
    char recv_buf[HEADER_SIZE];
    uint32_t recv_seq_num;
    bool acked = false;
    fprintf(stdout, "[Sender] disconnecting from receiver...\n");

    // Retry disconnect MAX_RETRY times
    for (int i = 0; i < MAX_RETRY; i++)
    {
        if (sendto(sock_fd, disc_packet, HEADER_SIZE, 0, addr_ptr->ai_addr, addr_ptr->ai_addrlen) == -1)
        {
            fprintf(stderr, "[Sender] disconnect attempt #%d: sendto error\n", i);
            continue;
        }

        // Wait for response
        if (poll(pfds, 1, 1000) == -1)
        {
            fprintf(stderr, "[Sender] disconnect attempt #%d: poll error\n", i);
            continue;
        }

        if (pfds[0].revents & POLLIN)
        {
            if (recvfrom(sock_fd, recv_buf, HEADER_SIZE, 0, NULL, 0) == -1)
            {
                fprintf(stderr, "[Sender] disconnect attempt #%d: no response\n", i);
                exit(3);
            }

            // Check if final sequence number is valid
            memcpy(&recv_seq_num, recv_buf, sizeof(uint32_t));
            recv_seq_num = ntohl(recv_seq_num);
            if (recv_seq_num != final_seq_num)
            {
                fprintf(stderr, "[Sender] disconnect attempt #%d: invalid final sequence number\n", i);
                exit(3);
            }

            // Check if ACK flag is 0x01
            if (recv_buf[4] != 0x01)
            {
                fprintf(stderr, "[Sender] disconnect attempt #%d: ACK flag invalid\n", i);
                exit(3);
            }

            // Check if control flag is 0x02
            if (recv_buf[5] != 0x02)
            {
                fprintf(stderr, "[Sender] disconnect attempt #%d: control flag invalid\n", i);
                exit(3);
            }

            // Check if data length is 0x0000
            if (recv_buf[6] != 0x00 || recv_buf[7] != 0x00)
            {
                fprintf(stderr, "[Sender] disconnect attempt #%d: invalid data length\n", i);
                exit(3);
            }

            // Disconnect request successful
            acked = true;
            break;
        }
    }

    if (acked)
    {
        fprintf(stdout, "[Sender] successfully disconnected\n");
    }
    else
    {
        fprintf(stderr, "[Sender] failed to disconnect\n");
        exit(-1);
    }
    return 0;
} // End of disconnect
