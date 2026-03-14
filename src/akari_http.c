#include "akari_core.h"
#include "akari_internal.h"
#include "picohttpparser.h"
#include "jsmn.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include "akari_http.h"

typedef struct {
    const char* method;
    const char* path;
    akari_route_handler handler;
} akari_route;

static akari_route route_table[AKARI_MAX_ROUTES];
static int route_count = 0;

typedef struct {
    uint32_t ip;
    uint32_t tokens;
    uint64_t last_refill_ms;
} akari_rate_entry;

static akari_rate_entry rate_table[AKARI_RATE_BUCKETS];

static uint64_t akari_time_ms(void) {
#if defined(__linux__) || defined(__APPLE__)
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return ((uint64_t)ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
#elif defined(ESP_PLATFORM)
    return (uint64_t)(esp_timer_get_time() / 1000ULL);
#else
    return 0;
#endif
}

static int rate_limit_check(uint32_t ip) {
    uint64_t now = akari_time_ms();
    if (now == 0) return 1;

    uint32_t hash = ip % AKARI_RATE_BUCKETS;
    akari_rate_entry* e = &rate_table[hash];

    if (e->ip != ip) {
        e->ip = ip;
        e->tokens = AKARI_RATE_BURST;
        e->last_refill_ms = now;
    }

    uint64_t elapsed = now - e->last_refill_ms;
    if (elapsed > 0) {
        uint32_t refill = (uint32_t)((elapsed * AKARI_RATE_REFILL_PER_SEC) / 1000);
        if (refill > 0) {
            e->tokens += refill;
            if (e->tokens > AKARI_RATE_BURST)
                e->tokens = AKARI_RATE_BURST;
            e->last_refill_ms = now;
        }
    }

    if (e->tokens == 0) return 0;
    e->tokens--;
    return 1;
}

static const char* status_text(int code) {
    switch (code) {
    case 200: return "OK";
    case 400: return "Bad Request";
    case 404: return "Not Found";
    case 408: return "Request Timeout";
    case 413: return "Content Too Large";
    case 414: return "URI Too Long";
    case 429: return "Too Many Requests";
    case 431: return "Request Header Fields Too Large";
    case 501: return "Not Implemented";
    default:  return "OK";
    }
}

static void send_response_raw(int fd, int status, const char* content_type,
                              const void* body, size_t body_len, int keep_alive) {
    char buf[512];
    int n = snprintf(buf, sizeof(buf),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: %s\r\n"
        "\r\n",
        status, status_text(status),
        content_type, body_len,
        keep_alive ? "keep-alive" : "close");
    if (n <= 0) return;
    if ((size_t)n >= sizeof(buf)) n = (int)sizeof(buf) - 1;
    akari_tcp_send(fd, buf, n);
    if (body && body_len > 0)
        akari_tcp_send(fd, body, body_len);
}

static void send_headers(akari_context* ctx, int status_code,
                         const char* content_type, size_t len) {
    send_response_raw(ctx->_conn->fd, status_code, content_type,
                      NULL, len, ctx->keep_alive);
}

void akari_send_error(int fd, int status, int keep_alive) {
    char body[64];
    int n = snprintf(body, sizeof(body), "%d %s", status, status_text(status));
    if (n < 0) n = 0;
    if ((size_t)n >= sizeof(body)) n = (int)sizeof(body) - 1;
    send_response_raw(fd, status, "text/plain", body, n, keep_alive);
}

void akari_res_send(akari_context* ctx, int status_code,
                    const char* content_type, const char* body) {
    akari_res_data(ctx, status_code, content_type, body, strlen(body));
}

void akari_res_data(akari_context* ctx, int status_code,
                    const char* content_type, const void* data, size_t len) {
    send_headers(ctx, status_code, content_type, len);
    akari_tcp_send(ctx->_conn->fd, data, len);

    if (!ctx->keep_alive) {
        akari_release_conn(ctx->_conn->fd);
        close(ctx->_conn->fd);
    }
}

static const char* get_mime_type(const char* filepath) {
    const char* ext = strrchr(filepath, '.');
    if (!ext) return "application/octet-stream";

    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    if (strcmp(ext, ".json") == 0) return "application/json";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".txt") == 0) return "text/plain";

    return "application/octet-stream";
}

void akari_res_file(akari_context* ctx, const char* filepath) {
    FILE* f = fopen(filepath, "rb");
    if (!f) {
        akari_res_send(ctx, 404, "text/plain", "404 File Not Found");
        return;
    }

    fseek(f, 0, SEEK_END);
    size_t size = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);

    send_headers(ctx, 200, get_mime_type(filepath), size);

    char buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        akari_tcp_send(ctx->_conn->fd, buffer, bytes_read);
    }

    if (!ctx->keep_alive) {
        akari_release_conn(ctx->_conn->fd);
        close(ctx->_conn->fd);
    }

    fclose(f);
}

