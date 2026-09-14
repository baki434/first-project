#include "input_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int read_line(const char *prompt, char *buffer, size_t size)
{
    size_t length;

    printf("%s", prompt);
    fflush(stdout);
    if (fgets(buffer, (int)size, stdin) == NULL) {
        return 0;
    }

    length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[length - 1] = '\0';
    }
    return 1;
}

int parse_port(const char *text, int *port)
{
    char *end;
    long value = strtol(text, &end, 10);

    if (*text == '\0' || *end != '\0' || value < 1 || value > 65535) {
        return 0;
    }
    *port = (int)value;
    return 1;
}
