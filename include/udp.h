#ifndef UDP_H
#define UDP_H

/**
 * @file udp.h
 * @brief Baglantisiz UDP istemci ve sunucu islemlerini tanimlar.
 */

/**
 * Belirtilen portta UDP datagramlarini bekler ve yanit gonderir.
 * Son mesaji gonderen istemcinin adresi cevap hedefi olarak kullanilir.
 * @param port Dinlenecek UDP portu.
 * @return Normal kapanista 0, soket hatasinda 1.
 */
int run_udp_server(int port);

/**
 * Belirtilen IPv4 adresine UDP datagramlari gonderir ve yanitlari okur.
 * @param ip_address Hedef sunucunun IPv4 adresi.
 * @param port Hedef UDP portu.
 * @return Normal kapanista 0, gonderme/alma veya soket hatasinda 1.
 */
int run_udp_client(const char *ip_address, int port);

#endif
