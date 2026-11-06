#ifndef FOX_SYMBOLS_H
#define FOX_SYMBOLS_H

#include "lexer.h"

#define MAX_VARIABLES 512
#define MAX_FUNCTIONS 64
#define MAX_PARAMS 8
#define MAX_NAME 64
#define MAX_ARGS 3

typedef struct {
    const char* name;
    int length;
    TokenType type;
} Variable;

typedef struct {
    char name[MAX_NAME];
    char param_names[MAX_PARAMS][MAX_NAME];
    TokenType param_types[MAX_PARAMS];
    int param_count;
    TokenType return_type; // TOKEN_NUMBER / TOKEN_STRING / 0 = void
    int has_return;
} Function;

typedef struct {
    const char* name;
    int param_count;
    TokenType param_types[MAX_ARGS];
    TokenType return_type;
    const char* c_template;
} Builtin;

extern int variable_count;

// Переменные
int find_variable(Token name);
void add_variable(Token name, TokenType type);
TokenType symbol_table_type(int index);

// Пользовательские функции
Function* find_function(const char* name, int length);
Function* add_function_slot(void); // NULL, если лимит исчерпан

// Встроенные функции
int builtin_name_taken(const char* name, int length);
const Builtin* find_builtin_match(const char* name, int length,
                                  int argc, const TokenType* atypes);

// Типы
const char* c_type(TokenType t);
int is_list_type(TokenType t);

// Сброс состояния между компиляциями
void symbols_reset(void);

#endif