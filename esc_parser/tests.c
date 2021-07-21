#include <stdio.h>
#include <string.h>
#include "esc_parser.h"

#define myAssertD(expr, expected)                          \
    test++;                                                \
    if (expr == expected) printf("\x1b[32mtrue\x1b[0m\n"); \
    else {errors++; printf("\x1b[31mtest %d false, got %d\x1b[0m\n", test, expr);}

#define myAssertS(expr, expected)                                                   \
    test++;                                                                         \
    if (strcmp(expr, expected) == 0) printf("\x1b[32mtrue\x1b[0m\n");               \
    else {errors++; printf("\x1b[32mtest %d false, got %d\x1b[0m\n", test, expr);}

int main() {
    int errors = 0;
    int test = 0;
    myAssertD(parseEsc("1J", 2, NULL).code, ERASE_START_TO_CURSOR);
    myAssertD(parseEsc("2J", 2, NULL).code, ERASE_VISIBLE_SCREEN);
    myAssertD(parseEsc("J", 1, NULL).code, ERASE_CUR_TO_END);

    myAssertD(parseEsc("1;32masdfgba", 12, NULL).code, STYLE);
    myAssertD(parseEsc("1;32masdfgba", 12, NULL).s.bold, 1);
    myAssertS(parseEsc("1;32masdfgba", 12, NULL).s.fColor, "green");
    myAssertS(parseEsc("35;41mGsdbgafgba", 16, NULL).s.bColor, "red");
    myAssertS(parseEsc("4;35;41mAsdbgafgba", 18, NULL).s.fColor, "magenta");
    myAssertD(parseEsc("4;35;41mAsdbgafgba", 18, NULL).s.underline, 1);
    myAssertD(parseEsc("38;5;41mAsdbgafgba", 18, NULL).code, NOT_SUPPORTED);
    myAssertD(parseEsc("48;2;41;24;15mAsdbgafgba", 24, NULL).code, NOT_SUPPORTED);

    myAssertD(parseEsc("2Kawd", 5, NULL).code, CLEAR_CURRENT_LINE);
    myAssertD(parseEsc("1Kawd", 5, NULL).code, CLEAR_START_TO_CURSOR_LINE);
    myAssertD(parseEsc("Kawd", 4, NULL).code, CLEAR_CUR_TO_END_OF_LINE);
    myAssertD(parseEsc("Kawd", 4, NULL).code, CLEAR_CUR_TO_END_OF_LINE);

    myAssertD(parseEsc("2;52Hfataset", 12, NULL).code, MOVE_CURSOR_LINE_COLUMN);
    myAssertD(parseEsc("2;52Hfataset", 12, NULL).cursor.line, 2);
    myAssertD(parseEsc("2;52Hfataset", 12, NULL).cursor.column, 52);
    myAssertD(parseEsc("251Bfataset", 11, NULL).code, MOVE_CURSOR_LINE);
    myAssertD(parseEsc("251Bfataset", 11, NULL).cursor.line, 251);
    myAssertD(parseEsc("251Bfataset", 11, NULL).cursor.column, 0);
    myAssertD(parseEsc("Aga afg", 7, NULL).code, MOVE_CURSOR_LINE);
    myAssertD(parseEsc("Aga afg", 7, NULL).cursor.line, -1);
    myAssertD(parseEsc("5Aga afg", 8, NULL).cursor.line, -5);
    myAssertD(parseEsc("5Cga afg", 8, NULL).cursor.line, 0);
    myAssertD(parseEsc("5Cga afg", 8, NULL).cursor.column, 5);
    myAssertD(parseEsc("5Cga afg", 8, NULL).cursor.line, 0);
    myAssertD(parseEsc("Dga afg", 7, NULL).cursor.line, 0);
    myAssertD(parseEsc("Dga afg", 7, NULL).cursor.column, -1);




    if (errors) {
        printf("\x1b[31mgot %d errors\x1b[0m\n", errors);
    } else {
        printf("\x1b[32meverything is true\x1b[0m\n");
    }
    return 0;
}