#include "arena.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    void** items;
    uint32_t count;
    uint32_t capacity;
} Lifetime;

#define Lifetime() (Lifetime){NULL, 0, 0}

static void append(Lifetime* lifetime, void* allocation) {
    if (lifetime->count >= lifetime->capacity) {
        if (lifetime->capacity == 0) {
            lifetime->capacity = 256;
        } else {
            lifetime->capacity *= 2;
        }
        lifetime->items =
            realloc(lifetime->items, lifetime->capacity * sizeof(void*));
    }
    lifetime->items[lifetime->count] = allocation;
    lifetime->count += 1;
}

typedef struct {
    Lifetime* items;
    uint32_t count;
    uint32_t capacity;
} LifetimeStack;

static void push(LifetimeStack* lifetimes, Lifetime lifetime) {
    if (lifetimes->count >= lifetimes->capacity) {
        if (lifetimes->capacity == 0) {
            lifetimes->capacity = 256;
        } else {
            lifetimes->capacity *= 2;
        }
        lifetimes->items =
            realloc(lifetimes->items, lifetimes->capacity * sizeof(Lifetime));
    }
    lifetimes->items[lifetimes->count] = lifetime;
    lifetimes->count += 1;
}

static void pop(LifetimeStack* lifetimes) { lifetimes->count -= 1; }

#define stack_top(stack) stack.items[stack.count - 1]

static LifetimeStack lifetimes = {NULL, 0, 0};

void arena_newLifetime() { push(&lifetimes, Lifetime()); }

uint32_t arena_currentLifetime() {
    return lifetimes.count > 0 ? lifetimes.count - 1 : 0;
}

void* arena_realloc(uint32_t lifetime, void* pointer, uint32_t numberOfBytes) {
    assert(lifetime < lifetimes.count);
    Lifetime* currentLifetime = &lifetimes.items[lifetime];
    if (pointer == NULL) {
        void* newAllocation = malloc(numberOfBytes);
        assert(newAllocation != NULL);
        append(currentLifetime, newAllocation);
        return newAllocation;
    } else {
        for (uint32_t i = 0; i < currentLifetime->count; i++) {
            if (currentLifetime->items[i] == pointer) {
                currentLifetime->items[i] = realloc(pointer, numberOfBytes);
                return currentLifetime->items[i];
            }
        }
        assert(false);
    }
}

void arena_swapAllocations(
    uint32_t lifetime, void** allocation, void* newAllocation
) {
    assert(lifetime < lifetimes.count);
    Lifetime* currentLifetime = &lifetimes.items[lifetime];
    if (*allocation == NULL) {
        append(currentLifetime, newAllocation);
    } else {
        bool founded = false;
        for (uint32_t i = 0; i < currentLifetime->count; i++) {
            if (currentLifetime->items[i] == allocation) {
                free(currentLifetime->items[i]);
                currentLifetime->items[i] = newAllocation;
                founded = true;
                break;
            }
        }
        assert(founded);
    }
    *allocation = newAllocation;
}

void arena_destroyCurrentLifetime() {
    Lifetime currentLifetime = stack_top(lifetimes);
    for (uint32_t i = 0; i < currentLifetime.count; i++) {
        free(currentLifetime.items[i]);
    }
    free(currentLifetime.items);
    pop(&lifetimes);
}

void arena_destroyAllLifetimes() {
    while (lifetimes.count > 0) {
        arena_destroyCurrentLifetime();
    }
    free(lifetimes.items);
}

void arena_assertNoLifetimesRemaining() { assert(lifetimes.count == 0); }