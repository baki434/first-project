#ifndef INPUT_UTILS_H
#define INPUT_UTILS_H

#include <stddef.h>

int read_line(const char *prompt, char *buffer, size_t size);
int parse_port(const char *text, int *port);

#endif
