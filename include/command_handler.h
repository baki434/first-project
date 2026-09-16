#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

/**
 * @file command_handler.h
 * @brief "saw" on ekiyle gelen ozel ag komutlarini tanir ve calistirir.
 */

#include <stddef.h>

/**
 * Gelen metin bir saw komutuysa uygun yanit metnini hazirlar.
 * Desteklenen komutlar: saw info, saw ping ve saw help.
 * @param message Kontrol edilecek gelen mesaj.
 * @param response Komut yanitinin yazilacagi karakter dizisi.
 * @param response_size Yanit buffer kapasitesi; sonlandirici karakter dahil.
 * @return Mesaj bir saw komutuysa 1, normal mesajsa 0.
 */
int handle_saw_command(const char *message, char *response,
                       size_t response_size);

#endif
