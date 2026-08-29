#include "../../include/archive_f/archive.h"
#include <stdint.h>

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
}