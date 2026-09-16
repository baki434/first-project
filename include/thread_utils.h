#ifndef THREAD_UTILS_H
#define THREAD_UTILS_H

/**
 * @file thread_utils.h
 * @brief Windows ve Linux icin ortak is parcacigi islemlerini tanimlar.
 *
 * Mesaj gonderme ve alma islemlerinin birbirinden bagimsiz calisabilmesi icin
 * gerekli platform farklarini bu modul icinde tutar.
 */

#include "platform.h"

#ifdef _WIN32
#include <windows.h>
/** Windows is parcacigi taniticisi icin ortak proje tipi. */
typedef HANDLE thread_t;
/** Windows kritik bolgesi icin ortak mutex tipi. */
typedef CRITICAL_SECTION mutex_t;
#else
#include <pthread.h>
/** POSIX is parcacigi taniticisi icin ortak proje tipi. */
typedef pthread_t thread_t;
/** POSIX mutex taniticisi icin ortak mutex tipi. */
typedef pthread_mutex_t mutex_t;
#endif

/** Yeni bir is parcaciginda calistirilabilecek fonksiyon tipi. */
typedef void *(*thread_function_t)(void *argument);

/**
 * Verilen fonksiyonu yeni bir is parcaciginda baslatir.
 * @param thread Olusturulan is parcacigi taniticisinin yazilacagi adres.
 * @param function Is parcaciginda calistirilacak fonksiyon.
 * @param argument Fonksiyona aktarilacak kullanici verisi.
 * @return Basarida 1, olusturma hatasinda 0.
 */
int thread_start(thread_t *thread, thread_function_t function, void *argument);

/**
 * Bir is parcaciginin tamamlanmasini bekler ve kaynaklarini serbest birakir.
 * @param thread Beklenecek is parcacigi.
 */
void thread_join(thread_t thread);

/**
 * Soketin gonderme ve alma islemlerini durdurarak bekleyen is parcacigini acar.
 * @param socket_value Durdurulacak soket.
 */
void shutdown_socket(socket_t socket_value);

/**
 * Alma isleminin belirli araliklarla kontrol edilebilmesi icin zaman asimi ayarlar.
 * @param socket_value Ayar uygulanacak soket.
 * @param milliseconds Zaman asimi suresi (milisaniye).
 * @return Basarida 1, ayar hatasinda 0.
 */
int set_receive_timeout(socket_t socket_value, int milliseconds);

/**
 * Son alma hatasinin zaman asimi veya yeniden deneme durumu olup olmadigini denetler.
 * @return Yeniden denenebilir durumda 1, diger hatalarda 0.
 */
int receive_should_retry(void);

/**
 * Bir mutex nesnesini kullanima hazirlar.
 * @param mutex Hazirlanacak mutex.
 * @return Basarida 1, baslatma hatasinda 0.
 */
int mutex_initialize(mutex_t *mutex);

/** Paylasilan bolgeye girmeden once mutex kilidini alir. */
void mutex_lock(mutex_t *mutex);

/** Paylasilan bolgedeki islem tamamlaninca mutex kilidini birakir. */
void mutex_unlock(mutex_t *mutex);

/** Mutex kaynaklarini serbest birakir. */
void mutex_destroy(mutex_t *mutex);

#endif
