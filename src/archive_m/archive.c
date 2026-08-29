#include "../../include/archive_m/archive.h"
#include "../../include/archive_f/archive.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

Archive * archive_create(char *name, char *description){
    Archive * archive = malloc(sizeof(Archive));
    if(!archive) return NULL;
    
    archive->directory_offset = sizeof(uint32_t) + sizeof(uint64_t); /* magic 4 bytes + the offset location section */
    archive->fp = malloc(sizeof(FILE)); // creating file pointer, opening the file and writing data is in archive_f
    archive->groups = vector_create();
    archive->entries = vector_create();
    archive->fields = vector_create();
    archive->name = name;
    archive->description = description;
    archive->version = ARCHIVE_VERSION /* current project version */ ; 
    archive->num_of_entries = 0;
    archive->num_of_groups = 0; 
    
    /* Creating the default group (root) */
    Group * root_group = group_create(archive, "root"); // increases num_of_groups by 1.

    /* archive size = archive header + directory size*/
    archive->size = MAGIC_SIZE + OFFSET_SIZE + directory_size(archive) ; 
    
    return archive;
}