#include <stdio.h>
#include "parser.h"
#include "gen.h"

int main() {
    Generator gen = Generator_new(NULL);
    Parser parser = Parser_new("enum MyEnum { Enum1 = 3, Enum2 = 7, Enum3 }");
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

