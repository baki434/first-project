#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

/**
 * @file system_info.h
 * @brief Calisilan sisteme ait temel bilgileri platformdan bagimsiz sunar.
 */

#include <stddef.h>

/**
 * Isletim sistemi, CPU ve RAM bilgilerini tek bir metin icinde hazirlar.
 * @param buffer Bilgilerin yazilacagi karakter dizisi.
 * @param size Buffer kapasitesi; sonlandirici karakter dahil.
 * @return Bilgiler tamamen yazildiysa 1, hata veya yetersiz alanda 0.
 */
int get_system_info(char *buffer, size_t size);

#endif
