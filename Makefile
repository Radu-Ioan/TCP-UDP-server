SOURCES = server.cpp subscriber.cpp
BIN = $(patsubst %.cpp,%,$(SOURCES))

CFLAGS = -Wall
CC = g++

IP_SERVER = 127.0.0.1
PORT = 10001

all: $(BIN) gates.o

.cpp.o:
	$(CC) $(CFLAGS) -c $<

subscriber: gates.o subscriber.o
	$(CC) $(CFLAGS) $^ -o $@
server: gates.o server.o
	$(CC) $(CFLAGS) $^ -o $@


run_server:
	./server ${PORT}
run_client:
	./subscriber $(id) ${IP_SERVER} ${PORT}
run_udp:
	python3 udp_client.py ${IP_SERVER} ${PORT}
manual:
	python3 udp_client.py --mode manual --delay 20 ${IP_SERVER} ${PORT}


test:
	sudo python3 test.py

clean:
	rm -rf $(BIN) *.o
