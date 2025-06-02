#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "types.h"
#include "parser.h"
#include "gen.h"
#include "syntax.h"

bool SyntaxResult_is_success(SyntaxResult result) {
    return result.success_flag;
}

static bool Syntax_add_error(ParserMsg msg, inout Generator* generator) {
    if(ParserMsg_is_success(msg)) {
        return false;
    }

    Error error;
    error.line = msg.line;
    strcpy(error.msg, msg.msg);

    Generator_add_error(generator, error);

    return true;
}

static void Syntax_build_typedef(inout Parser* parser, inout Generator* generator) {
    // typedef/ $type $ident;
    Type type;
    if(Syntax_add_error(Type_parse(parser, generator, &type), generator)) {
        return;
    }

    if(Syntax_add_error(Parser_parse_ident(parser, type.name), generator)) {
        Type_free(type);
        return;
    }

    if(Syntax_add_error(Parser_parse_symbol(parser, ";"), generator)) {
        Type_free(type);
        return;
    }

    Generator_add_normal_type(generator, type);

    return;
}

static void Syntax_build_type(inout Parser* parser, inout Generator* generator) {
    Type type;
    if(Syntax_add_error(Type_parse(parser, generator, &type), generator)) {
        return;
    }

    if(Syntax_add_error(Parser_parse_symbol(parser, ";"), generator)) {
        Type_free(type);
        return;
    }

    switch(type.type) {
        case Type_Normal:
            Generator_add_normal_type(generator, type);
            break;
        case Type_Struct:
            Generator_add_struct_type(generator, type);
            break;
        case Type_Enum:
            Generator_add_enum_type(generator, type);
            break;
        case Type_Union:
            Generator_add_union_type(generator, type);
            break;
        case Type_FnPtr:
            TODO();
            break;
    }

    return;
}

void Syntax_build(Parser parser, inout Generator* generator) {
    while(!Parser_is_empty(&parser)) {
        Parser parser_copy = parser;;

        if(ParserMsg_is_success(Parser_parse_keyword(&parser, "typedef"))) {
            Syntax_build_typedef(&parser, generator);
        }else if(ParserMsg_is_success(Parser_parse_keyword(&parser_copy, "struct"))
                || ParserMsg_is_success(Parser_parse_keyword(&parser_copy, "enum"))
                || ParserMsg_is_success(Parser_parse_keyword(&parser_copy, "union"))) {
            Syntax_build_type(&parser, generator);
        }else {
            Error error = {parser.line, "unknown expression"};
            Generator_add_error(generator, error);
            TODO();
        }
    }

    return;
}

