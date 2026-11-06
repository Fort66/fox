#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "symbols.h"

static Variable symbol_table[MAX_VARIABLES];
int variable_count = 0;

static Function functions[MAX_FUNCTIONS];
static int function_count = 0;

// return_type == 0 означает «void»: встроенную можно вызывать только оператором
static const Builtin builtins[] = {
    { "len",    1, { TOKEN_STRING }, TOKEN_NUMBER, "(double)strlen(%s)" },
    { "upper",  1, { TOKEN_STRING }, TOKEN_STRING, "fox_upper(%s)" },
    { "lower",  1, { TOKEN_STRING }, TOKEN_STRING, "fox_lower(%s)" },
    { "trim",   1, { TOKEN_STRING }, TOKEN_STRING, "fox_trim(%s)" },
    { "substr", 3, { TOKEN_STRING, TOKEN_NUMBER, TOKEN_NUMBER }, TOKEN_STRING, "fox_substr(%s)" },
    { "str",    1, { TOKEN_NUMBER }, TOKEN_STRING, "fox_str(%s)" },
    { "val",    1, { TOKEN_STRING }, TOKEN_NUMBER, "strtod(%s, NULL)" },
    { "count",  1, { TOKEN_LIST_NUMBER }, TOKEN_NUMBER, "(double)(%s).len" },
    { "count",  1, { TOKEN_LIST_STRING }, TOKEN_NUMBER, "(double)(%s).len" },
    { "append", 2, { TOKEN_LIST_NUMBER, TOKEN_NUMBER }, 0, "fox_num_list_append(&%s)" },
    { "append", 2, { TOKEN_LIST_STRING, TOKEN_STRING }, 0, "fox_str_list_append(&%s)" },
};
#define BUILTIN_COUNT ((int)(sizeof(builtins) / sizeof(builtins[0])))

static int name_eq(const char* a, const char* b, int length) {
    for (int i = 0; i < length; i++) {
        if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i])) {
            return 0;
        }
    }
    return 1;
}

// ---------- Переменные ----------

int find_variable(Token name) {
    for (int i = variable_count - 1; i >= 0; i--) {
        if (symbol_table[i].length == name.length &&
            strncmp(symbol_table[i].name, name.start, name.length) == 0) {
            return i;
        }
    }
    return -1;
}

void add_variable(Token name, TokenType type) {
    for (int i = variable_count - 1; i >= 0; i--) {
        if (symbol_table[i].length == name.length &&
            strncmp(symbol_table[i].name, name.start, name.length) == 0) {
            symbol_table[i].type = type;
            return;
        }
    }
    if (variable_count >= MAX_VARIABLES) {
        fprintf(stderr, "Ошибка компиляции: Превышен лимит переменных.\n");
        exit(1);
    }
    symbol_table[variable_count].name = name.start;
    symbol_table[variable_count].length = name.length;
    symbol_table[variable_count].type = type;
    variable_count++;
}

TokenType symbol_table_type(int index) {
    return symbol_table[index].type;
}

// ---------- Пользовательские функции ----------

Function* find_function(const char* name, int length) {
    for (int i = 0; i < function_count; i++) {
        if ((int)strlen(functions[i].name) == length &&
            strncmp(functions[i].name, name, length) == 0) {
            return &functions[i];
        }
    }
    return NULL;
}

Function* add_function_slot(void) {
    if (function_count >= MAX_FUNCTIONS) return NULL;
    Function* fn = &functions[function_count++];
    memset(fn, 0, sizeof(*fn));
    return fn;
}

// ---------- Встроенные функции ----------

int builtin_name_taken(const char* name, int length) {
    for (int i = 0; i < BUILTIN_COUNT; i++) {
        if ((int)strlen(builtins[i].name) == length &&
            name_eq(builtins[i].name, name, length)) {
            return 1;
        }
    }
    return 0;
}

const Builtin* find_builtin_match(const char* name, int length,
                                  int argc, const TokenType* atypes) {
    for (int i = 0; i < BUILTIN_COUNT; i++) {
        if ((int)strlen(builtins[i].name) != length ||
            !name_eq(builtins[i].name, name, length)) continue;
        if (builtins[i].param_count != argc) continue;
        int ok = 1;
        for (int p = 0; p < argc; p++) {
            if (builtins[i].param_types[p] != atypes[p]) { ok = 0; break; }
        }
        if (ok) return &builtins[i];
    }
    return NULL;
}

// ---------- Типы ----------

const char* c_type(TokenType t) {
    if (t == TOKEN_STRING) return "const char*";
    if (t == TOKEN_LIST_NUMBER) return "FoxNumList";
    if (t == TOKEN_LIST_STRING) return "FoxStrList";
    return "double";
}

int is_list_type(TokenType t) {
    return t == TOKEN_LIST_NUMBER || t == TOKEN_LIST_STRING;
}

void symbols_reset(void) {
    variable_count = 0;
    function_count = 0;
}