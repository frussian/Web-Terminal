//
// Created by Anton on 28.11.2021.
//

#include <editor.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tools.h>

int init_screen(size_t rows_num, size_t cols_num, struct screen *scr) {
    struct character **rows = malloc(rows_num * sizeof(struct character*));
    if (rows == NULL) {
        return -1;
    }

    struct character c;
    c.c[0] = ' ';
    c.size = 1;
    clearStyle(&c.s);

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
    if (err == 0) return 0;

    for (int i = 0; i < row_num; i++) {
        free(rows[i]);
    }
    free(rows);
    return -1;
}

int init_editor(struct editor *ed) {
    ed->rows_num = 24;
    ed->cols_num = 80;
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
    return 0;
}

void set_cx(struct editor *ed, int cx) {
    if (cx == 0) return;
    ed->screens[ed->alt_buf].cx = cx - 1;
}

void set_cy(struct editor *ed, int cy) {
    if (cy == 0) cy = 1;
    if (cy >= ed->rows_num) {
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
    ed->screens[ed->alt_buf].cx += offset;
    if (ed->screens[ed->alt_buf].cx < 0) {
        ed->screens[ed->alt_buf].cx = 0;
    }

    //todo: check right margin
}

void add_cy(struct editor *ed, int offset) {
    ed->screens[ed->alt_buf].cy += offset;
    if (ed->screens[ed->alt_buf].cy < 0) {
        ed->screens[ed->alt_buf].cy = 0;
    }

    //todo: check bottom margin
}

void fill_spaces(struct editor *ed, int pos, size_t size) {
    struct character c;
    c.c[0] = ' ';
    c.size = 1;
    clearStyle(&c.s);
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

void set_alt_buf(struct editor *ed, int alt_buf, int clear) {
    if (alt_buf) {
        ed->alt_buf = 1;
        if (clear) erase_visible_screen(ed);
    } else {
        if (clear) erase_visible_screen(ed);
        ed->alt_buf = 0;
    }
}

void init_row(struct char_row* r) {
    r->chars = NULL;
    r->row_size = 0;
}

void scroll_screen(struct editor *ed) {
    struct screen *scr = &ed->screens[ed->alt_buf];
    free(scr->rows[0]);
    for (int i = 0; i < ed->rows_num - 1; i++) {
        scr->rows[i] = scr->rows[i+1];
    }
    scr->rows[ed->rows_num-1] = malloc(ed->cols_num * sizeof(struct character));
    scr->cy--;
    fill_spaces(ed, 0, ed->cols_num);
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
    int cx = scr->cx;
    int cy = scr->cy;

//    fprintf(stderr, "char = %d cx = %d cy = %d\n", c.c[0], cx, cy);

    if (ed->conf.irm) {
        irm_insert_char(ed, c);
    } else {
        irm_replace_char(ed, c);
    }

    if (cx == ed->cols_num) {
        if (ed->conf.auto_wrap) {//TODO: на самом деле что-то посложнее
            scr->cy++;
            scr->cx = 0;
            if (scr->cy == ed->rows_num) {
                scroll_screen(ed);
            }
        } else {
            scr->cx--;
        }
    }
}

void add_char(struct editor *ed, struct character c) {
    //todo: check \r
    struct screen *scr = &ed->screens[ed->alt_buf];
    if (scr->cx >= ed->cols_num) {
        scr->cx = ed->cols_num-1;
    }
    if (scr->cy >= ed->rows_num) {
        scr->cy = ed->rows_num-1;
    }

    if (c.size == 1 && c.c[0] == '\n') {
        scr->cy++;
        scr->cx = 0;
        if (scr->cy == ed->rows_num) {
            scroll_screen(ed);
        }
        return;
    } else if (c.size == 1 && c.c[0] == '\x08') {
        scr->cx--;
        return;
    } else if (c.size == 1 && c.c[0] == '\x07') {
        //TODO: play sound
        return;
    } else if (c.size == 1 && c.c[0] == '\r') {
        return;
    }
    printf("adding char %c %d\n", c.c[0], *(int*)c.c);
    insert_char(ed, c);
}

char *generateStyleStr(struct style *s, int *num) {
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

char *getHTML(struct editor *ed, int *len) {
    /*if (!pt->changed) {
        *len = 10;
        printf("no changes\n");
        return "no changes";
    }
    */

    char *html = NULL;
    int sum = 0;
    int i = 0;

    struct style s;
    char *styleStr = NULL;
    int styleStrLen = 0;
    clearStyle(&s);

    struct screen *scr = &ed->screens[ed->alt_buf];

    for (i = 0; i < ed->rows_num; i++) {
        html = append(html, sum, "<p>", 3);
        sum += 3;

        struct character *row = scr->rows[i];
        for (int j = 0; j < ed->cols_num; j++) {
            struct character c = row[j];
            if (i == scr->cy && j == scr->cx && ed->conf.visible_cur) {
                c.s.underline = 1;
            }
//            fprintf(stderr, "char is %c\n", c.c[0]);
            if (!styleEqual(&c.s, &s)) {
                if (styleStr) {
                    html = append(html, sum, "</span>", 7);
                    sum += 7;
                }

                styleStrLen = 0;

                s = c.s;

                if (!styleIsEmpty(&c.s)) {
                    styleStr = generateStyleStr(&c.s, &styleStrLen);
                    if (styleStr < 0) {
                        fprintf(stderr, "\x1b[32mnum is negative\x1b[0m\n");
                        styleStr = 0;
                    }
                }

                if (styleStr) {
                    html = append(html, sum, styleStr, styleStrLen);
                    sum += styleStrLen;
                    //free(styleStr);
                }

            }

            html = append(html, sum, c.c, c.size);
            sum += c.size;

            if (*c.c == 0) {
                fprintf(stderr, "found 0 in pt.buf\n");
            }
        }

        html = append(html, sum, "</p>", 4);
        sum += 4;
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

