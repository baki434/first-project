#ifndef LOGIN_WEB_SERVER_H
#define LOGIN_WEB_SERVER_H

int run_login_web_server(int port,
                         const char *index_path,
                         const char *users_path,
                         const char *certificate_path,
                         const char *private_key_path,
                         int enable_hsts);

#endif
