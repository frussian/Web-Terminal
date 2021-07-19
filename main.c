//
// Created by Anton on 07.07.2021.
//

#include "server/httpd.h"

int main(int c, char** v) {
    serve_forever("3018");
    return 0;
}


//1 - ok
//-1 - close connection
int route() {
    ROUTE_START()

    GET("/") {
        httpCode(200);

        char data[256];
        int contentLen = sprintf(data, "Hello! You are using %s", request_header("User-Agent"));
        data[contentLen] = 0;

        char contentLenStr[6];
        int n = sprintf(contentLenStr, "%d", contentLen);
        contentLenStr[n] = 0;
        writeHeader("Content-Length", contentLenStr);

        if (request_header("Origin") != NULL) {
            fprintf(stderr, "Origin is present\n");
            writeHeader("Access-Control-Allow-Origin", "*");  //TODO: change it to one domain
        }

        writeConn("\r\n");

        writeConn(data);
    }

    GET("/test") {
        httpCode(200);
        writeHeader("Connection", "close");
        writeConn("\r\n");
        writeConn("Hello!");
        return CLOSE_CONN;
    }

    POST("/") {
        httpCode(200);
        writeHeader("Content-Length", "2");
        if (request_header("Origin") != NULL) {
            fprintf(stderr, "Origin is present\n");
            writeHeader("Access-Control-Allow-Origin", "*");  //TODO: change it to one domain
        }
        writeConn("\r\n");
        writeConn("32");
//        return CLOSE_CONN;
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
