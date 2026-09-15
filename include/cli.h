#ifndef CLI_H
#define CLI_H

/**
 * @file cli.h
 * @brief Komut satiri seceneklerinin okunmasi icin arayuz sunar.
 */

#include <stddef.h>

#include "app_config.h"

/**
 * Komut satiri argumanlarini uygulama ayarlarina donusturur.
 *
 * Desteklenen secenekler: ag turu (-n), calisma turu (-t), IP adresi (-i),
 * port (-p), yardim (-h) ve surum (-v).
 *
 * @param argc Programa verilen toplam arguman sayisi.
 * @param argv Arguman metinlerini tutan dizi.
 * @param network_type Secilen TCP/UDP turunun yazilacagi adres.
 * @param program_mode Secilen client/server turunun yazilacagi adres.
 * @param ip_address IP adresinin yazilacagi karakter dizisi.
 * @param ip_address_size IP karakter dizisinin toplam kapasitesi.
 * @param port Secilen port numarasinin yazilacagi adres.
 * @return 1 normal calisma, 0 yardim/surum gosterimi, -1 gecersiz arguman.
 */
int parse_arguments(int argc,
                    char *argv[],
                    network_type_t *network_type,
                    program_mode_t *program_mode,
                    char *ip_address,
                    size_t ip_address_size,
                    int *port);

#endif
