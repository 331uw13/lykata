#ifndef STRING_H
#define STRING_H


#include <stdint.h>

struct string_t {
    char* bytes;
    uint32_t size;
    uint32_t memsize;
};


void string_alloc(struct string_t* str);
void string_free(struct string_t* str);

struct string_t create_string();

// Makes sure the str->bytes is null terminated.
void string_nullterm(struct string_t* str);

// Move 'data' to beginning of 'str'
void string_move(struct string_t* str, char* data, uint32_t size);

// Add byte to end of string.
void string_pushbyte(struct string_t* str, char ch);

// Remove byte from end of string.
void string_popback(struct string_t* str);

// Sets all 'str->size' bytes to 0
void string_clear(struct string_t* str);

// Makes sure str can hold 'size' number of bytes.
void string_reserve(struct string_t* str, uint32_t size);

char string_lastbyte(struct string_t* str);
bool string_append(struct string_t* str, char* data, uint32_t size);


#endif
