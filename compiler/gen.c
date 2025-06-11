#include <stdio.h>
#include "types.h"
#include "gen.h"
#include "parser.h"
#include "register.h"

Type DATA_VOID = {"void", Type_Normal, {}, 0, 1, 0, ValueType_Default};
static Type EXPLICIT_NORMAL_TYPES[] = {
    {"void", Type_Normal, {}, 0, 1, 0, ValueType_Default},

    {"char", Type_Normal, {}, 0, 1, 1, ValueType_UnsignedInt},
    {"bool", Type_Normal, {}, 0, 1, 1, ValueType_UnsignedInt},

    {"i8", Type_Normal, {}, 0, 1, 1, ValueType_SignedInt},
    {"u8", Type_Normal, {}, 0, 1, 1, ValueType_UnsignedInt},
    {"i16", Type_Normal, {}, 0, 2, 2, ValueType_SignedInt},
    {"u16", Type_Normal, {}, 0, 2, 2, ValueType_UnsignedInt},
    {"i32", Type_Normal, {}, 0, 4, 4, ValueType_SignedInt},
    {"u32", Type_Normal, {}, 0, 4, 4, ValueType_UnsignedInt},
    {"i64", Type_Normal, {}, 0, 8, 8, ValueType_SignedInt},
    {"u64", Type_Normal, {}, 0, 8, 8, ValueType_UnsignedInt},

    {"f32", Type_Normal, {}, 0, 4, 4, ValueType_Default},
    {"f64", Type_Normal, {}, 0, 8, 8, ValueType_Default},
};

