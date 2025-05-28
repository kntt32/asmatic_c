#pragma once

#include "types.h"
#include "parser.h"

typedef enum {
    Rsp,
    Rbp,
    Rax,
    Rcx,
    Rdx,
    Rbx,
    Rsi,
    Rdi,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15,
} Register;

ParserMsg Register_parse(Parser* parser, Register* ptr);

