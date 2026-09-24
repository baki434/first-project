#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "login_web_server.h"
#include "mongoose.h"

#define DEFAULT_WEB_PORT 8000
#define LOGIN_PATH_SIZE 4096

static int executable_directory(char *output, size_t output_size)
{
    size_t length;
    char *separator;

    if (output == NULL || output_size < 2) {
        return 0;
    }
#ifdef _WIN32
    {
        DWORD result = GetModuleFileNameA(NULL, output, (DWORD)output_size);
        if (result == 0 || (size_t)result >= output_size) {
            return 0;
        }
        length = (size_t)result;
    }
#else
    {
        ssize_t result = readlink("/proc/self/exe", output, output_size - 1);
        if (result <= 0 || (size_t)result >= output_size - 1) {
            return 0;
        }
        length = (size_t)result;
        output[length] = '\0';
    }
#endif
    (void)length;
    separator = strrchr(output, '/');
#ifdef _WIN32
    {
        char *backslash = strrchr(output, '\\');
        if (backslash != NULL &&
            (separator == NULL || backslash > separator)) {
            separator = backslash;
        }
    }
#endif
    if (separator == NULL) {
        return 0;
    }
    *separator = '\0';
    return output[0] != '\0';
}

static int executable_sibling_path(const char *filename,
                                   char *output,
                                   size_t output_size)
{
    char directory[LOGIN_PATH_SIZE];
    int written;

    if (!executable_directory(directory, sizeof(directory))) {
        return 0;
    }
    written = snprintf(output, output_size, "%s/%s", directory, filename);
    return written >= 0 && (size_t)written < output_size;
}

static void show_usage(const char *program)
{
    printf("Kullanim: %s [-p PORT] [--index login.html] "
           "[--users users.json] [--cert CERT.pem --key KEY.pem] "
           "[--hsts]\n", program);
    printf("Varsayilan dosyalar: login.html ve users.json (exe klasorunde)\n");
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
    char default_index_path[LOGIN_PATH_SIZE];
    char default_users_path[LOGIN_PATH_SIZE];
    const char *index_path = NULL;
    const char *users_path = NULL;
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
        if (strcmp(argv[i], "-v") == 0 ||
            strcmp(argv[i], "--version") == 0) {
            printf("SAW Login Web Program version 1.0.0 (Mongoose %s)\n",
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
        } else if (strcmp(argv[i], "--index") == 0) {
            index_path = argv[++i];
        } else if (strcmp(argv[i], "--users") == 0) {
            users_path = argv[++i];
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
    if (index_path == NULL) {
        if (!executable_sibling_path("login.html", default_index_path,
                                     sizeof(default_index_path))) {
            fprintf(stderr, "Executable klasoru bulunamadi.\n");
            return 1;
        }
        index_path = default_index_path;
    }
    if (users_path == NULL) {
        if (!executable_sibling_path("users.json", default_users_path,
                                     sizeof(default_users_path))) {
            fprintf(stderr, "Executable klasoru bulunamadi.\n");
            return 1;
        }
        users_path = default_users_path;
    }

    return run_login_web_server(port, index_path, users_path,
                                certificate_path, private_key_path,
                                enable_hsts);
}