static ParserMsg Type_parse_helper(inout Parser* parser, in Generator* generator, optional Type* (in *getter)(in Generator*, in char*), out Type* type) {
    Parser parser_copy = *parser;

    char type_name[256] = "";
    
    if(!ParserMsg_is_success(Parser_parse_ident(&parser_copy, type_name))) {
        ParserMsg msg = {parser_copy.line, "expected type"};
        return msg;
    }

    Type* type_ptr = getter(generator, type_name);
    if(type_ptr == NULL) {
        ParserMsg msg = {parser_copy.line, "expected type"};
        return msg;
    }

    *parser = parser_copy;
    *type = Type_clone(type_ptr);

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

    type->property.members = Vec_new(sizeof(Variable));
    u32 offset = 0;
    u32 align = 1;

    while(!Parser_is_empty(&parser_copy)) {
        Variable struct_member;
        PARSERMSG_UNWRAP(
            Variable_parse(&parser_copy, generator, &struct_member),
            Type_free(*type)
        );
        PARSERMSG_UNWRAP(
            Parser_parse_symbol(&parser_copy, ";"),
            Type_free(*type)
        );
        if(struct_member.data.storage.type != Storage_Default) {
            Type_free(*type);
            ParserMsg msg = {parser_copy.line, "storage must be Storage_Default"};
            return msg;
        }
        struct_member.data.storage.type = Storage_Stack;
        u32 member_align = struct_member.data.type.align;
        struct_member.data.storage.place.base_offset = ((offset + member_align - 1)/member_align)*member_align;
        if(struct_member.len < 0) {
            offset = struct_member.data.storage.place.base_offset + struct_member.data.type.size;
        }else {
            offset = struct_member.data.storage.place.base_offset + struct_member.data.type.size * struct_member.len;
        }
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
    if(!ParserMsg_is_success(Type_parse_helper(parser, generator, Generator_get_struct_types, type))) {
        PARSERMSG_UNWRAP(
            Type_parse_literal(parser, generator, Type_parse_literal_struct_inner_parser, type),
            (void)(NULL)
        );
        type->type = Type_Struct;
        type->value_type = ValueType_Default;
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
        type->value_type = ValueType_UnsignedInt;

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
        type->value_type = ValueType_Default;
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

static bool Type_cmp_variable_cmp(in void* x, in void* y) {
    return Variable_cmp((Variable*)x, (Variable*)y);
}

static bool Type_cmp_enummember_cmp(in void* x, in void* y) {
    return EnumMember_cmp(x, y);
}

bool Type_cmp(in Type* self, in Type* other) {
    if(self->type != other->type) {
        return false;
    }

    switch(self->type) {
        case Type_Struct:
        case Type_Union:
            if(!Vec_cmp(&self->property.members, &other->property.members, Type_cmp_variable_cmp)) {
                return false;
            }
            break;
        case Type_Enum:
            if(!Vec_cmp(&self->property.enums, &other->property.members, Type_cmp_enummember_cmp)) {
                return false;
            }
            break;
        default:
            break;
    }

    return self->ref_depth == other->ref_depth
        && self->align == other->align
        && self->size == other->size
        && self->value_type == other->value_type;
}

static void Type_print_structmember(void* self) {
    Variable_print((Variable*)self);
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

static void Type_clone_variable(out void* dst, in void* src) {
    Variable* dst_type = dst;
    Variable* src_type = src;

    *dst_type = Variable_clone(src_type);

    return;
}

Type Type_clone(in Type* self) {
    Type type;

    strcpy(type.name, self->name);
    type.type = self->type;
    switch(type.type) {
        case Type_Struct:
        case Type_Union:
            type.property.members = Vec_clone(&self->property.members, Type_clone_variable);
            break;
        case Type_Enum:
            type.property.enums = Vec_clone(&self->property.enums, NULL);
            break;
        default:
            break;
    }

    type.ref_depth = self->ref_depth;
    type.align = self->align;
    type.size = self->size;

    return type;
}

void Type_free(Type self) {
    switch(self.type) {
        case Type_Struct:
        case Type_Union:
            for(u32 i=0; i<Vec_len(&self.property.members); i++) {
                Variable* member = Vec_index(&self.property.members, i);
                Variable_free(*member);
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

bool EnumMember_cmp(in EnumMember* self, in EnumMember* other) {
    return strcmp(self->name, other->name) && self->value == other->value;
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

bool Storage_cmp(in Storage* self, in Storage* other) {
    if(self->type != other->type) {
        return false;
    }

    switch(self->type) {
        case Storage_Default:
        case Storage_Data:
            return true;
        case Storage_Register:
            return self->place.reg == other->place.reg;
        case Storage_Stack:
            return self->place.base_offset == other->place.base_offset;
    }

    PANIC("unreachable");
    return false;
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

    if(ParserMsg_is_success(Parser_parse_symbol(&parser_copy, "@"))) {
        PARSERMSG_UNWRAP(
            Storage_parse(&parser_copy, generator, &data->type, &data->storage),
            (void)(NULL)
        );
    }else {
        data->storage.type = Storage_Default;
    }

    *parser = parser_copy;

    return SUCCESS_PARSER_MSG;
}

bool Data_cmp(in Data* self, in Data* other) {
    return Type_cmp(&self->type, &other->type)
        && Storage_cmp(&self->storage, &other->storage);
}

void Data_print(in Data* self) {
    printf("Data { type: ");
    Type_print(&self->type);
    printf(", storage: ");
    Storage_print(&self->storage);
    printf(" }");
    
    return;
}

Data Data_clone(in Data* self) {
    Data data;

    data.type = Type_clone(&self->type);
    data.storage = self->storage;

    return data;
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

bool Variable_cmp(in Variable* self, in Variable* other) {
    return strcmp(self->name, other->name)
        && Data_cmp(&self->data, &other->data)
        && self->const_flag == other->const_flag
        && self->static_flag == other->static_flag
        && self->len == other->len;
}

void Variable_print(in Variable* self) {
    printf("Variable { name: %s, data: ", self->name);
    Data_print(&self->data);
    printf(", const_flag: %s, static_flag: %s, len: %d }", BOOL_TO_STR(self->const_flag), BOOL_TO_STR(self->static_flag), self->len);

    return;
}

Variable Variable_clone(in Variable* self) {
    Variable variable;
    
    strcpy(variable.name, self->name);
    variable.data = Data_clone(&self->data);
    variable.const_flag = self->const_flag;
    variable.static_flag = self->static_flag;
    variable.len = self->len;

    return variable;
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

static void Function_clone_vec_variable(out void* dest, in void* src) {
    Variable* dest_ptr = dest;
    Variable* src_ptr = src;

    *dest_ptr = Variable_clone(src_ptr);

    return;
}

Function Function_clone(in Function* self) {
    Function function;
    
    strcpy(function.name, self->name);
    function.arguments = Vec_clone(&self->arguments, Function_clone_vec_variable);
    function.data = Data_clone(&self->data);
    function.is_static = self->is_static;

    return function;
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

VariableStack VariableStack_new(void) {
    VariableStack variable_stack;
    variable_stack.stack = Vec_new(sizeof(Variable));
    
    return variable_stack;
}

void VariableStack_free(VariableStack self) {
    for(u32 i=0; i<Vec_len(&self.stack); i++) {
        Variable* variable = Vec_index(&self.stack, i);
        Variable_free(*variable);
    }

    Vec_free(self.stack);

    return;
}

void VariableStack_push(inout VariableStack* self, in Variable* variable) {
    Vec_push(&self->stack, variable);
    return;
}

optional Variable* VariableStack_get(inout VariableStack* self, in char* name) {
    for(u32 i=0; i<Vec_len(&self->stack); i++) {
        Variable* variable = Vec_index(&self->stack, i);
        if(strcmp(variable->name, name) == 0) {
            return variable;
        }
    }

    return  NULL;
}

u32 VariableStack_get_depth(in VariableStack* self) {
    return Vec_len(&self->stack);
}

void VariableStack_set_depth(inout VariableStack* self, u32 depth) {
    while(depth < Vec_len(&self->stack)) {
        Variable variable;
        Vec_pop(&self->stack, &variable);
        Variable_free(variable);
    }

    return;
}

void Error_print(in Error* self) {
    printf("Error { line: %u, msg: %s }", self->line, self->msg);
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
    generator.local_variables = Vec_new(sizeof(Variable));

    generator.code.text = String_new();
    generator.code.data = String_new();

    generator.errors = Vec_new(sizeof(Error));

    return generator;
}

static void Generator_print_type(void* ptr) {
    Type_print(ptr);
}

static void Generator_print_variable(void* ptr) {
    Variable_print(ptr);
}

static void Generator_print_function(void* ptr) {
    Function_print(ptr);
}

static void Generator_print_error(void* ptr) {
    Error_print(ptr);
}

void Generator_print(in Generator* self) {
    printf("Generator { filename: %s, normal_types: ", self->filename);
    Vec_print(&self->normal_types, Generator_print_type);
    printf(", struct_types: ");
    Vec_print(&self->struct_types, Generator_print_type);
    printf(", enum_types: ");
    Vec_print(&self->enum_types, Generator_print_type);
    printf(", union_types: ");
    Vec_print(&self->union_types, Generator_print_type);
    printf(", functions: ");
    Vec_print(&self->functions, Generator_print_function);
    printf(", global_variables: ");
    Vec_print(&self->global_variables, Generator_print_variable);
    printf(", code: { text: ");
    String_print(&self->code.text);
    printf(", data: ");
    String_print(&self->code.data);
    printf(" }, errors: ");
    Vec_print(&self->errors, Generator_print_error);
    printf(" }");

    return;
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

static SResult Generator_add_type_helper(inout Vec* types_vec, Type type) {
    for(u32 i=0; i<Vec_len(types_vec); i++) {
        Type* i_type = Vec_index(types_vec, i);

        if(strcmp(i_type->name, type.name) == 0) {
            if(i_type->type == Type_Struct || i_type->type == Type_Union) {
                if(Vec_len(&i_type->property.members) == 0) {
                    continue;
                }
            }
            SResult result;
            result.ok_flag = false;
            snprintf(result.error, 256, "type \"%.10s\" has been already defined", type.name);
            result.error[255] = '\0';
            return result;
        }
    }

    Vec_push(types_vec, &type);

    return SRESULT_OK;
}

SResult Generator_add_normal_type(out Generator* self, Type type) {
    return Generator_add_type_helper(&self->normal_types, type);
}

SResult Generator_add_struct_type(out Generator* self, Type type) {
    return Generator_add_type_helper(&self->struct_types, type);
}

SResult Generator_add_enum_type(out Generator* self, Type type) {
    return Generator_add_type_helper(&self->enum_types, type);
}

SResult Generator_add_union_type(out Generator* self, Type type) {
    return Generator_add_type_helper(&self->union_types, type);
}

void Generator_asm_add_text(inout Generator* self, in char* s) {
    String_append(&self->code.text, s);
    return;
}

void Generator_asm_add_data(inout Generator* self, in char* s) {
    String_append(&self->code.text, s);
    return;
}

void Generator_add_error(inout Generator* self, in Error err) {
    Vec_push(&self->errors, &err);
    return;
}

u32 Generator_stack_push(inout Generator* self, in Type* type) {
    u32 stack_offset = (self->stack + type->align - 1)/type->align*type->align;
    self->stack = stack_offset + type->size;
    return stack_offset;
}

u32 Generator_stack_get(inout Generator* self) {
    return self->stack;
}

void Generator_stack_set(inout Generator* self, u32 offset) {
    self->stack = offset;
    return;
}

static bool Generator_add_function_variable_cmp(in void* x, in void* y) {
    return Variable_cmp((Variable*)x, (Variable*)y);
}

SResult Generator_add_function(inout Generator* self, Function function) {
    if(Generator_get_variable(self, function.name) != NULL) {
        SResult result = {false, "variable has been defined"};
        return result;
    }
    
    optional Function* f_ptr = Generator_get_function(self, function.name);
    if(f_ptr != NULL && !Vec_cmp(&f_ptr->arguments, &function.arguments, Generator_add_function_variable_cmp)) {
        SResult result = {false, "function definition conflict occured"};
        return result;
    }
    
    Vec_push(&self->functions, &function);

    return SRESULT_OK;
}

optional Function* Generator_get_function(in Generator* self, in char* name) {
    for(u32 i=0; i<Vec_len(&self->functions); i++) {
        Function* function = Vec_index(&self->functions, i);
        if(strcmp(function->name, name) == 0) {
            return function;
        }
    }

    return NULL;
}

static SResult Generator_add_variable_checker(in Generator* self, Variable* variable) {
    if(Generator_get_variable(self, variable->name) != NULL || Generator_get_function(self, variable->name) != NULL) {
        SResult result = {false, "variable has been already defined"};
        return result;
    }

    return SRESULT_OK;
}

SResult Generator_add_global_variable(inout Generator* self, Variable variable) {
    SRESULT_UNWRAP(
        Generator_add_variable_checker(self, &variable),
        (void)NULL
    );
    Vec_push(&self->global_variables, &variable);
    
    return SRESULT_OK;
}

SResult Generator_add_local_variable(inout Generator* self, Variable variable) {
    SRESULT_UNWRAP(
        Generator_add_variable_checker(self, &variable),
        (void)NULL
    );
    Vec_push(&self->local_variables, &variable);
    
    return SRESULT_OK;
}

optional Variable* Generator_get_variable(in Generator* self, in char* name) {
    for(u32 i=0; i<Vec_len(&self->local_variables); i++) {
        Variable* ptr = Vec_index(&self->local_variables, i);
        if(strcmp(ptr->name, name) == 0) {
            return ptr;
        }
    }

    for(u32 i=0; i<Vec_len(&self->global_variables); i++) {
        Variable* ptr = Vec_index(&self->global_variables, i);
        if(strcmp(ptr->name, name) == 0) {
            return ptr;
        }
    }

    return NULL;
}

u32 Generator_get_local_variables_count(in Generator* self) {
    return Vec_len(&self->local_variables);
}

void Generator_set_local_variables_count(in Generator* self, u32 count) {
    u32 len = Vec_len(&self->local_variables);
    if(len < count) {
        PANIC("count must be lower than variables count");
    }

    for(u32 i=0; i<len-count; i++) {
        Variable variable;
        Vec_pop(&self->local_variables, &variable);
        Variable_free(variable);
    }

    return;
}

static void Generator_free_type_free(void* ptr) {
    Type* type_ptr = ptr;
    Type_free(*type_ptr);
    return;
}

static void Generator_free_function_free(void* ptr) {
    Function* func_ptr = ptr;
    Function_free(*func_ptr);
    return;
}

static void Generator_free_variable_free(void* ptr) {
    Variable* variable_ptr = ptr;
    Variable_free(*variable_ptr);
    return;
}

void Generator_free(Generator self) {
    Vec_free_all(self.normal_types, Generator_free_type_free);
    Vec_free_all(self.struct_types, Generator_free_type_free);
    Vec_free_all(self.enum_types, Generator_free_type_free);
    Vec_free_all(self.union_types, Generator_free_type_free);

    Vec_free_all(self.functions, Generator_free_function_free);
    
    Vec_free_all(self.global_variables, Generator_free_variable_free);
    Vec_free_all(self.local_variables, Generator_free_variable_free);

    String_free(self.code.text);
    String_free(self.code.data);

    Vec_free(self.errors);

    return;
}

