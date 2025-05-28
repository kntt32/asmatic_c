#pragma once
#include "types.h"
#include "vec.h"
#include "string.h"
#include "register.h"

typedef struct {
    char name[256];
    enum {Type_primitive, Type_Struct, Type_Enum, Type_Union, Type_FnPtr} type;
    union {} property;
    u32 ref_depth;
} Type;

typedef struct {
    enum {Storage_Register, Storage_Stack, Storage_Data} type;
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
} Function;

typedef struct {
    char filename[256];
    u32 stack_usage;
    Vec normal_types;// Vec<Type>
    Vec struct_typees;// Vec<Type>
    Vec enum_types;// Vec<Type>
    Vec union_types;// Vec<Type>

    Vec functions;// Vec<Function>

    Vec global_variables;// Vec<Variable>
    Vec auto_variables;// Vec<Variable>
} Generator;

