#include <stdio.h>
#include "parser.h"
#include "gen.h"

int main() {
    Parser parser = Parser_new("abcde { a { b }} 123 static @");
    char token[256] = "";
    Parser_parse_ident(&parser, token);
    printf("%s\n", token);
    Parser parser2;
    printf("%s\n", Parser_parse_block(&parser, &parser2).msg);
    printf("%s\n", parser2.src);
    i64 value = 0;
    Parser_parse_number(&parser, &value);
    printf("%lld\n", value);
    Parser_parse_keyword(&parser, "static");
    printf("%s\n", parser.src);
    Parser_parse_symbol(&parser, "@");
    printf("%s\n", parser.src);

    Generator gen = Generator_new(NULL);
    return 0;
}
