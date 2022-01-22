//
// Created by Anton on 28.11.2021.
//

#include <editor.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tools.h>

void init_editor(struct editor *ed) {
    ed->rows = NULL;
    ed->rows_num = 0;
    ed->conf.auto_wrap = 0;
    ed->conf.irm = 0;
    ed->cx = 0;
    ed->cy = 0;
    ed->max_rows = 80;
    ed->max_cols = 24;
}

void set_cx(struct editor *ed, int cx) {
    if (cx == 0) return;
    ed->cx = cx - 1;
}

void set_cy(struct editor *ed, int cy) {
    if (cy == 0) return;
    ed->cy = cy - 1;
}

void set_cx_cy(struct editor *ed, int cx, int cy) {
    if (cx != 0) ed->cx = cx - 1;
    if (cy != 0) ed->cy = cy - 1;
}

void add_cx(struct editor *ed, int offset) {
    ed->cx += offset;
    if (ed->cx < 0) {
        ed->cx = 0;
    }

    //todo: check right margin
}

void add_cy(struct editor *ed, int offset) {
    ed->cy += offset;
    if (ed->cy < 0) {
        ed->cy = 0;
    }

    //todo: check bottom margin
}

void fill_spaces(struct editor *ed, int pos, size_t size) {
    struct character c;
    c.c[0] = ' ';
    c.size = 1;
    clearStyle(&c.s);
    size_t max = ed->rows[ed->cy].row_size;
    for (int i = pos; i < max && i < pos + size; i++) {
        ed->rows[ed->cy].chars[i] = c;
    }
}

void erase_visible_screen(struct editor *ed) {
    for (int y = 0; y < ed->rows_num; y++) {
        ed->cy = y;
        fill_spaces(ed, 0, ed->rows[y].row_size);
    }
    ed->cy = 0;
    ed->cx = 0;
}

void init_row(struct char_row* r) {
    r->chars = NULL;
    r->row_size = 0;
}

void add_rows(struct editor *ed, size_t num) {
    ed->rows = realloc(ed->rows, sizeof(struct char_row) * (ed->rows_num + num));
    if (ed->rows == NULL) {
        fprintf(stderr, "error with realloc ed->rows\n");
        return;
    }
    int old_size = ed->rows_num;
    ed->rows_num += num;

    for (int i = old_size; i < ed->rows_num; i++) {
        init_row(ed->rows + i);
    }
}

void insert_char(struct editor *ed, struct character c) {
    int cx = ed->cx;
    int cy = ed->cy;

    if (cy >= ed->rows_num) {
        fprintf(stderr, "cy >= rows_num");
        add_rows(ed, cy - ed->rows_num + 1);
    }

//    fprintf(stderr, "char = %d cx = %d cy = %d\n", c.c[0], cx, cy);

    struct char_row *row = ed->rows + cy;
    if (cx >= row->row_size) {
        size_t old_size = row->row_size;
        row->row_size = cx + 1;
        row->chars = realloc(row->chars, sizeof(struct character) * row->row_size);
        if (row->chars == NULL) {
            fprintf(stderr, "realloc problem row->chars\n");
            return;
        }
        fill_spaces(ed, old_size, cx - old_size);
    }
    row->chars[cx] = c;
    ed->cx++;
}

void add_char(struct editor *ed, struct character c) {
    //todo: check \r
    if (c.size == 1 && c.c[0] == '\n') {
        ed->cy++;
        ed->cx = 0;
        return;
    }

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

    for (i = 0; i < ed->rows_num; i++) {
        html = append(html, sum, "<p>", 3);
        sum += 3;

        struct char_row *row = ed->rows + i;
        for (int j = 0; j < row->row_size; j++) {
            struct character c = row->chars[j];
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

