#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <inttypes.h>

/*
 * Macro for errors checking
 * Example:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */
#define DIE(assertion, call_description)    \
	do {                                    \
		if (assertion) {                    \
			fprintf(stderr, "(%s, %d): ",   \
					__FILE__, __LINE__);    \
			perror(call_description);       \
			exit(EXIT_FAILURE);             \
		}                                   \
	} while(0)


extern void send_whole_packet(const int sockfd, const void *response,
		const int to_send, const char *error_message, const bool die);
extern void get_whole_packet(const int sockfd, void *response,
        const int to_receive, const char *error_message, const bool die);

// for messages capacity
#define BUFLEN 2000
#define TOPIC_SIZE 50
#define CONTENT_SIZE 1500
#define MAX_CLIENTS 5
#define ID_SIZE 10

// for communication between the server and a tcp client
#define STRING_DATA_SIZE 40
#define ALREADY_CONNECTED_MESSAGE "Client %s already connected."
#define SUBSCRIBED "Subscribed to topic."
#define UNSUBSCRIBED "Unsubscribed from topic."

// for the input/output of the server and clients
#define EXIT_STRING "exit"
#define SERVER_CLOSED "Server closed."
#define UNKNOWN_COMMAND "Unknown command.\nThe available commands are:\n\
> subscribe <TOPIC> <SF>\n> unsubscribe <TOPIC>"
#define CONNECTED_FORMAT "New client %s connected from %s:%d."
#define DISCONNECTED_FORMAT "Client %s disconnected."
#define NOTIFICATION_PREFIX "%s:%d - %s - %s - "
#define NL "\n"

// for the data from udp
#define INT ((uint8_t) 0)
#define SHORT_REAL ((uint8_t) 1)
#define FLOAT ((uint8_t) 2)
#define STRING ((uint8_t) 3)

// subscription type
#define SF_SET true
#define SF_NULL false



enum tcp_command {
	SUBSCRIBE, UNSUBSCRIBE, EXIT
};

// the packet format sent from a tcp client to the server
typedef struct {
	enum tcp_command command;
	char topic[TOPIC_SIZE];
	bool SF;
} tcp_msg;

// notification about a topic received from an udp client which will be
// forwarded by the server to all udp client's subscribers
typedef struct {
	char topic[TOPIC_SIZE];
	uint8_t data_type;
	char content[CONTENT_SIZE];
} udp_msg;

// wrapper of the above structure
typedef struct {
	int portno;
	struct in_addr ip_source;
	udp_msg notification;
} udp_data;

enum server_respond_type {
	STRING_DATA, UDP_DATA
};

// structure used by the server when sends information to a tcp client;
// can be an acknowledgement for a subscribe/unsubscribe request or a
// datagram from an udp client
typedef struct {
	enum server_respond_type type;
	union {
		char message[STRING_DATA_SIZE];
		udp_data info;
	} data;
} server_respond;


// message sent by a tcp client when tries to connect to the server
typedef struct {
	char id[ID_SIZE];
} connect_request;

#endif
