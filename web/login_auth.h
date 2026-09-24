#ifndef LOGIN_AUTH_H
#define LOGIN_AUTH_H

#include <stddef.h>
#include <stdint.h>

#define LOGIN_MAX_USERS 32
#define LOGIN_USERNAME_SIZE 65
#define LOGIN_SALT_MAX_SIZE 32
#define LOGIN_PASSWORD_MAX_SIZE 128

typedef struct {
    char username[LOGIN_USERNAME_SIZE];
    unsigned char salt[LOGIN_SALT_MAX_SIZE];
    size_t salt_size;
    uint32_t iterations;
    unsigned char password_hash[32];
    int enabled;
} login_user_t;

typedef struct {
    login_user_t users[LOGIN_MAX_USERS];
    size_t count;
} login_auth_store_t;

int login_auth_load(login_auth_store_t *store, const char *path);
int login_auth_verify(const login_auth_store_t *store,
                      const char *username,
                      const char *password);
void login_auth_clear(login_auth_store_t *store);

#endif
