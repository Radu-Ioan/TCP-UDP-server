#include "helpers.h"

#define COMMAND_SIZE 12
#define SUBSCRIBE_STRING "subscribe"
#define UNSUBSCRIBE_STRING "unsubscribe"
#define DATA_LEN 12

using namespace std;

static char *string_format(uint8_t type)
{
    static char data_type[DATA_LEN];
    switch (type) {
        case INT:
            strcpy(data_type, "INT");
            break;
        case SHORT_REAL:
            strcpy(data_type, "SHORT_REAL");
            break;
        case FLOAT:
            strcpy(data_type, "FLOAT");
            break;
        case STRING:
            strcpy(data_type, "STRING");
            break;
        default:
            break;
    }
    return data_type;
}

/* Displays the value from the content */
static void print_value(uint8_t type, char content[CONTENT_SIZE + 1])
{
    uint8_t sign = 0;
    uint32_t *a = NULL;
    uint8_t *offset = NULL;

    switch (type)
    {
        case INT:
        {
            sign = *((uint8_t *) content);
            offset = ((uint8_t *) content) + 1;
            a = ((uint32_t *) offset);
            *a = ntohl(*a);
            fprintf(stdout, "%d\n", (sign == 0) ? *a : -(*a));
            break;
        }
        case SHORT_REAL:
        {
            uint16_t *number_times_100 = ((uint16_t *) content);
            float real = ((float) ntohs(*number_times_100)) / ((float) 100);
            fprintf(stdout, "%.2f\n", sign == 0 ? real: -real);
            break;
        }
        case FLOAT:
        {
            sign = *((uint8_t *) content);
            a = ((uint32_t *) (((uint8_t *) content) + 1));
            *a = ntohl(*a);
            float n = ((float) (*a));
            const uint8_t power = *((uint8_t *) (((uint8_t *) content) + 5));

            for (int i = 0; i < power; i++)
                n = n / ((float) 10);

            if (!sign)
                fprintf(stdout, "%.4lf\n", n);
            else
                fprintf(stdout, "%.4lf\n", -n);
            break;
        }
        case STRING:
        {
            fprintf(stdout, "%s\n", content);
            break;
        }
        default:
        {
            fprintf(stdout, "\n");
            break;
        }
    }
}

/* Starts the communication with the server */
void run(const int sockfd, char id[ID_SIZE])
{
    char buf[BUFLEN], comm[COMMAND_SIZE], topic[TOPIC_SIZE];
    int SF;
    memset(buf, 0, BUFLEN);

    int ret;
    tcp_msg request;
    server_respond server_response;

    fd_set set;
    FD_ZERO(&set);
    FD_SET(sockfd, &set);
    FD_SET(STDIN_FILENO, &set);

    while (true) {
        fd_set tmp = set;
        ret = select(sockfd + 1, &tmp, NULL, NULL, NULL);
        DIE(ret < 0, "select in client");
        memset(&server_response, 0, sizeof(server_response));
        memset(buf, 0, BUFLEN);

        if (FD_ISSET(STDIN_FILENO, &tmp)) {
            fgets(buf, BUFLEN, stdin);
            sscanf(buf, "%s%s%d", comm, topic, &SF);

            strncpy(request.topic, topic, TOPIC_SIZE);

            if (strcmp(comm, SUBSCRIBE_STRING) == 0) {
                request.command = SUBSCRIBE;
                request.SF = SF == 1 ? SF_SET : SF_NULL;                
                send_whole_packet(sockfd, &request, sizeof(request),
                    "client request sending", 0);
            } else if (strcmp(comm, UNSUBSCRIBE_STRING) == 0) {
                request.command = UNSUBSCRIBE;
                send_whole_packet(sockfd, &request, sizeof(request),
                    "client request sending", 0);
            } else if (strcmp(comm, EXIT_STRING) == 0) {
                request.command = EXIT;
                send_whole_packet(sockfd, &request, sizeof(request),
                    "sending exit from client", 0);
                break;
            } else {
                printf(UNKNOWN_COMMAND NL);
            }
        }
        if (FD_ISSET(sockfd, &tmp)) {
            // a message from a topic or an acknowledgement has arrived or the
            // id cannot be used
            get_whole_packet(sockfd, &server_response, sizeof(server_response),
                "getting data from server", 0);

            if (server_response.type == STRING_DATA) {
                fprintf(stdout, "%s\n", server_response.data.message);

                if (strcmp(server_response.data.message, UNKNOWN_COMMAND) == 0)
                    continue;

                sprintf(buf, ALREADY_CONNECTED_MESSAGE, id);
                if (strcmp(server_response.data.message, buf) == 0)
                    break;

                sprintf(buf, SERVER_CLOSED);
                if (strcmp(server_response.data.message, buf) == 0)
                    break;
            } else if (server_response.type == UDP_DATA) {
                char content[CONTENT_SIZE + 1];
                memcpy(content,
                    server_response.data.info.notification.content,
                    CONTENT_SIZE);
                content[CONTENT_SIZE] = '\0';

                fprintf(stdout, NOTIFICATION_PREFIX,
                    inet_ntoa(server_response.data.info.ip_source),
                    server_response.data.info.portno,
                    server_response.data.info.notification.topic,
                    string_format(server_response.data.info.notification
                        .data_type));
                print_value(server_response.data.info.notification.data_type,
                    content);
            }
        }
    }
}

int main(int argc, char *argv[])
{
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    int sockfd, ret;
    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);
    memset(&serv_addr, 0, socket_len);

    if (argc != 4) {
        printf("Usage: %s <ID_CLIENT> <IP_SERVER> <PORT_SERVER>\n",
            argv[0]);
        return 1;
    }

    // create the tcp socket for the server's connection
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "could not create a socket!");

    // disable Nagle's algorithm
    int flag = 1;
    ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag,
            sizeof(int));
    if (ret < 0)
        fprintf(stderr, "Nagle's algorithm not disabled!\n");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));
    ret = inet_aton(argv[2], &(serv_addr.sin_addr));
    DIE(ret < 0, "client inet_aton");

    ret = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    DIE(ret < 0, "client connect");

    connect_request msg;
    strncpy(msg.id, argv[1], ID_SIZE);
    send_whole_packet(sockfd, &msg, sizeof(msg),
        "sendind id for connection", 0);

    run(sockfd, argv[1]);

    ret = close(sockfd);
    DIE(ret < 0, "closing socket from client");
    return 0;
}
