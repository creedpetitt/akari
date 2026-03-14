// Akari Single-Header Web Server Library
// Generated automatically.

#ifndef AKARI_SINGLE_HEADER_H
#define AKARI_SINGLE_HEADER_H

// --- include/akari_core.h ---
#ifndef AKARI_CORE_H
#define AKARI_CORE_H

#include <stdint.h>
#include <stddef.h>

// PLATFORM DETECTION
#if defined(__linux__) || defined(__APPLE__)
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
#elif defined(AKARI_USE_LWIP) || defined(ESP_PLATFORM)
    #include <sys/types.h>
    #include "lwip/sockets.h"
    #include "lwip/netdb.h"
#else
    typedef int32_t ssize_t;
    struct sockaddr_in {
        uint16_t sin_family;
        uint16_t sin_port;
        struct { uint32_t s_addr; } sin_addr;
    };
    #define AF_INET 2
    #define SOCK_STREAM 1
    #define SOMAXCONN 128
#endif

// --- PORTABILITY GUARDS ---
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#ifndef AKARI_LOG
    #ifdef AKARI_DEBUG
        #include <stdio.h>
        #define AKARI_LOG(fmt, ...) printf("[AKARI] " fmt "\n", ##__VA_ARGS__)
    #else
        #define AKARI_LOG(fmt, ...) ((void)0)
    #endif
#endif

int akari_tcp_init(void);
int akari_tcp_bind(int fd, const struct sockaddr_in* addr);
int akari_tcp_listen(int fd);
int akari_tcp_accept(int fd, struct sockaddr_in* addr);
int akari_tcp_start(uint16_t port);
struct sockaddr_in akari_addr_init(const char* host, uint16_t port);
ssize_t akari_tcp_recv(int fd, void *buf, size_t size);

#endif

// --- include/akari_event.h ---
#ifndef AKARI_EVENT_H
#define AKARI_EVENT_H


#ifndef AKARI_MAX_CONNECTIONS
#define AKARI_MAX_CONNECTIONS 8
#endif

#ifndef AKARI_REQ_BUF_SIZE
#define AKARI_REQ_BUF_SIZE 4096
#endif

#ifndef AKARI_RES_BUF_SIZE
#define AKARI_RES_BUF_SIZE 512
#endif

#ifndef AKARI_HEADER_TIMEOUT_MS
#define AKARI_HEADER_TIMEOUT_MS 5000
#endif

#ifndef AKARI_BODY_TIMEOUT_MS
#define AKARI_BODY_TIMEOUT_MS 10000
#endif

#ifndef AKARI_KEEPALIVE_TIMEOUT_MS
#define AKARI_KEEPALIVE_TIMEOUT_MS 10000
#endif

typedef enum {
    AKARI_CONN_IDLE,
    AKARI_CONN_READING_HEADERS,
    AKARI_CONN_READING_BODY,
    AKARI_CONN_DISPATCH,
    AKARI_CONN_SENDING
} akari_parse_state;

typedef struct {
    int fd;
    char buf[AKARI_REQ_BUF_SIZE];
    size_t buf_len;
    akari_parse_state state;
    uint64_t last_activity_ms;
    size_t parsed_header_len;
    size_t expected_body_len;
    struct in_addr client_ip;
    
    char res_buf[AKARI_RES_BUF_SIZE];
    size_t tx_len;
    size_t tx_sent;
    int tx_file_fd;
    size_t tx_file_len;
    size_t tx_file_sent;
    int tx_keep_alive;
} akari_connection;

typedef void (*akari_callback)(akari_connection* conn);
typedef void (*akari_timer_callback)(void);

akari_connection* akari_get_conn(int fd);
void akari_handle_write(akari_connection* conn);
void akari_run_server(uint16_t port, akari_callback on_data);
void akari_stop(void);
void akari_add_timer(akari_timer_callback cb, int interval_ms);

#endif

// --- include/akari_internal.h ---
#ifndef AKARI_INTERNAL_H
#define AKARI_INTERNAL_H


void akari_run_epoll(int srv_fd, akari_callback on_data);
void akari_run_poll(int srv_fd, akari_callback on_data);

int akari_handle_client(int fd, akari_callback on_data);
void akari_release_conn(int fd);
void akari_check_timers(void);
void akari_sweep_timeouts(void);
void akari_send_error(int fd, int status, int keep_alive);

extern volatile int akari_running;

#endif

// --- include/akari_http.h ---
#ifndef AKARI_HTTP_H
#define AKARI_HTTP_H


#ifndef AKARI_MAX_ROUTES
#define AKARI_MAX_ROUTES 16
#endif

#ifndef AKARI_MAX_PATH_PARAMS
#define AKARI_MAX_PATH_PARAMS 4
#endif

#ifndef AKARI_MAX_HEADERS
#define AKARI_MAX_HEADERS 32
#endif

#ifndef AKARI_MAX_HEADER_BYTES
#define AKARI_MAX_HEADER_BYTES 8192
#endif

#ifndef AKARI_MAX_BODY_BYTES
#define AKARI_MAX_BODY_BYTES (1024 * 1024)
#endif

#ifndef AKARI_MAX_PATH_LEN
#define AKARI_MAX_PATH_LEN 2048
#endif

#ifndef AKARI_MAX_METHOD_LEN
#define AKARI_MAX_METHOD_LEN 8
#endif

#ifndef AKARI_RATE_BUCKETS
#define AKARI_RATE_BUCKETS 64
#endif

#ifndef AKARI_RATE_REFILL_PER_SEC
#define AKARI_RATE_REFILL_PER_SEC 50
#endif

#ifndef AKARI_RATE_BURST
#define AKARI_RATE_BURST 100
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

void akari_res_data(akari_context* ctx, int status_code, 
                    const char* content_type, const void* data, size_t len);

void akari_res_file(akari_context* ctx, const char* filepath);

void akari_http_add_route(const char* method, 
                          const char* path, 
                          akari_route_handler handler);

void akari_http_start(uint16_t port);

const char* akari_get_query_param(akari_context* ctx, const char* key, size_t* out_len);
const char* akari_get_path_param(akari_context* ctx, const char* key, size_t* out_len);

#endif

// --- vendor/picohttpparser/picohttpparser.h ---
/*
 * Copyright (c) 2009-2014 Kazuho Oku, Tokuhiro Matsuno, Daisuke Murase,
 *                         Shigeo Mitsunari
 *
 * The software is licensed under either the MIT License (below) or the Perl
 * license.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef picohttpparser_h
#define picohttpparser_h

#include <stdint.h>
#include <sys/types.h>

#ifdef _MSC_VER
#define ssize_t intptr_t
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* contains name and value of a header (name == NULL if is a continuing line
 * of a multiline header */
struct phr_header {
    const char *name;
    size_t name_len;
    const char *value;
    size_t value_len;
};

/* returns number of bytes consumed if successful, -2 if request is partial,
 * -1 if failed */
int phr_parse_request(const char *buf, size_t len, const char **method, size_t *method_len, const char **path, size_t *path_len,
                      int *minor_version, struct phr_header *headers, size_t *num_headers, size_t last_len);

/* ditto */
int phr_parse_response(const char *_buf, size_t len, int *minor_version, int *status, const char **msg, size_t *msg_len,
                       struct phr_header *headers, size_t *num_headers, size_t last_len);

/* ditto */
int phr_parse_headers(const char *buf, size_t len, struct phr_header *headers, size_t *num_headers, size_t last_len);

/* should be zero-filled before start */
struct phr_chunked_decoder {
    size_t bytes_left_in_chunk; /* number of bytes left in current chunk */
    char consume_trailer;       /* if trailing headers should be consumed */
    char _hex_count;
    char _state;
    uint64_t _total_read;
    uint64_t _total_overhead;
};

