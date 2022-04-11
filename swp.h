#define UDP_PORT "4950"
#define MAXBUFLEN 100
#define MAX_DATA 1024
#define INIT_SEQ_NUM 0
#define MAX_RETRY 5

// global variables for signal handler
int sockfd;
struct addrinfo *ptr;
