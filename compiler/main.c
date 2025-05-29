#include <stdio.h>
#include "parser.h"
#include "gen.h"

int main() {
    Generator gen = Generator_new(NULL);
    Parser parser = Parser_new("enum MyUnion { a, b,}");
    Type type;
    ParserMsg msg = Type_parse(&parser, &gen, &type);
    if(ParserMsg_is_success(msg)) {
        Type_print(&type);
        printf("\n");
    }else {
        printf("err:%d:%s\n", msg.line, msg.msg);
    }

    return 0;
}

