#include <iostream>
#include <fstream>
#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>

#define BUFFER_SIZE 1024
#define HANDSHAKE "\r\n"

std::string compileFile(const char* filename) {
    std::string command = "gcc ";
    command += filename;
    command += " -o ";
    command += filename;
    command += ".out";
    command += " 2>&1 && ./";
    command += filename;
    command += ".out 2>&1";  // Redirect stderr to stdout for both compilation and execution

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

void handle_client(int client_sock, int server_id) {
    char buffer[BUFFER_SIZE] = {0};

    if (recv(client_sock, buffer, sizeof(buffer), 0) <= 0) {
        std::perror("Error reading filename");
        close(client_sock);
        return;
    }

    std::string filename = std::to_string(server_id) + buffer;
    bzero(buffer, BUFFER_SIZE);

    std::ofstream outfile(filename);
    if (!outfile) {
        std::perror("File creation error");
        close(client_sock);
        return;
    }

    int bytes_received;
    std::string received_string;

    while (true) {
        bytes_received = recv(client_sock, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            break;
        }
        if (std::string(buffer).find(HANDSHAKE) != std::string::npos) { // tem EOT no final
            outfile.write(buffer, bytes_received - strlen(HANDSHAKE)); // tirar o tamanho do HANDSHAKE
            break;
        } else {
            outfile.write(buffer, bytes_received);
        }
        bzero(buffer, BUFFER_SIZE);
    }

    if (bytes_received == -1) {
        perror("recv");
        close(client_sock);
        return;
    }

    outfile.close();

    std::string result = compileFile(filename.c_str());


    const char* data_ptr = result.c_str();
    size_t bytes_remaining = result.size();
    while (bytes_remaining > 0) {
        int bytes_to_send = std::min<size_t>(bytes_remaining, BUFFER_SIZE);
        int bytes_sent = send(client_sock, data_ptr, bytes_to_send, 0);
        if (bytes_sent == -1) {
            perror("send");
            close(client_sock);
            return;
        }
        data_ptr += bytes_sent;
        bytes_remaining -= bytes_sent;
    }

    send(client_sock, HANDSHAKE, strlen(HANDSHAKE), 0);
    close(client_sock);
    remove(filename.c_str());
    remove(filename.append(".out").c_str());
}

void start_server(int port, int server_id) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Socket failed\n";
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        std::cerr << "Setsockopt failed\n";
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed\n";
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        std::cerr << "Listen failed\n";
        exit(EXIT_FAILURE);
    }

    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            std::cerr << "Accept failed\n";
            continue; // Em vez de sair, continue aceitando novas conexões
        }
        std::thread client_thread(handle_client, new_socket, server_id);
        client_thread.detach();
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <server_id>\n";
        return 1;
    }

    int port = std::stoi(argv[1]);
    int server_id = std::stoi(argv[2]);

    start_server(port, server_id);

    return 0;
}
