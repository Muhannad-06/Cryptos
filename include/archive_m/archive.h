#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "../utils/vector.h"
#include "group.h"
#include "entry.h"
#include"field.h"



typedef struct Archive{

    uint64_t directory_offset;
    uint64_t size; // the offset of the last byte.
    FILE *fp;
    Vector *groups;
    Vector *entries;
    Vector *fields;
    char* name;
    char* description;
    uint16_t version;
    uint16_t num_of_entries;
    uint16_t num_of_groups;
    

} Archive;

/* #TODO: Functions*/

Archive * archive_create(char *name, char *description);
void archive_destroy(Archive * archive);

#endif