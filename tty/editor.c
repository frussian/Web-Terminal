//
// Created by Anton on 28.11.2021.
//

#include <editor.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void init_editor(struct editor *ed) {
    ed->rows = NULL;
    ed->rows_num = 0;
    ed->conf.auto_wrap = 0;
    ed->conf.irm = 0;
    ed->cx = 0;
    ed->cy = 0;
}

void inc_line(struct editor *ed) {
    ed->cy++;
}

void add_row(struct editor *ed);

void fill_spaces(struct editor *ed, int pos, size_t size) {
    struct character c;
    c.c[0] = ' ';
    c.size = 1;
    for (int i = pos; i < pos + size; i++) {
        ed->rows[ed->cy].chars[i] = c;
    }
}

void insert_char(struct editor *ed, struct character c) {
    int cx = ed->cx;
    int cy = ed->cy;

    if (cy > ed->rows_num) {
        fprintf(stderr, "error cy > rows_num");
        return;
    }

    if (cy == ed->rows_num) {
        add_row(ed);
    }

    struct char_row *row = ed->rows + cy;
    if (cx >= row->row_size) {
        size_t old_size = row->row_size;
        row->row_size = cx + 1;
        row->chars = realloc(row->chars, sizeof(struct character) * row->row_size);
        fill_spaces(ed, old_size, cx - old_size);
    }
    row->chars[cx] = c;
}

void init_row(struct char_row* r) {
    r->chars = NULL;
    r->row_size = 0;
}

void add_row(struct editor *ed) {
    ed->rows_num++;
    ed->rows = realloc(ed->rows, sizeof(struct char_row) * ed->rows_num);
    if (ed->rows == NULL) {
        fprintf(stderr, "error with realloc\n");
    }
    init_row(ed->rows + ed->rows_num - 1);
}

void add_char(struct editor *ed, struct character c) {
    //todo: check \r
    if (c.size == 1 && c.c[0] == '\n') {
        ed->cy++;
        return;
    }

    insert_char(ed, c);
}

char *append(char* s1, int len1, const char *s2, int len2) {
    char *new = realloc(s1, len1 + len2);
    if (new == NULL) return NULL;

    memcpy(new + len1, s2, len2);
    return new;
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
    html = append(html, 0, "<p>", 3);
    if (html == NULL) {
        fprintf(stderr, "html is null\n");
        return NULL;
    }
    sum += 3;

    struct style s;
    char *styleStr = NULL;
    int styleStrLen = 0;
    clearStyle(&s);

    while (i < pt->charSize) {
        struct character c = pt->chars[i];
        if (*c.c == '\n') {
            html = append(html, sum, "</p><p>", 7);
            sum += 7;
            i++;
        } else {
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

            i++;
        }
    }

    if (i == 0 || *pt->chars[i-1].c != '\n') {
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

