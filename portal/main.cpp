#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <cstring>
#include <vector>

#define BUFFER_SIZE 1024

class CFile {
public:
    CFile(std::string fn, std::string af, std::string c) : filename(fn), additional_flags(af), isCompile(c) {}
    std::string getFilename() const { return filename; }
    std::string getFlags() const { return additional_flags; }
    std::string getCompile() const { return isCompile; }
private:
    std::string filename;
    std::string additional_flags;
    std::string isCompile;
};

int receiveFile(const char*, int);
int sendFile(const char*, int);
std::string forwardToServer(const char*, const char*, const char*, const char*, const char*);

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <GATEWAY_PORT> <SERVER_IP> <SERVER_PORT>" << std::endl;
        exit(10);
    }

    // Create socket for gateway
    int gatewaySocket = socket(AF_INET, SOCK_STREAM, 0);
    if (gatewaySocket < 0) {
        perror("Socket creation error");
        exit(20);
    }

    // Bind gateway socket to port
    sockaddr_in gatewayAddress;
    gatewayAddress.sin_family = AF_INET;
    gatewayAddress.sin_port = htons(atoi(argv[1]));
    gatewayAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(gatewaySocket, (struct sockaddr*)&gatewayAddress, sizeof(gatewayAddress)) < 0) {
        perror("Bind error");
        exit(22);
    }

    if (listen(gatewaySocket, 5) < 0) {
        perror("Listen error");
        exit(30);
    }

    std::cout << "Gateway listening on port " << argv[1] << std::endl;

    while (true) {
        int clientSocket = accept(gatewaySocket, nullptr, nullptr);
        if (clientSocket < 0) {
            perror("Accept error");
            continue;
        }

        char buffer[BUFFER_SIZE] = {0};

        std::vector<CFile> cfiles;

        while(true) {
            memset(buffer, 0, BUFFER_SIZE); // Clear buffer
            int valread = read(clientSocket, buffer, BUFFER_SIZE);
            if(valread == 0) break; //cabo
            if (valread <= 0) {
                std::cerr << "Error reading filename from client" << std::endl;
                close(clientSocket);
                continue;
            }
            std::string filename = buffer;
            if (filename == "EOT") break; //end of transmission
            std::cout << "filename: " <<  filename << std::endl;

            valread = read(clientSocket, buffer, BUFFER_SIZE);
            if (valread <= 0) {
                std::cerr << "Error reading additional flags from client" << std::endl;
                close(clientSocket);
                continue;
            }
            std::string additional = buffer;
            std::cout << "additional: " <<  additional << std::endl;

            valread = read(clientSocket, buffer, BUFFER_SIZE);
            if (valread <= 0) {
                std::cerr << "Error reading compile flag from client" << std::endl;
                close(clientSocket);
                continue;
            }
            std::string isCompiling = buffer;
            std::cout << "isCompiling: " <<  isCompiling << std::endl;

            

            if (receiveFile(filename.c_str(), clientSocket) == -1) {
                std::cerr << "Error receiving file from client" << std::endl;
                close(clientSocket);
                continue;
            }
            std::cout << "Received filename: " << filename << std::endl;
            cfiles.push_back(CFile(filename, additional, isCompiling));

        }
        for (auto &cfile : cfiles) {
            std::string result = forwardToServer(argv[2], argv[3], cfile.getFilename().c_str(), cfile.getFlags().c_str(), cfile.getCompile().c_str());
            send(clientSocket, result.c_str(), result.size(), 0);
        }
        close(clientSocket);
    }

    close(gatewaySocket);
    return 0;
}

int receiveFile(const char* filename, int sock) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file\n";
        return -1;
    }

    char buffer[BUFFER_SIZE] = {0};
    int bytes_read;
    while ((bytes_read = read(sock, buffer, sizeof(buffer))) > 0) {
        std::string data(buffer, bytes_read);
        std::size_t eof_pos = data.find("EOF");
        if (eof_pos != std::string::npos) {
            file.write(data.c_str(), eof_pos); // Write only up to the EOF marker
            break;
        }
        file.write(data.c_str(), bytes_read);
    }

    file.close();
    return 0;
}

int sendFile(const char* filename, int sock) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file\n";
        return -1;
    }

    char buffer[BUFFER_SIZE] = {0};
    while (file.read(buffer, sizeof(buffer))) {
        send(sock, buffer, sizeof(buffer), 0);
    }

    if (file.gcount() > 0) {
        send(sock, buffer, file.gcount(), 0);
    }

    // Send EOF indicator
    send(sock, "EOF", 3, 0);

    file.close();
    return 0;
}

std::string forwardToServer(const char* serverIp, const char* serverPort, const char* localFilename, const char* additional, const char* isCompiling) {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        return "Server socket creation error";
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(atoi(serverPort));
    serverAddress.sin_addr.s_addr = inet_addr(serverIp);

    if (connect(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        close(serverSocket);
        return "Error connecting to server";
    }

    char buffer[BUFFER_SIZE] = {0};

    // Send filename
    strncpy(buffer, localFilename, sizeof(buffer) - 1);
    send(serverSocket, buffer, sizeof(buffer), 0);
    std::cout << "Sent filename " <<localFilename << " with size " << sizeof(localFilename) << std::endl; 

    // Send additional flags
    strncpy(buffer, additional, sizeof(buffer) - 1);
    send(serverSocket, buffer, sizeof(buffer), 0);
    std::cout << "Sent add " <<additional<< " with size " << sizeof(additional) << std::endl; 


    // Send compile flag
    strncpy(buffer, isCompiling, sizeof(buffer) - 1);
    send(serverSocket, buffer, sizeof(buffer), 0);
    std::cout << "Sent compileflag " <<isCompiling << " with size " << sizeof(isCompiling) << std::endl; 
     // Send file content
    if (sendFile(localFilename, serverSocket) == -1) {
        close(serverSocket);
        return "Error sending file to server";
    }
    std::cout << "sent file to server" << std::endl;

    // Receive response from server
    char result[BUFFER_SIZE] = {0};
    int valread = read(serverSocket, result, BUFFER_SIZE);
    if (valread < 0) {
        close(serverSocket);
        return "Error reading from server";
    }

    close(serverSocket);
    return std::string(result);
}
