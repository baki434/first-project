#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET socket_t;
typedef int address_length_t;
#define INVALID_SOCKET_VALUE INVALID_SOCKET
#define CLOSE_SOCKET(value) closesocket(value)
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int socket_t;
typedef socklen_t address_length_t;
#define INVALID_SOCKET_VALUE (-1)
#define CLOSE_SOCKET(value) close(value)
#endif

int network_initialize(void);
void network_cleanup(void);
void print_socket_error(const char *operation);
int set_reuse_address(socket_t socket_value);

#endif
