#ifndef _HTTPD_H___
#define _HTTPD_H___

#include <stdio.h>
#include <string.h>
#include <tty.h>

// Client request
struct request {
    char *method, // "GET" or "POST"
    *uri,            // "/index.html" things before '?'
    *qs,             // "a=1&b=2" things after  '?'
    *prot,           // "HTTP/1.1"
    *payload;        // for POST
    int payload_size;
    int fd;
    int dump_editor;
};

// Server control functions
void serve_forever(int port, struct tty *pt);

char *request_header(const char *name);

char header_is_present(const char *name);

int write_header(int fd, const char *k, const char *v);

int write_conn(int fd, const char *data, ...);

typedef struct {
    char *name, *value;
} header_t;

header_t *request_headers(void);

// user shall implement this function

int route(struct request);

// Response
#define RESPONSE_PROTOCOL "HTTP/1.1"

void http_code(int fd, int code);

// some interesting macro for `route()`
#define ROUTE_START() if (0) {
#define ROUTE(METHOD, URI)                                                     \
  }                                                                            \
  else if (strcmp(URI, req.uri) == 0 && strcmp(METHOD, req.method) == 0) {
#define ROUTE_END()                                                            \
  }                                                                            \
  else http_code(req.fd, 500);
#define GET(URI) ROUTE("GET", URI)
#define POST(URI) ROUTE("POST", URI)
#define OPTIONS(URI) ROUTE("OPTIONS", URI)

#define CLOSE_CONN 1
#define KEEP_CONN 0
#endif