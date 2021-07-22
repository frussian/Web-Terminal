//
// Created by Anton on 21.07.2021.
//

#include <stdio.h>
#include <string.h>
#include "esc_parser.h"
#include "../tty/tty.h"

//struct esc parseEsc(const char *, size_t, int *);

void clearStyle(struct style *s) {
    s->underline = 0;
    s->italic = 0;
    s->fColor = NULL;
    s->bold = 0;
    s->bColor = NULL;
}

int styleIsEmpty(struct style *s) {
    return !s->underline && !s->italic && !s->fColor &&
           !s->bold && !s->bColor;
}

int styleEqual(struct style *s1, struct style *s2) {
    return s1->underline == s2->underline &&
           s1->italic == s2->italic &&
           s1->bold == s2->bold &&
           s1->fColor == s2->fColor && //|| strcmp(s1->fColor, s2->fColor) == 0) &&  //TODO: check this
           s1->bColor == s2->bColor;//|| strcmp(s1->bColor, s2->bColor) == 0);

}

struct character *appendChars(const struct character *s1, int len1, const struct character *s2, int len2) {
    struct character *new = realloc(s1, sizeof(struct character) * (len1 + len2));
    if (new == NULL) return NULL;

    memcpy((new + len1), s2, sizeof(struct character) * len2);
    return new;
}

int parseTerminal(struct tty *pt) {
    fprintf(stderr, "%p %p size = %zu parsing terminal\n", (void*)pt, pt->buf, pt->size);
    char *buf = pt->buf;
    int i = pt->rawStart;
    size_t size = pt->size;

    struct style currentStyle;
    clearStyle(&currentStyle);

    size_t charsSize = 256;
    int charsNum = 0;
    struct character *chars = (struct character*) malloc(charsSize * sizeof(struct character));
    if (chars == NULL) {
        fprintf(stderr, "chars malloc error\n");
        return -1;
    }

    while (i < size) {
        fprintf(stderr, "%d\n", i);
        if (buf[i] == '\x1b' && i + 1 < size && buf[i+1] == '[') {
            i += 2;
            struct esc res = parseEsc(buf + i, size - i, &i);

            fprintf(stderr, "\x1b[32mparsing escape-seq: %d\x1b[0m\n", res.code);

            switch (res.code) {
                case ERROR:
                    fprintf(stderr, "internal parser error\n");
                    break;
                case NOT_SUPPORTED:
                    fprintf(stderr, "unsupported escape sequence\n");
                    break;
                case STYLE:
                    currentStyle = res.s;
                    break;
                case RESET_STYLE:
                    clearStyle(&currentStyle);
                    break;
            }
        } else {
            fprintf(stderr, "parsing character\n");
            struct character c;
            memcpy(c.c, &buf[i], 1);
            c.size = 1;
            c.s = currentStyle;
            chars[charsNum++] = c;

            if (charsNum >= charsSize) {
                charsSize *= 2;
                chars = realloc(chars, charsSize * sizeof(struct character));
            }

            i++;
        }
    }

    fprintf(stderr, "parsed %d\n", charsNum);

    if (charsNum != 0) {
        pt->chars = appendChars(pt->chars, pt->charSize, chars, charsNum);
        if (pt->chars == NULL) {
            fprintf(stderr, "append chars error\n");
            return -1;
        }

        pt->charSize += charsNum;//213 zeros, read 151 + 1 + 4; after newline + 256 + 82
        pt->rawStart = size; //TODO: check this
    }

    free(chars);
    return 0;
}

struct esc parseEsc(const char *buf, size_t maxsize, int *i) {
    int state = 0;
    int j = 0;
    struct esc res;
    res.code = ERROR;
    res.s.bColor = NULL;
    res.s.fColor = NULL;
    res.s.bold = 0;
    res.s.italic = 0;
    res.s.underline = 0;
    res.cursor.column = 0;
    res.cursor.line = 0;
    int flag = 0;

    if (maxsize <= 0) return res;

    short maxDigits = 6;
    short maxDigitLen = 10;
    char digits[maxDigits][maxDigitLen + 1];
    short digitsNum = 0;
    short currentDigitPos = 0;

