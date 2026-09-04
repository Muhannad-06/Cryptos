/* Dynamic array that is supposed to hold pointers to entries, groups or fields.
 * This implementation of vector holds void* pointers.
*/
#ifndef VECTOR_H
#define VECTOR_H
#include <stdlib.h>

typedef struct Vector{
    void ** data;
    size_t size;
    size_t capacity;
} Vector;

Vector * vector_create();
Vector *vector_copy(Vector *src);
void * vector_push_back(Vector * vector, void * value);
void * vector_pop_back(Vector * vector);
void * vector_at(Vector * vector, size_t index);
void * vector_remove_value(Vector *vector, void * value); /* clean the vector from specified data. */
void vector_destroy(Vector *vector);

#endif