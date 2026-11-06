#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

// Прототипы статических обработчиков (statement() объявлена в parser.h)
static void statement_if(void);
static void statement_do(void);
static void statement_while(void);
static void statement_for_each(void);
static void statement_for(void);
static void statement_print(void);
static void statement_local(void);
static void statement_return(void);
static void statement_element_assign(Token id);
static void statement_modification(TokenType op);

// ---------- if / elif / else / endif ----------
static void statement_if(void) {
    advance_parser();
    Buf cond;
    buf_init(&cond);
    TokenType ct;
    parse_expression(&cond, &ct);
    buf_appendf(parser.out, "    if (%s) {\n", cond.data);
    buf_free(&cond);

    while (parser.current.type != TOKEN_ELIF &&
           parser.current.type != TOKEN_ELSE &&
           parser.current.type != TOKEN_ENDIF &&
           parser.current.type != TOKEN_EOF &&
           !parser.has_error) {
        statement();
    }

    if (parser.current.type == TOKEN_ELIF) {
        buf_appendf(parser.out, "    } else ");
        statement_if();
    } else if (parser.current.type == TOKEN_ELSE) {
        advance_parser();
        buf_appendf(parser.out, "    } else {\n");
        while (parser.current.type != TOKEN_ENDIF && parser.current.type != TOKEN_EOF && !parser.has_error) {
            statement();
        }
        consume(TOKEN_ENDIF, "Ожидалось ключевое слово 'endif' для закрытия блока else.");
        buf_appendf(parser.out, "    }\n");
    } else {
        consume(TOKEN_ENDIF, "Ожидалось ключевое слово 'endif' для закрытия блока if.");
        buf_appendf(parser.out, "    }\n");
    }
}

// ---------- do case / case / otherwise / endcase ----------
static void statement_do(void) {
    advance_parser();
    consume(TOKEN_CASE, "Ожидалось ключевое слово 'case' после 'do' в конструкции 'do case'.");
    int is_first_case = 1;

    while (parser.current.type != TOKEN_ENDCASE && parser.current.type != TOKEN_EOF && !parser.has_error) {
        if (parser.current.type == TOKEN_CASE) {
            advance_parser();
            if (is_first_case) { buf_appendf(parser.out, "    if ("); is_first_case = 0; }
            else { buf_appendf(parser.out, "    } else if ("); }
            Buf cond;
            buf_init(&cond);
            TokenType ct;
            parse_expression(&cond, &ct);
            buf_appendf(parser.out, "%s) {\n", cond.data);
            buf_free(&cond);
            while (parser.current.type != TOKEN_CASE && parser.current.type != TOKEN_OTHERWISE &&
                   parser.current.type != TOKEN_ENDCASE && parser.current.type != TOKEN_EOF && !parser.has_error) {
                statement();
            }
        }
        else if (parser.current.type == TOKEN_OTHERWISE) {
            advance_parser();
            if (is_first_case) { buf_appendf(parser.out, "    if (1) {\n"); is_first_case = 0; }
            else { buf_appendf(parser.out, "    } else {\n"); }
            while (parser.current.type != TOKEN_ENDCASE && parser.current.type != TOKEN_EOF && !parser.has_error) {
                statement();
            }
        }
    }

    consume(TOKEN_ENDCASE, "Ожидалось ключевое слово 'endcase' для завершения блока do case.");
    buf_appendf(parser.out, "    }\n");
}

// ---------- while / endwhile ----------
static void statement_while(void) {
    advance_parser();
    Buf cond;
    buf_init(&cond);
    TokenType ct;
    parse_expression(&cond, &ct);
    buf_appendf(parser.out, "    while (%s) {\n", cond.data);
    buf_free(&cond);

    while (parser.current.type != TOKEN_ENDWHILE && parser.current.type != TOKEN_EOF && !parser.has_error) {
        if (parser.current.type == TOKEN_ENDWHILE) break;
        statement();
    }

    consume(TOKEN_ENDWHILE, "Ожидалось ключевое слово 'endwhile' для завершения цикла while.");
    buf_appendf(parser.out, "    }\n");
}

