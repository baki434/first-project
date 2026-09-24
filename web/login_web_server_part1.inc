#include "login_web_server.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>

#include "mini_zip.h"

#include "app_config.h"
#include "command_handler.h"
#include "login_auth.h"
#include "login_session.h"
#include "mongoose.h"

#define LOGIN_MAX_BODY_SIZE 1024
#define LOGIN_RATE_SLOTS 64
#define LOGIN_RATE_WINDOW_SECONDS 60
#define LOGIN_RATE_MAX_FAILURES 5

typedef struct {
    unsigned char ip[16];
    int is_ip6;
    unsigned int failures;
    time_t window_started;
    int used;
} login_rate_entry_t;

typedef struct {
    struct mg_str certificate;
    struct mg_str private_key;
    login_auth_store_t auth;
    login_session_store_t sessions;
    login_rate_entry_t rates[LOGIN_RATE_SLOTS];
    const char *index_path;
    char base_dir[4096];
    char login_path[4096];
    char dashboard_path[4096];
    char chat_path[4096];
    char saw_path[4096];
    char log_page_path[4096];
    char status_path[4096];
    char log_path[4096];
    char zip_path[4096];
    FILE *log_file;
    struct { unsigned long connection_id; int used; } clients[4];
    int tls_enabled;
    int hsts_enabled;
} login_server_state_t;

static volatile sig_atomic_t login_server_running = 1;

static void format_timestamp(char *output, size_t output_size)
{
    time_t now = time(NULL);
    struct tm tm_value;
#ifdef _WIN32
    localtime_s(&tm_value, &now);
#else
    localtime_r(&now, &tm_value);
#endif
    strftime(output, output_size, "%d.%m.%Y %H:%M:%S", &tm_value);
}

static void log_line(login_server_state_t *state, const char *format, ...)
{
    va_list args;
    if (state->log_file == NULL) return;
    va_start(args, format);
    vfprintf(state->log_file, format, args);
    va_end(args);
    fputc('\n', state->log_file);
    fflush(state->log_file);
}

static int path_join(char *output, size_t output_size, const char *dir, const char *name)
{
    int written = snprintf(output, output_size, "%s/%s", dir, name);
    return written >= 0 && (size_t)written < output_size;
}

static int dirname_from_path(const char *path, char *output, size_t output_size)
{
    const char *slash = strrchr(path, '/');
#ifdef _WIN32
    const char *backslash = strrchr(path, '\\');
    if (backslash != NULL && (slash == NULL || backslash > slash)) slash = backslash;
#endif
    if (slash == NULL || (size_t)(slash - path) >= output_size) return 0;
    memcpy(output, path, (size_t)(slash - path));
    output[slash - path] = '\0';
    return 1;
}

static int assign_client(login_server_state_t *state, unsigned long connection_id)
{
    int i;
    for (i = 0; i < 4; ++i) {
        if (!state->clients[i].used) {
            state->clients[i].used = 1;
            state->clients[i].connection_id = connection_id;
            return i + 1;
        }
    }
    return 0;
}

static int client_number(login_server_state_t *state, unsigned long connection_id)
{
    int i;
    for (i = 0; i < 4; ++i)
        if (state->clients[i].used && state->clients[i].connection_id == connection_id) return i + 1;
    return 0;
}

static void release_client(login_server_state_t *state, unsigned long connection_id)
{
    int i;
    for (i = 0; i < 4; ++i) {
        if (state->clients[i].used && state->clients[i].connection_id == connection_id) {
            state->clients[i].used = 0;
            state->clients[i].connection_id = 0;
            return;
        }
    }
}

static void redirect_to(struct mg_connection *connection, const char *location)
{
    char headers[512];
    snprintf(headers, sizeof(headers), "Location: %s\r\nCache-Control: no-store\r\n", location);
    mg_http_reply(connection, 302, headers, "");
}


static void handle_signal(int signal_number)
{
    (void)signal_number;
    login_server_running = 0;
}

static int method_is(const struct mg_http_message *request,
                     const char *method)
{
    return mg_strcmp(request->method, mg_str(method)) == 0;
}

static int build_headers(const login_server_state_t *state,
                         const char *content_type,
                         const char *extra,
                         char *output,
                         size_t output_size)
{
    char content_type_header[160];
    int type_written;
    int written;

    if (content_type == NULL) {
        content_type_header[0] = '\0';
    } else {
        type_written = snprintf(content_type_header,
                                sizeof(content_type_header),
                                "Content-Type: %s\r\n", content_type);
        if (type_written < 0 ||
            (size_t)type_written >= sizeof(content_type_header)) {
            return 0;
        }
    }
    written = snprintf(
        output, output_size,
        "%s"
        "X-Content-Type-Options: nosniff\r\n"
        "X-Frame-Options: DENY\r\n"
        "Referrer-Policy: no-referrer\r\n"
        "Cache-Control: no-store\r\n"
        "Cross-Origin-Opener-Policy: same-origin\r\n"
        "Permissions-Policy: camera=(), microphone=(), geolocation=()\r\n"
        "%s"
        "Content-Security-Policy: default-src 'self'; "
        "style-src 'self' 'unsafe-inline'; "
        "script-src 'self' 'unsafe-inline'; "
        "connect-src 'self' ws: wss:; img-src 'self' data:; "
        "object-src 'none'; base-uri 'none'; frame-ancestors 'none'; "
        "form-action 'self'; block-all-mixed-content\r\n"
        "%s",
        content_type_header,
        state->tls_enabled && state->hsts_enabled
            ? "Strict-Transport-Security: max-age=31536000\r\n"
            : "",
        extra == NULL ? "" : extra);

    return written >= 0 && (size_t)written < output_size;
}

static struct mg_str session_cookie(const struct mg_http_message *request)
{
    struct mg_str *cookie = mg_http_get_header(
        (struct mg_http_message *)request, "Cookie");
