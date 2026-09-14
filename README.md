# 008 Cross Compile

Tum derleme kurallari tek ve sade bir `Makefile` icindedir. Ek bir `.cmd`
dosyasi kullanilmaz.

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
