}

static void handle_zip_api(login_server_state_t *state, struct mg_connection *connection,
                           struct mg_http_message *request)
{
    char username[LOGIN_USERNAME_SIZE];
    if (!method_is(request,"POST")) { send_json(state,connection,405,"Allow: POST\r\n","{\"error\":\"Yontem desteklenmiyor\"}\n"); return; }
    if (!authenticated(state,request,username)) { send_json(state,connection,401,NULL,"{\"error\":\"Oturum gerekli\"}\n"); return; }
    fflush(state->log_file);
    if (!mini_zip_store_file(state->zip_path,state->log_path,"server.log")) { send_json(state,connection,500,NULL,"{\"error\":\"ZIP olusturulamadi\"}\n"); return; }
    { char ts[64]; format_timestamp(ts,sizeof(ts)); log_line(state,"%s log ZIP dosyasi olusturdu - %s",username,ts); }
    send_json(state,connection,200,NULL,"{\"ok\":true}\n");
}

static void handle_http(login_server_state_t *state, struct mg_connection *connection,
                        struct mg_http_message *request)
{
    int has_session = request_session(state, request, NULL, NULL);
    if (mg_match(request->uri, mg_str("/"), NULL)) {
        redirect_to(connection, has_session ? "/panel" : "/login");
    } else if (mg_match(request->uri, mg_str("/login"), NULL)) {
        if (has_session) redirect_to(connection, "/panel"); else serve_page(state,connection,request,state->login_path);
    } else if (mg_match(request->uri, mg_str("/panel"), NULL)) {
        if (!has_session) redirect_to(connection, "/login"); else serve_page(state,connection,request,state->dashboard_path);
    } else if (mg_match(request->uri, mg_str("/mesajlasma"), NULL)) {
        if (!has_session) redirect_to(connection, "/login"); else serve_page(state,connection,request,state->chat_path);
    } else if (mg_match(request->uri, mg_str("/saw"), NULL)) {
        if (!has_session) redirect_to(connection, "/login"); else serve_page(state,connection,request,state->saw_path);
    } else if (mg_match(request->uri, mg_str("/status"), NULL)) {
        serve_page(state,connection,request,state->status_path);
    } else if (mg_match(request->uri, mg_str("/log"), NULL)) {
        if (!has_session) redirect_to(connection, "/login"); else serve_page(state,connection,request,state->log_page_path);
    } else if (mg_match(request->uri, mg_str("/api/login"), NULL)) handle_login(state,connection,request);
    else if (mg_match(request->uri, mg_str("/api/session"), NULL)) handle_session(state,connection,request);
    else if (mg_match(request->uri, mg_str("/api/logout"), NULL)) handle_logout(state,connection,request);
    else if (mg_match(request->uri, mg_str("/api/chat-history"), NULL)) handle_chat_history(state,connection,request);
    else if (mg_match(request->uri, mg_str("/api/log"), NULL)) {
        if (!has_session) send_json(state,connection,401,NULL,"{\"error\":\"Oturum gerekli\"}\n");
        else handle_log_api(state,connection,request);
    }
    else if (mg_match(request->uri, mg_str("/api/saw"), NULL)) handle_saw_api(state,connection,request);
    else if (mg_match(request->uri, mg_str("/api/zip"), NULL)) handle_zip_api(state,connection,request);
    else if (mg_match(request->uri, mg_str("/download/log.zip"), NULL)) {
        struct mg_http_serve_opts opts={0};
        if(!has_session){send_json(state,connection,401,NULL,"{\"error\":\"Oturum gerekli\"}\n");return;}
        opts.extra_headers="Content-Disposition: attachment; filename=server_log.zip\r\nCache-Control: no-store\r\n";
        mg_http_serve_file(connection,request,state->zip_path,&opts);
    } else if (mg_match(request->uri, mg_str("/health"), NULL)) {
        if(method_is(request,"GET"))send_json(state,connection,200,NULL,"{\"status\":\"ok\"}\n"); else send_json(state,connection,405,"Allow: GET\r\n","{\"error\":\"Yontem desteklenmiyor\"}\n");
    } else if (mg_match(request->uri, mg_str("/ws"), NULL)) {
        unsigned char hash[LOGIN_SESSION_HASH_SIZE];
        if (!same_origin_if_present(state,request)) send_json(state,connection,403,NULL,"{\"error\":\"Kaynak reddedildi\"}\n");
        else if (request_session(state,request,NULL,hash)) { memcpy(connection->data,hash,sizeof(hash)); mg_ws_upgrade(connection,request,"X-Content-Type-Options: nosniff\r\n"); }
        else send_json(state,connection,401,NULL,"{\"error\":\"Oturum gerekli\"}\n");
        mg_bzero(hash,sizeof(hash));
    } else { char headers[1400]; build_headers(state,"text/plain; charset=utf-8",NULL,headers,sizeof(headers)); mg_http_reply(connection,404,headers,"404 - Sayfa bulunamadi\n"); }
}