/* the function rewrites the buffer given as (buf, bufsz) removing the chunked-
 * encoding headers.  When the function returns without an error, bufsz is
 * updated to the length of the decoded data available.  Applications should
 * repeatedly call the function while it returns -2 (incomplete) every time
 * supplying newly arrived data.  If the end of the chunked-encoded data is
 * found, the function returns a non-negative number indicating the number of
 * octets left undecoded, that starts from the offset returned by `*bufsz`.
 * Returns -1 on error.
 */
ssize_t phr_decode_chunked(struct phr_chunked_decoder *decoder, char *buf, size_t *bufsz);

/* returns if the chunked decoder is in middle of chunked data */
int phr_decode_chunked_is_in_data(struct phr_chunked_decoder *decoder);

#ifdef __cplusplus
}
#endif

#endif

// --- vendor/jsmn/jsmn.h ---
/*
 * MIT License
 *
 * Copyright (c) 2010 Serge Zaitsev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef JSMN_H
#define JSMN_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef JSMN_STATIC
#define JSMN_API static
#else
#define JSMN_API extern
#endif

/**
 * JSON type identifier. Basic types are:
 * 	o Object
 * 	o Array
 * 	o String
 * 	o Other primitive: number, boolean (true/false) or null
 */
typedef enum {
  JSMN_UNDEFINED = 0,
  JSMN_OBJECT = 1 << 0,
  JSMN_ARRAY = 1 << 1,
  JSMN_STRING = 1 << 2,
  JSMN_PRIMITIVE = 1 << 3
} jsmntype_t;

enum jsmnerr {
  /* Not enough tokens were provided */
  JSMN_ERROR_NOMEM = -1,
  /* Invalid character inside JSON string */
  JSMN_ERROR_INVAL = -2,
  /* The string is not a full JSON packet, more bytes expected */
  JSMN_ERROR_PART = -3
};

/**
 * JSON token description.
 * type		type (object, array, string etc.)
 * start	start position in JSON data string
 * end		end position in JSON data string
 */
typedef struct jsmntok {
  jsmntype_t type;
  int start;
  int end;
  int size;
#ifdef JSMN_PARENT_LINKS
  int parent;
#endif
} jsmntok_t;

/**
 * JSON parser. Contains an array of token blocks available. Also stores
 * the string being parsed now and current position in that string.
 */
typedef struct jsmn_parser {
  unsigned int pos;     /* offset in the JSON string */
  unsigned int toknext; /* next token to allocate */
  int toksuper;         /* superior token node, e.g. parent object or array */
} jsmn_parser;

/**
 * Create JSON parser over an array of tokens
 */
JSMN_API void jsmn_init(jsmn_parser *parser);

/**
 * Run JSON parser. It parses a JSON data string into and array of tokens, each
 * describing
 * a single JSON object.
 */
JSMN_API int jsmn_parse(jsmn_parser *parser, const char *js, const size_t len,
                        jsmntok_t *tokens, const unsigned int num_tokens);

#ifndef JSMN_HEADER
/**
 * Allocates a fresh unused token from the token pool.
 */
static jsmntok_t *jsmn_alloc_token(jsmn_parser *parser, jsmntok_t *tokens,
                                   const size_t num_tokens) {
  jsmntok_t *tok;
  if (parser->toknext >= num_tokens) {
    return NULL;
  }
  tok = &tokens[parser->toknext++];
  tok->start = tok->end = -1;
  tok->size = 0;
#ifdef JSMN_PARENT_LINKS
  tok->parent = -1;
#endif
  return tok;
}

/**
 * Fills token type and boundaries.
 */
static void jsmn_fill_token(jsmntok_t *token, const jsmntype_t type,
                            const int start, const int end) {
  token->type = type;
  token->start = start;
  token->end = end;
  token->size = 0;
}

/**
 * Fills next available token with JSON primitive.
 */
static int jsmn_parse_primitive(jsmn_parser *parser, const char *js,
                                const size_t len, jsmntok_t *tokens,
                                const size_t num_tokens) {
  jsmntok_t *token;
  int start;

  start = parser->pos;

  for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
    switch (js[parser->pos]) {
#ifndef JSMN_STRICT
    /* In strict mode primitive must be followed by "," or "}" or "]" */
    case ':':
#endif
    case '\t':
    case '\r':
    case '\n':
    case ' ':
    case ',':
    case ']':
    case '}':
      goto found;
    default:
                   /* to quiet a warning from gcc*/
      break;
    }
    if (js[parser->pos] < 32 || js[parser->pos] >= 127) {
      parser->pos = start;
      return JSMN_ERROR_INVAL;
    }
  }
#ifdef JSMN_STRICT
  /* In strict mode primitive must be followed by a comma/object/array */
  parser->pos = start;
  return JSMN_ERROR_PART;
#endif

found:
  if (tokens == NULL) {
    parser->pos--;
    return 0;
  }
  token = jsmn_alloc_token(parser, tokens, num_tokens);
  if (token == NULL) {
    parser->pos = start;
    return JSMN_ERROR_NOMEM;
  }
  jsmn_fill_token(token, JSMN_PRIMITIVE, start, parser->pos);
#ifdef JSMN_PARENT_LINKS
  token->parent = parser->toksuper;
#endif
  parser->pos--;
  return 0;
}

/**
 * Fills next token with JSON string.
 */
static int jsmn_parse_string(jsmn_parser *parser, const char *js,
                             const size_t len, jsmntok_t *tokens,
                             const size_t num_tokens) {
  jsmntok_t *token;

  int start = parser->pos;
  
  /* Skip starting quote */
  parser->pos++;
  
  for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
    char c = js[parser->pos];

    /* Quote: end of string */
    if (c == '\"') {
      if (tokens == NULL) {
        return 0;
      }
      token = jsmn_alloc_token(parser, tokens, num_tokens);
      if (token == NULL) {
        parser->pos = start;
        return JSMN_ERROR_NOMEM;
      }
      jsmn_fill_token(token, JSMN_STRING, start + 1, parser->pos);
#ifdef JSMN_PARENT_LINKS
      token->parent = parser->toksuper;
#endif
      return 0;
    }

    /* Backslash: Quoted symbol expected */
    if (c == '\\' && parser->pos + 1 < len) {
      int i;
      parser->pos++;
      switch (js[parser->pos]) {
      /* Allowed escaped symbols */
      case '\"':
      case '/':
      case '\\':
      case 'b':
      case 'f':
      case 'r':
      case 'n':
      case 't':
        break;
      /* Allows escaped symbol \uXXXX */
      case 'u':
        parser->pos++;
        for (i = 0; i < 4 && parser->pos < len && js[parser->pos] != '\0';
             i++) {
          /* If it isn't a hex character we have an error */
          if (!((js[parser->pos] >= 48 && js[parser->pos] <= 57) ||   /* 0-9 */
                (js[parser->pos] >= 65 && js[parser->pos] <= 70) ||   /* A-F */
                (js[parser->pos] >= 97 && js[parser->pos] <= 102))) { /* a-f */
            parser->pos = start;
            return JSMN_ERROR_INVAL;
          }
          parser->pos++;
        }
        parser->pos--;
        break;
      /* Unexpected symbol */
      default:
        parser->pos = start;
        return JSMN_ERROR_INVAL;
      }
    }
  }
  parser->pos = start;
  return JSMN_ERROR_PART;
}

/**
 * Parse JSON string and fill tokens.
 */
