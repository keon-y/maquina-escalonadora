#include "network_utils.hpp"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <cstdint>

//enviar um buffer de tamanho length para o socket
int send_all(int socket, const char* buffer, int length) {
    int total_sent = 0;
    while (total_sent < length) {
        int n = send(socket, buffer + total_sent, length - total_sent, 0); //se ja enviou uma parte do buffer, fazer aritmetica de ponteiro para nao enviar repetido
        if (n == -1) { 
            return -1; // erro
        }
        total_sent += n;
    }
    return total_sent;
}

// envia uma string para um socket
int send_message(int socket, const std::string& message) {
    uint32_t message_size = message.size();

    // primeiro avisa a quantidade de bytes que vai na mensagem (tamanho fixo de 4 bytes de header)
    if (send_all(socket, reinterpret_cast<const char*>(&message_size), sizeof(message_size)) == -1) {
        std::cerr << "Erro ao enviar o tamanho da mensagem." << std::endl; 
        return -1;
    }

    // envia a mensagem
    if (send_all(socket, message.c_str(), message_size) == -1) {
        std::cerr << "Erro ao enviar a mensagem." << std::endl;
        return -1;
    }

    return 1;
}

// recebe um buffer de tamanho length pelo socket
int recv_all(int socket, char* buffer, int length) {
    int total_received = 0;
    while (total_received < length) {
        int n = recv(socket, buffer + total_received, length - total_received, 0); //mesmo esquema da outra funcao, se ja recebeu algo precisa de aritmetica de ponteiro
        if (n == -1) { 
            return -1;
        } else if (n == 0) { //fechou a conexao
            break; 
        }
        total_received += n;
    }
    return total_received; 
}

// recebe uma string message pelo socket
int receive_message(int sock, std::string &message) {
    uint32_t message_size;
    
    // recebe o header fixo de 4 bytes que diz quantos bytes vem
    if (recv_all(sock, reinterpret_cast<char*>(&message_size), sizeof(message_size)) <= 0) {
        std::cerr << "Erro ao receber o tamanho da mensagem." << std::endl;
        return -1;
    }

    if (message_size == 0) {
        return 0; //handshake ou conexao fechada, importante saber para tratar no portal sem erros
    }

    // recebe a mensagem de tamanho message_size
    char buffer[10000] = {0}; // chutei um valor alto, o ideal seria colocar 2e32 visto que eh o tamanho maximo da mensagem, mas para um trabalho 10000 ta de otimo tamanho
    if (recv_all(sock, buffer, message_size) <= 0) {
        std::cerr << "Erro ao receber a mensagem." << std::endl;
        return -1;
    }
    buffer[message_size] = '\0'; // coloca o \0 no final da string (provavelmente o construtor ja faz isso mas sao 11:06 do dia 09/07 e estou com preguica de testar desculpa professor!)

    message = std::string(buffer, message_size);

    return 1; // Sucesso
}
