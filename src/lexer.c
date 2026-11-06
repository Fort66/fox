#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

typedef struct {
    const char* start;
    const char* current;
    int line;
} Lexer;

static Lexer lexer;

void init_lexer(const char* source) {
    lexer.start = source;
    lexer.current = source;
    lexer.line = 1;
}

static int is_at_end(void) {
    return *lexer.current == '\0';
}

static char advance(void) {
    lexer.current++;
    return lexer.current[-1];
}

static char peek(void) {
    return *lexer.current;
}

static void skip_whitespace(void) {
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                lexer.line++;
                advance();
                break;
            case '*':
                if (lexer.current == lexer.start || lexer.current[-1] == '\n') {
                    while (peek() != '\n' && !is_at_end()) advance();
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static Token make_token(TokenType type) {
    Token token;
    token.type = type;
    token.start = lexer.start;
    token.length = (int)(lexer.current - lexer.start);
    token.line = lexer.line;
    return token;
}

static Token error_token(const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = lexer.line;
    return token;
}

static int check_keyword(int start, int length, const char* rest, TokenType type) {
    if ((int)(lexer.current - lexer.start) == start + length) {
        for (int i = 0; i < length; i++) {
            if (tolower((unsigned char)lexer.start[start + i]) != rest[i]) {
                return TOKEN_IDENTIFIER;
            }
        }
        return type;
    }
    return TOKEN_IDENTIFIER;
}

static TokenType identifier_type(void) {
    switch (tolower((unsigned char)*lexer.start)) {
        case 'a':
            return check_keyword(1, 2, "nd", TOKEN_AND);
        case 'c':
            if (lexer.current - lexer.start > 1 && tolower((unsigned char)*(lexer.start + 1)) == 'a') {
                return check_keyword(1, 3, "ase", TOKEN_CASE);
            }
            return check_keyword(1, 4, "lass", TOKEN_CLASS);
        case 'p':
            return check_keyword(1, 4, "rint", TOKEN_PRINT);
        case 't':
            if (lexer.current - lexer.start > 1 && tolower((unsigned char)*(lexer.start + 1)) == 'r') {
                return check_keyword(1, 3, "rue", TOKEN_TRUE);
            }
            return check_keyword(1, 1, "o", TOKEN_TO);
        case 'd':
            return check_keyword(1, 1, "o", TOKEN_DO);
        case 'o':
            if (lexer.current - lexer.start > 1 && tolower((unsigned char)*(lexer.start + 1)) == 'r') {
                return check_keyword(1, 1, "r", TOKEN_OR);
            }
            return check_keyword(1, 8, "therwise", TOKEN_OTHERWISE);
        case 's':
            if (lexer.current - lexer.start > 2 && tolower((unsigned char)*(lexer.start + 2)) == 'r') {
                return check_keyword(1, 5, "tring", TOKEN_TYPE_STRING);
            }
            return check_keyword(1, 4, "tore", TOKEN_STORE);
        case 'i':
            if (lexer.current - lexer.start == 2 && tolower((unsigned char)*(lexer.start + 1)) == 'n') {
                return TOKEN_IN;
            }
            return check_keyword(1, 1, "f", TOKEN_IF);
        case 'l':
            /* Шаг 5: 'list' (тип-модификатор параметра), иначе 'local' */
            if (lexer.current - lexer.start > 1 && tolower((unsigned char)*(lexer.start + 1)) == 'i') {
                return check_keyword(1, 3, "ist", TOKEN_LIST);
            }
            return check_keyword(1, 4, "ocal", TOKEN_LOCAL);
        case 'n':
            if (lexer.current - lexer.start > 1 && tolower((unsigned char)*(lexer.start + 1)) == 'o') {
                return check_keyword(1, 2, "ot", TOKEN_NOT);
            }
            return check_keyword(1, 5, "umber", TOKEN_TYPE_NUMBER);
        case 'r':
            return check_keyword(1, 5, "eturn", TOKEN_RETURN);
        case 'f':
            if (lexer.current - lexer.start > 1 && tolower((unsigned char)*(lexer.start + 1)) == 'a') {
                return check_keyword(1, 4, "alse", TOKEN_FALSE);
            }
            if (lexer.current - lexer.start == 3 &&
                tolower((unsigned char)*(lexer.start + 1)) == 'o' &&
                tolower((unsigned char)*(lexer.start + 2)) == 'r')
                return TOKEN_FOR;
            return check_keyword(1, 3, "unc", TOKEN_FUNC);
        case 'w':
            return check_keyword(1, 4, "hile", TOKEN_WHILE);
        case 'e':
            if (lexer.current - lexer.start > 1) {
                switch (tolower((unsigned char)*(lexer.start + 1))) {
                    case 'a':
                        return check_keyword(1, 3, "ach", TOKEN_EACH);
                    case 'l':
                        if (lexer.current - lexer.start == 4 &&
                            tolower((unsigned char)*(lexer.start + 2)) == 'i' &&
                            tolower((unsigned char)*(lexer.start + 3)) == 'f')
                            return TOKEN_ELIF;
                        if (lexer.current - lexer.start == 4 &&
                            tolower((unsigned char)*(lexer.start + 2)) == 's' &&
                            tolower((unsigned char)*(lexer.start + 3)) == 'e')
                            return TOKEN_ELSE;
                        break;
                    case 'n':
                        if (tolower((unsigned char)*(lexer.start + 3)) == 'c') {
                            if (lexer.current - lexer.start == 7)
                                return check_keyword(1, 6, "ndcase", TOKEN_ENDCASE);
                            return check_keyword(1, 7, "ndclass", TOKEN_ENDCLASS);
                        }
                        if (tolower((unsigned char)*(lexer.start + 3)) == 'f') {
                            if (lexer.current - lexer.start == 7)
                                return check_keyword(1, 6, "ndfunc", TOKEN_ENDFUNC);
                            return check_keyword(1, 5, "ndfor", TOKEN_ENDFOR);
                        }
                        if (tolower((unsigned char)*(lexer.start + 3)) == 'i')
                            return check_keyword(1, 4, "ndif", TOKEN_ENDIF);
                        if (tolower((unsigned char)*(lexer.start + 3)) == 'w')
                            return check_keyword(1, 7, "ndwhile", TOKEN_ENDWHILE);
                        break;
                }
            }
            break;
    }
    return TOKEN_IDENTIFIER;
}

static Token scan_identifier(void) {
    while (isalnum((unsigned char)peek()) || peek() == '_') advance();
    return make_token(identifier_type());
}

static Token scan_number(void) {
    while (isdigit((unsigned char)peek())) advance();
    if (peek() == '.' && isdigit((unsigned char)lexer.current[0])) {
        advance();
        while (isdigit((unsigned char)peek())) advance();
    }
    return make_token(TOKEN_NUMBER);
}

static Token scan_string(void) {
    while (peek() != '"' && !is_at_end()) {
        if (peek() == '\n') lexer.line++;
        advance();
    }
    if (is_at_end()) return error_token("Незакрытая строка.");
    advance();
    return make_token(TOKEN_STRING);
}

Token scan_token(void) {
    skip_whitespace();
    lexer.start = lexer.current;
    if (is_at_end()) return make_token(TOKEN_EOF);

    char c = advance();

    if (isalpha((unsigned char)c) || c == '_') return scan_identifier();
    if (isdigit((unsigned char)c)) return scan_number();

    switch (c) {
        case '(':  return make_token(TOKEN_LEFT_PAREN);
        case ')':  return make_token(TOKEN_RIGHT_PAREN);
        case ',':  return make_token(TOKEN_COMMA);
        case '[':  return make_token(TOKEN_LEFT_BRACKET);
        case ']':  return make_token(TOKEN_RIGHT_BRACKET);
        case ':': return make_token(TOKEN_COLON);
        case '=':
            if (peek() == '=') { advance(); return make_token(TOKEN_EQUAL_EQUAL); }
            return make_token(TOKEN_EQUAL);
        case '!':
            if (peek() == '=') { advance(); return make_token(TOKEN_BANG_EQUAL); }
            return error_token("Ожидался знак '=' после '!'");
        case '>':
            if (peek() == '=') { advance(); return make_token(TOKEN_GREATER_EQUAL); }
            return make_token(TOKEN_GREATER);
        case '<':
            if (peek() == '=') { advance(); return make_token(TOKEN_LESS_EQUAL); }
            return make_token(TOKEN_LESS);
        case '+':
            if (peek() == '=') { advance(); return make_token(TOKEN_PLUS_EQUAL); }
            return make_token(TOKEN_PLUS);
        case '-':
            if (peek() == '=') { advance(); return make_token(TOKEN_MINUS_EQUAL); }
            return make_token(TOKEN_MINUS);
        case '*':
            if (peek() == '=') { advance(); return make_token(TOKEN_STAR_EQUAL); }
            return make_token(TOKEN_STAR);
        case '%':
            if (peek() == '=') { advance(); return make_token(TOKEN_PERCENT_EQUAL); }
            return make_token(TOKEN_PERCENT);
        case '"':  return scan_string();
        case '/':
            if (peek() == '/') {
                advance();
                if (peek() == '=') { advance(); return make_token(TOKEN_DSLASH_EQUAL); }
                return make_token(TOKEN_DOUBLE_SLASH);
            }
            if (peek() == '=') { advance(); return make_token(TOKEN_SLASH_EQUAL); }
            return make_token(TOKEN_SLASH);
    }

    return error_token("Неопознанный символ.");
}