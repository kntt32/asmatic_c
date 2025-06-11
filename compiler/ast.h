#pragma once

#include "types.h"
#include "gen.h"

typedef struct {
    enum { ImmValue_Integral, ImmValue_Floating, ImmValue_VariablePtr, ImmValue_FnPtr, ImmValue_StringLiteral } type;
    union { u64 integral; f64 floating; Variable variable_ptr; Function fnptr; char* string_literal; } body;
} ImmValue;

struct AstNode;

typedef struct {
    char name[16];
    bool left_arg;
    bool right_arg;
    u32 priority;
    SResult (*eval)(in struct AstNode* node, in Generator* generator, out ImmValue* ptr);
} Operator;

struct AstNode {
    enum { AstNode_Operator, AstNode_Number, AstNode_Variable, AstNode_Function, AstNode_Type, AstNode_StringLiteral } type;
    union {
        struct { Operator operator; optional struct AstNode* left; optional struct AstNode* right; } operator;
        char number[256];
        char variable[256];
        struct { struct AstNode* function_ptr; Vec arguments;/* Vec<AstTree> */ } function;
        Type type;
        char* string_literal;
    } body;
};

typedef struct AstNode AstNode;

typedef struct {
    AstNode* node;
} AstTree;

SResult ImmValue_as_integer(in ImmValue* self, out u64* value);

SResult ImmValue_as_floating(in ImmValue* self, out f64* value);

void ImmValue_print(in ImmValue* self);

void ImmValue_free(ImmValue self);

void Operator_print(in Operator* self);

bool Operator_cmp(in Operator* self, in Operator* other);

void AstNode_print(in AstNode* self);

void AstNode_free(AstNode self);

SResult AstNode_eval(in AstNode* self, in Generator* generator, out ImmValue* imm_value);

void AstTree_print(in AstTree* self);

void AstTree_free(AstTree self);

ParserMsg AstTree_parse(Parser parser, in Generator* generator, out AstTree* ptr);

SResult AstTree_eval(in AstTree* self, in Generator* generator, out ImmValue* imm_value);

