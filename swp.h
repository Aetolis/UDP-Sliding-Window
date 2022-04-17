// swp.h
// Class definition for SWP

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>

#define UDP_PORT "4950"
#define HEADER_SIZE 8
#define MAX_DATA_SIZE 1024
#define MAX_PACKET_SIZE 1032
#define INIT_SEQ_NUM 0
#define MAX_SEQ_NUM 50 
#define WINDOW_SIZE 25 //this can be half the size of seqnum, which can itself use 4 bytes 
#define MAX_RETRY 5

typedef struct Packet {
    uint32_t seq_num;
    // bool ack_status;
    uint16_t data_len;
    char packet[MAX_PACKET_SIZE];
    clock_t timestamp;
} Packet;

class SWPSender { 
    public:
        int connect(char *hostname);
        int send_file(char *filename);
        int disconnect(uint32_t seq_num, char *filename);

    private:
        // UDP variables
        int sock_fd;
        int status;
        int numbytes;
        struct addrinfo hints, *addr_ptr;
        struct pollfd pfds[1]; //data structure describing a polling request

        // SWP variables
        uint32_t packet_num;
        int LAR; //last ack recived
        int LFS; //last frame sent
        int windowSize; 

        // Buffer variables
        Packet send_buf[WINDOW_SIZE];
};

class SWPReceiver {
    public:
        //public methods
        int setup();
        int receive_file(char *filename);
        int disconnect();

    private:
        // UDP variables
        int sock_fd;
        int status;
        int numbytes;
        struct addrinfo hints, *addr_ptr;
        struct pollfd pfds[1];
        struct sockaddr_storage sender_addr;
        socklen_t sender_addr_len;
        char sender_ip[INET6_ADDRSTRLEN];


        // SWP variables
        uint32_t init_seq_num;
        int LAF; //largest acceptable frame
        int LFR; //last frame reeived
        int NFE; //next frame expected
        int windowSize; 

        // Buffer variables

};


