#include "irc_proto.h"
#include "debug.h"

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include "sircd.h"

#define MAX_COMMAND 16

/* You'll want to define the CMD_ARGS to match up with how you
 * keep track of clients.  Probably add a few args...
 * The command handler functions will look like
 * void cmd_nick(CMD_ARGS)
 * e.g., void cmd_nick(your_client_thingy *c, char *prefix, ...)
 * or however you set it up.
 */

#define CMD_ARGS char *prefix, char **params, int n_params, int clientID, client *clients, channel *channels, fd_set *socketset
typedef void (*cmd_handler_t)(CMD_ARGS);
#define COMMAND(cmd_name) void cmd_name(CMD_ARGS)

void register_user(client *clients, int clientID);
void logout(client *clients, int clientID, fd_set *socketset);
void error_to_all(char* error_msg, err_t errorno, int clientID, client *clients);
void client_error(char* error_msg, err_t errorno, int clientID, client *clients);

struct dispatch {
    char cmd[MAX_COMMAND];
    int needreg; /* Must the user be registered to issue this cmd? */
    int minparams; /* send NEEDMOREPARAMS if < this many params */
    cmd_handler_t handler;
};

#define NELMS(array) (sizeof(array) / sizeof(array[0]))

/* Define the command handlers here.  This is just a quick macro
 * to make it easy to set things up */
COMMAND(cmd_nick);
COMMAND(cmd_user);
COMMAND(cmd_quit);
COMMAND(cmd_join);
COMMAND(cmd_part);
COMMAND(cmd_list);
COMMAND(cmd_privmsg);
COMMAND(cmd_who);

/* Dispatch table.  "reg" means "user must be registered in order
 * to call this function".  "#param" is the # of parameters that
 * the command requires.  It may take more optional parameters.
 */
struct dispatch cmds[] = {
    /* cmd,    reg  #parm  function */
    { "NICK",    0, 1, cmd_nick },
    { "USER",    0, 4, cmd_user },
    { "QUIT",    1, 0, cmd_quit },
    { "JOIN",    1, 1, cmd_join },
    { "PART",    1, 1, cmd_part },
    { "LIST",    0, 0, cmd_list },
    { "PRIVMSG", 1, 1, cmd_privmsg },
    { "WHO",     0, 1, cmd_who }
};

