#include <iostream>
#include <fstream>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <filesystem>
#include <map>
#include "network_utils.hpp"


// dado um vetor de strings contendo nome de arquivos, envia todos para o socket sock
void sendFiles(const std::vector<std::string> &files, int sock)
{
    std::map<std::string, std::string> compilation_results;
    int sent_files = 0;
    for (const auto &filename : files)
    {
        char ch;
        std::string file_content;
        std::ifstream file(filename);
        if (!file)
        {
            std::cerr << filename << " nao foi encontrado" <<std::endl;
            continue;
        }

        while (file.get(ch)) // passar todo o arquivo para uma string, para enviar como uma mensagem
        {
            file_content += ch;
        }
        file.close();

        send_message(sock, filename);     // mandar primeiro o nome
        send_message(sock, file_content); // mandar o conteudo

        sent_files++;
    }

    send_message(sock, "");              // handshake para sinalizar que acabou o envio

    for (int i = 0; i < sent_files; i++) // receber as respostas de todos os arquivos fontes enviados
    {
        std::string compilation_result;
        std::string file_name;
        receive_message(sock, file_name);
        receive_message(sock, compilation_result);
        compilation_results.emplace(file_name, compilation_result);
    }
    for (const auto &[file_name, compilation_result] : compilation_results)
    { // listar em ordem de saida
        std::cout << compilation_result << std::endl;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Uso: " << argv[0] << " <ip_portal> <porta_portal>" << std::endl;
        return 1;
    }

    std::string gateway_ip = argv[1];
    int gateway_port = std::stoi(argv[2]);

    while (true)
    {
        int sock;
        if (( sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            std::cerr << "Erro ao criar socket" << std::endl;
            return 0;
        }

        struct sockaddr_in server_address;
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(gateway_port);

        if (inet_pton(AF_INET, gateway_ip.c_str(), &server_address.sin_addr) <= 0)
        {
            std::cerr << "Endereco invalido" << std::endl;
            close(sock);
            return 0;
        }
        if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
        {
            std::cerr << "Falha de conexao" << std::endl;
            close(sock);
            return 0;
        }
        std::string input;
        getline(std::cin, input);
        std::vector<std::string> files;
        if (input.at(0) == 'S' || input.at(0) == 's')
        {
            std::istringstream temp(input);
            std::string tok;
            while (getline(temp, tok, ' '))
            {
                files.push_back(tok);
            }
            files.erase(files.begin()); // remover o primeiro S
            sendFiles(files, sock);
            close(sock);
        }
        else if (input.at(0) == 'L' || input.at(0) == 'l')
        {
            bool empty = true;
            for (const auto &entry : std::filesystem::directory_iterator("./"))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".c")
                {
                    empty = false;
                    std::cout << entry.path().filename().string() << std::endl;
                }
            }
            if (empty)
                std::cout << "0" << std::endl;
        }
        else if (input.at(0) == 'Q' || input.at(0) == 'q') //sair
            break;

        std::cin.clear();
    }

    return 0;
}
