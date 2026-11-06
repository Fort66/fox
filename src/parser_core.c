#include <stdio.h>
#include "parser.h"

Parser parser;
Function* current_function = NULL;

void advance_parser(void) {
    parser.previous = parser.current;
    for (;;) {
        parser.current = scan_token();
        if (parser.current.type != TOKEN_ERROR) break;
        fprintf(stderr, "[Строка %d] Ошибка лексера: %.*s\n",
                parser.current.line, parser.current.length, parser.current.start);
        parser.has_error = 1;
    }
}

void consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance_parser();
        return;
    }
    fprintf(stderr, "[Строка %d] Ошибка синтаксиса: %s\n",
            parser.current.line, message);
    parser.has_error = 1;
}