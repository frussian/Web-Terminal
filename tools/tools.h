//
// Created by Anton on 28.11.2021.
//

#ifndef WEBTERMINAL_TOOLS_H
#define WEBTERMINAL_TOOLS_H

#include <string.h>
#include <malloc.h>

struct abuf {
    char *buf;
    int offset;
    size_t size;
};

int init_abuf(struct abuf *buf, size_t size);

int reinit_abuf(struct abuf *buf, size_t size);

void free_abuf(struct abuf *buf);

int append_abuf(struct abuf *buf, char *a_buf, size_t a_buf_size);

char *append(char* s1, int len1, const char *s2, int len2);

#endif //WEBTERMINAL_TOOLS_H
