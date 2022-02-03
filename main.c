//
// Created by Anton on 07.07.2021.
//

#include <fcntl.h>
#include <unistd.h>
#include <httpd.h>
#include <tty.h>
#include <editor.h>
#include <errno.h>

static struct tty pt;

int main(int c, char** v) {
    struct tty_settings settings;
    settings.terminal = "xterm-256color";

    pt = start_terminal(settings);
    if (pt.master < 0) return -1;

    int term_fd = open("terminal_out.txt", O_WRONLY | O_CREAT | O_TRUNC);
    if (term_fd < 0) {
        printf("term_fd, errno %d\n", errno);
        exit(1);
    }
    pt.term_log_fd = term_fd;
    //dup2(fd, STDERR_FILENO);
    serve_forever(3018, &pt);
    return 0;
}


//1 - ok
//-1 - close connection
int route(struct request req) {
    ROUTE_START()

    GET("/") {
        int len = 0;
        char *html = getHTML(&pt.ed, &len);
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
	    free(html);
    }

    POST("/") {
        if (req.dump_editor) {
            printf("dump editor\n");
            dump_editor(&pt.ed);
        }
        if (req.payload != NULL) {
            if (strcmp("newline", req.payload) == 0) {
                write_terminal("\n", 1, pt);
            } else {
                write_terminal(req.payload, req.payload_size, pt);
            }

            http_code(req.fd, 200);

            if (header_is_present("Origin")) {
                write_header(req.fd, "Access-Control-Allow-Origin", "*");  //TODO: change it to one domain
            }

            write_header(req.fd, "Content-Length", "2");
            write_conn(req.fd, "\r\n");

            write_conn(req.fd, "ok");
        } else {
//            http_code(req.fd, 400);
            http_code(req.fd, 200);
            write_header(req.fd, "Content-Length", "0");

            if (header_is_present("Origin")) {
                write_header(req.fd, "Access-Control-Allow-Origin", "*");  //TODO: change it to one domain
            }
            if (req.dump_editor) {
                printf("dump editor ok\n");
            }
            write_conn(req.fd, "\r\n");
//            write_conn(req.fd, "invalid data");
        }

    }

    OPTIONS("/") {
            http_code(req.fd, 200);
            write_header(req.fd, "Access-Control-Allow-Origin", "*");  //TODO: change it to one domain and add methods, headers
            write_header(req.fd, "Access-Control-Allow-Methods", "POST, GET, OPTIONS");
            write_header(req.fd, "Access-Control-Allow-Headers", "Content-Type, x-dump-editor");
            write_header(req.fd, "Access-Control-Max-Age", "86400");
            write_header(req.fd, "Connection", "keep-alive");
            write_header(req.fd, "Content-Length", "0");
            write_conn(req.fd, "\r\n");
    }

        ROUTE_END()

    return KEEP_CONN;
}
