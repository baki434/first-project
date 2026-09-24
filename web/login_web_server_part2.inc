
    if (cookie == NULL) {
        return mg_str_n(NULL, 0);
    }
    return mg_http_get_header_var(*cookie, mg_str("saw_session"));
}

static int request_session(login_server_state_t *state,
                           const struct mg_http_message *request,
                           char username[LOGIN_USERNAME_SIZE],
                           unsigned char hash[LOGIN_SESSION_HASH_SIZE])
{
    return login_session_validate_token(&state->sessions,
                                        session_cookie(request), username,
                                        hash);
}

static int same_origin_if_present(const login_server_state_t *state,
                                  struct mg_http_message *request)
{
    struct mg_str *origin = mg_http_get_header(request, "Origin");
    struct mg_str *host = mg_http_get_header(request, "Host");
    char expected[320];
    int written;

    if (origin == NULL) {
        return 1;
    }
    if (host == NULL || host->len == 0 || host->len > 255) {
        return 0;
    }
    written = snprintf(expected, sizeof(expected), "%s://%.*s",
                       state->tls_enabled ? "https" : "http",
                       (int)host->len, host->buf);
    return written > 0 && (size_t)written < sizeof(expected) &&
           mg_strcmp(*origin, mg_str(expected)) == 0;
}

static login_rate_entry_t *rate_entry(login_server_state_t *state,
                                      const struct mg_addr *address)
{
    size_t address_size = address->is_ip6 ? 16U : 4U;
    login_rate_entry_t *free_entry = NULL;
    login_rate_entry_t *oldest = &state->rates[0];
    size_t i;

    for (i = 0; i < LOGIN_RATE_SLOTS; i++) {
        login_rate_entry_t *entry = &state->rates[i];

        if (!entry->used && free_entry == NULL) {
            free_entry = entry;
        } else if (entry->used && entry->is_ip6 == (int)address->is_ip6 &&
                   memcmp(entry->ip, address->addr.ip, address_size) == 0) {
            return entry;
        }
        if (entry->window_started < oldest->window_started) {
            oldest = entry;
        }
    }
    if (free_entry == NULL) {
        free_entry = oldest;
    }
    memset(free_entry, 0, sizeof(*free_entry));
    memcpy(free_entry->ip, address->addr.ip, address_size);
    free_entry->is_ip6 = address->is_ip6 ? 1 : 0;
    free_entry->window_started = time(NULL);
    free_entry->used = 1;
    return free_entry;
}

static int rate_is_blocked(login_server_state_t *state,
                           const struct mg_addr *address)
{
    login_rate_entry_t *entry = rate_entry(state, address);
    time_t now = time(NULL);

    if (now != (time_t)-1 &&
        now - entry->window_started >= LOGIN_RATE_WINDOW_SECONDS) {
        entry->failures = 0;
        entry->window_started = now;
    }
    return entry->failures >= LOGIN_RATE_MAX_FAILURES;
}

static void rate_record(login_server_state_t *state,
                        const struct mg_addr *address,
                        int success)
{
    login_rate_entry_t *entry = rate_entry(state, address);

    if (success) {
        entry->failures = 0;
        entry->window_started = time(NULL);
    } else if (entry->failures < LOGIN_RATE_MAX_FAILURES) {
        entry->failures++;
    }
}

static void send_json(login_server_state_t *state,
                      struct mg_connection *connection,
                      int status,
                      const char *extra_headers,
                      const char *json)
{
    char headers[1400];

    if (!build_headers(state, "application/json; charset=utf-8",
                       extra_headers, headers, sizeof(headers))) {
        mg_http_reply(connection, 500, "", "Sunucu hatasi\n");
        return;
    }
    mg_http_reply(connection, status, headers, "%s", json);
}

static void send_websocket_text(struct mg_connection *connection,
                                const char *text)
{
    if (mg_ws_send(connection, text, strlen(text), WEBSOCKET_OP_TEXT) == 0) {
        fprintf(stderr, "WebSocket mesaji kuyruga alinamadi.\n");
    }
}

static void close_session_websockets(struct mg_mgr *manager,
                                     const unsigned char hash[32])
{
    struct mg_connection *current;

    for (current = manager->conns; current != NULL; current = current->next) {
        if (current->is_websocket &&
            mg_memeq(current->data, hash, LOGIN_SESSION_HASH_SIZE)) {
            mg_ws_send(current, "", 0, WEBSOCKET_OP_CLOSE);
            current->is_draining = 1;
        }
    }
}

static void handle_login(login_server_state_t *state,
                         struct mg_connection *connection,
                         struct mg_http_message *request)
{
    struct mg_str *content_type;
    char *username = NULL;
    char *password = NULL;
    char token[LOGIN_TOKEN_HEX_SIZE];
    unsigned char token_digest[LOGIN_SESSION_HASH_SIZE];
    char cookie[256];
    int verified = 0;

    if (!method_is(request, "POST")) {
        send_json(state, connection, 405, "Allow: POST\r\n",
                  "{\"error\":\"Yontem desteklenmiyor\"}\n");
        return;
    }
    content_type = mg_http_get_header(request, "Content-Type");
    if (request->body.len == 0 || request->body.len > LOGIN_MAX_BODY_SIZE ||
        content_type == NULL ||
        !mg_match(*content_type, mg_str("application/json*"), NULL) ||
        !same_origin_if_present(state, request)) {
        send_json(state, connection, 400, NULL,
                  "{\"error\":\"Gecersiz istek\"}\n");
        return;
    }
    if (rate_is_blocked(state, &connection->rem)) {
        send_json(state, connection, 429, "Retry-After: 60\r\n",
                  "{\"error\":\"Cok fazla basarisiz deneme\"}\n");
        return;
    }

    username = mg_json_get_str(request->body, "$.username");
    password = mg_json_get_str(request->body, "$.password");
    if (username != NULL && password != NULL) {
        verified = login_auth_verify(&state->auth, username, password);
    }
    if (!verified) {
        rate_record(state, &connection->rem, 0);
        send_json(state, connection, 401, NULL,
                  "{\"error\":\"Kullanici adi veya sifre hatali\"}\n");
        goto cleanup;
    }
    rate_record(state, &connection->rem, 1);
    if (!login_session_create(&state->sessions, username, token,
                              token_digest)) {
        send_json(state, connection, 503, NULL,
                  "{\"error\":\"Oturum olusturulamadi\"}\n");
        goto cleanup;
    }
    snprintf(cookie, sizeof(cookie),
             "Set-Cookie: saw_session=%s; Path=/; HttpOnly; "
             "SameSite=Strict; Max-Age=%d%s\r\n",
             token, LOGIN_SESSION_SECONDS,
             state->tls_enabled ? "; Secure" : "");
    {
        char response[160];
        snprintf(response, sizeof(response),
                 "{\"authenticated\":true,\"username\":\"%s\"}\n",
                 username);
        {
            char ts[64];
            format_timestamp(ts, sizeof(ts));
            log_line(state, "%s giris yapti - %s", username, ts);