JSMN_API int jsmn_parse(jsmn_parser *parser, const char *js, const size_t len,
                        jsmntok_t *tokens, const unsigned int num_tokens) {
  int r;
  int i;
  jsmntok_t *token;
  int count = parser->toknext;

  for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
    char c;
    jsmntype_t type;

    c = js[parser->pos];
    switch (c) {
    case '{':
    case '[':
      count++;
      if (tokens == NULL) {
        break;
      }
      token = jsmn_alloc_token(parser, tokens, num_tokens);
      if (token == NULL) {
        return JSMN_ERROR_NOMEM;
      }
      if (parser->toksuper != -1) {
        jsmntok_t *t = &tokens[parser->toksuper];
#ifdef JSMN_STRICT
        /* In strict mode an object or array can't become a key */
        if (t->type == JSMN_OBJECT) {
          return JSMN_ERROR_INVAL;
        }
#endif
        t->size++;
#ifdef JSMN_PARENT_LINKS
        token->parent = parser->toksuper;
#endif
      }
      token->type = (c == '{' ? JSMN_OBJECT : JSMN_ARRAY);
      token->start = parser->pos;
      parser->toksuper = parser->toknext - 1;
      break;
    case '}':
    case ']':
      if (tokens == NULL) {
        break;
      }
      type = (c == '}' ? JSMN_OBJECT : JSMN_ARRAY);
#ifdef JSMN_PARENT_LINKS
      if (parser->toknext < 1) {
        return JSMN_ERROR_INVAL;
      }
      token = &tokens[parser->toknext - 1];
      for (;;) {
        if (token->start != -1 && token->end == -1) {
          if (token->type != type) {
            return JSMN_ERROR_INVAL;
          }
          token->end = parser->pos + 1;
          parser->toksuper = token->parent;
          break;
        }
        if (token->parent == -1) {
          if (token->type != type || parser->toksuper == -1) {
            return JSMN_ERROR_INVAL;
          }
          break;
        }
        token = &tokens[token->parent];
      }
#else
      for (i = parser->toknext - 1; i >= 0; i--) {
        token = &tokens[i];
        if (token->start != -1 && token->end == -1) {
          if (token->type != type) {
            return JSMN_ERROR_INVAL;
          }
          parser->toksuper = -1;
          token->end = parser->pos + 1;
          break;
        }
      }
      /* Error if unmatched closing bracket */
      if (i == -1) {
        return JSMN_ERROR_INVAL;
      }
      for (; i >= 0; i--) {
        token = &tokens[i];
        if (token->start != -1 && token->end == -1) {
          parser->toksuper = i;
          break;
        }
      }
#endif
      break;
    case '\"':
      r = jsmn_parse_string(parser, js, len, tokens, num_tokens);
      if (r < 0) {
        return r;
      }
      count++;
      if (parser->toksuper != -1 && tokens != NULL) {
        tokens[parser->toksuper].size++;
      }
      break;
    case '\t':
    case '\r':
    case '\n':
    case ' ':
      break;
    case ':':
      parser->toksuper = parser->toknext - 1;
      break;
    case ',':
      if (tokens != NULL && parser->toksuper != -1 &&
          tokens[parser->toksuper].type != JSMN_ARRAY &&
          tokens[parser->toksuper].type != JSMN_OBJECT) {
#ifdef JSMN_PARENT_LINKS
        parser->toksuper = tokens[parser->toksuper].parent;
#else
        for (i = parser->toknext - 1; i >= 0; i--) {
          if (tokens[i].type == JSMN_ARRAY || tokens[i].type == JSMN_OBJECT) {
            if (tokens[i].start != -1 && tokens[i].end == -1) {
              parser->toksuper = i;
              break;
            }
          }
        }
#endif
      }
      break;
#ifdef JSMN_STRICT
    /* In strict mode primitives are: numbers and booleans */
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case 't':
    case 'f':
    case 'n':
      /* And they must not be keys of the object */
      if (tokens != NULL && parser->toksuper != -1) {
        const jsmntok_t *t = &tokens[parser->toksuper];
        if (t->type == JSMN_OBJECT ||
            (t->type == JSMN_STRING && t->size != 0)) {
          return JSMN_ERROR_INVAL;
        }
      }
#else
    /* In non-strict mode every unquoted value is a primitive */
    default:
#endif
      r = jsmn_parse_primitive(parser, js, len, tokens, num_tokens);
      if (r < 0) {
        return r;
      }
      count++;
      if (parser->toksuper != -1 && tokens != NULL) {
        tokens[parser->toksuper].size++;
      }
      break;

#ifdef JSMN_STRICT
    /* Unexpected char in strict mode */
    default:
      return JSMN_ERROR_INVAL;
#endif
    }
  }

  if (tokens != NULL) {
    for (i = parser->toknext - 1; i >= 0; i--) {
      /* Unmatched opened object or array */
      if (tokens[i].start != -1 && tokens[i].end == -1) {
        return JSMN_ERROR_PART;
      }
    }
  }

  return count;
}

/**
 * Creates a new parser based over a given buffer with an array of tokens
 * available.
 */
JSMN_API void jsmn_init(jsmn_parser *parser) {
  parser->pos = 0;
  parser->toknext = 0;
  parser->toksuper = -1;
}

#endif /* JSMN_HEADER */

#ifdef __cplusplus
}
#endif

#endif /* JSMN_H */

#endif // AKARI_SINGLE_HEADER_H

#ifdef AKARI_IMPLEMENTATION

// --- src/akari_core.c ---
#include <errno.h>
#include <fcntl.h>

static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) return -1;
    return 0;
}

int akari_tcp_init(void) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        AKARI_LOG("could not open socket");
        return -1;
    }
    
    if (set_nonblocking(fd) == -1) {
        AKARI_LOG("could not set socket to non-blocking");
        close(fd);
        return -1;
    }
    return fd;
}

int akari_tcp_bind(int fd, const struct sockaddr_in* addr) {
    int result = bind(fd, (struct sockaddr*)addr, sizeof *addr);
    if (result == -1) {
        AKARI_LOG("could not bind socket");
    }
    return result;
}

int akari_tcp_listen(int fd) {
    int result = listen(fd, SOMAXCONN);
    if (result == -1) {
        AKARI_LOG("could not listen to socket");
    }
    return result;
}

int akari_tcp_accept(int fd, struct sockaddr_in* addr) {
    for (;;) {
        socklen_t addr_len = sizeof(struct sockaddr_in);
        int client_fd = accept(fd, (struct sockaddr*)addr, addr ? &addr_len : NULL);
        
        if (client_fd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            break;
        }
        if (client_fd == -1 && (errno == EINTR || errno == ECONNABORTED)) {
            continue;
        }
        if (client_fd == -1) {
            AKARI_LOG("accept failed");
            return -1;
        }
        
        if (set_nonblocking(client_fd) == -1) {
            AKARI_LOG("could not set client socket to non-blocking");
            close(client_fd);
            return -1;
        }
        
        return client_fd;
    }
    return -1;
}


ssize_t akari_tcp_recv(int fd, void *buf, size_t size) {
    while (1) {
        ssize_t received = recv(fd, buf, size, 0);
        if (received == -1) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return 0;
            }
            AKARI_LOG("recv failed");
            return -1;
        }
        if (received == 0) {
            return -2;
        }
        return received;
    }
}

struct sockaddr_in akari_addr_init(const char* host, uint16_t port) {
    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (host == NULL) {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
            AKARI_LOG("invalid address");
            addr.sin_family = 0;
        }
    }
    return addr;
}

int akari_tcp_start(uint16_t port) {
    int fd = akari_tcp_init();
    if (fd == -1) {
        return -1;
    }
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        AKARI_LOG("setsockopt SO_REUSEADDR failed");
    }
    struct sockaddr_in addr = akari_addr_init(NULL, port);
    if (addr.sin_family == 0) {
        close(fd);
        return -1;
    }
    if (akari_tcp_bind(fd, &addr) == -1) {
        close(fd);
        return -1;
    }
    if (akari_tcp_listen(fd) == -1) {
        close(fd);
        return -1;
    }
    return fd;
}
// --- src/akari_event.c ---
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#if defined(__linux__) || defined(__APPLE__)
    #include <sys/time.h>