static void event_handler(struct mg_connection *connection,
                          int event,
                          void *event_data)
{
    login_server_state_t *state = connection->fn_data;

    if (event == MG_EV_ACCEPT && connection->is_tls) {
        struct mg_tls_opts options = {0};
        options.cert = state->certificate;
        options.key = state->private_key;
        mg_tls_init(connection, &options);
    } else if (event == MG_EV_HTTP_MSG) {
        handle_http(state, connection,
                    (struct mg_http_message *)event_data);
    } else if (event == MG_EV_WS_OPEN) {
        int number = assign_client(state, connection->id);
        char control[64];
        if (number == 0) {
            send_websocket_text(connection, "@status:Sunucu dolu");
            mg_ws_send(connection, "", 0, WEBSOCKET_OP_CLOSE);
            connection->is_draining = 1;
        } else {
            snprintf(control, sizeof(control), "@client:Client%d", number);
            send_websocket_text(connection, control);
        }
    } else if (event == MG_EV_WS_MSG) {
        handle_websocket_message(state, connection,
                                 (const struct mg_ws_message *)event_data);
    } else if (event == MG_EV_CLOSE && connection->is_websocket) {
        release_client(state, connection->id);
    } else if (event == MG_EV_ERROR) {
        fprintf(stderr, "[LOGIN-WEB] Baglanti hatasi: %s\n",
                (const char *)event_data);
    }
}

static void install_signal_handlers(void)
{
#ifdef _WIN32
    signal(SIGINT, handle_signal);
#ifdef SIGTERM
    signal(SIGTERM, handle_signal);
#endif
#else
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = handle_signal;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
#endif
}

int run_login_web_server(int port,
                         const char *index_path,
                         const char *users_path,
                         const char *certificate_path,
                         const char *private_key_path,
                         int enable_hsts)
{
    struct mg_mgr manager;
    struct mg_connection *listener;
    login_server_state_t state;
    char url[64];
    int written;
    int result = 1;
    size_t index_size = 0;

    memset(&state, 0, sizeof(state));
    state.index_path = index_path;
    if (!dirname_from_path(index_path, state.base_dir, sizeof(state.base_dir)) ||
        !path_join(state.login_path,sizeof(state.login_path),state.base_dir,"login.html") ||
        !path_join(state.dashboard_path,sizeof(state.dashboard_path),state.base_dir,"dashboard.html") ||
        !path_join(state.chat_path,sizeof(state.chat_path),state.base_dir,"chat.html") ||
        !path_join(state.saw_path,sizeof(state.saw_path),state.base_dir,"saw.html") ||
        !path_join(state.log_page_path,sizeof(state.log_page_path),state.base_dir,"log.html") ||
        !path_join(state.status_path,sizeof(state.status_path),state.base_dir,"status.html") ||
        !path_join(state.log_path,sizeof(state.log_path),state.base_dir,"server.log") ||
        !path_join(state.zip_path,sizeof(state.zip_path),state.base_dir,"server_log.zip")) {
        fprintf(stderr,"Dosya yollari olusturulamadi.\n"); goto cleanup;
    }
    state.tls_enabled = certificate_path != NULL && private_key_path != NULL;
    state.hsts_enabled = enable_hsts;
    login_sessions_init(&state.sessions);

    {
        const char *pages[] = {state.login_path,state.dashboard_path,state.chat_path,state.saw_path,state.log_page_path,state.status_path};
        size_t i;
        for(i=0;i<sizeof(pages)/sizeof(pages[0]);++i){
            index_size=0;
            if ((mg_fs_posix.st(pages[i], &index_size, NULL) & MG_FS_READ) == 0 || index_size == 0 || index_size > 2UL*1024UL*1024UL) {
                fprintf(stderr,"HTML dosyasi okunamadi: %s\n",pages[i]); goto cleanup;
            }
        }
    }
    state.log_file = fopen(state.log_path, "a");
    if (!state.log_file) { fprintf(stderr,"Log dosyasi acilamadi: %s\n",state.log_path); goto cleanup; }
    { char ts[64]; format_timestamp(ts,sizeof(ts)); log_line(&state,"Uygulama baslatildi: %s",ts); }
    if (!login_auth_load(&state.auth, users_path)) {
        goto cleanup;
    }
    if (state.tls_enabled) {
        state.certificate = mg_file_read(&mg_fs_posix, certificate_path);
        state.private_key = mg_file_read(&mg_fs_posix, private_key_path);
        if (state.certificate.len == 0 || state.private_key.len == 0) {
            fprintf(stderr, "TLS sertifikasi veya anahtari okunamadi.\n");
            goto cleanup;
        }
    }
    written = snprintf(url, sizeof(url), "%s://0.0.0.0:%d",
                       state.tls_enabled ? "https" : "http", port);
    if (written < 0 || (size_t)written >= sizeof(url)) {
        fprintf(stderr, "Web sunucu adresi olusturulamadi.\n");
        goto cleanup;
    }

    login_server_running = 1;
    install_signal_handlers();
    mg_log_set(MG_LL_ERROR);
    mg_mgr_init(&manager);
    listener = mg_http_listen(&manager, url, event_handler, &state);
    if (listener == NULL) {
        fprintf(stderr, "Login web sunucusu %s adresinde baslatilamadi.\n",
                url);
        mg_mgr_free(&manager);
        goto cleanup;
    }
    printf("SAW login web sunucusu: %s\n", url);
    printf("Dosyalar: %s, %s\n", index_path, users_path);
    printf("Durdurmak icin Ctrl+C.\n");
    while (login_server_running) {
        mg_mgr_poll(&manager, 100);
    }
    mg_mgr_free(&manager);
    printf("SAW login web sunucusu kapatildi.\n");
    result = 0;

cleanup:
    if (state.log_file != NULL) fclose(state.log_file);
    login_sessions_clear(&state.sessions);
    login_auth_clear(&state.auth);
    mg_free(state.certificate.buf);
    mg_free(state.private_key.buf);
    return result;
}
