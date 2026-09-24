#ifndef WEB_SERVER_H
#define WEB_SERVER_H

/**
 * Mongoose tabanli HTTP/WebSocket sunucusunu calistirir.
 *
 * Sertifika ve anahtar NULL ise HTTP, ikisi de verildiginde HTTPS kullanilir.
 * HSTS yalnizca HTTPS etkinse uygulanabilir.
 *
 * @param port Dinlenecek TCP portu.
 * @param certificate_path PEM sunucu sertifikasi veya NULL.
 * @param private_key_path PEM ozel anahtar veya NULL.
 * @param enable_hsts HTTPS yanitlarina HSTS basligi ekleme durumu.
 * @return Basarida 0, hata durumunda 1.
 */
int run_web_server(int port,
                   const char *certificate_path,
                   const char *private_key_path,
                   int enable_hsts);

#endif
