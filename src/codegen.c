#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

// ---------- Заголовки сгенерированного C-файла ----------
static void emit_includes(FILE* f) {
    fprintf(f, "#include <stdio.h>\n");
    fprintf(f, "#include <math.h>\n");
    fprintf(f, "#include <string.h>\n");
    fprintf(f, "#include <stdlib.h>\n");
    fprintf(f, "#include <ctype.h>\n");
    fprintf(f, "#include <stdarg.h>\n\n");
}

// ---------- Строковый рантайм ----------
static void emit_string_runtime(FILE* f) {
    fprintf(f, "static char* fox_concat(const char* a, const char* b) {\n");
    fprintf(f, "    size_t la = strlen(a), lb = strlen(b);\n");
    fprintf(f, "    char* r = (char*)malloc(la + lb + 1);\n");
    fprintf(f, "    memcpy(r, a, la);\n");
    fprintf(f, "    memcpy(r + la, b, lb + 1);\n");
    fprintf(f, "    return r;\n");
    fprintf(f, "}\n\n");
    fprintf(f, "static char* fox_upper(const char* s) {\n");
    fprintf(f, "    size_t n = strlen(s);\n");
    fprintf(f, "    char* r = (char*)malloc(n + 1);\n");
    fprintf(f, "    for (size_t i = 0; i < n; i++) r[i] = (char)toupper((unsigned char)s[i]);\n");
    fprintf(f, "    r[n] = '\\0';\n");
    fprintf(f, "    return r;\n");
    fprintf(f, "}\n\n");
    fprintf(f, "static char* fox_lower(const char* s) {\n");
    fprintf(f, "    size_t n = strlen(s);\n");
    fprintf(f, "    char* r = (char*)malloc(n + 1);\n");
    fprintf(f, "    for (size_t i = 0; i < n; i++) r[i] = (char)tolower((unsigned char)s[i]);\n");
    fprintf(f, "    r[n] = '\\0';\n");
    fprintf(f, "    return r;\n");
    fprintf(f, "}\n\n");
    fprintf(f, "static char* fox_trim(const char* s) {\n");
    fprintf(f, "    while (*s == ' ' || *s == '\\t' || *s == '\\r' || *s == '\\n') s++;\n");
    fprintf(f, "    size_t n = strlen(s);\n");
    fprintf(f, "    while (n > 0 && (s[n-1] == ' ' || s[n-1] == '\\t' || s[n-1] == '\\r' || s[n-1] == '\\n')) n--;\n");
    fprintf(f, "    char* r = (char*)malloc(n + 1);\n");
    fprintf(f, "    memcpy(r, s, n);\n");
    fprintf(f, "    r[n] = '\\0';\n");
    fprintf(f, "    return r;\n");
    fprintf(f, "}\n\n");
    fprintf(f, "static char* fox_substr(const char* s, double start_d, double count_d) {\n");
    fprintf(f, "    long start = (long)start_d;\n");
    fprintf(f, "    long count = (long)count_d;\n");
    fprintf(f, "    size_t n = strlen(s);\n");
    fprintf(f, "    if (start < 1) start = 1;\n");
    fprintf(f, "    size_t idx = (size_t)(start - 1);\n");
    fprintf(f, "    if (idx > n) idx = n;\n");
    fprintf(f, "    size_t avail = n - idx;\n");
    fprintf(f, "    size_t take = (count < 0) ? avail : ((size_t)count < avail ? (size_t)count : avail);\n");
    fprintf(f, "    char* r = (char*)malloc(take + 1);\n");
    fprintf(f, "    memcpy(r, s + idx, take);\n");
    fprintf(f, "    r[take] = '\\0';\n");
    fprintf(f, "    return r;\n");
    fprintf(f, "}\n\n");
    fprintf(f, "static char* fox_str(double v) {\n");
    fprintf(f, "    char* r = (char*)malloc(64);\n");
    fprintf(f, "    snprintf(r, 64, \"%%g\", v);\n");
    fprintf(f, "    return r;\n");
    fprintf(f, "}\n\n");
}

