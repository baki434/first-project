#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mongoose.h"
#include "web_server.h"

#define DEFAULT_WEB_PORT 8000

static void show_usage(const char *program)
{
    printf("Kullanim: %s [-p PORT] [--cert CERT.pem --key KEY.pem] "
           "[--hsts]\n", program);
    printf("Varsayilan: HTTP port %d\n", DEFAULT_WEB_PORT);
}

static int parse_port(const char *text, int *port)
{
    char *end;
    long value = strtol(text, &end, 10);

    if (*text == '\0' || *end != '\0' || value < 1 || value > 65535) {
        return 0;
    }
    *port = (int)value;
    return 1;
}

int main(int argc, char *argv[])
{
    const char *certificate_path = NULL;
    const char *private_key_path = NULL;
    int enable_hsts = 0;
    int port = DEFAULT_WEB_PORT;
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            show_usage(argv[0]);
            return 0;
        }
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("SAW Web Program version 1.0.0 (Mongoose %s)\n",
                   MG_VERSION);
            return 0;
        }
        if (strcmp(argv[i], "--hsts") == 0) {
            enable_hsts = 1;
            continue;
        }
        if (i + 1 >= argc) {
            fprintf(stderr, "%s icin deger gerekli.\n", argv[i]);
            return 1;
        }
        if (strcmp(argv[i], "-p") == 0) {
            if (!parse_port(argv[++i], &port)) {
                fprintf(stderr, "Port 1-65535 arasinda olmali.\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--cert") == 0) {
            certificate_path = argv[++i];
        } else if (strcmp(argv[i], "--key") == 0) {
            private_key_path = argv[++i];
        } else {
            fprintf(stderr, "Gecersiz arguman: %s\n", argv[i]);
            show_usage(argv[0]);
            return 1;
        }
    }

    if ((certificate_path == NULL) != (private_key_path == NULL)) {
        fprintf(stderr, "HTTPS icin --cert ve --key birlikte verilmeli.\n");
        return 1;
    }
    if (enable_hsts && certificate_path == NULL) {
        fprintf(stderr, "HSTS yalnizca HTTPS ile etkinlestirilebilir.\n");
        return 1;
    }

    return run_web_server(port, certificate_path, private_key_path,
                          enable_hsts);
}
