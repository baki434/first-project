#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/**
 * @file app_config.h
 * @brief Uygulamanin ortak ag ayarlarini ve temel veri tiplerini tanimlar.
 *
 * TCP/UDP modulleri ile komut satiri ayrıştırıcısının ayni varsayilan
 * degerleri kullanmasini saglar.
 */

/** IP adresi belirtilmediginde kullanilan yerel geri-donus adresi. */
#define DEFAULT_IP "127.0.0.1"
/** Port belirtilmediginde kullanilan varsayilan TCP/UDP portu. */
#define DEFAULT_PORT 8080
/** Tek seferde alinabilecek veya gonderilebilecek en buyuk mesaj boyutu. */
#define MAX_MESSAGE_SIZE 1024
/** IPv4 adres metni ve sonlandirici karakter icin ayrilan alan. */
#define IP_TEXT_SIZE 16

/** Kullanilacak tasima katmani protokolunu belirtir. */
typedef enum { NETWORK_TCP, NETWORK_UDP } network_type_t;

/** Programin istemci ya da sunucu olarak calisacagini belirtir. */
typedef enum { MODE_CLIENT, MODE_SERVER } program_mode_t;

#endif
