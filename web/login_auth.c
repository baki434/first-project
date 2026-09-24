#include "login_auth.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "mongoose.h"

#define LOGIN_MIN_ITERATIONS 10000UL
#define LOGIN_MAX_ITERATIONS 2000000UL
#define LOGIN_MAX_JSON_SIZE (64UL * 1024UL)

static int username_is_valid(const char *username)
{
    size_t i;
    size_t length;

    if (username == NULL) {
        return 0;
    }
    length = strlen(username);
    if (length == 0 || length >= LOGIN_USERNAME_SIZE) {
        return 0;
    }
    for (i = 0; i < length; i++) {
        unsigned char character = (unsigned char)username[i];

        if (!isalnum(character) && character != '_' && character != '-' &&
            character != '.') {
            return 0;
        }
    }
    return 1;
}

static void pbkdf2_sha256(const char *password,
                          const unsigned char *salt,
                          size_t salt_size,
                          uint32_t iterations,
                          unsigned char output[32])
{
    unsigned char block[LOGIN_SALT_MAX_SIZE + 4];
    unsigned char digest[32];
    unsigned char accumulator[32];
    size_t password_size = strlen(password);
    uint32_t round;
    size_t i;

    memcpy(block, salt, salt_size);
    block[salt_size] = 0;
    block[salt_size + 1] = 0;
    block[salt_size + 2] = 0;
    block[salt_size + 3] = 1;

    mg_hmac_sha256(digest, (uint8_t *)password, password_size, block,
                   salt_size + 4);
    memcpy(accumulator, digest, sizeof(accumulator));
    for (round = 1; round < iterations; round++) {
        mg_hmac_sha256(digest, (uint8_t *)password, password_size,
                       digest, sizeof(digest));
        for (i = 0; i < sizeof(accumulator); i++) {
            accumulator[i] ^= digest[i];
        }
    }
    memcpy(output, accumulator, 32);
    mg_bzero(block, sizeof(block));
    mg_bzero(digest, sizeof(digest));
    mg_bzero(accumulator, sizeof(accumulator));
}

static int user_is_duplicate(const login_auth_store_t *store,
                             const char *username)
{
    size_t i;

    for (i = 0; i < store->count; i++) {
        if (strcmp(store->users[i].username, username) == 0) {
            return 1;
        }
    }
    return 0;
}

int login_auth_load(login_auth_store_t *store, const char *path)
{
    struct mg_str json = {0};
    struct mg_str users;
    struct mg_str item;
    size_t offset = 0;
    long version;
    int result = 0;

    if (store == NULL || path == NULL) {
        return 0;
    }
    memset(store, 0, sizeof(*store));
    json = mg_file_read(&mg_fs_posix, path);
    if (json.buf == NULL || json.len == 0 || json.len > LOGIN_MAX_JSON_SIZE) {
        fprintf(stderr, "Kullanici dosyasi okunamadi veya cok buyuk: %s\n",
                path);
        goto cleanup;
    }

    version = mg_json_get_long(json, "$.version", -1);
    users = mg_json_get_tok(json, "$.users");
    if (version != 1 || users.buf == NULL || users.len < 2 ||
        users.buf[0] != '[') {
        fprintf(stderr, "Kullanici dosyasi semasi gecersiz.\n");
        goto cleanup;
    }

    while ((offset = mg_json_next(users, offset, NULL, &item)) > 0) {
        login_user_t *user;
        char *username = NULL;
        char *salt = NULL;
        char *hash = NULL;
        int salt_size = 0;
        int hash_size = 0;
        long iterations;
        bool enabled = true;
        struct mg_str plaintext_password;

        if (store->count >= LOGIN_MAX_USERS) {
            fprintf(stderr, "En fazla %d kullanici desteklenir.\n",
                    LOGIN_MAX_USERS);
            goto cleanup;
        }
        username = mg_json_get_str(item, "$.username");
        salt = mg_json_get_hex(item, "$.salt", &salt_size);
        hash = mg_json_get_hex(item, "$.password_hash", &hash_size);
        iterations = mg_json_get_long(item, "$.iterations", -1);
        (void)mg_json_get_bool(item, "$.enabled", &enabled);
        plaintext_password = mg_json_get_tok(item, "$.password");

        if (!username_is_valid(username) ||
            user_is_duplicate(store, username) || salt == NULL ||
            salt_size < 16 || salt_size > LOGIN_SALT_MAX_SIZE ||
            hash == NULL || hash_size != 32 ||
            iterations < (long)LOGIN_MIN_ITERATIONS ||
            iterations > (long)LOGIN_MAX_ITERATIONS ||
            plaintext_password.buf != NULL) {
            fprintf(stderr, "Kullanici kaydi gecersiz veya yinelenmis.\n");
            mg_free(username);
            mg_free(salt);
            mg_free(hash);
            goto cleanup;
        }

        user = &store->users[store->count++];
        snprintf(user->username, sizeof(user->username), "%s", username);
        memcpy(user->salt, salt, (size_t)salt_size);
        user->salt_size = (size_t)salt_size;
        user->iterations = (uint32_t)iterations;
        memcpy(user->password_hash, hash, sizeof(user->password_hash));
        user->enabled = enabled ? 1 : 0;

        mg_bzero((unsigned char *)username, strlen(username));
        mg_bzero((unsigned char *)salt, (size_t)salt_size);
        mg_bzero((unsigned char *)hash, (size_t)hash_size);
        mg_free(username);
        mg_free(salt);
        mg_free(hash);
    }

    if (store->count == 0) {
        fprintf(stderr, "Kullanici dosyasinda kullanici bulunamadi.\n");
        goto cleanup;
    }
    result = 1;

cleanup:
    mg_free(json.buf);
    if (!result) {
        login_auth_clear(store);
    }
    return result;
}

int login_auth_verify(const login_auth_store_t *store,
                      const char *username,
                      const char *password)
{
    unsigned char calculated[32];
    const login_user_t *selected = NULL;
    size_t i;
    int result = 0;

    if (store == NULL || !username_is_valid(username) || password == NULL ||
        strlen(password) == 0 || strlen(password) > LOGIN_PASSWORD_MAX_SIZE) {
        return 0;
    }
    for (i = 0; i < store->count; i++) {
        const login_user_t *user = &store->users[i];

        if (user->enabled && strcmp(user->username, username) == 0) {
            selected = user;
            break;
        }
    }
    /* Bilinmeyen kullanicilarda da PBKDF2 calistirarak zamanlama farkini
       azalt. Dosyada en az bir kayit oldugu yukleme sirasinda dogrulanir. */
    if (selected == NULL && store->count > 0) {
        selected = &store->users[0];
    }
    if (selected != NULL) {
        pbkdf2_sha256(password, selected->salt, selected->salt_size,
                      selected->iterations, calculated);
        result = selected->enabled &&
                 strcmp(selected->username, username) == 0 &&
                 mg_memeq(calculated, selected->password_hash,
                           sizeof(calculated));
        mg_bzero(calculated, sizeof(calculated));
    }
    return result;
}

void login_auth_clear(login_auth_store_t *store)
{
    if (store != NULL) {
        mg_bzero((unsigned char *)store, sizeof(*store));
    }
}
