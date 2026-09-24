# Son Eklemeler

1. Chat mesaji loga tarih-saat ile kaydedilir ve ayni formatta WebSocket ile yayinlanir.
2. Chat sayfasi her acilista `/api/chat-history` ile `server.log` icindeki Client1-Client4 mesajlarini geri yukler; F5 sonrasi gecmis kaybolmaz.
3. `/` route'u session varsa `/panel`, yoksa `/login` adresine server-side 302 ile yonlendirir.
4. Login sayfasi yanlis kullanici/sifre, rate limit ve sunucu/API hatalarini gorunur hata mesaji olarak gosterir.
5. `/panel`, `/mesajlasma`, `/saw` session olmadan acilamaz; `/login` adresine yonlendirilir.
6. Client adlari sabit havuzdan `Client1`, `Client2`, `Client3`, `Client4` olarak atanir; baglanti kapaninca slot serbest birakilir.
7. Chat alaninda server durum yazilari gosterilmez; sadece client mesajlari kalir.
8. `mini_zip.h` ile `server.log` harici sistem zip komutu kullanilmadan ZIP yapilir.

## Yetkisiz kullanici akisi duzeltmesi
- Login ekraninda 10 saniye islem yapilmazsa `/log` yerine genel ve hassas veri icermeyen `/status` sayfasina yonlendirilir.
- Basarisiz login denemesinde kullanici login ekraninda kalir ve hata mesaji gorur.
- `/log` ve `/api/log` artik gecerli session gerektirir.
- `/status` sadece sunucu durumu, baglanti tipi ve Giris Yap baglantisini gosterir.
