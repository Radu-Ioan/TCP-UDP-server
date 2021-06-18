#include "helpers.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <queue>


using namespace std;

// for loop condition
#define STOP false
#define CONTINUE true

struct Client {
	// the socket through the client changed data last time
	int sockfd;
	char id[ID_SIZE];
	bool is_on;

	// the set with the topics and their subscription type
	// true for SF = 1, otherwise false
	unordered_map<string, bool> topics{};

	// retains the lost info from each topic when the client has been offline
	queue<server_respond> lost_info{};

	Client() { }

	Client(const string &id, const int sockfd) {
		strcpy(this->id, id.c_str());
		this->sockfd = sockfd;
		is_on = true;
	}

	Client(const char id[], const int sockfd) {
		strcpy(this->id, id);
		this->sockfd = sockfd;
		is_on = true;
	}

	void receive_offline_info(void) {
		while (!lost_info.empty()) {
			server_respond notification = lost_info.front();
			lost_info.pop();
			send_whole_packet(sockfd, &notification, sizeof(server_respond),
				"receving lost data", 1);
		}
	}
};

void usage(const char *file)
{
	fprintf(stderr, "Usage: %s <PORT_DORIT>\n", file);
	exit(0);
}

void init(int &sockfd_udp, int &sockfd_tcp, fd_set &read_fds,
		struct sockaddr_in &serv_addr, const int portno, int &fd_max)
{
	FD_ZERO(&read_fds);

	sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
	sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sockfd_tcp < 0 || sockfd_udp < 0, "socket");

	fd_max = (sockfd_tcp > sockfd_udp) ? sockfd_tcp : sockfd_udp;

	// disable Nagle's algorithm
	int flag = 1;
	int ret = setsockopt(sockfd_tcp, IPPROTO_TCP, TCP_NODELAY, (char *) &flag,
						sizeof(int));
	if (ret < 0)
		fprintf(stderr, "Nagle's algorithm not disabled!\n");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sockfd_tcp, (struct sockaddr *) &serv_addr,
		sizeof(struct sockaddr));
	DIE(ret < 0, "bind tcp");
	ret = bind(sockfd_udp, (struct sockaddr *) &serv_addr,
		sizeof(struct sockaddr));
	DIE(ret < 0, "bind udp");

	ret = listen(sockfd_tcp, MAX_CLIENTS);
	DIE(ret < 0, "listen tcp");

	// add the new file descriptors
	FD_SET(sockfd_tcp, &read_fds);
	FD_SET(sockfd_udp, &read_fds);
	FD_SET(STDIN_FILENO, &read_fds);
}

