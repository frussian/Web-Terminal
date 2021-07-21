//
// Created by Anton on 21.07.2021.
//

#ifndef WEBTERMINAL_ESC_PARSER_H
#define WEBTERMINAL_ESC_PARSER_H

#include <stdlib.h>
#include "../tty/tty.h"

#define ERROR 0
#define ERASE_VISIBLE_SCREEN 1
#define MOVE_CURSOR_HOME 2
#define MOVE_CURSOR_LINE_COLUMN 3
#define MOVE_CURSOR_LINE 4
#define MOVE_CURSOR_COLUMN 5
#define MOVE_CURSOR_BEGIN_NEXT_LINE 6
#define MOVE_CURSOR_BEGIN_PREV_LINE 7
#define MOVE_CURSOR_POS_COL 8         //absolute, move to
#define REQ_CURSOR 9
#define SAVE_CURSOR 10
#define RESTORE_CURSOR 11
#define ERASE_CUR_TO_END 12  //default
#define ERASE_START_TO_CURSOR 13
#define ERASE_ENTIRE_BUFFER 14
#define CLEAR_CURRENT_LINE 15
#define CLEAR_CUR_TO_END_OF_LINE 16
#define CLEAR_START_TO_CURSOR_LINE 17
#define RESET_STYLE 18
#define STYLE 19
#define NOT_SUPPORTED 20

struct style {
    char *fColor;
    char *bColor;
    char bold, italic, underline;
};

struct moveCursor {
    long line;
    long column;
};

struct character {
    char *c;
    size_t size;
    struct style s;
};

struct esc {
    int code;
    struct style s;
    struct moveCursor cursor;
};


int parseTerminal(struct tty *);
struct esc parseEsc(const char *buf, size_t maxsize, int *i);   //TODO: remove from header

#endif //WEBTERMINAL_ESC_PARSER_H
