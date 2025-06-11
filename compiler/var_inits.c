#include <stdio.h>
#include "types.h"
#include "var_inits.h"
#include "parser.h"
#include "gen.h"

SResult VarInits_eval(inout Parser* parser, in Generator* generator, in Type* expected_type) {
    Parser parser_copy = *parser;

    TODO();

    *parser = parser_copy;

    return SRESULT_OK;
}

