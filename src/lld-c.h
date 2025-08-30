#ifndef link_h
#define link_h

#include <stdbool.h>
#include <stdint.h>

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

bool lld_link(CStringList args);

#ifdef __cplusplus
}
#endif

#endif