// ---------- for each <элемент> in <список> / endfor ----------
static void statement_for_each(void) {
    advance_parser(); // 'each'
    Token var_name = parser.current;
    consume(TOKEN_IDENTIFIER, "Ожидалось имя переменной элемента после 'for each'.");
    consume(TOKEN_IN, "Ожидалось ключевое слово 'in' после имени переменной элемента.");
    Token list_name = parser.current;
    consume(TOKEN_IDENTIFIER, "Ожидалось имя списка после 'in'.");

    int list_index = find_variable(list_name);
    if (list_index == -1) {
        fprintf(stderr, "[Строка %d] Ошибка: Список '%.*s' не объявлен.\n",
                list_name.line, list_name.length, list_name.start);
        parser.has_error = 1;
        return;
    }
    TokenType list_type = symbol_table_type(list_index);
    if (!is_list_type(list_type)) {
        fprintf(stderr, "[Строка %d] Ошибка: 'for each' применим только к спискам, а '%.*s' — не список.\n",
                list_name.line, list_name.length, list_name.start);
        parser.has_error = 1;
        return;
    }
    TokenType elem_type = (list_type == TOKEN_LIST_NUMBER) ? TOKEN_NUMBER : TOKEN_STRING;
    add_variable(var_name, elem_type);

    static int each_counter = 0;
    int tag = each_counter++;
    buf_appendf(parser.out, "    for (int fox_each_i_%d = 0; fox_each_i_%d < %.*s.len; fox_each_i_%d++) {\n",
                tag, tag, list_name.length, list_name.start, tag);
    if (elem_type == TOKEN_NUMBER) {
        buf_appendf(parser.out, "        double %.*s = %.*s.data[fox_each_i_%d];\n",
                    var_name.length, var_name.start, list_name.length, list_name.start, tag);
    } else {
        buf_appendf(parser.out, "        const char* %.*s = %.*s.data[fox_each_i_%d];\n",
                    var_name.length, var_name.start, list_name.length, list_name.start, tag);
    }

    while (parser.current.type != TOKEN_ENDFOR && parser.current.type != TOKEN_EOF && !parser.has_error) {
        if (parser.current.type == TOKEN_ENDFOR) break;
        statement();
    }

    consume(TOKEN_ENDFOR, "Ожидалось ключевое слово 'endfor' для завершения цикла for each.");
    buf_appendf(parser.out, "    }\n");
}

// ---------- for i = a to b / endfor ----------
static void statement_for(void) {
    advance_parser(); // 'for'
    if (parser.current.type == TOKEN_EACH) {
        statement_for_each();
        return;
    }
    Token var_name = parser.current;
    consume(TOKEN_IDENTIFIER, "Ожидалось имя переменной цикла после 'for'.");
    add_variable(var_name, TOKEN_NUMBER);
    consume(TOKEN_EQUAL, "Ожидался знак '=' после имени переменной в цикле for.");

    Buf start_b, end_b;
    buf_init(&start_b);
    buf_init(&end_b);
    TokenType t;
    parse_expression(&start_b, &t);
    consume(TOKEN_TO, "Ожидалось ключевое слово 'to' в конструкции цикла for.");
    parse_expression(&end_b, &t);

    buf_appendf(parser.out, "    double %.*s;\n", var_name.length, var_name.start);
    buf_appendf(parser.out, "    for (%.*s = %s; %.*s <= %s; %.*s++) {\n",
                var_name.length, var_name.start, start_b.data,
                var_name.length, var_name.start, end_b.data,
                var_name.length, var_name.start);
    buf_free(&start_b);
    buf_free(&end_b);

    while (parser.current.type != TOKEN_ENDFOR && parser.current.type != TOKEN_EOF && !parser.has_error) {
        if (parser.current.type == TOKEN_ENDFOR) break;
        statement();
    }

    consume(TOKEN_ENDFOR, "Ожидалось ключевое слово 'endfor' для завершения цикла.");
    buf_appendf(parser.out, "    }\n");
}

