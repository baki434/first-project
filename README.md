# 008 Cross Compile

Tum derleme kurallari tek ve sade bir `Makefile` icindedir. Ek bir `.cmd`
dosyasi kullanilmaz.

TCP ve UDP modullerinde mesaj alma ayri bir is parcaciginda calisir. Bu sayede
istemci ve sunucu sira beklemeden ayni anda mesaj gonderebilir ve alabilir;
bir taraf art arda birden fazla mesaj da gonderebilir.

## Eszamanli mesajlasma testi

TCP sunucusu ve istemcisi iki ayri terminalde calistirilir:

```text
network_app -n tcp -t server -p 8080
network_app -n tcp -t client -i SUNUCU_IP -p 8080
```

UDP testi icin `tcp` yerine `udp` yazilir. UDP sunucusu hedef istemcinin
adresini ogrenebilmek icin ilk istemci mesajini bekler; bu ilk mesajdan sonra
iki taraf da sira beklemeden mesaj gonderebilir. `exit` mesaji oturumu kapatir.

## SAW komutlari

Client mesaj satirinda `saw` on ekiyle sunucu komutlari calistirilabilir:

```text
saw info
saw ping
saw help
```

`saw info` sunucunun isletim sistemi, CPU ve RAM bilgilerini; `saw ping`
`pong` yanitini; `saw help` ise komut listesini client'a dondurur. `saw ` ile
baslamayan girdiler normal mesaj olarak iletilir. Otomatik komut yanitlari ile
kullanici mesajlarinin socket uzerinde karismamasi icin gonderme mutex'i
kullanilir.

## Native derleme

```text
mingw32-make
```

Makefile calisilan sistemi otomatik algilar:

- Windows: `build/native/network_app.exe`
- Linux: `build/native/network_app` (ELF)

## Windows'tan Raspberry Pi icin cross-compile

```text
mingw32-make clean
mingw32-make rpi
```

`rpi` hedefi, Windows'ta kurulu `aarch64-none-linux-gnu-gcc.exe` derleyicisini
dogrudan Makefile icinden cagirir. Olusan dosya:

```text
build/rpi/network_app
```

Bu dosya Windows programi degil, Raspberry Pi icin ELF64 AArch64 Linux
programidir. Makefile derleme sonunda ELF sinifini ve makine mimarisini de
gosterir.

## Mongoose web arayuzu

Terminal tabanli TCP/UDP programi korunmustur. Buna ek olarak Mongoose 7.23
kullanan ayri bir `network_web` programi bulunur. Web programi:

- tarayiciya gomulu HTML/CSS/JavaScript arayuzunu sunar,
- gercek zamanli mesajlasma icin WebSocket kullanir,
- `saw ping`, `saw info` ve `saw help` komutlarini mevcut komut isleyiciyle
  calistirir,
- normal mesajlari bagli web istemcilerine yayinlar,
- `/health` adresinde basit bir saglik yaniti verir,
- 1024 baytlik uygulama mesaj sinirini kontrol eder.

Windows veya Linux icin web programini derlemek:

```text
mingw32-make web
```

Raspberry Pi AArch64 hedefi icin cross-compile:

```text
mingw32-make web-rpi
```

Olusan dosyalar:

```text
build/native/network_web.exe
build/rpi/network_web
```

HTTP sunucusunu varsayilan 8000 portunda baslatmak:

```text
network_web -p 8000
```

Tarayicidan `http://SUNUCU_IP:8000` adresi acilir. Windows guvenlik duvari
baglantiyi sorarsa yalnizca guvenilen ozel ag icin izin verilmelidir.

### HTTPS ve HSTS

HTTPS kullanmak icin PEM biciminde bir sunucu sertifikasi ve ozel anahtar
verilir:

```text
network_web -p 8443 --cert cert.pem --key key.pem
```

HSTS yalnizca tarayicinin guvendigi, dogrulanmis bir sertifika ile ve HTTPS
testleri tamamlandiktan sonra etkinlestirilmelidir:

```text
network_web -p 8443 --cert cert.pem --key key.pem --hsts
```

Self-signed sertifika gelistirme testi icin kullanilabilir ancak tarayici
tarafindan otomatik olarak guvenilir sayilmaz. Sertifika ve ozel anahtarlar
Git deposuna eklenmez.

Web arayuzu harici script, stil veya resim yuklemez. WebSocket adresi sayfanin
HTTP/HTTPS protokolune gore otomatik olarak `ws://` veya `wss://` secilir; bu
sayede HTTPS modunda mixed-content olusmaz.

### Mongoose lisansi

Projede sabitlenen Mongoose 7.23 kaynaklari `third_party/mongoose` altindadir.
Mongoose GPLv2 veya ticari lisansla sunulur; lisans metni ayni klasordeki
`LICENSE` dosyasinda korunmustur.

## Giris korumali web surumu

Mevcut `network_web` ve terminal uygulamasi degistirilmeden, ayri bir
`network_web_login` programi eklenmistir. Bu surum harici HTML dosyalari ve
`users.json` ile login, oturum, cikis ve kimligi dogrulanmis WebSocket
mesajlasmasi saglar.

Derleme:

```text
make clean
make web-login
```

Linux/Raspberry Pi uzerinde sistem OpenSSL kutuphanesiyle uretim derlemesi:

```text
sudo apt install build-essential libssl-dev
make web-login-openssl
```

Gercek `users.json` dosyasi repoya eklenmez. `web-login` hedefi Python
gerektirmeyen `create_saw_user` yardimci programini da derler. Kullanici
olusturmak icin:

```text
build/native/create_saw_user admin --file build/native/users.json
```

Parola dosyaya acik metin olarak yazilmaz; benzersiz salt ile
PBKDF2-HMAC-SHA256 ozeti saklanir.

Raspberry Pi dagitiminda uygulama `/opt/saw-web` altinda calisir. systemd servis
ornegi `deploy/saw-web-login.service` dosyasindadir. Son surumde `/login`
ekraninda 10 saniye islem yapilmazsa hassas veri icermeyen `/status` sayfasina
gidilir; `/log` ve `/api/log` ise gecerli session gerektirir.
