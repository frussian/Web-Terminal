//
// Created by Anton on 28.11.2021.
//

#include <editor.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <tools.h>

int init_screen(size_t rows_num, size_t cols_num, struct screen *scr) {
    struct character **rows = malloc(rows_num * sizeof(struct character*));
    if (rows == NULL) {
        return -1;
    }

    struct character c;
    c.c[0] = ' ';
    c.size = 1;
    clear_style(&c.s);

    int err = 0;
    int row_num;
    for (row_num = 0; row_num < rows_num; row_num++) {
        struct character *row = malloc(cols_num * sizeof(struct character));
        if (row == NULL) {
            err = 1;
            break;
        }
        for (int col = 0; col < cols_num; col++) {
            row[col] = c;
        }
        rows[row_num] = row;
    }

    scr->cx = 0;
    scr->cy = 0;
    scr->rows = rows;
    scr->top_margin = 0;
    scr->bottom_margin = rows_num-1;
    if (err == 0) return 0;

    for (int i = 0; i < row_num; i++) {
        free(rows[i]);
    }
    free(rows);
    return -1;
}

int init_editor(struct editor *ed, size_t rows, size_t cols) {
    ed->rows_num = rows;
    ed->cols_num = cols;
    int res = init_screen(ed->rows_num, ed->cols_num, &ed->screens[0]);
    if (res < 0) {
        return res;
    }
    res = init_screen(ed->rows_num, ed->cols_num, &ed->screens[1]);
    if (res < 0) {
        return res;
    }
    ed->conf.auto_wrap = 0;
    ed->conf.irm = 0;
	ed->conf.visible_cur = 1;
    ed->alt_buf = 0;
    ed->showed_cur = 0;
    ed->pending_wrap = 0;
    clear_style(&ed->curr_style);
    return 0;
}

void set_cx(struct editor *ed, int cx) {
    if (cx == 0) cx = 1;
    if (cx > ed->cols_num) {
        cx = ed->cols_num;
    }
    if (cx == 80) {
        dump_editor(ed);
    }
    ed->screens[ed->alt_buf].cx = cx - 1;
}

void set_cy(struct editor *ed, int cy) {
    if (cy == 0) cy = 1;
    if (cy > ed->rows_num) {
        cy = ed->rows_num;
    }
    ed->screens[ed->alt_buf].cy = cy - 1;
}

void set_cx_cy(struct editor *ed, int cx, int cy) {
    set_cx(ed, cx);
    set_cy(ed, cy);
//    if (cx != 0) ed->screens[ed->alt_buf].cx = cx - 1;
//    if (cy != 0) ed->screens[ed->alt_buf].cy = cy - 1;
}

void add_cx(struct editor *ed, int offset) {
    struct screen *scr = &ed->screens[ed->alt_buf];
    scr->cx += offset;
    if (scr->cx < 0) {
        scr->cx = 0;
    }
    if (scr->cx >= ed->cols_num) {
        scr->cx = ed->cols_num - 1;
    }
    //todo: check right margin
}

void add_cy(struct editor *ed, int offset) {
    struct screen *scr = &ed->screens[ed->alt_buf];
    scr->cy += offset;
    if (scr->cy < 0) {
        scr->cy = 0;
    }
    if (scr->cy >= ed->rows_num) {
        scr->cy = ed->rows_num - 1;
    }

}

void set_row_margins(struct editor *ed, int top, int bottom) {
    struct screen *scr = &ed->screens[ed->alt_buf];

    if (bottom == 0 || bottom >= ed->rows_num) {
        bottom = ed->rows_num - 1;
    }
    if (top == 0) {
        top = 1;
    }
    if (top < bottom && bottom < ed->cols_num) {
        scr->top_margin = top-1;
        scr->bottom_margin = bottom-1;
    } else {
        scr->top_margin = 0;
        scr->bottom_margin = ed->rows_num-1;
    }
}

void check_index(struct editor *ed) {
    struct screen *scr = &ed->screens[ed->alt_buf];
    if (scr->cy != scr->bottom_margin) {
        scr->cy++;
        if (scr->cy >= ed->rows_num) {
            scr->cy = ed->rows_num-1;
        }
        return;
    }

    scroll_up_screen(ed, 1);
}

void check_reverse_index(struct editor *ed) {
    struct screen *scr = &ed->screens[ed->alt_buf];
    if (scr->cy != scr->top_margin) {
        scr->cy--;
        scr->cy = scr->cy >= 0 ? scr->cy : 0;
        return;
    }

    scroll_down_screen(ed, 1);
}

