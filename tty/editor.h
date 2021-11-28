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
};

struct editor {
    struct char_row *rows;
    size_t rows_num;
    struct editor_config conf;
    int cx, cy;
};

void init_editor(struct editor *ed);
void add_char(struct editor *ed, struct character c);
char *getHTML(struct editor *ed, int *len);



#endif //WEBTERMINAL_EDITOR_H