#elif defined(ESP_PLATFORM)
    #include "esp_timer.h"
#endif

static uint64_t get_time_ms(void) {
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

#define AKARI_MAX_TIMERS 4

volatile int akari_running = 1;

typedef struct {
    akari_timer_callback cb;
    uint64_t interval_ms;
    uint64_t last_run_ms;
} akari_timer;

static akari_timer timer_pool[AKARI_MAX_TIMERS];
static int timer_count = 0;

static akari_connection conn_pool[AKARI_MAX_CONNECTIONS];
static int conn_pool_initialized = 0;

void akari_add_timer(akari_timer_callback cb, int interval_ms) {
    if (timer_count < AKARI_MAX_TIMERS) {
        timer_pool[timer_count].cb = cb;
        timer_pool[timer_count].interval_ms = interval_ms;
        timer_pool[timer_count].last_run_ms = 0;
        timer_count++;
    }
}

void akari_check_timers(void) {
    if (timer_count == 0) return;
    uint64_t now = get_time_ms();
    for (int i = 0; i < timer_count; i++) {
        if (timer_pool[i].last_run_ms == 0 ||
            (now - timer_pool[i].last_run_ms) >= timer_pool[i].interval_ms) {
            timer_pool[i].cb();
            timer_pool[i].last_run_ms = now;
        }
    }
}

void akari_stop(void) {
    akari_running = 0;
}

static void init_conn_pool(void) {
    if (conn_pool_initialized) return;
    memset(conn_pool, 0, sizeof(conn_pool));
    for (int i = 0; i < AKARI_MAX_CONNECTIONS; i++) {
        conn_pool[i].fd = -1;
        conn_pool[i].state = AKARI_CONN_IDLE;
    }
    conn_pool_initialized = 1;
}

akari_connection* akari_get_conn(int fd) {
    init_conn_pool();
    int first_empty = -1;

    for (int i = 0; i < AKARI_MAX_CONNECTIONS; i++) {
        if (conn_pool[i].fd == fd)
            return &conn_pool[i];
        if (first_empty == -1 && conn_pool[i].fd == -1)
            first_empty = i;
    }

    if (first_empty != -1) {
        akari_connection* c = &conn_pool[first_empty];
        c->fd = fd;
        c->buf_len = 0;
        c->state = AKARI_CONN_IDLE;
        c->last_activity_ms = get_time_ms();
        c->parsed_header_len = 0;
        c->expected_body_len = 0;
        memset(&c->client_ip, 0, sizeof(c->client_ip));
        c->tx_len = 0;
        c->tx_sent = 0;
        c->tx_file_fd = -1;
        c->tx_file_len = 0;
        c->tx_file_sent = 0;
        c->tx_keep_alive = 0;
        return c;
    }

    return NULL;
}

void akari_release_conn(int fd) {
    for (int i = 0; i < AKARI_MAX_CONNECTIONS; i++) {
        if (conn_pool[i].fd == fd) {
            if (conn_pool[i].tx_file_fd != -1) {
                close(conn_pool[i].tx_file_fd);
                conn_pool[i].tx_file_fd = -1;
            }
            conn_pool[i].fd = -1;
            conn_pool[i].buf_len = 0;
            conn_pool[i].state = AKARI_CONN_IDLE;
            return;
        }
    }
}

void akari_sweep_timeouts(void) {
    uint64_t now = get_time_ms();
    if (now == 0) return;

    for (int i = 0; i < AKARI_MAX_CONNECTIONS; i++) {
        akari_connection* c = &conn_pool[i];
        if (c->fd < 0) continue;
        if (c->last_activity_ms == 0) continue;

        uint64_t elapsed = now - c->last_activity_ms;
        int timed_out = 0;

        switch (c->state) {
        case AKARI_CONN_READING_HEADERS:
            if (elapsed >= AKARI_HEADER_TIMEOUT_MS) timed_out = 1;
            break;
        case AKARI_CONN_READING_BODY:
            if (elapsed >= AKARI_BODY_TIMEOUT_MS) timed_out = 1;
            break;
        case AKARI_CONN_IDLE:
            if (elapsed >= AKARI_KEEPALIVE_TIMEOUT_MS) timed_out = 1;
            break;
        default:
            break;
        }

        if (timed_out) {
            AKARI_LOG("timeout on fd %d (state=%d, elapsed=%llu ms)",
                      c->fd, c->state, (unsigned long long)elapsed);
            akari_send_error(c->fd, 408, 0);
            int fd = c->fd;
            akari_release_conn(fd);
            close(fd);
        }
    }
}

int akari_handle_client(int fd, akari_callback on_data) {
    akari_connection* conn = akari_get_conn(fd);
    if (!conn) return -1;

    size_t space_left = sizeof(conn->buf) - conn->buf_len;
    if (space_left == 0) {
        if (conn->state == AKARI_CONN_READING_HEADERS)
            akari_send_error(fd, 431, 0);
        else if (conn->state == AKARI_CONN_READING_BODY)
            akari_send_error(fd, 413, 0);
        akari_release_conn(fd);
        return -1;
    }

    ssize_t n = akari_tcp_recv(fd, &conn->buf[conn->buf_len], space_left);

    if (n > 0) {
        conn->buf_len += n;
        conn->last_activity_ms = get_time_ms();
        if (conn->state == AKARI_CONN_IDLE)
            conn->state = AKARI_CONN_READING_HEADERS;
        on_data(conn);
        return 0;
    } else if (n == -2 || n == -1) {
        akari_release_conn(fd);
        return -1;
    }
    return 0;
}

static void close_tx_file(akari_connection* conn) {
    if (conn->tx_file_fd != -1) {
        close(conn->tx_file_fd);
        conn->tx_file_fd = -1;
    }
}

static int send_tx_buffer(akari_connection* conn) {
    if (conn->tx_sent >= conn->tx_len) return 1;
    
    ssize_t sent = send(conn->fd, conn->res_buf + conn->tx_sent, 
                        conn->tx_len - conn->tx_sent, MSG_NOSIGNAL);
    if (sent > 0) {
        conn->tx_sent += sent;
        conn->last_activity_ms = get_time_ms();
    } else if (sent == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) return 0;
        return -1;
    }
    return conn->tx_sent >= conn->tx_len;
}

static int send_tx_file(akari_connection* conn) {
    if (conn->tx_file_fd == -1 || conn->tx_file_sent >= conn->tx_file_len) return 1;
    
    char chunk[1024];
    size_t to_read = sizeof(chunk);
    size_t remaining = conn->tx_file_len - conn->tx_file_sent;
    if (remaining < to_read) to_read = remaining;

    ssize_t bytes_read = read(conn->tx_file_fd, chunk, to_read);
    if (bytes_read > 0) {
        ssize_t sent = send(conn->fd, chunk, bytes_read, MSG_NOSIGNAL);
        if (sent > 0) {
            conn->tx_file_sent += sent;
            conn->last_activity_ms = get_time_ms();
            if (sent < bytes_read) {
                lseek(conn->tx_file_fd, sent - bytes_read, SEEK_CUR);
            }
        } else if (sent == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                lseek(conn->tx_file_fd, -bytes_read, SEEK_CUR);
                return 0;
            }
            return -1;
        }
    } else if (bytes_read < 0) {
        return -1;
    }
    return conn->tx_file_sent >= conn->tx_file_len;
}

