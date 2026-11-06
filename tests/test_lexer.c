#include <stdio.h>
#include "lexer.h"

static int failures = 0;

static void expect(const char* src, TokenType expected) {
    init_lexer(src);
    Token t = scan_token();
    if (t.type != expected) {
        fprintf(stderr, "ПРОВАЛ: \"%s\" -> токен %d, ожидался %d\n",
                src, (int)t.type, (int)expected);
        failures++;
    }
}

int main(void) {
    /* Ключевые слова операторов */
    expect("local", TOKEN_LOCAL);
    expect("LOCAL", TOKEN_LOCAL);
    expect("class", TOKEN_CLASS);
    expect("endclass", TOKEN_ENDCLASS);
    expect("func", TOKEN_FUNC);
    expect("endfunc", TOKEN_ENDFUNC);
    expect("return", TOKEN_RETURN);
    expect("if", TOKEN_IF);
    expect("elif", TOKEN_ELIF);
    expect("else", TOKEN_ELSE);
    expect("endif", TOKEN_ENDIF);
    expect("do", TOKEN_DO);
    expect("case", TOKEN_CASE);
    expect("otherwise", TOKEN_OTHERWISE);
    expect("endcase", TOKEN_ENDCASE);
    expect("while", TOKEN_WHILE);
    expect("endwhile", TOKEN_ENDWHILE);
    expect("for", TOKEN_FOR);
    expect("endfor", TOKEN_ENDFOR);
    expect("each", TOKEN_EACH);
    expect("Each", TOKEN_EACH);
    expect("in", TOKEN_IN);
    expect("IN", TOKEN_IN);
    expect("print", TOKEN_PRINT);
    expect("store", TOKEN_STORE);
    expect("to", TOKEN_TO);

    /* Булевы и логика */
    expect("true", TOKEN_TRUE);
    expect("false", TOKEN_FALSE);
    expect("not", TOKEN_NOT);
    expect("and", TOKEN_AND);
    expect("or", TOKEN_OR);

    /* Типы в сигнатурах функций (шаг 5: + list) */
    expect("number", TOKEN_TYPE_NUMBER);
    expect("string", TOKEN_TYPE_STRING);
    expect("list", TOKEN_LIST);
    expect("LIST", TOKEN_LIST);

    /* Соседи-идентификаторы не должны становиться ключевыми словами */
    expect("eachx", TOKEN_IDENTIFIER);
    expect("reach", TOKEN_IDENTIFIER);
    expect("inn", TOKEN_IDENTIFIER);
    expect("index", TOKEN_IDENTIFIER);
    expect("info", TOKEN_IDENTIFIER);
    expect("ifx", TOKEN_IDENTIFIER);
    expect("forx", TOKEN_IDENTIFIER);
    expect("formatter", TOKEN_IDENTIFIER);
    expect("numeric", TOKEN_IDENTIFIER);
    expect("stringify", TOKEN_IDENTIFIER);
    expect("truthy", TOKEN_IDENTIFIER);
    expect("falsehood", TOKEN_IDENTIFIER);
    expect("notify", TOKEN_IDENTIFIER);
    expect("android", TOKEN_IDENTIFIER);
    expect("orderly", TOKEN_IDENTIFIER);
    expect("listen", TOKEN_IDENTIFIER);
    expect("lis", TOKEN_IDENTIFIER);
    expect("e", TOKEN_IDENTIFIER);
    expect("i", TOKEN_IDENTIFIER);
    expect("n", TOKEN_IDENTIFIER);

    /* Комментарий с начала строки пропускается */
    expect("* комментарий\nlocal", TOKEN_LOCAL);

    if (failures == 0) {
        printf("Юнит-тест лексера: OK\n");
        return 0;
    }
    fprintf(stderr, "Юнит-тест лексера: ПРОВАЛОВ %d\n", failures);
    return 1;
}