// ---------- Списковый рантайм ----------
static void emit_list_runtime(FILE* f) {
    fprintf(f, "typedef struct { double* data; int len; int cap; } FoxNumList;\n");
    fprintf(f, "typedef struct { const char** data; int len; int cap; } FoxStrList;\n\n");
    fprintf(f, "static FoxNumList fox_num_list_of(int n, ...) {\n");
    fprintf(f, "    FoxNumList l; l.data = NULL; l.len = 0; l.cap = 0;\n");
    fprintf(f, "    va_list ap; va_start(ap, n);\n");
    fprintf(f, "    if (n > 0) {\n");
    fprintf(f, "        l.cap = n; l.len = n;\n");
    fprintf(f, "        l.data = (double*)malloc(sizeof(double) * (size_t)n);\n");
    fprintf(f, "        for (int i = 0; i < n; i++) l.data[i] = va_arg(ap, double);\n");
    fprintf(f, "    }\n");
    fprintf(f, "    va_end(ap);\n");
    fprintf(f, "    return l;\n");
    fprintf(f, "}\n\n");
    fprintf(f, "static FoxStrList fox_str_list_of(int n, ...) {\n");
    fprintf(f, "    FoxStrList l; l.data = NULL; l.len = 0; l.cap = 0;\n");
    fprintf(f, "    va_list ap; va_start(ap, n);\n");
    fprintf(f, "    if (n > 0) {\n");
    fprintf(f, "        l.cap = n; l.len = n;\n");
    fprintf(f, "        l.data = (const char**)malloc(sizeof(const char*) * (size_t)n);\n");
    fprintf(f, "        for (int i = 0; i < n; i++) l.data[i] = va_arg(ap, const char*);\n");
    fprintf(f, "    }\n");
    fprintf(f, "    va_end(ap);\n");
    fprintf(f, "    return l;\n");
    fprintf(f, "}\n\n");
    fprintf(f, "static FoxNumList fox_num_list_clone(FoxNumList src) {\n");
    fprintf(f, "    FoxNumList l = src;\n");
    fprintf(f, "    if (src.len > 0) {\n");
    fprintf(f, "        l.data = (double*)malloc(sizeof(double) * (size_t)src.len);\n");
    fprintf(f, "        memcpy(l.data, src.data, sizeof(double) * (size_t)src.len);\n");
    fprintf(f, "    } else { l.data = NULL; }\n");
    fprintf(f, "    l.cap = src.len;\n");
    fprintf(f, "    return l;\n");
    fprintf(f, "}\n\n");
    fprintf(f, "static FoxStrList fox_str_list_clone(FoxStrList src) {\n");
    fprintf(f, "    FoxStrList l = src;\n");
    fprintf(f, "    if (src.len > 0) {\n");
    fprintf(f, "        l.data = (const char**)malloc(sizeof(const char*) * (size_t)src.len);\n");
    fprintf(f, "        memcpy(l.data, src.data, sizeof(const char*) * (size_t)src.len);\n");
    fprintf(f, "    } else { l.data = NULL; }\n");
    fprintf(f, "    l.cap = src.len;\n");
    fprintf(f, "    return l;\n");
    fprintf(f, "}\n\n");
    fprintf(f, "static double fox_num_list_get(FoxNumList* l, double i) {\n");
    fprintf(f, "    long idx = (long)i;\n");
    fprintf(f, "    if (idx < 1 || idx > l->len) {\n");
    fprintf(f, "        fprintf(stderr, \"Ошибка выполнения: индекс %%ld вне диапазона 1..%%d\\n\", idx, l->len);\n");
    fprintf(f, "        exit(1);\n");
    fprintf(f, "    }\n");
    fprintf(f, "    return l->data[idx - 1];\n");
    fprintf(f, "}\n\n");
    fprintf(f, "static const char* fox_str_list_get(FoxStrList* l, double i) {\n");
    fprintf(f, "    long idx = (long)i;\n");
    fprintf(f, "    if (idx < 1 || idx > l->len) {\n");
    fprintf(f, "        fprintf(stderr, \"Ошибка выполнения: индекс %%ld вне диапазона 1..%%d\\n\", idx, l->len);\n");
    fprintf(f, "        exit(1);\n");
    fprintf(f, "    }\n");
    fprintf(f, "    return l->data[idx - 1];\n");
    fprintf(f, "}\n\n");
    fprintf(f, "static void fox_num_list_set(FoxNumList* l, double i, double v) {\n");
    fprintf(f, "    long idx = (long)i;\n");
    fprintf(f, "    if (idx < 1 || idx > l->len) {\n");
    fprintf(f, "        fprintf(stderr, \"Ошибка выполнения: индекс %%ld вне диапазона 1..%%d\\n\", idx, l->len);\n");
    fprintf(f, "        exit(1);\n");
    fprintf(f, "    }\n");
    fprintf(f, "    l->data[idx - 1] = v;\n");
    fprintf(f, "}\n\n");
    fprintf(f, "static void fox_str_list_set(FoxStrList* l, double i, const char* v) {\n");
    fprintf(f, "    long idx = (long)i;\n");
    fprintf(f, "    if (idx < 1 || idx > l->len) {\n");
    fprintf(f, "        fprintf(stderr, \"Ошибка выполнения: индекс %%ld вне диапазона 1..%%d\\n\", idx, l->len);\n");
    fprintf(f, "        exit(1);\n");
    fprintf(f, "    }\n");
    fprintf(f, "    l->data[idx - 1] = v;\n");
    fprintf(f, "}\n\n");
    // Добавление в конец с амортизированным ростом (4c-1)
    fprintf(f, "static void fox_num_list_append(FoxNumList* l, double v) {\n");
    fprintf(f, "    if (l->len == l->cap) {\n");
    fprintf(f, "        int newcap = (l->cap == 0) ? 4 : l->cap * 2;\n");
    fprintf(f, "        l->data = (double*)realloc(l->data, sizeof(double) * (size_t)newcap);\n");
    fprintf(f, "        l->cap = newcap;\n");
    fprintf(f, "    }\n");
    fprintf(f, "    l->data[l->len++] = v;\n");
    fprintf(f, "}\n\n");
    fprintf(f, "static void fox_str_list_append(FoxStrList* l, const char* v) {\n");
    fprintf(f, "    if (l->len == l->cap) {\n");
    fprintf(f, "        int newcap = (l->cap == 0) ? 4 : l->cap * 2;\n");
    fprintf(f, "        l->data = (const char**)realloc(l->data, sizeof(const char*) * (size_t)newcap);\n");
    fprintf(f, "        l->cap = newcap;\n");
    fprintf(f, "    }\n");
    fprintf(f, "    l->data[l->len++] = v;\n");
    fprintf(f, "}\n\n");
}

