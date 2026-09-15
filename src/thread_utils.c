#include "thread_utils.h"

#include <stdlib.h>

#ifndef _WIN32
#include <errno.h>
#include <sys/time.h>
#endif

#ifdef _WIN32
typedef struct {
    thread_function_t function;
    void *argument;
} thread_start_data_t;

static DWORD WINAPI thread_entry(LPVOID value)
{
    thread_start_data_t *data = value;
    thread_function_t function = data->function;
    void *argument = data->argument;

    free(data);
    function(argument);
    return 0;
}
#endif

int thread_start(thread_t *thread, thread_function_t function, void *argument)
{
#ifdef _WIN32
    thread_start_data_t *data = malloc(sizeof(*data));

    if (data == NULL) {
        return 0;
    }
    data->function = function;
    data->argument = argument;
    *thread = CreateThread(NULL, 0, thread_entry, data, 0, NULL);
    if (*thread == NULL) {
        free(data);
        return 0;
    }
    return 1;
#else
    return pthread_create(thread, NULL, function, argument) == 0;
#endif
}

void thread_join(thread_t thread)
{
#ifdef _WIN32
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
#else
    pthread_join(thread, NULL);
#endif
}

void shutdown_socket(socket_t socket_value)
{
#ifdef _WIN32
    shutdown(socket_value, SD_BOTH);
#else
    shutdown(socket_value, SHUT_RDWR);
#endif
}

int set_receive_timeout(socket_t socket_value, int milliseconds)
{
#ifdef _WIN32
    DWORD timeout = (DWORD)milliseconds;

    return setsockopt(socket_value, SOL_SOCKET, SO_RCVTIMEO,
                      (const char *)&timeout, sizeof(timeout)) == 0;
#else
    struct timeval timeout;

    timeout.tv_sec = milliseconds / 1000;
    timeout.tv_usec = (milliseconds % 1000) * 1000;
    return setsockopt(socket_value, SOL_SOCKET, SO_RCVTIMEO,
                      &timeout, sizeof(timeout)) == 0;
#endif
}

int receive_should_retry(void)
{
#ifdef _WIN32
    int error = WSAGetLastError();

    return error == WSAETIMEDOUT || error == WSAEWOULDBLOCK ||
           error == WSAEINTR;
#else
    return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR;
#endif
}
