#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "ast.h"
#include "parser.h"

static Operator OPERATORS[] = {
    {"+", true, true, 5},
    {"-", true, true, 5},
    {"*", true, true, 4},
    {"/", true, true, 4},
    {"%", true, true, 4},
    {"++", true, false, 2}
};

static AstNode** AstNode_get_leaf(in AstNode* self) {
    switch(self->type) {
        case AstNode_Operator:
            Operator* operator = &self->body.operator.operator;
            if(operator->left_arg && self->body.operator.left == NULL) {
                return &self->body.operator.left;
            }
            if(operator->right_arg && self->body.operator.right == NULL) {
                return &self->body.operator.right;
            }
            return AstNode_get_leaf(self->body.operator.right);
        default:
            break;
    }

    return NULL;
}

static ParserMsg AstNode_parse_number(inout AstNode* self, inout Parser* parser) {
    ImmValue imm_value;
    imm_value.type = ImmValue_Number;
    PARSERMSG_UNWRAP(
        Parser_parse_number_raw(parser, imm_value.body.number),
        (void)NULL
    );

    AstNode* ast_node = malloc(sizeof(AstNode));
    UNWRAP_NULL(ast_node);
    ast_node->type = AstNode_Imm;
    ast_node->body.imm = imm_value;

    AstNode** leaf_node = AstNode_get_leaf(self);
    if(leaf_node == NULL) {
        ParserMsg msg = {parser->line, "unexpected token"};
        return msg;
    }

    *leaf_node = ast_node;

    return SUCCESS_PARSER_MSG;
}

static ParserMsg AstNode_parse_operator(inout AstNode* self, inout Parser* parser) {
    TODO();
    return SUCCESS_PARSER_MSG;
}



