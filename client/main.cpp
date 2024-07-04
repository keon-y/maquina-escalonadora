#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
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

// Sends a file, returns -1 on failure
int sendFile(const char*, int);

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cout << "1_ - sintaxe incorreta           USO CORRETO: " << argv[0] << " <IP/NOME MAQUINA> <PORTA>" << std::endl;
        exit(10);
    }

    // Create socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        perror("20 - erro ao criar socket");
        printf("\n");
        exit(20);
    }

    // Specify address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(atoi(argv[2]));
    serverAddress.sin_addr.s_addr = inet_addr(argv[1]);

    // Connect
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) != 0) {
        perror("21 - erro ao conectar");
        exit(21);
    } else {
        std::cout << "success" << std::endl;
    }

    std::string filename;
    std::vector<CFile> files;

    while (true) {
        std::cout << "Enter file name (or press Enter to finish): ";
        std::getline(std::cin, filename);
        if (filename.empty()) {
            break;
        }
        std::string willCompile;
        std::cout << "Will it compile? (sim/nao): ";
        std::getline(std::cin, willCompile);

        std::string add = " ";
        if (willCompile == "sim" || willCompile == "SIM") {
            std::cout << "Additional compilation flags: ";
            std::getline(std::cin, add);
        }
        files.push_back(CFile(filename, add, willCompile));
    }

    std::cout << "left loop" << std::endl;

    for (const auto& file : files) {
    // Send file name
        char buffer[BUFFER_SIZE] = {0};
        strncpy(buffer, file.getFilename().c_str(), sizeof(buffer) - 1);
        send(clientSocket, buffer, sizeof(buffer), 0);

        // Send additional flags
        //memset(buffer, 0, sizeof(buffer));  // Clear buffer
        strncpy(buffer, file.getFlags().c_str(), sizeof(buffer) - 1);
        send(clientSocket,buffer, sizeof(buffer), 0);

        // Send compile flag
        //memset(buffer, 0, sizeof(buffer));  // Clear buffer
        strncpy(buffer, file.getCompile().c_str(), sizeof(buffer) - 1);
        send(clientSocket, buffer, sizeof(buffer), 0);

        // Send file content
        if (sendFile(file.getFilename().c_str(), clientSocket) == -1) {
            std::cout << "FAIL SENDING FILE: " << file.getFilename() << std::endl;
            close(clientSocket);
            exit(31);
        }
        std::cout << "sent file" << std::endl;
    }

    std::cout << "sent all files" << std::endl;
    // Send end of transmission indicator
    send(clientSocket, "EOT", 3, 0);

    char result[BUFFER_SIZE] = {0};
    for (auto &f : files) {
        int valread = read(clientSocket, result, BUFFER_SIZE);
        std::cout << "Compilation result for "  << f.getFilename() << ": " << result << std::endl;
    }
    // Close socket
    close(clientSocket);

    return 0;
}

int sendFile(const char* filename, int sock) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
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