void fill_spaces(struct editor *ed, int pos, size_t size) {
    struct character c;
    c.c[0] = ' ';
    c.size = 1;
    c.s = ed->curr_style;
    struct screen *scr = &ed->screens[ed->alt_buf];
    for (int i = pos; i < ed->cols_num && i < pos + size; i++) {
        scr->rows[scr->cy][i] = c;
    }
}

void erase_visible_screen(struct editor *ed) {
    struct screen *scr = &ed->screens[ed->alt_buf];
    for (int y = 0; y < ed->rows_num; y++) {
        scr->cy = y;
        fill_spaces(ed, 0, ed->cols_num);
    }
    scr->cy = 0;
    scr->cx = 0;
}

void erase_n_chars_from_screen(struct editor *ed, size_t n) {
    struct screen *scr = &ed->screens[ed->alt_buf];
    fill_spaces(ed, scr->cx, n);
}

void clear_cur_to_end_of_line(struct editor *ed) {
    struct screen *scr = &ed->screens[ed->alt_buf];
    fill_spaces(ed, scr->cx, ed->cols_num - scr->cx);
}

void clear_cur_line(struct editor *ed) {
    fill_spaces(ed, 0, ed->cols_num);
}

void clear_start_to_cur_line(struct editor *ed) {
    struct screen *scr = &ed->screens[ed->alt_buf];
    fill_spaces(ed, 0, scr->cx+1);
}

void delete_n_chars_right_from_cursor_with_shift(struct editor *ed, int n) {
    struct screen *scr = &ed->screens[ed->alt_buf];
    int cy = scr->cy;
    int cx = scr->cx;
    for (int i = cx + n; i < ed->cols_num; i++) {
        scr->rows[cy][i - n] = scr->rows[cy][i];
    }
    fill_spaces(ed, ed->cols_num - n, n);
}

void set_alt_buf(struct editor *ed, int alt_buf, int clear) {
    struct screen *scr = &ed->screens[ed->alt_buf];
    scr->top_margin = 0;
    scr->bottom_margin = ed->rows_num-1;
    if (alt_buf) {
        ed->alt_buf = 1;
        if (clear) erase_visible_screen(ed);
    } else {
        if (clear) erase_visible_screen(ed);
        ed->alt_buf = 0;
    }
}

void show_cur(struct editor *ed, int show) {
    ed->conf.visible_cur = show;
}

void set_auto_wrap(struct editor *ed, int auto_wrap) {
    ed->conf.auto_wrap = auto_wrap;
}

void update_style(struct editor *ed, struct style s) {
    ed->curr_style = s;
}

void dump_screen(struct editor *ed, int n, FILE *file) {
    struct screen *scr = &ed->screens[n];
    struct style curr_style = scr->rows[0][0].s;

    fprintf(file, "cx = %d, cy = %d\ntop margin = %d, bottom margin = %d\n", scr->cx, scr->cy, scr->top_margin, scr->bottom_margin);

    for (int i = 0; i < ed->rows_num; i++) {
        for (int j = 0; j < ed->cols_num; j++) {
            struct character c = scr->rows[i][j];
            if (!style_equal(&curr_style, &c.s)) {
                fprintf(file, " style {back %s, fore %s, bold %d, italic %d, underline %d} ",
                        curr_style.bColor, curr_style.fColor,
                        curr_style.bold, curr_style.italic, curr_style.underline);
                curr_style = c.s;
            }
            if (c.size > 1) {
                fprintf(file, "{");
                for (int k = 0; k < c.size; k++) {
                    fprintf(file, "%u", (unsigned char)c.c[k]);
                    if (k == c.size - 1) {
                        fprintf(file, "}");
                    } else {
                        fprintf(file, " ");
                    }
                }
                fprintf(file, "(%zu)", c.size);
            } else {
                fprintf(file, "%c", c.c[0]);
            }
        }
        fprintf(file, "\n");
    }
    fprintf(file, "\n");
}

void dump_editor(struct editor *ed) {
    FILE *file = stdout;
    fprintf(stderr, "editor:\nrows = %zu columns = %zu\naltbuf %d\n",
            ed->rows_num, ed->cols_num, ed->alt_buf);
    fprintf(stderr, "config:\nauto_wrap %d, irm %d, visible cursor %d\n", ed->conf.auto_wrap, ed->conf.irm, ed->conf.visible_cur);
    fprintf(file, "main screen:\n");
    dump_screen(ed, 0, file);
    fprintf(file, "alternate screen:\n");
    dump_screen(ed, 1, file);
}

