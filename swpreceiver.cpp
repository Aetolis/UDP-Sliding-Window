// Sliding Window Protocol receiver
#include <swp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>

using namespace std;

class SWPReceiver { //why are you not using cammel case my dude?
    public:
        //public methods
        int wait_for_connection();
        int receive_file(char *filename);
        int disconnect();

    private:
        // UDP variables
        int sockfd;
        int status;
        int numbytes;
        struct addrinfo hints, *addr_ptr;
        struct pollfd pfds[2]; //data structure describing a polling request

        // SWP variables
        int LAF; //largest acceptable frame
        int LFR; //last frame reeived
        int NFE; //next frame expected
        int windowSize; 

        // Buffer variables



};

//I dont think I need this struct but I will see as I go

//go in and trim values that you do not need
#define MYPORT "4950" // the port users will be connecting to
#define MAXBUFLEN 10000 //should this value be changed from 10000 given that we are just using one client
//#define MAXCLIENTS 2
//#define BACKLOG 10
struct Client
{
    char username[MAXBUFLEN];
    struct sockaddr_storage client_addr;
    socklen_t addr_len;
    char client_ip_string[INET6_ADDRSTRLEN];
};





int SWPReceiver::wait_for_connection(){
    //very similar to project 2 

    //set up buffers to store info about client
    char s[INET6_ADDRSTRLEN];

    struct Client client;//changed from clients[MAXCLIENTS]

    struct sockaddr_storage recv_addr;
    socklen_t recv_addr_len = sizeof recv_addr;//size of does not have brakets?
    char recv_ip_str[INET6_ADDRSTRLEN];

    struct addrinfo hints, *server_info, *ptr;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    // it is NULL because we don't need to manually put in host name
    if ((status = getaddrinfo(NULL, MYPORT, &hints, &server_info)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }

   // create a socket by going through the list of address structers that getadderinfo returns
    for (ptr = server_info; ptr != NULL; ptr = ptr->ai_next)
    {
        // create socket
        if ((sockfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == -1)
        {
            continue;
        }
        if (bind(sockfd, server_info->ai_addr, server_info->ai_addrlen) == 0)
        {
            break;
        }
    }//end of create a socket

    if (ptr == NULL)//check to see if bind was successful 
    {
        perror("[Server] could not bind");
        exit(2);
    }


    // print server IP address
    if (inet_ntop(ptr->ai_family, &((struct sockaddr_in *)ptr->ai_addr)->sin_addr, s, INET6_ADDRSTRLEN) == NULL)
    {
        perror("[receiver] inet_ntop");
        exit(3);
    }
    printf("[receiver] waiting for connections on %s...\n", s);
    fprintf(fp, "[receiver] waiting for connections on %s...\n", s);

    freeaddrinfo(server_info);

    //This is where the code gets weird compared to the last project

    client.addr_len = sizeof client.client_addr;
        if ((numbytes = recvfrom(sockfd, client.username, MAXBUFLEN - 1, 0, (struct sockaddr *)&client.client_addr, &client.addr_len)) == -1)
        {
            perror("[receiver] recvfrom");
            exit(1);
        }
        client.username[numbytes] = '\0'; // add null terminator for printing

        // convert client's address to a string
        if (inet_ntop(client.client_addr.ss_family, &(((struct sockaddr_in *)&client.client_addr)->sin_addr), client.client_ip_string, INET6_ADDRSTRLEN) == NULL)
        {
            perror("[receiver] inet_ntop");
            exit(3);
        }
        printf("[receiver] client (%s) \"%s\" has successfully connected...\n", client.client_ip_string, client.username);
























}//end wait_for_connection


int SWPReceiver::receive_file(char *filename){

    //open up a file to dump info to
    fp = fopen ("server_log.txt", "a"); //a for append, w for write which will overwrite
    fprintf(fp,"==================================================================\n");
    //we need to astablish some kind of buffer that acts as a slidding frame

    //I think its an endless while loop, but we might use poll
    //we only write to the file when the window shifts, next expected
    //we need some way of sending acks and keeping track of the frames

    //



}//end of send_file




int SWPReceiver::disconnect(){
    

    printf("[receiver] closing socket...\n");

    //close log file and socket pointer
    fclose (fp);
    close(sockfd);

}//end of disconnect



//read from file
//set up own port/strucks
//make connection

//recive inital set up from server

//send information

//send ack

//send packet
