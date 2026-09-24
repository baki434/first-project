        }
        send_json(state, connection, 200, cookie, response);
    }

cleanup:
    if (password != NULL) {
        mg_bzero((unsigned char *)password, strlen(password));
    }
    mg_bzero((unsigned char *)token, sizeof(token));
    mg_bzero(token_digest, sizeof(token_digest));
    mg_free(username);
    mg_free(password);
}

static void handle_session(login_server_state_t *state,
                           struct mg_connection *connection,
                           struct mg_http_message *request)
{
    char username[LOGIN_USERNAME_SIZE];

    if (!method_is(request, "GET")) {
        send_json(state, connection, 405, "Allow: GET\r\n",
                  "{\"error\":\"Yontem desteklenmiyor\"}\n");
    } else if (request_session(state, request, username, NULL)) {
        char response[160];
        snprintf(response, sizeof(response),
                 "{\"authenticated\":true,\"username\":\"%s\"}\n",
                 username);
        send_json(state, connection, 200, NULL, response);
    } else {
        send_json(state, connection, 401, NULL,
                  "{\"authenticated\":false}\n");
    }
}

static void handle_logout(login_server_state_t *state,
                          struct mg_connection *connection,
                          struct mg_http_message *request)
{
    unsigned char hash[LOGIN_SESSION_HASH_SIZE];
    int invalidated;
    const char *secure = state->tls_enabled ? "; Secure" : "";
    char cookie[220];

    if (!method_is(request, "POST")) {
        send_json(state, connection, 405, "Allow: POST\r\n",
                  "{\"error\":\"Yontem desteklenmiyor\"}\n");
        return;
    }
    if (!same_origin_if_present(state, request)) {
        send_json(state, connection, 403, NULL,
                  "{\"error\":\"Kaynak reddedildi\"}\n");
        return;
    }
    invalidated = login_session_invalidate(&state->sessions,
                                            session_cookie(request), hash);
    if (invalidated) {
        close_session_websockets(connection->mgr, hash);
    }
    snprintf(cookie, sizeof(cookie),
             "Set-Cookie: saw_session=; Path=/; HttpOnly; SameSite=Strict; "
             "Max-Age=0%s\r\n", secure);
    send_json(state, connection, 200, cookie, "{\"ok\":true}\n");
    mg_bzero(hash, sizeof(hash));
}

static void broadcast_message(login_server_state_t *state,
                              struct mg_connection *source,
                              const char *message)
{
    struct mg_connection *current;
    char output[MAX_MESSAGE_SIZE + 128];
    char ts[64];
    int number = client_number(state, source->id);
    int written;

    if (number == 0) {
        send_websocket_text(source, "@status:Sunucu dolu");
        return;
    }
    format_timestamp(ts, sizeof(ts));
    written = snprintf(output, sizeof(output), "Client%d: %s - %s", number, message, ts);
    if (written < 0 || (size_t)written >= sizeof(output)) {
        send_websocket_text(source, "@status:Mesaj cok uzun");
        return;
    }
    log_line(state, "%s", output);
    for (current = source->mgr->conns; current != NULL; current = current->next) {
        if (current->is_websocket &&
            login_session_validate_hash(&state->sessions, (unsigned char *)current->data, NULL)) {
            send_websocket_text(current, output);
        }
    }
}

static void handle_websocket_message(login_server_state_t *state,
                                     struct mg_connection *connection,
                                     const struct mg_ws_message *message)
{
    char input[MAX_MESSAGE_SIZE];
    if (!login_session_validate_hash(&state->sessions, (unsigned char *)connection->data, NULL)) {
        mg_ws_send(connection, "", 0, WEBSOCKET_OP_CLOSE);
        connection->is_draining = 1;
        return;
    }
    if ((message->flags & 0x0f) != WEBSOCKET_OP_TEXT) {
        send_websocket_text(connection, "@status:Yalnizca metin mesaji desteklenir");
        return;
    }
    if (message->data.len == 0 || message->data.len >= sizeof(input)) {
        send_websocket_text(connection, "@status:Mesaj bos veya izin verilenden uzun");
        return;
    }
    memcpy(input, message->data.buf, message->data.len);
    input[message->data.len] = '\0';
    broadcast_message(state, connection, input);
}

static int authenticated(login_server_state_t *state, struct mg_http_message *request, char *username)
{
    return request_session(state, request, username, NULL);
}

static void serve_page(login_server_state_t *state, struct mg_connection *connection,
                       struct mg_http_message *request, const char *path)
{
    char headers[1400];
    struct mg_http_serve_opts options = {0};
    if (!method_is(request, "GET")) {
        send_json(state, connection, 405, "Allow: GET\r\n", "{\"error\":\"Yontem desteklenmiyor\"}\n");
        return;
    }
    if (!build_headers(state, NULL, NULL, headers, sizeof(headers))) {
        mg_http_reply(connection, 500, "", "Sunucu hatasi\n");
        return;
    }
    options.extra_headers = headers;
    options.mime_types = "html=text/html; charset=utf-8";
    mg_http_serve_file(connection, request, path, &options);
}


