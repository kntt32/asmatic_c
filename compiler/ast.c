#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "types.h"
#include "ast.h"
#include "parser.h"

static Operator OPERATORS[] = {
    {".", true, true, 16},
    {"->", true, true, 16},
    {"++", true, false, 16},
    {"--", true, false, 16},

    {"sizeof", false, true, 15},
    {"&", false, true, 15},
    {"*", false, true, 15},
    {"+", false, true, 15},
    {"-", false, true, 15},
    {"~", false, true, 15},
    {"!", false, true, 15},
    {"++", false, true, 15},
    {"--", false, true, 15},

    {"*", true, true, 13},
    {"/", true, true, 13},
    {"%", true, true, 13},
    
    {"+", true, true, 12},
    {"-", true, true, 12},

    {"<<", true, true, 11},
    {">>", true, true, 11},

    {"<", true, true, 10},
    {">", true, true, 10},
    {"<=", true, true, 10},
    {">=", true, true, 10},

    {"==", true, true, 9},
    {"!=", true, true, 9},

    {"&", true, true, 8},

    {"^", true, true, 7},
    
    {"|", true, true, 6},
    
    {"&&", true, true, 5},

    {"||", true, true, 4},

    {"?", true, true, 3},
    {":", true, true, 3},

    {"=", true, true, 2},
    {"*=", true, true, 2},
    {"/=", true, true, 2},
    {"%=", true, true, 2},
    {"+=", true, true, 2},
    {"-=", true, true, 2},
    {"<<=", true, true, 2},
    {">>=", true, true, 2},
    {"&=", true, true, 2},
    {"^=", true, true, 2},
    {"|=", true, true, 2},
};

void ImmValue_print(in ImmValue* self) {
    printf("ImmValue { type: %d, body: ", self->type);
    switch(self->type) {
        case ImmValue_Number:
            printf(".number: %s", self->body.number);
            break;
        case ImmValue_String:
            printf(".string: ");
            String_print(&self->body.string);
            break;
    }
    printf(" }");

    return;
}

void ImmValue_free(ImmValue self) {
    if(self.type == ImmValue_String) {
        String_free(self.body.string);
    }

    return;
}

void Operator_print(in Operator* self) {
    printf(
        "Operator { name: %s, left_arg: %s, right_arg: %s, priority: %u}",
        self->name,
        BOOL_TO_STR(self->left_arg),
        BOOL_TO_STR(self->right_arg),
        self->priority
    );

    return;
}

static optional AstNode** AstNode_get_leaf(in AstNode** self_ptr) {
    AstNode* self = *self_ptr;
    if(self == NULL) {
        return self_ptr;
    }

    switch(self->type) {
        case AstNode_Operator:
            {
                Operator* operator = &self->body.operator.operator;
                if(!operator->right_arg) {
                    return NULL;
                }
                return AstNode_get_leaf(&self->body.operator.right);
            }
        default:
            break;
    }

    return NULL;
}

static AstNode** AstNode_get_operand(in AstNode** self_ptr, u32 operator_priority) {
    AstNode* self = *self_ptr;
    if(self == NULL) {
        return self_ptr;
    }

    switch(self->type) {
        case AstNode_Operator:
            if(operator_priority <= self->body.operator.operator.priority) {
                return self_ptr;
            }
            return AstNode_get_operand(&self->body.operator.right, operator_priority);
        case AstNode_Imm:
        case AstNode_Variable:
        case AstNode_Function:
            return self_ptr;
    }

    if(self->type == AstNode_Imm) {
        return self_ptr;
    }

    if(operator_priority <= self->body.operator.operator.priority) {
        return self_ptr;
    }
    
    return AstNode_get_operand(&self->body.operator.right, operator_priority);
}

static void AstNode_print_ast_tree(in void* ptr) {
    AstTree_print(ptr);
    return;
}

void AstNode_print(in AstNode* self) {
    printf("AstNode { type: %d, body: ", self->type);

    switch(self->type) {
        case AstNode_Operator:
            printf(".operator: { operator: ");
            Operator_print(&self->body.operator.operator);
            printf(", left: ");
            if(self->body.operator.left == NULL) {
                printf("null");
            }else {
                AstNode_print(self->body.operator.left);
            }
            printf(", right: ");
            if(self->body.operator.right == NULL) {
                printf("null");
            }else {
                AstNode_print(self->body.operator.right);
            }
            printf(" }");
            break;
        case AstNode_Imm:
            printf(".imm: ");
            ImmValue_print(&self->body.imm);
            break;
        case AstNode_Variable:
            printf(".variable: %s", self->body.variable);
            break;
        case AstNode_Function:
            printf(".function: { name: %s, arguments: ", self->body.function.name);
            Vec_print(&self->body.function.arguments, AstNode_print_ast_tree);
            printf(" }");
            break;
    }

    printf(" }");

    return;
}

static void AstNode_free_ast_tree(inout void* ptr) {
    AstTree* tree = ptr;
    AstTree_free(*tree);
    return;
}

