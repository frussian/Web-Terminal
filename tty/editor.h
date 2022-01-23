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

};

struct editor {
    struct char_row *rows;  //TODO: fixed size buffer?
    size_t rows_num;
    struct editor_config conf;
    int cx, cy;
    size_t max_rows;
    size_t max_cols;
};

void init_editor(struct editor *ed);
void free_editor(struct editor *ed);
void add_char(struct editor *ed, struct character c);
char *getHTML(struct editor *ed, int *len);

//accepts terminal coordinates (home - 1;1)
void set_cx(struct editor *ed, int cx);
void set_cy(struct editor *ed, int cy);
void set_cx_cy(struct editor *ed, int cx, int cy);

void add_cx(struct editor *ed, int offset);
void add_cy(struct editor *ed, int offset);
void fill_spaces(struct editor *ed, int pos, size_t size);
void erase_visible_screen(struct editor *ed);

#endif //WEBTERMINAL_EDITOR_H
