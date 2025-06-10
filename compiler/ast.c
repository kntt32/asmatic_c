#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "types.h"
#include "ast.h"
#include "util.h"
#include "parser.h"

#define OPERATOR_PRIORITY_MAX (20)
#define OPERATOR_ADD {"+", true, true, 12}

static SResult AstNode_eval_operator_sizeof(in AstNode* self, out ImmValue* imm_value);
static SResult AstNode_eval_operator_and(in AstNode* self, out ImmValue* imm_value);
static SResult AstNode_eval_operator_or(in AstNode* self, out ImmValue* imm_value);
static SResult AstNode_eval_operator_xor(in AstNode* self, out ImmValue* imm_value);
static SResult AstNode_eval_operator_booland(in AstNode* self, out ImmValue* imm_value);
static SResult AstNode_eval_operator_boolor(in AstNode* self, out ImmValue* imm_value);
static SResult AstNode_eval_operator_equal(in AstNode* self, out ImmValue* imm_value);
static SResult AstNode_eval_operator_notequal(in AstNode* self, out ImmValue* imm_value);
static SResult AstNode_eval_operator_lessthan(in AstNode* self, out ImmValue* imm_value);
static SResult AstNode_eval_operator_greaterthan(in AstNode* self, out ImmValue* imm_value);
static SResult AstNode_eval_operator_eqgreaterthan(in AstNode* self, out ImmValue* imm_value);
static SResult AstNode_eval_operator_eqlessthan(in AstNode* self, out ImmValue* imm_value);
static SResult AstNode_eval_operator_leftbitshift(in AstNode* self, out ImmValue* imm_value);
static SResult AstNode_eval_operator_rightbitshift(in AstNode* self, out ImmValue* imm_value);
static SResult AstNode_eval_operator_add(in AstNode* self, out ImmValue* imm_value);
static SResult AstNode_eval_operator_sub(in AstNode* self, out ImmValue* imm_value);
static SResult AstNode_eval_operator_mult(in AstNode* self, out ImmValue* imm_value);
static SResult AstNode_eval_operator_div(in AstNode* self, out ImmValue* imm_value);
static SResult AstNode_eval_operator_mod(in AstNode* self, out ImmValue* imm_value);
static SResult AstNode_eval_operator_invert(in AstNode* self, out ImmValue* imm_value);
static SResult AstNode_eval_operator_boolinvert(in AstNode* self, out ImmValue* imm_value);
static SResult AstNode_eval_operator_plus(in AstNode* self, out ImmValue* imm_value);
static SResult AstNode_eval_operator_minus(in AstNode* self, out ImmValue* imm_value);
static SResult AstNode_eval_operator_hatenacolon(in AstNode* self, out ImmValue* imm_value);

static Operator OPERATORS[] = {
    {"sizeof", false, true, 15, AstNode_eval_operator_sizeof},

    {"<<=", true, true, 2, NULL},
    {">>=", true, true, 2, NULL},

    {"->", true, true, 16, NULL},
    {"++", true, false, 16, NULL},
    {"--", true, false, 16, NULL},

    {"++", false, true, 15, NULL},
    {"--", false, true, 15, NULL},

    {"<<", true, true, 11, AstNode_eval_operator_leftbitshift},
    {">>", true, true, 11, AstNode_eval_operator_rightbitshift},
    
    {"<=", true, true, 10, AstNode_eval_operator_eqgreaterthan},
    {">=", true, true, 10, AstNode_eval_operator_eqlessthan},

    {"==", true, true, 9, AstNode_eval_operator_equal},
    {"!=", true, true, 9, AstNode_eval_operator_notequal},

    {"&&", true, true, 5, AstNode_eval_operator_booland},
    {"||", true, true, 4, AstNode_eval_operator_boolor},

    {"*=", true, true, 2, NULL},
    {"/=", true, true, 2, NULL},
    {"%=", true, true, 2, NULL},
    {"+=", true, true, 2, NULL},
    {"-=", true, true, 2, NULL},
    {"&=", true, true, 2, NULL},
    {"^=", true, true, 2, NULL},
    {"|=", true, true, 2, NULL},

    {".", true, true, 16, NULL},
    {"&", false, true, 15, NULL},
    {"*", false, true, 15, NULL},
    {"+", false, true, 15, AstNode_eval_operator_plus},
    {"-", false, true, 15, AstNode_eval_operator_minus},
    {"~", false, true, 15, AstNode_eval_operator_invert},
    {"!", false, true, 15, AstNode_eval_operator_boolinvert},

    {"*", true, true, 13, AstNode_eval_operator_mult},
    {"/", true, true, 13, AstNode_eval_operator_div},
    {"%", true, true, 13, AstNode_eval_operator_mod},
    
    {"+", true, true, 12, AstNode_eval_operator_add},
    {"-", true, true, 12, AstNode_eval_operator_sub},

    {"<", true, true, 10, AstNode_eval_operator_greaterthan},
    {">", true, true, 10, AstNode_eval_operator_lessthan},

    {"&", true, true, 8, AstNode_eval_operator_and},

    {"^", true, true, 7, AstNode_eval_operator_xor},
    
    {"|", true, true, 6, AstNode_eval_operator_or},
    
    {"?", true, true, 3, NULL},
    {":", true, true, 3, AstNode_eval_operator_hatenacolon},

    {"=", true, true, 2, NULL},
};