void akari_handle_write(akari_connection* conn) {
    if (conn->state != AKARI_CONN_SENDING) return;

    int buf_status = send_tx_buffer(conn);
    if (buf_status == 0) return;
    if (buf_status == -1) goto error;

    int file_status = send_tx_file(conn);
    if (file_status == 0) return;
    if (file_status == -1) goto error;

    close_tx_file(conn);

    if (conn->tx_keep_alive) {
        size_t consumed = conn->parsed_header_len + conn->expected_body_len;
        size_t leftover = conn->buf_len - consumed;
        if (leftover > 0 && consumed > 0)
            memmove(conn->buf, conn->buf + consumed, leftover);
        conn->buf_len = leftover;
        conn->parsed_header_len = 0;
        conn->expected_body_len = 0;
        conn->tx_len = 0;
        conn->tx_sent = 0;
        conn->state = AKARI_CONN_IDLE;
    } else {
        akari_release_conn(conn->fd);
        close(conn->fd);
    }
    return;

error:
    close_tx_file(conn);
    akari_release_conn(conn->fd);
    close(conn->fd);
}

void akari_run_server(uint16_t port, akari_callback on_data) {
    int srv_fd = akari_tcp_start(port);
    if (srv_fd == -1) return;

#ifdef AKARI_USE_POLL
    akari_run_poll(srv_fd, on_data);
#else
    akari_run_epoll(srv_fd, on_data);
#endif
}

// --- src/akari_http.c ---

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

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
    akari_connection* conn = akari_get_conn(fd);
    if (!conn) return;
    
    int n = snprintf(conn->res_buf, sizeof(conn->res_buf),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: %s\r\n"
        "\r\n",
        status, status_text(status),
        content_type, body_len,
        keep_alive ? "keep-alive" : "close");
        
    if (n <= 0) return;
    if ((size_t)n >= sizeof(conn->res_buf)) n = (int)sizeof(conn->res_buf) - 1;
    
    conn->tx_len = (size_t)n;
    
    if (body && body_len > 0) {
        size_t space = sizeof(conn->res_buf) - conn->tx_len;
        if (body_len <= space) {
            memcpy(conn->res_buf + conn->tx_len, body, body_len);
            conn->tx_len += body_len;
        } else {
            memcpy(conn->res_buf + conn->tx_len, body, space);
            conn->tx_len += space;
        }
    }
    
    conn->tx_sent = 0;
    conn->tx_keep_alive = keep_alive;
    conn->state = AKARI_CONN_SENDING;
    akari_handle_write(conn);
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
    send_response_raw(ctx->_conn->fd, status_code, content_type, data, len, ctx->keep_alive);
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
    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        akari_res_send(ctx, 404, "text/plain", "404 File Not Found");
        return;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        akari_res_send(ctx, 500, "text/plain", "500 Internal Server Error");
        return;
    }

    send_headers(ctx, 200, get_mime_type(filepath), st.st_size);
    akari_connection* conn = ctx->_conn;
    conn->tx_file_fd = fd;
    conn->tx_file_len = st.st_size;
    conn->tx_file_sent = 0;
    
    akari_handle_write(conn);
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
    akari_connection* conn = ctx->_conn;
    va_list args;
    va_start(args, fmt);
    size_t avail = sizeof(conn->res_buf) - ctx->res_len - 150;
    if (avail <= 0) {
        va_end(args);
        return;
    }
    int n = vsnprintf(conn->res_buf + ctx->res_len, avail, fmt, args);
    va_end(args);
    if (n > 0) {
        if ((size_t)n > avail - 1)
            n = (int)(avail - 1);
        ctx->res_len += n;
    }
}

void akari_send(akari_context* ctx, int status, const char* content_type) {
    akari_connection* conn = ctx->_conn;
    
    size_t body_len = ctx->res_len;
    char temp_body[AKARI_RES_BUF_SIZE];
    if (body_len > 0) {
        memcpy(temp_body, conn->res_buf, body_len);
    }
    
    send_response_raw(conn->fd, status, content_type, body_len > 0 ? temp_body : NULL, body_len, ctx->keep_alive);
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

// --- vendor/picohttpparser/picohttpparser.c ---
/*
 * Copyright (c) 2009-2014 Kazuho Oku, Tokuhiro Matsuno, Daisuke Murase,
 *                         Shigeo Mitsunari
 *
 * The software is licensed under either the MIT License (below) or the Perl
 * license.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <assert.h>
#include <stddef.h>
#include <string.h>
#ifdef __SSE4_2__
#ifdef _MSC_VER
#include <nmmintrin.h>
#else
#include <x86intrin.h>
#endif
#endif

#if __GNUC__ >= 3
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

#ifdef _MSC_VER
#define ALIGNED(n) _declspec(align(n))
#else
#define ALIGNED(n) __attribute__((aligned(n)))
#endif

#define IS_PRINTABLE_ASCII(c) ((unsigned char)(c)-040u < 0137u)

#define CHECK_EOF()                                                                                                                \
    if (buf == buf_end) {                                                                                                          \
        *ret = -2;                                                                                                                 \
        return NULL;                                                                                                               \
    }

#define EXPECT_CHAR_NO_CHECK(ch)                                                                                                   \
    if (*buf++ != ch) {                                                                                                            \
        *ret = -1;                                                                                                                 \
        return NULL;                                                                                                               \
    }

#define EXPECT_CHAR(ch)                                                                                                            \
    CHECK_EOF();                                                                                                                   \
    EXPECT_CHAR_NO_CHECK(ch);

#define ADVANCE_TOKEN(tok, toklen)                                                                                                 \
    do {                                                                                                                           \
        const char *tok_start = buf;                                                                                               \
        static const char ALIGNED(16) ranges2[16] = "\000\040\177\177";                                                            \
        int found2;                                                                                                                \
        buf = findchar_fast(buf, buf_end, ranges2, 4, &found2);                                                                    \
        if (!found2) {                                                                                                             \
            CHECK_EOF();                                                                                                           \
        }                                                                                                                          \
        while (1) {                                                                                                                \
            if (*buf == ' ') {                                                                                                     \
                break;                                                                                                             \
            } else if (unlikely(!IS_PRINTABLE_ASCII(*buf))) {                                                                      \
                if ((unsigned char)*buf < '\040' || *buf == '\177') {                                                              \
                    *ret = -1;                                                                                                     \
                    return NULL;                                                                                                   \
                }                                                                                                                  \
            }                                                                                                                      \
            ++buf;                                                                                                                 \
            CHECK_EOF();                                                                                                           \
        }                                                                                                                          \
        tok = tok_start;                                                                                                           \
        toklen = buf - tok_start;                                                                                                  \
    } while (0)

static const char *token_char_map = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                                    "\0\1\0\1\1\1\1\1\0\0\1\1\0\1\1\0\1\1\1\1\1\1\1\1\1\1\0\0\0\0\0\0"
                                    "\0\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\0\0\1\1"
                                    "\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\1\0\1\0"
                                    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                                    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                                    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                                    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

static const char *findchar_fast(const char *buf, const char *buf_end, const char *ranges, size_t ranges_size, int *found)
{
    *found = 0;
#if __SSE4_2__
    if (likely(buf_end - buf >= 16)) {
        __m128i ranges16 = _mm_loadu_si128((const __m128i *)ranges);

        size_t left = (buf_end - buf) & ~15;
        do {
            __m128i b16 = _mm_loadu_si128((const __m128i *)buf);
            int r = _mm_cmpestri(ranges16, ranges_size, b16, 16, _SIDD_LEAST_SIGNIFICANT | _SIDD_CMP_RANGES | _SIDD_UBYTE_OPS);
            if (unlikely(r != 16)) {
                buf += r;
                *found = 1;
                break;
            }
            buf += 16;
            left -= 16;
        } while (likely(left != 0));
    }
#else
    /* suppress unused parameter warning */
    (void)buf_end;
    (void)ranges;
    (void)ranges_size;
#endif
    return buf;
}

