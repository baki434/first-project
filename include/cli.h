#ifndef CLI_H
#define CLI_H

#include <stddef.h>

#include "app_config.h"

int parse_arguments(int argc,
                    char *argv[],
                    network_type_t *network_type,
                    program_mode_t *program_mode,
                    char *ip_address,
                    size_t ip_address_size,
                    int *port);

#endif
