#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdio>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <sstream>
#include <filesystem>

#define BUFFER_SIZE 1024
#define HANDSHAKE "\r\n"
//dado um vetor de strings contendo nome de arquivos, envia todos para o socket sock
void sendFiles(const std::vector<std::string> &files, int sock)
{
    for (const auto &filename : files)
    {
        char buffer[BUFFER_SIZE] = {0};
        std::string fileContent = "";
        size_t bytes_remaining;
        char ch;
        const char *data_ptr;
        int bytes_to_send;
        int bytes_sent;
        int bytes_received;
        std::string received_string;

        /*   abrir arquivo e passar o conteudo dele para uma string    */
        std::ifstream file(filename);
        if (!file)
        {
            std::cerr << filename << " nao encontrado" << std::endl;
            continue;
        }

    
        while (file.get(ch))
        {
            fileContent += ch;
        }
        file.close();
        //////////////////////////////////////////////////////////////////


        /*        enviar o arquivo para o portal           */

        //primeiro enviar o nome ele
        strncpy(buffer, filename.c_str(), BUFFER_SIZE - 1);
        if (send(sock, buffer, BUFFER_SIZE, 0) < 0)
        {
            std::cerr << "Nao foi possivel enviar o nome do arquivo" << std::endl;
            continue;
        }

    
        data_ptr = fileContent.c_str(); //ponteiro que aponta para o comeco da string que sera enviada
        bytes_remaining = fileContent.size();// quantidade de bytes que faltam para serem enviados

        while (bytes_remaining > 0)
        {
            bytes_to_send = std::min<size_t>(bytes_remaining, BUFFER_SIZE); //como o limite de bytes sao BUFFER_SIZE bytes, ver quem eh menor
            bytes_sent = send(sock, data_ptr, bytes_to_send, 0);
            if (bytes_sent == -1)
            {
                std::cerr << "Erro ao enviar conteudo do arquivo " << filename << " para o portal" << std::endl;
                return;
            }
            data_ptr += bytes_sent;
            bytes_remaining -= bytes_sent;
        }

        if (send(sock, HANDSHAKE, sizeof(HANDSHAKE), 0) <= 0 ) {
            std::cerr << "Nao foi possivel enviar o handshake" << std::endl;
            return;
        }
        //////////////////////////////////////////////////////////////////////


        /*      receber a resposta do portal       */
        bytes_received = 0;
        while (true)
        {
            bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
            std::string tempString; // para armazenar n bytes
            if (std::string(buffer).find(HANDSHAKE) != std::string::npos)
            { 
                bytes_received -= strlen(HANDSHAKE); //remover os bytes do handshake
            }

            std::string tempstring(buffer, bytes_received); // armazena buffer pra concatenar
            received_string += tempstring;
            if (bytes_received <= 0 || std::string(buffer).find(HANDSHAKE) != std::string::npos)
                break;
        }

        if (bytes_received == -1)
        {
            std::cerr << "Nao foi possivel enviar o handshake" << std::endl;
            return;
        }
        //////////////////////////////////////////////


        std::cout << received_string << std::endl;
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

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
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
    while (true)
    {
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
        else if (input.at(0) == 'Q' || input.at(0) == 'q') break;
        

        std::cin.clear();
    }
    close(sock);

    return 0;
}
