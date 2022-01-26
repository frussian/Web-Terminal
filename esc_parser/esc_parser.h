//
// Created by Anton on 21.07.2021.
//

#ifndef WEBTERMINAL_ESC_PARSER_H
#define WEBTERMINAL_ESC_PARSER_H

#include <stdlib.h>
#include <tools.h>

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
#define SET_ICON_WINDOW_NAME 20
#define SET_ICON_NAME 21
#define SET_WINDOW_NAME 22
#define ERASE_N_CHARS_FROM_CURSOR 23
#define ALT_BUF_ON 24
#define ALT_BUF_OFF 25
#define HIDE_CUR 26
#define SHOW_CUR 27
#define AUTO_WRAP_ON 28
#define AUTO_WRAP_OFF 29
#define MOVE_CUR_ABS 30
#define DELETE_N_CHARS_RIGHT_FROM_CURSOR_WITH_SHIFT 31  //ESC[amount P
#define NOT_SUPPORTED 100

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
    char c[4];
    size_t size;    //TODO: for utf
    struct style s;
};

struct esc {
    int code;
    struct style s;
    struct moveCursor cursor;
    int alt_buf_clear_on_enter;
    int alt_buf_clear_on_exit;
};

#define MAX_DIGITS 6
#define MAX_DIGITS_LEN 10

struct esc_parser {
    int ended;
    struct esc res;
    int state;
    char digits[MAX_DIGITS][MAX_DIGITS_LEN];
    short digitsNum;
    short currentDigitPos;
    short maxDigits;
    short maxDigitLen;
    struct abuf text;
};

void clearStyle(struct style *s);
int styleIsEmpty(struct style *s);
int styleEqual(struct style *s1, struct style *s2);
void parseEsc(struct esc_parser*, char);   //TODO: remove from header
int init_parser(struct esc_parser *parser);

#endif //WEBTERMINAL_ESC_PARSER_H
