#ifndef TCP_H
#define TCP_H

/**
 * @file tcp.h
 * @brief Baglanti odakli TCP istemci ve sunucu islemlerini tanimlar.
 */

/**
 * Belirtilen portu dinleyen tek istemcili TCP sunucusunu calistirir.
 * Gonderme ve alma islemlerini ayri akislarda calistirarak iki tarafin
 * birbirini beklemeden mesajlasmasini saglar.
 * @param port Dinlenecek TCP portu.
 * @return Normal kapanista 0, soket hatasinda 1.
 */
int run_tcp_server(int port);

/**
 * Belirtilen IPv4 adresindeki TCP sunucusuna baglanir ve iki yonlu,
 * eszamanli mesajlasmayi baslatir.
 * @param ip_address Baglanilacak sunucunun IPv4 adresi.
 * @param port Baglanilacak TCP portu.
 * @return Normal kapanista 0, baglanti veya soket hatasinda 1.
 */
int run_tcp_client(const char *ip_address, int port);

#endif
