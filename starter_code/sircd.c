#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <assert.h>
#include "debug.h"
#include "rtlib.h"
#include "rtgrading.h"
#include "sircd.h"
#include "irc_proto.h"
#include <errno.h>

#define PORT        6667        //port of our echo server 
// PORT = curr_node_config_entry->irc_port
#define BUFLEN      1024         //length of the 
#define EINTR 4

u_long curr_nodeID;
rt_config_file_t   curr_node_config_file;  /* The config_file  for this node */
rt_config_entry_t *curr_node_config_entry; /* The config_entry for this node */

void usage() {
    fprintf(stderr, "sircd [-h] [-D debug_lvl] <nodeID> <config file>\n");
    exit(-1);
}

void err_sys(const char* x){
    perror(x);
    exit(-1);
}
 
void err_quit(const char* x){
    err_sys(x);
}

void irc_server () {

    char buf[BUFLEN];                        // buffer
    int socketid;                            //socket id of server
    int maxi, maxFD;                  
    int conn_client, sockFD;
    int nready; 
    ssize_t n;
    socklen_t clilen;           
    struct sockaddr_in cliaddr;
    struct sockaddr_in servaddr;  
    int i; 
    char *token;                

    client clients[MAX_CLIENTS];             // Array of Clients
    channel channels[MAX_CLIENTS];           // Array of Channels
   
    //Allocating a Socket
    socketid = socket(AF_INET, SOCK_STREAM, 0);
    if (socketid < 0) {
        err_sys("Couldn't Allocate the Socket!");
    }

    //Initialising the socket
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
   
    //Binding the Socket
    if (bind(socketid, (struct sockaddr *) &servaddr, sizeof(servaddr)) != 0) {
        err_sys("Error in assigning socket address to socket file descriptor");
    }
 
    //Listening from the Socket
    if (listen(socketid, MAX_CLIENTS) != 0){
        err_sys("Listening Error");
    } puts("Server is Listening...");
   
    // Initializing
    maxFD = socketid;
    maxi = -1;
   
    // Initialize Client Array
    for (i = 0; i < MAX_CLIENTS; i++) {
        clients[i] = init_client();
    }
    // Initialize Channel Array
    set_channel(channels);
    
    // Creating a set of Sockets to add incoming connections.  
    fd_set unread_sockets;              // set of open sockets
    fd_set socket_set;                  // set of sockets waiting to be read

    FD_ZERO(&socket_set);               // Clearing the socket_set
    FD_SET(socketid, &socket_set);      // Adding the initial socket into the set
 
    while (1) {

        unread_sockets = socket_set;
        // monitor multiple file descriptors, waiting until one or more of the
        // file descriptors become "ready" for some class of I/O operation
        // status: number of ready client.

        if ((nready = select(maxFD + 1, &unread_sockets, NULL, NULL, NULL)) == -1) {
            err_sys("Select Error.");
        }
       
        // Check if sockid in socket_set
        if (FD_ISSET(socketid, &unread_sockets)) {


            clilen = sizeof(cliaddr);
            //Accepting a new connection from a client
            if ((conn_client = accept(socketid, (struct sockaddr *) &cliaddr, &clilen)) == -1) {
                err_sys("Error in accept");
            }
           
            // Checking which sockets are ready to be read
            for (i = 0; i < MAX_CLIENTS; i++) {
                if(clients[i].sock < 0) {
                    clients[i].sock = conn_client;
                    break;
                }
            }

            if (i == FD_SETSIZE) err_quit("No more clients please.");
               
            // Add our connection to socket_set
            FD_SET(conn_client, &socket_set);
               
            // Keeping track of our maximum FD
            if (conn_client > maxFD) maxFD = conn_client;
               
            // Keeping track of our maximum index into the client array
            if (i > maxi) maxi = i;
 
            // Continue until there are no more readable FDs
            if (--nready <= 0) continue;
        }
       
        // Loop through our client array to read data from existing connections
        for (i = 0; i <= maxi; i++) {

            if ((sockFD = clients[i].sock) < 0) continue;
 
            if (FD_ISSET(sockFD, &unread_sockets)) {
 
                // Reading from the incomming connection
                bzero(buf, BUFLEN);
                if ( (n = read(sockFD, buf, BUFLEN)) == 0) {
                    handle_line("QUIT", i, clients, channels, &socket_set);
                }
 
                // Otherwise, broadcast the message to all connected clients               
                else {

                    token = strtok(buf, "\r\n");
                    while (token != NULL){
                        printf("Recieved Message from Client %d: %s\n", sockFD, token);
                        handle_line(token, i, clients, channels, &socket_set);
                        token = strtok(NULL, "\r\n");
                    }

                    //broadcast(buf, clients, i, channels);
                }
 
                 // check buffer and clear if anything inside 
                bzero(buf, BUFLEN);
                if (--nready <= 0) break;
            }
        }
    }
}


