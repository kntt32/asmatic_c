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

typedef struct {
    ValueType type;
    union {
        String string;
        i64 signedint;
        u64 unsignedint;
        Vec structvalue; // Vec<ImmValue>
    } body;
} ImmValue;

typedef struct {
    SyntaxStatus status;
    ImmValue imm_value;
} ExprResult;

static ExprResult EXPRRESULT_NONE = {SyntaxStatus_None, {ValueType_Default, {}}};
/*
static bool Syntax_build_expr_addsubterm_splitparser(inout Parser* parser) {
    return Parser_start_with_symbol(parser, "+") || Parser_start_with_symbol(parser, "-");
}

static bool Syntax_build_expr_addsub_term(
    inout Parser* parser,
    inout Generator* generator,
    u32 index,
    out Parser* term_parser
) {
    Parser_split(parser, Syntax_build_expr_addsubterm_splitparser, term_parser);

    return true;
}

static ExprResult Syntax_build_expr_addsub(Parser parser, inout Generator* generator, in Data* expected_data) {
    Vec terms = Vec_new(sizeof(Parser));// Vec<Parser>
    u32 index = 0;

    while(!Parser_is_empty(&parser)) {
        Parser term_parser;
        Syntax_build_expr_addsub_term(&parser, generator, index, &term_parser);
        Vec_push(&term_parser);
        index ++;
    }

    if(index == 0 || (index == 1 && Parser_is_empty(Vec_index(&term, 0)))) {
        ExprResult result = {SyntaxStatus_None, {ValueType_Default, {}}};
        Vec_free(&terms);
        return result;
    }

    ExprResult result;
    result.status = SyntaxStatus_Success;
    result.imm_value.type = expected_data->type.value_type;
    switch(result.imm_value.type) {
        case ValueType_SignedInt:
            result.imm_value.body.signedint = 0;
            break;
        case ValueType_UnsignedInt:
            result.imm_value.body.unsignedint = 0;
            break;
        default:
            Error error = {parser.line, "expected number"};
            Generator_add_error(generator, error);
            result.status = SyntaxStatus_Error;
            Vec_free(&terms);
            return result;
    }

    for(u32 i=0; i<index; i++) {
        Parser* term_parser = Vec_index(&terms, i);
        
        Parser_parse_symbol(term_parser, "+");
        bool minus_flag = ParserMsg_is_success(term_parser, "-")
        ExprResult branch_result = Syntax_build_expr(Vec_index(&terms, i), generator, expected_data);
        switch(result.imm_value.type) {
            case ValueType_SignedInt:
                result.imm_value.body.signedint += (minus_flag)?(-1):(1) * 
        }
    }
}

static ExprResult Syntax_build_expr(inout Parser* parser, inout Generator* generator, in Data* expected_data) {
    TODO();
    return EXPRRESULT_NONE;
}*/

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

