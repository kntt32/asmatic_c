#pragma once

#include "types.h"
#include "gen.h"

typedef enum {
    SyntaxStatus_Success,
    SyntaxStatus_Error,
    SyntaxStatus_None,
} SyntaxStatus;

void Syntax_build(Parser parser, inout Generator* generator);