void
handle_line(char *line, int clientID, client * clients, channel * channels, fd_set * socketset)
{
    char *prefix = NULL, *command = NULL, *pstart, *params[MAX_MSG_TOKENS];
    int n_params = 0;
    char *trailing = NULL;

    //printf("Handling line: %s\n", line);
    DPRINTF(DEBUG_INPUT, "Handling line: %s\n", line);

    command = line;
    
    if (*line == ':') {
	prefix = ++line;
	command = strchr(prefix, ' ');
    }
    if (!command || *command == '\0') {
		/* Send an unknown command error! */
		client_error("ERR_UNKNOWNCOMMAND 1 ", ERR_UNKNOWNCOMMAND, clientID, clients);
		return;
    }
    
    while (*command == ' ') {
		*command++ = 0;
    }
    if (*command == '\0') {
		/* Send an unknown command error! */
		client_error("ERR_UNKNOWNCOMMAND 2 ", ERR_UNKNOWNCOMMAND, clientID, clients);
		return;
    }
    pstart = strchr(command, ' ');
    if (pstart) {
	while (*pstart == ' ') {
	    *pstart++ = '\0';
	}
	if (*pstart == ':') {
	    trailing = pstart;
	} else {
	    trailing = strstr(pstart, " :");
	}
	if (trailing) {
	    while (*trailing == ' ')
		*trailing++ = 0;
	    if (*trailing == ':')
		*trailing++ = 0;
	}
	
	do {
	    if (*pstart != '\0') {
		params[n_params++] = pstart;
	    } else {
		break;
	    }
	    pstart = strchr(pstart, ' ');
	    if (pstart) {
		while (*pstart == ' ') {
		    *pstart++ = '\0';
		}
	    }
	} while (pstart != NULL && n_params < MAX_MSG_TOKENS);
    }

    if (trailing && n_params < MAX_MSG_TOKENS) {
	params[n_params++] = trailing;
    }
    
    //printf("Prefix:  %s\nCommand: %s\nParams (%d):\n", prefix ? prefix : "<none>", command, n_params);

    DPRINTF(DEBUG_INPUT, "Prefix:  %s\nCommand: %s\nParams (%d):\n",
	   prefix ? prefix : "<none>", command, n_params);
    int i;
    for (i = 0; i < n_params; i++) {
    	//printf( "   %s\n", params[i]);
		DPRINTF(DEBUG_INPUT, "   %s\n", params[i]);
    }
    //printf("\n");
    DPRINTF(DEBUG_INPUT, "\n");

    for (i = 0; i < NELMS(cmds); i++) {
		if (!strcasecmp(cmds[i].cmd, command)) {
		    if (cmds[i].needreg && clients[clientID].registered != 1 ) { /* checking if client is registered */
			        /* ERROR THAT CLIENT IS NOT REGISTERED */

					client_error("ERR_NOTREGISTERED ", ERR_NOTREGISTERED, clientID, clients);
			    	return;

				 //ERROR - the client is not registered and they need
				 //* to be in order to use this command! 

		    } else if (n_params < cmds[i].minparams) {
				/* ERROR - the client didn't specify enough parameters
				 * for this command! */
					client_error("ERR_NEEDMOREPARAMS ", ERR_NEEDMOREPARAMS, clientID, clients);
			    	return;
		    } else {
				/* Here's the call to the cmd_foo handler... modify
				 * to send it the right params per your program
				 * structure. */
				(*cmds[i].handler)(prefix, params, n_params, clientID, clients, channels, socketset);
		    }
		    break;
		}
     }
    if (i == NELMS(cmds)) {
		/* ERROR - unknown command! */
		client_error("ERR_UNKNOWNCOMMAND ", ERR_UNKNOWNCOMMAND, clientID, clients);
    	return;
    }
}
// -------------------------------- BASIC COMMANDS -------------------------------- //

/* Command handlers */
/* MODIFY to take the arguments you specified above! */

/*
	USER GETS REGISTERED WHEN BOTH USER AND NICK are set
	Errors:
	ERR_NONICKNAMEGIVEN	ERR_ERRONEUSNICKNAME
	ERR_NICKNAMEINUSE	ERR_NICKCOLLISION
*/
void cmd_nick(char *prefix, char **params, int n_params, int clientID, client *clients, channel *channels, fd_set *socketset)
{	

	int c;								// client iterator
	char msg[MAX_MESSAGE_LEN];			
	memset(msg, '\0', sizeof(msg));

	// Length Check
	if (strlen(params[0]) > MAX_USERNAME){
		client_error("Nick too long. ", ERR_NICKTOOLONG, clientID, clients);
		return;
	} 

    // If the Client is Registered. || Checking: for Nick name Clash
	for (c = 0; c < MAX_CLIENTS; c++){
		if (c != clientID){
			if (strcmp(clients[c].nick, params[0]) == 0){
				client_error("This nick in use. ", ERR_NICKNAMEINUSE, clientID, clients);
				break;
			}
		}
	}

	// At the end of the loop and Nick not found then set NICK
	if (c == MAX_CLIENTS){
		strcpy(clients[clientID].nick, params[0]);	  		// Setting the nick
		if (clients[clientID].nick_set != 1){
			sprintf(msg, ":%s your nick has been registered! \n", clients[clientID].nick);
		} else {
			sprintf(msg, "Your nick has been changed to %s! \n", clients[clientID].nick);
		}
		response(msg, clients, clientID);
		clients[clientID].nick_set = 1; 					// Set flag
		register_user(clients,clientID);					
   
    }
    return;
}

/*
	Sets the <username> <hostname> <servername> <realname> of the client
	USER GETS REGISTERED WHEN BOTH USER AND NICK are set
	Errors:	ERR_NEEDMOREPARAMS	ERR_ALREADYREGISTRED
*/

