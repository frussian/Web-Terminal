//
// Created by zoomo on 16.07.2021.
//

#define _XOPEN_SOURCE 600
#define _DEFAULT_SOURCE

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <string.h>

#include "tty.h"

struct tty startTerminal() {
    struct tty pt;
    int master = posix_openpt(O_RDWR); //TODO: set O_NOCTTY?
    if (master < 0) {
        fprintf(stderr, "error %d on posix_openpt()", errno);
        pt.master = master;
        return pt;
    }

    pt.master = master;
    pt.buf = NULL;
    pt.size = 0;

    int flags = fcntl(master, F_GETFL, 0);
    if (flags == -1) {
        pt.master = -1;
        return pt;
    }
    flags = flags | O_NONBLOCK;
    fcntl(master, F_SETFL, flags);

//    flags = fcntl(master, F_GETFD, 0);  //TODO: fix
//    if (flags == -1) {
//        pt.master = flags;
//        return pt;
//    }
//    flags = flags | FD_CLOEXEC;
//    fcntl(master, F_SETFD, flags);

    int rc = grantpt(master);
    if (rc != 0) {
        fprintf(stderr, "error %d on grantpt()", errno);
        pt.master = -1;
        return pt;
    }

    rc = unlockpt(master);
    if (rc != 0) {
        fprintf(stderr, "error %d on unlockpt()", errno);
        pt.master = -1;
        return pt;
    }

    if (!fork()) {
        int slave = open(ptsname(master), O_RDWR);
        close(master);

//        struct termios slave_orig_term_settings;
//        struct termios new_term_settings;
//
//        // Save the defaults parameters of the slave side of the PTY
//        rc = tcgetattr(slave, &slave_orig_term_settings);
//
//        // Set RAW mode on slave side of PTY
//        new_term_settings = slave_orig_term_settings;
//        cfmakeraw(&new_term_settings);
//        tcsetattr(slave, TCSANOW, &new_term_settings);

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        dup2(slave, STDIN_FILENO);
        dup2(slave, STDOUT_FILENO);
        dup2(slave, STDERR_FILENO);
        close(slave);

        setsid();

        ioctl(STDIN_FILENO, TIOCSCTTY, 1);
        execl("/bin/sh", "/bin/sh", NULL);
    }

    return pt;
}

char *append(char* s1, int len1, const char *s2, int len2) {
    char *new = realloc(s1, len1 + len2);
    if (new == NULL) return NULL;

    memcpy(new + len1, s2, len2);
    return new;
}

int writeTerminal(char *data, size_t len, struct tty pt) {
    return write(pt.master, data, len);
}

int readTerminal(struct tty *pt) {
    size_t size = 256, sum = 0;
    char *data = malloc(size);
    while (1) {
        int i = read(pt->master, data + sum, size - sum);
        fprintf(stderr, "read %d bytes from terminal\n", i);
        if (i < 0) {
            break;
        }
        sum += i;
        if (sum >= size) {
            size *= 2;
            data = realloc(data, size);     //TODO: use append?
        }
    }

    if (sum != 0) {
        data = realloc(data, sum);
        if (data == NULL) {
            fprintf(stderr, "problem with realloc1\n");
            return -1;
        }

        pt->buf = append(pt->buf, pt->size, data, sum);
        pt->size += sum;
    }
    //TODO: parseTerminal to remove some characters if esc seq is present,
    // do not add to header

    free(data);
//    fprintf(stderr, "read success, pt->size = %zu\n", pt->size);
    return 0;
}

char *getBuf(struct tty pt) {
    return pt.buf;
}

char *getHTML(struct tty pt, int *len) {
//    fprintf(stderr, "getting html\n");
    char *html = NULL;
    int sum = 0;
    int i = 0;
    html = append(html, 0, "<p>", 3);
    if (html == NULL) {
        fprintf(stderr, "html is null\n");
        return NULL;
    }
    sum += 3;

    while (i < pt.size) {
        switch (pt.buf[i]) {
            case '\n':
                html = append(html, sum, "</p><p>", 7);
                sum += 7;
                break;
            default:
                if (pt.buf[i] == 0) {
                    fprintf(stderr, "found 0 in pt.buf\n");
                }
                html = append(html, sum, &pt.buf[i], 1);
                sum++;
                break;
        }
        i++;
    }

    if (i == 0 || pt.buf[i-1] != '\n') {
        html = append(html, sum, "</p>", 4);
        sum += 4;
    }

    html = realloc(html, sum + 1);
    if (html == NULL) {
        fprintf(stderr, "html: realloc returned null\n");
        return NULL;
    }
    html[sum] = 0;

    if (len) *len = sum;
    return html;
}