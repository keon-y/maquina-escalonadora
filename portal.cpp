#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <random>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

std::mutex mtx;
int round_robin_counter = 0;
#define BUFFER_SIZE 1024
#define HANDSHAKE "\r\n"

void handle_client(int client_sock, const std::vector<std::pair<std::string, int>> &server_list, bool use_round_robin)
{
    while (true)
    {
        int server_index;
        int bytes_received;
        int server_sock = 0;
        struct sockaddr_in serv_addr;
        char buffer[BUFFER_SIZE] = {0};

        if (use_round_robin)
        {
            std::lock_guard<std::mutex> lock(mtx);
            server_index = round_robin_counter % server_list.size();
            round_robin_counter++;
        }
        else
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, server_list.size() - 1);
            server_index = dis(gen);
        }
        round_robin_counter++;

        if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            std::cerr << "Socket creation error" << std::endl;
            return;
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(server_list[server_index].second);

        if (inet_pton(AF_INET, server_list[server_index].first.c_str(), &serv_addr.sin_addr) <= 0)
        {
            std::cerr << "Invalid address/ Address not supported\n";
            return;
        }

        if (connect(server_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            std::cerr << "Connection Failed\n";
            return;
        }

        // RECEBER O NOME DO ARQUIVO
        if ( (bytes_received = recv(client_sock, buffer, sizeof(buffer), 0)) < 0)
        {
            std::perror("Error reading filename");
            close(client_sock);
            close(server_sock);
            return;
        }
        //cliente so fechou a conexao, tranquilo
        else if ( bytes_received == 0)
        {
            close(client_sock);
            close(server_sock);
            break;
        }

        // MANDAR NOME PRO SERVER
        send(server_sock, buffer, sizeof(buffer), 0);

        // RECEBER CONTEUDO
        int total_bytes = 0;
        std::string received_string;

        bzero(buffer, BUFFER_SIZE);

        while (true)
        {

            bytes_received = recv(client_sock, buffer, BUFFER_SIZE, 0);
            std::string tempString; // para armazenar n bytes
            if (std::string(buffer).find(HANDSHAKE) != std::string::npos)
            { // tem EOT no final
                bytes_received -= strlen(HANDSHAKE);
            }

            std::string tempstring(buffer, bytes_received); // armazena buffer pra concatenar
            received_string += tempstring;
            if (bytes_received <= 0 || std::string(buffer).find(HANDSHAKE) != std::string::npos)
                break;
        }

        if (bytes_received == -1)
        {
            perror("recv");
            close(server_sock);
            return;
        }
        /////////////////////////////////////////////////////////

        // ENVIAR CONTEUDO PARA O SERVER

        const char *data_ptr = received_string.c_str();
        size_t bytes_remaining = received_string.size();


        while (bytes_remaining > 0)
        {
            int bytes_to_send = std::min<size_t>(bytes_remaining, BUFFER_SIZE);
            int bytes_sent = send(server_sock, data_ptr, bytes_to_send, 0);
            if (bytes_sent == -1)
            {
                perror("send");
                close(server_sock);
                return;
            }
            data_ptr += bytes_sent;
            bytes_remaining -= bytes_sent;
        }
        send(server_sock, HANDSHAKE, strlen(HANDSHAKE), 0);
        //////////////////////////////////////////////////////////

        // RECEBER RESPOSTA DO SERVER
        received_string = "";

        bzero(buffer, BUFFER_SIZE);

        while (true)
        {
            bytes_received = recv(server_sock, buffer, BUFFER_SIZE, 0);
            std::string tempString; // para armazenar n bytes
            if (std::string(buffer).find(HANDSHAKE) != std::string::npos)
            { // tem EOT no final
                bytes_received -= strlen(HANDSHAKE);
            }

            std::string tempstring(buffer, bytes_received); // armazena buffer pra concatenar
            received_string += tempstring;
            if (bytes_received <= 0 || std::string(buffer).find(HANDSHAKE) != std::string::npos)
                break;
        }

        if (bytes_received == -1)
        {
            perror("recv");
            close(server_sock);
            return;
        }

        //////////////////////////////////////////

        // ENVIAR RESPOSTA DE VOLTA PARA O CLIENTE
        const char *data2_ptr = received_string.c_str();
        bytes_remaining = received_string.size();

        while (bytes_remaining > 0)
        {
            int bytes_to_send = std::min<size_t>(bytes_remaining, BUFFER_SIZE);
            int bytes_sent = send(client_sock, data2_ptr, bytes_to_send, 0);
            if (bytes_sent == -1)
            {
                perror("send");
                close(client_sock);
                return;
            }
            data2_ptr += bytes_sent;
            bytes_remaining -= bytes_sent;
        }
        send(client_sock, HANDSHAKE, strlen(HANDSHAKE), 0);

        bzero(buffer, BUFFER_SIZE);
        /////////////////////////////////////////////////////////////////
        close(server_sock);
    }

    close(client_sock);
}

int main(int argc, char *argv[])
{
    if (argc != 8)
    {
        std::cerr << "Usage: " << argv[0] << " <round_robin> <ipserver1> <portserver1> <ipserver2> <portserver2> <ipserver3> <portserver3>\n";
        return 1;
    }

    bool use_round_robin = std::string(argv[1]) == "1";

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    std::vector<std::pair<std::string, int>> server_list = {
        {argv[2], std::stoi(argv[3])},
        {argv[4], std::stoi(argv[5])},
        {argv[6], std::stoi(argv[7])}};


    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        std::cerr << "Socket failed\n";
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        std::cerr << "Setsockopt failed\n";
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        std::cerr << "Bind failed\n";
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)
    {
        std::cerr << "Listen failed\n";
        exit(EXIT_FAILURE);
    }

    while (true)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            std::cerr << "Accept failed\n";
            exit(EXIT_FAILURE);
        }
        std::thread client_thread(handle_client, new_socket, server_list, use_round_robin);
        client_thread.detach();
    }

    return 0;
}
