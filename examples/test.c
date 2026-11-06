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
    printf("%s\n", "=== ТЕСТ 1: Переменные и арифметика ===");
    double a = (double)10;
    double b = (double)3;
    double sum = a + b;
    double diff = a - b;
    double prod = a * b;
    double quot = a / b;
    double int_div = floor(a / b);
    double remainder = fmod(a, b);
    printf("%g\n", sum);
    printf("%g\n", diff);
    printf("%g\n", prod);
    printf("%g\n", quot);
    printf("%g\n", int_div);
    printf("%g\n", remainder);
    printf("%s\n", "=== ТЕСТ 2: Строки ===");
    const char* greeting = "Hello";
    const char* name = "Fox";
    printf("%s\n", greeting);
    printf("%s\n", name);
    printf("%s\n", "=== ТЕСТ 3: Условия ===");
    double x = (double)15;
    if ((double)(x > (double)20)) {
    printf("%s\n", "x больше 20");
    } else     if ((double)(x > (double)10)) {
    printf("%s\n", "x больше 10, но не больше 20");
    } else     if ((double)(x > (double)5)) {
    printf("%s\n", "x больше 5, но не больше 10");
    } else {
    printf("%s\n", "x меньше или равно 5");
    }
    printf("%s\n", "=== ТЕСТ 4: Цикл while ===");
    double i = (double)1;
    while ((double)(i <= (double)5)) {
    printf("%g\n", i);
    i += (double)1;
    }
    printf("%s\n", "=== ТЕСТ 5: Цикл for ===");
    double j;
    for (j = (double)1; j <= (double)5; j++) {
    printf("%g\n", j);
    }
    printf("%s\n", "=== ТЕСТ 6: Do-case ===");
    double choice = (double)2;
    if ((double)(choice == (double)1)) {
    printf("%s\n", "Выбран вариант 1");
    } else if ((double)(choice == (double)2)) {
    printf("%s\n", "Выбран вариант 2");
    } else if ((double)(choice == (double)3)) {
    printf("%s\n", "Выбран вариант 3");
    } else {
    printf("%s\n", "Неизвестный вариант");
    }
    printf("%s\n", "=== ТЕСТ 7: Составные операторы ===");
    double val = (double)10;
    val += (double)5;
    printf("%g\n", val);
    val -= (double)3;
    printf("%g\n", val);
    val *= (double)2;
    printf("%g\n", val);
    val /= (double)4;
    printf("%g\n", val);
    val = (double)17;
    val = fmod(val, (double)5);
    printf("%g\n", val);
    val = (double)17;
    val = floor(val / (double)5);
    printf("%g\n", val);
    printf("%s\n", "=== ТЕСТ 8: Вложенные конструкции ===");
    double row;
    for (row = (double)1; row <= (double)3; row++) {
    double col;
    for (col = (double)1; col <= (double)3; col++) {
    if ((double)(row == col)) {
    printf("%s\n", "X");
    } else {
    printf("%s\n", "O");
    }
    }
    }
    printf("%s\n", "=== ТЕСТ 9: Сложные выражения ===");
    double result = ((double)2 + (double)3) * (double)4 - (double)10 / (double)2;
    printf("%g\n", result);
    double check = (double)((double)10 > (double)5);
    printf("%g\n", check);
    printf("%s\n", "=== ТЕСТ 10: Граничные случаи ===");
    double zero = (double)0;
    double negative = (-(double)5);
    printf("%g\n", zero);
    printf("%g\n", negative);
    if ((double)(zero == (double)0)) {
    printf("%s\n", "Ноль равен нулю");
    }
    if ((double)(negative < (double)0)) {
    printf("%s\n", "Отрицательное число");
    }
    printf("%s\n", "=== ВСЕ ТЕСТЫ ЗАВЕРШЕНЫ ===");
    return 0;
}
