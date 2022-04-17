// swpsender.cpp
// SWP sender class implementation

#include "swp.h"

using namespace std;

typedef struct Packet {
    uint32_t seq_num;
    // bool ack_status;
    uint16_t data_len;
    char packet[MAX_PACKET_SIZE];
    clock_t timestamp;
} Packet;

// Constructor
SWPSender::SWPSender(){
    // Initialize UDP variables
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket

    packet_num = 0;
    LAR = 0;
    LFS = 0;
    // windowSize = WINDOW_SIZE;

    // Initialize the send buffer
    for (int i = 0; i < WINDOW_SIZE; i++){
        // send_buf[i].ack_status = false;
        send_buf[i].data_len = 0;
        send_buf[i].timestamp = 0;
    }
}

// Connect
int SWPSender::connect(char *hostname) {
    struct addrinfo *sender_info;

    // Get sender's address info
    if ((status = getaddrinfo(hostname, UDP_PORT, &hints, &sender_info)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
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
    bool connected = false;
    fprintf(stdout, "[Sender] establishing connection with receiver...\n");

    // Retry connection MAX_RETRY times
    for (int i = 0; i < MAX_RETRY; i++) { 
        if (sendto(sock_fd, vcon_packet, 8, 0, addr_ptr->ai_addr, addr_ptr->ai_addrlen) == -1) {
            fprintf(stderr, "[Sender] connection attempt #%d: sendto error\n", i);
            continue;
        }

        // Wait for response
        if (poll(pfds, 1, 1000) == -1) {
            fprintf(stderr, "[Sender] connection attempt #%d: poll error\n", i);
            continue;
        }

        if (pfds[0].revents & POLLIN) {
            if (recvfrom(sock_fd, recv_buf, 8, 0, NULL, 0) == -1) {
                fprintf(stderr, "[Sender] connection attempt #%d: no response\n", i);
                exit(3);
            }

            // Check if initial sequence number is valid
            uint32_t recv_seq_num;
            memcpy(&recv_seq_num, recv_buf, sizeof(uint32_t));
            if (ntohl(recv_seq_num) != INIT_SEQ_NUM) {
                fprintf(stderr, "[Sender] connection attempt #%d: invalid initial sequence number\n", i);
                exit(3);
            }

            // Check if ACK flag is 0x01
            if (recv_buf[4] != 0x01) {
                fprintf(stderr, "[Sender] connection attempt #%d: ACK flag invalid\n", i);
                exit(3);
            }

            // Check if control flag is 0x01
            if (recv_buf[5] != 0x01) {
                fprintf(stderr, "[Sender] Connection attempt #%d: control flag invalid\n", i);
                exit(3);
            }

            // Check if data length is 0x0000
            //if ((recv_buf[6] > 0x01 && recv_buf[7]  0x00)|| recv_buf[6] != 0x00 || recv_buf[7] != 0x00) {
            if (recv_buf[6] != 0x00 || recv_buf[7] != 0x00) {
                fprintf(stderr, "[Sender] Connection attempt #%d: invalid data length\n", i);
                exit(3);
            }

            // Connection request successful
            connected = true;
            break;
        }
    }

    if (connected) {
        char recv_ip[INET6_ADDRSTRLEN];
        // get server IP address
        if (inet_ntop(addr_ptr->ai_family, &((struct sockaddr_in *)addr_ptr->ai_addr)->sin_addr, recv_ip, INET6_ADDRSTRLEN) == NULL)
        {
            perror("[Client] inet_ntop");
            exit(1);
        }
        fprintf(stdout, "[Sender] virtual connection established with %s!\n", recv_ip);
    } else {
        fprintf(stderr, "[Sender] failed to establish virtual connection\n");
        exit(1);
    }
}

int SWPSender::send_file(char *filename) {

    // Check file exists
    if (access(filename, F_OK) == -1) {
        fprintf(stderr, "[Sender] file %s does not exist\n", filename);
        exit(1);
    }

    // Open file
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "[Sender] failed to open file %s\n", filename);
        exit(1);
    }

    bool EOF_flag = false;
    int cur_len;
    char *read_buf[MAX_DATA_SIZE];
    uint32_t temp_seq_num;
    uint16_t temp_data_len;

    while (!EOF_flag) {
        // Check for ACKs
        if (poll(pfds, 1, -1) == -1) {
            fprintf(stderr, "[Sender] failed to poll\n");
            exit(1);
        }

        if (pfds[0].revents & POLLIN) {
            // Receive ACK
            char recv_buf[8];
            if (recvfrom(sock_fd, recv_buf, 8, 0, NULL, 0) == -1) {
                fprintf(stderr, "[Sender] failed to receive ACK\n");
            }

            // Check if ACK flag is set
            else if (recv_buf[4] != 0x01) {
                fprintf(stderr, "[Sender] ACK flag not set\n");
            }

            // Check if connection setup flag is valid
            else if (recv_buf[5] != 0x01) {
                fprintf(stderr, "[Sender] connection setup flag not valid\n");
            }

            // Check if length is valid
            //if ((recv_buf[6] > 0x01 && recv_buf[7]  0x00)|| recv_buf[6] != 0x00 || recv_buf[7] != 0x00) {
            else if (recv_buf[6] != 0x00 || recv_buf[7] != 0x00) {
                fprintf(stderr, "[Sender] invalid length\n");
            }

            else {
                // Check if sequence number is valid
                uint32_t recv_seq_num;
                memcpy(&recv_seq_num, recv_buf, sizeof(uint32_t));
                recv_seq_num = ntohl(recv_seq_num);
                if (recv_seq_num > MAX_SEQ_NUM || recv_seq_num <= LAR || recv_seq_num > LFS) {
                    fprintf(stderr, "[Sender] invalid sequence number\n");
                } else {
                    // ACK received
                    fprintf(stdout, "[Sender] ACK received for sequence number %d\n", recv_seq_num);
                    for (int i = LAR + 1; i <= recv_seq_num; i++) {
                        send_buf[i].data_len = 0;
                        memset(send_buf[i].packet, 0, MAX_PACKET_SIZE);
                    }
                    LAR = recv_seq_num;
                    LFS = (recv_seq_num + WINDOW_SIZE) % MAX_SEQ_NUM;
                }
            }
        }

        // Populate buffer
        for (int i = LAR + 1; i <= LAR + WINDOW_SIZE; i++) {
            if (send_buf[i].data_len == 0) {
                // Set packet sequence number
                send_buf[i].seq_num = packet_num % MAX_SEQ_NUM;
                packet_num++;   
                
                // Read file into buffer
                cur_len = fread(read_buf, 1, MAX_DATA_SIZE, fp);
                if (cur_len < MAX_DATA_SIZE || cur_len == 0) {
                    EOF_flag = true;
                }
                send_buf[i].data_len = (uint32_t)cur_len;

                // Set packet sequence number
                temp_seq_num = htonl(send_buf[i].seq_num);
                memcpy(send_buf[i].packet, &temp_seq_num, sizeof(uint32_t));

                // Set ACK flag to 0x00
                send_buf[i].packet[4] = 0x00;

                // Set control flag to 0x00
                send_buf[i].packet[5] = 0x00;

                // Set length of data
                temp_data_len = htons(send_buf[i].data_len);
                memcpy(send_buf[i].packet + 6, &temp_data_len, sizeof(uint16_t));

                // Copy data into packet buffer
                memcpy(send_buf[i].packet + 8, read_buf, cur_len);
            } 
            
            if (sendto(sock_fd, send_buf[i].packet, HEADER_SIZE + send_buf[i].data_len, 0, addr_ptr->ai_addr, addr_ptr->ai_addrlen) == -1) {
                fprintf(stderr, "[Sender] failed to send packet #%d\n", send_buf[i].seq_num);
            }
            fprintf(stdout, "[Sender] sent packet #%d\n", send_buf[i].seq_num);

            if (EOF_flag) {
                break;
            }
        }
    }




}//end_send file


