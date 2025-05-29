#include <stdio.h>
#include "types.h"
#include "gen.h"
#include "parser.h"

static Type PRIMITIVE_TYPES[] = {
    {"i32", Type_primitive, {}, 0}
};

static ParserMsg Type_parse_helper(inout Parser* parser, in Generator* generator, optional Type* (in *getter)(in Generator*, in char*), out Type* type) {
    Parser parser_copy = *parser;

    char type_name[256] = "";
    
    if(Parser_parse_ident(&parser_copy, type_name).msg[0] != '\0') {
        ParserMsg msg = {parser_copy.line, "expected type"};
        return msg;
    }

    Type* type_ptr = getter(generator, type_name);
    if(type_ptr == NULL) {
        ParserMsg msg = {parser_copy.line, "expected type"};
        return msg;
    }

    *parser = parser_copy;
    *type = *type_ptr;

    return SUCCESS_PARSER_MSG;
}

static ParserMsg Type_parse_struct_literal(inout Parser* parser, in Generator* generator, out Type* type) {
    Parser parser_copy = *parser;

    if(!ParserMsg_is_success(Parser_parse_ident(&parser_copy, type->name))) {
        type->name[0] = '\0';
    }
    type->type = Type_Struct;
    type->property.members = Vec_new(sizeof(StructMember));
    type->ref_depth = 0;
    
    Parser block_parser;
    PARSERMSG_UNWRAP(
        Parser_parse_block(&parser_copy, &block_parser)
    );

    while(!Parser_is_empty(&block_parser)) {
        StructMember struct_member;
        PARSERMSG_UNWRAP(
            StructMember_parse(&block_parser, generator, &struct_member)
        );
        PARSERMSG_UNWRAP(
            Parser_parse_symbol(&block_parser, ";")
        );
        Vec_push(&type->property.members, &struct_member);
    }

    *parser = parser_copy;

    return SUCCESS_PARSER_MSG;
}

static ParserMsg Type_parse_struct(inout Parser* parser, in Generator* generator, out Type* type) {
    if(Type_parse_helper(parser, generator, Generator_get_struct_types, type).msg[0] == '\0') {
        return SUCCESS_PARSER_MSG;
    }

    return Type_parse_struct_literal(parser, generator, type);
}

ParserMsg Type_parse(inout Parser* parser, in Generator* generator, out Type* type) {
    Parser parser_copy = *parser;

    if(ParserMsg_is_success(Parser_parse_keyword(&parser_copy, "struct"))) {
        PARSERMSG_UNWRAP(
            Type_parse_struct(&parser_copy, generator, type) 
        );
    }else if(ParserMsg_is_success(Parser_parse_keyword(&parser_copy, "enum"))) {
        PARSERMSG_UNWRAP(
            Type_parse_helper(&parser_copy, generator, Generator_get_enum_types, type)
        );
    }else if(ParserMsg_is_success(Parser_parse_keyword(&parser_copy, "union"))) {
        PARSERMSG_UNWRAP(
            Type_parse_helper(&parser_copy, generator, Generator_get_union_types, type)
        );
    }else {
        PARSERMSG_UNWRAP(
            Type_parse_helper(&parser_copy, generator, Generator_get_normal_types, type)
        );
    }
    
    *parser = parser_copy;
    
    return SUCCESS_PARSER_MSG;
}

static void Type_print_structmember(void* self) {
    StructMember_print((StructMember*)self);
    return;
}

static void Type_print_enummember(void* self) {
    EnumMember_print((EnumMember*)self);
    return ;
}

void Type_print(Type* self) {
    printf("Type { name: %s, type: %d, property: ", self->name, self->type);
    switch(self->type) {
        case Type_Struct:
        case Type_Union:
            Vec_print(&self->property.members, Type_print_structmember);
            break;
        case Type_Enum:
            Vec_print(&self->property.members, Type_print_enummember);
            break;
        default:
            printf("none");
            break;
    }
    printf(", ref_depth: %d }", self->ref_depth);

    return;
}

void Type_free(Type self) {
    switch(self.type) {
        case Type_Struct:
        case Type_Union:
            Vec_free(self.property.members);
            break;
        case Type_Enum:
            Vec_free(self.property.enums);
            break;
        default:
    }

    return;
}

void StructMember_print(StructMember* self) {
    printf("StructMember { name: %s, type: ", self->name);
    Type_print(&self->type);
    printf(" }");
    return;
}

void EnumMember_print(EnumMember* self) {
    printf("EnumMember { name: %s, value: %d }", self->name, self->value);
    return;
}

void Data_free(Data self) {
    Type_free(self.type);
}

void Variable_free(Variable self) {
   Data_free(self.data); 
}

ParserMsg StructMember_parse(inout Parser* parser, in Generator* generator, out StructMember* struct_member) {
    // ex: i32 n
    Parser parser_copy = *parser;
    ParserMsg type_msg = Type_parse(&parser_copy, generator, &struct_member->type);
    if(type_msg.msg[0] != '\0') {
        return type_msg;
    }

    ParserMsg ident_msg = Parser_parse_ident(&parser_copy, struct_member->name);
    if(ident_msg.msg[0] != '\0') {
        return ident_msg;
    }

    *parser = parser_copy;

    return SUCCESS_PARSER_MSG;
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

