#include <stdio.h>
#include <unistd.h>

int main() {
    printf("Hello\x7f\xff");
    printf("f\n");
    return 0;
}