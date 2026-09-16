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
