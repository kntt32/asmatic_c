#pragma once
#include "types.h"
#include "vec.h"
#include "str.h"
#include "register.h"

typedef struct {
    char name[256];
    enum {Type_Normal, Type_Struct, Type_Enum, Type_Union, Type_FnPtr} type;
    union {
        Vec members;// Vec<StructMember> struct/union
        Vec enums;  // Vec<EnumMember> enum
    } property;
    u32 ref_depth;
    u32 align;
    u32 size;
} Type;

typedef struct {
    char name[256];
    Type type;
    u32 offset;
} StructMember;

typedef struct {
    char name[256];
    i32 value;
} EnumMember;

typedef struct {
    enum {Storage_Default, Storage_Register, Storage_Stack, Storage_Data} type;
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
    char filename[256];
    
    u32 stack;

    Vec normal_types;// Vec<Type>
    Vec struct_types;// Vec<Type>
    Vec enum_types;// Vec<Type>
    Vec union_types;// Vec<Type>

    Vec functions;// Vec<Function>

    Vec global_variables;// Vec<Variable>

    String code;
    String error;
} Generator;

extern Type TYPE_VOID;

ParserMsg Type_parse(inout Parser* parser, in Generator* generator, out Type* type);

void Type_print(Type* self);

void Type_free(Type self);

ParserMsg StructMember_parse(inout Parser* parser, in Generator* generator, out StructMember* struct_member);

void StructMember_print(StructMember* self);

void StructMember_free(StructMember self);

ParserMsg EnumMember_parse(inout Parser* parser, in Generator* generator, out EnumMember* enum_member);

void EnumMember_print(EnumMember* self);

ParserMsg Storage_parse(inout Parser* parser, inout Generator* generator, in Type* type, out Storage* storage);

void Storage_print(in Storage* self);

ParserMsg Data_parse(inout Parser* parser, inout Generator* generator, out Data* data);

void Data_print(in Data* self);

void Data_free(Data self);

ParserMsg Variable_parse(inout Parser* parser, in Generator* generator, out Variable* variable);

void Variable_print(in Variable* self);

void Variable_free(Variable self);

ParserMsg Function_parse(inout Parser* parser, inout Generator* generator, out Function* function);

void Function_print(Function* self);

void Function_free(Function self);

VariableStack VariableStack_new(void);

void VariableStack_free(VariableStack self);

void VariableStack_push(inout VariableStack* self, in Variable* variable);

u32 VariableStack_get_depth(in VariableStack* self);

void VariableStack_set_depth(inout VariableStack* self, u32 depth);

Generator Generator_new(optional in char* filename);

optional Type* Generator_get_normal_types(in Generator* self, in char* name);

optional Type* Generator_get_struct_types(in Generator* self, in char* name); 

optional Type* Generator_get_enum_types(in Generator* self, in char* name); 

optional Type* Generator_get_union_types(in Generator* self, in char* name); 

SResult Generator_add_normal_type(out Generator* self, in Type* type);

SResult Generator_add_struct_type(out Generator* self, in Type* type);

SResult Generator_add_enum_type(out Generator* self, in Type* type);

SResult Generator_add_union_type(out Generator* self, in Type* type);

u32 Generator_stack_push(inout Generator* self, in Type* type);

void Generator_free(Generator self);