// ---------- print ----------
static void statement_print(void) {
    advance_parser();
    Buf expr;
    buf_init(&expr);
    TokenType t;
    parse_expression(&expr, &t);
    if (parser.has_error) {
        buf_free(&expr);
        return;
    }
    if (is_list_type(t)) {
        fprintf(stderr, "[Строка %d] Ошибка: Список нельзя напечатать напрямую. Используй count() или цикл.\n",
                parser.previous.line);
        parser.has_error = 1;
        buf_free(&expr);
        return;
    }
    if (t == TOKEN_STRING) {
        buf_appendf(parser.out, "    printf(\"%%s\\n\", %s);\n", expr.data);
    } else {
        buf_appendf(parser.out, "    printf(\"%%g\\n\", %s);\n", expr.data);
    }
    buf_free(&expr);
}

// ---------- local ----------
static void statement_local(void) {
    advance_parser();
    Token var_name = parser.current;
    consume(TOKEN_IDENTIFIER, "Ожидалось имя переменной после 'local'.");
    
    // Проверяем наличие явной аннотации типа: local имя: тип = значение
    TokenType annotated_type = TOKEN_EOF; // TOKEN_EOF означает "нет аннотации"
    if (parser.current.type == TOKEN_COLON) {
        advance_parser(); // съедаем ':'
        
        // Читаем базовый тип: number или string
        if (parser.current.type == TOKEN_TYPE_NUMBER) {
            annotated_type = TOKEN_NUMBER;
            advance_parser();
        } else if (parser.current.type == TOKEN_TYPE_STRING) {
            annotated_type = TOKEN_STRING;
            advance_parser();
        } else {
            fprintf(stderr, "[Строка %d] Ошибка: Ожидался тип 'number' или 'string' после ':'.\n",
                    parser.current.line);
            parser.has_error = 1;
            return;
        }
        
        // Проверяем модификатор list: number list или string list
        if (parser.current.type == TOKEN_LIST) {
            advance_parser();
            if (annotated_type == TOKEN_NUMBER) {
                annotated_type = TOKEN_LIST_NUMBER;
            } else {
                annotated_type = TOKEN_LIST_STRING;
            }
        }
    }
    
    consume(TOKEN_EQUAL, "Ожидался знак '=' при объявлении переменной.");

    Buf expr;
    buf_init(&expr);
    TokenType t;
    parse_expression(&expr, &t);
    if (parser.has_error) {
        buf_free(&expr);
        return;
    }
    
    // Если есть аннотация — проверяем совместимость типов
    if (annotated_type != TOKEN_EOF) {
        if (annotated_type != t) {
            // Специальный случай: пустой список [] по умолчанию получает тип
            // number list, но аннотация может задать string list.
            // Разрешаем и перегенерируем конструктор под нужный тип.
            if (is_list_type(annotated_type) && is_list_type(t) &&
                strcmp(expr.data, "fox_num_list_of(0)") == 0) {
                buf_free(&expr);
                buf_init(&expr);
                if (annotated_type == TOKEN_LIST_STRING) {
                    buf_appendf(&expr, "fox_str_list_of(0)");
                } else {
                    buf_appendf(&expr, "fox_num_list_of(0)");
                }
            } else {
                fprintf(stderr, "[Строка %d] ЖЕСТКАЯ ОШИБКА ТИПИЗАЦИИ: Аннотация типа не совпадает с типом значения.\n",
                        var_name.line);
                parser.has_error = 1;
                buf_free(&expr);
                return;
            }
        }
    } else {
        // Без аннотации — используем выведенный тип
        annotated_type = t;
    }
    
    add_variable(var_name, annotated_type);
    
    if (annotated_type == TOKEN_STRING) {
        buf_appendf(parser.out, "    const char* %.*s = %s;\n",
                    var_name.length, var_name.start, expr.data);
    } else if (annotated_type == TOKEN_LIST_NUMBER) {
        buf_appendf(parser.out, "    FoxNumList %.*s = fox_num_list_clone(%s);\n",
                    var_name.length, var_name.start, expr.data);
    } else if (annotated_type == TOKEN_LIST_STRING) {
        buf_appendf(parser.out, "    FoxStrList %.*s = fox_str_list_clone(%s);\n",
                    var_name.length, var_name.start, expr.data);
    } else {
        buf_appendf(parser.out, "    double %.*s = %s;\n",
                    var_name.length, var_name.start, expr.data);
    }
    buf_free(&expr);
}

