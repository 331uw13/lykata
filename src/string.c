#include <stdlib.h>
#include <string.h>

#include "string.h"


#define STR_DEFMEMSIZE 64
#define STR_REALLOC_BYTES 64



// Allocates more memory for string if needed.
static bool string_memcheck(struct string_t* str, uint32_t size_add) {
    if(!str->bytes) {
        str->bytes = malloc(STR_DEFMEMSIZE);
        str->memsize = STR_DEFMEMSIZE;
        str->size = 0;
        // TODO: handle memory errors.
        return true;
    }

    if((str->size + size_add) < str->memsize) {
        return true;
    }


    uint32_t new_size = str->memsize + size_add + STR_REALLOC_BYTES;
    char* new_ptr = realloc(str->bytes, new_size);
    
    if(!new_ptr) {
        // TODO: handle memory errors.
        return false;
    }

    str->bytes = new_ptr;
    str->memsize = new_size;

    return true;
}


void string_alloc(struct string_t* str) {
    str->memsize = STR_DEFMEMSIZE;
    str->bytes = calloc(1, str->memsize);
    str->size = 0;
    // TODO: handle memory errors.
}

void string_free(struct string_t* str) {
    if(str->bytes) {
        free(str->bytes);
        str->bytes = NULL;
        str->memsize = 0;
        str->size = 0;
    }
}

struct string_t create_string() {
    struct string_t str;
    string_alloc(&str);
    return str;
}

void string_nullterm(struct string_t* str) {
    if(!str->bytes) {
        return;
    }
    if(str->bytes[str->size] == '\0') {
        return;
    }
    
    if(!string_memcheck(str, 1)) {
        return; // TODO: handle memory errors.
    }

    str->bytes[str->size] = '\0';
}

void string_move(struct string_t* str, char* data, uint32_t size) {
    if(!string_memcheck(str, size)) {
        return; // TODO: handle memory errors.
    }

    string_clear(str);
    memmove(str->bytes, data, size);
    str->size = size;
}

void string_pushbyte(struct string_t* str, char ch) {
    if(!string_memcheck(str, 1)) {
        return; // TODO: handle memory errors.
    }

    str->bytes[str->size] = ch;
    str->size += 1;
}

void string_popback(struct string_t* str) {
    if(!str) {
        return;
    }
    if(str->size > 0) {
        str->bytes[str->size-1] = 0;
        str->size--;
    }
}

void string_clear(struct string_t* str) {
    if(!str->bytes) {
        return;
    }

    memset(str->bytes, 0, (str->size < str->memsize) ? str->size : str->memsize);
    str->size = 0;
}

void string_reserve(struct string_t* str, uint32_t size) {
    string_memcheck(str, size);
}

char string_lastbyte(struct string_t* str) {
    if(!str) {
        return 0;
    }
    if(!str->bytes) {
        return 0;
    }
    if(str->size >= str->memsize) {
        return 0;
    }
    return str->bytes[str->size];
}

bool string_append(struct string_t* str, char* data, uint32_t size) {
    if(!string_memcheck(str, size)) {
        return false;
    }

    memmove(str->bytes + str->size, data, size);
    str->size += size;
    
    return true;
}


