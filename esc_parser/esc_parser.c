//
// Created by Anton on 21.07.2021.
//

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <esc_parser.h>
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

void reinit_parser(struct esc_parser *parser) {
    parser->state = -1;
    parser->ended = 0;

    parser->res.code = ERROR;
    parser->res.s.bColor = NULL;
    parser->res.s.fColor = NULL;
    parser->res.s.bold = 0;
    parser->res.s.italic = 0;
    parser->res.s.underline = 0;
    parser->res.cursor.column = 0;
    parser->res.cursor.line = 0;

    parser->digitsNum = 0;
    parser->currentDigitPos = 0;
    parser->maxDigits = MAX_DIGITS;
    parser->maxDigitLen = MAX_DIGITS_LEN;

}

void parseEsc(struct esc_parser *pars, char c) {

    switch (pars->state) {
        //start
        case -1: {
            if (c == '[') {
                pars->state = 0;
            } else {
                pars->ended = 1;
                pars->res.code = NOT_SUPPORTED;
            }
            break;
        }
        case 0: {
            if (c == 'J') {
                pars->res.code = ERASE_CUR_TO_END;
                pars->ended = 1;
            } else if (c == '6') {
                pars->state = 4;
                //TODO: save digit
                pars->digits[pars->digitsNum][pars->currentDigitPos] = c;
                pars->currentDigitPos++;
            } else if (c >= '0' && c <= '9') {
                pars->state = 1;
                pars->digits[pars->digitsNum][pars->currentDigitPos] = c;
                pars->currentDigitPos++;
                //TODO: save digit
            } else if (c == '=') {
                pars->state = 2;
            } else if (c == '?') {
                pars->state = 3;
            } else if (c == 'H') {
                pars->res.code = MOVE_CURSOR_HOME;
                pars->ended = 1;
            } else if (c == 's') {
                pars->res.code = SAVE_CURSOR;
                pars->ended = 1;
            } else if (c == 'u') {
                pars->res.code = RESTORE_CURSOR;
                pars->ended = 1;
            } else if (c == 'K') {
                pars->res.code = CLEAR_CUR_TO_END_OF_LINE;
                pars->ended = 1;
            } else {
                //TODO: or state = 1?  //for move cursor without parameters
                pars->state = 1;
                parseEsc(pars, c);
            }
            break;
        }
        //digits: cursor position || erase functions || colors
        case 1: {
//                fprintf(stderr, "parsing case 1, buf[j] %c\n", buf[j]);
            if (c >= '0' && c <= '9') {
                if (pars->digitsNum >= pars->maxDigits) {
                    fprintf(stderr, "error: pars->digitsNum >= 6\n");
                    pars->res.code = ERROR;
                    pars->ended = 1;
                    break;
                }

                pars->digits[pars->digitsNum][pars->currentDigitPos] = c;
                pars->currentDigitPos++;
            } else if (c == ';') {
                if (pars->currentDigitPos == 0) {
                    pars->digits[pars->digitsNum][0] = '0';
                    pars->currentDigitPos++;
                }

                pars->digits[pars->digitsNum][pars->currentDigitPos] = 0;
                pars->digitsNum++;
                pars->currentDigitPos = 0;
            } else {
                if (pars->currentDigitPos != 0) {
                    pars->digits[pars->digitsNum][pars->currentDigitPos] = 0;
                    pars->digitsNum++;
                    fprintf(stderr, "pars->digitsNum = %d\n", pars->digitsNum);
                }

                switch (c) {
                    case 'f':
                    case 'H':
                        pars->res.code = MOVE_CURSOR_LINE_COLUMN;
                        if (pars->digitsNum > 0) {
                            long line = strtol(pars->digits[0], NULL, 10);
                            pars->res.cursor.line = line;
                        }
                        if (pars->digitsNum > 1) {
                            long column = strtol(pars->digits[1], NULL, 10);
                            pars->res.cursor.column = column;
                        }

                        pars->ended = 1;
                        break;

                    case 'A':  //up
                        pars->res.code = MOVE_CURSOR_LINE;
                        if (pars->digitsNum > 0) {
                            long line = strtol(pars->digits[0], NULL, 10);
                            pars->res.cursor.line = -line;
                        } else {
                            pars->res.cursor.line = -1;
                        }

                        pars->ended = 1;
                        break;
                    case 'B':  //down
                        pars->res.code = MOVE_CURSOR_LINE;
                        if (pars->digitsNum > 0) {
                            long line = strtol(pars->digits[0], NULL, 10);
                            pars->res.cursor.line = line;
                        } else {
                            pars->res.cursor.line = 1;
                        }

                        pars->ended = 1;
                        break;
                    case 'C':  //right
                        pars->res.code = MOVE_CURSOR_COLUMN;
                        if (pars->digitsNum > 0) {
                            long col = strtol(pars->digits[0], NULL, 10);
                            pars->res.cursor.column = col;
                        } else {
                            pars->res.cursor.column = 1;
                        }
                        pars->ended = 1;
                        break;
                    case 'D':  //left
                        pars->res.code = MOVE_CURSOR_COLUMN;
                        if (pars->digitsNum > 0) {
                            long col = strtol(pars->digits[0], NULL, 10);
                            pars->res.cursor.column = -col;
                        } else {
                            pars->res.cursor.column = -1;
                        }

                        pars->ended = 1;
                        break;
                    case 'E':
                        pars->res.code = MOVE_CURSOR_BEGIN_NEXT_LINE;
                        if (pars->digitsNum > 0) {
                            long line = strtol(pars->digits[0], NULL, 10);
                            pars->res.cursor.line = line;
                        } else {
                            pars->res.cursor.line = 1;
                        }

                        pars->ended = 1;
                        break;
                    case 'F':
                        pars->res.code = MOVE_CURSOR_BEGIN_PREV_LINE;
                        if (pars->digitsNum > 0) {
                            long line = strtol(pars->digits[0], NULL, 10);
                            pars->res.cursor.line = -line;
                        } else {
                            pars->res.cursor.line = -1;
                        }

                        pars->ended = 1;
                        break;
                    case 'G':
                        pars->res.code = MOVE_CURSOR_POS_COL;
                        if (pars->digitsNum > 0) {
                            long col = strtol(pars->digits[0], NULL, 10);
                            pars->res.cursor.column = -col;
                        } else {
                            pars->res.cursor.column = -1;
                        }

                        pars->ended = 1;
                        break;
                    case 'J':
                        if (pars->digitsNum == 0) {
                            pars->res.code = ERROR;
                        } else if (strcmp(pars->digits[0], "1") == 0) {
                            pars->res.code = ERASE_START_TO_CURSOR;
                        } else if (strcmp(pars->digits[0], "2") == 0) {
                            pars->res.code = ERASE_VISIBLE_SCREEN;
                        } else if (strcmp(pars->digits[0], "3") == 0) {
                            pars->res.code = ERASE_ENTIRE_BUFFER;
                        }

                        pars->ended = 1;
                        break;

                    case 'K':
                        if (pars->digitsNum == 0) {
                            pars->res.code = ERROR;
                        } else if (strcmp(pars->digits[0], "1") == 0) {
                            pars->res.code = CLEAR_START_TO_CURSOR_LINE;
                        } else if (strcmp(pars->digits[0], "2") == 0) {
                            pars->res.code = CLEAR_CURRENT_LINE;
                        }

                        pars->ended = 1;
                        break;

                    case 'm':

                        if ((pars->digitsNum == 3 || pars->digitsNum == 5) &&
                            (strcmp(pars->digits[0], "38") == 0 || strcmp(pars->digits[0], "48") == 0) &&
                            (strcmp(pars->digits[1], "2") == 0 || strcmp(pars->digits[1], "5") == 0)) {
                            pars->res.code = NOT_SUPPORTED;
                            pars->ended = 1;  //not supported
                            break;
                        }

                        if (pars->digitsNum == 0) {
                            pars->res.code = RESET_STYLE;
                            pars->ended = 1;
                            break;
                        }

                        for (int n = 0; n < pars->digitsNum; n++) {
                            if (strcmp(pars->digits[n], "0") == 0) {
                                pars->res.code = RESET_STYLE;         //TODO: allow more modes after 0
                                                                      //reset style and continue loop
                            } else if (strcmp(pars->digits[n], "1") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.bold = 1;
                            } else if (strcmp(pars->digits[n], "3") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.italic = 1;
                            } else if (strcmp(pars->digits[n], "4") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.underline = 1;
                            } else if (strcmp(pars->digits[n], "30") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.fColor = "black";
                            } else if (strcmp(pars->digits[n], "31") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.fColor = "red";
                            } else if (strcmp(pars->digits[n], "32") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.fColor = "green";
                            } else if (strcmp(pars->digits[n], "33") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.fColor = "yellow";
                            } else if (strcmp(pars->digits[n], "34") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.fColor = "blue";
                            } else if (strcmp(pars->digits[n], "35") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.fColor = "magenta";
                            } else if (strcmp(pars->digits[n], "36") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.fColor = "cyan";
                            } else if (strcmp(pars->digits[n], "37") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.fColor = "white";
                            } else if (strcmp(pars->digits[n], "40") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.bColor = "black";
                            } else if (strcmp(pars->digits[n], "41") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.bColor = "red";
                            } else if (strcmp(pars->digits[n], "42") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.bColor = "green";
                            } else if (strcmp(pars->digits[n], "43") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.bColor = "yellow";
                            } else if (strcmp(pars->digits[n], "44") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.bColor = "blue";
                            } else if (strcmp(pars->digits[n], "45") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.bColor = "magenta";
                            } else if (strcmp(pars->digits[n], "46") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.bColor = "cyan";
                            } else if (strcmp(pars->digits[n], "47") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.bColor = "white";
                            }
                        }

                        pars->ended = 1;
                        break;
                    default:
                        pars->res.code = NOT_SUPPORTED;   //TODO: unsupported?
                        pars->ended = 1;
                        fprintf(stderr, "unsupported esc seq c = %d", c);
                        break;
                }
            }
            break;
        }
        //set mode
        case 2: {
            pars->res.code = NOT_SUPPORTED;
            pars->ended = 1;
            break;
        }
        //common private modes
        case 3: {
            pars->res.code = NOT_SUPPORTED;
            pars->ended = 1;
            break;
        }
        case 4: {
            if (c == 'n') {
                pars->res.code = REQ_CURSOR;
                pars->ended = 1;
            } else {
                pars->state = 1;
                parseEsc(pars, c);
            }
        }
    }

}