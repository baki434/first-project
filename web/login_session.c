#include "login_session.h"

#include <stdio.h>
#include <string.h>

static void token_hash(struct mg_str token,
                       unsigned char output[LOGIN_SESSION_HASH_SIZE])
{
    mg_sha256(output, (uint8_t *)token.buf, token.len);
}

static void bytes_to_hex(const unsigned char *input,
                         size_t input_size,
                         char *output,
                         size_t output_size)
{
    static const char digits[] = "0123456789abcdef";
    size_t i;

    if (output_size < input_size * 2 + 1) {
        if (output_size > 0) {
            output[0] = '\0';
        }
        return;
    }
    for (i = 0; i < input_size; i++) {
        output[i * 2] = digits[input[i] >> 4];
        output[i * 2 + 1] = digits[input[i] & 0x0f];
    }
    output[input_size * 2] = '\0';
}

static void expire_old_sessions(login_session_store_t *store, time_t now)
{
    size_t i;

    for (i = 0; i < LOGIN_MAX_SESSIONS; i++) {
        if (store->entries[i].used && store->entries[i].expires_at <= now) {
            mg_bzero((unsigned char *)&store->entries[i],
                     sizeof(store->entries[i]));
        }
    }
}

void login_sessions_init(login_session_store_t *store)
{
    if (store != NULL) {
        memset(store, 0, sizeof(*store));
    }
}

int login_session_create(login_session_store_t *store,
                         const char *username,
                         char token[LOGIN_TOKEN_HEX_SIZE],
                         unsigned char hash[LOGIN_SESSION_HASH_SIZE])
{
    unsigned char random_bytes[32];
    time_t now = time(NULL);
    size_t i;
    login_session_t *entry = NULL;

    if (store == NULL || username == NULL || token == NULL || hash == NULL ||
        now == (time_t)-1) {
        return 0;
    }
    expire_old_sessions(store, now);
    for (i = 0; i < LOGIN_MAX_SESSIONS; i++) {
        if (!store->entries[i].used) {
            entry = &store->entries[i];
            break;
        }
    }
    if (entry == NULL || !mg_random(random_bytes, sizeof(random_bytes))) {
        fprintf(stderr, "Guvenli oturum belirteci olusturulamadi.\n");
        mg_bzero(random_bytes, sizeof(random_bytes));
        return 0;
    }

    bytes_to_hex(random_bytes, sizeof(random_bytes), token,
                 LOGIN_TOKEN_HEX_SIZE);
    token_hash(mg_str(token), hash);
    memcpy(entry->token_hash, hash, LOGIN_SESSION_HASH_SIZE);
    snprintf(entry->username, sizeof(entry->username), "%s", username);
    entry->expires_at = now + LOGIN_SESSION_SECONDS;
    entry->used = 1;
    mg_bzero(random_bytes, sizeof(random_bytes));
    return 1;
}

int login_session_validate_token(login_session_store_t *store,
                                 struct mg_str token,
                                 char username[LOGIN_USERNAME_SIZE],
                                 unsigned char hash[LOGIN_SESSION_HASH_SIZE])
{
    unsigned char calculated[LOGIN_SESSION_HASH_SIZE];
    int result;

    if (token.buf == NULL || token.len != LOGIN_TOKEN_HEX_SIZE - 1) {
        return 0;
    }
    token_hash(token, calculated);
    result = login_session_validate_hash(store, calculated, username);
    if (result && hash != NULL) {
        memcpy(hash, calculated, LOGIN_SESSION_HASH_SIZE);
    }
    mg_bzero(calculated, sizeof(calculated));
    return result;
}

int login_session_validate_hash(login_session_store_t *store,
                                const unsigned char hash[LOGIN_SESSION_HASH_SIZE],
                                char username[LOGIN_USERNAME_SIZE])
{
    time_t now = time(NULL);
    size_t i;

    if (store == NULL || hash == NULL || now == (time_t)-1) {
        return 0;
    }
    expire_old_sessions(store, now);
    for (i = 0; i < LOGIN_MAX_SESSIONS; i++) {
        login_session_t *entry = &store->entries[i];

        if (entry->used &&
            mg_memeq(entry->token_hash, hash, LOGIN_SESSION_HASH_SIZE)) {
            if (username != NULL) {
                snprintf(username, LOGIN_USERNAME_SIZE, "%s",
                         entry->username);
            }
            return 1;
        }
    }
    return 0;
}

int login_session_invalidate(login_session_store_t *store,
                             struct mg_str token,
                             unsigned char hash[LOGIN_SESSION_HASH_SIZE])
{
    unsigned char calculated[LOGIN_SESSION_HASH_SIZE];
    size_t i;

    if (store == NULL || token.buf == NULL ||
        token.len != LOGIN_TOKEN_HEX_SIZE - 1) {
        return 0;
    }
    token_hash(token, calculated);
    for (i = 0; i < LOGIN_MAX_SESSIONS; i++) {
        login_session_t *entry = &store->entries[i];

        if (entry->used &&
            mg_memeq(entry->token_hash, calculated,
                      LOGIN_SESSION_HASH_SIZE)) {
            if (hash != NULL) {
                memcpy(hash, calculated, LOGIN_SESSION_HASH_SIZE);
            }
            mg_bzero((unsigned char *)entry, sizeof(*entry));
            mg_bzero(calculated, sizeof(calculated));
            return 1;
        }
    }
    mg_bzero(calculated, sizeof(calculated));
    return 0;
}

void login_sessions_clear(login_session_store_t *store)
{
    if (store != NULL) {
        mg_bzero((unsigned char *)store, sizeof(*store));
    }
}
