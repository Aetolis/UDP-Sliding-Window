#define UDP_PORT "4950"
#define MAX_DATA_LEN 1024
#define INIT_SEQ_NUM 0
#define MAX_SEQ_NUM 50 
#define WINDOW_SIZE 25 //this can be half the size of seqnum, which can itself use 4 bytes 
#define MAX_RETRY 5

// global variables for signal handler
int sockfd;
struct addrinfo *ptr;
