#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>

#include <httpd.h>
#include <tty.h>

#define MAX_CONNECTIONS 10
#define BUF_SIZE 65535
#define QUEUE_SIZE 32

static int start_server(int);
static int respond(int);

// Client request
char *method, // "GET" or "POST"
        *uri,     // "/index.html" things before '?'
        *qs,      // "a=1&b=2" things after  '?'
        *prot,    // "HTTP/1.1"
        *payload; // for POST

int payload_size;

static header_t reqhdr[17] = {{"\0", "\0"}};
static int header_num = 0;

void serve_forever(int port, struct tty *pt) {
    int new_sd, listen_sd = 0;
    struct pollfd fds[MAX_CONNECTIONS];
    int nfds = 2, current_size = 0, i, j;
    int timeout;
    int rc;
    int end_server = 0, conn, compress_array = 0;

    printf("Server started %shttp://127.0.0.1:%d%s\n", "\033[92m", port,
           "\033[0m");

    listen_sd = start_server(port);
    fds[0].fd = listen_sd;
    fds[0].events = POLLIN;
    timeout = -1;
    fds[1].fd = pt->master;
    fds[1].events = POLLIN;

    do {
        rc = poll(fds, nfds, timeout);
        if (rc < 0) {
            perror(" poll() failed\n");
            break;
        }


        current_size = nfds;
        for (i = 0; i < current_size; i++) {
            if (fds[i].revents == 0) {
                continue;
            }

            if (fds[i].revents != POLLIN) {
                fprintf(stderr, "unexpected revents: %d\n", fds[i].revents);
                end_server = 1;
                break;
            }

            if (fds[i].fd == listen_sd) {
                do {
                    new_sd = accept(listen_sd, NULL, NULL);
                    printf("accepting client\n");
                    if (new_sd < 0) {
                        if (errno != EWOULDBLOCK) {
                            perror(" accept() failed");
                            end_server = 1;
                        }
                        break;
                    }
                    if (nfds == MAX_CONNECTIONS) {
                        close(new_sd);
                        break;
                    }
                    fds[nfds].fd = new_sd;
                    fds[nfds].events = POLLIN;
                    nfds++;
                } while (new_sd != -1);
            } else if (fds[i].fd == pt->master) {
                rc = readTerminal(pt);
                if (rc != 0) {
                    fprintf(stderr, "read terminal error %d\n", rc);
                }
            } else {
                conn = respond(fds[i].fd);
                if (conn == CLOSE_CONN) {
                    printf("user left\n");
                    close(fds[i].fd);
                    fds[i].fd = -1;
                    compress_array = 1;
                }
            }
        }

        if (compress_array) {
            compress_array = 0;
            for (i = 0; i < nfds; i++) {
                if (fds[i].fd == -1) {
                    for(j = i; j < nfds - 1; j++) {
                        fds[j].fd = fds[j+1].fd;
                    }
                    i--;
                    nfds--;
                }
            }
        }
    } while (end_server == 0);

    for (i = 0; i < nfds; i++) {
        if (fds[i].fd >= 0) close(fds[i].fd);
    }
}

