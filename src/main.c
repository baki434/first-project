#include <stdio.h>

#include "app_config.h"
#include "cli.h"
#include "platform.h"
#include "tcp.h"
#include "udp.h"

int main(int argc, char *argv[])
{
    network_type_t network;
    program_mode_t mode;
    char ip[IP_TEXT_SIZE];
    int port;
    int result;

    result = parse_arguments(argc, argv, &network, &mode,
                             ip, sizeof(ip), &port);
    if (result <= 0) {
        return result < 0;
    }
    if (!network_initialize()) {
        fprintf(stderr, "Ag baslatilamadi.\n");
        return 1;
    }

    if (network == NETWORK_TCP) {
        result = mode == MODE_SERVER ? run_tcp_server(port)
                                     : run_tcp_client(ip, port);
    } else {
        result = mode == MODE_SERVER ? run_udp_server(port)
                                     : run_udp_client(ip, port);
    }

    network_cleanup();
    return result;
}
