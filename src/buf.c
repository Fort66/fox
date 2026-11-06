#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "buf.h"

void buf_init(Buf* b) {
    b->cap = 64;
    b->len = 0;
    b->data = (char*)malloc(b->cap);
    b->data[0] = '\0';
}

void buf_free(Buf* b) {
    free(b->data);
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

void buf_appendf(Buf* b, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    va_list copy;
    va_copy(copy, args);
    int need = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (need < 0) { va_end(copy); return; }
    if (b->len + need + 1 > b->cap) {
        while (b->len + need + 1 > b->cap) b->cap *= 2;
        b->data = (char*)realloc(b->data, b->cap);
    }
    vsnprintf(b->data + b->len, need + 1, fmt, copy);
    va_end(copy);
    b->len += need;
}