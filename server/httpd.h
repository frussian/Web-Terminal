#ifndef _HTTPD_H___
#define _HTTPD_H___

#include <stdio.h>
#include <string.h>

// Client request
extern char *method, // "GET" or "POST"
        *uri,            // "/index.html" things before '?'
        *qs,             // "a=1&b=2" things after  '?'
        *prot,           // "HTTP/1.1"
        *payload;        // for POST

extern int payload_size;

// Server control functions
void serve_forever(const char *PORT);

char *request_header(const char *name);

char headerIsPresent(const char *name);

int writeHeader(const char *k, const char *v);

int writeConn(const char *data, ...);

typedef struct {
    char *name, *value;
} header_t;
static header_t reqhdr[17] = {{"\0", "\0"}};
header_t *request_headers(void);

// user shall implement this function

int route();

// Response
#define RESPONSE_PROTOCOL "HTTP/1.1"

void httpCode(int code);

// some interesting macro for `route()`
#define ROUTE_START() if (0) {
#define ROUTE(METHOD, URI)                                                     \
  }                                                                            \
  else if (strcmp(URI, uri) == 0 && strcmp(METHOD, method) == 0) {
#define ROUTE_END()                                                            \
  }                                                                            \
  else httpCode(500);
#define GET(URI) ROUTE("GET", URI)
#define POST(URI) ROUTE("POST", URI)
#define OPTIONS(URI) ROUTE("OPTIONS", URI)

#define CLOSE_CONN -1
#define KEEP_CONN 1
#endif