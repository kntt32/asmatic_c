#include <stdio.h>
#include "types.h"
#include "gen.h"
#include "parser.h"

static Type PRIMITIVE_TYPES[] = {
    {"i32", Type_primitive, {}, 0}
};

static ParserMsg Type_parse_helper_(inout Parser* parser, in Generator* generator, optional Type* (in *getter)(in Generator*, in char*), out Type* type) {
    char type_name[256] = "";
    
    if(Parser_parse_ident(parser, type_name).msg[0] != '\0') {
        ParserMsg msg = {parser->line, "expected type"};
        return msg;
    }

    Type* type_ptr = getter(generator, type_name);
    if(type_ptr == NULL) {
        ParserMsg msg = {parser->line, "expected type"};
        return msg;
    }

    *type = *type_ptr;

    return SUCCESS_PARSER_MSG;
}

ParserMsg Type_parse(inout Parser* parser, in Generator* generator, out Type* type) {
    Parser parser_copy = *parser;

    ParserMsg msg;
    if(Parser_parse_keyword(&parser_copy, "struct").msg[0] == '\0') {
        msg = Type_parse_helper_(&parser_copy, generator, Generator_get_struct_types, type);
    }else if(Parser_parse_keyword(&parser_copy, "enum").msg[0] == '\0') {
        msg = Type_parse_helper_(&parser_copy, generator, Generator_get_enum_types, type);
    }else if(Parser_parse_keyword(&parser_copy, "union").msg[0] == '\0') {
        msg = Type_parse_helper_(&parser_copy, generator, Generator_get_union_types, type);
    }else {
        msg = Type_parse_helper_(&parser_copy, generator, Generator_get_normal_types, type);
    }

    if(msg.msg[0] == '\0') {
        *parser = parser_copy;
    }
    return msg;
}

Generator Generator_new(optional in char* filename) {
    Generator generator;
    
    if(filename == NULL) {
        filename = "anonymous.c";
    }
    strcpy(generator.filename, filename);
    
    generator.stack_usage = 0;

    generator.normal_types = Vec_from(PRIMITIVE_TYPES, LEN(PRIMITIVE_TYPES), sizeof(Type));
    generator.struct_types = Vec_new(sizeof(Type));
    generator.enum_types = Vec_new(sizeof(Type));
    generator.union_types = Vec_new(sizeof(Type));

    generator.functions = Vec_new(sizeof(Function));

    generator.global_variables = Vec_new(sizeof(Variable));
    generator.auto_variables = Vec_new(sizeof(Variable));

    generator.code = String_new();
    generator.error = String_new();

    return generator;
}

static optional Type* Generator_get_type_helper(in Vec* types, in char* name) {
    for(u32 i=0; i<Vec_len(types); i++) {
        Type* type = Vec_index(types, i);
        if(strcmp(name, type->name) == 0) {
            return type;
        }
    }

    return NULL;
}

optional Type* Generator_get_normal_types(in Generator* self, in char* name) {
    return Generator_get_type_helper(&self->normal_types, name);
}

optional Type* Generator_get_struct_types(in Generator* self, in char* name) {
    return Generator_get_type_helper(&self->struct_types, name);
}

optional Type* Generator_get_enum_types(in Generator* self, in char* name) {
    return Generator_get_type_helper(&self->enum_types, name);
}

optional Type* Generator_get_union_types(in Generator* self, in char* name) {
    return Generator_get_type_helper(&self->union_types, name);
}

