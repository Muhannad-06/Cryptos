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
    uint64_t prev_dir_offset; /* the offset of the previous directory. This field is used for file history. */
    uint64_t size; // the offset of the last byte.
    FILE *fp;
    Vector *groups;
    Vector *entries;
    Vector *fields;
    /* vectors to collect entities that have changes to be written. */
    Vector *groups_updated;
    // Vector *entries_updated;/* groups & entries are not necessary as there info would be written in the directory in each change. */
    // Vector *fields_updated;
    uint32_t num_of_changes; /* represents number of directories in the archive */
    char* name;
    char* description;
    uint16_t version;
    uint16_t num_of_entries;
    uint16_t num_of_groups;
    uint8_t written; /* a boolean value indicates if this version or current changes is written into the archive */

} Archive;

/* #TODO: Functions*/

Archive * archive_create(char *name, char *description);
void archive_destroy(Archive * archive);

#endif