#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define	PORT		6667        //port of our echo server 
#define	BUFLEN		512          //length of the buffer

int main( int argc, char *argv[] ) {

	char buf[BUFLEN];					//buffer
	int sockid; 						//socket id of server
    int clientid;                       //client id of server
	int conn_client;					//connection with client
	int bind_status;					//Value Retured by Bind
	int listen_status;					//Valye Returned by Listen
	struct sockaddr_in servaddr;		//structure that holds server's address
	struct sockaddr_in	clientaddr; 	           //structure that holds clients's address
    unsigned int addr_size = sizeof(clientaddr);   //address length of client

	//Allocating a Socket
    sockid = socket(AF_INET, SOCK_STREAM, 0);
    if (sockid < 0){
    	perror("Couldn't Allocate the Socket!");
        exit(1);
    }

	//Initialising the socket
	bzero( &servaddr, sizeof(servaddr)); 
	servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(PORT);	

    //Binding the Socket
    bind_status = bind(sockid, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if (bind_status != 0) {
    	perror("Bind Error");
        exit(1);
    }

    //Listening from the Socket
    listen_status = listen(sockid, 10);
    if (listen_status != 0){
    	perror("Listening Error");
        exit(1);
    } puts("Server is Listening...");
 
    size_t table_size = getdtablesize();    //Size of the file descriptor table

    //Closing all connections except the socket.
    for (int i = 0; i < table_size; i++){
        if (i != sockid) close(i);
    }
    
    // Creating a set of Sockets to add incoming connections.    
    fd_set socket_set;            // set of open sockets
    fd_set unread_sockets;        // set of sockets waiting to be read

    FD_ZERO(&socket_set);         // Clearing the socket_set
    FD_SET(sockid, &socket_set);  // Adding the initial socket into the set

    while(1) {

        unread_sockets = socket_set;
        //monitor multiple file descriptors, waiting until one or more of the
        //file descriptors become "ready" for some class of I/O operation
        int status = select(table_size, &unread_sockets, NULL, NULL, (struct timeval *)NULL);

        if (FD_ISSET(sockid, &unread_sockets)){
            
            //Accepting a new connection from a client
            conn_client = accept(sockid, (struct sockaddr*)&clientaddr, &addr_size);
            //conn_client = accept(sockid, (struct sockaddr*)NULL, NULL);

            //Checking for Errors
            if (conn_client < 0) continue;

            //Adding new connection to the set
            FD_SET(conn_client, &socket_set);
            continue;
        }

        // Checking which sockets are ready to be read
        for (int i = 0; i < table_size; i++){
            if (i != sockid && FD_ISSET(i, &unread_sockets)){
                
                // Reading from the incomming connection
                bzero(buf, BUFLEN);
                int read_status = read(i, buf, BUFLEN);

                // If the connection no longer exist close it and eject from the set
                if (read_status == 0)
                {
                    close(i);
                    FD_CLR(i, &socket_set);
                } 

                // Else Echo the Message Back to all the active clients
                else 
                {   
                    write(i, buf, strlen(buf)+1);

                    for (int j = 0; j < table_size; j++){
                        if (j != sockid && FD_ISSET(j, &socket_set) && i != j){
                            write(j, buf, strlen(buf)+1);
                        }
                    }
                }
            }
            
        }

	}
 
}
