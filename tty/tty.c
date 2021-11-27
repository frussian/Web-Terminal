//
// Created by zoomo on 16.07.2021.
//

#define _XOPEN_SOURCE 600
#define _DEFAULT_SOURCE

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <string.h>

#include <esc_parser.h>

struct tty startTerminal() {
    struct tty pt;
    int master = posix_openpt(O_RDWR); //TODO: set O_NOCTTY?
    if (master < 0) {
        fprintf(stderr, "error %d on posix_openpt()\n", errno);
        pt.master = master;
        return pt;
    }

    pt.master = master;
    pt.buf = NULL;
    pt.size = 0;
    pt.rawStart = 0;
    pt.chars = NULL;
    pt.charSize = 0;
    pt.changed = 1;

    int flags = fcntl(master, F_GETFL, 0);
    if (flags == -1) {
        pt.master = -1;
        return pt;
    }
    flags = flags | O_NONBLOCK;
    fcntl(master, F_SETFL, flags);

    flags = fcntl(master, F_GETFD, 0);  //TODO: fix
    if (flags == -1) {
        pt.master = flags;
        return pt;
    }
    flags = flags | FD_CLOEXEC;
    fcntl(master, F_SETFD, flags);

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
        char *name = ptsname(master);
        fprintf(stderr, "%s\n", name);
        int slave = open(name, O_RDWR);
        close(master);

        struct termios terminalCfg;

        rc = tcgetattr(slave, &terminalCfg);
//        cfmakeraw(&terminalCfg);
        terminalCfg.c_lflag &= ~ECHOCTL;
        terminalCfg.c_lflag |= ECHOE;   //TODO: might be the reason for ]K when delete on ubuntu vt
        tcsetattr(slave, TCSANOW, &terminalCfg);

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        dup2(slave, STDIN_FILENO);
        dup2(slave, STDOUT_FILENO);
        dup2(slave, STDERR_FILENO);

        close(slave);

        setsid();

        ioctl(STDIN_FILENO, TIOCSCTTY, 1); //1
        execl("/bin/bash", "/bin/bash", NULL);
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

int checkZeros(char *buf, size_t size) {

}

int readTerminal(struct tty *pt) {
    size_t size = 256, sum = 0;
    char *data = malloc(size);
    while (1) {
        int i = read(pt->master, data + sum, size - sum);   //TODO: wsl bug redundant zeros
        fprintf(stderr, "read %d bytes from terminal\n", i);
        if (i <= 0) {
            break;
        }
        sum += i;
        if (sum >= size) {
            size *= 2;
            data = realloc(data, size);
        }
    }

//    fprintf(stderr, "sum = %zu\n", sum);
    if (sum != 0) {
        if (data == NULL) {
            fprintf(stderr, "problem with realloc1\n");
            return -1;
        }

        pt->buf = append(pt->buf, pt->size, data, sum);
        pt->rawStart = pt->size;
        pt->size += sum;
//        write(STDERR_FILENO, data, sum);
    }

    free(data);

    fprintf(stderr, "%p parsing terminal\n", (void*)pt);
    int res = parseTerminal(pt);
    if (res < 0) {
        fprintf(stderr, "\x1b[32mparse error\x1b[0m\n");
        return res;
    }
//    fprintf(stderr, "read success, pt->size = %zu\n", pt->size);
    return 0;
}

char *getBuf(struct tty pt) {
    return pt.buf;
}

char *generateStyleStr(struct style *s, int *num) {
    char *buf = NULL;
    int len = 0;

    buf = append(buf, 0, "<span style=\"", 13);
    len += 13;

    if (s->bColor) {
        buf = append(buf, len, "background-color:", 17);
        len += 17;

        int len2 = strlen(s->bColor);
        buf = append(buf, len, s->bColor, len2);
        len += len2;

        buf = append(buf, len, ";", 1);
        len++;
    }

    if (s->fColor) {
        buf = append(buf, len, "color:", 6);
        len += 6;

        int len2 = strlen(s->fColor);
        buf = append(buf, len, s->fColor, len2);
        len += len2;

        buf = append(buf, len, ";", 1);
        len++;
    }

    if (s->bold) {
        buf = append(buf, len, "font-weight:bold;", 17);
        len += 17;
    }

    if (s->italic) {
        buf = append(buf, len, "font-style:italic;", 18);
        len += 18;
    }

    if (s->underline) {
        buf = append(buf, len, "text-decoration:underline;", 26);
        len += 26;
    }

    if (len == 13) {
        free(buf);
        return NULL;
    }

    buf = append(buf, len, "\">", 2);
    len += 2;

    *num = len;

    return buf;
}

char *getHTML(struct tty *pt, int *len) {
    fprintf(stderr, "\x1b[32mgetting html\x1b[0m\n");

//    write(STDERR_FILENO, pt->buf, pt->size);

    if (!pt->changed) {
        *len = 10;
        printf("no changes\n");
        return "no changes";
    }

    char *html = NULL;
    int sum = 0;
    int i = 0;
    html = append(html, 0, "<p>", 3);
    if (html == NULL) {
        fprintf(stderr, "html is null\n");
        return NULL;
    }
    sum += 3;

    struct style s;
    char *styleStr = NULL;
    int styleStrLen = 0;
    clearStyle(&s);

    while (i < pt->charSize) {
        struct character c = pt->chars[i];
        if (*c.c == '\n') {
            html = append(html, sum, "</p><p>", 7);
            sum += 7;
            i++;
        } else {
            if (!styleEqual(&c.s, &s)) {
                if (styleStr) {
                    html = append(html, sum, "</span>", 7);
                    sum += 7;
                }

                styleStrLen = 0;

                s = c.s;

                if (!styleIsEmpty(&c.s)) {
                    styleStr = generateStyleStr(&c.s, &styleStrLen);
                    if (styleStr < 0) {
                        fprintf(stderr, "\x1b[32mnum is negative\x1b[0m\n");
                        styleStr = 0;
                    }
                }

                if (styleStr) {
                    html = append(html, sum, styleStr, styleStrLen);
                    sum += styleStrLen;
                }

            }

            html = append(html, sum, c.c, c.size);
            sum += c.size;

            if (*c.c == 0) {
                fprintf(stderr, "found 0 in pt.buf\n");
            }

            i++;
        }
    }

    if (i == 0 || *pt->chars[i-1].c != '\n') {
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