#include <stdio.h>
#include "parser.h"
#include "gen.h"

int main() {
    Generator gen = Generator_new(NULL);
    Parser parser = Parser_new("struct MyStruct { i32 a; i32 b;}");
    Type type;
    ParserMsg msg = Type_parse(&parser, &gen, &type);
    if(ParserMsg_is_success(msg)) {
        /*printf("name: %s\ntype: %d\n", type.name, type.type);
        printf("members(%d):\n", Vec_len(&type.property.members));
        for(u32 i=0; i<Vec_len(&type.property.members); i++) {
            StructMember* member = Vec_index(&type.property.members, i);
            printf("    type: %s name: %s\n", member->type.name, member->name);
        }
        printf("ref_depth: %d\n", type.ref_depth);*/
        Type_print(&type);
    }else {
        printf("err:%d:%s\n", msg.line, msg.msg);
    }

    return 0;
}
