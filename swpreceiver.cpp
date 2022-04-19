// swpreceiver.cpp
// SWP Receiver class implementation

#include "swp.h"

using namespace std;

// Constructor
SWPReceiver::SWPReceiver() {
    // Initialize UDP variables
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    sender_addr_len = sizeof(sender_addr);

    // SWP variables
    NFE = INIT_SEQ_NUM;
    LAF = (INIT_SEQ_NUM + WINDOW_SIZE - 1) % MAX_SEQ_NUM;
    windowSize = 0;

    // Buffer variables
    for (int i = 0; i < WINDOW_SIZE; i++)
    {
        window_buf[i].data_len = 0;
    }
}

// Destructor
SWPReceiver::~SWPReceiver() {
    // Close socket
    close(sock_fd);
}


int SWPReceiver::setup(){
    // struct sockaddr_storage recv_addr;
    // socklen_t recv_addr_len = sizeof(recv_addr);

    struct addrinfo *recv_info;

    if ((status = getaddrinfo(NULL, UDP_PORT, &hints, &recv_info)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }

    // Iterate over linked-list of addrinfo structs returned by getadderinfo
    for (addr_ptr = recv_info; addr_ptr != NULL; addr_ptr = addr_ptr->ai_next)
    {
        // create socket
        if ((sock_fd = socket(addr_ptr->ai_family, addr_ptr->ai_socktype, addr_ptr->ai_protocol)) == -1)
        {
            continue;
        }
        if (bind(sock_fd, recv_info->ai_addr, recv_info->ai_addrlen) == 0)
        {
            break;
        }
    } // End of create socket

    // Check if socket was created
    if (addr_ptr == NULL)
    {
        perror("[Receiver] could not bind");
        exit(1);
    }

    // Get receiver IP address
    char recv_ip_str[INET6_ADDRSTRLEN];
    if (inet_ntop(addr_ptr->ai_family, &((struct sockaddr_in *)addr_ptr->ai_addr)->sin_addr, recv_ip_str, INET6_ADDRSTRLEN) == NULL)
    {
        perror("[Receiver] inet_ntop");
        exit(1);
    }
    fprintf(stdout, "[Receiver] waiting for connection on %s...\n", recv_ip_str);

    // Set up pollfd struct
    pfds[0].fd = sock_fd;
    pfds[0].events = POLLIN;

    freeaddrinfo(recv_info);

    // Establish virtual connection with sender
    char recv_buf[HEADER_SIZE];
    bool connected = false;
    char vcon_packet[HEADER_SIZE];
    uint32_t recv_seq_num;

    // Retry receive MAX_RETRY times
    for (int i = 0; i < MAX_RETRY; i++) {
        // Wait for connection request
        if (poll(pfds, 1, 1000) == -1) {
            fprintf(stderr, "[Receiver] connection attempt #%d: poll error\n", i);
            continue;
        }

        if (pfds[0].revents & POLLIN) {
            if ((numbytes = recvfrom(sock_fd, recv_buf, 8, 0, (struct sockaddr *)&sender_addr, &sender_addr_len)) == -1)
            {
                fprintf(stderr, "[Receiver] connection attempt #%d: recvfrom error\n", i);
                continue;
            }

            // Check if ACK flag is 0x00
            if (recv_buf[4] != 0x00) {
                fprintf(stderr, "[Receiver] connection attempt #%d: ACK flag invalid\n", i);
                continue;
            }

            // Check if control flag is 0x01
            if (recv_buf[5] != 0x01) {
                fprintf(stderr, "[Receiver] connection attempt #%d: control flag invalid\n", i);
                continue;
            }

            // Check if data length is 0
            if (recv_buf[6] != 0x00 || recv_buf[7] != 0x00) {
                fprintf(stderr, "[Receiver] connection attempt #%d: data length invalid\n", i);
                continue;
            }

            // Set initial sequence number
            memcpy(&recv_seq_num, recv_buf, sizeof(uint32_t));
            seq_num = ntohl(recv_seq_num);
            memcpy(vcon_packet, &recv_seq_num, sizeof(uint32_t));

            // Set ACK flag as 0x01
            vcon_packet[4] = 0x01;

            // Set control flag as 0x01
            vcon_packet[5] = 0x01;

            // Set data length as 0
            vcon_packet[6] = 0x00;
            vcon_packet[7] = 0x00;

            if (!error(2)) {
                if (sendto(sock_fd, vcon_packet, 8, 0, (struct sockaddr *)&sender_addr, sender_addr_len) == -1) {
                    fprintf(stderr, "[Receiver] connection attempt #%d: sendto error\n", i);
                    continue;
                }

                // Connection established
                connected = true;
                break;
            } else {
                fprintf(stderr, "[Receiver] connection attempt #%d: packet lost\n", i);
            }
        }
    }

    if (connected) {
        // convert client's address to a string
        if (inet_ntop(sender_addr.ss_family, &(((struct sockaddr_in *)&sender_addr)->sin_addr), sender_ip, INET6_ADDRSTRLEN) == NULL)
        {
            perror("[Receiver] inet_ntop error");
            exit(2);
        }
        printf("[Receiver] connection established with %s\n", sender_ip);
    } else {
        fprintf(stderr, "[Receiver] failed to establish virtual connection with sender\n");
        exit(2);
    }

    fprintf(stdout, "[Receiver] initial sequence number: %d\n", seq_num);

    return 0;
} // End setup()


int SWPReceiver::receive_file(char *filename){
    // Open file for output
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        fprintf(stderr, "[Sender] failed to open \"%s\"\n", filename);
        exit(1);
    }

    bool EOF_flag = false;
    bool EOT_flag = false;
    char recv_buf[MAX_PACKET_SIZE];
    char ack_buf[HEADER_SIZE];
    uint32_t recv_seq_num;
    uint32_t ack_seq_num;
    uint32_t final_seq_num;

    while (!EOT_flag) {
        // Wait for data packet
        if (poll(pfds, 1, -1) == -1) {
            fprintf(stderr, "[Receiver] poll error\n");
            exit(1);
        }

        // Receive data packet from sender
        if (pfds[0].revents & POLLIN) {
            if (recvfrom(sock_fd, recv_buf, MAX_PACKET_SIZE, 0, (struct sockaddr *)&sender_addr, &sender_addr_len) == -1)
            {
                fprintf(stderr, "[Receiver] recvfrom error\n");
                continue;
            }

            // Check if ACK flag is 0x00
            if (recv_buf[4] != 0x00) {
                fprintf(stderr, "[Receiver] ACK flag invalid\n");
                continue;
            }

            // Check if control flag is disconnect packet
            if (recv_buf[5] == 0x02) {
                // Set EOF flag
                EOF_flag = true;
                fprintf(stdout, "[Receiver] received disconnect packet\n");
            }
            // Check if control flag is valid
            else if (recv_buf[5] != 0x00) {
                fprintf(stderr, "[Receiver] control flag invalid\n");
                continue;
            }

            // Check if sequence number is valid
            memcpy(&recv_seq_num, recv_buf, sizeof(uint32_t));
            recv_seq_num = ntohl(recv_seq_num);
            // Set final_seq_num if EOF flag is set
            if (EOF_flag) {
                final_seq_num = recv_seq_num;
                fprintf(stdout, "[Receiver] final sequence number: %d\n", final_seq_num);
            } else if (recv_seq_num > (u_int32_t)MAX_SEQ_NUM) {
                fprintf(stderr, "[Receiver] invalid sequence number: %d\n", recv_seq_num);
                continue;
            } else if (NFE < LAF) {
                if (recv_seq_num < NFE || recv_seq_num > LAF) {
                    fprintf(stderr, "[Receiver] sequence number not in window: %d\n", recv_seq_num);
                    continue;
                }
            } else {
                if (recv_seq_num < NFE && recv_seq_num > LAF) {
                    fprintf(stderr, "[Receiver] sequence number not in window: %d\n", recv_seq_num);
                    continue;
                }
            }

            // Check data length
            if (EOF_flag) {
                if (recv_buf[6] != 0x00 || recv_buf[7] != 0x00) {
                    fprintf(stderr, "[Receiver] data length invalid\n");
                    continue;
                }
            } else if (window_buf[recv_seq_num % WINDOW_SIZE].data_len != 0) {
                fprintf(stderr, "[Receiver] duplicate packet: %d\n", recv_seq_num);
                continue;
            } else {
                memcpy(&window_buf[recv_seq_num % WINDOW_SIZE].data_len, recv_buf + 6, sizeof(uint16_t));
                window_buf[recv_seq_num % WINDOW_SIZE].data_len = ntohs(window_buf[recv_seq_num % WINDOW_SIZE].data_len);
                if (window_buf[recv_seq_num % WINDOW_SIZE].data_len >(u_int16_t)MAX_DATA_SIZE) {
                    fprintf(stderr, "[Receiver] data length invalid\n");
                    continue;
                }
            }

            // Read packet data to window buffer
            if (!EOF_flag) {
                memcpy(window_buf[recv_seq_num % WINDOW_SIZE].data, recv_buf + 8, window_buf[recv_seq_num % WINDOW_SIZE].data_len);
                printf("[Receiver] received data packet #%d with len %d\n", recv_seq_num, window_buf[recv_seq_num % WINDOW_SIZE].data_len);
            }
        }

        // Determine packet to ACK
        if (window_buf[NFE % WINDOW_SIZE].data_len == 0) {
            // Check if EOF flag is set
            if (EOF_flag && NFE - 1 == final_seq_num) {
                // Set sequence number
                    ack_seq_num = htonl((uint32_t)((NFE - 1) % MAX_SEQ_NUM));
                    memcpy(&ack_buf, &ack_seq_num, sizeof(uint32_t));

                    // Set ACK flag as 0x01
                    ack_buf[4] = 0x01;

                    // Set control flag as 0x02
                    ack_buf[5] = 0x02;

                    // Set data length as 0
                    ack_buf[6] = 0x00;
                    ack_buf[7] = 0x00;

                    if (!error(4)) {
                        // Send ACK packet
                        if (sendto(sock_fd, ack_buf, 8, 0, (struct sockaddr *)&sender_addr, sender_addr_len) == -1) {
                            fprintf(stderr, "[Receiver] sendto error\n");
                            exit(1);
                        }
                        fprintf(stdout, "[Receiver] ACK sent: %d\n", (NFE - 1) % MAX_SEQ_NUM);
                    } else {
                        fprintf(stderr, "[Sender] ACK packet #%d dropped\n", (NFE - 1) % MAX_SEQ_NUM);
                    }

                    // Update LFR and LAF
                    NFE = NFE % MAX_SEQ_NUM;
                    LAF = (NFE - 1 + WINDOW_SIZE) % MAX_SEQ_NUM;
                    fprintf(stdout, "[Receiver] NFE: %d\n", NFE);
                    fprintf(stdout, "[Receiver] LAF: %d\n", LAF);

                    // Set EOT flag
                    EOT_flag = true;
                    break;
            }
            fprintf(stdout, "[Receiver] no data packet to ACK: NFE %d\n", NFE);
            continue;
        } else {
            for (int i = NFE; i <= NFE + WINDOW_SIZE; i++) {
                // Write data to file
                // fprintf(stdout, "data: %s\n", window_buf[i % WINDOW_SIZE].data);
                if (fwrite(window_buf[i % WINDOW_SIZE].data, sizeof(char), window_buf[i % WINDOW_SIZE].data_len, fp) != window_buf[i % WINDOW_SIZE].data_len) {
                    fprintf(stderr, "[Receiver] fwrite error\n");
                    exit(1);
                }

                // Reset data length
                window_buf[i % WINDOW_SIZE].data_len = 0;

                // Reset data
                memset(window_buf[i % WINDOW_SIZE].data, 0, MAX_DATA_SIZE);

                // Construct ack packet
                if (window_buf[(i+1) % WINDOW_SIZE].data_len == 0) {
                    // Set sequence number
                    ack_seq_num = htonl((uint32_t)i % MAX_SEQ_NUM);
                    memcpy(&ack_buf, &ack_seq_num, sizeof(uint32_t));

                    // Set ACK flag as 0x01
                    ack_buf[4] = 0x01;

                    // Set control flag as 0x00
                    ack_buf[5] = 0x00;

                    // Set data length as 0
                    ack_buf[6] = 0x00;
                    ack_buf[7] = 0x00;

                    // Send ACK packet
                    if (!error(4)) {
                        if (sendto(sock_fd, ack_buf, 8, 0, (struct sockaddr *)&sender_addr, sender_addr_len) == -1) {
                            fprintf(stderr, "[Receiver] sendto error\n");
                            exit(1);
                        }
                        fprintf(stdout, "[Receiver] ACK sent: %d\n", i % MAX_SEQ_NUM);
                    } else {
                        fprintf(stderr, "[Sender] ACK packet #%d dropped\n", (NFE - 1) % MAX_SEQ_NUM);
                    }

                    // Update LFR and LAF
                    NFE = (i + 1) % MAX_SEQ_NUM;
                    LAF = (i + WINDOW_SIZE) % MAX_SEQ_NUM;
                    fprintf(stdout, "[Receiver] NFE: %d\n", NFE);
                    fprintf(stdout, "[Receiver] LAF: %d\n", LAF);
                    break;
                }
            }
        }
    }

    // Close file
    fclose(fp);

    return 0;
} // End of receive_file

