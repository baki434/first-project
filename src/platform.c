#include "platform.h"

#include <stdio.h>

int network_initialize(void)
{
#ifdef _WIN32
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data) == 0;
#else
    return 1;
#endif
}

void network_cleanup(void)
{
#ifdef _WIN32
    WSACleanup();
#endif
}

void print_socket_error(const char *operation)
{
#ifdef _WIN32
    fprintf(stderr, "%s failed: %d\n", operation, WSAGetLastError());
#else
    perror(operation);
#endif
}

int set_reuse_address(socket_t socket_value)
{
    int enabled = 1;
#ifdef _WIN32
    return setsockopt(socket_value, SOL_SOCKET, SO_REUSEADDR,
                      (const char *)&enabled, sizeof(enabled));
#else
    return setsockopt(socket_value, SOL_SOCKET, SO_REUSEADDR,
                      &enabled, sizeof(enabled));
#endif
}
