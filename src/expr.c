#include <stdio.h>
#include <stdlib.h>
#include "parser.h"

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,
    PREC_OR,
    PREC_AND,
    PREC_COMPARISON,
    PREC_TERM,
    PREC_FACTOR,
    PREC_PRIMARY
} Precedence;

static Precedence get_precedence(TokenType type) {
    switch (type) {
        case TOKEN_EQUAL_EQUAL:
        case TOKEN_BANG_EQUAL:
        case TOKEN_GREATER:
        case TOKEN_GREATER_EQUAL:
        case TOKEN_LESS:
        case TOKEN_LESS_EQUAL:
            return PREC_COMPARISON;
        case TOKEN_PLUS:
        case TOKEN_MINUS:
            return PREC_TERM;
        case TOKEN_STAR:
        case TOKEN_SLASH:
        case TOKEN_DOUBLE_SLASH:
        case TOKEN_PERCENT:
            return PREC_FACTOR;
        case TOKEN_AND:
            return PREC_AND;
        case TOKEN_OR:
            return PREC_OR;
        default:
            return PREC_NONE;
    }
}

static void parse_precedence(Buf* out, Precedence precedence, TokenType* out_type);

void parse_expression(Buf* out, TokenType* out_type) {
    parse_precedence(out, PREC_ASSIGNMENT, out_type);
}

int parse_args_raw(Buf* args, TokenType* atypes) {
    int argc = 0;
    advance_parser(); // '('
    if (parser.current.type != TOKEN_RIGHT_PAREN) {
        for (;;) {
            if (argc >= MAX_ARGS) {
                fprintf(stderr, "[Строка %d] Ошибка: Слишком много аргументов (максимум %d).\n",
                        parser.current.line, MAX_ARGS);
                parser.has_error = 1;
                break;
            }
            buf_init(&args[argc]);
            parse_expression(&args[argc], &atypes[argc]);
            argc++;
            if (parser.current.type == TOKEN_COMMA) {
                advance_parser();
                continue;
            }
            break;
        }
    }
    consume(TOKEN_RIGHT_PAREN, "Ожидалась ')' после аргументов функции.");
    return argc;
}

void join_args(Buf* out, int argc, Buf* args) {
    for (int i = 0; i < argc; i++) {
        if (i > 0) buf_appendf(out, ", ");
        buf_appendf(out, "%s", args[i].data);
    }
}

void parse_arg_list(int param_count, const TokenType* param_types, const char* callee, Buf* out) {
    int argc = 0;
    if (parser.current.type != TOKEN_RIGHT_PAREN) {
        for (;;) {
            Buf arg;
            buf_init(&arg);
            TokenType at;
            parse_expression(&arg, &at);
            if (argc < param_count) {
                if (param_types[argc] != at) {
                    fprintf(stderr, "[Строка %d] Ошибка: Аргумент %d '%s' должен быть типа %s.\n",
                            parser.current.line, argc + 1, callee,
                            param_types[argc] == TOKEN_STRING ? "string" : "number");
                    parser.has_error = 1;
                }
                if (argc > 0) buf_appendf(out, ", ");
                buf_appendf(out, "%s", arg.data);
            }
            buf_free(&arg);
            argc++;
            if (parser.current.type == TOKEN_COMMA) {
                advance_parser();
                continue;
            }
            break;
        }
    }
    consume(TOKEN_RIGHT_PAREN, "Ожидалась ')' после аргументов функции.");
    if (argc != param_count) {
        fprintf(stderr, "[Строка %d] Ошибка: '%s' ожидает %d аргументов, передано %d.\n",
                parser.current.line, callee, param_count, argc);
        parser.has_error = 1;
    }
}

void parse_call(Function* fn, Buf* out) {
    advance_parser(); // '('
    buf_appendf(out, "%s(", fn->name);
    parse_arg_list(fn->param_count, fn->param_types, fn->name, out);
    buf_appendf(out, ")");
}

// ---------- Ядро выражений ----------

