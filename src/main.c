#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

char* read_fox_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Ошибка: Не удалось открыть файл \"%s\".\n", path);
        exit(74);
    }
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);
    char* buffer = (char*)malloc(fileSize + 1);
    if (!buffer) {
        fprintf(stderr, "Ошибка: Недостаточно памяти.\n");
        exit(74);
    }
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    buffer[bytesRead] = '\0';
    fclose(file);
    return buffer;
}

// Подстановка плейсхолдеров {src} и {out} в шаблон команды C-компилятора
static void substitute(char* dst, size_t cap, const char* tpl,
                       const char* src, const char* out) {
    dst[0] = '\0';
    const char* p = tpl;
    while (*p) {
        if (strncmp(p, "{src}", 5) == 0) {
            strncat(dst, src, cap - strlen(dst) - 1);
            p += 5;
        } else if (strncmp(p, "{out}", 5) == 0) {
            strncat(dst, out, cap - strlen(dst) - 1);
            p += 5;
        } else {
            size_t cur = strlen(dst);
            if (cur + 1 < cap) {
                dst[cur] = *p;
                dst[cur + 1] = '\0';
            }
            p++;
        }
    }
}

// Команда системного C-компилятора. По умолчанию gcc;
// переопределяется переменной окружения FOXC_CC, например:
//   set FOXC_CC=cl /nologo {src} /Fe{out}
static void build_cc_command(char* dst, size_t cap, const char* src, const char* out) {
    const char* tpl = getenv("FOXC_CC");
    if (!tpl || !*tpl) tpl = "gcc \"{src}\" -o \"{out}\" -lm";
    substitute(dst, cap, tpl, src, out);
}

int main(int argc, char* argv[]) {
    printf("--- [Fox Compiler] v1.0.0 ---\n");

    if (argc < 2) {
        printf("Использование: foxc <имя_файла.fox>\n");
        return 64;
    }

    const char* filename = argv[1];
    const char* ext = strrchr(filename, '.');
    if (!ext || strcmp(ext, ".fox") != 0) {
        fprintf(stderr, "Ошибка: Компилятор работает только с файлами формата .fox\n");
        return 65;
    }

    char* source_code = read_fox_file(filename);

    // Имя для Си-файла: path/name.fox -> path/name.c
    char output_c_path[4096];
    memset(output_c_path, 0, sizeof(output_c_path));
    strncpy(output_c_path, filename, sizeof(output_c_path) - 1);
    char* ext_pos = strrchr(output_c_path, '.');
    if (ext_pos) {
        strcpy(ext_pos, ".c");
    }

    // Имя для финального бинарника: path/name.fox -> path/name_app[.exe]
    char output_app_path[4096];
    memset(output_app_path, 0, sizeof(output_app_path));
    strncpy(output_app_path, filename, sizeof(output_app_path) - 1);
    char* app_ext_pos = strrchr(output_app_path, '.');
    if (app_ext_pos) {
        *app_ext_pos = '\0';
    }
#ifdef _WIN32
    strcat(output_app_path, "_app.exe");
#else
    strcat(output_app_path, "_app");
#endif

    // 1. Транспиляция в Си
    int compile_result = compile(source_code, output_c_path);

    // Если была ошибка — C-компилятор не вызываем
    if (compile_result != 0) {
        fprintf(stderr, "Транспиляция завершилась с ошибками. C-компилятор не вызывается.\n");
        free(source_code);
        return 1;
    }

    // 2. Вызов системного C-компилятора (gcc по умолчанию, либо FOXC_CC)
    char cc_command[8192];
    build_cc_command(cc_command, sizeof(cc_command), output_c_path, output_app_path);
    printf("Запуск системного C-компилятора...\n");

    int cc_result = system(cc_command);

    if (cc_result == 0) {
        printf("Сборка нативного бинарника успешна: %s\n", output_app_path);
    } else {
        fprintf(stderr, "Ошибка: системный C-компилятор не смог собрать файл.\n");
        free(source_code);
        return 1;
    }

    free(source_code);
    return 0;
}