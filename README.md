# About the program
    This is a platform which has 3 components: a server, a set of udp clients
    and a set of tcp clients.
    The server forwards packets with data received from the udp clients - which
    are the providers of information - to the tcp clients which act as subscribed
    clients to some topics of the udp clients.

# Server
    After the server starts, the tcp clients can send connection requests
    along with an id. If the id is available, the client can continue to
    communicate with the server, otherwise is closed if another client with
    same id is online (and the server announces him that the id is taken).

# Subscriptions
    The clients may choose 2 types of subscription:
        * the first type assumes that when the client is disconnected and data
        about a topic is sent, it will be forwarded to the client when he is
        connected again;
        * the second one assumes that a client receives data about a subscribed
        topic only when the information has been sent when he was online.

# Packets format
    When a packet from an udp client arrives at a tcp client, the subscriber
    displays a string with the following format:
    <SENDER_IP>:<SENDER_PORT> - <TOPIC_NAME> - <TYPE> - <VALUE>
    where SENDER_IP is the IP address of the udp client who transmitted the
    packet and SENDER_PORT is the port through the packet was sent, TOPIC_NAME
    represents the title of the topic to which the client was subscribed, TYPE
    can take a value of INT, SHORT_REAL, FLOAT or STRING, and VALUE is the content coresponding to TYPE.

    Examples:
    127.0.0.1:12321 - EURO2020/group-F/France-Germany - STRING - Benzema goal canceled for offside
    55.44.22.11:30000 - world-temperature/Romania/Bucharest - FLOAT - 20.0

    The topic name cannot contain whitespaces.
    The repository contains a demo udp client - developed using python - which has some illustrative topics.

# How to use
    To run the program, the server port must be set in the run script. It has
    an initial value of 12321. After that, the server must be turned on first using the command:
    $./run server

    In another terminal, a tcp client can be connected to the server using:
    $./run subscriber <ID>
    $# or
    $./run client <ID>

    To run an udp client, the script provides 2 ways: a manual run, and
    an all once mode. For manual mode enter:
    $./run udp manual
    $# or
    $./run udp_client.py manual

    For the simple mode:
    $./run udp
    $# or
    $./run udp_client.py

    After the server is started and a subscriber made a connection with it,
    he can subscribe to a topic using:
    subscribe <TOPIC> <SF>
    where TOPIC is the title of the topic about which he will be informed and
    SF comes from store-and-forward and can be 0 if the client does not want to
    receive the messages (related to the topic obviously) transmited in the time
    when he was offline or 1 otherwise. A client receives only the messages sent
    after his first connection.
    To unsubscribe from a topic, the subscriber enters:
    unsubscribe <TOPIC>
    If the client enters several times in a row a subscribe command, the last SF
    value is considered.
    To disconnect from the server, a subscriber can enter 'exit'. Also, when the
    server gets 'exit' from input, all the clients get informed and become closed.
    The upd_client.py is just a demo provider for topics.
