all: clean client portal server1 server2 server3

client: client.cpp
	g++ client.cpp network_utils.cpp -o client -lpthread

portal: portal.cpp
	g++ portal.cpp network_utils.cpp -o portal -lpthread

server1: server.cpp
	g++ server.cpp network_utils.cpp -o server1 -lpthread

server2: server.cpp
	g++ server.cpp network_utils.cpp -o server2 -lpthread

server3: server.cpp
	g++ server.cpp network_utils.cpp -o server3 -lpthread
	
clean:
	rm -f client portal server1 server2 server3 *.o *.out *.log *.result received_file_*.c

.PHONY: 
	all clean

serv1:
	./server1 8081 1

serv2:
	./server2 8082 2

serv3:
	./server3 8083 3

port:
	./portal 1 127.0.0.1 8081 127.0.0.1 8082 127.0.0.1 8083

cli:
	./client 127.0.0.1 8080