static void parse_precedence(Buf* out, Precedence precedence, TokenType* out_type) {
    advance_parser();
    TokenType left_type = TOKEN_NUMBER;
    Token left_token = parser.previous;
    Buf left;
    buf_init(&left);
    int left_lvalue = 0;

    if (left_token.type == TOKEN_MINUS) {
        Buf operand;
        buf_init(&operand);
        parse_precedence(&operand, PREC_FACTOR, &left_type);
        buf_appendf(&left, "(-%s)", operand.data);
        buf_free(&operand);
        left_type = TOKEN_NUMBER;
    }
    else if (left_token.type == TOKEN_NOT) {
        Buf operand;
        buf_init(&operand);
        parse_precedence(&operand, PREC_COMPARISON, &left_type);
        if (left_type != TOKEN_NUMBER) {
            fprintf(stderr, "[Строка %d] Ошибка: 'not' применим только к числам/булевым.\n", left_token.line);
            parser.has_error = 1;
        }
        buf_appendf(&left, "(double)(!%s)", operand.data);
        buf_free(&operand);
        left_type = TOKEN_NUMBER;
    }
    else if (left_token.type == TOKEN_TRUE) {
        buf_appendf(&left, "1.0");
        left_type = TOKEN_NUMBER;
    }
    else if (left_token.type == TOKEN_FALSE) {
        buf_appendf(&left, "0.0");
        left_type = TOKEN_NUMBER;
    }
    else if (left_token.type == TOKEN_LEFT_PAREN) {
        Buf inner;
        buf_init(&inner);
        parse_expression(&inner, &left_type);
        consume(TOKEN_RIGHT_PAREN, "Ожидалась закрывающая скобка ')'.");
        buf_appendf(&left, "(%s)", inner.data);
        buf_free(&inner);
    }
    else if (left_token.type == TOKEN_LEFT_BRACKET) {
        Buf elems;
        buf_init(&elems);
        int n = 0;
        int is_str = 0;
        if (parser.current.type != TOKEN_RIGHT_BRACKET) {
            for (;;) {
                Buf e;
                buf_init(&e);
                TokenType et;
                parse_expression(&e, &et);
                if (is_list_type(et)) {
                    fprintf(stderr, "[Строка %d] Ошибка: Вложенные списки не поддерживаются.\n", left_token.line);
                    parser.has_error = 1;
                }
                if (n == 0) {
                    is_str = (et == TOKEN_STRING);
                } else if ((et == TOKEN_STRING) != (is_str != 0)) {
                    fprintf(stderr, "[Строка %d] Ошибка: Все элементы списка должны быть одного типа.\n", left_token.line);
                    parser.has_error = 1;
                }
                if (n > 0) buf_appendf(&elems, ", ");
                buf_appendf(&elems, "%s", e.data);
                n++;
                buf_free(&e);
                if (parser.current.type == TOKEN_COMMA) {
                    advance_parser();
                    continue;
                }
                break;
            }
        }
        consume(TOKEN_RIGHT_BRACKET, "Ожидалась закрывающая скобка ']'.");
        if (is_str) {
            if (n > 0) buf_appendf(&left, "fox_str_list_of(%d, %s)", n, elems.data);
            else buf_appendf(&left, "fox_str_list_of(0)");
            left_type = TOKEN_LIST_STRING;
        } else {
            if (n > 0) buf_appendf(&left, "fox_num_list_of(%d, %s)", n, elems.data);
            else buf_appendf(&left, "fox_num_list_of(0)");
            left_type = TOKEN_LIST_NUMBER;
        }
        buf_free(&elems);
    }
    else if (left_token.type == TOKEN_NUMBER) {
        buf_appendf(&left, "(double)%.*s", left_token.length, left_token.start);
        left_type = TOKEN_NUMBER;
    }
    else if (left_token.type == TOKEN_STRING) {
        buf_appendf(&left, "%.*s", left_token.length, left_token.start);
        left_type = TOKEN_STRING;
    }
    else if (left_token.type == TOKEN_IDENTIFIER) {
        if (parser.current.type == TOKEN_LEFT_PAREN) {
            Buf args[MAX_ARGS];
            TokenType atypes[MAX_ARGS];
            int argc = parse_args_raw(args, atypes);
            const Builtin* bi = find_builtin_match(left_token.start, left_token.length, argc, atypes);
            if (bi) {
                if (bi->return_type == 0) {
                    fprintf(stderr, "[Строка %d] Ошибка: '%.*s' — это оператор, его нельзя использовать внутри выражения.\n",
                            left_token.line, left_token.length, left_token.start);
                    parser.has_error = 1;
                }
                Buf joined;
                buf_init(&joined);
                join_args(&joined, argc, args);
                buf_appendf(&left, bi->c_template, joined.data);
                buf_free(&joined);
                left_type = bi->return_type;
            } else {
                Function* fn = find_function(left_token.start, left_token.length);
                if (fn) {
                    if (argc != fn->param_count) {
                        fprintf(stderr, "[Строка %d] Ошибка: '%s' ожидает %d аргументов, передано %d.\n",
                                left_token.line, fn->name, fn->param_count, argc);
                        parser.has_error = 1;
                    }
                    Buf joined;
                    buf_init(&joined);
                    for (int i = 0; i < argc && i < fn->param_count; i++) {
                        if (fn->param_types[i] != atypes[i]) {
                            fprintf(stderr, "[Строка %d] Ошибка: Аргумент %d '%s' должен быть типа %s.\n",
                                    left_token.line, i + 1, fn->name,
                                    fn->param_types[i] == TOKEN_STRING ? "string" : "number");
                            parser.has_error = 1;
                        }
                        if (i > 0) buf_appendf(&joined, ", ");
                        buf_appendf(&joined, "%s", args[i].data);
                    }
                    buf_appendf(&left, "%s(%s)", fn->name, joined.data);
                    buf_free(&joined);
                    if (!fn->has_return) {
                        fprintf(stderr, "[Строка %d] Ошибка: Функция '%s' без return не может использоваться в выражении.\n",
                                left_token.line, fn->name);
                        parser.has_error = 1;
                    }
                    left_type = fn->return_type;
                } else if (builtin_name_taken(left_token.start, left_token.length)) {
                    fprintf(stderr, "[Строка %d] Ошибка: Встроенная функция '%.*s' не принимает таких типов/числа аргументов.\n",
                            left_token.line, left_token.length, left_token.start);
                    parser.has_error = 1;
                    left_type = TOKEN_NUMBER;
                } else {
                    fprintf(stderr, "[Строка %d] Ошибка: Неизвестная функция '%.*s'.\n",
                            left_token.line, left_token.length, left_token.start);
                    parser.has_error = 1;
                    left_type = TOKEN_NUMBER;
                }
            }
            for (int i = 0; i < argc; i++) buf_free(&args[i]);
        } else {
            int index = find_variable(left_token);
            if (index == -1) {
                fprintf(stderr, "[Строка %d] Ошибка: Переменная '%.*s' не объявлена.\n",
                        left_token.line, left_token.length, left_token.start);
                parser.has_error = 1;
                buf_free(&left);
                *out_type = TOKEN_NUMBER;
                return;
            }
            buf_appendf(&left, "%.*s", left_token.length, left_token.start);
            left_type = symbol_table_type(index);
            left_lvalue = 1;
        }
    } else {
        fprintf(stderr, "[Строка %d] Ошибка: Ожидалось выражение.\n", left_token.line);
        parser.has_error = 1;
        buf_free(&left);
        *out_type = TOKEN_NUMBER;
        return;
    }

    // Постфиксная индексация списков: a[1], a[i + 1] (нумерация с 1)
    while (!parser.has_error && parser.current.type == TOKEN_LEFT_BRACKET) {
        if (!is_list_type(left_type)) {
            fprintf(stderr, "[Строка %d] Ошибка: Индексация '[...]' применима только к спискам.\n",
                    parser.current.line);
            parser.has_error = 1;
            break;
        }
        if (!left_lvalue) {
            fprintf(stderr, "[Строка %d] Ошибка: Индексация поддерживается только для переменной-списка (MVP).\n",
                    parser.current.line);
            parser.has_error = 1;
            break;
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

        Buf combined;
        buf_init(&combined);
        if (left_type == TOKEN_LIST_NUMBER) {
            buf_appendf(&combined, "fox_num_list_get(&%s, %s)", left.data, idx.data);
        } else {
            buf_appendf(&combined, "fox_str_list_get(&%s, %s)", left.data, idx.data);
        }
        buf_free(&idx);
        buf_free(&left);
        left = combined;
        left_type = (left_type == TOKEN_LIST_NUMBER) ? TOKEN_NUMBER : TOKEN_STRING;
        left_lvalue = 0;
    }
    if (parser.has_error) {
        buf_free(&left);
        *out_type = TOKEN_NUMBER;
        return;
    }

    // Бинарные операторы по приоритетам
    while (!parser.has_error && precedence <= get_precedence(parser.current.type)) {
        advance_parser();
        Token op = parser.previous;

        Buf right;
        buf_init(&right);
        TokenType right_type;
        parse_precedence(&right, (Precedence)(get_precedence(op.type) + 1), &right_type);

        int is_compare = (op.type == TOKEN_EQUAL_EQUAL || op.type == TOKEN_BANG_EQUAL ||
                          op.type == TOKEN_GREATER || op.type == TOKEN_GREATER_EQUAL ||
                          op.type == TOKEN_LESS || op.type == TOKEN_LESS_EQUAL);
        int is_concat = (op.type == TOKEN_PLUS &&
                         left_type == TOKEN_STRING && right_type == TOKEN_STRING);
        int is_logic = (op.type == TOKEN_AND || op.type == TOKEN_OR);

        if (is_compare && (is_list_type(left_type) || is_list_type(right_type))) {
            fprintf(stderr, "[Строка %d] Ошибка: Списки нельзя сравнивать.\n", op.line);
            parser.has_error = 1;
            buf_free(&right);
            buf_free(&left);
            *out_type = TOKEN_NUMBER;
            return;
        }
        if (is_logic && (left_type != TOKEN_NUMBER || right_type != TOKEN_NUMBER)) {
            fprintf(stderr, "[Строка %d] Ошибка: 'and'/'or' применимы только к числам/булевым.\n", op.line);
            parser.has_error = 1;
            buf_free(&right);
            buf_free(&left);
            *out_type = TOKEN_NUMBER;
            return;
        }
        if (!is_compare && !is_concat && !is_logic &&
            (left_type != TOKEN_NUMBER || right_type != TOKEN_NUMBER)) {
            fprintf(stderr, "[Строка %d] Ошибка: Математика применима только к числам.\n", op.line);
            parser.has_error = 1;
            buf_free(&right);
            buf_free(&left);
            *out_type = TOKEN_NUMBER;
            return;
        }

        Buf combined;
        buf_init(&combined);

        if (is_concat) {
            buf_appendf(&combined, "fox_concat(%s, %s)", left.data, right.data);
        }
        else if (is_compare && left_type == TOKEN_STRING && right_type == TOKEN_STRING) {
            if (op.type == TOKEN_EQUAL_EQUAL)        buf_appendf(&combined, "(double)(strcmp(%s, %s) == 0)", left.data, right.data);
            else if (op.type == TOKEN_BANG_EQUAL)    buf_appendf(&combined, "(double)(strcmp(%s, %s) != 0)", left.data, right.data);
            else if (op.type == TOKEN_GREATER)       buf_appendf(&combined, "(double)(strcmp(%s, %s) > 0)", left.data, right.data);
            else if (op.type == TOKEN_GREATER_EQUAL) buf_appendf(&combined, "(double)(strcmp(%s, %s) >= 0)", left.data, right.data);
            else if (op.type == TOKEN_LESS)          buf_appendf(&combined, "(double)(strcmp(%s, %s) < 0)", left.data, right.data);
            else if (op.type == TOKEN_LESS_EQUAL)    buf_appendf(&combined, "(double)(strcmp(%s, %s) <= 0)", left.data, right.data);
        }
        else if (op.type == TOKEN_EQUAL_EQUAL)       buf_appendf(&combined, "(double)(%s == %s)", left.data, right.data);
        else if (op.type == TOKEN_BANG_EQUAL)        buf_appendf(&combined, "(double)(%s != %s)", left.data, right.data);
        else if (op.type == TOKEN_GREATER)           buf_appendf(&combined, "(double)(%s > %s)", left.data, right.data);
        else if (op.type == TOKEN_GREATER_EQUAL)     buf_appendf(&combined, "(double)(%s >= %s)", left.data, right.data);
        else if (op.type == TOKEN_LESS)              buf_appendf(&combined, "(double)(%s < %s)", left.data, right.data);
        else if (op.type == TOKEN_LESS_EQUAL)        buf_appendf(&combined, "(double)(%s <= %s)", left.data, right.data);
        else if (op.type == TOKEN_PLUS)              buf_appendf(&combined, "%s + %s", left.data, right.data);
        else if (op.type == TOKEN_MINUS)             buf_appendf(&combined, "%s - %s", left.data, right.data);
        else if (op.type == TOKEN_STAR)              buf_appendf(&combined, "%s * %s", left.data, right.data);
        else if (op.type == TOKEN_SLASH)             buf_appendf(&combined, "%s / %s", left.data, right.data);
        else if (op.type == TOKEN_DOUBLE_SLASH)      buf_appendf(&combined, "floor(%s / %s)", left.data, right.data);
        else if (op.type == TOKEN_PERCENT)           buf_appendf(&combined, "fmod(%s, %s)", left.data, right.data);
        else if (op.type == TOKEN_AND)               buf_appendf(&combined, "(double)(%s && %s)", left.data, right.data);
        else if (op.type == TOKEN_OR)                buf_appendf(&combined, "(double)(%s || %s)", left.data, right.data);

        buf_free(&left);
        buf_free(&right);
        left = combined;
        left_type = is_concat ? TOKEN_STRING : TOKEN_NUMBER;
    }

    buf_appendf(out, "%s", left.data);
    buf_free(&left);
    *out_type = left_type;
}