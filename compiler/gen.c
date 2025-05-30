#include <stdio.h>
#include "types.h"
#include "gen.h"
#include "parser.h"
#include "register.h"

Type DATA_VOID = {"void", Type_Normal, {}, 0, 1, 0};
static Type EXPLICIT_NORMAL_TYPES[] = {
    {"void", Type_Normal, {}, 0, 1, 0},

    {"char", Type_Normal, {}, 0, 1, 1},
    {"bool", Type_Normal, {}, 0, 1, 1},

    {"i8", Type_Normal, {}, 0, 1, 1},
    {"u8", Type_Normal, {}, 0, 1, 1},
    {"i16", Type_Normal, {}, 0, 2, 2},
    {"u16", Type_Normal, {}, 0, 2, 2},
    {"i32", Type_Normal, {}, 0, 4, 4},
    {"u32", Type_Normal, {}, 0, 4, 4},
    {"i64", Type_Normal, {}, 0, 8, 8},
    {"u64", Type_Normal, {}, 0, 8, 8},

    {"f32", Type_Normal, {}, 0, 4, 4},
    {"f64", Type_Normal, {}, 0, 8, 8},
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

static ParserMsg Type_parse_literal(
    inout Parser* parser,
    in Generator* generator,
    in ParserMsg (*inner_parser)(inout Parser*, in Generator*, out Type*),
    out Type* type
) {
    Parser parser_copy = *parser;

    if(!ParserMsg_is_success(Parser_parse_ident(&parser_copy, type->name))) {
        type->name[0] = '\0';
    }
    type->ref_depth = 0;
    
    Parser block_parser;
    PARSERMSG_UNWRAP(
        Parser_parse_block(&parser_copy, &block_parser),
        (void)(NULL)
    );

    PARSERMSG_UNWRAP(
        inner_parser(&block_parser, generator, type),
        (void)(NULL)
    );

    *parser = parser_copy;

    return SUCCESS_PARSER_MSG;

}

static ParserMsg Type_parse_literal_struct_inner_parser(inout Parser* parser, in Generator* generator, out Type* type) {
    Parser parser_copy = *parser;

    type->property.members = Vec_new(sizeof(StructMember));
    u32 offset = 0;
    u32 align = 1;

    while(!Parser_is_empty(&parser_copy)) {
        StructMember struct_member;
        PARSERMSG_UNWRAP(
            StructMember_parse(&parser_copy, generator, &struct_member),
            Vec_free(type->property.members)
        );
        PARSERMSG_UNWRAP(
            Parser_parse_symbol(&parser_copy, ";"),
            Vec_free(type->property.members)
        );
        u32 member_align = struct_member.type.align;
        struct_member.offset = ((offset + member_align - 1)/member_align)*member_align;
        offset = struct_member.offset + struct_member.type.size;
        align = MAX(align, member_align);

        Vec_push(&type->property.members, &struct_member);
    }
    type->align = align;
    type->size = offset;
    *parser = parser_copy;

    return SUCCESS_PARSER_MSG;
}

static ParserMsg Type_parse_literal_enum_inner_parser(inout Parser* parser, in Generator* generator, out Type* type) {
    Parser parser_copy = *parser;

    type->property.enums = Vec_new(sizeof(EnumMember));

    while(!Parser_is_empty(&parser_copy)) {
        EnumMember member;
        PARSERMSG_UNWRAP(
            EnumMember_parse(&parser_copy, generator, &member),
            Vec_free(type->property.enums)
        );
        
        ParserMsg msg = Parser_parse_symbol(&parser_copy, ",");
        if(!ParserMsg_is_success(msg) && !Parser_is_empty(&parser_copy)) {
            return msg;
        }

        Vec_push(&type->property.enums, &member);
    }
    type->align = 4;
    type->size = 4;

    *parser = parser_copy;

    return SUCCESS_PARSER_MSG;
}

static ParserMsg Type_parse_literal_union_inner_parser(inout Parser* parser, in Generator* generator, out Type* type) {
    return Type_parse_literal_struct_inner_parser(parser, generator, type);
}

static ParserMsg Type_parse_struct(inout Parser* parser, in Generator* generator, out Type* type) {
    if(!Type_parse_helper(parser, generator, Generator_get_struct_types, type).msg[0] == '\0') {
        PARSERMSG_UNWRAP(
            Type_parse_literal(parser, generator, Type_parse_literal_struct_inner_parser, type),
            (void)(NULL)
        );
        type->type = Type_Struct;
    }

    return SUCCESS_PARSER_MSG;
}

static ParserMsg Type_parse_enum(inout Parser* parser, in Generator* generator, out Type* type) {
    if(!ParserMsg_is_success(Type_parse_helper(parser, generator, Generator_get_enum_types, type))) {
        PARSERMSG_UNWRAP(
            Type_parse_literal(parser, generator, Type_parse_literal_enum_inner_parser, type),
            (void)(NULL)
        );
        type->type = Type_Enum;

        i32 value = 0;
        for(u32 i=0; i<Vec_len(&type->property.enums); i++) {
            EnumMember* member = Vec_index(&type->property.enums, i);
            if(member->value != 0) {
                value = member->value;
            }else {
                member->value = value;
            }
            value ++;
        }
    }

    return SUCCESS_PARSER_MSG;
}

static ParserMsg Type_parse_union(inout Parser* parser, in Generator* generator, out Type* type) {
    if(!ParserMsg_is_success(Type_parse_helper(parser, generator, Generator_get_union_types, type))) {
        PARSERMSG_UNWRAP(
            Type_parse_literal(parser, generator, Type_parse_literal_union_inner_parser, type),
            (void)(NULL)
        );
        type->type = Type_Union;
    }
   
    return SUCCESS_PARSER_MSG;
}

static u32 Type_parse_get_ref_depth(inout Parser* parser) {
    u32 ref_depth = 0;
    
    while(ParserMsg_is_success(Parser_parse_symbol(parser, "*"))) {
        ref_depth ++;
    }

    return ref_depth;
}

ParserMsg Type_parse(inout Parser* parser, in Generator* generator, out Type* type) {
    Parser parser_copy = *parser;

    if(ParserMsg_is_success(Parser_parse_keyword(&parser_copy, "struct"))) {
        PARSERMSG_UNWRAP(
            Type_parse_struct(&parser_copy, generator, type),
            (void)(NULL)
        );
    }else if(ParserMsg_is_success(Parser_parse_keyword(&parser_copy, "enum"))) {
        PARSERMSG_UNWRAP(
            Type_parse_enum(&parser_copy, generator, type),
            (void)(NULL)
        );
    }else if(ParserMsg_is_success(Parser_parse_keyword(&parser_copy, "union"))) {
        PARSERMSG_UNWRAP(
            Type_parse_union(&parser_copy, generator, type),
            (void)(NULL)
        );
    }else {
        PARSERMSG_UNWRAP(
            Type_parse_helper(&parser_copy, generator, Generator_get_normal_types, type),
            (void)(NULL)
        );
    }

    type->ref_depth += Type_parse_get_ref_depth(&parser_copy);
    
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
    printf(", ref_depth: %d, align: %u, size: %u }", self->ref_depth, self->align, self->size);

    return;
}

void Type_free(Type self) {
    switch(self.type) {
        case Type_Struct:
        case Type_Union:
            for(u32 i=0; i<Vec_len(&self.property.members); i++) {
                StructMember* member = Vec_index(&self.property.members, i);
                StructMember_free(*member);
            }
            Vec_free(self.property.members);
            break;
        case Type_Enum:
            Vec_free(self.property.enums);
            break;
        default:
            break;
    }

    return;
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

void StructMember_print(StructMember* self) {
    printf("StructMember { name: %s, type: ", self->name);
    Type_print(&self->type);
    printf(", offset: %u }", self->offset);
    return;
}

void StructMember_free(StructMember self) {
    Type_free(self.type);

    return;
}

ParserMsg EnumMember_parse(inout Parser* parser, in Generator* generator, out EnumMember* enum_member) {
    (void) generator;
    // ex: MyEnumMember1 = 5
    //     MyEnumMember2
    Parser parser_copy = *parser;

    PARSERMSG_UNWRAP(
        Parser_parse_ident(&parser_copy, enum_member->name),
        (void)(NULL)
    );
    i64 value = 0;
    if(ParserMsg_is_success(Parser_parse_symbol(&parser_copy, "="))) {
        PARSERMSG_UNWRAP(
            Parser_parse_number(&parser_copy, &value),
            (void)(NULL)
        );
    }
    enum_member->value = value;

    *parser = parser_copy;

    return SUCCESS_PARSER_MSG;
}

void EnumMember_print(EnumMember* self) {
    printf("EnumMember { name: %s, value: %d }", self->name, self->value);
    return;
}

ParserMsg Storage_parse(inout Parser* parser, inout Generator* generator, in Type* type, out Storage* storage) {
    Register reg;
    if(ParserMsg_is_success(Parser_parse_keyword(parser, "stack"))) {
        storage->type = Storage_Stack;
        storage->place.base_offset = Generator_stack_push(generator, type);
    }else if(ParserMsg_is_success(Parser_parse_keyword(parser, "data"))) {
        storage->type = Storage_Data;
    }else if(ParserMsg_is_success(Register_parse(parser, &reg))) {
        storage->type = Storage_Register;
        storage->place.reg = reg;
    }else {
        ParserMsg msg = {parser->line, "expected storage"};
        return msg;
    }

    return SUCCESS_PARSER_MSG;
}

void Storage_print(in Storage* self) {
    printf("Storage { type: %d, place: { ", self->type);
    switch(self->type) {
        case Storage_Register:
            printf("reg: ");
            Register_print(self->place.reg);
            break;
        case Storage_Stack:
            printf("base_offset: %d", self->place.base_offset);
            break;
        default:
            printf("none");
            break;
    }
    printf(" } }");

    return;
}

ParserMsg Data_parse(inout Parser* parser, inout Generator* generator, out Data* data) {
    Parser parser_copy = *parser;

    PARSERMSG_UNWRAP(
        Type_parse(&parser_copy, generator, &data->type),
        (void)(NULL)
    );

    PARSERMSG_UNWRAP(
        Parser_parse_symbol(&parser_copy, "@"),
        (void)(NULL)
    );

    PARSERMSG_UNWRAP(
        Storage_parse(&parser_copy, generator, &data->type, &data->storage),
        (void)(NULL)
    );

    *parser = parser_copy;

    return SUCCESS_PARSER_MSG;
}

void Data_print(in Data* self) {
    printf("Data { type: ");
    Type_print(&self->type);
    printf(", storage: ");
    Storage_print(&self->storage);
    printf(" }");
    
    return;
}

void Data_free(Data self) {
    Type_free(self.type);
}

ParserMsg Variable_parse(inout Parser* parser, in Generator* generator, out Variable* variable) {
    // (const) (static) $data $name ([$len])
    Parser parser_copy = *parser;

    variable->const_flag = ParserMsg_is_success(Parser_parse_keyword(&parser_copy, "const"));
    variable->static_flag = ParserMsg_is_success(Parser_parse_keyword(&parser_copy, "static"));

    PARSERMSG_UNWRAP(
        Data_parse(&parser_copy, generator, &variable->data),
        (void)(NULL)
    );

    PARSERMSG_UNWRAP(
        Parser_parse_ident(&parser_copy, variable->name),
        (void)(NULL)
    );

    Parser index_parser;
    if(ParserMsg_is_success(Parser_parse_index(&parser_copy, &index_parser))) {
        i64 array_len;
        PARSERMSG_UNWRAP(
            Parser_parse_number(&index_parser, &array_len),
        );
        variable->len = (i32)array_len;

        if(!Parser_is_empty(&index_parser)) {
            ParserMsg msg = {index_parser.line, "expected array length"};
            return msg;
        }
    }else {
        variable->len = -1;
    }

    *parser = parser_copy;

    return SUCCESS_PARSER_MSG;
}

void Variable_print(in Variable* self) {
    printf("Variable { name: %s, data: ", self->name);
    Data_print(&self->data);
    printf(", const_flag: %s, static_flag: %s, len: %d }", BOOL_TO_STR(self->const_flag), BOOL_TO_STR(self->static_flag), self->len);

    return;
}

void Variable_free(Variable self) {
   Data_free(self.data); 
}

static ParserMsg Function_parse_arguments(inout Parser* parser, inout Generator* generator, out Function* function) {
    Parser parser_copy = *parser;

    function->arguments = Vec_new(sizeof(Variable));

    while(!Parser_is_empty(&parser_copy)) {
        Variable arg;
        PARSERMSG_UNWRAP(
           Variable_parse(&parser_copy, generator, &arg),
           (void)(NULL)
        );
        if(arg.data.storage.type == Storage_Data) {
            ParserMsg msg = {parser_copy.line, "variable on .data is not allowed here"};
            return msg;
        }

        Vec_push(&function->arguments, &arg);

        ParserMsg symbol_msg = Parser_parse_symbol(&parser_copy, ",");
        if(!ParserMsg_is_success(symbol_msg) && !Parser_is_empty(&parser_copy)) {
            return symbol_msg;
        }
    }

    *parser = parser_copy;

    return SUCCESS_PARSER_MSG;
}

ParserMsg Function_parse(inout Parser* parser, inout Generator* generator, out Function* function) {
    // (static) $data $name ( $variable , ..)
    Parser parser_copy = *parser;

    function->is_static = ParserMsg_is_success(Parser_parse_keyword(parser, "static"));
    
    PARSERMSG_UNWRAP(
        Data_parse(&parser_copy, generator, &function->data),
        (void)(NULL)
    );

    if(function->data.storage.type == Storage_Data) {
        ParserMsg msg = {parser_copy.line, "return data on .data is not allowed here"};
        return msg;
    }

    PARSERMSG_UNWRAP(
        Parser_parse_ident(&parser_copy, function->name),
        (void)(NULL)
    );

    Parser paren_parser;
    PARSERMSG_UNWRAP(
        Parser_parse_paren(&parser_copy, &paren_parser),
        (void)(NULL)
    );

    PARSERMSG_UNWRAP(
        Function_parse_arguments(&paren_parser, generator, function),
        Function_free(*function)
    );

    *parser = parser_copy;

    return SUCCESS_PARSER_MSG;
}

static void Function_print_variable_print(void* ptr) {
    Variable_print((Variable*)ptr);
    return;
}

void Function_print(Function* self) {
    printf("Function { name: %s, arguments: ", self->name);
    Vec_print(&self->arguments, Function_print_variable_print);
    printf(", data: ");
    Data_print(&self->data);
    printf(", is_static: %s }", BOOL_TO_STR(self->is_static));
}

void Function_free(Function self) {
    for(u32 i=0; i<Vec_len(&self.arguments); i++) {
        Variable* variable = (Variable*)Vec_index(&self.arguments, i);
        Variable_free(*variable);
    }
    Vec_free(self.arguments);
    Data_free(self.data);

    return;
}

Generator Generator_new(optional in char* filename) {
    Generator generator;
    
    if(filename == NULL) {
        filename = "anonymous";
    }
    strcpy(generator.filename, filename);
    
    generator.stack = 0;

    generator.normal_types = Vec_from(EXPLICIT_NORMAL_TYPES, LEN(EXPLICIT_NORMAL_TYPES), sizeof(Type));
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
    for(i32 i=Vec_len(types)-1; 0<=i; i--) {
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

static SResult Generator_add_type_helper(inout Vec* types_vec, in Type* type) {
    for(u32 i=0; i<Vec_len(types_vec); i++) {
        Type* i_type = Vec_index(types_vec, i);

        if(i_type->name == type->name) {
            SResult result;
            result.ok_flag = false;
            snprintf(result.error, 256, "type \"%.10s\" has been already defined", type->name);
            result.error[255] = '\0';
            return result;
        }
    }

    Vec_push(types_vec, type);

    return SRESULT_OK;
}

SResult Generator_add_normal_type(out Generator* self, in Type* type) {
    return Generator_add_type_helper(&self->normal_types, type);
}

SResult Generator_add_struct_type(out Generator* self, in Type* type) {
    return Generator_add_type_helper(&self->struct_types, type);
}

SResult Generator_add_enum_type(out Generator* self, in Type* type) {
    return Generator_add_type_helper(&self->enum_types, type);
}

SResult Generator_add_union_type(out Generator* self, in Type* type) {
    return Generator_add_type_helper(&self->union_types, type);
}

u32 Generator_stack_push(inout Generator* self, in Type* type) {
    u32 stack_offset = (self->stack + type->align - 1)/type->align*type->align;
    self->stack = stack_offset + type->size;
    return stack_offset;
}

void Generator_free(Generator self) {
    Vec_free(self.normal_types);
    Vec_free(self.struct_types);
    Vec_free(self.enum_types);
    Vec_free(self.union_types);

    Vec_free(self.functions);
    
    Vec_free(self.global_variables);
    Vec_free(self.auto_variables);
    
    String_free(self.code);
    String_free(self.error);

    return;
}

