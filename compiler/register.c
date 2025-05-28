#include "types.h"
#include "register.h"

static struct { Register reg; char* str; } REGISTER_TABLE[] = {
    {Rsp, "rsp"},
    {Rbp, "rbp"},
    {Rax, "rax"},
    {Rcx, "rcx"},
    {Rdx, "rdx"},
    {Rbx, "rbx"},
    {Rsi, "rsi"},
    {Rdi, "rdi"},
    {R8, "r8"},
    {R9, "r9"},
    {R10, "r10"},
    {R11, "r11"},
    {R12, "r12"},
    {R13, "r13"},
    {R14, "r14"},
    {R15, "r15"},
};

ParserMsg Register_parse(Parser* parser, Register* restrict ptr) {
    for(u32 i=0; i<LEN(REGISTER_TABLE); i++) {
        char* rtable_str = REGISTER_TABLE[i].str;
        Register rtable_reg = REGISTER_TABLE[i].reg;

        if(Parser_parse_keyword(parser, rtable_str).msg[0] == '\0') {
            *ptr = rtable_reg;
            return SUCCESS_PARSER_MSG;
        }
    }

    ParserMsg msg = {parser->line, "expected keyword"};
    return msg;
}

