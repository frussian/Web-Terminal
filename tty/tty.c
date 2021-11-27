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

#include <tty.h>


//https://stackoverflow.com/questions/1031645/how-to-detect-utf-8-in-plain-c
#define UTF8_ACCEPT 0
#define UTF8_REJECT 1

static const uint8_t utf8d[] = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
        8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
        0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
        0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff
        0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
        1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
        1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
        1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
};

uint32_t validate_utf8(uint32_t *state, char *str, size_t len) {
    size_t i;
    uint32_t type;

    for (i = 0; i < len; i++) {
        // We don't care about the codepoint, so this is
        // a simplified version of the decode function.
        type = utf8d[(uint8_t)str[i]];
        *state = utf8d[256 + (*state) * 16 + type];

        if (*state == UTF8_REJECT)
            break;
    }

    return *state;
}

void clearChar(struct character *c) {
    c->size = 0;
    for (int i = 0; i < 4; i++) {
        c->c[i] = 0;
    }
}

struct tty startTerminal(struct tty_settings settings) {
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
    pt.esc_seq = 0;

    pt.utf8_state = UTF8_ACCEPT;
    clearChar(&pt.current_char);

    reinit_parser(&pt.pars);

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

        char *envp[2] = {NULL, NULL};
        char buf[64];
        if (settings.terminal) {
            memcpy(buf, "TERM=", 5);
            buf[5] = 0;
            strcat(buf, settings.terminal);
            envp[0] = buf;
        }

        execle("/bin/bash", "/bin/bash", NULL, envp);
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

struct character *appendChars(const struct character *s1, int len1, const struct character *s2, int len2) {
    struct character *new = realloc(s1, sizeof(struct character) * (len1 + len2));
    if (new == NULL) return NULL;

    memcpy((new + len1), s2, sizeof(struct character) * len2);
    return new;
}

int parseTerminal(struct tty *pt) {
    fprintf(stderr, "%p %p size = %zu parsing terminal\n", (void *) pt, pt->buf, pt->size);
    char *buf = pt->buf;
    int i = pt->rawStart;
    size_t size = pt->size;

    struct style currentStyle;
    clearStyle(&currentStyle);

    size_t charsSize = 256;
    int charsNum = 0;
    struct character *chars = (struct character *) malloc(charsSize * sizeof(struct character));
    if (chars == NULL) {
        fprintf(stderr, "chars malloc error\n");
        return -1;
    }

//    for (int j = 0; j < size; j++) {
//        fprintf(stderr, "%d\n", buf[j]);
//    }

    for (; i < size; i++) {
        char c = buf[i];
        if (pt->esc_seq) {
            fprintf(stderr, "%c\n", c);
            parseEsc(&pt->pars, c);
            if (!pt->pars.ended) continue;
            pt->pars.ended = 0;
            pt->esc_seq = 0;
            struct esc res = pt->pars.res;
            switch (res.code) {
                case ERROR: {
                    fprintf(stderr, "parsing escape seq error\n");
                    break;
                }
                case NOT_SUPPORTED: {
                    fprintf(stderr, "unsupported esc seq %d\n", c);
                    break;
                }
                case STYLE: {
                    currentStyle = res.s;
                    break;
                }
                case RESET_STYLE: {
                    clearStyle(&currentStyle);
                    break;
                }
                default: {
                    fprintf(stderr, "res.code = %d\n", res.code);
                }
            }
            reinit_parser(&pt->pars);
        } else {
            if (c == '\x1b') {
                fprintf(stderr, "\n");
                pt->esc_seq = 1;
                continue;
            }

            size_t char_size = pt->current_char.size;
            pt->current_char.c[char_size] = c;
            pt->current_char.size++;

            validate_utf8(&pt->utf8_state, &c, 1);
            if (pt->utf8_state == UTF8_REJECT) {
                fprintf(stderr, "symbol %d is not valid\n", *(int*)pt->current_char.c);
                clearChar(&pt->current_char);
                pt->utf8_state = UTF8_ACCEPT;
            } else if (pt->utf8_state == UTF8_ACCEPT) {
                pt->current_char.s = currentStyle;
                chars[charsNum++] = pt->current_char;
                clearChar(&pt->current_char);

                if (charsNum >= charsSize) {
                    charsSize *= 2;
                    chars = realloc(chars, charsSize * sizeof(struct character));
                }
            } else {
                fprintf(stderr, "needs more characters\n");
            }
        }
    }

    fprintf(stderr, "parsed %d\n", charsNum);

    if (charsNum != 0) {
        pt->chars = appendChars(pt->chars, pt->charSize, chars, charsNum);
        if (pt->chars == NULL) {
            fprintf(stderr, "append chars error\n");
            return -1;
        }

        pt->charSize += charsNum;
        pt->rawStart = size;
    }

    free(chars);
    return 0;
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
        if (sum == 2) {
            fprintf(stderr, "%d %d\n", data[0], data[1]);
        }
    }

    free(data);

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