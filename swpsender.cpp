// Sliding Window Protocol
#include <Shared.h>

#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>

using namespace std;


class SWPSender {
    public:
        int connect(char *hostname);
        int send_file(char *filename);

    private:
        // UDP variables
        int sockfd;
        int status;
        struct addrinfo hints, *addr_ptr;
        struct pollfd pfds[2];

        // SWP variables
        int LAR;
        int LFS;
        int windowSize;

        // Buffer variables



};

// Constructor

// Connect
int SWPSender::connect(char *hostname) {
    struct addrinfo *server_info;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket

    if ((status = getaddrinfo(hostname, SERVER_PORT, &hints, &server_info)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }

    // loop through all the results and make a socket
    for (addr_ptr = server_info; addr_ptr != NULL; addr_ptr = addr_ptr->ai_next)
    {
        sockfd = socket(addr_ptr->ai_family, addr_ptr->ai_socktype, addr_ptr->ai_protocol);
        if (sockfd != -1)
        {
            break;
        }
    }

    if (addr_ptr == NULL)
    {
        fprintf(stderr, "[Client] failed to create socket\n");
        exit(2);
    }

    // setup poll
    pfds[0].fd = 0;
    pfds[0].events = POLLIN;
    pfds[1].fd = sockfd;
    pfds[1].events = POLLIN;

    freeaddrinfo(server_info);

    char con_buf[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};

    // uint32_t seq_num = htons((uint32_t)0);

    // memcpy(con_buf, &seq_num, sizeof(uint32_t));

    // memcpy(con_buf, 0x00000000, sizeof(uint32_t));
    // memcpy(con_buf + sizeof(uint32_t), , 1);
    // memcpy(con_buf + sizeof(uint32_t) + 1, , 1);

    // Retry connection a set number of times
    
    


}

int main() {

    

    

    
    
}

