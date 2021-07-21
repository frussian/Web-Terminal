//
// Created by Anton on 21.07.2021.
//

#include <stdio.h>
#include <string.h>
#include "esc_parser.h"

struct esc parseEsc(const char *, size_t, int *);

int parseTerminal(struct tty *pt) {
    char *buf = pt->buf;
    int i = pt->rawStart;
    size_t size = pt->size;


    while (i < size) {
        if (buf[i] == '\x1b' && i + 1 < size && buf[i+1] == '[') {
            i += 2;
            struct esc res = parseEsc(buf + i, size - i, &i);

        }
    }
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
    if (res.code == NOT_SUPPORTED) {
        fprintf(stderr, "unsupported escape sequence\n");
    }

    if (i) *i += j;

    return res;
}