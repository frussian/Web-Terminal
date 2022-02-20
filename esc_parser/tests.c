#include <stdio.h>
#include <string.h>
#include "esc_parser.h"

// #define myAssertD(expr, expected)                          \
//     test++;                                                \
//     if (expr == expected) printf("\x1b[32mtrue\x1b[0m\n"); \
//     else {errors++; printf("\x1b[31mtest %ld false, got %ld\x1b[0m\n", test, expr);}

// #define myAssertS(expr, expected)                                                   \
//     test++;                                                                         \
//     if (strcmp(expr, expected) == 0) printf("\x1b[32mtrue\x1b[0m\n");               \
//     else {errors++; printf("\x1b[32mtest %s false, got %s\x1b[0m\n", test, expr);}

int errors = 0;
int test = 0;

void myAssertD(int expr, int expected) {
  test++;
  if (expr == expected) printf("\x1b[32mtrue\x1b[0m\n");
  else {
    printf("\x1b[31mtest %ld false, got %ld\x1b[0m\n", test, expr);
    errors++;
  }
}

void myAssertS(char *expr, char *expected) {
  test++;
  if (strcmp(expr, expected) == 0) printf("\x1b[32mtrue\x1b[0m\n");
  else {
    errors++;
    printf("\x1b[32mtest %d false, got %s\x1b[0m\n", test, expr);
  }
}

struct esc parse_esc_seq(char *seq, int n) {
  struct esc_parser p;
  init_parser(&p);
  parse_esc(&p, '[');
  for (int i = 0; i < n; i++) {
    parse_esc(&p, seq[i]);
    printf("%c ", seq[i]);
    if (p.ended) {
      break;
    }
  }
  return p.res;
}

int main() {
    myAssertD(parse_esc_seq("1J", 2).code, ERASE_START_TO_CURSOR);
    myAssertD(parse_esc_seq("2J", 2).code, ERASE_VISIBLE_SCREEN);
    myAssertD(parse_esc_seq("J", 1).code, ERASE_CUR_TO_END);

    myAssertD(parse_esc_seq("1;32masdfgba", 12).code, STYLE);
    myAssertD(parse_esc_seq("1;32masdfgba", 12).s.bold, 1);
    myAssertS(parse_esc_seq("1;32masdfgba", 12).s.fColor, "green");
    myAssertS(parse_esc_seq("35;41mGsdbgafgba", 16).s.bColor, "red");
    myAssertS(parse_esc_seq("4;35;41mAsdbgafgba", 18).s.fColor, "magenta");
    myAssertD(parse_esc_seq("4;35;41mAsdbgafgba", 18).s.underline, 1);
    myAssertD(parse_esc_seq("38;5;41mAsdbgafgba", 18).code, NOT_SUPPORTED);
    myAssertD(parse_esc_seq("48;2;41;24;15mAsdbgafgba", 24).code, NOT_SUPPORTED);

    myAssertD(parse_esc_seq("2Kawd", 5).code, CLEAR_CURRENT_LINE);
    myAssertD(parse_esc_seq("1Kawd", 5).code, CLEAR_START_TO_CURSOR_LINE);
    myAssertD(parse_esc_seq("Kawd", 4).code, CLEAR_CUR_TO_END_OF_LINE);

    myAssertD(parse_esc_seq("2;52Hfataset", 12).code, MOVE_CURSOR_LINE_COLUMN);
    myAssertD(parse_esc_seq("2;52Hfataset", 12).cursor.line, 2);
    myAssertD(parse_esc_seq("2;52Hfataset", 12).cursor.column, 52);
    myAssertD(parse_esc_seq("251Bfataset", 11).code, MOVE_CURSOR_LINE);
    myAssertD(parse_esc_seq("251Bfataset", 11).cursor.line, 251);
    myAssertD(parse_esc_seq("251Bfataset", 11).cursor.column, 0);
    myAssertD(parse_esc_seq("Aga afg", 7).code, MOVE_CURSOR_LINE);
    myAssertD(parse_esc_seq("Aga afg", 7).cursor.line, -1);
    myAssertD(parse_esc_seq("5Aga afg", 8).cursor.line, -5);
    myAssertD(parse_esc_seq("5Cga afg", 8).cursor.line, 0);
    myAssertD(parse_esc_seq("5Cga afg", 8).cursor.column, 5);
    myAssertD(parse_esc_seq("5Cga afg", 8).cursor.line, 0);
    myAssertD(parse_esc_seq("Dga afg", 7).cursor.line, 0);
    myAssertD(parse_esc_seq("Dga afg", 7).cursor.column, -1);




    if (errors) {
        printf("\x1b[31mgot %d errors\x1b[0m\n", errors);
    } else {
        printf("\x1b[32meverything is true\x1b[0m\n");
    }
    return 0;
}
