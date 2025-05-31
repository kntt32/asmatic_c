#pragma once

#include "types.h"
#include "gen.h"

typedef struct {
    u32 line;
    char msg[256];
    Data data;
} SyntaxResult;

