#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <conio.h>
#else
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#endif

#include "mongoose.h"

#define USERNAME_SIZE 65
#define PASSWORD_SIZE 129
#define SALT_SIZE 16
#define HASH_SIZE 32
#define DEFAULT_ITERATIONS 210000UL

static int username_is_valid(const char *username)
{
    size_t i;
    size_t length = username == NULL ? 0 : strlen(username);

    if (length == 0 || length >= USERNAME_SIZE) return 0;
    for (i = 0; i < length; i++) {
        unsigned char character = (unsigned char)username[i];
        if (!isalnum(character) && character != '_' && character != '-' && character != '.') return 0;
    }
    return 1;
}

static void pbkdf2_sha256(const char *password, const unsigned char salt[SALT_SIZE], uint32_t iterations, unsigned char output[HASH_SIZE])
{
    unsigned char block[SALT_SIZE + 4], digest[HASH_SIZE], accumulator[HASH_SIZE];
    size_t password_size = strlen(password), i;
    uint32_t round;
    memcpy(block, salt, SALT_SIZE);
    block[SALT_SIZE] = 0; block[SALT_SIZE + 1] = 0; block[SALT_SIZE + 2] = 0; block[SALT_SIZE + 3] = 1;
    mg_hmac_sha256(digest, (uint8_t *)password, password_size, block, sizeof(block));
    memcpy(accumulator, digest, sizeof(accumulator));
    for (round = 1; round < iterations; round++) {
        mg_hmac_sha256(digest, (uint8_t *)password, password_size, digest, sizeof(digest));
        for (i = 0; i < sizeof(accumulator); i++) accumulator[i] ^= digest[i];
    }
    memcpy(output, accumulator, HASH_SIZE);
    mg_bzero(block, sizeof(block)); mg_bzero(digest, sizeof(digest)); mg_bzero(accumulator, sizeof(accumulator));
}

static int read_password(const char *prompt, char *output, size_t output_size)
{
    size_t length = 0;
    fputs(prompt, stdout); fflush(stdout);
#ifdef _WIN32
    for (;;) {
        int character = _getch();
        if (character == '\r' || character == '\n') break;
        if (character == 3) { putchar('\n'); return 0; }
        if (character == 8) { if (length > 0) length--; continue; }
        if (character == 0 || character == 224) { (void)_getch(); continue; }
        if (character >= 32 && character <= 255 && length + 1 < output_size) output[length++] = (char)character;
    }
    output[length] = '\0'; putchar('\n');
#else
    struct termios previous, hidden;
    int terminal_ready = tcgetattr(STDIN_FILENO, &previous) == 0;
    if (terminal_ready) { hidden = previous; hidden.c_lflag &= (tcflag_t)~ECHO; tcsetattr(STDIN_FILENO, TCSAFLUSH, &hidden); }
    if (fgets(output, (int)output_size, stdin) == NULL) {
        if (terminal_ready) tcsetattr(STDIN_FILENO, TCSAFLUSH, &previous);
        putchar('\n'); return 0;
    }
    if (terminal_ready) tcsetattr(STDIN_FILENO, TCSAFLUSH, &previous);
    putchar('\n'); length = strcspn(output, "\r\n"); output[length] = '\0';
#endif
    return 1;
}

static int file_exists(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL) return 0;
    fclose(file); return 1;
}

static void print_hex(FILE *file, const unsigned char *data, size_t size)
{
    static const char digits[] = "0123456789abcdef";
    size_t i;
    for (i = 0; i < size; i++) { fputc(digits[data[i] >> 4], file); fputc(digits[data[i] & 0x0f], file); }
}

