#ifndef FOX_PARSER_H
#define FOX_PARSER_H

#include <stdio.h>
#include "lexer.h"
#include "buf.h"
#include "symbols.h"

// Состояние парсера
typedef struct {
    Token current;    // Текущий анализируемый токен
    Token previous;   // Предыдущий токен
    Buf* out;         // Буфер, куда пишем сгенерированный C-код
    int has_error;    // Флаг ошибки компиляции
} Parser;

extern Parser parser;
extern Function* current_function; // NULL вне тела функции

// parser_core.c
void advance_parser(void);
void consume(TokenType type, const char* message);

// expr.c
void parse_expression(Buf* out, TokenType* out_type);
void parse_call(Function* fn, Buf* out);
void parse_arg_list(int param_count, const TokenType* param_types,
                    const char* callee, Buf* out);
int  parse_args_raw(Buf* args, TokenType* atypes);
void join_args(Buf* out, int argc, Buf* args);

// stmt.c
void statement(void);

// func.c
void parse_function(Buf* funcs);

// codegen.c
int compile(const char* source_code, const char* output_c_path);

#endif