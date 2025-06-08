#pragma once

#include "types.h"
#include "gen.h"

typedef struct {
    enum { ImmValue_String, ImmValue_Integral, ImmValue_Floating } type;
    union { String string; u64 integral; f64 floating; } body;
} ImmValue;

typedef struct {
    char name[16];
    bool left_arg;
    bool right_arg;
    u32 priority;
} Operator;

struct AstNode;

struct AstNode {
    enum { AstNode_Operator, AstNode_Number, AstNode_Variable, AstNode_Function, AstNode_Type } type;
    union {
        struct { Operator operator; optional struct AstNode* left; optional struct AstNode* right; } operator;
        char number[256];
        char variable[256];
        struct { struct AstNode* function_ptr; Vec arguments;/* Vec<AstTree> */ } function;
        Type type;
    } body;
};

typedef struct AstNode AstNode;

typedef struct {
    AstNode* node;
} AstTree;

void ImmValue_print(in ImmValue* self);

void ImmValue_free(ImmValue self);

void Operator_print(in Operator* self);

bool Operator_cmp(in Operator* self, in Operator* other);

void AstNode_print(in AstNode* self);

void AstNode_free(AstNode self);

optional ImmValue* AstNode_eval(in AstNode* self, out ImmValue* imm_value);

void AstTree_print(in AstTree* self);

void AstTree_free(AstTree self);

ParserMsg AstTree_parse(Parser parser, in Generator* generator, out AstTree* ptr);

ImmValue* AstTree_eval(in AstTree* self, out ImmValue* imm_value);

