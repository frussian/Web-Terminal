#include <stdio.h>
#include <string.h>
#include "esc_parser.h"

#define myAssertD(expr, expected)            \
    if (expr == expected) printf("\x1b[32mtrue\x1b[0m\n"); \
    else printf("\x1b[32mfalse, got %d\x1b[0m\n", expr);

#define myAssertS(expr, expected)            \
    if (strcmp(expr, expected) == 0) printf("\x1b[32mtrue\x1b[0m\n"); \
    else printf("\x1b[32mfalse, got %d\x1b[0m\n", expr);

int main() {
    myAssertD(parseEsc("1J", 2, NULL).code, ERASE_START_TO_CURSOR);
    myAssertD(parseEsc("2J", 2, NULL).code, ERASE_VISIBLE_SCREEN);
    myAssertD(parseEsc("J", 1, NULL).code, ERASE_CUR_TO_END);
    myAssertD(parseEsc("1;32masdfgba", 12, NULL).code, STYLE);
    myAssertD(parseEsc("1;32masdfgba", 12, NULL).s.bold, 1);
    myAssertS(parseEsc("1;32masdfgba", 12, NULL).s.fColor, "green");
    return 0;
}