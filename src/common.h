#ifndef common_h
#define common_h

#include <assert.h>

#define switch_all_cases(expression, scoped_switch)                           \
    _Pragma("GCC diagnostic push")                                            \
        _Pragma("GCC diagnostic error \"-Wswitch-enum\"") switch (expression) \
            scoped_switch _Pragma("GCC diagnostic pop")

#define todo() assert(false);

#endif