static int write_user_file(const char *path, const char *username, const unsigned char salt[SALT_SIZE], const unsigned char hash[HASH_SIZE], uint32_t iterations)
{
    char temporary[1024]; FILE *file; int written, write_failed, close_failed;
    written = snprintf(temporary, sizeof(temporary), "%s.tmp", path);
    if (written < 0 || (size_t)written >= sizeof(temporary)) { fprintf(stderr, "Dosya yolu cok uzun.\n"); return 0; }
    file = fopen(temporary, "wb");
    if (file == NULL) { fprintf(stderr, "Gecici kullanici dosyasi acilamadi.\n"); return 0; }
    fprintf(file, "{\n  \"version\": 1,\n  \"users\": [\n    {\n      \"username\": \"%s\",\n      \"salt\": \"", username);
    print_hex(file, salt, SALT_SIZE);
    fprintf(file, "\",\n      \"iterations\": %lu,\n      \"password_hash\": \"", (unsigned long)iterations);
    print_hex(file, hash, HASH_SIZE);
    fprintf(file, "\",\n      \"enabled\": true\n    }\n  ]\n}\n");
    write_failed = ferror(file) || fflush(file) != 0; close_failed = fclose(file) != 0;
    if (write_failed || close_failed) { fprintf(stderr, "Kullanici dosyasi yazilamadi.\n"); remove(temporary); return 0; }
#ifndef _WIN32
    (void)chmod(temporary, S_IRUSR | S_IWUSR);
#endif
    if (rename(temporary, path) != 0) { fprintf(stderr, "Kullanici dosyasi son konumuna tasinamadi.\n"); remove(temporary); return 0; }
    return 1;
}

static void show_usage(const char *program)
{
    printf("Kullanim: %s KULLANICI_ADI [--file users.json] [--iterations SAYI]\n", program);
}

int main(int argc, char *argv[])
{
    const char *username, *path = "users.json";
    unsigned long iterations = DEFAULT_ITERATIONS;
    char password[PASSWORD_SIZE] = {0}, confirmation[PASSWORD_SIZE] = {0};
    unsigned char salt[SALT_SIZE] = {0}, hash[HASH_SIZE] = {0};
    int i, result = 1;
    if (argc < 2) { show_usage(argv[0]); return 1; }
    username = argv[1];
    for (i = 2; i < argc; i++) {
        if (i + 1 >= argc) { show_usage(argv[0]); return 1; }
        if (strcmp(argv[i], "--file") == 0) path = argv[++i];
        else if (strcmp(argv[i], "--iterations") == 0) {
            char *end; iterations = strtoul(argv[++i], &end, 10);
            if (*argv[i] == '\0' || *end != '\0' || iterations < 10000UL || iterations > 2000000UL) { fprintf(stderr, "Iterations 10000-2000000 arasinda olmali.\n"); return 1; }
        } else { show_usage(argv[0]); return 1; }
    }
    if (!username_is_valid(username)) { fprintf(stderr, "Kullanici adi gecersiz.\n"); return 1; }
    if (file_exists(path)) { fprintf(stderr, "Dosya zaten var; guvenlik icin uzerine yazilmadi: %s\n", path); return 1; }
    if (!read_password("Sifre: ", password, sizeof(password)) || !read_password("Sifre tekrar: ", confirmation, sizeof(confirmation))) goto cleanup;
    if (strlen(password) < 8 || strlen(password) > 128) { fprintf(stderr, "Sifre 8-128 karakter olmali.\n"); goto cleanup; }
    if (strlen(password) != strlen(confirmation) || !mg_memeq(password, confirmation, strlen(password))) { fprintf(stderr, "Sifreler eslesmiyor.\n"); goto cleanup; }
    if (!mg_random(salt, sizeof(salt))) { fprintf(stderr, "Guvenli salt olusturulamadi.\n"); goto cleanup; }
    pbkdf2_sha256(password, salt, (uint32_t)iterations, hash);
    if (!write_user_file(path, username, salt, hash, (uint32_t)iterations)) goto cleanup;
    printf("Kullanici kaydedildi: %s\n", path); result = 0;
cleanup:
    mg_bzero((unsigned char *)password, sizeof(password)); mg_bzero((unsigned char *)confirmation, sizeof(confirmation)); mg_bzero(salt, sizeof(salt)); mg_bzero(hash, sizeof(hash));
    return result;
}
