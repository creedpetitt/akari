#ifndef AKARI_HTTP_H
#define AKARI_HTTP_H

#include "akari_event.h"
#include "picohttpparser.h"

#ifndef AKARI_MAX_ROUTES
#define AKARI_MAX_ROUTES 16
#endif

#ifndef AKARI_MAX_PATH_PARAMS
#define AKARI_MAX_PATH_PARAMS 4
#endif

#ifndef AKARI_RES_BUF_SIZE
#define AKARI_RES_BUF_SIZE 512
#endif

typedef struct {
    const char* key;
    size_t key_len;
    const char* val;
    size_t val_len;
} akari_path_param;

typedef struct {
    const char* method;
    size_t method_len;
    const char* path;
    size_t path_len;

    struct phr_header* headers;
    size_t num_headers;

    const char* body;
    size_t body_len;

    akari_path_param path_params[AKARI_MAX_PATH_PARAMS];
    int num_path_params;

    char res_buf[AKARI_RES_BUF_SIZE];
    size_t res_len;

    int keep_alive;

    akari_connection* _conn;
} akari_context;

typedef void (*akari_route_handler)(akari_context* ctx);

#define AKARI_GET(path, handler)  akari_http_add_route("GET", path, handler)
#define AKARI_POST(path, handler) akari_http_add_route("POST", path, handler)

#define AKARI_GROUP(prefix) prefix
#define akari_res_json(ctx, code, body) akari_res_send(ctx, code, "application/json", body)

void akari_printf(akari_context* ctx, const char* fmt, ...);
void akari_send(akari_context* ctx, int status, const char* content_type);

const char* akari_query_str(akari_context* ctx, const char* key, const char* def);

int akari_param_to_int(akari_context* ctx, const char* key);

const char* akari_json_get_string(akari_context* ctx, const char* key, size_t* out_len);
int akari_json_get_int(akari_context* ctx, const char* key);
int akari_json_get_bool(akari_context* ctx, const char* key);

size_t akari_url_decode(char* dest, const char* src, size_t src_len);

void akari_res_send(akari_context* ctx, int status_code, 
                    const char* content_type, const char* body);

void akari_res_file(akari_context* ctx, const char* filepath);

void akari_http_add_route(const char* method, 
                          const char* path, 
                          akari_route_handler handler);

void akari_http_start(uint16_t port);

const char* akari_get_query_param(akari_context* ctx, const char* key, size_t* out_len);
const char* akari_get_path_param(akari_context* ctx, const char* key, size_t* out_len);

#endif
