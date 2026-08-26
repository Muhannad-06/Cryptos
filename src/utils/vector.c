#include "../../include/utils/vector.h"
#include <stdio.h> 
#include <stdlib.h>

Vector * vector_create(){
    Vector * vector = malloc(sizeof(Vector));    
    vector->data = NULL;
    vector->capacity = 0;
    vector->size = 0;
    
    return vector;
}

void * vector_push_back(Vector * vector, void * value){
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

    if(vector->size <= 0)
        return NULL;

    void * value = vector->data[vector->size];
    vector->data[vector->size] = NULL;
    (vector->size)--;
    
    return value;
}

void * vector_at(Vector * vector, size_t index){
    if (index >= vector->size) {
    fprintf(stderr, "Index out of bounds\n"); // Print an error message
    exit(1); 
    }

    return vector->data[index];
}

void * vector_remove_value(Vector *vector, void * value){
    /* #TODO: Change implementation to make new vector to avoid errors of accessing null pointers */
    int c = 0;
    for (int i = 0; i < (int) vector->size ; i++) {
        if(vector->data[i] == value){
            c++;
            vector->data[i] = NULL;
        }
    }

    if(c > 0){
        return value;
    } else {
        return NULL; // nothing was deleted.
    }
}

void vector_destroy(Vector *vector){
    if(!vector)
        return

    free(vector->data);
    free(vector);
}