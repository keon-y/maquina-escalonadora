#ifndef NETWORK_UTILS_HPP
#define NETWORK_UTILS_HPP

#include <string>

// Função auxiliar para enviar todos os dados garantindo que todos os bytes sejam enviados
int send_all(int socket, const char* buffer, int length);

// Função para enviar uma mensagem
int send_message(int socket, const std::string& message);

// Função auxiliar para receber dados garantindo que todos os bytes sejam recebidos
int recv_all(int socket, char* buffer, int length);

// Função para receber uma mensagem
int receive_message(int sock, std::string &message);

#endif // NETWORK_UTILS_HPP
