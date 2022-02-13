//
// Created by Anton on 28.11.2021.
//

#ifndef WEBTERMINAL_EDITOR_H
#define WEBTERMINAL_EDITOR_H
#include <esc_parser.h>

struct editor_config {
    int irm;            //0 replace, 1 - insert
    int auto_wrap;
	int visible_cur;
};

struct screen {
    struct character **rows;
    int cx, cy;
    int bottom_margin; //scroll region
    int top_margin;
};

struct editor {
    struct editor_config conf;
    struct screen screens[2];
    int alt_buf;
    size_t rows_num;
    size_t cols_num;
    int showed_cur;
    struct style curr_style;
    int pending_wrap;
};

int init_editor(struct editor *ed, size_t rows, size_t cols);
void free_editor(struct editor *ed);
void add_char(struct editor *ed, struct character c);
char *get_html(struct editor *ed, int *len);

//accepts terminal coordinates (home - 1;1)
void set_cx(struct editor *ed, int cx);
void set_cy(struct editor *ed, int cy);
void set_cx_cy(struct editor *ed, int cx, int cy);
void add_cx(struct editor *ed, int offset);
void add_cy(struct editor *ed, int offset);
void set_row_margins(struct editor *ed, int top, int bottom);

void check_index(struct editor *ed);
void check_reverse_index(struct editor *ed);
void scroll_up_screen(struct editor *ed, size_t n);
void scroll_down_screen(struct editor *ed, size_t n);
void delete_lines(struct editor *ed, size_t n);
void insert_lines(struct editor *ed, int n);

void erase_visible_screen(struct editor *ed);
void erase_n_chars_from_screen(struct editor *ed, size_t n);
void clear_cur_to_end_of_line(struct editor *ed);
void clear_cur_line(struct editor *ed);
void clear_start_to_cur_line(struct editor *ed);
void delete_n_chars_right_from_cursor_with_shift(struct editor *ed, int n);
void set_alt_buf(struct editor *ed, int alt_buf, int clear);
void show_cur(struct editor *ed, int show);
void set_auto_wrap(struct editor *ed, int auto_wrap);
void update_style(struct editor *ed, struct style s);

void dump_editor(struct editor *ed);
#endif //WEBTERMINAL_EDITOR_H