bool process_requests(fd_set * const read_fds, int &fd_max,
		const int &sockfd_tcp, const int &sockfd_udp,
		const struct sockaddr_in &serv_addr,
		unordered_map<string, Client> * const tcp_clients,
		unordered_map<int, string> * const sockets,
		unordered_map<string, unordered_set<string>> * const all_topics)
{
	fd_set tmp_fds = *read_fds;
	int ret, newsockfd;
	struct sockaddr_in cli_addr;
	socklen_t clilen = sizeof(cli_addr);

	ret = select(fd_max + 1, &tmp_fds, NULL, NULL, NULL);
	DIE(ret < 0, "select in server");

	server_respond response;

	for (int sock_it = 0; sock_it <= fd_max; sock_it++)
	{
		// check the descriptors changed
		if (!FD_ISSET(sock_it, &tmp_fds))
			continue;

		memset(&response, 0, sizeof(response));

		if (sock_it == STDIN_FILENO)
		{
			char buffer[BUFLEN];
			memset(buffer, 0, BUFLEN);

			fgets(buffer, BUFLEN, stdin);
			// remove '\n' from input
			buffer[strlen(buffer) - 1] = '\0';

			if (strcmp(buffer, EXIT_STRING) == 0) {
				response.type = STRING_DATA;
				strcpy(response.data.message, SERVER_CLOSED);
				for (auto &mapping : *tcp_clients) {
					Client &client = mapping.second;
					if (client.is_on) {
						char error_m[60];
						sprintf(error_m,
							"client %s not well informed about server closing",
							client.id);
						send_whole_packet(client.sockfd, &response,
								sizeof(server_respond), error_m, 0);
					}
				}
				return STOP;
			}
		}
		else if (sock_it == sockfd_tcp)
		{
			// a connection request has arrived
			newsockfd = accept(sockfd_tcp, (struct sockaddr *) &cli_addr,
							&clilen);
			DIE(newsockfd < 0, "accept tcp connection");

			connect_request req;
			get_whole_packet(newsockfd, &req, sizeof(req), "recveive", 0);

			if (tcp_clients->find(req.id) != tcp_clients->end())
			{
				// this id has been used another time
				Client &client = (*tcp_clients)[req.id];

				if (!client.is_on) {
					// a client is reconnecting
					client.is_on = true;
					client.sockfd = newsockfd;

					// the old socket has been closed when the client
					// disconnected
					sockets->emplace(newsockfd, req.id);
					FD_SET(newsockfd, read_fds);
					if (newsockfd > fd_max)
						fd_max = newsockfd;

					// in case he has some data about a subscribed topic sent
					// when he was offline, he receives it now
					client.receive_offline_info();

					// the server displays the acknowledgement of the client's
					// connection
					fprintf(stdout, CONNECTED_FORMAT, client.id,
						inet_ntoa(cli_addr.sin_addr), cli_addr.sin_port);
					fprintf(stdout, NL);
					continue;
				}

				// already exists a client with the same id
				response.type = STRING_DATA;
				sprintf(response.data.message, ALREADY_CONNECTED_MESSAGE,
					req.id);

				fprintf(stdout, "%s" NL, response.data.message);

				send_whole_packet(newsockfd, &response, sizeof(server_respond),
								"send 'already connected' error", 0);

				ret = close(newsockfd);
				DIE(ret < 0, "newsockfd closing");

				continue;
			}

			// the id is available and no one has been connected another time
			Client new_client(req.id, newsockfd);
			tcp_clients->emplace(req.id, new_client);

			sockets->emplace(newsockfd, req.id);

			FD_SET(newsockfd, read_fds);
			if (newsockfd > fd_max)
				fd_max = newsockfd;

			fprintf(stdout, CONNECTED_FORMAT, new_client.id,
				inet_ntoa(cli_addr.sin_addr), cli_addr.sin_port);
			fprintf(stdout, NL);
		}
		else if (sock_it == sockfd_udp)
		{
			// data about a topic has been sent
			udp_msg datagram;
			memset(&datagram, 0, sizeof(datagram));

			int bytes_c = recvfrom(sockfd_udp, &datagram, sizeof(datagram), 0,
				(struct sockaddr *) &cli_addr, &clilen);
			DIE(bytes_c < 0, "recvfrom error");

			if (all_topics->find(datagram.topic) == all_topics->end())
				all_topics->emplace(datagram.topic,
					unordered_set<string>{});

			response.type = UDP_DATA;
			response.data.info.notification = datagram;
			response.data.info.portno = cli_addr.sin_port;
			response.data.info.ip_source = cli_addr.sin_addr;

			// try to inform all clients about this topic
			for (const string &id : (*all_topics)[datagram.topic]) {
				Client &subscribed_client = (*tcp_clients)[id];
				if (subscribed_client.is_on) {
					// if is online, I am forwarding it now
					send_whole_packet(subscribed_client.sockfd, &response,
						sizeof(response), "datagram send", 1);
				} else if (subscribed_client.topics[datagram.topic]
						== SF_SET) {
					// otherwise I put the packet in the queue only if is
					// subscribed with SF = 1 to this topic
					subscribed_client.lost_info.emplace(response);
				}
			}
		}
		else
		{
			// a subscribe/unsubscribe request has been sent or an exit command
			const string &client_id = (*sockets)[sock_it];
			Client &client = (*tcp_clients)[client_id];

			if (ret == 0) {
				client.is_on = false;
				sockets->erase(sock_it);
				FD_CLR(sock_it, read_fds);

				ret = close(sock_it);
				DIE(ret < 0, "socket closing");

				fprintf(stdout, DISCONNECTED_FORMAT, client_id.c_str());
				fprintf(stdout, NL);

				continue;
			}

			tcp_msg req;
			get_whole_packet(sock_it, &req, sizeof(req), "receiving", 0);

			response.type = STRING_DATA;

			if (req.command == SUBSCRIBE)
			{
				if (client.topics.find(req.topic) == client.topics.end())
					client.topics.emplace(req.topic, req.SF);
				else
					client.topics[req.topic] = req.SF;
				(*all_topics)[req.topic].emplace(client.id);
				strcpy(response.data.message, SUBSCRIBED);
			}
			else if (req.command == UNSUBSCRIBE)
			{
				client.topics.erase(req.topic);
				(*all_topics)[req.topic].erase(client.id);
				strcpy(response.data.message, UNSUBSCRIBED);
			}
			else if (req.command == EXIT)
			{
				client.is_on = false;
				sockets->erase(sock_it);
				FD_CLR(sock_it, read_fds);

				ret = close(sock_it);
				DIE(ret < 0, "socket closing");

				fprintf(stdout, DISCONNECTED_FORMAT, client_id.c_str());
				fprintf(stdout, NL);

				continue;
			}
			else
			{
				strcpy(response.data.message, UNKNOWN_COMMAND);
			}

			send_whole_packet(sock_it, &response, sizeof(server_respond),
				"subscriber request respond", 1);
		}
	}
	return CONTINUE;
}

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	int sockfd_udp, sockfd_tcp, portno;
	struct sockaddr_in serv_addr;

	fd_set read_fds;
	int fd_max;

	if (argc < 2)
		usage(argv[0]);

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	init(sockfd_udp, sockfd_tcp, read_fds, serv_addr, portno, fd_max);

	// maps a client's id to his container
	unordered_map<string, Client> *clients
		= new unordered_map<string, Client>();

	// maps the socket through the client connected to his id
	unordered_map<int, string> *sockets
		= new unordered_map<int, string>();

	// retains every client subscribed to a topic and identifies them after id
	unordered_map<string, unordered_set<string>> *all_topics
		= new unordered_map<string, unordered_set<string>>();

	while (true) {
		if (STOP == process_requests(&read_fds, fd_max, sockfd_tcp, sockfd_udp,
							serv_addr, clients, sockets, all_topics))
			break;
	}

	delete clients;
	delete sockets;
	delete all_topics;
	return 0;
}
