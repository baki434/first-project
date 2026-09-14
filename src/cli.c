#include "cli.h"

#include <stdio.h>
#include <string.h>

#include "input_utils.h"

static void show_help(const char *program)
{
    printf("Kullanim: %s -n tcp|udp -t client|server "
           "[-i IP] [-p PORT]\n", program);
    printf("Varsayilan: tcp client %s:%d\n", DEFAULT_IP, DEFAULT_PORT);
}

static int next_value(int argc, char *argv[], int *index)
{
    if (*index + 1 >= argc) {
        fprintf(stderr, "%s icin deger gerekli.\n", argv[*index]);
        return 0;
    }
    (*index)++;
    return 1;
}

int parse_arguments(int argc, char *argv[], network_type_t *network,
                    program_mode_t *mode, char *ip, size_t ip_size, int *port)
{
    int i;

    *network = NETWORK_TCP;
    *mode = MODE_CLIENT;
    *port = DEFAULT_PORT;
    snprintf(ip, ip_size, "%s", DEFAULT_IP);

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            show_help(argv[0]);
            return 0;
        }
        if (strcmp(argv[i], "-v") == 0) {
            printf("Unified TCP/UDP Program version 1.0.0\n");
            return 0;
        }
        if (!next_value(argc, argv, &i)) {
            return -1;
        }

        if (strcmp(argv[i - 1], "-n") == 0) {
            if (strcmp(argv[i], "tcp") == 0) {
                *network = NETWORK_TCP;
            } else if (strcmp(argv[i], "udp") == 0) {
                *network = NETWORK_UDP;
            } else {
                fprintf(stderr, "Ag turu tcp veya udp olmali.\n");
                return -1;
            }
        } else if (strcmp(argv[i - 1], "-t") == 0) {
            if (strcmp(argv[i], "client") == 0) {
                *mode = MODE_CLIENT;
            } else if (strcmp(argv[i], "server") == 0) {
                *mode = MODE_SERVER;
            } else {
                fprintf(stderr, "Tur client veya server olmali.\n");
                return -1;
            }
        } else if (strcmp(argv[i - 1], "-i") == 0) {
            if (strlen(argv[i]) >= ip_size) {
                fprintf(stderr, "IP adresi cok uzun.\n");
                return -1;
            }
            snprintf(ip, ip_size, "%s", argv[i]);
        } else if (strcmp(argv[i - 1], "-p") == 0) {
            if (!parse_port(argv[i], port)) {
                fprintf(stderr, "Port 1-65535 arasinda olmali.\n");
                return -1;
            }
        } else {
            fprintf(stderr, "Gecersiz arguman: %s\n", argv[i - 1]);
            return -1;
        }
    }
    return 1;
}
