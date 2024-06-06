#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "../messages.cpp"

/*
1_ - sintaxe incorreta
2_ - socket
    20 erro ao criar socket
    21 erro ao conectar
    22 erro no bind
*/


int main(int argc, char** argv){
    if (argc != 3) {
        std::cout << SYNTAX_ERROR << "           USO CORRETO: " << argv[0] << " <IP/NOME MAQUINA> <PORTA>" << std::endl;
        exit(10);
    }

    // criar socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocket < 0) {
        perror(SOCKET_ERROR);
        printf("\n");
        exit(20);
    } 
  
    // especificar endereço
    sockaddr_in serverAddress; 
    serverAddress.sin_family = AF_INET; 
    serverAddress.sin_port = htons(atoi(argv[2])); 
    serverAddress.sin_addr.s_addr = INADDR_ANY;

  
    // conectar
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) != 0) {
        perror(CONNECT_ERROR);
        exit(21);
    }
  
    // closing socket 
    close(clientSocket); 

    return 0;
}