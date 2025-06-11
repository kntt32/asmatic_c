#pragma once
#include "types.h"
#include "parser.h"
#include "gen.h"

SResult VarInits_eval(inout Parser* parser, in Generator* generator, in Type* expected_type);