static const char *get_token_to_eol(const char *buf, const char *buf_end, const char **token, size_t *token_len, int *ret)
{
    const char *token_start = buf;

#ifdef __SSE4_2__
    static const char ALIGNED(16) ranges1[16] = "\0\010"    /* allow HT */
                                                "\012\037"  /* allow SP and up to but not including DEL */
                                                "\177\177"; /* allow chars w. MSB set */
    int found;
    buf = findchar_fast(buf, buf_end, ranges1, 6, &found);
    if (found)
        goto FOUND_CTL;
#else
    /* find non-printable char within the next 8 bytes, this is the hottest code; manually inlined */
    while (likely(buf_end - buf >= 8)) {
#define DOIT()                                                                                                                     \
    do {                                                                                                                           \
        if (unlikely(!IS_PRINTABLE_ASCII(*buf)))                                                                                   \
            goto NonPrintable;                                                                                                     \
        ++buf;                                                                                                                     \
    } while (0)
        DOIT();
        DOIT();
        DOIT();
        DOIT();
        DOIT();
        DOIT();
        DOIT();
        DOIT();
#undef DOIT
        continue;
    NonPrintable:
        if ((likely((unsigned char)*buf < '\040') && likely(*buf != '\011')) || unlikely(*buf == '\177')) {
            goto FOUND_CTL;
        }
        ++buf;
    }
#endif
    for (;; ++buf) {
        CHECK_EOF();
        if (unlikely(!IS_PRINTABLE_ASCII(*buf))) {
            if ((likely((unsigned char)*buf < '\040') && likely(*buf != '\011')) || unlikely(*buf == '\177')) {
                goto FOUND_CTL;
            }
        }
    }
FOUND_CTL:
    if (likely(*buf == '\015')) {
        ++buf;
        EXPECT_CHAR('\012');
        *token_len = buf - 2 - token_start;
    } else if (*buf == '\012') {
        *token_len = buf - token_start;
        ++buf;
    } else {
        *ret = -1;
        return NULL;
    }
    *token = token_start;

    return buf;
}

static const char *is_complete(const char *buf, const char *buf_end, size_t last_len, int *ret)
{
    int ret_cnt = 0;
    buf = last_len < 3 ? buf : buf + last_len - 3;

    while (1) {
        CHECK_EOF();
        if (*buf == '\015') {
            ++buf;
            CHECK_EOF();
            EXPECT_CHAR('\012');
            ++ret_cnt;
        } else if (*buf == '\012') {
            ++buf;
            ++ret_cnt;
        } else {
            ++buf;
            ret_cnt = 0;
        }
        if (ret_cnt == 2) {
            return buf;
        }
    }

    *ret = -2;
    return NULL;
}

#define PARSE_INT(valp_, mul_)                                                                                                     \
    if (*buf < '0' || '9' < *buf) {                                                                                                \
        buf++;                                                                                                                     \
        *ret = -1;                                                                                                                 \
        return NULL;                                                                                                               \
    }                                                                                                                              \
    *(valp_) = (mul_) * (*buf++ - '0');

#define PARSE_INT_3(valp_)                                                                                                         \
    do {                                                                                                                           \
        int res_ = 0;                                                                                                              \
        PARSE_INT(&res_, 100)                                                                                                      \
        *valp_ = res_;                                                                                                             \
        PARSE_INT(&res_, 10)                                                                                                       \
        *valp_ += res_;                                                                                                            \
        PARSE_INT(&res_, 1)                                                                                                        \
        *valp_ += res_;                                                                                                            \
    } while (0)

/* returned pointer is always within [buf, buf_end), or null */
static const char *parse_token(const char *buf, const char *buf_end, const char **token, size_t *token_len, char next_char,
                               int *ret)
{
    /* We use pcmpestri to detect non-token characters. This instruction can take no more than eight character ranges (8*2*8=128
     * bits that is the size of a SSE register). Due to this restriction, characters `|` and `~` are handled in the slow loop. */
    static const char ALIGNED(16) ranges[] = "\x00 "  /* control chars and up to SP */
                                             "\"\""   /* 0x22 */
                                             "()"     /* 0x28,0x29 */
                                             ",,"     /* 0x2c */
                                             "//"     /* 0x2f */
                                             ":@"     /* 0x3a-0x40 */
                                             "[]"     /* 0x5b-0x5d */
                                             "{\xff"; /* 0x7b-0xff */
    const char *buf_start = buf;
    int found;
    buf = findchar_fast(buf, buf_end, ranges, sizeof(ranges) - 1, &found);
    if (!found) {
        CHECK_EOF();
    }
    while (1) {
        if (*buf == next_char) {
            break;
        } else if (!token_char_map[(unsigned char)*buf]) {
            *ret = -1;
            return NULL;
        }
        ++buf;
        CHECK_EOF();
    }
    *token = buf_start;
    *token_len = buf - buf_start;
    return buf;
}

/* returned pointer is always within [buf, buf_end), or null */
static const char *parse_http_version(const char *buf, const char *buf_end, int *minor_version, int *ret)
{
    /* we want at least [HTTP/1.<two chars>] to try to parse */
    if (buf_end - buf < 9) {
        *ret = -2;
        return NULL;
    }
    EXPECT_CHAR_NO_CHECK('H');
    EXPECT_CHAR_NO_CHECK('T');
    EXPECT_CHAR_NO_CHECK('T');
    EXPECT_CHAR_NO_CHECK('P');
    EXPECT_CHAR_NO_CHECK('/');
    EXPECT_CHAR_NO_CHECK('1');
    EXPECT_CHAR_NO_CHECK('.');
    PARSE_INT(minor_version, 1);
    return buf;
}

static const char *parse_headers(const char *buf, const char *buf_end, struct phr_header *headers, size_t *num_headers,
                                 size_t max_headers, int *ret)
{
    for (;; ++*num_headers) {
        CHECK_EOF();
        if (*buf == '\015') {
            ++buf;
            EXPECT_CHAR('\012');
            break;
        } else if (*buf == '\012') {
            ++buf;
            break;
        }
        if (*num_headers == max_headers) {
            *ret = -1;
            return NULL;
        }
        if (!(*num_headers != 0 && (*buf == ' ' || *buf == '\t'))) {
            /* parsing name, but do not discard SP before colon, see
             * http://www.mozilla.org/security/announce/2006/mfsa2006-33.html */
            if ((buf = parse_token(buf, buf_end, &headers[*num_headers].name, &headers[*num_headers].name_len, ':', ret)) == NULL) {
                return NULL;
            }
            if (headers[*num_headers].name_len == 0) {
                *ret = -1;
                return NULL;
            }
            ++buf;
            for (;; ++buf) {
                CHECK_EOF();
                if (!(*buf == ' ' || *buf == '\t')) {
                    break;
                }
            }
        } else {
            headers[*num_headers].name = NULL;
            headers[*num_headers].name_len = 0;
        }
        const char *value;
        size_t value_len;
        if ((buf = get_token_to_eol(buf, buf_end, &value, &value_len, ret)) == NULL) {
            return NULL;
        }
        /* remove trailing SPs and HTABs */
        const char *value_end = value + value_len;
        for (; value_end != value; --value_end) {
            const char c = *(value_end - 1);
            if (!(c == ' ' || c == '\t')) {
                break;
            }
        }
        headers[*num_headers].value = value;
        headers[*num_headers].value_len = value_end - value;
    }
    return buf;
}

static const char *parse_request(const char *buf, const char *buf_end, const char **method, size_t *method_len, const char **path,
                                 size_t *path_len, int *minor_version, struct phr_header *headers, size_t *num_headers,
                                 size_t max_headers, int *ret)
{
    /* skip first empty line (some clients add CRLF after POST content) */
    CHECK_EOF();
    if (*buf == '\015') {
        ++buf;
        EXPECT_CHAR('\012');
    } else if (*buf == '\012') {
        ++buf;
    }

    /* parse request line */
    if ((buf = parse_token(buf, buf_end, method, method_len, ' ', ret)) == NULL) {
        return NULL;
    }
    do {
        ++buf;
        CHECK_EOF();
    } while (*buf == ' ');
    ADVANCE_TOKEN(*path, *path_len);
    do {
        ++buf;
        CHECK_EOF();
    } while (*buf == ' ');
    if (*method_len == 0 || *path_len == 0) {
        *ret = -1;
        return NULL;
    }
    if ((buf = parse_http_version(buf, buf_end, minor_version, ret)) == NULL) {
        return NULL;
    }
    if (*buf == '\015') {
        ++buf;
        EXPECT_CHAR('\012');
    } else if (*buf == '\012') {
        ++buf;
    } else {
        *ret = -1;
        return NULL;
    }

    return parse_headers(buf, buf_end, headers, num_headers, max_headers, ret);
}

