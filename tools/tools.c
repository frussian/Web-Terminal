//
// Created by Anton on 28.11.2021.
//

#include "tools.h"
#include <stdlib.h>
#include <string.h>

int init_abuf(struct abuf *buf, size_t size) {
    buf->buf = malloc(size);
    if (buf->buf == NULL) return -1;
    buf->offset = 0;
    buf->size = size;
    return 0;
}

int reinit_abuf(struct abuf *buf, size_t size) {
    if (size != -1) {
        buf->buf = realloc(buf->buf, size);
        if (buf->buf == NULL) return -1;
        buf->size = size;
    }
    buf->offset = 0;
    return 0;
}

void free_abuf(struct abuf *buf) {
    free(buf->buf);
}

int append_abuf(struct abuf *buf, char *a_buf, size_t a_buf_size) {
    char *new_buf = buf->buf;
    size_t new_size = buf->size;
    int offset = buf->offset;
    if (offset + a_buf_size >= new_size) {
        new_size = 2 * (offset + a_buf_size);
        new_buf = realloc(new_buf, new_size);
        if (new_buf == NULL) return -1;
    }
    memcpy(new_buf + offset, a_buf, a_buf_size);
    buf->buf = new_buf;
    buf->offset += a_buf_size;
    buf->size = new_size;
    return 0;
}

char *append(char* s1, int len1, const char *s2, int len2) {
    char *new = realloc(s1, len1 + len2);
    if (new == NULL) return NULL;

    memcpy(new + len1, s2, len2);
    return new;
}
