#include "udp.h"

#include <stdio.h>
#include <string.h>

#include "app_config.h"
#include "input_utils.h"
#include "platform.h"

int run_udp_server(int port)
{
    socket_t server_socket = INVALID_SOCKET_VALUE;
    struct sockaddr_in server_address = {0};
    struct sockaddr_in client_address;
    address_length_t client_size;
    char buffer[MAX_MESSAGE_SIZE];
    int status = 1;

    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket == INVALID_SOCKET_VALUE ||
        set_reuse_address(server_socket) < 0) {
        print_socket_error("socket");
        goto cleanup;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons((unsigned short)port);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(server_socket, (struct sockaddr *)&server_address,
             sizeof(server_address)) < 0) {
        print_socket_error("bind");
        goto cleanup;
    }

    printf("UDP server %d portunu dinliyor.\n", port);
    for (;;) {
        int received;
        client_size = sizeof(client_address);
        received = recvfrom(server_socket, buffer, sizeof(buffer) - 1, 0,
                            (struct sockaddr *)&client_address, &client_size);
        if (received < 0) {
            print_socket_error("recvfrom");
            break;
        }
        buffer[received] = '\0';
        printf("Client: %s\n", buffer);
        if (strcmp(buffer, "exit") == 0 ||
            !read_line("Server: ", buffer, sizeof(buffer))) {
            status = 0;
            break;
        }
        if (sendto(server_socket, buffer, (int)strlen(buffer) + 1, 0,
                   (struct sockaddr *)&client_address, client_size) < 0) {
            print_socket_error("sendto");
            break;
        }
        if (strcmp(buffer, "exit") == 0) {
            status = 0;
            break;
        }
    }

cleanup:
    if (server_socket != INVALID_SOCKET_VALUE) CLOSE_SOCKET(server_socket);
    printf("UDP server kapatildi.\n");
    return status;
}

int run_udp_client(const char *ip_address, int port)
{
    socket_t client_socket = INVALID_SOCKET_VALUE;
    struct sockaddr_in server_address = {0};
    char buffer[MAX_MESSAGE_SIZE];
    int status = 1;

    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket == INVALID_SOCKET_VALUE) {
        print_socket_error("socket");
        goto cleanup;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons((unsigned short)port);
    if (inet_pton(AF_INET, ip_address, &server_address.sin_addr) != 1) {
        fprintf(stderr, "Gecersiz IP adresi.\n");
        goto cleanup;
    }
    printf("UDP client %s:%d adresini kullaniyor.\n", ip_address, port);

    while (read_line("Client: ", buffer, sizeof(buffer))) {
        int received;
        if (sendto(client_socket, buffer, (int)strlen(buffer) + 1, 0,
                   (struct sockaddr *)&server_address,
                   sizeof(server_address)) < 0) {
            print_socket_error("sendto");
            break;
        }
        if (strcmp(buffer, "exit") == 0) {
            status = 0;
            break;
        }
        received = recvfrom(client_socket, buffer, sizeof(buffer) - 1,
                            0, NULL, NULL);
        if (received < 0) {
            print_socket_error("recvfrom");
            break;
        }
        buffer[received] = '\0';
        printf("Server: %s\n", buffer);
        if (strcmp(buffer, "exit") == 0) {
            break;
        }
    }
    status = 0;

cleanup:
    if (client_socket != INVALID_SOCKET_VALUE) CLOSE_SOCKET(client_socket);
    printf("UDP client kapatildi.\n");
    return status;
}
