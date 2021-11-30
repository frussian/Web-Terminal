//
// Created by Anton on 28.11.2021.
//

#include "tools.h"
#include <stdlib.h>
#include <string.h>

char *append(char* s1, int len1, const char *s2, int len2) {
    char *new = realloc(s1, len1 + len2);
    if (new == NULL) return NULL;

    memcpy(new + len1, s2, len2);
    return new;
}