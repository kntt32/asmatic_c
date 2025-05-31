#include <stdio.h>
#include "parser.h"
#include "gen.h"

int main() {
    Generator gen = Generator_new(NULL);
    Parser parser = Parser_new("const static i32@data a[5]");
    Variable variable;
    ParserMsg msg = Variable_parse(&parser, &gen, &variable);
    if(ParserMsg_is_success(msg)) {
        Variable_print(&variable);
        printf("\n");
        Variable_free(variable);
    }else {
        printf("err:%d:%s\n", msg.line, msg.msg);
    }
    Generator_free(gen);

    return 0;
}

