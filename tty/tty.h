//
// Created by zoomo on 16.07.2021.
//

#ifndef WEBTERMINAL_TTY_H
#define WEBTERMINAL_TTY_H

struct tty {
    int master;
    char *buf;
    size_t size;
};

struct tty startTerminal();
int writeTerminal(char*, size_t, struct tty);
int readTerminal(struct tty *);
char *getBuf(struct tty);
char *getHTML(struct tty, int *);

#endif //WEBTERMINAL_TTY_H
