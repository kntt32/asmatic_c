#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "vec.h"

Vec Vec_new(u32 size) {
    Vec vec = {NULL, size, 0, 0};
    return vec;
}

void Vec_free(Vec vec) {
    free(vec.ptr);
}

void* Vec_index(Vec* self, u32 index) {
    if(self->len <= index) {
        PANIC("out of range");
    }
    return self->ptr + self->size * index;
}

void* Vec_as_ptr(Vec* self) {
    return self->ptr;
}

void Vec_print(Vec* self, void (*f)(void*)) {
    printf("Vec[");
    for(u32 i=0; i<self->len; i++) {
        f(Vec_index(self, i));
        printf(", ");
    }
    printf("]");
}

void Vec_push(Vec* self, void* restrict object) {
    if(self->capacity == self->len) {
        u32 new_capacity = (self->len + 1) * 2;
        self->ptr = realloc(self->ptr, new_capacity * self->size);
        if(self->ptr == NULL) {
            PANIC("failed to realloc");
        }
        self->capacity = new_capacity;
    }
    memcpy(self->ptr + self->size * self->len, object, self->size);
    self->len ++;
}

void Vec_pop(Vec* self, void* restrict ptr) {
    if(ptr != NULL) {
        Vec_last(self, ptr);
    }
    self->len --;
}

void Vec_last(Vec* self, void* restrict ptr) {
    if(self->len == 0) {
        PANIC("length is zero");
    } else {
        memcpy(ptr, self->ptr + self->size * (self->len - 1), self->size);
    }
}

Vec Vec_from(void* src, u32 len, u32 size) {
    Vec vec = Vec_new(size);
    Vec_append(&vec, src, len);
    return vec;
}

void Vec_append(Vec* self, void* src, u32 len) {
    for(u32 i=0; i<len; i++) {
        Vec_push(self, src + self->size * i);
    }
    return;
}

u32 Vec_len(Vec* self) {
    return self->len;
}

u32 Vec_capacity(Vec* self) {
    return self->capacity;
}

u32 Vec_size(Vec* self) {
    return self->size;
}

