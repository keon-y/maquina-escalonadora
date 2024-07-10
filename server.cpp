#include <iostream>
#include <fstream>
#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "network_utils.hpp"

#define BUFFER_SIZE 1024

std::string compileFile(const char *filename)
{
    std::string command = "gcc ";
    command += filename;
    command += " -o ";
    command += filename;
    command += ".out";
    command += " 2>&1 && ./";
    command += filename;
    command += ".out 2>&1"; // redirecionar stderr para stdout

    char buffer[BUFFER_SIZE] = {0}; // PROBLEMA: se a saida > BUFFER_SIZE da problema
    std::string result = "";
    FILE *pipe = popen(command.c_str(), "r"); // abrir o pipe pra compilar e ver a saida
    if (!pipe)
        return "popen failed!";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        result += buffer;
    }
    pclose(pipe);

    return result;
}

void handle_client(int client_sock)
{
    std::string file_name;
    std::string file_content;

    receive_message(client_sock, file_name);

    std::ofstream outfile(file_name);
    if (!outfile)
    {
        // std::perror("Erro ao criar arquivo");
        close(client_sock);
        return;
    }

    receive_message(client_sock, file_content);
    outfile << file_content;

    outfile.close();

    std::string result = compileFile(file_name.c_str());

    send_message(client_sock, result);
    close(client_sock);

    // remover os arquivos depois pra nao ficar cheio de lixo
    remove(file_name.c_str());
    remove(file_name.append(".out").c_str());
}

void start_server(int port)
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        // std::cerr << "Falha ao criar socket" << std::endl;
        exit(EXIT_FAILURE);
    }

    // debug
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        // std::cerr << "Falha ao configurar socket" << std::endl;
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        // std::cerr << "Falha ao bindar" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0)
    {
        // std::cerr << "Falha no listen" << std::endl;
        exit(EXIT_FAILURE);
    }

    while (true)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            // std::cerr << "Falha ao aceitar socket" << std::endl;
            continue; // Em vez de sair, continue aceitando novas conexões
        }
        std::thread client_thread(handle_client, new_socket);
        client_thread.detach();
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Uso: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[1]);

    start_server(port);

    return 0;
}
