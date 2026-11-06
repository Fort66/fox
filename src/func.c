#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

// Разбор определения функции:
//   func имя(тип параметр, ...) ... endfunc
// Типы параметров: number | string | number list | string list
void parse_function(Buf* funcs) {
    advance_parser(); // 'func'
    Token fname = parser.current;
    consume(TOKEN_IDENTIFIER, "Ожидалось имя функции после 'func'.");
    if (parser.has_error) return;
    if (find_function(fname.start, fname.length)) {
        fprintf(stderr, "[Строка %d] Ошибка: Функция '%.*s' уже определена.\n",
                fname.line, fname.length, fname.start);
        parser.has_error = 1;
        return;
    }
    if (builtin_name_taken(fname.start, fname.length)) {
        fprintf(stderr, "[Строка %d] Ошибка: Имя '%.*s' зарезервировано встроенной функцией.\n",
                fname.line, fname.length, fname.start);
        parser.has_error = 1;
        return;
    }
    consume(TOKEN_LEFT_PAREN, "Ожидалась '(' после имени функции.");

    Function* fn = add_function_slot();
    if (!fn) {
        fprintf(stderr, "Ошибка: Превышен лимит функций.\n");
        parser.has_error = 1;
        return;
    }
    snprintf(fn->name, MAX_NAME, "%.*s", fname.length, fname.start);

    int saved_vars = variable_count;

    // Параметры
    if (parser.current.type != TOKEN_RIGHT_PAREN) {
        for (;;) {
            // Разбор типа. После этого блока parser.current указывает ровно
            // на имя параметра, поэтому дополнительный advance здесь НЕ нужен.
            TokenType ptype;
            if (parser.current.type == TOKEN_TYPE_NUMBER ||
                parser.current.type == TOKEN_TYPE_STRING) {
                TokenType base = (parser.current.type == TOKEN_TYPE_NUMBER)
                                 ? TOKEN_NUMBER : TOKEN_STRING;
                advance_parser();
                if (parser.current.type == TOKEN_LIST) {
                    advance_parser();
                    ptype = (base == TOKEN_NUMBER) ? TOKEN_LIST_NUMBER : TOKEN_LIST_STRING;
                } else {
                    ptype = base;
                }
            } else {
                consume(TOKEN_TYPE_NUMBER,
                        "Ожидался тип параметра: number, string, number list или string list.");
                variable_count = saved_vars;
                return;
            }

            Token pname = parser.current;
            consume(TOKEN_IDENTIFIER, "Ожидалось имя параметра функции.");
            if (parser.has_error) { variable_count = saved_vars; return; }
            if (fn->param_count >= MAX_PARAMS) {
                fprintf(stderr, "[Строка %d] Ошибка: Слишком много параметров (максимум %d).\n",
                        pname.line, MAX_PARAMS);
                parser.has_error = 1;
                variable_count = saved_vars;
                return;
            }
            fn->param_types[fn->param_count] = ptype;
            snprintf(fn->param_names[fn->param_count], MAX_NAME, "%.*s", pname.length, pname.start);
            fn->param_count++;
            add_variable(pname, ptype);
            if (parser.current.type == TOKEN_COMMA) {
                advance_parser();
                continue;
            }
            break;
        }
    }
    consume(TOKEN_RIGHT_PAREN, "Ожидалась ')' после списка параметров.");

    // Тело в отдельный буфер
    Buf body;
    buf_init(&body);

    // Клон-прелюдия: списочные параметры получают собственную копию,
    // чтобы изменения внутри функции не затрагивали список вызывающего.
    for (int i = 0; i < fn->param_count; i++) {
        if (fn->param_types[i] == TOKEN_LIST_NUMBER) {
            buf_appendf(&body, "    %s = fox_num_list_clone(%s);\n",
                        fn->param_names[i], fn->param_names[i]);
        } else if (fn->param_types[i] == TOKEN_LIST_STRING) {
            buf_appendf(&body, "    %s = fox_str_list_clone(%s);\n",
                        fn->param_names[i], fn->param_names[i]);
        }
    }

    Buf* saved_out = parser.out;
    Function* saved_current = current_function;
    parser.out = &body;
    current_function = fn;

    while (parser.current.type != TOKEN_ENDFUNC && parser.current.type != TOKEN_EOF && !parser.has_error) {
        statement();
    }
    consume(TOKEN_ENDFUNC, "Ожидалось ключевое слово 'endfunc' для завершения функции.");

    parser.out = saved_out;
    current_function = saved_current;
    variable_count = saved_vars;

    const char* ret_c = fn->has_return ? c_type(fn->return_type) : "void";

    // Прототип
    buf_appendf(funcs, "static %s %s(", ret_c, fn->name);
    for (int i = 0; i < fn->param_count; i++) {
        if (i > 0) buf_appendf(funcs, ", ");
        buf_appendf(funcs, "%s %s", c_type(fn->param_types[i]), fn->param_names[i]);
    }
    buf_appendf(funcs, ");\n");

    // Определение
    buf_appendf(funcs, "static %s %s(", ret_c, fn->name);
    for (int i = 0; i < fn->param_count; i++) {
        if (i > 0) buf_appendf(funcs, ", ");
        buf_appendf(funcs, "%s %s", c_type(fn->param_types[i]), fn->param_names[i]);
    }
    buf_appendf(funcs, ") {\n");
    buf_appendf(funcs, "%s", body.data);
    buf_appendf(funcs, "}\n\n");
    buf_free(&body);
}