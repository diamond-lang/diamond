#ifndef arena_h
#define arena_h

#include <stddef.h>
#include <stdlib.h>

void arena_newLifetime();
uint32_t arena_currentLifetime();
void* arena_realloc(uint32_t lifetime, void* pointer, uint32_t numberOfBytes);
void arena_swapAllocations(
    uint32_t lifetime, void** allocation, void* newAllocation
);
void arena_destroyCurrentLifetime();
void arena_assertNoLifetimesRemaining();

#endif