static void handle_log_api(login_server_state_t *state, struct mg_connection *connection,
                           struct mg_http_message *request)
{
    FILE *f; long size; char *body; char headers[1400];
    if (!method_is(request, "GET")) {
        send_json(state, connection, 405, "Allow: GET\r\n", "{\"error\":\"Yontem desteklenmiyor\"}\n");
        return;
    }
    fflush(state->log_file);
    f = fopen(state->log_path, "rb");
    if (!f) { send_json(state, connection, 500, NULL, "{\"error\":\"Log dosyasi okunamadi\"}\n"); return; }
    if (fseek(f, 0, SEEK_END) != 0 || (size = ftell(f)) < 0 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f); send_json(state, connection, 500, NULL, "{\"error\":\"Log dosyasi okunamadi\"}\n"); return;
    }
    body = (char *)malloc((size_t)size + 1);
    if (!body) { fclose(f); send_json(state, connection, 500, NULL, "{\"error\":\"Bellek hatasi\"}\n"); return; }
    if (size > 0 && fread(body, 1, (size_t)size, f) != (size_t)size) {
        fclose(f); free(body); send_json(state, connection, 500, NULL, "{\"error\":\"Log dosyasi okunamadi\"}\n"); return;
    }
    fclose(f); body[size] = '\0';
    if (!build_headers(state, "text/plain; charset=utf-8", NULL, headers, sizeof(headers))) {
        free(body); mg_http_reply(connection, 500, "", "Sunucu hatasi\n"); return;
    }
    mg_http_reply(connection, 200, headers, "%s", body);
    free(body);
}

static void handle_chat_history(login_server_state_t *state, struct mg_connection *connection,
                                struct mg_http_message *request)
{
    FILE *f; char line[2048]; char *body = NULL; size_t used = 0, cap = 1;
    if (!method_is(request, "GET")) { send_json(state, connection, 405, "Allow: GET\r\n", "{\"error\":\"Yontem desteklenmiyor\"}\n"); return; }
    if (!authenticated(state, request, NULL)) { send_json(state, connection, 401, NULL, "{\"error\":\"Oturum gerekli\"}\n"); return; }
    body = (char *)malloc(cap); if (!body) { send_json(state, connection, 500, NULL, "{\"error\":\"Bellek hatasi\"}\n"); return; } body[0]='\0';
    fflush(state->log_file);
    f = fopen(state->log_path, "r");
    if (!f) { free(body); send_json(state, connection, 500, NULL, "{\"error\":\"Log okunamadi\"}\n"); return; }
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "Client1:",8)&&strncmp(line,"Client2:",8)&&strncmp(line,"Client3:",8)&&strncmp(line,"Client4:",8)) continue;
        { size_t n=strlen(line); if (used+n+1>cap) { size_t nc=(used+n+1)*2; char *tmp=(char*)realloc(body,nc); if(!tmp){fclose(f);free(body);send_json(state,connection,500,NULL,"{\"error\":\"Bellek hatasi\"}\n");return;} body=tmp;cap=nc;} memcpy(body+used,line,n);used+=n;body[used]='\0'; }
    }
    fclose(f);
    { char headers[1400]; build_headers(state,"text/plain; charset=utf-8",NULL,headers,sizeof(headers)); mg_http_reply(connection,200,headers,"%s",body); }
    free(body);
}

static void handle_saw_api(login_server_state_t *state, struct mg_connection *connection,
                           struct mg_http_message *request)
{
    char username[LOGIN_USERNAME_SIZE]; char *cmd=NULL; char response[MAX_MESSAGE_SIZE];
    if (!method_is(request,"POST")) { send_json(state,connection,405,"Allow: POST\r\n","{\"error\":\"Yontem desteklenmiyor\"}\n"); return; }
    if (!authenticated(state,request,username)) { send_json(state,connection,401,NULL,"{\"error\":\"Oturum gerekli\"}\n"); return; }
    cmd=mg_json_get_str(request->body,"$.command");
    if(!cmd || !handle_saw_command(cmd,response,sizeof(response))) { mg_free(cmd); send_json(state,connection,400,NULL,"{\"error\":\"Gecersiz SAW komutu\"}\n"); return; }
    { char ts[64]; format_timestamp(ts,sizeof(ts)); log_line(state,"%s SAW komutu kullandi: %s - %s",username,cmd,ts); }
    { char *escaped=mg_json_get_str(mg_str("{}"),"$.x"); (void)escaped; }
    { char json[MAX_MESSAGE_SIZE+128]; size_t j=0,i; j+=(size_t)snprintf(json,sizeof(json),"{\"output\":\""); for(i=0;response[i]&&j+8<sizeof(json);++i){unsigned char c=(unsigned char)response[i]; if(c=='\"'||c=='\\'){json[j++]='\\';json[j++]=(char)c;} else if(c=='\n'){json[j++]='\\';json[j++]='n';} else if(c=='\r'){} else if(c<0x20){ } else json[j++]=(char)c;} snprintf(json+j,sizeof(json)-j,"\"}\n"); send_json(state,connection,200,NULL,json); }
    mg_free(cmd);
