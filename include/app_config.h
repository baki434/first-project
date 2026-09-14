#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT 8080
#define MAX_MESSAGE_SIZE 1024
#define IP_TEXT_SIZE 16

typedef enum { NETWORK_TCP, NETWORK_UDP } network_type_t;
typedef enum { MODE_CLIENT, MODE_SERVER } program_mode_t;

#endif
