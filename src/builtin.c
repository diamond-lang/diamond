#include "builtin.h"

char* builtin =
    "type Bool\n"
    "type Float64\n"
    "type None\n"
    "function print(value: t): None builtin\n"
    "interface ==[t](left: t, right: t): Bool\n"
    "interface !=[t](left: t, right: t): Bool\n"
    "interface <[t](left: t, right: t): Bool\n"
    "interface <=[t](left: t, right: t): Bool\n"
    "interface >[t](left: t, right: t): Bool\n"
    "interface >=[t](left: t, right: t): Bool\n"
    "interface *[t](left: t, right: t): t\n"
    "interface /[t](left: t, right: t): t\n"
    "interface +[t](left: t, right: t): t\n"
    "interface -[t](left: t, right: t): t\n"
    "";