//
// Created by Anton on 21.07.2021.
//

#ifndef WEBTERMINAL_ESC_PARSER_H
#define WEBTERMINAL_ESC_PARSER_H

#include <stdlib.h>
#include <tools.h>

#define ERROR 0
#define ERASE_VISIBLE_SCREEN 1 //CSI 2 J - erase whole buffer, CSI is ESC [
#define MOVE_CURSOR_HOME 2 //CSI H - move cursor home
#define MOVE_CURSOR_LINE_COLUMN 3 //CSI <line> <column> H - move cursor
#define MOVE_CURSOR_LINE 4 //CSI <lines_num> A, CSI <lines_num> B - cursor UP/DOWN on amount <lines_num>
#define MOVE_CURSOR_COLUMN 5 //CSI <cols_num> C, CSI <cols_num> D - cursor RIGHT/LEFT on amount <cols_num>
#define MOVE_CURSOR_BEGIN_NEXT_LINE 6 //CSI <lines_num> E - move <lines_num> lines down and to the beginning of the line
#define MOVE_CURSOR_BEGIN_PREV_LINE 7 //CSI <lines_num> F - move <lines_num> lines up and to the beginning of the line
#define MOVE_CURSOR_POS_COL 8 //CSI <col_pos> G - move cursor to <col_pos> column
#define REQ_CURSOR 9 //CSI 6 n - request cursor
#define SAVE_CURSOR 10 //CSI s - save cursor on stack
#define RESTORE_CURSOR 11 //CSI u - pop from stack and set cursor position
#define ERASE_CUR_TO_END 12 //CSI J - erase screen from current cursor position to the end
#define ERASE_START_TO_CURSOR 13 //CSI 1 J - erase screen from start position to cursor
#define ERASE_ENTIRE_BUFFER 14 //CSI 3 J - erase scrollback area
#define CLEAR_CURRENT_LINE 15 //CSI 2 K - clear current line
#define CLEAR_CUR_TO_END_OF_LINE 16 //CSI K - clear elements from cursor position to the end of line
#define CLEAR_START_TO_CURSOR_LINE 17 //CSI 1 K - clear elements from line start to cursor position
#define RESET_STYLE 18 // CSI 0 m, CSI m - reset current SGR state
#define STYLE 19 //CSI <params> m - style structure is filled, <params> ::= <n> | <n> ; <params>
#define SET_ICON_WINDOW_NAME 20 //OSC 0 ; txt \x07 - set txt as window and icon name of terminal emulator, OSC is ESC ]
#define SET_ICON_NAME 21 //OSC 1 ; txt \x07 - set txt as icon name
#define SET_WINDOW_NAME 22 //OSC 2 ; txt \x07 - set txt as window name
#define ERASE_N_CHARS_FROM_CURSOR 23 //CSI <n> X - erase <n> chars from cursor
#define ALT_BUF_ON 24 //CSI ? <47 | 1047 | 1049> h - set alt buf with specific params
#define ALT_BUF_OFF 25 //CSI ? <47 | 1047 | 1049> l - set main buf with specific params
#define HIDE_CUR 26 //CSI ? 25 l - cursor is not visible
#define SHOW_CUR 27 //CSI ? 25 h - cursor is visible
#define AUTO_WRAP_ON 28 //CSI ? 7 h - set auto wrap mode
#define AUTO_WRAP_OFF 29 //CSI ? 7 l - reset auto wrap mode
#define MOVE_CUR_ABS 30 //CSI <n> d - move cursor to absolute position in row <n>
#define DELETE_N_CHARS_RIGHT_FROM_CURSOR_WITH_SHIFT 31 //CSI <n> P - delete <n> chars right from cursor with shift
#define ROW_MARGINS 32 //CSI <top> ; <bottom> r - vertical margins set scroll area
#define INDEX 33 //CSI D - invoke index
#define REVERSE_INDEX 34 //CSI M - invoke reversed index
#define SCROLL_UP 35 //CSI <n> S - perform scroll up within scroll area on <n> rows
#define SCROLL_DOWN 36 //CSI <n> T - perform scroll down on <n> rows
#define INSERT_LINE 37 //CSI <n> L - insert <n> lines on current cursor row position with shift
#define DELETE_LINE 38 //CSI <n> M - delete <n> lines from current cursor row position with shift
#define NOT_SUPPORTED 100

//bitmask
#define B_COLOR 1
#define F_COLOR 2
#define BOLD 4
#define ITALIC 8
#define UNDERLINE 16

struct style {
    char *fColor;
    char *bColor;
    char bold, italic, underline;
    int changed;
};

struct moveCursor {
    long line;
    long column;
};

struct character {
    char c[4];
    size_t size;
    struct style s;
};

struct esc {
    int code;
    struct style s;
    struct moveCursor cursor;
    int alt_buf_clear_on_enter;
    int alt_buf_clear_on_exit;
    int margins[2];
    int scroll_num;
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

void clear_style(struct style *s);
int style_is_empty(struct style *s);
int style_equal(struct style *s1, struct style *s2);
void parse_esc(struct esc_parser *pars, char c);   //TODO: remove from header
int init_parser(struct esc_parser *parser);

#endif //WEBTERMINAL_ESC_PARSER_H
