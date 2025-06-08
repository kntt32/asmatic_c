#pragma once
#include "types.h"
#include "vec.h"
#include "str.h"
#include "register.h"

typedef enum {
    ValueType_Default,
    ValueType_String,
    ValueType_SignedInt,
    ValueType_UnsignedInt
} ValueType;

typedef struct {
    char name[256];
    enum {Type_Normal, Type_Struct, Type_Enum, Type_Union, Type_FnPtr} type;
    union {
        Vec members;// Vec<Variable> struct/union
        Vec enums;  // Vec<EnumMember> enum
    } property;
    u32 ref_depth;
    u32 align;
    u32 size;
    ValueType value_type;
} Type;

typedef struct {
    char name[256];
    i32 value;
} EnumMember;

typedef struct {
    enum {Storage_Default, Storage_Register, Storage_Stack, Storage_Data } type;
    union {Register reg; i32 base_offset;} place;
} Storage;

typedef struct {
    Type type;
    Storage storage;
} Data;

typedef struct {
    char name[256];
    Data data;
    bool const_flag;
    bool static_flag;
    i32 len;// -1: not array
} Variable;

typedef struct {
    char name[256];
    Vec arguments;// Vec<Variable>
    Data data;
    bool is_static;
} Function;

typedef struct {
    Vec stack;// Vec<Variable>
} VariableStack;

typedef struct {
    u32 line;
    char msg[256];
} Error;

typedef struct {
    char filename[256];
    
    u32 stack;

    Vec normal_types;// Vec<Type>
    Vec struct_types;// Vec<Type>
    Vec enum_types;// Vec<Type>
    Vec union_types;// Vec<Type>

    Vec functions;// Vec<Function>

    Vec global_variables;// Vec<Variable>
    Vec local_variables;// Vec<Variable>

    struct {
        String text;
        String data;
    } code;
    
    Vec errors;// Vec<Error>
} Generator;

extern Type TYPE_VOID;

ParserMsg Type_parse(inout Parser* parser, in Generator* generator, out Type* type);
void Type_print(Type* self);
Type Type_clone(in Type* self);
void Type_free(Type self);

ParserMsg EnumMember_parse(inout Parser* parser, in Generator* generator, out EnumMember* enum_member);
void EnumMember_print(EnumMember* self);

ParserMsg Storage_parse(inout Parser* parser, inout Generator* generator, in Type* type, out Storage* storage);
void Storage_print(in Storage* self);

ParserMsg Data_parse(inout Parser* parser, inout Generator* generator, out Data* data);
void Data_print(in Data* self);
Data Data_clone(in Data* self);
void Data_free(Data self);

ParserMsg Variable_parse(inout Parser* parser, in Generator* generator, out Variable* variable);
void Variable_print(in Variable* self);
Variable Variable_clone(in Variable* self);
void Variable_free(Variable self);

ParserMsg Function_parse(inout Parser* parser, inout Generator* generator, out Function* function);
void Function_print(Function* self);
Function Function_clone(in Function* self);
void Function_free(Function self);

VariableStack VariableStack_new(void);
void VariableStack_free(VariableStack self);
void VariableStack_push(inout VariableStack* self, in Variable* variable);
optional Variable* VariableStack_get(inout VariableStack* self, in char* name);

Generator Generator_new(optional in char* filename);
void Generator_print(in Generator* self);
optional Type* Generator_get_normal_types(in Generator* self, in char* name);
optional Type* Generator_get_struct_types(in Generator* self, in char* name); 
optional Type* Generator_get_enum_types(in Generator* self, in char* name); 
optional Type* Generator_get_union_types(in Generator* self, in char* name); 
SResult Generator_add_normal_type(out Generator* self, Type type);
SResult Generator_add_struct_type(out Generator* self, Type type);
SResult Generator_add_enum_type(out Generator* self, Type type);
SResult Generator_add_union_type(out Generator* self, Type type);
void Generator_asm_add_text(inout Generator* self, in char* s);
void Generator_asm_add_data(inout Generator* self, in char* s);
void Generator_add_error(inout Generator* self, in Error err);
u32 Generator_stack_push(inout Generator* self, in Type* type);
u32 Generator_stack_get(inout Generator* self);
void Generator_add_function(inout Generator* self, Function function);
optional Function* Generator_get_function(inout Generator* self, in char* name);
void Generator_add_global_variable(inout Generator* self, Variable variable);
void Generator_add_local_variable(inout Generator* self, Variable variable);
optional Variable* Generator_get_variable(in Generator* self, in char* name);
u32 Generator_get_local_variables_count(in Generator* self);
void Generator_set_local_variables_count(in Generator* self, u32 count);
void Generator_stack_set(inout Generator* self, u32 offset);
void Generator_free(Generator self);

