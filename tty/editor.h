//
// Created by Anton on 28.11.2021.
//

#ifndef WEBTERMINAL_EDITOR_H
#define WEBTERMINAL_EDITOR_H
#include <esc_parser.h>

struct char_row {
    struct character *chars;
    size_t row_size;
};

struct editor_config {
    int irm;            //0 replace, 1 - insert
    int auto_wrap;
	int visible_cur;
};

struct screen {
    struct character **rows;
    int cx, cy;
};

struct editor {
    struct editor_config conf;
    struct screen screens[2];
    int alt_buf;
    size_t rows_num;
    size_t cols_num;
};

int init_editor(struct editor *ed);
void free_editor(struct editor *ed);
void add_char(struct editor *ed, struct character c);
char *getHTML(struct editor *ed, int *len);

//accepts terminal coordinates (home - 1;1)
void set_cx(struct editor *ed, int cx);
void set_cy(struct editor *ed, int cy);
void set_cx_cy(struct editor *ed, int cx, int cy);

void add_cx(struct editor *ed, int offset);
void add_cy(struct editor *ed, int offset);
void erase_visible_screen(struct editor *ed);
void erase_n_chars_from_screen(struct editor *ed, size_t n);
void clear_cur_to_end_of_line(struct editor *ed);
void clear_cur_line(struct editor *ed);
void clear_start_to_cur_line(struct editor *ed);
void set_alt_buf(struct editor *ed, int alt_buf, int clear);
#endif //WEBTERMINAL_EDITOR_H
