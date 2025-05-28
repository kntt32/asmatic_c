#include <stdio.h>
#include "str.h"

static char null_char = '\0';

String String_new() {
    String string = {Vec_new(sizeof(char))};
    Vec_push(&string.vec, &null_char);
    return string;
}

void String_free(String self) {
    Vec_free(self.vec);
}

char* String_as_ptr(String* self) {
    return Vec_as_ptr(&self->vec);
}

void String_append(String* self, char* src) {
    u32 src_len = strlen(src);
    
    Vec_pop(&self->vec, NULL);
    for(u32 i=0; i<src_len; i++) {
        Vec_push(&self->vec, src + i);
    }
    Vec_push(&self->vec, &null_char);

    return;
}