int SWPSender::disconnect(uint32_t final_seq_num, char *filename) {
    // Establish virtual connection
    char disc_buf[8]; //8 bytes bc sequnum is 4, ack is 1, control is 1, and length is 2

    // Set initial sequence number
    uint32_t seq_num = htonl((uint32_t)final_seq_num);
    memcpy(disc_buf, &seq_num, sizeof(uint32_t)); 

    // Set ACK flag as 0
    disc_buf[4] = 0x00;

    // Set control flag as 0x02
    disc_buf[5] = 0x02;

    // Set length as 0
    disc_buf[6] = 0x00;
    disc_buf[8] = 0x00;

    // Send connection request
    char recv_buf[8];
    bool acked = false;
    fprintf(stdout, "[Sender] Establishing connection with receiver...\n");

    // Retry connection a set number of times
    for (int i = 0; i < MAX_RETRY; i++) { 
        if (sendto(sock_fd, disc_buf, 8, 0, addr_ptr->ai_addr, addr_ptr->ai_addrlen) == -1) {
            fprintf(stderr, "[Sender] Connection attempt #%d: failed to send initial connection request\n", i);
            continue;
        }

        // Wait for response
        if (poll(pfds, 1, 1000) == -1) {
            fprintf(stderr, "[Sender] Connection attempt #%d: failed to poll\n", i);
            continue;
        }

        if (pfds[0].revents & POLLIN) {
            if (recvfrom(sock_fd, recv_buf, 8, 0, NULL, 0) == -1) {
                fprintf(stderr, "[Sender] Connection attempt #%d: failed to recieve initial connection response\n", i);
                continue;
            }

            // Check if initial sequence number is correct
            uint32_t recv_seq_num;
            memcpy(&recv_seq_num, recv_buf, sizeof(uint32_t));
            if (ntohl(recv_seq_num) != final_seq_num) {
                fprintf(stderr, "[Sender] Connection attempt #%d: invalid initial sequence number\n", i);
                continue;
            }

            // Check if ACK flag is set
            if (recv_buf[4] != 0x01) {
                fprintf(stderr, "[Sender] Connection attempt #%d: ACK flag not set\n", i);
                continue;
            }

            // Check if control flag is valid
            if (recv_buf[5] != 0x02) {
                fprintf(stderr, "[Sender] Connection attempt #%d: connection setup flag not valid\n", i);
                continue;
            }

            // Check if length is valid
            //if ((recv_buf[6] > 0x01 && recv_buf[7]  0x00)|| recv_buf[6] != 0x00 || recv_buf[7] != 0x00) {
            if (recv_buf[6] != 0x00 || recv_buf[7] != 0x00) {
                fprintf(stderr, "[Sender] Connection attempt #%d: invalid length\n", i);
                continue;
            }

            // Connection request successful
            acked = true;
            break;
        }

    //same code as connect, but we are just sending diffrent info

}//end of disconnect







// int main() {

    

    

    
    
// }

