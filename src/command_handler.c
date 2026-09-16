#include "command_handler.h"

#include <stdio.h>
#include <string.h>

#include "system_info.h"

#define SAW_PREFIX "saw "
#define SAW_HELP_TEXT \
    "Kullanilabilir komutlar:\n" \
    "saw info - Isletim sistemi, CPU ve RAM bilgilerini gosterir.\n" \
    "saw ping - Sunucunun erisilebilirligini kontrol eder.\n" \
    "saw help - Kullanilabilir komutlari listeler."

int handle_saw_command(const char *message, char *response,
                       size_t response_size)
{
    if (message == NULL || response == NULL || response_size == 0) {
        return 0;
    }
    if (strcmp(message, "saw") != 0 &&
        strncmp(message, SAW_PREFIX, strlen(SAW_PREFIX)) != 0) {
        return 0;
    }

    if (strcmp(message, "saw ping") == 0) {
        snprintf(response, response_size, "pong");
    } else if (strcmp(message, "saw info") == 0) {
        if (!get_system_info(response, response_size)) {
            snprintf(response, response_size,
                     "Sistem bilgileri alinamadi.");
        }
    } else if (strcmp(message, "saw help") == 0) {
        snprintf(response, response_size, "%s", SAW_HELP_TEXT);
    } else {
        snprintf(response, response_size,
                 "Bilinmeyen saw komutu. Komut listesi icin: saw help");
    }

    return 1;
}
