#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>

#define SERVER_PORT "4950"
#define MAXBUFLEN 100
#define MAX_DATA 1024

// global variables for signal handler
int sockfd;
struct addrinfo *ptr;
