#include<stdlib.h>
#include<stdio.h>

int main () {
    char s[] = "[5;13;1234;54m";
    int a, b;
    sscanf(s, "[%d;%dm", &a, &b);
    printf("%d %d\n", a, b);
    return 0;
}