void cmd_user(char *prefix, char **params, int n_params, int clientID, client *clients, channel *channels, fd_set *socketset)
{	
	char msg[MAX_MESSAGE_LEN];
	memset(msg, '\0', sizeof(msg));

	if (n_params < 4){
		client_error("Need for parameters.", ERR_NEEDMOREPARAMS, clientID, clients);
	} else if (clients[clientID].user_set > 0) {
		client_error("Already Registered.", ERR_ALREADYREGISTRED, clientID, clients);
	} else {

		/* Setting the <username> <hostname> <servername> <realname> of the client */
		sprintf(clients[clientID].user,"%s",params[0]);
		sprintf(clients[clientID].hostname,"%s",params[1]);
		sprintf(clients[clientID].servername,"%s",params[2]);
		sprintf(clients[clientID].realname,"%s",params[3]);

		sprintf(msg,"User Information Registered.\n");
		clients[clientID].user_set = 1; 			// Set flag
		register_user(clients,clientID);
		response(msg, clients, clientID);
		return;
	}
}

/*
	 Parameters:	[<Quit message>]
	 If a "Quit Message" is given, this will be sent instead of the default message, 
	 the nickname.
	 If user leaves without informing send a default message.
*/

void cmd_quit(char *prefix, char **params, int n_params, int clientID, client *clients, channel *channels, fd_set *socketset){

	char msg[MAX_MESSAGE_LEN];
	memset(msg, '\0', sizeof(msg));

	if (n_params == 0){
		sprintf(msg, ":%s QUIT : Connection closed!", clients[clientID].nick);
	} else if (n_params == 1){
		sprintf(msg, ":%s QUIT Message : %s", clients[clientID].nick, params[0]);
	}

	if (clients[clientID].registered > 0) {
		broadcast(msg, clients, clients[clientID].channelID, channels);
	}
		
	channels[clients[clientID].channelID].num_clients--;
	logout(clients, clientID, socketset);

	return;
}

// -------------------------------- CHANNEL COMMANDS -------------------------------- //

/*
	Start listening to a specific channel
	Parameters:	<channel>{,<channel>} [<key>{,<key>}]
	-Client can only join one channel
	-If the channel does not exist, it is created automatically. 
	-Ignore if user is already on the channel
	-The JOIN command is echoed to all users on the channel, including the new user.
*/

void cmd_join(char *prefix, char **params, int n_params, int clientID, client *clients, channel *channels, fd_set *socketset){

	int channel_ID;
	int c;
	char temp[MAX_MESSAGE_LEN];
	char msg[MAX_MESSAGE_LEN];

	if (strlen(params[0]) > MAX_CHANNAME){
		client_error("Channel name too long.", ERR_INVALID, clientID, clients);
		return;
	}

	if (clients[clientID].registered > 0){

		// If user is already in a channel
		if (clients[clientID].channelID > 0){
			channel_ID = clients[clientID].channelID;
		}

		// IF THE CHANNEL PREVIOUSLY EXISTS
		for (c = 0; c < MAX_CLIENTS; c++){
			if ((strcmp(params[0], channels[c].ch_name) == 0) && channels[c].num_clients > 0){
				// Leave the previous channel
				if (channel_ID > -1){
					sprintf(msg, ":%s has parted the %s", clients[clientID].nick, channels[channel_ID].ch_name);
					broadcast(msg, clients, clients[clientID].channelID, channels);
					channels[channel_ID].num_clients--;
					clients[clientID].channelID = -1;
				}
				channels[c].num_clients++;
				clients[clientID].channelID = channels[c].id;
				sprintf(channels[c].ch_name, "%s", params[0]);
				sprintf(msg, ":%s has joined %s", clients[clientID].nick,channels[c].ch_name);
				broadcast(msg, clients, clients[clientID].channelID, channels);

				memset(temp, '\0', sizeof(temp));
				sprintf(temp, "ID :%d: Name:%s Users:%d\n", channels[c].id, channels[c].ch_name, channels[c].num_clients);
				response(temp, clients, clientID);
				return;
			}
		}

		// OTHERWISE CREATE A NEW CHANNEL
		for (c = 0; c < MAX_CLIENTS; c++){
			if ((strcmp(params[0], channels[c].ch_name) != 0) && channels[c].num_clients == 0){

				// Leave the previous channel
				if (channel_ID > -1){
					sprintf(msg, ":%s has parted the %s", clients[clientID].nick, channels[channel_ID].ch_name);
					broadcast(msg, clients, clients[clientID].channelID, channels);
					channels[channel_ID].num_clients--;
					clients[clientID].channelID = -1;
				}

				// Create new Channel
				channels[c].num_clients++;
				clients[clientID].channelID = channels[c].id;
				sprintf(channels[c].ch_name, "%s", params[0]);
				sprintf(msg, ":%s has joined %s", clients[clientID].nick,channels[c].ch_name);
				broadcast(msg, clients, clients[clientID].channelID, channels);

				memset(temp, '\0', sizeof(temp));
				sprintf(temp, "ID :%d: Name:%s Users:%d\n", channels[c].id, channels[c].ch_name, channels[c].num_clients);
				response(temp, clients, clientID);
				break;
			}
		}
		
		if (c == MAX_CLIENTS){
			client_error("Can't create more channels channel.", ERR_INVALID, clientID, clients);
			return;
		}
	}
	return;
}

