//
// Created by Anton on 07.07.2021.
//

#include "server/httpd.h"
#include "tty/tty.h"

static struct tty pt; //create shared memory for this

int main(int c, char** v) {
    pt = startTerminal();
    if (pt.master < 0) return 1;

    serve_forever("3018");
    return 0;
}


//1 - ok
//-1 - close connection
int route() {
    ROUTE_START()

    GET("/") {
        int len = 0;
        char *html = getHTML(&pt, &len);
        if (html == NULL) {
            fprintf(stderr, "html is null\n");
            httpCode(500);
            return KEEP_CONN;
        } else {
//            fprintf(stderr, "got html\n");
            char lenstr[16];
            int i = sprintf(lenstr, "%d", len);
            lenstr[i] = 0;

            httpCode(200);
            writeHeader("Content-Length", lenstr);
            writeHeader("Content-Type", "text/html; charset=utf-8");
        }

        if (headerIsPresent("Origin")) {
            writeHeader("Access-Control-Allow-Origin", "*");  //TODO: change it to one domain
        }

        writeConn("\r\n");

        writeConn(html);
    }

    POST("/") {
        if (payload != NULL) {
            if (strcmp("newline", payload) == 0) {
                writeTerminal("\n", 1, pt);
            } else {
                writeTerminal(payload, payload_size, pt);
            }

            httpCode(200);

            if (headerIsPresent("Origin")) {
                writeHeader("Access-Control-Allow-Origin", "*");  //TODO: change it to one domain
            }

            writeHeader("Content-Length", "2");
            writeConn("\r\n");

            writeConn("ok");
        } else {
            httpCode(400);
            writeHeader("Content-Length", "12");

            if (headerIsPresent("Origin")) {
                writeHeader("Access-Control-Allow-Origin", "*");  //TODO: change it to one domain
            }

            writeConn("\r\n");
            writeConn("invalid data");
        }

    }

    OPTIONS("/") {
        httpCode(200);
        writeHeader("Access-Control-Allow-Origin", "*");  //TODO: change it to one domain and add methods, headers
//        writeHeader("")
        writeHeader("Connection", "keep-alive");
        writeHeader("Content-Length", "0");
        writeConn("\r\n");
    }

    ROUTE_END()

    return KEEP_CONN;
}