// ---------- Сборка итогового .c ----------
int compile(const char* source_code, const char* output_c_path) {
    init_lexer(source_code);
    symbols_reset();
    current_function = NULL;
    parser.has_error = 0;

    Buf funcs, main_body;
    buf_init(&funcs);
    buf_init(&main_body);
    parser.out = &main_body;

    advance_parser();
    while (parser.current.type != TOKEN_EOF && !parser.has_error) {
        if (parser.current.type == TOKEN_FUNC) {
            parse_function(&funcs);
        } else {
            statement();
        }
    }

    if (parser.has_error) {
        fprintf(stderr, "Компиляция прервана из-за ошибок.\n");
        buf_free(&funcs);
        buf_free(&main_body);
        remove(output_c_path);
        return 1;
    }

    FILE* f = fopen(output_c_path, "w");
    if (!f) {
        fprintf(stderr, "Ошибка: Не удалось создать выходной файл %s\n", output_c_path);
        buf_free(&funcs);
        buf_free(&main_body);
        return 1;
    }

    emit_includes(f);
    emit_string_runtime(f);
    emit_list_runtime(f);

    fwrite(funcs.data, 1, funcs.len, f);
    fprintf(f, "int main() {\n");
    fwrite(main_body.data, 1, main_body.len, f);
    fprintf(f, "    return 0;\n");
    fprintf(f, "}\n");
    fclose(f);

    buf_free(&funcs);
    buf_free(&main_body);

    printf("Транспиляция успешно завершена. Создан файл: %s\n", output_c_path);
    return 0;
}