SResult ImmValue_as_integer(in ImmValue* self, out u64* value) {
    static SResult EXPECTED_INTEGER = {false, "expected integer"};

    if(self->type != ImmValue_Integral) {
        return EXPECTED_INTEGER;
    }

    *value = self->body.integral;

    return SRESULT_OK;
}

SResult ImmValue_as_floating(in ImmValue* self, out f64* value) {
    static SResult EXPECTED_FLOATING = {false, "expected floating point number"};

    switch(self->type) {
        case ImmValue_Integral:
            *value = (f64)self->body.integral;
            break;
        case ImmValue_Floating:
            *value = self->body.floating;
            break;
        default:
            return EXPECTED_FLOATING;
    }

    return SRESULT_OK;
}

void ImmValue_print(in ImmValue* self) {
    printf("ImmValue { type: %d, body: ", self->type);
    switch(self->type) {
        case ImmValue_Integral:
            printf(".integral: %llu", self->body.integral);
            break;
        case ImmValue_Floating:
            printf(".floating: %f", self->body.floating);
            break;
    }
    printf(" }");
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

bool Operator_cmp(in Operator* self, in Operator* other) {
    return strcmp(self->name, other->name) == 0
        && self->left_arg == other->left_arg
        && self->right_arg == other->right_arg
        && self->priority == other->priority;
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
                AstNode* right = self->body.operator.right;
                if(right != NULL
                    && right->type == AstNode_Operator
                    && right->body.operator.operator.priority < operator->priority) {
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
            AstNode* right = self->body.operator.right;
            if(right != NULL
                && right->type == AstNode_Operator
                && right->body.operator.operator.priority < self->body.operator.operator.priority) {
                return self_ptr;
            }
            return AstNode_get_operand(&self->body.operator.right, operator_priority);
        case AstNode_Number:
        case AstNode_Variable:
        case AstNode_Function:
        case AstNode_Type:
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
        case AstNode_Number:
            printf(".number: %s", self->body.number);
            break;
        case AstNode_Variable:
            printf(".variable: %s", self->body.variable);
            break;
        case AstNode_Function:
            printf(".function: { function_ptr: ");
            AstNode_print(self->body.function.function_ptr);
            printf(", arguments: ");
            Vec_print(&self->body.function.arguments, AstNode_print_ast_tree);
            printf(" }");
            break;
        case AstNode_Type:
            printf(".type: ");
            Type_print(&self->body.type);
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
        case AstNode_Number:
            break;
        case AstNode_Variable:
            break;
        case AstNode_Function:
            Vec_free_all(self.body.function.arguments, AstNode_free_ast_tree);
            break;
        case AstNode_Type:
            break;
    }
    return;
}

static SResult AstNode_eval_operator_integers_helper(in AstNode* self, out u64* left, out u64* right) {
    static SResult EXPECTED_ARGUMENT = {false, "expected argument"};
    if(self->body.operator.left == NULL || self->body.operator.right == NULL) {
        return EXPECTED_ARGUMENT;
    }

    ImmValue left_imm;
    ImmValue right_imm;
    
    SRESULT_UNWRAP(
        AstNode_eval(self->body.operator.left, &left_imm),
        (void)NULL
    );
    SRESULT_UNWRAP(
        AstNode_eval(self->body.operator.right, &right_imm),
        (void)NULL
    );

    SRESULT_UNWRAP(
        ImmValue_as_integer(&left_imm, left),
        (void)NULL
    );
    SRESULT_UNWRAP(
        ImmValue_as_integer(&right_imm, right),
        (void)NULL
    );

    return SRESULT_OK;
}

static SResult AstNode_eval_operator_floatings_helper(in AstNode* self, out f64* left, out f64* right) {
    ImmValue left_imm;
    ImmValue right_imm;
    
    SRESULT_UNWRAP(
        AstNode_eval(self->body.operator.left, &left_imm),
        (void)NULL
    );
    SRESULT_UNWRAP(
        AstNode_eval(self->body.operator.right, &right_imm),
        (void)NULL
    );

    SRESULT_UNWRAP(
        ImmValue_as_floating(&left_imm, left),
        (void)NULL
    );
    SRESULT_UNWRAP(
        ImmValue_as_floating(&right_imm, right),
        (void)NULL
    );

    return SRESULT_OK;
}

static SResult AstNode_eval_operator_paren(in AstNode* self, out ImmValue* imm_value) {
    if(self->body.operator.left != NULL || self->body.operator.right == NULL) {
        SResult result = {false, "invalid arguments of () operator"};
        return result;
    }

    SRESULT_UNWRAP(
        AstNode_eval(self->body.operator.right, imm_value),
        (void)NULL
    );

    return SRESULT_OK;
}

static SResult AstNode_eval_operator_sizeof(in AstNode* self, out ImmValue* imm_value) {
    if(self->body.operator.left != NULL || self->body.operator.right == NULL) {
        SResult result = {false, "invalid arguments of sizeof operator"};
        return result;
    }

    ImmValue arg_imm_value;
    SRESULT_UNWRAP(
        AstNode_eval(self->body.operator.right, &arg_imm_value),
        (void)NULL
    );

    imm_value->type = ImmValue_Integral;
    switch(arg_imm_value.type) {
        case ImmValue_Integral:
            imm_value->body.integral = 8;
            break;
        case ImmValue_Floating:
            imm_value->body.floating = 8;
            break;
    }

    return SRESULT_OK;
}

static SResult AstNode_eval_operator_and(in AstNode* self, out ImmValue* imm_value) {
    u64 left_int;
    u64 right_int;
    
    SRESULT_UNWRAP(
        AstNode_eval_operator_integers_helper(self, &left_int, &right_int),
        (void)NULL
    );
    imm_value->type = ImmValue_Integral;
    imm_value->body.integral = left_int & right_int;

    return SRESULT_OK;
}

static SResult AstNode_eval_operator_or(in AstNode* self, out ImmValue* imm_value) {
    u64 left_int;
    u64 right_int;
    
    SRESULT_UNWRAP(
        AstNode_eval_operator_integers_helper(self, &left_int, &right_int),
        (void)NULL
    );
    imm_value->type = ImmValue_Integral;
    imm_value->body.integral = left_int | right_int;

    return SRESULT_OK;
}

static SResult AstNode_eval_operator_xor(in AstNode* self, out ImmValue* imm_value) {
    u64 left_int;
    u64 right_int;
    
    SRESULT_UNWRAP(
        AstNode_eval_operator_integers_helper(self, &left_int, &right_int),
        (void)NULL
    );
    imm_value->type = ImmValue_Integral;
    imm_value->body.integral = left_int ^ right_int;

    return SRESULT_OK;
}

static SResult AstNode_eval_operator_booland(in AstNode* self, out ImmValue* imm_value) {
    u64 left_int;
    u64 right_int;
    
    SRESULT_UNWRAP(
        AstNode_eval_operator_integers_helper(self, &left_int, &right_int),
        (void)NULL
    );
    imm_value->type = ImmValue_Integral;
    imm_value->body.integral = left_int && right_int;

    return SRESULT_OK;
}

static SResult AstNode_eval_operator_boolor(in AstNode* self, out ImmValue* imm_value) {
    u64 left_int;
    u64 right_int;
    
    SRESULT_UNWRAP(
        AstNode_eval_operator_integers_helper(self, &left_int, &right_int),
        (void)NULL
    );
    imm_value->type = ImmValue_Integral;
    imm_value->body.integral = left_int || right_int;

    return SRESULT_OK;
}

static SResult AstNode_eval_operator_equal(in AstNode* self, out ImmValue* imm_value) {
    u64 left_int;
    u64 right_int;
    f64 left_float;
    f64 right_float;
    if(SRESULT_IS_OK(AstNode_eval_operator_integers_helper(self, &left_int, &right_int))) {
        imm_value->type = ImmValue_Integral;
        imm_value->body.integral = (u64)((i64)left_int == (i64)right_int);
    }else {
        SRESULT_UNWRAP(AstNode_eval_operator_floatings_helper(self, &left_float, &right_float), (void)NULL);
        imm_value->type = ImmValue_Integral;
        imm_value->body.integral = left_float == right_float;
    }

    return SRESULT_OK;
}

static SResult AstNode_eval_operator_notequal(in AstNode* self, out ImmValue* imm_value) {
    u64 left_int;
    u64 right_int;
    f64 left_float;
    f64 right_float;
    if(SRESULT_IS_OK(AstNode_eval_operator_integers_helper(self, &left_int, &right_int))) {
        imm_value->type = ImmValue_Integral;
        imm_value->body.integral = (u64)((i64)left_int != (i64)right_int);
    }else {
        SRESULT_UNWRAP(AstNode_eval_operator_floatings_helper(self, &left_float, &right_float), (void)NULL);
        imm_value->type = ImmValue_Integral;
        imm_value->body.integral = left_float != right_float;
    }

    return SRESULT_OK;
}

static SResult AstNode_eval_operator_lessthan(in AstNode* self, out ImmValue* imm_value) {
    u64 left_int;
    u64 right_int;
    f64 left_float;
    f64 right_float;
    if(SRESULT_IS_OK(AstNode_eval_operator_integers_helper(self, &left_int, &right_int))) {
        imm_value->type = ImmValue_Integral;
        imm_value->body.integral = (u64)((i64)left_int > (i64)right_int);
    }else {
        SRESULT_UNWRAP(AstNode_eval_operator_floatings_helper(self, &left_float, &right_float), (void)NULL);
        imm_value->type = ImmValue_Integral;
        imm_value->body.integral = left_float > right_float;
    }

    return SRESULT_OK;
}

static SResult AstNode_eval_operator_greaterthan(in AstNode* self, out ImmValue* imm_value) {
    u64 left_int;
    u64 right_int;
    f64 left_float;
    f64 right_float;
    if(SRESULT_IS_OK(AstNode_eval_operator_integers_helper(self, &left_int, &right_int))) {
        imm_value->type = ImmValue_Integral;
        imm_value->body.integral = (u64)((i64)left_int < (i64)right_int);
    }else {
        SRESULT_UNWRAP(AstNode_eval_operator_floatings_helper(self, &left_float, &right_float), (void)NULL);
        imm_value->type = ImmValue_Integral;
        imm_value->body.integral = left_float < right_float;
    }

    return SRESULT_OK;
   
}

static SResult AstNode_eval_operator_eqgreaterthan(in AstNode* self, out ImmValue* imm_value) {
    u64 left_int;
    u64 right_int;
    f64 left_float;
    f64 right_float;
    if(SRESULT_IS_OK(AstNode_eval_operator_integers_helper(self, &left_int, &right_int))) {
        imm_value->type = ImmValue_Integral;
        imm_value->body.integral = (u64)((i64)left_int <= (i64)right_int);
    }else {
        SRESULT_UNWRAP(AstNode_eval_operator_floatings_helper(self, &left_float, &right_float), (void)NULL);
        imm_value->type = ImmValue_Integral;
        imm_value->body.integral = left_float <= right_float;
    }

    return SRESULT_OK;
   
}

static SResult AstNode_eval_operator_eqlessthan(in AstNode* self, out ImmValue* imm_value) {
    u64 left_int;
    u64 right_int;
    f64 left_float;
    f64 right_float;
    if(SRESULT_IS_OK(AstNode_eval_operator_integers_helper(self, &left_int, &right_int))) {
        imm_value->type = ImmValue_Integral;
        imm_value->body.integral = (u64)((i64)left_int >= (i64)right_int);
    }else {
        SRESULT_UNWRAP(AstNode_eval_operator_floatings_helper(self, &left_float, &right_float), (void)NULL);
        imm_value->type = ImmValue_Integral;
        imm_value->body.integral = left_float >= right_float;
    }

    return SRESULT_OK;
   
}

static SResult AstNode_eval_operator_leftbitshift(in AstNode* self, out ImmValue* imm_value) {
    u64 left_int;
    u64 right_int;
    
    SRESULT_UNWRAP(
        AstNode_eval_operator_integers_helper(self, &left_int, &right_int),
        (void)NULL
    );
    imm_value->type = ImmValue_Integral;
    imm_value->body.integral = left_int << right_int;

    return SRESULT_OK;
}

static SResult AstNode_eval_operator_rightbitshift(in AstNode* self, out ImmValue* imm_value) {
    u64 left_int;
    u64 right_int;
    
    SRESULT_UNWRAP(
        AstNode_eval_operator_integers_helper(self, &left_int, &right_int),
        (void)NULL
    );
    imm_value->type = ImmValue_Integral;
    imm_value->body.integral = left_int >> right_int;

    return SRESULT_OK;
}

static SResult AstNode_eval_operator_add(in AstNode* self, out ImmValue* imm_value) {
    u64 left_int;
    u64 right_int;
    f64 left_float;
    f64 right_float;
    if(SRESULT_IS_OK(AstNode_eval_operator_integers_helper(self, &left_int, &right_int))) {
        imm_value->type = ImmValue_Integral;
        imm_value->body.integral = (u64)((i64)left_int + (i64)right_int);
    }else {
        SRESULT_UNWRAP(AstNode_eval_operator_floatings_helper(self, &left_float, &right_float), (void)NULL);
        imm_value->type = ImmValue_Floating;
        imm_value->body.floating = left_float + right_float;
    }

    return SRESULT_OK;
}

static SResult AstNode_eval_operator_sub(in AstNode* self, out ImmValue* imm_value) {
    u64 left_int;
    u64 right_int;
    f64 left_float;
    f64 right_float;
    if(SRESULT_IS_OK(AstNode_eval_operator_integers_helper(self, &left_int, &right_int))) {
        imm_value->type = ImmValue_Integral;
        imm_value->body.integral = (u64)((i64)left_int - (i64)right_int);
    }else {
        SRESULT_UNWRAP(AstNode_eval_operator_floatings_helper(self, &left_float, &right_float), (void)NULL);
        imm_value->type = ImmValue_Floating;
        imm_value->body.floating = left_float - right_float;
    }

    return SRESULT_OK;
}

static SResult AstNode_eval_operator_mult(in AstNode* self, out ImmValue* imm_value) {
    u64 left_int;
    u64 right_int;
    f64 left_float;
    f64 right_float;
    if(SRESULT_IS_OK(AstNode_eval_operator_integers_helper(self, &left_int, &right_int))) {
        imm_value->type = ImmValue_Integral;
        imm_value->body.integral = (u64)((i64)left_int * (i64)right_int);
    }else {
        SRESULT_UNWRAP(AstNode_eval_operator_floatings_helper(self, &left_float, &right_float), (void)NULL);
        imm_value->type = ImmValue_Floating;
        imm_value->body.floating = left_float * right_float;
    }

    return SRESULT_OK;
}

static SResult AstNode_eval_operator_div(in AstNode* self, out ImmValue* imm_value) {
    u64 left_int;
    u64 right_int;
    f64 left_float;
    f64 right_float;
    if(SRESULT_IS_OK(AstNode_eval_operator_integers_helper(self, &left_int, &right_int))) {
        imm_value->type = ImmValue_Integral;
        imm_value->body.integral = (u64)((i64)left_int / (i64)right_int);
    }else {
        SRESULT_UNWRAP(AstNode_eval_operator_floatings_helper(self, &left_float, &right_float), (void)NULL);
        imm_value->type = ImmValue_Floating;
        imm_value->body.floating = left_float / right_float;
    }

    return SRESULT_OK;
}

static SResult AstNode_eval_operator_mod(in AstNode* self, out ImmValue* imm_value) {
    u64 left_int;
    u64 right_int;
    SRESULT_UNWRAP(
        AstNode_eval_operator_integers_helper(self, &left_int, &right_int),
        (void)NULL
    );
    imm_value->type = ImmValue_Integral;
    imm_value->body.integral = (u64)((i64)left_int % (i64)right_int);

    return SRESULT_OK;
}

static SResult AstNode_eval_operator_aninteger_helper(in AstNode* self, out u64* value) {
    ImmValue imm_value;
    if(self->body.operator.operator.left_arg) {
        SRESULT_UNWRAP(
            AstNode_eval(self->body.operator.left, &imm_value),
            (void)NULL
        );
    }else {
        SRESULT_UNWRAP(
            AstNode_eval(self->body.operator.right, &imm_value),
            (void)NULL
        );
    }

    if(imm_value.type != ImmValue_Integral) {
        SResult result = {false, "expected integer"};
        return result;
    }

    *value = imm_value.body.integral;

    return SRESULT_OK;
}

static SResult AstNode_eval_operator_afloating_helper(in AstNode* self, out f64* value) {
    ImmValue imm_value;
    if(self->body.operator.operator.left_arg) {
        SRESULT_UNWRAP(
            AstNode_eval(self->body.operator.left, &imm_value),
            (void)NULL
        );
    }else {
        SRESULT_UNWRAP(
            AstNode_eval(self->body.operator.right, &imm_value),
            (void)NULL
        );
    }

    switch(imm_value.type) {
        case ImmValue_Integral:
            *value = (f64)imm_value.body.integral;
            break;
        case ImmValue_Floating:
            *value = (f64)imm_value.body.floating;
            break;
    }

    return SRESULT_OK;
}

static SResult AstNode_eval_operator_invert(in AstNode* self, out ImmValue* imm_value) {
    u64 value;
    SRESULT_UNWRAP(
        AstNode_eval_operator_aninteger_helper(self, &value),
        (void)NULL
    );

    imm_value->type = ImmValue_Integral;
    imm_value->body.integral = ~value;

    return SRESULT_OK;
}

static SResult AstNode_eval_operator_boolinvert(in AstNode* self, out ImmValue* imm_value) {
    u64 value;
    SRESULT_UNWRAP(
        AstNode_eval_operator_aninteger_helper(self, &value),
        (void)NULL
    );

    imm_value->type = ImmValue_Integral;
    imm_value->body.integral = !value;

    return SRESULT_OK;
}

static SResult AstNode_eval_operator_plus(in AstNode* self, out ImmValue* imm_value) {
    if(SRESULT_IS_OK(AstNode_eval_operator_aninteger_helper(self, &imm_value->body.integral))) {
        imm_value->type = ImmValue_Integral;
    }else if(SRESULT_IS_OK(AstNode_eval_operator_afloating_helper(self, &imm_value->body.floating))) {
        imm_value->type = ImmValue_Floating;
    }else {
        SResult result = {false, "expected number"};
        return result;
    }

    return SRESULT_OK;
}

static SResult AstNode_eval_operator_minus(in AstNode* self, out ImmValue* imm_value) {
    if(SRESULT_IS_OK(AstNode_eval_operator_aninteger_helper(self, &imm_value->body.integral))) {
        imm_value->body.integral = - imm_value->body.integral;
        imm_value->type = ImmValue_Integral;
    }else if(SRESULT_IS_OK(AstNode_eval_operator_afloating_helper(self, &imm_value->body.floating))) {
        imm_value->body.floating = - imm_value->body.floating;
        imm_value->type = ImmValue_Floating;
    }else {
        SResult result = {false, "expected number"};
        return result;
    }

    return SRESULT_OK;
}

static SResult AstNode_eval_operator_hatenacolon(in AstNode* self, out ImmValue* imm_value) {
    static SResult result = {false, "expected argument"};
    if(self->body.operator.left == NULL || self->body.operator.right == NULL) {
        return result;
    }
    AstNode* hatena_node = self->body.operator.left;
    if(hatena_node->type != AstNode_Operator || strcmp(hatena_node->body.operator.operator.name, "?") != 0) {
        return result;
    }
    if(hatena_node->body.operator.left == NULL || hatena_node->body.operator.right == NULL) {
        return result;
    }

    ImmValue arg_1;
    SRESULT_UNWRAP(
        AstNode_eval(hatena_node->body.operator.left, &arg_1),
        (void)NULL
    );
    u64 arg_1_u64;
    SRESULT_UNWRAP(
        ImmValue_as_integer(&arg_1, &arg_1_u64),
        (void)NULL
    );

    if(arg_1_u64) {
        SRESULT_UNWRAP(
            AstNode_eval(hatena_node->body.operator.right, imm_value),
            (void)NULL
        );
    }else {
        SRESULT_UNWRAP(
            AstNode_eval(self->body.operator.right, imm_value),
            (void)NULL
        );
    }

    return SRESULT_OK;
}

static SResult AstNode_eval_number(in AstNode* self, out ImmValue* imm_value) {
    assert(self->type == AstNode_Number);

    i64 value;
    imm_value->type = ImmValue_Integral;
    UNWRAP_NULL(Util_str_to_i64(self->body.number, &value));
    imm_value->body.integral = (u64)value;

    return SRESULT_OK;
}

static SResult AstNode_eval_operator(in AstNode* self, out ImmValue* imm_value) {
    return self->body.operator.operator.eval(self, imm_value);
}

SResult AstNode_eval(in AstNode* self, out ImmValue* imm_value) {
    switch(self->type) {
        case AstNode_Operator:
            return AstNode_eval_operator(self, imm_value);
        case AstNode_Number:
            return AstNode_eval_number(self, imm_value);
        default:
            TODO();
            break;
    }

    PANIC("unreachable here");

    return SRESULT_OK;
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

static ParserMsg AstNode_parse_number(inout Parser* parser, out AstNode** ptr) {
    char number_literal[256];
    PARSERMSG_UNWRAP(
        Parser_parse_number_raw(parser, number_literal),
        (void)NULL
    );

    AstNode* node = malloc(sizeof(AstNode));
    UNWRAP_NULL(node);
    node->type = AstNode_Number;
    strcpy(node->body.number, number_literal);

    *ptr = node;

    return SUCCESS_PARSER_MSG;
}

static ParserMsg AstTree_parse_number(in AstTree* self, inout Parser* parser) {
    AstNode* ast_node = NULL;
    PARSERMSG_UNWRAP(
        AstNode_parse_number(parser, &ast_node),
        (void)NULL
    );

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

static ParserMsg AstTree_parse_variable(inout AstTree* self, inout Parser* parser, in Generator* generator) {
    Parser parser_copy = *parser;

    char var_name[256];
    PARSERMSG_UNWRAP(
        Parser_parse_ident(&parser_copy, var_name),
        (void)NULL
    );

    if(Generator_get_variable(generator, var_name) == NULL || Generator_get_function(generator, var_name)) {
        ParserMsg msg = {parser->line, "undefined variable found"};
        return msg;
    }

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

    *parser = parser_copy;

    return SUCCESS_PARSER_MSG;
}

static void AstTree_parse_function_free_ast_tree(inout void* ptr) {
    AstTree* tree = ptr;
    AstTree_free(*tree);
    return;
}

static ParserMsg AstTree_parse_function_get_arguments(Parser parser, inout Vec* arguments, in Generator* generator) {
    while(!Parser_is_empty(&parser)) {
        Parser argument_parser;
        PARSERMSG_UNWRAP(
            Parser_split(&parser, ",", &argument_parser),
            (void)NULL
        );

        AstTree tree;
        PARSERMSG_UNWRAP(
            AstTree_parse(argument_parser, generator, &tree),
            (void)NULL
        );

        Vec_push(arguments, &tree);
    }

    return SUCCESS_PARSER_MSG;
}

static ParserMsg AstTree_parse_function(inout AstTree* self, inout Parser* parser, in Generator* generator) {
    Parser parser_copy = *parser;

    AstNode** operand = AstNode_get_operand(&self->node, OPERATOR_PRIORITY_MAX);
    if(*operand == NULL) {
        ParserMsg msg = {parser->line, "expected function name"};
        return msg;
    }

    Parser paren_parser;
    PARSERMSG_UNWRAP(
        Parser_parse_paren(&parser_copy, &paren_parser),
        (void)NULL
    );

    Vec arguments = Vec_new(sizeof(AstTree));
    PARSERMSG_UNWRAP(
        AstTree_parse_function_get_arguments(paren_parser, &arguments, generator),
        Vec_free_all(arguments, AstTree_parse_function_free_ast_tree)
    );

    AstNode** leaf_node = AstNode_get_operand(&self->node, OPERATOR_PRIORITY_MAX);
    if(*leaf_node == NULL) {
        Vec_free_all(arguments, AstTree_parse_function_free_ast_tree);
        ParserMsg msg = {parser_copy.line, "expected token"};
        return msg;
    }

    AstNode* node = malloc(sizeof(AstNode));
    node->type = AstNode_Function;
    node->body.function.function_ptr = *leaf_node;
    node->body.function.arguments = arguments;

    *leaf_node = node;

    *parser = parser_copy;

    return SUCCESS_PARSER_MSG;
}

static ParserMsg AstNode_parse_type(inout Parser* parser, in Generator* generator, out AstNode** ptr) {
    Type type;
    PARSERMSG_UNWRAP(
        Type_parse(parser, generator, &type),
        (void)NULL
    );

    AstNode* node = malloc(sizeof(AstNode));
    UNWRAP_NULL(node);
    node->type = AstNode_Type;
    node->body.type = type;
    *ptr = node;

    return SUCCESS_PARSER_MSG;
}

static ParserMsg AstTree_parse_type(inout AstTree* self, inout Parser* parser, in Generator* generator) {
    optional AstNode** leaf = AstNode_get_leaf(&self->node);
    if(leaf == NULL) {
        ParserMsg msg = {parser->line, "unexpected token"};
        return msg;
    }

    AstNode* nodeptr;
    PARSERMSG_UNWRAP(
        AstNode_parse_type(parser, generator, &nodeptr),
        (void)NULL
    );

    *leaf = nodeptr;

    return SUCCESS_PARSER_MSG;
}

static ParserMsg AstTree_parse_type_cast(inout AstTree* self, inout Parser* parser, in Generator* generator) {
    static Operator TYPE_CAST_OPERATOR = {"(cast)", true, true, 14, NULL};

    Parser parser_copy = *parser;

    AstNode** operand = AstNode_get_operand(&self->node, TYPE_CAST_OPERATOR.priority);
    if(*operand != NULL) {
        ParserMsg msg = {parser->line, "mismatching operator"};
        return msg;
    }

    AstNode* type_node_ptr;
    PARSERMSG_UNWRAP(
        AstNode_parse_type(parser, generator, &type_node_ptr),
        (void)NULL
    );

    AstNode* node = malloc(sizeof(AstNode));
    UNWRAP_NULL(node);

    node->type = AstNode_Operator;
    node->body.operator.operator = TYPE_CAST_OPERATOR;
    node->body.operator.left = type_node_ptr;
    node->body.operator.right = NULL;

    *operand = node;

    *parser = parser_copy;

    return SUCCESS_PARSER_MSG;
}

static ParserMsg AstTree_parse_index(inout AstTree* self, inout Parser* parser, in Generator* generator) {
    static Operator INDEX_OPERATOR = {"[]", true, false, 16, NULL};

    Parser parser_copy = *parser;

    Parser index_parser;
    PARSERMSG_UNWRAP(
        Parser_parse_index(&parser_copy, &index_parser),
        (void)NULL
    );


    AstNode** operand = AstNode_get_operand(&self->node, INDEX_OPERATOR.priority);
    if(*operand == NULL) {
        ParserMsg msg = {parser_copy.line, "expected operand"};
        return msg;
    }

    AstTree tree;
    PARSERMSG_UNWRAP(
        AstTree_parse(index_parser, generator, &tree),
        (void)NULL
    );

    AstNode* node = malloc(sizeof(AstNode));
    UNWRAP_NULL(node);
    node->type = AstNode_Operator;
    node->body.operator.operator = INDEX_OPERATOR;
    node->body.operator.left = *operand;
    node->body.operator.right = tree.node;

    *operand = node;

    *parser = parser_copy;

    return SUCCESS_PARSER_MSG;
}

static ParserMsg AstTree_parse_paren(inout AstTree* self, inout Parser* parser, in Generator* generator) {
    static Operator PAREN_OPERATOR = {"()", false, true, 16, AstNode_eval_operator_paren};
    static u32 PRIORITY = 16;

    Parser parser_copy = *parser;

    AstNode** operand = AstNode_get_operand(&self->node, PRIORITY);
    if(*operand != NULL) {
        ParserMsg msg = {parser->line, "unexpected token"};
        return msg;
    }

    Parser inner_parser;
    Operator operator = PAREN_OPERATOR;
    PARSERMSG_UNWRAP(Parser_parse_paren(&parser_copy, &inner_parser), (void)NULL);

    AstTree tree;
    PARSERMSG_UNWRAP(
        AstTree_parse(inner_parser, generator, &tree),
        (void)(NULL)
    );

    AstNode* node = malloc(sizeof(AstNode));
    UNWRAP_NULL(node);
    node->type = AstNode_Operator;
    node->body.operator.operator = operator;
    node->body.operator.left = NULL;
    node->body.operator.right = tree.node;

    *operand = node;

    *parser = parser_copy;

    return SUCCESS_PARSER_MSG;
}

ParserMsg AstTree_parse(Parser parser, in Generator* generator, out AstTree* ptr) {
    ptr->node = NULL;

    while(!Parser_is_empty(&parser)) {
        if(!ParserMsg_is_success(AstTree_parse_number(ptr, &parser))
            && !ParserMsg_is_success(AstTree_parse_type_cast(ptr, &parser, generator))
            && !ParserMsg_is_success(AstTree_parse_index(ptr, &parser, generator))
            && !ParserMsg_is_success(AstTree_parse_paren(ptr, &parser, generator))
            && !ParserMsg_is_success(AstTree_parse_operator(ptr, &parser))
            && !ParserMsg_is_success(AstTree_parse_function(ptr, &parser, generator))
            && !ParserMsg_is_success(AstTree_parse_type(ptr, &parser, generator))
            && !ParserMsg_is_success(AstTree_parse_variable(ptr, &parser, generator))) {
            ParserMsg msg = {parser.line, "invalid expression"};
            return msg;
        }
    }

    return SUCCESS_PARSER_MSG;
}

SResult AstTree_eval(in AstTree* self, out ImmValue* imm_value) {
    return AstNode_eval(self->node, imm_value);
}