void scroll_up_from_pos_screen(struct editor *ed, int pos, size_t n) {
    struct screen *scr = &ed->screens[ed->alt_buf];
    int cy = scr->cy;
    printf("scrolling up %d %d\n", scr->top_margin, scr->bottom_margin);
    for (int i = pos; i < n + pos; i++) {
        free(scr->rows[i]);
    }
    for (int i = pos; i <= scr->bottom_margin - n; i++) {
        printf("swapping %d <- %d\n", i, i+n);
        scr->rows[i] = scr->rows[i+n];
    }
    for (int i = scr->bottom_margin-n+1; i <= scr->bottom_margin; i++) {
        scr->cy = i;
        scr->rows[i] = malloc(ed->cols_num * sizeof(struct character));
        fill_spaces(ed, 0, ed->cols_num);
    }
    scr->cy = cy;
}

void delete_lines(struct editor *ed, size_t n) {
    struct screen *scr = &ed->screens[ed->alt_buf];
    if (scr->cy < scr->top_margin || scr->cy > scr->bottom_margin) {
        return;
    }
    if (scr->bottom_margin - scr->cy + 1 < n) {
        n = scr->bottom_margin - scr->cy + 1;
    }
    scroll_up_from_pos_screen(ed, scr->cy, n);
}

void scroll_up_screen(struct editor *ed, size_t n) {
    struct screen *scr = &ed->screens[ed->alt_buf];
    scroll_up_from_pos_screen(ed, scr->top_margin, n);
}

void scroll_down_from_pos_screen(struct editor *ed, int pos, size_t n) {
    struct screen *scr = &ed->screens[ed->alt_buf];
    int cy = scr->cy;
    printf("scrolling down %d %d\n", scr->top_margin, scr->bottom_margin);
    for (int i = scr->bottom_margin-n+1; i <= scr->bottom_margin; i++) {
        free(scr->rows[i]);
    }
    for (int i = scr->bottom_margin; i >= pos + n; i--) {
        printf("swapping %d <- %d\n", i, i-n);
        scr->rows[i] = scr->rows[i-n];
    }

    for (int i = pos; i < pos + n; i++) {
        scr->cy = i;
        scr->rows[i] = malloc(ed->cols_num * sizeof(struct character));
        fill_spaces(ed, 0, ed->cols_num);
    }
    scr->cy = cy;
}

void insert_lines(struct editor *ed, int n) {
    struct screen *scr = &ed->screens[ed->alt_buf];
    if (scr->cy < scr->top_margin || scr->cy > scr->bottom_margin) {
        return;
    }
    if (scr->bottom_margin - scr->cy + 1 < n) {
        n = scr->bottom_margin - scr->cy + 1;
    }
    scroll_down_from_pos_screen(ed, scr->cy, n);
}

void scroll_down_screen(struct editor *ed, size_t n) {
    struct screen *scr = &ed->screens[ed->alt_buf];
    scroll_down_from_pos_screen(ed, scr->top_margin, n);
}

void irm_insert_char(struct editor *ed, struct character c) {
    struct screen *scr = &ed->screens[ed->alt_buf];
    int cx = scr->cx;
    for (int i = ed->cols_num - 1; i > cx; i++) {
        scr->rows[scr->cy][i] = scr->rows[scr->cy][i-1];
    }
    scr->rows[scr->cy][cx] = c;
    scr->cx++;
}

void irm_replace_char(struct editor *ed, struct character c) {
    struct screen *scr = &ed->screens[ed->alt_buf];
    scr->rows[scr->cy][scr->cx] = c;
    scr->cx++;
}

void insert_char(struct editor *ed, struct character c) {
    struct screen *scr = &ed->screens[ed->alt_buf];

    if (scr->cx == ed->cols_num - 1 && ed->pending_wrap && ed->conf.auto_wrap) {
        scr->cx = 0;
        check_index(ed);
    }
    ed->pending_wrap = 0;
//    fprintf(stderr, "char = %d cx = %d cy = %d\n", c.c[0], cx, cy);

    if (ed->conf.irm) {
        irm_insert_char(ed, c);
    } else {
        irm_replace_char(ed, c);
    }

    if (scr->cx == ed->cols_num) {
        if (ed->conf.auto_wrap) {
            ed->pending_wrap = 1;
        }
        scr->cx--;
    }
}

