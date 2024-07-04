#include <iostream>
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fstream>
#include <cstring>
#include <vector>
#include <map>

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
std::string compileFile(const char*, const char*);

int main(int argc, char **argv) {

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <PORTA>" << std::endl;
        exit(10);
    }

    // Create socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("Socket error");
        exit(20);
    }

    // Address configuration
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(atoi(argv[1]));
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Bind socket to address
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) != 0) {
        perror("Bind error");
        exit(22);
    }

    // Listen for incoming connections
    if (listen(serverSocket, 5) == -1) {
        perror("Listen error");
        exit(30);
    }

    std::cout << "Server listening on port " << argv[1] << std::endl;

    while (true) {
        // Accept incoming connection
        sockaddr_in clientAddress;
        int clientSize = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, (socklen_t*)&clientSize);
        if (clientSocket < 0) {
            perror("Accept error");
            continue;
        }

        std::cout << "Client connected" << std::endl;
        std::vector<CFile> cfiles;
        std::map<std::string, std::string> results;

        while(true) {
        // Receive filename
            char buffer[BUFFER_SIZE] = {0};
            if (read(clientSocket, buffer, BUFFER_SIZE) < 0) {
                std::cerr << "Error reading filename from client" << std::endl;
                close(clientSocket);
                continue;
            }
            std::string filename = buffer;
            std::cout << "Filename received: " << filename << std::endl;

            // Read additional flags
            if (read(clientSocket, buffer, BUFFER_SIZE) < 0) {
                std::cerr << "Error reading additional flags from client" << std::endl;
                close(clientSocket);
                continue;
            }
            std::string additional_flags = buffer;
            std::cout << "Additional flags received: " << additional_flags << std::endl;

            // Read compilation flag
            if (read(clientSocket, buffer, BUFFER_SIZE) < 0) {
                std::cerr << "Error reading compilation flag from client" << std::endl;
                close(clientSocket);
                continue;
            }
            std::string isCompiling = buffer;
            std::cout << "Compile flag received: " << isCompiling << std::endl;



            // Receive file content
            if (receiveFile(filename.c_str(), clientSocket) == -1) {
                std::cerr << "Error receiving file from client" << std::endl;
                close(clientSocket);
                continue;
            }

            std::string result = "File received successfully";
            std::cout << "Received filename: " << filename << std::endl;

            cfiles.push_back(CFile(filename, additional_flags, isCompiling));
        }
        // Compile file if necessary
        for (auto &cfile : cfiles) {
            std::string result = "";
            if (cfile.getCompile() == "sim") {
                std::cout << "Compilando: " << cfile.getFilename() << std::endl;
                result = compileFile(cfile.getFilename().c_str(), cfile.getFlags().c_str());
            }
            results[cfile.getFilename()] = result;
        }

        // Send result back to client through the portal
        for (auto &result : results) {
            char buffer[BUFFER_SIZE] = {0};
            strncpy(buffer, result.second.c_str(), sizeof(buffer) - 1 );
            send(clientSocket, buffer, sizeof(buffer), 0);
        }

        close(clientSocket);
    }

    close(serverSocket);
    return 0;
}

int receiveFile(const char* filename, int sock) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file" << std::endl;
        return -1;
    }

    char buffer[BUFFER_SIZE] = {0};
    int bytes_read;
    while ((bytes_read = read(sock, buffer, BUFFER_SIZE)) > 0) {
        std::string data(buffer, bytes_read);
        std::size_t eof_pos = data.find("EOF");
        if (eof_pos != std::string::npos) {
            file.write(data.c_str(), eof_pos); // Write only up to the EOF marker
            break;
        }
        file.write(buffer, bytes_read);
    }

    file.close();
    return 0;
}

std::string compileFile(const char* filename, const char* additional) {
    std::cout << "Compiling " << filename << " with additional flags: " << additional << std::endl;
    std::string command = "gcc ";
    command += filename;
    command += " -o ";
    command += filename;
    command += ".out";
    command += " 2>&1 && ./program 2>&1 ";  // Redirect stderr to stdout for both compilation and execution
    command += additional;

    char buffer[BUFFER_SIZE] = {0};
    std::string result = "";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return "popen failed!";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);

    return result;
}