/*
	Depart a specific channel.
	Parameters:	<channel>{,<channel>}
	-Depart from all the Channels the user is in
	-If the parameters are valid, the server must echo the command to all users on the channel(s).
	Errors:
			ERR_NEEDMOREPARAMS	ERR_NOSUCHCHANNEL
			ERR_NOTONCHANNEL	
*/

void cmd_part(char *prefix, char **params, int n_params, int clientID, client *clients, channel *channels, fd_set *socketset){

	char msg[MAX_MESSAGE_LEN];
	memset(msg, '\0', sizeof(msg));
	int channel_ID;
	int c;

	printf("%s\n", params[0]);

	for (c = 0; c < MAX_CLIENTS; c++){
		if (strcmp(params[0], channels[c].ch_name) == 0){
			channel_ID = channels[c].id;
			break;
		} 
	}

	if (c == MAX_CLIENTS){
		client_error("No such Channel Exists.", ERR_NOSUCHCHANNEL, clientID, clients);
		return;
	}

	if (n_params < 1){
		client_error("Enter the name of Channel to Part.", ERR_NEEDMOREPARAMS, clientID, clients);
		return;
	} else if (strcmp(channels[channel_ID].ch_name, params[0]) == 0){
		channels[channel_ID].num_clients--;
		sprintf(msg, ":%s PARTED %s", clients[clientID].nick, channels[channel_ID].ch_name);
		broadcast(msg, clients, clients[clientID].channelID, channels);
		clients[clientID].channelID = -1;
	} else {
		client_error("Not on this Channel.", ERR_NOTONCHANNEL, clientID, clients);
	}
	return;
}

/*
	List all existing channels and number of client per channel on the local server only.
	Error & Numeric Replies:
		ERR_NOSUCHSERVER	
		RPL_LISTSTART	RPL_LIST
		RPL_LISTEND
*/

void cmd_list(char *prefix, char **params, int n_params, int clientID, client *clients, channel *channels, fd_set *socketset){

	response("Channels Available: \n",clients,clientID);
	char chanInfo[MAX_MESSAGE_LEN];
	char temp[MAX_MESSAGE_LEN];
	memset(temp, '\0', sizeof(temp));
	int c;
	int ch_count = 0;

	for (c = 0; c < MAX_CLIENTS; c++){
		if (channels[c].num_clients > 0) {
			memset(chanInfo, '\0', sizeof(chanInfo));
			sprintf(chanInfo, "ID :%d: Name:%s Users:%d\n", channels[c].id, channels[c].ch_name, channels[c].num_clients);
			strcat(temp,chanInfo);
			ch_count++;
		}
	}

	if (ch_count == 0) {
		response("There are no channels.",clients,clientID);
	}

	response(temp, clients, clientID);
	return;
}

