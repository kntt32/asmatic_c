#include <stdio.h>
#include "types.h"
#include "parser.h"
#include "syntax.h"
/*
SyntaxResult Syntax_build_struct_declaration(inout Parser* parser, inout Generator* generator) {
    Parser parser_copy = *parser;
    PARSERMSG_UNWRAP(
        Parser_parse_keyword(&parser_copy, "struct"),
        (void)(NULL)
    );

    Type type;
    PARSERMSG_UNWRAP(
        Type_parse(parser, generator, &type),
        (void)(NULL)
    );

    switch(type.storage.type) {
        case Storage_Default:
        case Storage_Data:
            type.storage.type = Storage_Data;
            break;
        default:
            Type_free(type);
            return DATA_VOID;
    }

    Generator_add_struct_type(generator, type);

    return DATA_VOID;
}

*/
