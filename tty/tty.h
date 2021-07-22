//
// Created by zoomo on 16.07.2021.
//

#ifndef WEBTERMINAL_TTY_H
#define WEBTERMINAL_TTY_H

#include "../esc_parser/esc_parser.h"

struct tty {
    int master;
    char *buf;
    size_t size;
    char changed;
    int rawStart;
    struct character *chars;
    size_t charSize;
};

struct tty startTerminal();
int writeTerminal(char*, size_t, struct tty);
char *getBuf(struct tty);
char *getHTML(struct tty *, int *);

#endif //WEBTERMINAL_TTY_H