int phr_parse_request(const char *buf_start, size_t len, const char **method, size_t *method_len, const char **path,
                      size_t *path_len, int *minor_version, struct phr_header *headers, size_t *num_headers, size_t last_len)
{
    const char *buf = buf_start, *buf_end = buf_start + len;
    size_t max_headers = *num_headers;
    int r;

    *method = NULL;
    *method_len = 0;
    *path = NULL;
    *path_len = 0;
    *minor_version = -1;
    *num_headers = 0;

    /* if last_len != 0, check if the request is complete (a fast countermeasure
       againt slowloris */
    if (last_len != 0 && is_complete(buf, buf_end, last_len, &r) == NULL) {
        return r;
    }

    if ((buf = parse_request(buf, buf_end, method, method_len, path, path_len, minor_version, headers, num_headers, max_headers,
                             &r)) == NULL) {
        return r;
    }

    return (int)(buf - buf_start);
}

static const char *parse_response(const char *buf, const char *buf_end, int *minor_version, int *status, const char **msg,
                                  size_t *msg_len, struct phr_header *headers, size_t *num_headers, size_t max_headers, int *ret)
{
    /* parse "HTTP/1.x" */
    if ((buf = parse_http_version(buf, buf_end, minor_version, ret)) == NULL) {
        return NULL;
    }
    /* skip space */
    if (*buf != ' ') {
        *ret = -1;
        return NULL;
    }
    do {
        ++buf;
        CHECK_EOF();
    } while (*buf == ' ');
    /* parse status code, we want at least [:digit:][:digit:][:digit:]<other char> to try to parse */
    if (buf_end - buf < 4) {
        *ret = -2;
        return NULL;
    }
    PARSE_INT_3(status);

    /* get message including preceding space */
    if ((buf = get_token_to_eol(buf, buf_end, msg, msg_len, ret)) == NULL) {
        return NULL;
    }
    if (*msg_len == 0) {
        /* ok */
    } else if (**msg == ' ') {
        /* Remove preceding space. Successful return from `get_token_to_eol` guarantees that we would hit something other than SP
         * before running past the end of the given buffer. */
        do {
            ++*msg;
            --*msg_len;
        } while (**msg == ' ');
    } else {
        /* garbage found after status code */
        *ret = -1;
        return NULL;
    }

    return parse_headers(buf, buf_end, headers, num_headers, max_headers, ret);
}

int phr_parse_response(const char *buf_start, size_t len, int *minor_version, int *status, const char **msg, size_t *msg_len,
                       struct phr_header *headers, size_t *num_headers, size_t last_len)
{
    const char *buf = buf_start, *buf_end = buf + len;
    size_t max_headers = *num_headers;
    int r;

    *minor_version = -1;
    *status = 0;
    *msg = NULL;
    *msg_len = 0;
    *num_headers = 0;

    /* if last_len != 0, check if the response is complete (a fast countermeasure
       against slowloris */
    if (last_len != 0 && is_complete(buf, buf_end, last_len, &r) == NULL) {
        return r;
    }

    if ((buf = parse_response(buf, buf_end, minor_version, status, msg, msg_len, headers, num_headers, max_headers, &r)) == NULL) {
        return r;
    }

    return (int)(buf - buf_start);
}

int phr_parse_headers(const char *buf_start, size_t len, struct phr_header *headers, size_t *num_headers, size_t last_len)
{
    const char *buf = buf_start, *buf_end = buf + len;
    size_t max_headers = *num_headers;
    int r;

    *num_headers = 0;

    /* if last_len != 0, check if the response is complete (a fast countermeasure
       against slowloris */
    if (last_len != 0 && is_complete(buf, buf_end, last_len, &r) == NULL) {
        return r;
    }

    if ((buf = parse_headers(buf, buf_end, headers, num_headers, max_headers, &r)) == NULL) {
        return r;
    }

    return (int)(buf - buf_start);
}

enum {
    CHUNKED_IN_CHUNK_SIZE,
    CHUNKED_IN_CHUNK_EXT,
    CHUNKED_IN_CHUNK_HEADER_EXPECT_LF,
    CHUNKED_IN_CHUNK_DATA,
    CHUNKED_IN_CHUNK_DATA_EXPECT_CR,
    CHUNKED_IN_CHUNK_DATA_EXPECT_LF,
    CHUNKED_IN_TRAILERS_LINE_HEAD,
    CHUNKED_IN_TRAILERS_LINE_MIDDLE
};

static int decode_hex(int ch)
{
    if ('0' <= ch && ch <= '9') {
        return ch - '0';
    } else if ('A' <= ch && ch <= 'F') {
        return ch - 'A' + 0xa;
    } else if ('a' <= ch && ch <= 'f') {
        return ch - 'a' + 0xa;
    } else {
        return -1;
    }
}

