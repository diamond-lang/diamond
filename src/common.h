#ifndef common_h
#define common_h

#include <assert.h>

#define todo() assert(false);
#define unreachable() assert(false);

typedef enum { Windows, Linux, MacOS } Platform;

Platform currentPlatform();

#endif