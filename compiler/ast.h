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
    enum { AstNode_Operator, AstNode_Imm } type;
    union {
        struct { Operator operator; struct AstNode* left; struct AstNode* right; } operator;
        ImmValue imm;
    } body;
};

typedef struct AstNode AstNode;

