
#include <string> 
#include <iostream> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <unistd.h>
#include "../messages.cpp"
  
using namespace std; 
  
int main(int argc, char **argv) 
{ 

    if (argc != 2) {
        std::cout << SYNTAX_ERROR << "           USO CORRETO: " << argv[0] << " <PORTA>" << std::endl;
        exit(10);
    }


    // criar socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0); 
    if (serverSocket < 0) {
        perror(SOCKET_ERROR);
        exit(20);
    }

    // endereço
    sockaddr_in serverAddress; 
    serverAddress.sin_family = AF_INET; 
    serverAddress.sin_port = htons(atoi(argv[1])); 
    serverAddress.sin_addr.s_addr = INADDR_ANY; 
  
    // binding socket. 
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) != 0) {
        perror(BIND_ERROR);
        exit(22);
    }
  
    // listening to the assigned socket 
    if(listen(serverSocket, 5) == -1) {
        perror(LISTEN_ERROR);
        exit(30);
    }

    std::cout << SUCCESS_LISTENING << argv[1] << std::endl;
  
    // accepting connection request 
    int clientSocket = accept(serverSocket, nullptr, nullptr);
    if (clientSocket < 0) {
        perror(CLIENT_ERROR);
        exit(31);
    }
    else {
        std::cout << "Cliente ouvido na porta " << argv[1] <<std::endl;
    }
  
    // closing the socket. 
    close(serverSocket); 
  
    return 0; 
}
