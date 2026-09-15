APP    := network_app
SRC    := $(wildcard src/*.c)
CFLAGS := -std=c11 -Wall -Wextra -Iinclude
ifeq ($(OS),Windows_NT)
    HOST  := Windows
    EXT   := .exe
    LIBS  := -lws2_32
    RPI_LIBS := -pthread
    SHELL := cmd.exe
    .SHELLFLAGS := /C
    ARM_BIN := C:/Program Files (x86)/Arm GNU Toolchain aarch64-none-linux-gnu/11.2 2022.02/bin
    RPI_CC := "$(ARM_BIN)/aarch64-none-linux-gnu-gcc.exe"
    READELF := "$(ARM_BIN)/aarch64-none-linux-gnu-readelf.exe"
    MKDIR = if not exist "$(subst /,\,$(1))" mkdir "$(subst /,\,$(1))"
else
    HOST := Linux
    EXT  :=
    LIBS := -pthread
    RPI_LIBS := -pthread
    RPI_CC ?= aarch64-linux-gnu-gcc
    READELF ?= aarch64-linux-gnu-readelf
    MKDIR = mkdir -p "$(1)"
endif
NATIVE := build/native/$(APP)$(EXT)
RPI    := build/rpi/$(APP)
.DEFAULT_GOAL := native
.PHONY: native rpi clean
native:
	@echo [HOST] $(HOST)
	@$(call MKDIR,build/native)
	@gcc $(CFLAGS) $(SRC) -o $(NATIVE) $(LIBS)
	@echo [OK] Native cikti: $(NATIVE)
rpi:
	@$(call MKDIR,build/rpi)
	@echo [TOOLCHAIN] host=$(HOST) target=aarch64-none-linux-gnu
	@$(RPI_CC) $(CFLAGS) $(SRC) -o $(RPI) $(RPI_LIBS)
	@$(READELF) -h $(RPI) | findstr /C:"Class:" /C:"Machine:"
	@echo [OK] Cross-compile tamamlandi: $(RPI)
clean:
ifeq ($(OS),Windows_NT)
	@if exist build rmdir /S /Q build
else
	@rm -rf build
endif
	@echo [OK] Build klasoru temizlendi.
