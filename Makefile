all: clean client portal server1 server2 server3

client: client.cpp
	g++ client.cpp -o client -lpthread

portal: portal.cpp
	g++ portal.cpp -o portal -lpthread

server1: server.cpp
	g++ server.cpp -o server1 -lpthread

server2: server.cpp
	g++ server.cpp -o server2 -lpthread

server3: server.cpp
	g++ server.cpp -o server3 -lpthread
	
clean:
	rm -f client portal server1 server2 server3 *.o *.out *.log *.result received_file_*.c

.PHONY: 
	all clean