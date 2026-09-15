#ifndef INPUT_UTILS_H
#define INPUT_UTILS_H

/**
 * @file input_utils.h
 * @brief Guvenli metin girisi ve port dogrulamasi icin yardimci fonksiyonlar.
 */

#include <stddef.h>

/**
 * Kullaniciya bir istem metni gosterir ve tek satirlik girdi okur.
 * Satir sonundaki yeni satir karakterini otomatik olarak kaldirir.
 *
 * @param prompt Girdi oncesinde ekrana yazilacak metin.
 * @param buffer Okunan metnin kaydedilecegi karakter dizisi.
 * @param size Buffer kapasitesi; sonlandirici karakter dahil.
 * @return Basarili okumada 1, girdi akisi kapandiginda 0.
 */
int read_line(const char *prompt, char *buffer, size_t size);

/**
 * Metin olarak verilen portu dogrular ve tam sayiya cevirir.
 * Yalnizca 1-65535 araligindaki tamamen sayisal degerleri kabul eder.
 *
 * @param text Donusturulecek port metni.
 * @param port Dogrulanan portun yazilacagi adres.
 * @return Gecerli portta 1, gecersiz degerde 0.
 */
int parse_port(const char *text, int *port);

#endif
