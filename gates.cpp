#include "helpers.h"

/**
 * Forwards to_send bytes from response address through sockfd ensuring their 
 * transmission; 
 * Prints error_message in case of an error and if die is true stops the entire 
 * program.
 * @param sockfd the socket through is send the information
 * @param response the address where from the information is taken to be sent
 * @param to_send the number of bytes sent from the response address
 * @param error_message the string displayed if an error occurs
 * @param die specifies whether the program should stop or could continue if an 
 * error occurs
 */
void send_whole_packet(const int sockfd, const void *response,
	const int to_send, const char *error_message, const bool die)
{
	int bytes_sent = send(sockfd, response, to_send, 0);
	if (die) {
		DIE(bytes_sent < 0, error_message);
	} else if (bytes_sent < 0) {
		fprintf(stderr, "%s\n", error_message);
		return;
	}

	if (bytes_sent == to_send)
		return;

	int rest = to_send - bytes_sent;
	do {
		response = ((char *) response) + bytes_sent;
		bytes_sent = send(sockfd, response, rest, 0);
		if (die)
			DIE(bytes_sent < 0, error_message);
		else if (bytes_sent < 0)
			fprintf(stderr, "%s\n", error_message);

		rest -= bytes_sent;
	} while (rest > 0);
}

/**
 * Gets to_receive bytes from response address through sockfd ensuring their 
 * arrival; 
 * Prints error_message in case of an error and if die is true stops the entire 
 * program.
 * @param sockfd the socket through is received the information
 * @param response the address where the information is placed
 * @param to_send the number of bytes sent from the response address
 * @param error_message the string displayed if an error occurs
 * @param die specifies whether the program should stop or could continue if an
 * error occurs
 */
void get_whole_packet(const int sockfd, void *response,
        const int to_receive, const char *error_message, const bool die)
{
    int bytes_received = recv(sockfd, response, to_receive, 0);
	if (die) {
		DIE(bytes_received < 0, error_message);
	} else if (bytes_received < 0) {
		fprintf(stderr, "%s\n", error_message);
		return;
	}

	if (bytes_received  == to_receive)
		return;

	int rest = to_receive - bytes_received;
	do {
		response = (void *) (((char *) response) + bytes_received);
		bytes_received = recv(sockfd, response, rest, 0);
		if (die)
			DIE(bytes_received < 0, error_message);
		else if (bytes_received < 0)
			fprintf(stderr, "%s\n", error_message);

		rest -= bytes_received;
	} while (rest > 0);
}
