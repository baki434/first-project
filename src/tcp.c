#include "tcp.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_config.h"
#include "input_utils.h"
#include "platform.h"

static int send_all(socket_t socket_value, const char *data, size_t size)
{
    size_t sent_total = 0;

    while (sent_total < size) {
        int sent = send(socket_value, data + sent_total,
                        (int)(size - sent_total), 0);
        if (sent <= 0) {
            print_socket_error("send");
            return 0;
        }
        sent_total += (size_t)sent;
    }
    return 1;
}

static int receive_all(socket_t socket_value, char *data, size_t size)
{
    size_t received_total = 0;

    while (received_total < size) {
        int received = recv(socket_value, data + received_total,
                            (int)(size - received_total), 0);
        if (received <= 0) {
            return received;
        }
        received_total += (size_t)received;
    }
    return 1;
}

static int send_message(socket_t socket_value, const char *message)
{
    uint32_t length = (uint32_t)strlen(message);
    uint32_t network_length = htonl(length);

    return send_all(socket_value, (const char *)&network_length,
                    sizeof(network_length)) &&
           send_all(socket_value, message, length);
}

static int receive_message(socket_t socket_value, char *message, size_t size)
{
    uint32_t network_length;
    uint32_t length;
    int result = receive_all(socket_value, (char *)&network_length,
                             sizeof(network_length));

    if (result <= 0) {
        return result;
    }
    length = ntohl(network_length);
    if (length >= size) {
        fprintf(stderr, "Mesaj cok uzun.\n");
        return -1;
    }
    result = receive_all(socket_value, message, length);
    if (result <= 0) {
        return result;
    }
    message[length] = '\0';
    return 1;
}

int run_tcp_server(int port)
{
    socket_t server_socket = INVALID_SOCKET_VALUE;
    socket_t client_socket = INVALID_SOCKET_VALUE;
    struct sockaddr_in address = {0};
    char buffer[MAX_MESSAGE_SIZE];
    int status = 1;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET_VALUE ||
        set_reuse_address(server_socket) < 0) {
        print_socket_error("socket");
        goto cleanup;
    }

    address.sin_family = AF_INET;
    address.sin_port = htons((unsigned short)port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) < 0 ||
        listen(server_socket, 1) < 0) {
        print_socket_error("bind/listen");
        goto cleanup;
    }

    printf("TCP server %d portunu dinliyor.\n", port);
    client_socket = accept(server_socket, NULL, NULL);
    if (client_socket == INVALID_SOCKET_VALUE) {
        print_socket_error("accept");
        goto cleanup;
    }
    printf("TCP client baglandi.\n");

    while (receive_message(client_socket, buffer, sizeof(buffer)) > 0) {
        printf("Client: %s\n", buffer);
        if (strcmp(buffer, "exit") == 0 ||
            !read_line("Server: ", buffer, sizeof(buffer)) ||
            !send_message(client_socket, buffer) ||
            strcmp(buffer, "exit") == 0) {
            break;
        }
    }
    status = 0;

cleanup:
    if (client_socket != INVALID_SOCKET_VALUE) CLOSE_SOCKET(client_socket);
    if (server_socket != INVALID_SOCKET_VALUE) CLOSE_SOCKET(server_socket);
    printf("TCP server kapatildi.\n");
    return status;
}

int run_tcp_client(const char *ip_address, int port)
{
    socket_t client_socket = INVALID_SOCKET_VALUE;
    struct sockaddr_in address = {0};
    char buffer[MAX_MESSAGE_SIZE];
    int status = 1;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET_VALUE) {
        print_socket_error("socket");
        goto cleanup;
    }

    address.sin_family = AF_INET;
    address.sin_port = htons((unsigned short)port);
    if (inet_pton(AF_INET, ip_address, &address.sin_addr) != 1) {
        fprintf(stderr, "Gecersiz IP adresi.\n");
        goto cleanup;
    }
    if (connect(client_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        print_socket_error("connect");
        goto cleanup;
    }
    printf("TCP client %s:%d adresine baglandi.\n", ip_address, port);

    while (read_line("Client: ", buffer, sizeof(buffer))) {
        if (!send_message(client_socket, buffer) || strcmp(buffer, "exit") == 0) {
            break;
        }
        if (receive_message(client_socket, buffer, sizeof(buffer)) <= 0) {
            break;
        }
        printf("Server: %s\n", buffer);
        if (strcmp(buffer, "exit") == 0) {
            break;
        }
    }
    status = 0;

cleanup:
    if (client_socket != INVALID_SOCKET_VALUE) CLOSE_SOCKET(client_socket);
    printf("TCP client kapatildi.\n");
    return status;
}
