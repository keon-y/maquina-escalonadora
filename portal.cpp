#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <random>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <map>
#include "network_utils.hpp"

std::mutex mtx;
int round_robin_counter = 0;

void send_server(const std::string &file_name,
                 const std::string &file_content,
                 std::map<std::string, std::string> &responses,
                 const std::vector<std::pair<std::string, int>> &server_list,
                 bool use_round_robin)
{
    int server_sock = 0;
    int server_index;
    struct sockaddr_in serv_addr;
    std::string response;

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

    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        std::cerr << "Erro ao criar socket" << std::endl;
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_list[server_index].second);

    if (inet_pton(AF_INET, server_list[server_index].first.c_str(), &serv_addr.sin_addr) <= 0)
    {
        std::cerr << "Endereco invalido" << std::endl;
        close(server_sock);
        return;
    }

    if (connect(server_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        std::cerr << "Falha de conexao" << std::endl;
        close(server_sock);
        return;
    }
    //manda as informacoes pro server
    if (send_message(server_sock, file_name) <= 0)
        return;
    if (send_message(server_sock, file_content) <= 0)
        return;

    //recebe a resposta
    receive_message(server_sock, response);

    // atualiza no map
    {
        std::lock_guard<std::mutex> lock(mtx);
        responses.emplace(file_name, response);
    }

    close(server_sock);
}

void handle_client(int client_sock, const std::vector<std::pair<std::string, int>> &server_list, bool use_round_robin)
{
    std::vector<std::thread> server_threads;
    std::map<std::string, std::string> responses;
    int id = 0;
    while (true)
    {
        //recebe o nome do arquivo, se tamanho for 0, significa que o client deu o handshake e acabou os arquivos
        std::string file_name;
        if (receive_message(client_sock, file_name) <= 0)
        {
            break;
        }
        file_name = std::to_string(id) + file_name; // se enviar varios arquivos iguais, evita que o map de bug
        id++;

        // recebe o conteudo
        std::string file_content;
        if (receive_message(client_sock, file_content) <= 0)
        {
            break;
        }

        server_threads.emplace_back(send_server, file_name, file_content, std::ref(responses), std::cref(server_list), use_round_robin);
    }

    for (auto &server_thread : server_threads) //junta as threads para enviar tudo junto pro cliente
    {
        if (server_thread.joinable())
        {
            server_thread.join();
        }
    }

    for (const auto &[file_name, response] : responses)
    {
        if (send_message(client_sock, file_name) < 0) return;
        if (send_message(client_sock,  response) < 0) return;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 9)
    {
        std::cerr << "Usage: " << argv[0] << "<porta> <round_robin> <ipserver1> <portserver1> <ipserver2> <portserver2> <ipserver3> <portserver3>\n";
        return 1;
    }

    bool use_round_robin = std::string(argv[2]) == "1";

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    std::vector<std::pair<std::string, int>> server_list = {
        {argv[3], std::stoi(argv[4])},
        {argv[5], std::stoi(argv[6])},
        {argv[7], std::stoi(argv[8])}};

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        std::cerr << "Falha ao criar socket" << std::endl;
        exit(EXIT_FAILURE);
    }

    //debug tava muito ruim de ficar reiniciando toda hora e mudar de port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        std::cerr << "Falha ao configurar opcoes do socket" << std::endl;
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(atoi(argv[1]));

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        std::cerr << "Falha ao bindar" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)
    {
        std::cerr << "Falha no listen" << std::endl;
        exit(EXIT_FAILURE);
    }

    while (true)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            std::cerr << "Falha ao aceitar socket" << std::endl;
            exit(EXIT_FAILURE);
        }
        std::thread client_thread(handle_client, new_socket, server_list, use_round_robin);
        client_thread.detach();
        //o cliente que fecha o socket, nao precisa do close aqui
    }
    return 0;
}
