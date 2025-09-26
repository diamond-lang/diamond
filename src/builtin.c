#include "builtin.h"

char* builtin =
    "type Bool\n"
    "type Float64\n"
    "type None\n"
    "function what(a: t, b: t): t builtin\n"
    "function identity(x: t): t builtin\n"
    "function print(value: t): None builtin\n"
    "";