void akari_http_add_route(const char* method, const char* path,
                          akari_route_handler handler) {
    if (route_count >= AKARI_MAX_ROUTES) {
        AKARI_LOG("too many routes, consider changing the AKARI_MAX_ROUTES macro");
        return;
    }
    route_table[route_count].method = method;
    route_table[route_count].path = path;
    route_table[route_count].handler = handler;
    route_count++;
}

static int match_route(akari_context* ctx, const char* route_path) {
    const char* req_p = ctx->path;
    const char* req_end = ctx->path + ctx->path_len;

    for (const char* p = req_p; p < req_end; p++) {
        if (*p == '?') {
            req_end = p;
            break;
        }
    }

    const char* rt_p = route_path;
    ctx->num_path_params = 0;

    while (req_p < req_end && *rt_p != '\0') {
        if (*rt_p == ':') {
            rt_p++;
            const char* key_start = rt_p;
            while (*rt_p != '/' && *rt_p != '\0') rt_p++;
            size_t key_len = rt_p - key_start;

            const char* val_start = req_p;
            while (req_p < req_end && *req_p != '/') req_p++;
            size_t val_len = req_p - val_start;

            if (ctx->num_path_params < AKARI_MAX_PATH_PARAMS) {
                ctx->path_params[ctx->num_path_params].key = key_start;
                ctx->path_params[ctx->num_path_params].key_len = key_len;
                ctx->path_params[ctx->num_path_params].val = val_start;
                ctx->path_params[ctx->num_path_params].val_len = val_len;
                ctx->num_path_params++;
            }
        } else if (*req_p == *rt_p) {
            req_p++;
            rt_p++;
        } else {
            return 0;
        }
    }

    if (req_p < req_end && *req_p == '/') req_p++;
    if (*rt_p == '/') rt_p++;

    return (req_p == req_end && *rt_p == '\0');
}

static int parse_content_length(struct phr_header* headers, size_t num_headers,
                                size_t* out_len) {
    int found = 0;
    *out_len = 0;

    for (size_t i = 0; i < num_headers; i++) {
        if (headers[i].name_len == 14 &&
            strncasecmp(headers[i].name, "content-length", 14) == 0) {

            size_t val = 0;
            for (size_t j = 0; j < headers[i].value_len; j++) {
                char c = headers[i].value[j];
                if (c < '0' || c > '9') return -1;
                size_t next = val * 10 + (c - '0');
                if (next < val) return -1;
                val = next;
            }

            if (found && val != *out_len) return -1;
            *out_len = val;
            found = 1;
        }
    }

    return found;
}

static int has_transfer_encoding(struct phr_header* headers, size_t num_headers) {
    for (size_t i = 0; i < num_headers; i++) {
        if (headers[i].name_len == 17 &&
            strncasecmp(headers[i].name, "transfer-encoding", 17) == 0)
            return 1;
    }
    return 0;
}

static int detect_keep_alive(int minor_version, struct phr_header* headers,
                             size_t num_headers) {
    int default_ka = (minor_version >= 1) ? 1 : 0;

    for (size_t i = 0; i < num_headers; i++) {
        if (headers[i].name_len == 10 &&
            strncasecmp(headers[i].name, "connection", 10) == 0) {
            if (headers[i].value_len == 5 &&
                strncasecmp(headers[i].value, "close", 5) == 0)
                return 0;
            if (headers[i].value_len == 10 &&
                strncasecmp(headers[i].value, "keep-alive", 10) == 0)
                return 1;
        }
    }
    return default_ka;
}

