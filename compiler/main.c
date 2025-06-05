#include <stdio.h>
#include "parser.h"
#include "gen.h"
#include "syntax.h"
#include "ast.h"

int main() {
    
    Generator gen = Generator_new(NULL);
    Parser parser = Parser_new("const static struct {i32 a; i32 b;} @data a[5]");
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

    printf("AAAAAAAAAAAAAAAAAAAAAA\n\n\n");

    Generator gen2 = Generator_new(NULL);
    Parser parser3 = Parser_new("\
        typedef struct { u32 line; char msg[256]; } ParserMsg;\n\
        struct MyStruct {\n\
            bool flag;\n\
            ParserMsg msg;\n\
        };\n\
        typedef struct MyStruct MyS;\n\
    ");

    Syntax_build(parser3, &gen2);
    Generator_print(&gen2);
    Generator_free(gen2);

    printf("\n\n");

    printf("BBBBBBBBBBBBBBBBBBBBB\n\n");

    AstTree tree;
    Parser parser4 = Parser_new("2 + 3 * 5 / 7 - 8");
    if(ParserMsg_is_success(AstTree_parse(parser4, &tree))) {
        AstTree_print(&tree);
    }else {
        printf("failed");
    }

    printf("\n\n");

    return 0;
}