void add_char(struct editor *ed, struct character c) {
    struct screen *scr = &ed->screens[ed->alt_buf];
    c.s = ed->curr_style;

    if (scr->cx < 0) {
        printf("correcting cx = %d", scr->cx);
        scr->cx = 0;
    }
    if (scr->cy < 0) {
        printf("correcting cy = %d", scr->cy);
        scr->cy = 0;
    }
    if (scr->cx >= ed->cols_num) {
        scr->cx = ed->cols_num-1;
    }
    if (scr->cy >= ed->rows_num) {
        scr->cy = ed->rows_num-1;
    }

    if (c.size == 1 && c.c[0] == '\n') {
        scr->cy++;
        scr->cx = 0;
        printf("newline scroll");
        if (scr->cy > scr->bottom_margin) {
            scroll_up_screen(ed, 1);
        }
        return;
    } else if (c.size == 1 && c.c[0] == '\x08') {
        scr->cx--;
        return;
    } else if (c.size == 1 && c.c[0] == '\x07') {
        //TODO: play sound
        return;
    } else if (c.size == 1 && c.c[0] == '\r') {
        scr->cx = 0;
        return;
    }
//    printf("adding char %c %d\n", c.c[0], *(int*)c.c);
    insert_char(ed, c);
}

char *generate_style_str(struct style *s, int *num) {
    char *buf = NULL;
    int len = 0;

    buf = append(buf, 0, "<span style=\"", 13);
    len += 13;

    if (s->bColor) {
        buf = append(buf, len, "background-color:", 17);
        len += 17;

        int len2 = strlen(s->bColor);
        buf = append(buf, len, s->bColor, len2);
        len += len2;

        buf = append(buf, len, ";", 1);
        len++;
    }

    if (s->fColor) {
        buf = append(buf, len, "color:", 6);
        len += 6;

        int len2 = strlen(s->fColor);
        buf = append(buf, len, s->fColor, len2);
        len += len2;

        buf = append(buf, len, ";", 1);
        len++;
    }

    if (s->bold) {
        buf = append(buf, len, "font-weight:bold;", 17);
        len += 17;
    }

    if (s->italic) {
        buf = append(buf, len, "font-style:italic;", 18);
        len += 18;
    }

    if (s->underline) {
        buf = append(buf, len, "text-decoration:underline;", 26);
        len += 26;
    }

    if (len == 13) {
        free(buf);
        return NULL;
    }

    buf = append(buf, len, "\">", 2);
    len += 2;

    *num = len;

    return buf;
}

char *get_html(struct editor *ed, int *len) {
    char *html = NULL;
    int sum = 0;
    int i = 0;

    struct style s;
    char *style_str = NULL;
    int style_strLen = 0;
    clear_style(&s);

    struct screen *scr = &ed->screens[ed->alt_buf];

    for (i = 0; i < ed->rows_num; i++) {
        struct character *row = scr->rows[i];
        for (int j = 0; j < ed->cols_num; j++) {
            struct character c = row[j];
            if (i == scr->cy && j == scr->cx && ed->conf.visible_cur && !ed->showed_cur) {
//                printf("cursor: %d %d\n", scr->cx, scr->cy);
                if (!ed->showed_cur) {
                    c.s.underline = 1;
                    ed->showed_cur = 1;
                } else {
                    ed->showed_cur = 0;
                }
            } else if (i == scr->cy && j == scr->cx && ed->conf.visible_cur) {
                ed->showed_cur = 0;
            }
//            fprintf(stderr, "char is %c\n", c.c[0]);
            if (!style_equal(&c.s, &s)) {
                if (style_str) {
                    html = append(html, sum, "</span>", 7);
                    sum += 7;
                }

                style_strLen = 0;

                s = c.s;

                if (!style_is_empty(&c.s)) {
                    style_str = generate_style_str(&c.s, &style_strLen);
                    if (style_str < 0) {
                        fprintf(stderr, "\x1b[32mnum is negative\x1b[0m\n");
                        style_str = 0;
                    }
                }

                if (style_str) {
                    html = append(html, sum, style_str, style_strLen);
                    sum += style_strLen;
                    //free(styleStr);
                }

            }

            if (c.c[0] == '&' && c.size == 1) {
                html = append(html, sum, "&amp;", 5);
                sum += 5;
            } else if (c.c[0] == '<' && c.size == 1) {
                html = append(html, sum, "&lt;", 4);
                sum += 4;
            } else if (c.c[0] == '>' && c.size == 1) {
                html = append(html, sum, "&gt;", 4);
                sum += 4;
            } else {
                html = append(html, sum, c.c, c.size);
                sum += c.size;
            }

            if (*c.c == 0) {
                fprintf(stderr, "found 0 in pt.buf\n");
            }
        }

        html = append(html, sum, "\n", 1);
        sum++;
    }

    html = realloc(html, sum + 1);
    if (html == NULL) {
        fprintf(stderr, "html: realloc returned null\n");
        return NULL;
    }
    html[sum] = 0;

    if (len) *len = sum;
    return html;
}

