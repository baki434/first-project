#ifndef UDP_H
#define UDP_H

/**
 * @file udp.h
 * @brief Baglantisiz UDP istemci ve sunucu islemlerini tanimlar.
 */

/**
 * Belirtilen portta ilk UDP datagramini bekler, gonderen istemciyi es olarak
 * belirler ve ardindan gonderme ile alma islemlerini eszamanli yurutur.
 * @param port Dinlenecek UDP portu.
 * @return Normal kapanista 0, soket hatasinda 1.
 */
int run_udp_server(int port);

/**
 * Belirtilen IPv4 adresine UDP datagramlari gonderir ve yanitlari ayri bir
 * is parcaciginda okuyarak iki tarafin birbirini beklememesini saglar.
 * @param ip_address Hedef sunucunun IPv4 adresi.
 * @param port Hedef UDP portu.
 * @return Normal kapanista 0, gonderme/alma veya soket hatasinda 1.
 */
int run_udp_client(const char *ip_address, int port);

#endif