// -------------------------------- ADVANCED COMMANDS -------------------------------- //

/*
	Send messages to users.
	Parameters:	<target>{,<target>} <text to be sent>
	The target can be either a nickname or a channel.
	If the target is a channel, broadcast except self.
	Errors & Replies:
		ERR_NORECIPIENT	ERR_NOTEXTTOSEND
		ERR_CANNOTSENDTOCHAN	ERR_NOTOPLEVEL
		ERR_WILDTOPLEVEL	ERR_TOOMANYTARGETS
		ERR_NOSUCHNICK	
		RPL_AWAY	
*/
void cmd_privmsg(char *prefix, char **params, int n_params, int clientID, client *clients, channel *channels, fd_set *socketset){
	
	int c;
	int p;
	int channel_ID = -1;
	char msg[MAX_MSG_LEN];
	char temp[MAX_MSG_LEN];


	memset(msg, '\0', sizeof(msg));
	sprintf(msg, "%s said: ", clients[clientID].nick);

	for (c = 0; c < MAX_CLIENTS; c++){
		if (strcmp(channels[c].ch_name, params[0]) == 0 && channels[c].num_clients > 0) {
			channel_ID = channels[c].id;
			break;
		}
	}

	// Getting the whole message from the sender
	for (p = 1; p < n_params; p++){
		memset(temp, '\0', sizeof(msg));
		sprintf(temp, "%s ", params[p]);
		strcat(msg, temp);
	}

		if (channel_ID > -1){
			broadcast(msg, clients, clients[clientID].channelID, channels);
		} else {
			for (c = 0; c < MAX_CLIENTS; c++){
				if ((strcmp(clients[c].nick, params[0]) == 0)){
					response(msg, clients, c);
				} 
			}
		}	
	return;
}

void cmd_who(char *prefix, char **params, int n_params, int clientID, client *clients, channel *channels, fd_set *socketset){

	int c, i;
	int channel_ID = -1;
	char msg[MAX_MSG_LEN];
	char temp[MAX_MESSAGE_LEN];
	memset(msg, '\0', sizeof(msg));

	//Go through all channels and their clients

	for (c = 0; c < MAX_CLIENTS; c++){
		if (strcmp(channels[c].ch_name, params[0]) == 0 && channels[c].num_clients > 0) {
			channel_ID = channels[c].id;
			break;
		}
	}

	if (c == MAX_CLIENTS){
		client_error("Nothing Found!", ERR_NOSUCHCHANNEL, clientID, clients);
	} else if (channel_ID > -1) {

		sprintf(temp, "Found in channel: %s\n", channels[channel_ID].ch_name);
		response(temp, clients, clientID);

		for (i = 0; i < MAX_CLIENTS; i++){
			if (clients[i].channelID == channel_ID){
				memset(temp, '\0', sizeof(temp));
				sprintf(temp, "NICK: %s REALNAME: %s\n", clients[i].nick, clients[i].realname);
				strcat(msg, temp);
			}
		}
	}

	response(msg, clients, clientID);
	return;
	
}

/*
	Sends an Error Message to a Client
*/

void client_error(char* error_msg, err_t errorno, int clientID, client *clients){

	char msg[MAX_MSG_LEN];
	memset(msg, '\0', sizeof(msg));
	sprintf(msg, "Error %d: %s\n", errorno, error_msg);
	response(msg,clients,clientID);
	return;
}

/*
	Used to register a client
	USER GETS REGISTERED WHEN BOTH USER AND NICK are set	
*/

void register_user(client *clients, int clientID){
	if (clients[clientID].user_set == 1 && clients[clientID].nick_set == 1){
		clients[clientID].registered = 1;
	} 
}

/*
	Close a Connection of a user. 
	Log the client out
*/

void logout(client *clients, int clientID, fd_set *socketset){

	close(clients[clientID].sock);
	FD_CLR(clients[clientID].sock, socketset);
	clients[clientID].registered = -1;
	clients[clientID].user_set = -1;
	clients[clientID].nick_set = -1;
	clients[clientID].sock = -1;
	clients[clientID].channelID = -1;
}