ssize_t phr_decode_chunked(struct phr_chunked_decoder *decoder, char *buf, size_t *_bufsz)
{
    size_t dst = 0, src = 0, bufsz = *_bufsz;
    ssize_t ret = -2; /* incomplete */

    decoder->_total_read += bufsz;

    while (1) {
        switch (decoder->_state) {
        case CHUNKED_IN_CHUNK_SIZE:
            for (;; ++src) {
                int v;
                if (src == bufsz)
                    goto Exit;
                if ((v = decode_hex(buf[src])) == -1) {
                    if (decoder->_hex_count == 0) {
                        ret = -1;
                        goto Exit;
                    }
                    /* the only characters that may appear after the chunk size are BWS, semicolon, or CRLF */
                    switch (buf[src]) {
                    case ' ':
                    case '\011':
                    case ';':
                    case '\012':
                    case '\015':
                        break;
                    default:
                        ret = -1;
                        goto Exit;
                    }
                    break;
                }
                if (decoder->_hex_count == sizeof(size_t) * 2) {
                    ret = -1;
                    goto Exit;
                }
                decoder->bytes_left_in_chunk = decoder->bytes_left_in_chunk * 16 + v;
                ++decoder->_hex_count;
            }
            decoder->_hex_count = 0;
            decoder->_state = CHUNKED_IN_CHUNK_EXT;
        /* fallthru */
        case CHUNKED_IN_CHUNK_EXT:
            /* RFC 7230 A.2 "Line folding in chunk extensions is disallowed" */
            for (;; ++src) {
                if (src == bufsz)
                    goto Exit;
                if (buf[src] == '\015') {
                    break;
                } else if (buf[src] == '\012') {
                    ret = -1;
                    goto Exit;
                }
            }
            ++src;
            decoder->_state = CHUNKED_IN_CHUNK_HEADER_EXPECT_LF;
        /* fallthru */
        case CHUNKED_IN_CHUNK_HEADER_EXPECT_LF:
            if (src == bufsz)
                goto Exit;
            if (buf[src] != '\012') {
                ret = -1;
                goto Exit;
            }
            ++src;
            if (decoder->bytes_left_in_chunk == 0) {
                if (decoder->consume_trailer) {
                    decoder->_state = CHUNKED_IN_TRAILERS_LINE_HEAD;
                    break;
                } else {
                    goto Complete;
                }
            }
            decoder->_state = CHUNKED_IN_CHUNK_DATA;
        /* fallthru */
        case CHUNKED_IN_CHUNK_DATA: {
            size_t avail = bufsz - src;
            if (avail < decoder->bytes_left_in_chunk) {
                if (dst != src)
                    memmove(buf + dst, buf + src, avail);
                src += avail;
                dst += avail;
                decoder->bytes_left_in_chunk -= avail;
                goto Exit;
            }
            if (dst != src)
                memmove(buf + dst, buf + src, decoder->bytes_left_in_chunk);
            src += decoder->bytes_left_in_chunk;
            dst += decoder->bytes_left_in_chunk;
            decoder->bytes_left_in_chunk = 0;
            decoder->_state = CHUNKED_IN_CHUNK_DATA_EXPECT_CR;
        }
        /* fallthru */
        case CHUNKED_IN_CHUNK_DATA_EXPECT_CR:
            if (src == bufsz)
                goto Exit;
            if (buf[src] != '\015') {
                ret = -1;
                goto Exit;
            }
            ++src;
            decoder->_state = CHUNKED_IN_CHUNK_DATA_EXPECT_LF;
        /* fallthru */
        case CHUNKED_IN_CHUNK_DATA_EXPECT_LF:
            if (src == bufsz)
                goto Exit;
            if (buf[src] != '\012') {
                ret = -1;
                goto Exit;
            }
            ++src;
            decoder->_state = CHUNKED_IN_CHUNK_SIZE;
            break;
        case CHUNKED_IN_TRAILERS_LINE_HEAD:
            for (;; ++src) {
                if (src == bufsz)
                    goto Exit;
                if (buf[src] != '\015')
                    break;
            }
            if (buf[src++] == '\012')
                goto Complete;
            decoder->_state = CHUNKED_IN_TRAILERS_LINE_MIDDLE;
        /* fallthru */
        case CHUNKED_IN_TRAILERS_LINE_MIDDLE:
            for (;; ++src) {
                if (src == bufsz)
                    goto Exit;
                if (buf[src] == '\012')
                    break;
            }
            ++src;
            decoder->_state = CHUNKED_IN_TRAILERS_LINE_HEAD;
            break;
        default:
            assert(!"decoder is corrupt");
        }
    }

Complete:
    ret = bufsz - src;
Exit:
    if (dst != src)
        memmove(buf + dst, buf + src, bufsz - src);
    *_bufsz = dst;
    /* if incomplete but the overhead of the chunked encoding is >=100KB and >80%, signal an error */
    if (ret == -2) {
        decoder->_total_overhead += bufsz - dst;
        if (decoder->_total_overhead >= 100 * 1024 && decoder->_total_read - decoder->_total_overhead < decoder->_total_read / 4)
            ret = -1;
    }
    return ret;
}

int phr_decode_chunked_is_in_data(struct phr_chunked_decoder *decoder)
{
    return decoder->_state == CHUNKED_IN_CHUNK_DATA;
}

#undef CHECK_EOF
#undef EXPECT_CHAR
#undef ADVANCE_TOKEN


// --- PLATFORM SPECIFIC EVENT ENGINES ---
#ifndef AKARI_USE_POLL
#include <sys/epoll.h>
#include <unistd.h>

void akari_run_epoll(int srv_fd, akari_callback on_data) {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        AKARI_LOG("epoll_create1 failed");
        return;
    }

    struct epoll_event ev, events[AKARI_MAX_CONNECTIONS];

    ev.events = EPOLLIN;
    ev.data.fd = srv_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, srv_fd, &ev) == -1) {
        AKARI_LOG("epoll_ctl server fd failed");
        close(epoll_fd);
        return;
    }

    AKARI_LOG("epoll engine started");

    while (akari_running) {
        int nfds = epoll_wait(epoll_fd, events, AKARI_MAX_CONNECTIONS, 100);
        if (nfds == -1) {
            if (akari_running) AKARI_LOG("epoll_wait failed");
            break;
        }
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == srv_fd) {
                struct sockaddr_in client_addr;
                int client_fd = akari_tcp_accept(srv_fd, &client_addr);
                if (client_fd != -1) {
                    akari_connection* conn = akari_get_conn(client_fd);
                    if (conn) {
                        conn->client_ip = client_addr.sin_addr;
                    }
                    ev.events = EPOLLIN;
                    ev.data.fd = client_fd;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                        AKARI_LOG("epoll_ctl client fd failed");
                        akari_release_conn(client_fd);
                        close(client_fd);
                    }
                }
            } else {
                int client_fd = events[i].data.fd;
                akari_connection* conn = akari_get_conn(client_fd);
                
                if (events[i].events & EPOLLIN) {
                    int status = akari_handle_client(client_fd, on_data);
                    if (status == -1) {
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                        close(client_fd);
                        continue;
                    }
                }
                
                if (conn && (events[i].events & EPOLLOUT)) {
                    akari_handle_write(conn);
                }
                
                // Update epoll mask
                if (conn && conn->fd != -1) {
                    ev.events = EPOLLIN;
                    if (conn->state == AKARI_CONN_SENDING) {
                        ev.events |= EPOLLOUT;
                    }
                    ev.data.fd = client_fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev);
                }
            }
        }
        akari_check_timers();
        akari_sweep_timeouts();
    }

    close(epoll_fd);
}
#endif // AKARI_USE_POLL

#ifdef AKARI_USE_POLL
#include <poll.h>
#include <unistd.h>

void akari_run_poll(int srv_fd, akari_callback on_data) {
    struct pollfd fds[AKARI_MAX_CONNECTIONS + 1];
    int nfds = 1;

    fds[0].fd = srv_fd;
    fds[0].events = POLLIN;

    for (int i = 1; i < AKARI_MAX_CONNECTIONS + 1; i++) {
        fds[i].fd = -1;
    }

    AKARI_LOG("poll engine started");

    while (akari_running) {
        int ready = poll(fds, nfds, 100);
        if (ready == -1) {
            if (akari_running) AKARI_LOG("poll failed");
            break;
        }

        for (int i = 0; i < nfds; i++) {
            if (fds[i].revents == 0) continue;

            if (fds[i].fd == srv_fd) {
                struct sockaddr_in client_addr;
                int client_fd = akari_tcp_accept(srv_fd, &client_addr);
                if (client_fd != -1) {
                    akari_connection* conn = akari_get_conn(client_fd);
                    if (conn) {
                        conn->client_ip = client_addr.sin_addr;
                    }
                    int added = 0;
                    for (int j = 1; j < AKARI_MAX_CONNECTIONS + 1; j++) {
                        if (fds[j].fd == -1) {
                            fds[j].fd = client_fd;
                            fds[j].events = POLLIN;
                            if (j >= nfds) nfds = j + 1;
                            added = 1;
                            break;
                        }
                    }
                    if (!added) {
                        AKARI_LOG("poll fds full");
                        akari_release_conn(client_fd);
                        close(client_fd);
                    }
                }
            } else {
                int client_fd = fds[i].fd;
                akari_connection* conn = akari_get_conn(client_fd);
                
                if (fds[i].revents & POLLIN) {
                    int status = akari_handle_client(client_fd, on_data);
                    if (status == -1 || (fds[i].revents & (POLLHUP | POLLERR))) {
                        close(client_fd);
                        fds[i].fd = -1;
                        continue;
                    }
                }
                
                if (conn && (fds[i].revents & POLLOUT)) {
                    akari_handle_write(conn);
                }
                
                if (conn && conn->fd != -1) {
                    fds[i].events = POLLIN;
                    if (conn->state == AKARI_CONN_SENDING) {
                        fds[i].events |= POLLOUT;
                    }
                }
            }
        }
        akari_check_timers();
        akari_sweep_timeouts();
    }
}
#endif // AKARI_USE_POLL

#endif // AKARI_IMPLEMENTATION
