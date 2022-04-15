#define UDP_PORT "4950"
#define HEADER_SIZE 8
#define MAX_DATA_SIZE 1024
#define MAX_PACKET_SIZE 1032
#define INIT_SEQ_NUM 0
#define MAX_SEQ_NUM 50 
#define WINDOW_SIZE 25 //this can be half the size of seqnum, which can itself use 4 bytes 
#define MAX_RETRY 5

// global variables for signal handler
int sockfd;
struct addrinfo *ptr;