// ---------- return (шаг 5: возврат списков разрешён) ----------
static void statement_return(void) {
    advance_parser();
    if (!current_function) {
        fprintf(stderr, "[Строка %d] Ошибка: 'return' вне функции.\n", parser.current.line);
        parser.has_error = 1;
        return;
    }
    Buf expr;
    buf_init(&expr);
    TokenType t;
    parse_expression(&expr, &t);
    if (parser.has_error) {
        buf_free(&expr);
        return;
    }
    if (current_function->has_return && current_function->return_type != t) {
        fprintf(stderr, "[Строка %d] Ошибка: Функция '%s' возвращает значения разных типов.\n",
                parser.current.line, current_function->name);
        parser.has_error = 1;
        buf_free(&expr);
        return;
    }
    current_function->has_return = 1;
    current_function->return_type = t;
    buf_appendf(parser.out, "    return %s;\n", expr.data);
    buf_free(&expr);
}

// ---------- присваивание элементу списка: a[i] = значение ----------
static void statement_element_assign(Token id) {
    int index = find_variable(id);
    if (index == -1) {
        fprintf(stderr, "[Строка %d] Ошибка: Переменная '%.*s' не объявлена.\n",
                id.line, id.length, id.start);
        parser.has_error = 1;
        return;
    }
    TokenType list_type = symbol_table_type(index);
    if (!is_list_type(list_type)) {
        fprintf(stderr, "[Строка %d] Ошибка: Индексация применима только к спискам, а '%.*s' — не список.\n",
                id.line, id.length, id.start);
        parser.has_error = 1;
        return;
    }

    advance_parser(); // '['
    Buf idx;
    buf_init(&idx);
    TokenType idx_type;
    parse_expression(&idx, &idx_type);
    if (idx_type != TOKEN_NUMBER) {
        fprintf(stderr, "[Строка %d] Ошибка: Индекс списка должен быть числом.\n", parser.previous.line);
        parser.has_error = 1;
    }
    consume(TOKEN_RIGHT_BRACKET, "Ожидалась закрывающая скобка ']'.");
    consume(TOKEN_EQUAL, "Ожидался знак '=' после a[i] (сокращённые операторы для элементов пока не поддерживаются).");

    Buf val;
    buf_init(&val);
    TokenType val_type;
    parse_expression(&val, &val_type);

    TokenType elem_type = (list_type == TOKEN_LIST_NUMBER) ? TOKEN_NUMBER : TOKEN_STRING;
    if (val_type != elem_type) {
        fprintf(stderr, "[Строка %d] ЖЕСТКАЯ ОШИБКА ТИПИЗАЦИИ: Тип элемента списка и тип присваиваемого значения не совпадают.\n",
                parser.current.line);
        parser.has_error = 1;
    }

    if (list_type == TOKEN_LIST_NUMBER) {
        buf_appendf(parser.out, "    fox_num_list_set(&%.*s, %s, %s);\n",
                    id.length, id.start, idx.data, val.data);
    } else {
        buf_appendf(parser.out, "    fox_str_list_set(&%.*s, %s, %s);\n",
                    id.length, id.start, idx.data, val.data);
    }
    buf_free(&idx);
    buf_free(&val);
}

