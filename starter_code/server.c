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
//#include "rtgrading.h"
#include "sircd.h"
#include "irc_proto.h"


#define	PORT		6667        /* port of our echo server */
#define	BUFLEN		1024        /* buffer length */

void main()
{
    int	i;                      /* index counter for loop operations */
    int	rc;                     /* system calls return value storage */
    int	s;                      /* socket descriptor */
    int	cs;                     /* new connection's socket descriptor */
    char buf[BUFLEN+1];         /* buffer for incoming data */
    struct sockaddr_in	sa; 	/* Internet address struct */
    struct sockaddr_in	csa; 	/* client's address struct */
    int         	size_csa; 	/* size of client's address struct */
    fd_set		rfd;            /* set of open sockets */
    fd_set		c_rfd;          /* set of sockets waiting to be read */
    int			dsize;          /* size of file descriptors table */
    

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(PORT);
    sa.sin_addr.s_addr = INADDR_ANY;
    
    /* allocate a free socket                 */
    /* Internet address family, Stream socket */
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        perror("socket: allocation failed");
    }
    
    /* bind the socket to the newly formed address */
    rc = bind(s, (struct sockaddr *)&sa, sizeof(sa));
    
    /* check there was no error */
    if (rc) {
        perror("bind");
    }
    
    rc = listen(s, 5);
    
    if (rc) {
        perror("listen");
    }
    
    /* remember size for later usage */
    size_csa = sizeof(csa);
    
    /* calculate size of file descriptors table */
    dsize = getdtablesize();
    
    /* close all file descriptors, except our communication socket	*/
    /* this is done to avoid blocking on tty operations and such.	*/
    for (i = 0; i < dsize; i++)
        if (i != s)
            close(i);
    
    /* we innitialy have only one socket open,	*/
    /* to receive new incoming connections.	*/
    FD_ZERO(&rfd);
    FD_SET(s, &rfd);
    /* enter an accept-write-close infinite loop */
    while (1) {

        c_rfd = rfd;
        rc = select(dsize, &c_rfd, NULL, NULL, (struct timeval *)NULL);
        
        if (FD_ISSET(s, &c_rfd)) {
            /* accept the incoming connection */
       	    cs = accept(s, (struct sockaddr *)&csa, &size_csa);
            
       	    /* check for errors. if any, ignore new connection */
       	    if (cs < 0)
                continue;
            
            /* add the new socket to the set of open sockets */
            FD_SET(cs, &rfd);
            
            continue;
        }
        
        /* check which sockets are ready for reading,	*/
        /* and handle them with care.			*/
        for (i=0; i<dsize; i++)
            if (i != s && FD_ISSET(i, &c_rfd)) {
                /* read from the socket */
                rc = read(i, buf, BUFLEN);
                
                /* if client closed the connection... */
                if (rc == 0) {
                    /* close the socket */
                    close(i);
                    FD_CLR(i, &rfd);
                }
                /* if there was data to read */
                else {
                    /* echo it back to the client */
                    write(i, buf, rc);
                }
            }
    }
}