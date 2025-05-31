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

    Parser parser2 = Parser_new("i32@rax main(i32@rax arg1, i32@rdx arg2)");
    Function function;
    ParserMsg msg2 = Function_parse(&parser2, &gen, &function);
    if(ParserMsg_is_success(msg2)) {
        Function_print(&function);
        printf("\n");
        Function_free(function);
    }else {
        printf("err:%d:%s\n", msg2.line, msg2.msg);
    }

    Generator_free(gen);

    return 0;
}

