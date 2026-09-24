#ifndef LOGIN_SESSION_H
#define LOGIN_SESSION_H

#include <stddef.h>
#include <time.h>

#include "login_auth.h"
#include "mongoose.h"

#define LOGIN_MAX_SESSIONS 32
#define LOGIN_TOKEN_HEX_SIZE 65
#define LOGIN_SESSION_HASH_SIZE 32
#define LOGIN_SESSION_SECONDS 1800

typedef struct {
    unsigned char token_hash[LOGIN_SESSION_HASH_SIZE];
    char username[LOGIN_USERNAME_SIZE];
    time_t expires_at;
    int used;
} login_session_t;

typedef struct {
    login_session_t entries[LOGIN_MAX_SESSIONS];
} login_session_store_t;

void login_sessions_init(login_session_store_t *store);
int login_session_create(login_session_store_t *store,
                         const char *username,
                         char token[LOGIN_TOKEN_HEX_SIZE],
                         unsigned char hash[LOGIN_SESSION_HASH_SIZE]);
int login_session_validate_token(login_session_store_t *store,
                                 struct mg_str token,
                                 char username[LOGIN_USERNAME_SIZE],
                                 unsigned char hash[LOGIN_SESSION_HASH_SIZE]);
int login_session_validate_hash(login_session_store_t *store,
                                const unsigned char hash[LOGIN_SESSION_HASH_SIZE],
                                char username[LOGIN_USERNAME_SIZE]);
int login_session_invalidate(login_session_store_t *store,
                             struct mg_str token,
                             unsigned char hash[LOGIN_SESSION_HASH_SIZE]);
void login_sessions_clear(login_session_store_t *store);

#endif