// ---------- присваивание и сокращённые операторы ----------
static void statement_modification(TokenType op) {
    Token var_name = parser.previous;
    int index = find_variable(var_name);
    if (index == -1) {
        fprintf(stderr, "[Строка %d] Ошибка: Переменная '%.*s' не объявлена.\n",
                var_name.line, var_name.length, var_name.start);
        parser.has_error = 1;
        return;
    }
    advance_parser();

    Buf expr;
    buf_init(&expr);
    TokenType t;
    parse_expression(&expr, &t);
    if (parser.has_error) {
        buf_free(&expr);
        return;
    }

    TokenType var_type = symbol_table_type(index);

    if (op != TOKEN_EQUAL && (var_type != TOKEN_NUMBER || t != TOKEN_NUMBER)) {
        fprintf(stderr, "[Строка %d] Ошибка: Сокращенные операторы применимы только к Числам!\n",
                parser.current.line);
        parser.has_error = 1;
        buf_free(&expr);
        return;
    }

    if (op == TOKEN_EQUAL) {
        if (is_list_type(var_type) || is_list_type(t)) {
            if (var_type != t) {
                fprintf(stderr, "[Строка %d] ЖЕСТКАЯ ОШИБКА ТИПИЗАЦИИ: Нельзя присвоить значение другого типа переменной '%.*s'.\n",
                        parser.current.line, var_name.length, var_name.start);
                parser.has_error = 1;
                buf_free(&expr);
                return;
            }
            if (var_type == TOKEN_LIST_NUMBER) {
                buf_appendf(parser.out, "    %.*s = fox_num_list_clone(%s);\n",
                            var_name.length, var_name.start, expr.data);
            } else {
                buf_appendf(parser.out, "    %.*s = fox_str_list_clone(%s);\n",
                            var_name.length, var_name.start, expr.data);
            }
            buf_free(&expr);
            return;
        }
        if (var_type == TOKEN_NUMBER && t == TOKEN_STRING) {
            fprintf(stderr, "[Строка %d] ЖЕСТКАЯ ОШИБКА ТИПИЗАЦИИ: Переменная '%.*s' (Число). Нельзя присвоить Строку!\n",
                    parser.current.line, var_name.length, var_name.start);
            parser.has_error = 1;
            buf_free(&expr);
            return;
        }
        if (var_type == TOKEN_STRING && t == TOKEN_NUMBER) {
            fprintf(stderr, "[Строка %d] ЖЕСТКАЯ ОШИБКА ТИПИЗАЦИИ: Переменная '%.*s' (Строка). Нельзя присвоить Число!\n",
                    parser.current.line, var_name.length, var_name.start);
            parser.has_error = 1;
            buf_free(&expr);
            return;
        }
    }

    if (op == TOKEN_EQUAL) {
        buf_appendf(parser.out, "    %.*s = %s;\n", var_name.length, var_name.start, expr.data);
    } else if (op == TOKEN_PLUS_EQUAL) {
        buf_appendf(parser.out, "    %.*s += %s;\n", var_name.length, var_name.start, expr.data);
    } else if (op == TOKEN_MINUS_EQUAL) {
        buf_appendf(parser.out, "    %.*s -= %s;\n", var_name.length, var_name.start, expr.data);
    } else if (op == TOKEN_STAR_EQUAL) {
        buf_appendf(parser.out, "    %.*s *= %s;\n", var_name.length, var_name.start, expr.data);
    } else if (op == TOKEN_SLASH_EQUAL) {
        buf_appendf(parser.out, "    %.*s /= %s;\n", var_name.length, var_name.start, expr.data);
    } else if (op == TOKEN_PERCENT_EQUAL) {
        buf_appendf(parser.out, "    %.*s = fmod(%.*s, %s);\n",
                    var_name.length, var_name.start, var_name.length, var_name.start, expr.data);
    } else if (op == TOKEN_DSLASH_EQUAL) {
        buf_appendf(parser.out, "    %.*s = floor(%.*s / %s);\n",
                    var_name.length, var_name.start, var_name.length, var_name.start, expr.data);
    }

    buf_free(&expr);
}

