APP    := network_app
SRC    := $(wildcard src/*.c)
CFLAGS := -std=c11 -Wall -Wextra -Iinclude
WEB_APP    := network_web
WEB_SRC    := web/main.c web/web_server.c src/command_handler.c \
              src/system_info.c third_party/mongoose/mongoose.c
LOGIN_WEB_APP := network_web_login
LOGIN_OPENSSL_APP := network_web_login_openssl
CREATE_USER_APP := create_saw_user
LOGIN_WEB_SRC := web/login_main.c web/login_web_server.c web/login_auth.c \
                 web/login_session.c src/command_handler.c src/system_info.c \
                 third_party/mongoose/mongoose.c
WEB_CFLAGS := $(CFLAGS) -Iweb -Ithird_party/mongoose \
              -DMG_TLS=MG_TLS_BUILTIN -D_GNU_SOURCE
OPENSSL_WEB_CFLAGS := $(CFLAGS) -Iweb -Ithird_party/mongoose \
                      -DMG_TLS=MG_TLS_OPENSSL -D_GNU_SOURCE
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
    COPY_FILE = copy /Y "$(subst /,\,$(1))" "$(subst /,\,$(2))" >NUL
    ELF_INFO = $(READELF) -h $(1) | findstr /C:"Class:" /C:"Machine:"
else
    HOST := Linux
    EXT  :=
    LIBS := -pthread
    RPI_LIBS := -pthread
    RPI_CC ?= aarch64-linux-gnu-gcc
    READELF ?= aarch64-linux-gnu-readelf
    MKDIR = mkdir -p "$(1)"
    COPY_FILE = cp "$(1)" "$(2)"
    ELF_INFO = $(READELF) -h $(1) | grep -E "Class:|Machine:"
endif
NATIVE := build/native/$(APP)$(EXT)
RPI    := build/rpi/$(APP)
WEB_NATIVE := build/native/$(WEB_APP)$(EXT)
WEB_RPI    := build/rpi/$(WEB_APP)
LOGIN_WEB_NATIVE := build/native/$(LOGIN_WEB_APP)$(EXT)
LOGIN_WEB_RPI    := build/rpi/$(LOGIN_WEB_APP)
LOGIN_OPENSSL_NATIVE := build/native/$(LOGIN_OPENSSL_APP)
CREATE_USER_NATIVE := build/native/$(CREATE_USER_APP)$(EXT)
.DEFAULT_GOAL := native
.PHONY: native rpi web web-rpi web-login web-login-openssl \
        web-login-rpi clean
native:
	@echo [HOST] $(HOST)
	@$(call MKDIR,build/native)
	@gcc $(CFLAGS) $(SRC) -o $(NATIVE) $(LIBS)
	@echo [OK] Native cikti: $(NATIVE)
rpi:
	@$(call MKDIR,build/rpi)
	@echo [TOOLCHAIN] host=$(HOST) target=aarch64-none-linux-gnu
	@$(RPI_CC) $(CFLAGS) $(SRC) -o $(RPI) $(RPI_LIBS)
	@$(call ELF_INFO,$(RPI))
	@echo [OK] Cross-compile tamamlandi: $(RPI)
web:
	@echo [HOST] $(HOST) Mongoose Web
	@$(call MKDIR,build/native)
	@gcc $(WEB_CFLAGS) $(WEB_SRC) -o $(WEB_NATIVE) $(LIBS)
	@echo [OK] Web cikti: $(WEB_NATIVE)
web-rpi:
	@$(call MKDIR,build/rpi)
	@echo [TOOLCHAIN] host=$(HOST) target=aarch64-none-linux-gnu web
	@$(RPI_CC) $(WEB_CFLAGS) $(WEB_SRC) -o $(WEB_RPI) $(RPI_LIBS)
	@$(call ELF_INFO,$(WEB_RPI))
	@echo [OK] Web cross-compile tamamlandi: $(WEB_RPI)
web-login:
	@echo [HOST] $(HOST) Mongoose Login Web
	@$(call MKDIR,build/native)
	@gcc $(WEB_CFLAGS) $(LOGIN_WEB_SRC) -o $(LOGIN_WEB_NATIVE) $(LIBS)
	@gcc $(WEB_CFLAGS) tools/create_user.c third_party/mongoose/mongoose.c \
		-o $(CREATE_USER_NATIVE) $(LIBS)
	@$(call COPY_FILE,web/login.html,build/native/login.html)
	@$(call COPY_FILE,web/dashboard.html,build/native/dashboard.html)
	@$(call COPY_FILE,web/chat.html,build/native/chat.html)
	@$(call COPY_FILE,web/saw.html,build/native/saw.html)
	@$(call COPY_FILE,web/log.html,build/native/log.html)
	@$(call COPY_FILE,web/status.html,build/native/status.html)
	@$(call COPY_FILE,users.example.json,build/native/users.example.json)
	@echo [OK] Login web cikti: $(LOGIN_WEB_NATIVE)
	@echo [OK] Kullanici araci: $(CREATE_USER_NATIVE)
web-login-openssl:
ifeq ($(OS),Windows_NT)
	@echo [ERROR] OpenSSL hedefi Linux/Raspberry Pi uzerinde derlenmelidir.
	@exit /B 1
else
	@echo [HOST] $(HOST) Mongoose Login Web OpenSSL
	@$(call MKDIR,build/native)
	@gcc $(OPENSSL_WEB_CFLAGS) $(LOGIN_WEB_SRC) -o $(LOGIN_OPENSSL_NATIVE) \
		-pthread -lssl -lcrypto
	@$(call COPY_FILE,web/login.html,build/native/login.html)
	@$(call COPY_FILE,web/dashboard.html,build/native/dashboard.html)
	@$(call COPY_FILE,web/chat.html,build/native/chat.html)
	@$(call COPY_FILE,web/saw.html,build/native/saw.html)
	@$(call COPY_FILE,web/log.html,build/native/log.html)
	@$(call COPY_FILE,web/status.html,build/native/status.html)
	@$(call COPY_FILE,users.example.json,build/native/users.example.json)
	@echo [OK] OpenSSL login web cikti: $(LOGIN_OPENSSL_NATIVE)
endif
web-login-rpi:
	@$(call MKDIR,build/rpi)
	@echo [TOOLCHAIN] host=$(HOST) target=aarch64-none-linux-gnu login-web
	@$(RPI_CC) $(WEB_CFLAGS) $(LOGIN_WEB_SRC) -o $(LOGIN_WEB_RPI) $(RPI_LIBS)
	@$(call ELF_INFO,$(LOGIN_WEB_RPI))
	@$(call COPY_FILE,web/login.html,build/rpi/login.html)
	@$(call COPY_FILE,web/dashboard.html,build/rpi/dashboard.html)
	@$(call COPY_FILE,web/chat.html,build/rpi/chat.html)
	@$(call COPY_FILE,web/saw.html,build/rpi/saw.html)
	@$(call COPY_FILE,web/log.html,build/rpi/log.html)
	@$(call COPY_FILE,web/status.html,build/rpi/status.html)
	@$(call COPY_FILE,users.example.json,build/rpi/users.example.json)
	@echo [OK] Login web cross-compile tamamlandi: $(LOGIN_WEB_RPI)
clean:
ifeq ($(OS),Windows_NT)
	@if exist build rmdir /S /Q build
else
	@rm -rf build
endif
	@echo [OK] Build klasoru temizlendi.
