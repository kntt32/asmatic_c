#include <stdio.h>
#include "parser.h"
#include "gen.h"

int main() {
    Generator gen = Generator_new(NULL);
    Parser parser = Parser_new("struct MyStruct { i32* a; i32 b;} @ stack");
    Data data;
    ParserMsg msg = Data_parse(&parser, &gen, &data);
    if(ParserMsg_is_success(msg)) {
        Data_print(&data);
        printf("\n");
    }else {
        printf("err:%d:%s\n", msg.line, msg.msg);
    }

    return 0;
}