void AstNode_free(AstNode self) {
    switch(self.type) {
        case AstNode_Operator:
            if(self.body.operator.left != NULL) {
                AstNode_free(*self.body.operator.left);
                free(self.body.operator.left);
            }
            if(self.body.operator.right != NULL) {
                AstNode_free(*self.body.operator.right);
                free(self.body.operator.right);
            }
            break;
        case AstNode_Imm:
            ImmValue_free(self.body.imm);
            break;
        case AstNode_Variable:
            break;
        case AstNode_Function:
            Vec_free_all(self.body.function.arguments, AstNode_free_ast_tree);
            break;
    }
    return;
}

void AstTree_print(in AstTree* self) {
    printf("AstTree { node: ");
    if(self->node == NULL) {
        printf("null");
    }else {
        AstNode_print(self->node);
    }

    printf(" }");
}

void AstTree_free(AstTree self) {
    if(self.node != NULL) {
        AstNode_free(*self.node);
        free(self.node);
    }

    return;
}

static ParserMsg AstTree_parse_number(in AstTree* self, inout Parser* parser) {
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

    AstNode** leaf_node = AstNode_get_leaf(&self->node);
    if(leaf_node == NULL) {
        ParserMsg msg = {parser->line, "unexpected token"};
        return msg;
    }

    *leaf_node = ast_node;

    return SUCCESS_PARSER_MSG;
}

static ParserMsg AstTree_parse_operator(inout AstTree* self, inout Parser* parser) {
    AstNode* node = malloc(sizeof(AstNode));

    for(u32 i=0; i<LEN(OPERATORS); i++) {
        Operator* operator = &OPERATORS[i];
        Parser parser_copy = *parser;

        if(ispunct(operator->name[0])) {
            if(!ParserMsg_is_success(Parser_parse_symbol(&parser_copy, operator->name))) {
                continue;
            }
        }else {
            if(!ParserMsg_is_success(Parser_parse_keyword(&parser_copy, operator->name))) {
                continue;
            }
        }
        AstNode** operand = AstNode_get_operand(&self->node, operator->priority);

        node->type = AstNode_Operator;
        node->body.operator.operator = *operator;
        node->body.operator.left = NULL;
        node->body.operator.right = NULL;

        if(operator->left_arg && *operand != NULL) {
            node->body.operator.left = *operand;
        }else if(!(!operator->left_arg && *operand == NULL)) {
            continue;
        }

        *operand = node;
        *parser = parser_copy;

        return SUCCESS_PARSER_MSG;
    }

    free(node);
    ParserMsg msg = {parser->line, "unknown operator"};

    return msg;
}

ParserMsg AstTree_parse_variable(inout AstTree* self, inout Parser* parser) {
    char var_name[256];
    PARSERMSG_UNWRAP(
        Parser_parse_ident(parser, var_name),
        (void)NULL
    );

    AstNode* ast_node = malloc(sizeof(AstNode));
    UNWRAP_NULL(ast_node);
    ast_node->type = AstNode_Variable;
    strcpy(ast_node->body.variable, var_name);

    AstNode** leaf_node = AstNode_get_leaf(&self->node);
    if(leaf_node == NULL) {
        ParserMsg msg = {parser->line, "unexpected token"};
        return msg;
    }

    *leaf_node = ast_node;

    return SUCCESS_PARSER_MSG;
}

static void AstTree_parse_function_free_ast_tree(inout void* ptr) {
    AstTree* tree = ptr;
    AstTree_free(*tree);
    return;
}

static ParserMsg AstTree_parse_function(inout AstTree* self, inout Parser* parser) {
    Parser parser_copy = *parser;

    char function_name[256] = "";
    Parser_parse_ident(&parser_copy, function_name);

    Parser block_parser;
    PARSERMSG_UNWRAP(
        Parser_parse_paren(&parser_copy, &block_parser),
        (void)NULL
    );

    Vec arguments = Vec_new(sizeof(AstTree));

    while(!Parser_is_empty(&block_parser)) {
        Parser argument_parser;
        PARSERMSG_UNWRAP(
            Parser_split(&block_parser, ",", &argument_parser),
            Vec_free_all(arguments, AstTree_parse_function_free_ast_tree)
        );

        AstTree tree;
        PARSERMSG_UNWRAP(
            AstTree_parse(argument_parser, &tree),
            Vec_free_all(arguments, AstTree_parse_function_free_ast_tree)
        );

        Vec_push(&arguments, &tree);
    }

    AstNode** leaf_node = AstNode_get_leaf(&self->node);
    if(leaf_node == NULL) {
        ParserMsg msg = {parser_copy.line, "unexpected token"};
        return msg;
    }

    AstNode* node = malloc(sizeof(AstNode));
    node->type = AstNode_Function;
    strcpy(node->body.function.name, function_name);
    node->body.function.arguments = arguments;

    *leaf_node = node;

    *parser = parser_copy;

    return SUCCESS_PARSER_MSG;
}

ParserMsg AstTree_parse(Parser parser, out AstTree* ptr) {
    ptr->node = NULL;

    while(!Parser_is_empty(&parser)) {
        if(!ParserMsg_is_success(AstTree_parse_number(ptr, &parser))
            && !ParserMsg_is_success(AstTree_parse_operator(ptr, &parser))
            && !ParserMsg_is_success(AstTree_parse_function(ptr, &parser))
            && !ParserMsg_is_success(AstTree_parse_variable(ptr, &parser))) {
            ParserMsg msg = {parser.line, "invalid expression"};
            return msg;
        }
    }

    return SUCCESS_PARSER_MSG;
}


