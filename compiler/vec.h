#pragma once

#include "types.h"

typedef struct Vec {
    void* ptr;
    u32 size;
    u32 len;
    u32 capacity;
} Vec;

Vec Vec_new(u32 size);

void Vec_free(Vec vec);

void* Vec_index(Vec* self, u32 index);

void* Vec_as_ptr(Vec* self);

void Vec_push(Vec* self, void* object);

void Vec_pop(Vec* self, void* ptr);

void Vec_last(Vec* self, void* ptr);

u32 Vec_len(Vec* self);

u32 Vec_capacity(Vec* self);

u32 Vec_size(Vec* self);

