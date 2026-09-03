#include "../../include/archive/archive.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* memory functions. */
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

/* archive format functions */

<<<<<<< Updated upstream
=======
/* dynamic size calculation functions */

>>>>>>> Stashed changes
uint64_t group_header_size(Group *group){
    return MAGIC_SIZE +
        ID_SIZE +
        DATE_SIZE*2 +
        STRING_LENGTH_SIZE +
        (uint64_t) sizeof(group->name);
}

uint64_t entry_header_size(Entry *entry){
    return MAGIC_SIZE+
        ID_SIZE*2 +
        DATE_SIZE*2 +
        STRING_LENGTH_SIZE +
        (uint64_t) sizeof(entry->name);
}

uint64_t field_header_size(Field *field){
    return MAGIC_SIZE +
        ID_SIZE +
        TYPE_SIZE*2 +
        OFFSET_SIZE*3 +
        CRC_SIZE +
        DATE_SIZE*2 +
        STRING_LENGTH_SIZE +
        (uint64_t) sizeof(field->name);
}

uint64_t local_field_header_size(Field *field){
    return MAGIC_SIZE +
        ID_SIZE +
        TYPE_SIZE*2 +
        OFFSET_SIZE*2 +
        CRC_SIZE +
        DATE_SIZE*2 +
        STRING_LENGTH_SIZE +
        (uint64_t) sizeof(field->name);
}

uint64_t directory_size(Archive *archive){
    uint64_t group_headers_size= 0;
    uint64_t entry_headers_size= 0;
    uint64_t field_headers_size= 0;

    for(int i = 0; i<(archive->fields->size); i++){
        field_headers_size += field_header_size(archive->fields->data[i]);
    }

    for(int i = 0; i<(archive->groups->size); i++){
        group_headers_size += group_header_size(archive->groups->data[i]);
    }

    for(int i = 0; i<(archive->entries->size); i++){
        entry_headers_size += entry_header_size(archive->entries->data[i]);
    }

    return MAGIC_SIZE +
        FILE_VERSION_SIZE +
        OFFSET_SIZE +
        DATE_SIZE*2 +
        ID_SIZE*3 +
        STRING_LENGTH_SIZE*2 +
        (uint64_t) sizeof(archive->name) +
        (uint64_t) sizeof(archive->description) +
        group_headers_size +
        entry_headers_size +
        field_headers_size;
<<<<<<< Updated upstream
=======
}

/* writing functions */

ErrorCode write_group_header(Group * group){
    /* check if file is open and appendable. */
    if(group->archive->fp == NULL){
        error();
    }
        /*write magic number. */
    if()

>>>>>>> Stashed changes
}