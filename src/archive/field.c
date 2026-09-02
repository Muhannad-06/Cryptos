#include "../../include/archive/archive.h"
#include <stdint.h>
#include <time.h>

Field * field_create(Entry *entry, uint64_t offset, uint16_t type, char* name){
    Field * field = malloc(sizeof(Field));
    
    if(!field){
        return NULL;
    }
    
    field->entry = entry;
    field->offset = ++(entry->group->archive->size);
    field->compressed_size=0;
    field->size = 0;
    field->crc = 0;
    field->creation_date = (uint32_t)time(NULL);
    field -> last_modification_date = field->creation_date;
    field->type = type;
    field->compression = 0;
    
    // add the field pointer to other elements
    vector_push_back(entry->fields, field);
    vector_push_back(entry->group->fields, field);
    vector_push_back(entry->group->archive->fields, field);

    return field;
}

void field_destroy(Field *field) {
    if (!field) return;
    free(field->name);
    free(field);
}