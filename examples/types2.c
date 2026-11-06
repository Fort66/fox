#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

static char* fox_concat(const char* a, const char* b) {
    size_t la = strlen(a), lb = strlen(b);
    char* r = (char*)malloc(la + lb + 1);
    memcpy(r, a, la);
    memcpy(r + la, b, lb + 1);
    return r;
}

static char* fox_upper(const char* s) {
    size_t n = strlen(s);
    char* r = (char*)malloc(n + 1);
    for (size_t i = 0; i < n; i++) r[i] = (char)toupper((unsigned char)s[i]);
    r[n] = '\0';
    return r;
}

static char* fox_lower(const char* s) {
    size_t n = strlen(s);
    char* r = (char*)malloc(n + 1);
    for (size_t i = 0; i < n; i++) r[i] = (char)tolower((unsigned char)s[i]);
    r[n] = '\0';
    return r;
}

static char* fox_trim(const char* s) {
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') s++;
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == ' ' || s[n-1] == '\t' || s[n-1] == '\r' || s[n-1] == '\n')) n--;
    char* r = (char*)malloc(n + 1);
    memcpy(r, s, n);
    r[n] = '\0';
    return r;
}

static char* fox_substr(const char* s, double start_d, double count_d) {
    long start = (long)start_d;
    long count = (long)count_d;
    size_t n = strlen(s);
    if (start < 1) start = 1;
    size_t idx = (size_t)(start - 1);
    if (idx > n) idx = n;
    size_t avail = n - idx;
    size_t take = (count < 0) ? avail : ((size_t)count < avail ? (size_t)count : avail);
    char* r = (char*)malloc(take + 1);
    memcpy(r, s + idx, take);
    r[take] = '\0';
    return r;
}

static char* fox_str(double v) {
    char* r = (char*)malloc(64);
    snprintf(r, 64, "%g", v);
    return r;
}

typedef struct { double* data; int len; int cap; } FoxNumList;
typedef struct { const char** data; int len; int cap; } FoxStrList;

static FoxNumList fox_num_list_of(int n, ...) {
    FoxNumList l; l.data = NULL; l.len = 0; l.cap = 0;
    va_list ap; va_start(ap, n);
    if (n > 0) {
        l.cap = n; l.len = n;
        l.data = (double*)malloc(sizeof(double) * (size_t)n);
        for (int i = 0; i < n; i++) l.data[i] = va_arg(ap, double);
    }
    va_end(ap);
    return l;
}

static FoxStrList fox_str_list_of(int n, ...) {
    FoxStrList l; l.data = NULL; l.len = 0; l.cap = 0;
    va_list ap; va_start(ap, n);
    if (n > 0) {
        l.cap = n; l.len = n;
        l.data = (const char**)malloc(sizeof(const char*) * (size_t)n);
        for (int i = 0; i < n; i++) l.data[i] = va_arg(ap, const char*);
    }
    va_end(ap);
    return l;
}

static FoxNumList fox_num_list_clone(FoxNumList src) {
    FoxNumList l = src;
    if (src.len > 0) {
        l.data = (double*)malloc(sizeof(double) * (size_t)src.len);
        memcpy(l.data, src.data, sizeof(double) * (size_t)src.len);
    } else { l.data = NULL; }
    l.cap = src.len;
    return l;
}

static FoxStrList fox_str_list_clone(FoxStrList src) {
    FoxStrList l = src;
    if (src.len > 0) {
        l.data = (const char**)malloc(sizeof(const char*) * (size_t)src.len);
        memcpy(l.data, src.data, sizeof(const char*) * (size_t)src.len);
    } else { l.data = NULL; }
    l.cap = src.len;
    return l;
}

static double fox_num_list_get(FoxNumList* l, double i) {
    long idx = (long)i;
    if (idx < 1 || idx > l->len) {
        fprintf(stderr, "Ошибка выполнения: индекс %ld вне диапазона 1..%d\n", idx, l->len);
        exit(1);
    }
    return l->data[idx - 1];
}

static const char* fox_str_list_get(FoxStrList* l, double i) {
    long idx = (long)i;
    if (idx < 1 || idx > l->len) {
        fprintf(stderr, "Ошибка выполнения: индекс %ld вне диапазона 1..%d\n", idx, l->len);
        exit(1);
    }
    return l->data[idx - 1];
}

static void fox_num_list_set(FoxNumList* l, double i, double v) {
    long idx = (long)i;
    if (idx < 1 || idx > l->len) {
        fprintf(stderr, "Ошибка выполнения: индекс %ld вне диапазона 1..%d\n", idx, l->len);
        exit(1);
    }
    l->data[idx - 1] = v;
}

static void fox_str_list_set(FoxStrList* l, double i, const char* v) {
    long idx = (long)i;
    if (idx < 1 || idx > l->len) {
        fprintf(stderr, "Ошибка выполнения: индекс %ld вне диапазона 1..%d\n", idx, l->len);
        exit(1);
    }
    l->data[idx - 1] = v;
}

static void fox_num_list_append(FoxNumList* l, double v) {
    if (l->len == l->cap) {
        int newcap = (l->cap == 0) ? 4 : l->cap * 2;
        l->data = (double*)realloc(l->data, sizeof(double) * (size_t)newcap);
        l->cap = newcap;
    }
    l->data[l->len++] = v;
}

static void fox_str_list_append(FoxStrList* l, const char* v) {
    if (l->len == l->cap) {
        int newcap = (l->cap == 0) ? 4 : l->cap * 2;
        l->data = (const char**)realloc(l->data, sizeof(const char*) * (size_t)newcap);
        l->cap = newcap;
    }
    l->data[l->len++] = v;
}

int main() {
    double flag = 1.0;
    double other = 0.0;
    printf("%g\n", flag);
    printf("%g\n", other);
    printf("%g\n", (double)(flag && other));
    printf("%g\n", (double)(flag || other));
    printf("%g\n", (double)(!other));
    printf("%g\n", (double)((double)((double)5 > (double)3) && (double)(!other)));
    const char* s = "  Hello, Fox  ";
    const char* t = fox_trim(s);
    printf("%s\n", t);
    printf("%s\n", fox_upper(t));
    printf("%s\n", fox_lower(t));
    printf("%g\n", (double)strlen(t));
    printf("%s\n", fox_substr(t, (double)8, (double)3));
    printf("%s\n", fox_str((double)42));
    printf("%g\n", strtod("123", NULL) + (double)7);
    const char* mixed = fox_concat(fox_upper(fox_substr(t, (double)1, (double)5)), "!");
    printf("%s\n", mixed);
    if ((double)((double)((double)strlen(t) > (double)0) && (double)(!(double)(flag == 0.0)))) {
    printf("%s\n", "logic ok");
    }
    return 0;
}
