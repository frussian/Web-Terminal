//
// Created by Anton on 07.07.2021.
//

#include <fcntl.h>
#include <unistd.h>
#include <httpd.h>
#include <tty.h>

static struct tty pt;

int main(int c, char** v) {
    pt = startTerminal();
    if (pt.master < 0) return -1;

    int fd = open("stderr_log.txt", O_WRONLY | O_CREAT | O_TRUNC);
    dup2(fd, STDERR_FILENO);
    serve_forever(3018, &pt);
    return 0;
}


//1 - ok
//-1 - close connection
int route(struct request req) {
    ROUTE_START()

    GET("/") {
        int len = 0;
        char *html = getHTML(&pt, &len);
        if (html == NULL) {
            fprintf(stderr, "html is null\n");
            http_code(req.fd, 500);
            return KEEP_CONN;
        } else {
//            fprintf(stderr, "got html\n");
            char lenstr[16];
            int i = sprintf(lenstr, "%d", len);
            lenstr[i] = 0;

            http_code(req.fd, 200);
            write_header(req.fd, "Content-Length", lenstr);
            write_header(req.fd, "Content-Type", "text/html; charset=utf-8");
        }

        if (header_is_present("Origin")) {
            write_header(req.fd, "Access-Control-Allow-Origin", "*");  //TODO: change it to one domain
        }

            write_conn(req.fd, "\r\n");

            write_conn(req.fd, html);
    }

    POST("/") {
        if (req.payload != NULL) {
            if (strcmp("newline", req.payload) == 0) {
                writeTerminal("\n", 1, pt);
            } else {
                writeTerminal(req.payload, req.payload_size, pt);
            }

            http_code(req.fd, 200);

            if (header_is_present("Origin")) {
                write_header(req.fd, "Access-Control-Allow-Origin", "*");  //TODO: change it to one domain
            }

            write_header(req.fd, "Content-Length", "2");
            write_conn(req.fd, "\r\n");

            write_conn(req.fd, "ok");
        } else {
            http_code(req.fd, 400);
            write_header(req.fd, "Content-Length", "12");

            if (header_is_present("Origin")) {
                write_header(req.fd, "Access-Control-Allow-Origin", "*");  //TODO: change it to one domain
            }

            write_conn(req.fd, "\r\n");
            write_conn(req.fd, "invalid data");
        }

    }

    OPTIONS("/") {
            http_code(req.fd, 200);
            write_header(req.fd, "Access-Control-Allow-Origin", "*");  //TODO: change it to one domain and add methods, headers
            write_header(req.fd, "Access-Control-Allow-Methods", "POST, GET, OPTIONS");
            write_header(req.fd, "Access-Control-Allow-Headers", "Content-Type");
            write_header(req.fd, "Access-Control-Max-Age", "86400");
            write_header(req.fd, "Connection", "keep-alive");
            write_header(req.fd, "Content-Length", "0");
            write_conn(req.fd, "\r\n");
    }

        ROUTE_END()

    return KEEP_CONN;
}
