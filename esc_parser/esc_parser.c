//
// Created by Anton on 21.07.2021.
//

#include <stdio.h>
#include <string.h>
#include <esc_parser.h>


void clear_style(struct style *s) {
    s->underline = 0;
    s->italic = 0;
    s->fColor = NULL;
    s->bold = 0;
    s->bColor = NULL;
    s->changed = 0;
}

int style_is_empty(struct style *s) {
    return !s->underline && !s->italic && !s->fColor &&
           !s->bold && !s->bColor;
}

int style_equal(struct style *s1, struct style *s2) {
    return s1->underline == s2->underline &&
           s1->italic == s2->italic &&
           s1->bold == s2->bold &&
           s1->fColor == s2->fColor && //|| strcmp(s1->fColor, s2->fColor) == 0) &&  //TODO: check this
           s1->bColor == s2->bColor;//|| strcmp(s1->bColor, s2->bColor) == 0);

}

int init_parser(struct esc_parser *parser) {
    parser->state = -1;
    parser->ended = 0;

    parser->res.code = ERROR;
    clear_style(&parser->res.s);
    parser->res.cursor.column = 0; //0 is absence of field
    parser->res.cursor.line = 0;   //coordinates start from 1
    parser->res.alt_buf_clear_on_enter = 0;
    parser->res.alt_buf_clear_on_exit = 0;
    parser->res.margins[0] = 0;
    parser->res.margins[1] = 0;
    parser->res.scroll_num = 0;

    parser->digitsNum = 0;
    parser->currentDigitPos = 0;
    parser->maxDigits = MAX_DIGITS;
    parser->maxDigitLen = MAX_DIGITS_LEN;

    return init_abuf(&parser->text, 128);
}

int update_digit(struct esc_parser *pars, char c);
void inc_digits(struct esc_parser *pars);