// start server
int start_server(int port) {
    int    rc, on = 1;
    int    listen_sd = -1;
    struct sockaddr_in6   addr;

    listen_sd = socket(AF_INET6, SOCK_STREAM, 0);
    if (listen_sd < 0) {
        perror("socket() failed");
        exit(-1);
    }

    rc = setsockopt(listen_sd, SOL_SOCKET,  SO_REUSEADDR,
                    (char *)&on, sizeof(on));
    if (rc < 0) {
        perror("setsockopt() failed");
        close(listen_sd);
        exit(-1);
    }

    rc = ioctl(listen_sd, FIONBIO, (char *)&on);
    if (rc < 0) {
        perror("ioctl() failed");
        close(listen_sd);
        exit(-1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin6_family      = AF_INET6;
    memcpy(&addr.sin6_addr, &in6addr_any, sizeof(in6addr_any));

    addr.sin6_port        = htons(port);
    rc = bind(listen_sd,
              (struct sockaddr *)&addr, sizeof(addr));
    if (rc < 0) {
        perror("bind() failed");
        close(listen_sd);
        exit(-1);
    }

    rc = listen(listen_sd, QUEUE_SIZE);
    if (rc < 0) {
        perror("listen() failed");
        close(listen_sd);
        exit(-1);
    }
    return listen_sd;
}

// get request header by name
char *request_header(const char *name) {
    header_t *h = reqhdr;
    int i = 0;
    while (i < header_num) {
        if (strcmp(h->name, name) == 0)
            return h->value;
        h++;
        i++;
    }
    return NULL;
}

char header_is_present(const char *name) {
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


//client connection
int respond(int fd) {
    int rcvd = 0;
    char *buf = malloc(BUF_SIZE);
    char *freebuf = buf;
    const char *connection = NULL;
    int conn = 0;
    struct request req;

    rcvd = recv(fd, buf, BUF_SIZE, 0);

    if (rcvd < 0) {   // receive error
        if (errno != EWOULDBLOCK) {
            fprintf(stderr, "recv() error %d\n", rcvd);
            conn = CLOSE_CONN;
        }
    } else if (rcvd == 0) {   // receive socket closed
        printf("Client disconnected unexpectedly.\n");
        conn = CLOSE_CONN;
    } else { // message received
        buf[rcvd] = '\0';

        method = strtok(buf, " \t\r\n");
        uri = strtok(NULL, " \t");
        prot = strtok(NULL, " \t\r\n");

        uri_unescape(uri);

        qs = strchr(uri, '?');
        if (qs) {
            *qs++ = '\0'; // split URI
        }
        else {
            qs = uri - 1; // use an empty string
        }

        header_t *h = reqhdr;
        header_num = 0;
        char *t, *t2 = NULL;
        while (h < reqhdr + 16) {
            char *key, *val;

            key = strtok(NULL, "\r\n: \t");
            if (!key) break;

            val = strtok(NULL, "\r\n");
            while (*val && *val == ' ') val++;

            h->name = key;
            h->value = val;
            h++;
            header_num++;
            t = val + 1 + strlen(val);
            if (t[1] == '\r' && t[2] == '\n') break;
        }

        t = strtok(NULL, "\r\n"); // now the *t shall be the beginning of user payload
        t2 = request_header("Content-Length"); // and the related header if there is
        payload = t;
        payload_size = t2 ? atol(t2) : (rcvd - (t - buf));
        if (payload != NULL) {
            /*fprintf(stderr, "string: %s\n", payload);
            fprintf(stderr, "code: %d\n", *payload);
            fprintf(stderr, "payload_size = %d\n", payload_size);
             */
        } else {
//               fprintf(stderr, "body is empty\n");
        }

        connection = request_header("Connection");
        req.method = method;
        req.payload = payload;
        req.payload_size = payload_size;
        req.prot = prot;
        req.qs = qs;
        req.uri = uri;
        req.fd = fd;

        conn = route(req);
    }

    if (conn == KEEP_CONN && connection && strcmp("keep-alive", connection) != 0) {
        conn = CLOSE_CONN;
    }

    free(freebuf);
    return conn;
}

//TODO: make one to use this only for body
int write_conn(int fd, const char *data, ...) {
    va_list args;
    size_t len2 = strlen(data);
    char buffer[len2 + 64];
    char *wrbuf = buffer;
    va_start(args, data);
    int len = vsprintf(buffer, data, args);
//    fprintf(stderr, "strlen in write_conn: %d, strlen = %lu\n", len, len2);
    va_end(args);

    if (len != len2) {
        wrbuf = data;
        len = len2;
    }

    int sum = 0;
    while (len > 0) {
        int i = send(fd, wrbuf + sum, len - sum, 0);
        if (i < 1) return -1;
        sum += i;
        len -= i;
    }
    return 1;
}

void http_code(int fd, int code) {
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
    write_conn(fd, msg);
}

int write_header(int fd, const char *k, const char *v) {
    char data[256];
    int n = sprintf(data, "%s: %s\r\n", k, v);
    data[n] = 0;
    return write_conn(fd, data);
}
