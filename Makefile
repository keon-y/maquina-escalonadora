all: clean client portal server

client: client.cpp
	g++ client.cpp network_utils.cpp -o client.out

portal: portal.cpp
	g++ portal.cpp network_utils.cpp -o portal.out

server: server.cpp
	g++ server.cpp network_utils.cpp -o server.out
	
clean:
	rm -f *.out

.PHONY: 
	all clean

serv1:
	./server.out 8081 1

serv2:
	./server.out 8082 2

serv3:
	./server.out 8083 3

port:
	./portal.out 8088 1 127.0.0.1 8081 127.0.0.1 8082 127.0.0.1 8083

cli:
	./client.out 127.0.0.1 8080