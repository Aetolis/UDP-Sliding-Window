// Sliding Window Protocol sender
#include <swp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>

using namespace std;

struct packet {
    uint32_t seq_num;
    bool ack_status;
    uint16_t data_len;
    // timer
    char data[MAX_DATA_SIZE];
};


class SWPSender{ 
    public:
        int connect(char *hostname);
        int send_file(char *filename);
        int disconnect();

    private:
        // UDP variables
        int sock_fd;
        int status;
        int numbytes;
        struct addrinfo hints, *addr_ptr;
        struct pollfd pfds[2]; //data structure describing a polling request

        // SWP variables
        int LAR; //last ack recived
        int LFS; //last frame sent
        int windowSize; 

        // Buffer variables
        char *send_buf[WINDOW_SIZE];

        // Helper functions



};

// Constructor

// Connect
int SWPSender::connect(char *hostname) {
    struct addrinfo *client_info;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket

    if ((status = getaddrinfo(hostname, UDP_PORT, &hints, &client_info)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }

    // loop through all the results and make a socket
    for (addr_ptr = client_info; addr_ptr != NULL; addr_ptr = addr_ptr->ai_next)
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

    // setup poll
    pfds[0].fd = 0; //this is being changed to another fd
    pfds[0].events = POLLIN;
    pfds[1].fd = sock_fd;
    pfds[1].events = POLLIN;

    freeaddrinfo(client_info);

    // Establish virtual connection
    char con_buf[8]; //8 bytes bc sequnum is 4, ack is 1, control is 1, and length is 2

    // Set initial sequence number
    uint32_t seq_num = htonl((uint32_t)INIT_SEQ_NUM);
    memcpy(con_buf, &seq_num, sizeof(uint32_t)); 

    // Set ACK flag as 0
    con_buf[4] = 0x00;

    // Set Connection setup flag as 1
    con_buf[5] = 0x01;

    // Set length as 0
    con_buf[6] = 0x00;
    con_buf[8] = 0x00;

    // Send connection request
    char recv_buf[8];
    bool connected = false;
    fprintf(stdout, "[Sender] Establishing connection with receiver...\n");

    // Retry connection a set number of times
    for (int i = 0; i < MAX_RETRY; i++) { 
        if (sendto(sockfd, con_buf, 8, 0, addr_ptr->ai_addr, addr_ptr->ai_addrlen) == -1) {
            fprintf(stderr, "[Sender] Connection attempt #%d: failed to send initial connection request\n", i);
            continue;
        }

        // Wait for response
        if (poll(pfds, 2, 1000) == -1) {
            fprintf(stderr, "[Sender] Connection attempt #%d: failed to poll\n", i);
            continue;
        }

        if (pfds[1].revents & POLLIN) {
            if (recvfrom(sock_fd, recv_buf, 8, 0, NULL, 0) == -1) {
                fprintf(stderr, "[Sender] Connection attempt #%d: failed to recieve initial connection response\n", i);
                continue;
            }

            // Check if initial sequence number is correct
            uint32_t recv_seq_num;
            memcpy(&recv_seq_num, recv_buf, sizeof(uint32_t));
            if (ntohl(recv_seq_num) != INIT_SEQ_NUM) {
                fprintf(stderr, "[Sender] Connection attempt #%d: invalid initial sequence number\n", i);
                continue;
            }

            // Check if ACK flag is set
            if (recv_buf[4] != 0x01) {
                fprintf(stderr, "[Sender] Connection attempt #%d: ACK flag not set\n", i);
                continue;
            }

            // Check if connection setup flag is valid
            if (recv_buf[5] != 0x01) {
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
            connected = true;
            break;
        }
    }

    if (!connected) {
        fprintf(stderr, "[Sender] failed to establish connection\n");
        exit(1);
    } else {
        char server_ip[INET6_ADDRSTRLEN];
        // get server IP address
        if (inet_ntop(addr_ptr->ai_family, &((struct sockaddr_in *)addr_ptr->ai_addr)->sin_addr, server_ip, INET6_ADDRSTRLEN) == NULL)
        {
            perror("[Client] inet_ntop");
            exit(1);
        }
        fprintf(stdout, "[Sender] connection established with %s!\n", server_ip);
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

    // Get file size
    fseek(fp, 0, SEEK_END);
    int file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    //initilize buffer / turn file into an array of structs
    //while loop until file ends



    //begin sending, but also we need to check ttl as we are sending 

    //do we need to tell reciver that we are done sending


}//end_send file


int SWPSender::disconnect(char *filename) {

    //same code as connect, but we are just sending diffrent info

}//end of disconnect







int main() {

    

    

    
    
}

