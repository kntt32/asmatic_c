#pragma once

#include "types.h"
#include "gen.h"

typedef struct {
    enum { ImmValue_Number, ImmValue_String } type;
    union { char number[256]; String string; } body;
} ImmValue;

typedef struct {
    char name[16];
    bool left_arg;
    bool right_arg;
    u32 priority;
} Operator;

struct AstNode;

struct AstNode {
    enum { AstNode_Operator, AstNode_Imm, AstNode_Variable } type;
    union {
        struct { Operator operator; optional struct AstNode* left; optional struct AstNode* right; } operator;
        ImmValue imm;
        char variable[256];
    } body;
};

typedef struct AstNode AstNode;

typedef struct {
    AstNode* node;
} AstTree;

void ImmValue_print(in ImmValue* self);

void ImmValue_free(ImmValue self);

void Operator_print(in Operator* self);

void AstNode_print(in AstNode* self);

void AstNode_free(AstNode self);

void AstTree_print(in AstTree* self);

void AstTree_free(AstTree self);

ParserMsg AstTree_parse(Parser parser, out AstTree* ptr);