// ---------- диспетчер операторов ----------
void statement(void) {
    if (parser.current.type == TOKEN_PRINT) {
        statement_print();
    } else if (parser.current.type == TOKEN_LOCAL) {
        statement_local();
    } else if (parser.current.type == TOKEN_RETURN) {
        statement_return();
    } else if (parser.current.type == TOKEN_IF) {
        statement_if();
    } else if (parser.current.type == TOKEN_DO) {
        statement_do();
    } else if (parser.current.type == TOKEN_WHILE) {
        statement_while();
    } else if (parser.current.type == TOKEN_FOR) {
        statement_for();
    } else if (parser.current.type == TOKEN_IDENTIFIER) {
        Token id = parser.current;
        advance_parser();
        if (parser.current.type == TOKEN_LEFT_PAREN) {
            // Вызов как оператор: встроенная (в т.ч. void-операторы вроде append)
            // или пользовательская функция
            Buf args[MAX_ARGS];
            TokenType atypes[MAX_ARGS];
            int argc = parse_args_raw(args, atypes);
            const Builtin* bi = find_builtin_match(id.start, id.length, argc, atypes);
            Function* fn = find_function(id.start, id.length);
            Buf call;
            buf_init(&call);
            if (bi) {
                Buf joined;
                buf_init(&joined);
                join_args(&joined, argc, args);
                buf_appendf(&call, bi->c_template, joined.data);
                buf_free(&joined);
            } else if (fn) {
                if (argc != fn->param_count) {
                    fprintf(stderr, "[Строка %d] Ошибка: '%s' ожидает %d аргументов, передано %d.\n",
                            id.line, fn->name, fn->param_count, argc);
                    parser.has_error = 1;
                }
                Buf joined;
                buf_init(&joined);
                for (int i = 0; i < argc && i < fn->param_count; i++) {
                    if (fn->param_types[i] != atypes[i]) {
                        fprintf(stderr, "[Строка %d] Ошибка: Аргумент %d '%s' должен быть типа %s.\n",
                                id.line, i + 1, fn->name,
                                fn->param_types[i] == TOKEN_STRING ? "string" : "number");
                        parser.has_error = 1;
                    }
                    if (i > 0) buf_appendf(&joined, ", ");
                    buf_appendf(&joined, "%s", args[i].data);
                }
                buf_appendf(&call, "%s(%s)", fn->name, joined.data);
                buf_free(&joined);
            } else if (builtin_name_taken(id.start, id.length)) {
                fprintf(stderr, "[Строка %d] Ошибка: Встроенная функция '%.*s' не принимает таких типов/числа аргументов.\n",
                        id.line, id.length, id.start);
                parser.has_error = 1;
                for (int i = 0; i < argc; i++) buf_free(&args[i]);
                buf_free(&call);
                return;
            } else {
                fprintf(stderr, "[Строка %d] Ошибка: Неизвестная функция '%.*s'.\n",
                        id.line, id.length, id.start);
                parser.has_error = 1;
                for (int i = 0; i < argc; i++) buf_free(&args[i]);
                buf_free(&call);
                return;
            }
            buf_appendf(parser.out, "    %s;\n", call.data);
            buf_free(&call);
            for (int i = 0; i < argc; i++) buf_free(&args[i]);
        } else if (parser.current.type == TOKEN_LEFT_BRACKET) {
            // Присваивание элементу списка: a[i] = значение
            statement_element_assign(id);
        } else {
            TokenType op = parser.current.type;
            if (op == TOKEN_EQUAL || op == TOKEN_PLUS_EQUAL || op == TOKEN_MINUS_EQUAL ||
                op == TOKEN_STAR_EQUAL || op == TOKEN_SLASH_EQUAL ||
                op == TOKEN_PERCENT_EQUAL || op == TOKEN_DSLASH_EQUAL) {
                statement_modification(op);
            } else {
                fprintf(stderr, "[Строка %d] Ошибка: Неожиданный токен после идентификатора '%.*s'.\n",
                        id.line, id.length, id.start);
                parser.has_error = 1;
            }
        }
    } else {
        fprintf(stderr, "[Строка %d] Ошибка: Неожиданный токен '%.*s'.\n",
                parser.current.line, parser.current.length, parser.current.start);
        parser.has_error = 1;
        advance_parser();
    }
}