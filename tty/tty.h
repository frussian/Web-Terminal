//
// Created by zoomo on 16.07.2021.
//

#ifndef WEBTERMINAL_TTY_H
#define WEBTERMINAL_TTY_H

#include <esc_parser.h>
#include <editor.h>
#include <stdint.h>

struct tty {
    int master;
    int term_log_fd;
    char *buf;
    size_t size;
    int rawStart;
    struct esc_parser pars;
    struct character current_char;
    int esc_seq;
    u_int32_t utf8_state;
    struct editor ed;
};

struct tty_settings {
    char *terminal;
};

struct tty start_terminal(struct tty_settings settings);
int write_terminal(char *data, size_t len, struct tty pt);
int read_terminal(struct tty *pt);
int parse_terminal(struct tty *pt);

#endif //WEBTERMINAL_TTY_H