void akari_handle_http(akari_connection* conn) {
    const char *method, *path;
    size_t method_len, path_len;
    int minor_version;
    struct phr_header headers[AKARI_MAX_HEADERS];
    size_t num_headers = AKARI_MAX_HEADERS;

    int pret = phr_parse_request(conn->buf, conn->buf_len,
                                 &method, &method_len,
                                 &path, &path_len,
                                 &minor_version,
                                 headers, &num_headers,
                                 conn->parsed_header_len);

    if (pret == -2) {
        conn->parsed_header_len = conn->buf_len;
        conn->state = AKARI_CONN_READING_HEADERS;
        if (conn->buf_len > AKARI_MAX_HEADER_BYTES) {
            akari_send_error(conn->fd, 431, 0);
            akari_release_conn(conn->fd);
            close(conn->fd);
        }
        return;
    }

    if (pret == -1) {
        akari_send_error(conn->fd, 400, 0);
        akari_release_conn(conn->fd);
        close(conn->fd);
        return;
    }

    if ((size_t)pret > AKARI_MAX_HEADER_BYTES) {
        akari_send_error(conn->fd, 431, 0);
        akari_release_conn(conn->fd);
        close(conn->fd);
        return;
    }

    if (path_len > AKARI_MAX_PATH_LEN) {
        akari_send_error(conn->fd, 414, 0);
        akari_release_conn(conn->fd);
        close(conn->fd);
        return;
    }

    if (method_len > AKARI_MAX_METHOD_LEN) {
        akari_send_error(conn->fd, 400, 0);
        akari_release_conn(conn->fd);
        close(conn->fd);
        return;
    }

    if (has_transfer_encoding(headers, num_headers)) {
        akari_send_error(conn->fd, 501, 0);
        akari_release_conn(conn->fd);
        close(conn->fd);
        return;
    }

    size_t expected_body = 0;
    int cl_result = parse_content_length(headers, num_headers, &expected_body);

    if (cl_result == -1) {
        akari_send_error(conn->fd, 400, 0);
        akari_release_conn(conn->fd);
        close(conn->fd);
        return;
    }

    if (expected_body > AKARI_MAX_BODY_BYTES) {
        akari_send_error(conn->fd, 413, 0);
        akari_release_conn(conn->fd);
        close(conn->fd);
        return;
    }

    if ((size_t)pret + expected_body > AKARI_REQ_BUF_SIZE) {
        akari_send_error(conn->fd, 413, 0);
        akari_release_conn(conn->fd);
        close(conn->fd);
        return;
    }

    size_t body_received = conn->buf_len - pret;
    if (body_received < expected_body) {
        conn->state = AKARI_CONN_READING_BODY;
        conn->parsed_header_len = pret;
        conn->expected_body_len = expected_body;
        return;
    }

    if (!rate_limit_check(conn->client_ip.s_addr)) {
        int ka = detect_keep_alive(minor_version, headers, num_headers);
        akari_send_error(conn->fd, 429, ka);
        if (!ka) {
            akari_release_conn(conn->fd);
            close(conn->fd);
        } else {
            size_t consumed = pret + expected_body;
            size_t leftover = conn->buf_len - consumed;
            if (leftover > 0)
                memmove(conn->buf, conn->buf + consumed, leftover);
            conn->buf_len = leftover;
            conn->parsed_header_len = 0;
            conn->expected_body_len = 0;
            conn->state = AKARI_CONN_IDLE;
        }
        return;
    }

    int keep_alive = detect_keep_alive(minor_version, headers, num_headers);

    akari_context ctx;
    ctx.method = method;
    ctx.method_len = method_len;
    ctx.path = path;
    ctx.path_len = path_len;
    ctx.headers = headers;
    ctx.num_headers = num_headers;
    ctx.body = conn->buf + pret;
    ctx.body_len = expected_body;
    ctx.num_path_params = 0;
    ctx.res_len = 0;
    ctx.keep_alive = keep_alive;
    ctx._conn = conn;

    conn->state = AKARI_CONN_DISPATCH;

    for (int i = 0; i < route_count; i++) {
        if (ctx.method_len != strlen(route_table[i].method) ||
            strncmp(ctx.method, route_table[i].method, ctx.method_len) != 0)
            continue;

        if (match_route(&ctx, route_table[i].path)) {
            route_table[i].handler(&ctx);
            goto done;
        }
    }

    akari_res_send(&ctx, 404, "text/plain", "404 Route Not Found");

done:
    if (ctx.keep_alive) {
        size_t consumed = pret + expected_body;
        size_t leftover = conn->buf_len - consumed;
        if (leftover > 0)
            memmove(conn->buf, conn->buf + consumed, leftover);
        conn->buf_len = leftover;
        conn->parsed_header_len = 0;
        conn->expected_body_len = 0;
        conn->state = AKARI_CONN_IDLE;
    }
}

void akari_printf(akari_context* ctx, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    size_t avail = sizeof(ctx->res_buf) - ctx->res_len;
    if (avail == 0) {
        va_end(args);
        return;
    }
    int n = vsnprintf(ctx->res_buf + ctx->res_len, avail, fmt, args);
    va_end(args);
    if (n > 0) {
        if ((size_t)n > avail - 1)
            n = (int)(avail - 1);
        ctx->res_len += n;
    }
}

void akari_send(akari_context* ctx, int status, const char* content_type) {
    send_headers(ctx, status, content_type, ctx->res_len);
    akari_tcp_send(ctx->_conn->fd, ctx->res_buf, ctx->res_len);

    if (!ctx->keep_alive) {
        akari_release_conn(ctx->_conn->fd);
        close(ctx->_conn->fd);
    }
}

