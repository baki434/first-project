# SAW Web - Engineer Review Notes

Bu surum Raspberry Pi uzerinde C + Mongoose ile calisan HTTPS/WebSocket web uygulamasinin son inceleme adayidir.

## Uygulama akisi
- `/` oturum varsa `/panel`, yoksa `/login` adresine yonlendirir.
- `/login`: harici `login.html`, hatali giriste kullaniciya acik hata mesaji gosterir.
- `/panel`: Mesajlasma ve SAW bolumlerine gecis noktasi.
- `/mesajlasma`: WebSocket tabanli chat.
- `/saw`: Help/Info komutlari icin ayri ekran.

## Mesaj ve log mantigi
- WebSocket istemcilerine ilk bos `Client1`..`Client4` ismi atanir.
- Mesaj once `server.log` dosyasina `ClientN: mesaj - tarih-saat` olarak yazilir, sonra bagli istemcilere broadcast edilir.
- Chat sayfasi acildiginda/F5 yapildiginda `/api/chat-history` ile sadece Client1..Client4 mesajlari `server.log` dosyasindan geri yuklenir.
- Login, uygulama baslangici, SAW komutlari ve ZIP olusturma da loglanir; bunlar chat gecmisinde gosterilmez.

## ZIP
- `web/mini_zip.h` header-only minimal ZIP writer'dir.
- `/api/zip`, `server.log` dosyasini harici `zip` programi cagirmadan `server_log.zip` haline getirir.
- `/download/log.zip` olusan arsivi indirir.

## Hata yonetimi
- Login hatalari JSON hata mesaji olarak doner ve `login.html` bunlari ekranda gosterir.
- Yetkisiz API erisimi 401, yanlis method 405, gecersiz SAW komutu 400, olmayan route 404 doner.

## Servis
- `deploy/saw-web-login.service` uygulamayi `/opt/saw-web` altindan calistirir.
- `Restart=on-failure` ile hata sonrasi yeniden baslatma vardir.
- Log/ZIP olusturabilmesi icin `/opt/saw-web` yazilabilir olarak tanimlanmistir.

## Dogrulanan testler
- `make clean && make web-login`: basarili.
- `/` -> `/login` redirect: basarili.
- Basarisiz login: 401 + hata mesaji.
- Basarili login + session: basarili.
- `/panel`: session ile 200.
- SAW Help API: basarili.
- Header-only ZIP olusturma: basarili.
- 2 WebSocket istemcisi: Client1 / Client2 atamasi basarili.
- Client1 mesaji iki istemciye broadcast edildi.
- F5/gecmis mantigi icin `/api/chat-history`, logdaki Client mesajini tarih-saat ile geri dondurdu.
