#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "parser.h"
#include "util.h"

ParserMsg SUCCESS_PARSER_MSG = {0, ""};

Parser Parser_new(in char* src) {
    Parser parser = {src, 1, strlen(src)};
    return parser;
}

bool Parser_is_empty(in Parser* self) {
    return 0 == self->len;
}

static char Parser_read(inout Parser* self) {
    if(Parser_is_empty(self)) {
        return '\0';
    }

    char c = self->src[0];

    if(c == '\n') {
        self->line ++;
    }

    self->src ++;
    self->len --;

    return c;
}

static void Parser_skip_space(inout Parser* self) {
    while(isspace(self->src[0]) && !Parser_is_empty(self)) {
        Parser_read(self);
    }
}

static bool Parser_is_gap(in Parser* self) {
    char c = self->src[0];
    return !Parser_is_empty(self) && (isspace(c) || ispunct(c));
}

static void Parser_run_for_gap(inout Parser* self, out char token[256]) {
    Parser_skip_space(self);
    u32 len = 0;
    
    while(isascii(self->src[0]) && !Parser_is_gap(self) && len < 256) {
        token[len] = Parser_read(self);
        len ++;
    }

    token[len] = '\0';

    return;
}

static bool Parser_skip(inout Parser* self) {
    Parser_skip_space(self);

    char token[256];
    Parser_run_for_gap(self, token);
    if(token[0] != '\0') {
        return true;
    }

    static ParserMsg (*BLOCK_PARSERS[3])(Parser*, Parser*) = {Parser_parse_block, Parser_parse_paren, Parser_parse_index};
    for(u32 i=0; i<3; i++) {
        Parser parser;
        if(BLOCK_PARSERS[i](self, &parser).msg[0] == '\0') {
            return true;
        }
    }

    if(ispunct(self->src[0])) {
        Parser_read(self);
        return true;
    }

    return false;
}

static ParserMsg Parser_parse_block_helper(inout Parser* self, out Parser* parser, in char* start, in char* end) {
    Parser self_copy = *self;

    if(Parser_parse_symbol(&self_copy, start).msg[0] != '\0') {
        ParserMsg msg = {self_copy.line, ""};
        sprintf(msg.msg, "expected symbol \"%s\"", start);
        return msg;
    }

    *parser = self_copy;

    while(Parser_parse_symbol(&self_copy, end).msg[0] != '\0') {
        if(!Parser_skip(&self_copy)) {
            ParserMsg msg = {self_copy.line, ""};
            sprintf(msg.msg, "expected symbol \"%s\"", end);
            return msg;
        }
    }

    parser->len = parser->len - self_copy.len - 1;
    *self = self_copy;

    return SUCCESS_PARSER_MSG;
}

ParserMsg Parser_parse_ident(inout Parser* self, out char token[256]) {
    Parser self_copy = *self;
    Parser_run_for_gap(&self_copy, token);
    u32 len = strlen(token);

    ParserMsg msg = {self_copy.line, "expected identifier"};
    
    if(len == 0) {
        return msg;
    }

    for(u32 i=0; i<len; i++) {
        if(!(isascii(token[i]) && (isalpha(token[i]) || (i != 0 && isalnum(token[i]))))) {
            return msg;
        }
    }

    *self = self_copy;

    return SUCCESS_PARSER_MSG;
}

ParserMsg Parser_parse_keyword(inout Parser* self, in char* keyword) {
    Parser self_copy = *self;
    char token[256];
    Parser_run_for_gap(&self_copy, token);
    if(strcmp(token, keyword) != 0) {
        ParserMsg msg = {self_copy.line, "expected keyword"};
        return msg;
    }

    *self = self_copy;

    return SUCCESS_PARSER_MSG;
}

ParserMsg Parser_parse_symbol(inout Parser* self, in char* symbol) {
    Parser_skip_space(self);

    u32 symbol_len = strlen(symbol);
    if(strncmp(self->src, symbol, symbol_len) != 0) {
        ParserMsg msg = {self->line, "expected symbol"};
        return msg;
    }

    for(u32 i=0; i<symbol_len; i++) {
        Parser_read(self);
    }

    return SUCCESS_PARSER_MSG;
}

ParserMsg Parser_parse_number(inout Parser* self, out i64* value) {
    Parser self_copy = *self;
    
    char token[256];
    
    Parser_run_for_gap(&self_copy, token);
    
    if(Util_str_to_i64(token, value) == NULL) {
        ParserMsg msg = {self_copy.line, "expected number literal"};
        return msg;
    }

    *self = self_copy;
    
    return SUCCESS_PARSER_MSG;
}

ParserMsg Parser_parse_block(inout Parser* self, out Parser* parser) {
    return Parser_parse_block_helper(self, parser, "{", "}");
}

ParserMsg Parser_parse_paren(inout Parser* self, out Parser* parser) {
    return Parser_parse_block_helper(self, parser, "(", ")");
}

ParserMsg Parser_parse_index(inout Parser* self, out Parser* parser) {
    return Parser_parse_block_helper(self, parser, "[", "]");
}