    while (j < maxsize) {
        switch (state) {
            case 0:  //start
                if (buf[j] == 'J') {
                    res.code = ERASE_CUR_TO_END;
                    flag = 1;
                } else if (buf[j] >= '0' && buf[j] <= '9') {
                    state = 1;
                    break;
                } else if (buf[j] == '=') {
                    state = 2;
                } else if (buf[j] == '?') {
                    state = 3;
                } else if (buf[j] == 'H') {
                    res.code = MOVE_CURSOR_HOME;
                    flag = 1;
                } else if (buf[j] == 's') {
                    res.code = SAVE_CURSOR;
                    flag = 1;
                } else if (buf[j] == 'u') {
                    res.code = RESTORE_CURSOR;
                    flag = 1;
                } else if (buf[j] == 'K') {
                    res.code = CLEAR_CUR_TO_END_OF_LINE;
                    flag = 1;
                } else {
                    state = 1;  //for move cursor without parameters
                    break;
                }

                j++;
                break;

            case 1:  //digits: cursor position || erase functions || colors
                fprintf(stderr, "parsing case 1, buf[j] %c\n", buf[j]);
                if (buf[j] >= '0' && buf[j] <= '9') {
                    fprintf(stderr, "parsing digit\n");
                    if (digitsNum >= 6) {
                        fprintf(stderr, "error: digitsNum >= 6\n");
                    }

                    digits[digitsNum][currentDigitPos] = buf[j];
                    currentDigitPos++;
                } else if (buf[j] == ';') {
                    if (currentDigitPos == 0) {
                        digits[digitsNum][0] = '0';
                        currentDigitPos++;
                    }

                    digits[digitsNum][currentDigitPos] = 0;
                    digitsNum++;
                    currentDigitPos = 0;
                } else {
                    fprintf(stderr, "parsing else\n");
                    if (currentDigitPos != 0) {
                        digits[digitsNum][currentDigitPos] = 0;
                        digitsNum++;
                        fprintf(stderr, "digitsNum = %d\n", digitsNum);
                    }

                    switch (buf[j]) {
                        case 'f':
                        case 'H':
                            res.code = MOVE_CURSOR_LINE_COLUMN;
                            if (digitsNum > 0) {
                                long line = strtol(digits[0], NULL, 10);
                                res.cursor.line = line;
                            }
                            if (digitsNum > 1) {
                                long column = strtol(digits[1], NULL, 10);
                                res.cursor.column = column;
                            }

                            flag = 1;
                            break;

                        case 'A':  //up
                            res.code = MOVE_CURSOR_LINE;
                            if (digitsNum > 0) {
                                long line = strtol(digits[0], NULL, 10);
                                res.cursor.line = -line;
                            } else {
                                res.cursor.line = -1;
                            }

                            flag = 1;
                            break;
                        case 'B':  //down
                            res.code = MOVE_CURSOR_LINE;
                            if (digitsNum > 0) {
                                long line = strtol(digits[0], NULL, 10);
                                res.cursor.line = line;
                            } else {
                                res.cursor.line = 1;
                            }

                            flag = 1;
                            break;
                        case 'C':  //right
                            res.code = MOVE_CURSOR_COLUMN;
                            if (digitsNum > 0) {
                                long col = strtol(digits[0], NULL, 10);
                                res.cursor.column = col;
                            } else {
                                res.cursor.column = 1;
                            }
                            flag = 1;
                            break;
                        case 'D':  //left
                            res.code = MOVE_CURSOR_COLUMN;
                            if (digitsNum > 0) {
                                long col = strtol(digits[0], NULL, 10);
                                res.cursor.column = -col;
                            } else {
                                res.cursor.column = -1;
                            }

                            flag = 1;
                            break;
                        case 'E':
                            res.code = MOVE_CURSOR_BEGIN_NEXT_LINE;
                            if (digitsNum > 0) {
                                long line = strtol(digits[0], NULL, 10);
                                res.cursor.line = line;
                            } else {
                                res.cursor.line = 1;
                            }

                            flag = 1;
                            break;
                        case 'F':
                            res.code = MOVE_CURSOR_BEGIN_PREV_LINE;
                            if (digitsNum > 0) {
                                long line = strtol(digits[0], NULL, 10);
                                res.cursor.line = -line;
                            } else {
                                res.cursor.line = -1;
                            }

                            flag = 1;
                            break;
                        case 'G':
                            res.code = MOVE_CURSOR_POS_COL;
                            if (digitsNum > 0) {
                                long col = strtol(digits[0], NULL, 10);
                                res.cursor.column = -col;
                            } else {
                                res.cursor.column = -1;
                            }

                            flag = 1;
                            break;
                        case 'J':
                            if (digitsNum == 0) {
                                res.code = ERROR;
                            } else if (strcmp(digits[0], "1") == 0) {
                                res.code = ERASE_START_TO_CURSOR;
                            } else if (strcmp(digits[0], "2") == 0) {
                                res.code = ERASE_VISIBLE_SCREEN;
                            } else if (strcmp(digits[0], "3") == 0) {
                                res.code = ERASE_ENTIRE_BUFFER;
                            }

                            flag = 1;
                            break;

                        case 'K':
                            if (digitsNum == 0) {
                                res.code = ERROR;
                            } else if (strcmp(digits[0], "1") == 0) {
                                res.code = CLEAR_START_TO_CURSOR_LINE;
                            } else if (strcmp(digits[0], "2") == 0) {
                                res.code = CLEAR_CURRENT_LINE;
                            }

                            flag = 1;
                            break;

                        case 'm':

                            if ((digitsNum == 3 || digitsNum == 5) &&
                                (strcmp(digits[0], "38") == 0 || strcmp(digits[0], "48") == 0) &&
                                (strcmp(digits[1], "2") == 0 || strcmp(digits[1], "5") == 0)) {
                                res.code = NOT_SUPPORTED;
                                flag = 1;  //not supported
                                break;
                            }

                            for (int n = 0; n < digitsNum; n++) {
                                if (strcmp(digits[n], "0") == 0) {
                                    res.code = RESET_STYLE;
                                    break;
                                } else if (strcmp(digits[n], "1") == 0) {
                                    res.code = STYLE;
                                    res.s.bold = 1;
                                } else if (strcmp(digits[n], "3") == 0) {
                                    res.code = STYLE;
                                    res.s.italic = 1;
                                } else if (strcmp(digits[n], "4") == 0) {
                                    res.code = STYLE;
                                    res.s.underline = 1;
                                } else if (strcmp(digits[n], "30") == 0) {
                                    res.code = STYLE;
                                    res.s.fColor = "black";
                                } else if (strcmp(digits[n], "31") == 0) {
                                    res.code = STYLE;
                                    res.s.fColor = "red";
                                } else if (strcmp(digits[n], "32") == 0) {
                                    res.code = STYLE;
                                    res.s.fColor = "green";
                                } else if (strcmp(digits[n], "33") == 0) {
                                    res.code = STYLE;
                                    res.s.fColor = "yellow";
                                } else if (strcmp(digits[n], "34") == 0) {
                                    res.code = STYLE;
                                    res.s.fColor = "blue";
                                } else if (strcmp(digits[n], "35") == 0) {
                                    res.code = STYLE;
                                    res.s.fColor = "magenta";
                                } else if (strcmp(digits[n], "36") == 0) {
                                    res.code = STYLE;
                                    res.s.fColor = "cyan";
                                } else if (strcmp(digits[n], "37") == 0) {
                                    res.code = STYLE;
                                    res.s.fColor = "white";
                                } else if (strcmp(digits[n], "40") == 0) {
                                    res.code = STYLE;
                                    res.s.bColor = "black";
                                } else if (strcmp(digits[n], "41") == 0) {
                                    res.code = STYLE;
                                    res.s.bColor = "red";
                                } else if (strcmp(digits[n], "42") == 0) {
                                    res.code = STYLE;
                                    res.s.bColor = "green";
                                } else if (strcmp(digits[n], "43") == 0) {
                                    res.code = STYLE;
                                    res.s.bColor = "yellow";
                                } else if (strcmp(digits[n], "44") == 0) {
                                    res.code = STYLE;
                                    res.s.bColor = "blue";
                                } else if (strcmp(digits[n], "45") == 0) {
                                    res.code = STYLE;
                                    res.s.bColor = "magenta";
                                } else if (strcmp(digits[n], "46") == 0) {
                                    res.code = STYLE;
                                    res.s.bColor = "cyan";
                                } else if (strcmp(digits[n], "47") == 0) {
                                    res.code = STYLE;
                                    res.s.bColor = "white";
                                }
                            }

                            flag = 1;
                            break;
                        default:
                            res.code = ERROR;
                            flag = 1;
                            break;
                    }
                }

                j++;
                break;

            case 2:  //set mode
                res.code = NOT_SUPPORTED;
                flag = 1;
                break;

            case 3:  //common private modes
                res.code = NOT_SUPPORTED;
                flag = 1;
                break;
        }

        if (flag) break;
    }

    if (i) *i += j;

    return res;
}