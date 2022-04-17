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
    LAF = 0;
    LFR = 0;
    NFE = 0;
    windowSize = 0;
}


int SWPReceiver::setup(){
    struct sockaddr_storage recv_addr;
    socklen_t recv_addr_len = sizeof(recv_addr);

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
    }// End of create socket

    // Check if socket was created
    if (addr_ptr == NULL)
    {
        perror("[Receiver] could not bind");
        exit(1);
    }

    // Set up pollfd struct
    pfds[0].fd = sock_fd;
    pfds[0].events = POLLIN;

    // Get receiver IP address
    char recv_ip_str[INET6_ADDRSTRLEN];
    if (inet_ntop(addr_ptr->ai_family, &((struct sockaddr_in *)addr_ptr->ai_addr)->sin_addr, recv_ip_str, INET6_ADDRSTRLEN) == NULL)
    {
        perror("[Receiver] inet_ntop");
        exit(1);
    }
    printf("[Receiver] waiting for connection on %s...\n", recv_ip_str);

    freeaddrinfo(recv_info);

    // Establish virtual connection with sender
    char recv_buf[HEADER_SIZE];
    bool connected = false;

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

            // Send ACK packet to sender
            char vcon_packet[HEADER_SIZE];

            // Set initial sequence number
            uint32_t recv_seq_num;
            memcpy(&recv_seq_num, recv_buf, sizeof(uint32_t));
            init_seq_num = ntohl(recv_seq_num);
            memcpy(vcon_packet, &init_seq_num, sizeof(uint32_t));

            // Set ACK flag as 0x01
            vcon_packet[4] = 0x01;

            // Set control flag as 0x01
            vcon_packet[5] = 0x01;

            // Set data length as 0
            vcon_packet[6] = 0x00;
            vcon_packet[7] = 0x00;

            if (sendto(sock_fd, vcon_packet, 8, 0, addr_ptr->ai_addr, addr_ptr->ai_addrlen) == -1) {
                fprintf(stderr, "[Sender] connection attempt #%d: sendto error\n", i);
                continue;
            }

            // Connection established
            connected = true;
            break;

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
}// End setup()


int SWPReceiver::receive_file(char *filename){

    //open up a file to dump info to
    // fp = fopen ("server_log.txt", "a"); //a for append, w for write which will overwrite
    // fprintf(fp,"==================================================================\n");
    //we need to astablish some kind of buffer that acts as a slidding frame

    //I think its an endless while loop, but we might use poll
    //we only write to the file when the window shifts, next expected
    //we need some way of sending acks and keeping track of the frames

    //



}//end of send_file




int SWPReceiver::disconnect(){
    

    printf("[Receiver] closing socket...\n");

    //close log file and socket pointer
    // fclose (fp);
    // close(sock_fd);

}//end of disconnect



//read from file
//set up own port/strucks
//make connection

//recive inital set up from server

//send information

//send ack

//send packet
