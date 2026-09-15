#ifndef PLATFORM_H
#define PLATFORM_H

/**
 * @file platform.h
 * @brief Windows ve Linux soket API'leri arasinda ortak bir arayuz saglar.
 *
 * Isletim sistemine ozgu tipleri ve fonksiyonlari tek noktada esleyerek TCP
 * ve UDP modullerinin ayni kaynak kodla iki platformda derlenmesini saglar.
 */

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
/** Windows soket taniticisi icin ortak proje tipi. */
typedef SOCKET socket_t;
/** Windows soket adres uzunlugu tipi. */
typedef int address_length_t;
/** Gecersiz Windows soket degerinin platformdan bagimsiz adi. */
#define INVALID_SOCKET_VALUE INVALID_SOCKET
/** Windows'ta bir soketi guvenli bicimde kapatir. */
#define CLOSE_SOCKET(value) closesocket(value)
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
/** Linux dosya taniticisi tabanli soket tipi. */
typedef int socket_t;
/** Linux soket adres uzunlugu tipi. */
typedef socklen_t address_length_t;
/** Gecersiz Linux soket degerinin platformdan bagimsiz adi. */
#define INVALID_SOCKET_VALUE (-1)
/** Linux'ta bir soketi guvenli bicimde kapatir. */
#define CLOSE_SOCKET(value) close(value)
#endif

/**
 * Platformun ag altyapisini baslatir.
 * Windows'ta Winsock'u hazirlar; Linux'ta ek islem yapmadan basarili olur.
 * @return Basarida 1, baslatma hatasinda 0.
 */
int network_initialize(void);

/** Windows'ta Winsock kaynaklarini serbest birakir; Linux'ta islem yapmaz. */
void network_cleanup(void);

/**
 * Son soket hatasini platforma uygun bicimde stderr akimina yazar.
 * @param operation Hataya neden olan islemin ekranda gosterilecek adi.
 */
void print_socket_error(const char *operation);

/**
 * Soket adresinin yeniden kullanilabilmesi icin SO_REUSEADDR secenegini acar.
 * @param socket_value Ayar uygulanacak soket.
 * @return Basarida 0, hatada negatif bir deger.
 */
int set_reuse_address(socket_t socket_value);

#endif