void parse_esc(struct esc_parser *pars, char c) {

    switch (pars->state) {
        //start
        case -1: {
            if (c == '[') {
                pars->state = 0;
            } else if (c == '(') {
                pars->state = 5;
            } else if (c == ']') {
                pars->state = 6;
            } else if (c == 'D') {
                pars->res.code = INDEX;
                pars->ended = 1;
            } else if (c == 'M') {
                pars->res.code = REVERSE_INDEX;
                pars->ended = 1;
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
                pars->digits[pars->digitsNum][pars->currentDigitPos] = c;
                pars->currentDigitPos++;
            } else if (c >= '0' && c <= '9') {
                pars->state = 1;
                pars->digits[pars->digitsNum][pars->currentDigitPos] = c;
                pars->currentDigitPos++;
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
                pars->state = 1;
                parse_esc(pars, c);
            }
            break;
        }
        //digits: cursor position || erase functions || colors
        case 1: {
//                fprintf(stderr, "parsing case 1, buf[j] %c\n", buf[j]);
            if (c >= '0' && c <= '9') {
                int res = update_digit(pars, c);
                if (res < 0) {
                    fprintf(stderr, "error: pars->digitsNum >= 6\n");
                    pars->res.code = ERROR;
                    pars->ended = 1;
                    break;
                }
            } else if (c == ';') {
                inc_digits(pars);
            } else {
                if (pars->currentDigitPos != 0) {
                    inc_digits(pars);
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
                            pars->res.cursor.column = col;
                        } else {
                            pars->res.cursor.column = 1;
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
                            if (strcmp(pars->digits[n], "0") == 0 ||
                                strcmp(pars->digits[n], "00") == 0) {
                                pars->res.code = RESET_STYLE;         //TODO: allow more modes after 0
                                                                      //reset style and continue loop
                            } else if (strcmp(pars->digits[n], "1") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.bold = 1;
                                pars->res.s.changed |= BOLD;
                            } else if (strcmp(pars->digits[n], "22") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.bold = 0;
                                pars->res.s.changed |= BOLD;
                            } else if (strcmp(pars->digits[n], "3") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.italic = 1;
                                pars->res.s.changed |= ITALIC;
                            } else if (strcmp(pars->digits[n], "23") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.italic = 0;
                                pars->res.s.changed |= ITALIC;
                            } else if (strcmp(pars->digits[n], "4") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.underline = 1;
                                pars->res.s.changed |= UNDERLINE;
                            } else if (strcmp(pars->digits[n], "24") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.underline = 0;
                                pars->res.s.changed |= UNDERLINE;
                            } else if (strcmp(pars->digits[n], "30") == 0 ||
                                    strcmp(pars->digits[n], "90") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.fColor = "black";
                            } else if (strcmp(pars->digits[n], "31") == 0 ||
                                    strcmp(pars->digits[n], "91") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.fColor = "red";
                            } else if (strcmp(pars->digits[n], "32") == 0 ||
                                    strcmp(pars->digits[n], "92") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.fColor = "green";
                            } else if (strcmp(pars->digits[n], "33") == 0 ||
                                    strcmp(pars->digits[n], "93") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.fColor = "yellow";
                            } else if (strcmp(pars->digits[n], "34") == 0 ||
                                    strcmp(pars->digits[n], "94") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.fColor = "blue";
                            } else if (strcmp(pars->digits[n], "35") == 0 ||
                                    strcmp(pars->digits[n], "95") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.fColor = "magenta";
                            } else if (strcmp(pars->digits[n], "36") == 0 ||
                                    strcmp(pars->digits[n], "96") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.fColor = "cyan";
                            } else if (strcmp(pars->digits[n], "37") == 0 ||
                                        strcmp(pars->digits[n], "97") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.fColor = "white";
                            } else if (strcmp(pars->digits[n], "40") == 0 ||
                                    strcmp(pars->digits[n], "100") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.bColor = "black";
                            } else if (strcmp(pars->digits[n], "41") == 0 ||
                                    strcmp(pars->digits[n], "101") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.bColor = "red";
                            } else if (strcmp(pars->digits[n], "42") == 0 ||
                                    strcmp(pars->digits[n], "102") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.bColor = "green";
                            } else if (strcmp(pars->digits[n], "43") == 0 ||
                                    strcmp(pars->digits[n], "103") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.bColor = "yellow";
                            } else if (strcmp(pars->digits[n], "44") == 0 ||
                                    strcmp(pars->digits[n], "104") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.bColor = "blue";
                            } else if (strcmp(pars->digits[n], "45") == 0 ||
                                    strcmp(pars->digits[n], "105") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.bColor = "magenta";
                            } else if (strcmp(pars->digits[n], "46") == 0 ||
                                    strcmp(pars->digits[n], "106") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.bColor = "cyan";
                            } else if (strcmp(pars->digits[n], "47") == 0 ||
                                    strcmp(pars->digits[n], "107") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.bColor = "white";
                            } else if (strcmp(pars->digits[n], "39") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.fColor = NULL;
                                pars->res.s.changed |= F_COLOR;
                            } else if (strcmp(pars->digits[n], "49") == 0) {
                                pars->res.code = STYLE;
                                pars->res.s.bColor = NULL;
                                pars->res.s.changed |= B_COLOR;
                            } else {
                                pars->res.code = NOT_SUPPORTED;
                            }

                            if (pars->res.s.fColor) {
                                pars->res.s.changed |= F_COLOR;
                            }
                            if (pars->res.s.bColor) {
                                pars->res.s.changed |= B_COLOR;
                            }
                        }

                        pars->ended = 1;
                        break;

                    case 'X': {
                        pars->res.code = ERASE_N_CHARS_FROM_CURSOR;
                        if (pars->digitsNum > 0) {
                            long col = strtol(pars->digits[0], NULL, 10);
                            pars->res.cursor.column = col;
                        } else {
                            fprintf(stderr, "number of characters expected for ESC[X");
                            pars->res.code = ERROR;
                        }
                        pars->ended = 1;
                        break;
                    }
                    case 'd': {
                        pars->res.code = MOVE_CUR_ABS;
                        if (pars->digitsNum > 0) {
                            long line = strtol(pars->digits[0], NULL, 10);
                            pars->res.cursor.line = line;
                        } else {
                            pars->res.cursor.line = 1;
                        }
                        pars->ended = 1;
                        break;
                    }
                    case 'P': {
                        pars->res.code = DELETE_N_CHARS_RIGHT_FROM_CURSOR_WITH_SHIFT;
                        if (pars->digitsNum > 0) {
                            long col = strtol(pars->digits[0], NULL, 10);
                            pars->res.cursor.column = col;
                        } else {
                            pars->res.cursor.column = 1;
                        }
                        pars->ended = 1;
                        break;
                    }
                    case 'r': {
                        pars->res.code = ROW_MARGINS;
                        if (pars->digitsNum > 0) {
                            long top = strtol(pars->digits[0], NULL, 10);
                            pars->res.margins[1] = top;
                        }
                        if (pars->digitsNum > 1) {
                            long bottom = strtol(pars->digits[1], NULL, 10);
                            pars->res.margins[0] = bottom;
                        }
                        pars->ended = 1;
                        break;
                    }
                    case 'S': {
                        pars->res.code = SCROLL_UP;
                        if (pars->digitsNum > 0) {
                            long num = strtol(pars->digits[0], NULL, 10);
                            pars->res.scroll_num = num;
                        } else {
                            pars->res.code = ERROR;
                            fprintf(stderr, "absent amount of scroll up rows\n");
                        }
                        pars->ended = 1;
                        break;
                    }
                    case 'T': {
                        pars->res.code = SCROLL_DOWN;
                        if (pars->digitsNum > 0) {
                            long num = strtol(pars->digits[0], NULL, 10);
                            pars->res.scroll_num = num;
                        } else {
                            pars->res.code = ERROR;
                            fprintf(stderr, "absent amount of scroll down rows\n");
                        }
                        pars->ended = 1;
                        break;
                    }
                    case 'L': {
                        pars->res.code = INSERT_LINE;
                        if (pars->digitsNum > 0) {
                            long num = strtol(pars->digits[0], NULL, 10);
                            pars->res.scroll_num = num;
                        } else {
                            pars->res.code = ERROR;
                            fprintf(stderr, "absent amount of insert line\n");
                        }
                        pars->ended = 1;
                        break;
                    }
                    case 'M': {
                        pars->res.code = DELETE_LINE;
                        if (pars->digitsNum > 0) {
                            long num = strtol(pars->digits[0], NULL, 10);
                            pars->res.scroll_num = num;
                        } else {
                            pars->res.code = ERROR;
                            fprintf(stderr, "absent amount of delete line\n");
                        }
                        pars->ended = 1;
                        break;
                    }
                    default: {
                        pars->res.code = NOT_SUPPORTED;
                        pars->ended = 1;
                        break;
                    }
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
        //?
        case 3: {
            if (c >= '0' && c <= '9') {
                int res = update_digit(pars, c);
                if (res < 0) {
                    pars->res.code = ERROR;
                    pars->ended = 1;
                    break;
                }
            } else if (c == ';') {
                inc_digits(pars);
            } else {
                if (pars->currentDigitPos != 0) {
                    inc_digits(pars);
                }

                long code;
                if (pars->digitsNum > 0) {
                    code = strtol(pars->digits[0], NULL, 10);
                } else {
                    fprintf(stderr, "number of characters expected for ESC[?");
                    pars->res.code = ERROR;
                    pars->ended = 1;
                    break;
                }
                if (c != 'h' && c != 'l') {
                    fprintf(stderr, "unsupported ESC[?{num}%c", c);
                    pars->res.code = NOT_SUPPORTED;
                    pars->ended = 1;
                    break;
                }
                
                switch (code) {
                    case 47:
                    case 1047:
                    case 1049: {
                        if (code == 1049) {
                            pars->res.alt_buf_clear_on_enter = 1;
                        } else if (code == 1047) {
                            pars->res.alt_buf_clear_on_exit = 1;
                        }
                        if (c == 'h') {
                            pars->res.code = ALT_BUF_ON;
                        } else {
                            pars->res.code = ALT_BUF_OFF;
                        }
                        break;
                    }
                    case 7: {
                        if (c == 'h') {
                            pars->res.code = AUTO_WRAP_ON;
                        } else {
                            pars->res.code = AUTO_WRAP_OFF;
                        }
                        break;
                    }
                    case 25: {
                        if (c == 'h') {
                            pars->res.code = SHOW_CUR;
                        } else {
                            pars->res.code = HIDE_CUR;
                        }
                        break;
                    }
                    default: {
                        pars->res.code = NOT_SUPPORTED;
                    }
                }
                pars->ended = 1;
            }
            break;
        }
        case 4: {
            if (c == 'n') {
                pars->res.code = REQ_CURSOR;
                pars->ended = 1;
            } else {
                pars->state = 1;
                parse_esc(pars, c);
            }
        }
        case 5: {
            if (c == 'B') {
                pars->res.code = NOT_SUPPORTED;
                pars->ended = 1;
            }

            break;
        }
        //operating system commands
        case 6: {
            if (pars->res.code != ERROR) {
                if (c == ';') {
                    break;
                }
                if (c == '\x07') {
                    pars->ended = 1;
                    break;
                }
                int res = append_abuf(&pars->text, &c, 1);
                if (res < 0) {
                    fprintf(stderr, "cannot append buf\n");
                    exit(1);
                }
                break;
            }

            switch (c) {
                case '0': {
                    pars->res.code = SET_ICON_WINDOW_NAME;
                    break;
                }
                case '1': {
                    pars->res.code = SET_ICON_NAME;
                    break;
                }
                case '2': {
                    pars->res.code = SET_WINDOW_NAME;
                    break;
                }
                default : {
                    pars->res.code = NOT_SUPPORTED;
                    pars->ended = 1;
                }
            }
            break;
        }
    }

}

int update_digit(struct esc_parser *pars, char c) {
    if (pars->digitsNum >= pars->maxDigits) {
        fprintf(stderr, "error: pars->digitsNum >= 6\n");
        return -1;
    }

    pars->digits[pars->digitsNum][pars->currentDigitPos] = c;
    pars->currentDigitPos++;
    return 0;
}

void inc_digits(struct esc_parser *pars) {
    if (pars->currentDigitPos == 0) {
        pars->digits[pars->digitsNum][0] = '0';
        pars->currentDigitPos++;
    }

    pars->digits[pars->digitsNum][pars->currentDigitPos] = 0;
    pars->digitsNum++;
    pars->currentDigitPos = 0;
}