int main( int argc, char *argv[] )
{
    extern char *optarg;
    extern int optind;
    int ch;    

    while ((ch = getopt(argc, argv, "hD:")) != -1)
        switch (ch) {
	case 'D':
	    if (set_debug(optarg)) {
		exit(0);
	    }
	    break;
    case 'h':
        default: /* FALLTHROUGH */
            usage();
        }
    argc -= optind;
    argv += optind;

    if (argc < 2) {
	usage();
    }
    
    init_node(argv[0], argv[1]);
    
        printf( "I am node %lu and I listen on port %d for new users\n", curr_nodeID, curr_node_config_entry->irc_port );

    /* Start your engines here! */

     irc_server();
    
    return 0;
}


 // * void init_node( int argc, char *argv[] )
 // *
 // * Takes care of initializing a node for an IRC server
 // * from the given command line arguments
 
void init_node(char *nodeID, char *config_file) 
{
    int i;

    curr_nodeID = atol(nodeID);
    rt_parse_config_file("sircd", &curr_node_config_file, config_file );

    /* Get config file for this node */
    for( i = 0; i < curr_node_config_file.size; ++i )
        if( curr_node_config_file.entries[i].nodeID == curr_nodeID )
             curr_node_config_entry = &curr_node_config_file.entries[i];

    /* Check to see if nodeID is valid */
    if( !curr_node_config_entry )
    {
        printf( "Invalid NodeID\n" );
        exit(1);
    }
}

/*
    Inits a client by setting all the values to default.
*/

client init_client(){

    client aClient;

    aClient.sock = -1;
    bzero(&aClient.cliaddr,sizeof(aClient.cliaddr));
    aClient.registered = -1;
    aClient.user_set = -1;
    aClient.nick_set = -1;
    aClient.channelID = -1;

    memset(aClient.hostname,'\0', sizeof(aClient.hostname));
    memset(aClient.servername,'\0', sizeof(aClient.servername));
    memset(aClient.user,'\0', sizeof(aClient.user));
    memset(aClient.nick,'\0', sizeof(aClient.nick));
    memset(aClient.realname,'\0', sizeof(aClient.realname));
    memset(aClient.inbuf,'\0', sizeof(aClient.hostname));
    memset(aClient.ch_name,'\0', sizeof(aClient.hostname));

    return aClient;
}

/*
    Init channel
*/

void set_channel(channel *channels){

    for (int i = 0; i < MAX_CLIENTS; i++) {
        channels[i].id = i;
        channels[i].num_clients = 0;
    }
}

/*
    Broadcast Message to all Clients in a Channel
    where arg channel is the user's channel
*/

void broadcast(char *msg, client *clients, int channelID, channel *channels){

    int j;
    for (j = 0; j < MAX_CLIENTS; j++) {
        if (channelID == clients[j].channelID) {
            response(msg,clients,j);
        }
    }
}

/*
    Send Message to the particular client
*/

void response(char *msg, client *clients, int clientID){
    writen(clients[clientID].sock, msg, strlen(msg)+1);
}


/* Write "n" bytes to a descriptor. */
ssize_t writen(int fd, const void *vptr, size_t n){
     size_t nleft;
     ssize_t nwritten;
     const char *ptr;
 
     ptr = vptr;
     nleft = n;
     while (nleft > 0) {
         if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
             if (nwritten < 0 && errno == EINTR)
                 nwritten = 0;   /* and call write() again */
             else
                 return (-1);    /* error */
          }
 
          nleft -= nwritten;
          ptr += nwritten;
     }
     return (n);
}


