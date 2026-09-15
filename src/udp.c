#include "udp.h"

#include <stdio.h>
#include <stdatomic.h>
#include <string.h>

#include "app_config.h"
#include "input_utils.h"
#include "platform.h"
#include "thread_utils.h"

typedef struct {
    socket_t socket_value;
    atomic_int running;
    const char *peer_label;
    const char *prompt;
    struct sockaddr_in peer_address;
    address_length_t peer_size;
} udp_chat_t;

static void *receive_messages(void *argument)
{
    udp_chat_t *chat = argument;
    char buffer[MAX_MESSAGE_SIZE];

    while (atomic_load(&chat->running)) {
        int received = recvfrom(chat->socket_value, buffer, sizeof(buffer) - 1,
                                0, NULL, NULL);

        if (received < 0) {
            if (atomic_load(&chat->running) && receive_should_retry()) {
                continue;
            }
            break;
        }
        buffer[received] = '\0';
        printf("\n%s: %s\n%s", chat->peer_label, buffer, chat->prompt);
        fflush(stdout);
        if (strcmp(buffer, "exit") == 0) {
            break;
        }
    }
    atomic_store(&chat->running, 0);
    return NULL;
}

static int run_chat(socket_t socket_value, const char *prompt,
                    const char *peer_label,
                    const struct sockaddr_in *peer_address,
                    address_length_t peer_size)
{
    udp_chat_t chat;
    thread_t receiver;
    char buffer[MAX_MESSAGE_SIZE];
    int status = 0;

    chat.socket_value = socket_value;
    atomic_init(&chat.running, 1);
    chat.peer_label = peer_label;
    chat.prompt = prompt;
    chat.peer_address = *peer_address;
    chat.peer_size = peer_size;

    if (!set_receive_timeout(socket_value, 200)) {
        print_socket_error("setsockopt");
        return 1;
    }
    if (!thread_start(&receiver, receive_messages, &chat)) {
        fprintf(stderr, "Alma is parcacigi baslatilamadi.\n");
        return 1;
    }

    while (atomic_load(&chat.running) &&
           read_line(prompt, buffer, sizeof(buffer))) {
        if (!atomic_load(&chat.running)) {
            break;
        }
        if (sendto(socket_value, buffer, (int)strlen(buffer) + 1, 0,
                   (struct sockaddr *)&chat.peer_address,
                   chat.peer_size) < 0) {
            print_socket_error("sendto");
            status = 1;
            break;
        }
        if (strcmp(buffer, "exit") == 0) {
            break;
        }
    }

    atomic_store(&chat.running, 0);
    thread_join(receiver);
    return status;
}

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
    client_size = sizeof(client_address);
    {
        int received = recvfrom(server_socket, buffer, sizeof(buffer) - 1, 0,
                                (struct sockaddr *)&client_address, &client_size);
        if (received < 0) {
            print_socket_error("recvfrom");
            goto cleanup;
        }
        buffer[received] = '\0';
        printf("Client: %s\n", buffer);
        if (strcmp(buffer, "exit") == 0) {
            status = 0;
            goto cleanup;
        }
    }

    status = run_chat(server_socket, "Server: ", "Client",
                      &client_address, client_size);

cleanup:
    if (server_socket != INVALID_SOCKET_VALUE) CLOSE_SOCKET(server_socket);
    printf("UDP server kapatildi.\n");
    return status;
}

int run_udp_client(const char *ip_address, int port)
{
    socket_t client_socket = INVALID_SOCKET_VALUE;
    struct sockaddr_in local_address = {0};
    struct sockaddr_in server_address = {0};
    int status = 1;

    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket == INVALID_SOCKET_VALUE) {
        print_socket_error("socket");
        goto cleanup;
    }

    local_address.sin_family = AF_INET;
    local_address.sin_port = htons(0);
    local_address.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(client_socket, (struct sockaddr *)&local_address,
             sizeof(local_address)) < 0) {
        print_socket_error("bind");
        goto cleanup;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons((unsigned short)port);
    if (inet_pton(AF_INET, ip_address, &server_address.sin_addr) != 1) {
        fprintf(stderr, "Gecersiz IP adresi.\n");
        goto cleanup;
    }
    printf("UDP client %s:%d adresini kullaniyor.\n", ip_address, port);
    status = run_chat(client_socket, "Client: ", "Server",
                      &server_address, sizeof(server_address));

cleanup:
    if (client_socket != INVALID_SOCKET_VALUE) CLOSE_SOCKET(client_socket);
    printf("UDP client kapatildi.\n");
    return status;
}
