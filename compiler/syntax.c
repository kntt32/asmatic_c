#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "types.h"
#include "parser.h"
#include "gen.h"
#include "syntax.h"

#define SYNTAX_ADD_ERROR(parsermsg, generator, catch_proc) {\
    ParserMsg msg_copy = parsermsg;\
    if(!ParserMsg_is_success(msg_copy)) {\
        Error error;\
        error.line = msg_copy.line;\
        strcpy(error.msg, msg_copy.msg);\
        Generator_add_error(generator, error);\
        catch_proc;\
        return SyntaxStatus_Error;\
    }\
}

static SyntaxStatus Syntax_build_typedef(inout Parser* parser, inout Generator* generator) {
    // typedef/ $type $ident;
    Type type;

    if(!ParserMsg_is_success(Parser_parse_keyword(parser, "typedef"))) {
        return SyntaxStatus_None;
    }

    SYNTAX_ADD_ERROR(Type_parse(parser, generator, &type), generator, Parser_skip_to_semicolon(parser));
    SYNTAX_ADD_ERROR(Parser_parse_ident(parser, type.name), generator, Parser_skip_to_semicolon(parser));
    SYNTAX_ADD_ERROR(Parser_parse_symbol(parser, ";"), generator, (void)(NULL));

    Generator_add_normal_type(generator, type);

    return SyntaxStatus_Success;
}

static SyntaxStatus Syntax_build_type_declare(inout Parser* parser, inout Generator* generator) {
    if(!(Parser_start_with(parser, "struct") || Parser_start_with(parser, "enum") || Parser_start_with(parser, "union"))) {
        return SyntaxStatus_None;
    }

    Type type;
    SYNTAX_ADD_ERROR(
        Type_parse(parser, generator, &type),
        generator,
        Parser_skip_to_semicolon(parser)
    );
    SYNTAX_ADD_ERROR(
        Parser_parse_symbol(parser, ";"),
        generator,
        (void)NULL
    );

    switch(type.type) {
        case Type_Struct:
            Generator_add_struct_type(generator, type);
            break;
        case Type_Enum:
            Generator_add_enum_type(generator, type);
            break;
        case Type_Union:
            Generator_add_union_type(generator, type);
            break;
        default:
            PANIC("unexpected type");
            break;
    }

    return SyntaxStatus_Success;
}

void Syntax_build(Parser parser, inout Generator* generator) {
    SyntaxStatus (*BUILDERS[])(inout Parser*, inout Generator*) = {Syntax_build_typedef, Syntax_build_type_declare};
    u32 builders_len = LEN(BUILDERS);

    while(!Parser_is_empty(&parser)) {
        bool unknown_flag = true;
        
        for(u32 i=0; i<builders_len; i++) {
            SyntaxStatus status = BUILDERS[i](&parser, generator);
            if(status == SyntaxStatus_Success || status == SyntaxStatus_Error) {
                unknown_flag = false;
                break;
            }
        }

        if(unknown_flag) {
            Error error = {parser.line, "unknown expression"};
            Generator_add_error(generator, error);

            Parser_skip_to_semicolon(&parser);
        }
    }

    return;
}

