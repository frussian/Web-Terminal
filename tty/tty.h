//
// Created by zoomo on 16.07.2021.
//

#ifndef WEBTERMINAL_TTY_H
#define WEBTERMINAL_TTY_H

#include <esc_parser.h>
#include <stdint.h>

struct tty {
    int master;
    char *buf;
    size_t size;
    char changed;
    int rawStart;
    struct character *chars;
    struct esc_parser pars;
    size_t charSize;
    struct character current_char;
    int esc_seq;
    u_int32_t utf8_state;
};

struct tty startTerminal();
int writeTerminal(char*, size_t, struct tty);
char *getBuf(struct tty);
char *getHTML(struct tty *, int *);
int readTerminal(struct tty *pt);
int parseTerminal(struct tty *pt);

#endif //WEBTERMINAL_TTY_H
