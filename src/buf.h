#ifndef FOX_BUF_H
#define FOX_BUF_H

typedef struct {
    char* data;
    int len;
    int cap;
} Buf;

void buf_init(Buf* b);
void buf_free(Buf* b);
void buf_appendf(Buf* b, const char* fmt, ...);

#endif