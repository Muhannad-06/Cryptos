#include "../../include/utils/vector.h"
#include "../../include/utils/error.h"
#include <stdio.h> 
#include <stdlib.h>

Vector * vector_create(){
    Vector * vector = malloc(sizeof(Vector));    
    vector->data = NULL;
    vector->capacity = 0;
    vector->size = 0;
    
    return vector;
}

Vector *vector_copy(Vector *src){
    // if (!src) return NULL;
    if (!src) {
        error("vector_copy: null pointer");
    }
    Vector *copy = vector_create();    
    copy->capacity = src->capacity;
    // deep copy for the data array.
    if(copy->capacity > 0){
        copy->data = malloc((copy->capacity) * sizeof(void *));
        
        void **sp = src->data;
        void **np = copy->data;
        for(int i = 0; i<src->size ; i++){
            if(*sp == NULL){
                sp++;
                continue;
            }
            *np++ = *sp++;
            copy->size++;
        }
    }
    
    return copy;
}

void * vector_push_back(Vector * vector, void * value){
    if (vector == NULL) {
        error("vector_push_back: null pointer");
    }
    if(vector->data == NULL){
        vector->data = malloc(sizeof(void *));
        if(vector->data == NULL)
            return NULL;
        
        vector->capacity = 1;
    } else if(vector->size >= vector-> capacity){
        vector->capacity *= 2;
        vector->data = realloc(vector->data, vector->capacity * sizeof(void *));
        if(vector->data == NULL)
            return NULL;
       
    }
    
    vector->data[vector->size] = value;
    (vector->size)++;

    return value;
}

void * vector_pop_back(Vector * vector){
    if (vector == NULL) {
        error("vector_pop_back: null pointer");
    }

    if(vector->size <= 0)
        return NULL;

    void * value = vector->data[vector->size];
    vector->data[vector->size] = NULL;
    (vector->size)--;
    
    return value;
}

void * vector_at(Vector * vector, size_t index){
    if (vector == NULL) {
        error("vector_at: null pointer");
    }
    if (index >= vector->size) {
        /* note: size_t is unsigned. >= is enough */
        error("vector_at: index out of bounds");
    }

    return vector->data[index];
}

void * vector_remove_value(Vector *vector, void * value){
    if (vector == NULL) {
        error("vector_remove_value: null pointer");
    }
    if (!vector || vector->capacity == 0) return NULL;
    /* create a new data array. */ 
    void **newData = malloc(vector->capacity * sizeof(void *));


    void **np = newData;
    int c = 0;
    for (int i = 0; i < (int) vector->size ; i++) {
        if(vector->data[i] == value){
            c++;
            // vector->data[i] = NULL;
            continue;
        }
        *np++ = vector->data[i];
    }
    
   if(c <= 0){
    free(newData);
    return NULL;
   }

   free(vector->data);
   vector->size -= c;
   vector->data = newData;

    return value;
}

void vector_destroy(Vector *vector){
    if(!vector)
        return

    free(vector->data);
    free(vector);
}