#pragma once

#include "types.h"

typedef struct {
    char* src;
    u32 line;
    u64 len;
} Parser;

typedef struct {
    u32 line;
    char msg[256];
} ParserMsg;

extern ParserMsg SUCCESS_PARSER_MSG;

Parser Parser_new(in char* src);

bool Parser_is_empty(in Parser* self);

ParserMsg Parser_parse_ident(inout Parser* self, out char token[256]);

ParserMsg Parser_parse_keyword(inout Parser* self, in char* keyword);

ParserMsg Parser_parse_keyword(inout Parser* self, in char* keyword);

ParserMsg Parser_parse_symbol(inout Parser* self, in char* symbol);

ParserMsg Parser_parse_number(inout Parser* self, out i64* value);

ParserMsg Parser_parse_block(inout Parser* self, out Parser* parser);

ParserMsg Parser_parse_paren(inout Parser* self, out Parser* parser);

ParserMsg Parser_parse_index(inout Parser* self, out Parser* parser);

