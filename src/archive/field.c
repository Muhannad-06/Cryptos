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

/* calculate crc */
uint32_t field_crc(Field *field){
    // #TODO
    return 0;
}

/* mutators */
void field_update_parents(Field *field){
    field->entry->last_modification_date = field->last_modification_date;
    field->entry->group->last_modification_date = field->last_modification_date;

    Archive * archive = field->entry->group->archive;
    archive->last_modification_date = field->last_modification_date;
    archive->written = 0;
    vector_push_back(archive->fields_updated, field);
}

void field_set_name(Field *field, char * name){
    field->last_modification_date = (uint32_t) time(NULL);
    field->name = name;

    field_update_parents(field);
}

void field_set_content(Field *field, FieldType type, void *data){
    field->last_modification_date = (uint32_t) time(NULL);
    field->type = type;
    field->content = data;
    field->crc = field_crc(field);
    field_update_parents(field);
}

void field_set_compression(Field *field, CompressionType compression_type){
    field->last_modification_date = (uint32_t) time(NULL);
    field->compression = compression_type;

    field_update_parents(field);
}

void field_set_entry(Field *field, Entry *new_entry){
    field->last_modification_date = (uint32_t) time(NULL);

    vector_remove_value(field->entry->group->fields, field);
    vector_remove_value(field->entry->fields, field);
    field_update_parents(field);
    
    field->entry = new_entry;
    vector_push_back(field->entry->group->fields, field);
    field_update_parents(field);
}

void field_delete(Field *field){
    // remove from achive
    vector_remove_value(field->entry->group->archive->fields, field);
    // remove from group.
    vector_remove_value(field->entry->group->fields, field);
    // remove from entry
    vector_remove_value(field->entry->fields, field);

    field->last_modification_date = (uint32_t) time(NULL);
    field_update_parents(field);
    field_destroy(field);
}