#pragma once

#include "types.h"
#include "gen.h"

typedef struct {
    bool success_flag;
    union { Data success; struct { u32 line; char msg[256]; } error; } body;
} SyntaxResult;

bool SyntaxResult_is_success(SyntaxResult result);

#define SYNTAXRESULT_UNWRAP(r, catch_proc) (\
    SyntaxResult result_copy = r;\
    if(!SyntaxResult_is_success(result_copy)) {\
        catch_proc;\
        return result_copy;\
    }\
)