const char* akari_get_path_param(akari_context* ctx, const char* key, size_t* out_len) {
    size_t target_len = strlen(key);
    for (int i = 0; i < ctx->num_path_params; i++) {
        if (ctx->path_params[i].key_len == target_len &&
            strncmp(ctx->path_params[i].key, key, target_len) == 0) {
            *out_len = ctx->path_params[i].val_len;
            return ctx->path_params[i].val;
        }
    }
    return NULL;
}

const char* akari_get_query_param(akari_context* ctx, const char* key, size_t* out_len) {
    const char* q_start = NULL;
    for (size_t i = 0; i < ctx->path_len; i++) {
        if (ctx->path[i] == '?') {
            q_start = ctx->path + i + 1;
            break;
        }
    }
    if (!q_start) return NULL;

    const char* q_end = ctx->path + ctx->path_len;
    size_t target_len = strlen(key);

    const char* current = q_start;
    while (current < q_end) {
        const char* eq = current;
        while (eq < q_end && *eq != '=' && *eq != '&') eq++;
        size_t key_len = eq - current;
        if (key_len == target_len && strncmp(current, key, target_len) == 0) {
            if (*eq == '=') {
                const char* val_start = eq + 1;
                const char* val_end = val_start;
                while (val_end < q_end && *val_end != '&') val_end++;
                *out_len = val_end - val_start;
                return val_start;
            } else {
                *out_len = 0;
                return eq;
            }
        }
        current = eq;
        while (current < q_end && *current != '&') current++;
        if (current < q_end && *current == '&') current++;
    }
    return NULL;
}

const char* akari_query_str(akari_context* ctx, const char* key, const char* def) {
    size_t len = 0;
    const char* val = akari_get_query_param(ctx, key, &len);
    return val ? val : def;
}

int akari_param_to_int(akari_context* ctx, const char* key) {
    size_t len = 0;
    const char* str_val = akari_get_path_param(ctx, key, &len);
    if (!str_val) str_val = akari_get_query_param(ctx, key, &len);
    if (!str_val || len == 0 || len > 15) return 0;
    char buf[16];
    memcpy(buf, str_val, len);
    buf[len] = '\0';
    return atoi(buf);
}

const char* find_json_token(const char* json, jsmntok_t* tokens, int num_tokens,
                            const char* key, size_t* out_len) {
    size_t key_len = strlen(key);
    for (int i = 1; i < num_tokens - 1; i++) {
        if (tokens[i].type == JSMN_STRING &&
            (size_t)(tokens[i].end - tokens[i].start) == key_len &&
            strncmp(json + tokens[i].start, key, key_len) == 0) {
            *out_len = tokens[i+1].end - tokens[i+1].start;
            return json + tokens[i+1].start;
        }
    }
    return NULL;
}

const char* akari_json_get_string(akari_context* ctx, const char* key, size_t* out_len) {
    if (!ctx->body || ctx->body_len == 0) return NULL;
    jsmn_parser p;
    jsmntok_t t[64];
    jsmn_init(&p);
    int r = jsmn_parse(&p, ctx->body, ctx->body_len, t, sizeof(t) / sizeof(t[0]));
    if (r < 0 || t[0].type != JSMN_OBJECT) return NULL;
    return find_json_token(ctx->body, t, r, key, out_len);
}

int akari_json_get_int(akari_context* ctx, const char* key) {
    size_t len = 0;
    const char* val_str = akari_json_get_string(ctx, key, &len);
    if (!val_str || len == 0 || len > 15) return 0;
    char buf[16];
    memcpy(buf, val_str, len);
    buf[len] = '\0';
    return atoi(buf);
}

int akari_json_get_bool(akari_context* ctx, const char* key) {
    size_t len = 0;
    const char* val_str = akari_json_get_string(ctx, key, &len);
    if (!val_str || len == 0) return 0;
    if (len == 4 && strncmp(val_str, "true", 4) == 0) return 1;
    return 0;
}

static int hex_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

size_t akari_url_decode(char* dest, const char* src, size_t src_len) {
    size_t i, j = 0;
    for (i = 0; i < src_len; i++) {
        if (src[i] == '%' && i + 2 < src_len) {
            dest[j++] = (hex_to_int(src[i+1]) << 4) | hex_to_int(src[i+2]);
            i += 2;
        } else if (src[i] == '+') {
            dest[j++] = ' ';
        } else {
            dest[j++] = src[i];
        }
    }
    return j;
}

void akari_http_start(uint16_t port) {
    akari_run_server(port, akari_handle_http);
}
