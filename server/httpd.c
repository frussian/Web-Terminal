#include "httpd.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdarg.h>

#define MAX_CONNECTIONS 10
#define BUF_SIZE 65535
#define QUEUE_SIZE 1000000

static int listenfd;
int *clients;
static void start_server(const char *);
static void respond(int);

// Client request
char *method, // "GET" or "POST"
        *uri,     // "/index.html" things before '?'
        *qs,      // "a=1&b=2" things after  '?'
        *prot,    // "HTTP/1.1"
        *payload; // for POST

int payload_size;

void serve_forever(const char *PORT) {
    struct sockaddr_in clientaddr;
    socklen_t addrlen;

    int slot = 0;

    printf("Server started %shttp://127.0.0.1:%s%s\n", "\033[92m", PORT,
           "\033[0m");

    // create shared memory for client slot array
    clients = mmap(NULL, sizeof(*clients) * MAX_CONNECTIONS,
                   PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    // Setting all elements to -1: signifies there is no client connected
    int i;
    for (i = 0; i < MAX_CONNECTIONS; i++) clients[i] = -1;
    start_server(PORT);

    // Ignore SIGCHLD to avoid zombie threads
    signal(SIGCHLD, SIG_IGN);

    // ACCEPT connections
    while (1) {
        addrlen = sizeof(clientaddr);
        clients[slot] = accept(listenfd, (struct sockaddr *)&clientaddr, &addrlen);

        if (clients[slot] < 0) {
            perror("accept() error");
            exit(1);
        } else {
            fprintf(stderr, "\x1b[32mnew connection, slot: %d, %s:%d\x1b[0m\n", slot, inet_ntoa(((struct sockaddr_in)clientaddr).sin_addr), clientaddr.sin_port);
            if (fork() == 0) {
                close(listenfd);
                respond(slot);
                close(clients[slot]);
                clients[slot] = -1;
                exit(0);
            } else {
                close(clients[slot]);
            }
        }

        while (clients[slot] != -1)
            slot = (slot + 1) % MAX_CONNECTIONS;
    }
}

// start server
void start_server(const char *port) {
    struct addrinfo hints, *res, *p;

    // getaddrinfo for host
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(NULL, port, &hints, &res) != 0) {
        perror("getaddrinfo() error");
        exit(1);
    }
    // socket and bind
    for (p = res; p != NULL; p = p->ai_next) {
        int option = 1;
        listenfd = socket(p->ai_family, p->ai_socktype, 0);
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
        if (listenfd == -1)
            continue;
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
            break;
    }
    if (p == NULL) {
        perror("socket() or bind()");
        exit(1);
    }

    freeaddrinfo(res);

    // listen for incoming connections
    if (listen(listenfd, QUEUE_SIZE) != 0) {
        perror("listen() error");
        exit(1);
    }
}

// get request header by name
char *request_header(const char *name) {
    header_t *h = reqhdr;
    while (h->name) {
        if (strcmp(h->name, name) == 0)
            return h->value;
        h++;
    }
    return NULL;
}

char headerIsPresent(const char *name) {
    header_t *h = reqhdr;
    while (h->name) {
        if (strcmp(h->name, name) == 0) return 1;
        h++;
    }

    return 0;
}

// get all request headers
header_t *request_headers(void) { return reqhdr; }

// Handle escape characters (%xx)
static void uri_unescape(char *uri) {
    char chr = 0;
    char *src = uri;
    char *dst = uri;

    // Skip inital non encoded character
    while (*src && !isspace((int)(*src)) && (*src != '%'))
        src++;

    // Replace encoded characters with corresponding code.
    dst = src;
    while (*src && !isspace((int)(*src))) {
        if (*src == '+')
            chr = ' ';
        else if ((*src == '%') && src[1] && src[2]) {
            src++;
            chr = ((*src & 0x0F) + 9 * (*src > '9')) * 16;
            src++;
            chr += ((*src & 0x0F) + 9 * (*src > '9'));
        } else
            chr = *src;
        *dst++ = chr;
        src++;
    }
    *dst = '\0';
}

