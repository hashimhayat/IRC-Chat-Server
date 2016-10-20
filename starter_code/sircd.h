
#ifndef _SIRCD_H_
#define _SIRCD_H_

#include <sys/types.h>
#include <netinet/in.h>
#include "irc_proto.h"

#define MAX_CLIENTS 512
#define MAX_MSG_TOKENS 10
#define MAX_MSG_LEN 512
#define MAX_USERNAME 32
#define MAX_HOSTNAME 512
#define MAX_SERVERNAME 512
#define MAX_REALNAME 512
#define MAX_CHANNAME 512

typedef struct {
    int sock;
    struct sockaddr_in cliaddr;         // Cliend Address 
    unsigned inbuf_size;                // Message Buffer Size
    int registered;                     // Not Registered: -1 Registered: 1
    char hostname[MAX_HOSTNAME];        
    char servername[MAX_SERVERNAME];    // Server's Address
    char user[MAX_USERNAME];            // username of the client
    char nick[MAX_USERNAME];            // Nick name of the client
    char realname[MAX_REALNAME];        // Real name of the client
    char inbuf[MAX_MSG_LEN+1];          // Buffer to store Message
    char ch_name[MAX_CHANNAME];         // Name of the Channel the user is in
    int user_set;                       // Flag to check user_set
    int nick_set;                       // Flag to check user_set
    int channelID;                      // Flag to check user's in channel status
} client;

typedef struct {
    int id;                         // Channel ID
    int num_clients;                // Number of Clients in a Channel
    char ch_name[MAX_CHANNAME];     // Channel Name
} channel;

void handle_line(char *line, int clientID, client * clients, channel * channels, fd_set * allset);
void init_node(char *nodeID, char *config_file);
void set_channel(channel *channel);
void broadcast(char *msg, client *clients, int channelID, channel *channels);
void response(char *msg, client *clients, int clientID);
void irc_server();
void set_channel();
client init_client();
ssize_t writen(int fd, const void *vptr, size_t n);

#endif /* _SIRCD_H_ */
