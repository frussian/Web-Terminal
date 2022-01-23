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
#include <tools.h>

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

    int res = init_editor(&pt.ed);
    if (res < 0) {
        fprintf(stderr, "cannot initialize editor\n");
        exit(1);
    }

    pt.changed = 1;
    pt.esc_seq = 0;

    pt.utf8_state = UTF8_ACCEPT;
    clearChar(&pt.current_char);

    if (init_parser(&pt.pars) < 0) {
        fprintf(stderr, "cannot init parser: malloc");
        exit(1);
    }

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

        if (settings.terminal) {
            setenv("TERM", settings.terminal, 1);
        }
        //inherits all environment variables
        execl("/bin/bash", "/bin/bash", NULL);
    }

    return pt;
}

int writeTerminal(char *data, size_t len, struct tty pt) {
    return write(pt.master, data, len);
}

int checkZeros(char *buf, size_t size) {

}

struct character *appendChars(const struct character *s1, int len1, const struct character *s2, int len2) {
    struct character *new = realloc((void*)s1, sizeof(struct character) * (len1 + len2));
    if (new == NULL) return NULL;

    memcpy((new + len1), s2, sizeof(struct character) * len2);
    return new;
}

int parseTerminal(struct tty *pt) {
    char *buf = pt->buf;
    int i = pt->rawStart;
    size_t size = pt->size;

    struct style currentStyle;
    clearStyle(&currentStyle);

    for (; i < size; i++) {
        char c = buf[i];
//        fprintf(stderr, "%d\n", c);
        if (pt->esc_seq) {
            if (c == 7) {
                fprintf(stderr, "bell\n");
            } else {
                fprintf(stderr, "%c\n", c);
            }
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
                case MOVE_CURSOR_HOME: {
                    set_cx_cy(&pt->ed, 1, 1);
                    break;
                }
                case MOVE_CURSOR_LINE_COLUMN: {
                    set_cx_cy(&pt->ed, res.cursor.column, res.cursor.line);
                    break;
                }
                case MOVE_CURSOR_LINE: {
                    add_cy(&pt->ed, res.cursor.line);
                    break;
                }
                case MOVE_CURSOR_COLUMN: {
                    add_cx(&pt->ed, res.cursor.column);
                    break;
                }
                case MOVE_CURSOR_BEGIN_NEXT_LINE:
                case MOVE_CURSOR_BEGIN_PREV_LINE: {
                    set_cx(&pt->ed, 1);
                    add_cy(&pt->ed, res.cursor.line);
                    break;
                }
                case MOVE_CURSOR_POS_COL: {
                    set_cx(&pt->ed, res.cursor.column);
                    break;
                }
                case ERASE_VISIBLE_SCREEN: {
                    erase_visible_screen(&pt->ed);
                    break;
                }
                case SET_ICON_WINDOW_NAME: {
                    struct abuf a_buf = pt->pars.text;
                    write(STDOUT_FILENO, a_buf.buf, a_buf.offset);
                    write(STDOUT_FILENO, "\n", 1);
                    break;
                }
                case ERASE_N_CHARS_FROM_CURSOR: {
                    erase_n_chars_from_screen(&pt->ed, res.cursor.column);
                    break;
                }
                default: {
                    fprintf(stderr, "ignoring res.code = %d\n", res.code);
                }
            }

            if (init_parser(&pt->pars) < 0) {
                fprintf(stderr, "cannot reinit parser: malloc\n");
                exit(1);
            }
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
                add_char(&pt->ed, pt->current_char);
                clearChar(&pt->current_char);
            } else {
                fprintf(stderr, "needs more characters\n");
            }
        }
    }

//    pt->rawStart = size; todo: не нужно?

    return 0;
}

int readTerminal(struct tty *pt) {
    size_t size = 256, sum = 0;
    char *data = malloc(size);
    while (1) {
        int i = read(pt->master, data + sum, size - sum);   //TODO: wsl bug redundant zeros
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
        int res = write(pt->term_log_fd, data, sum);
       	if (res < 0) {
		    fprintf(stderr, "error writing, res %d, errno %d", res, errno);
	    }
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

//TODO: move to editor