static int slot = 0;
//client connection
void respond(int n) {
    int rcvd;

    char *buf = malloc(BUF_SIZE);
    char *freebuf = buf;
    const char *connection;
    int clientfd = clients[n];
    slot = n;
    do {
//        fprintf(stderr, "waiting on receive\n");
        rcvd = recv(clientfd, buf, BUF_SIZE, 0);

        if (rcvd < 0) {   // receive error
            fprintf(stderr, "recv() error %d\n", rcvd);
            break;
        } else if (rcvd == 0) {   // receive socket closed
            fprintf(stderr, "Client disconnected unexpectedly.\n");
            break;
        } else { // message received
            buf[rcvd] = '\0';

            method = strtok(buf, " \t\r\n");
            uri = strtok(NULL, " \t");
            prot = strtok(NULL, " \t\r\n");

            uri_unescape(uri);

//            fprintf(stderr, "\x1b[32m + [%s] %s\x1b[0m\n", method, uri);

            qs = strchr(uri, '?');
            //-bash: привет: command not found 28+12, 7 + 12 + 19 + 1?
            if (qs) {
                *qs++ = '\0'; // split URI
            }
            else {
                qs = uri - 1; // use an empty string
            }

            header_t *h = reqhdr;
            char *t, *t2;
            while (h < reqhdr + 16) {
                char *key, *val;

                key = strtok(NULL, "\r\n: \t");
                if (!key) break;

                val = strtok(NULL, "\r\n");
                while (*val && *val == ' ') val++;

                h->name = key;
                h->value = val;
                h++;
//                fprintf(stderr, "[H] %s: %s\n", key, val);
                t = val + 1 + strlen(val);
                if (t[1] == '\r' && t[2] == '\n') break;
            }

            t = strtok(NULL, "\r\n"); // now the *t shall be the beginning of user payload
            t2 = request_header("Content-Length"); // and the related header if there is
            payload = t;
            payload_size = t2 ? atol(t2) : (rcvd - (t - buf));
            if (payload != NULL) {
                fprintf(stderr, "string: %s\n", payload);
                fprintf(stderr, "code: %d\n", *payload);
                fprintf(stderr, "payload_size = %d\n", payload_size);
            } else {
//                fprintf(stderr, "body is empty\n");
            }

            connection = request_header("Connection");

            int closeConn = route();
            if (closeConn == CLOSE_CONN) break;
        }
    } while (connection && strcmp("keep-alive", connection) == 0);

    shutdown(clientfd, SHUT_WR);
    free(freebuf);
}

//TODO: make one to use this only for body
int writeConn(const char *data, ...) {
    va_list args;
    size_t len2 = strlen(data);
    char buffer[len2 + 64];
    char *wrbuf = buffer;
    va_start(args, data);
    int len = vsprintf(buffer, data, args);
//    fprintf(stderr, "strlen in writeConn: %d, strlen = %lu\n", len, len2);
    va_end(args);

    if (len != len2) {
        wrbuf = data;
        len = len2;
    }

    int sum = 0;
    while (len > 0) {
        int i = send(clients[slot], wrbuf + sum, len - sum, 0);
        if (i < 1) return -1;
        sum += i;
        len -= i;
    }
    return 1;
}

void httpCode(int code) {
    char msg[64];
    int n = 0;
    switch (code) {
        case 200:
            n = sprintf(msg, "%s 200 OK\r\n", RESPONSE_PROTOCOL);
            break;
        case 201:
            n = sprintf(msg, "%s 201 Created\r\n", RESPONSE_PROTOCOL);
            break;
        case 404:
            n = sprintf(msg, "%s 404 Not found\r\n", RESPONSE_PROTOCOL);
            break;
        case 500:
            n = sprintf(msg, "%s 500 Internal Server Error\r\n", RESPONSE_PROTOCOL);
            break;
    }

    msg[n] = 0;
    writeConn(msg);
}

int writeHeader(const char *k, const char *v) {
    char data[256];
    int n = sprintf(data, "%s: %s\r\n", k, v);
    data[n] = 0;
    return writeConn(data);
}
