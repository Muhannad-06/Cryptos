#include "../../include/archive/archive.h"
#include "../../include/utils/error.h"
#include <stdint.h>
#include <time.h>

Field * field_create(Entry *entry, FieldType type, char* name){
    Field * field = malloc(sizeof(Field));
    
    if(!field){
        error("field_create: null pointer");
    }
    
    field->entry = entry;
    field->offset = 0;
    /* offset is set to 0 initially.
    * the offset of the field is assigned in the write operation.
     */
    field->name = name;
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
    // update modification dates
    field_update(field);
    field_update_parents(field);

    return field;
}

void field_destroy(Field *field) {
    if (!field) return;

    // if (field->type == TEXT || field->type == PASSWORD) {
    //     free(field->content);
    // } else if (field->type == BINARY) {
    //     fclose((FILE *)field->content);
    // }
    //
    // free(field->name);
    free(field);
}

/* calculate crc */
uint32_t field_crc(Field *field){
    // #TODO
    return 0;
}

/* mutators */
ErrorCode field_update_parents(Field *field){
    if(!field || !(field->entry) || !(field->entry->group) || !(field->entry->group->archive)){
        errorp("field_update_parents: null pointer");
        return NULL_POINTER;
    }
    // ErrorCode status = SUCCESS;

    field->entry->last_modification_date = field->last_modification_date;
    field->entry->group->last_modification_date = field->last_modification_date;

    Archive * archive = field->entry->group->archive;
    archive->last_modification_date = field->last_modification_date;
    archive->written = 0;
    int exists = 0;
    for(size_t i = 0; i < archive->fields_updated->size; i++) {
        if(vector_at(archive->fields_updated, i) == field) {
            exists = 1;
            break;
        }
    }
    if(!exists) {
        vector_push_back(archive->fields_updated, field);
    }

    return SUCCESS;
}

ErrorCode field_update(Field *field){ // set field to be unwritten.
    if(!field || !(field->entry) || !(field->entry->group) || !(field->entry->group->archive)){
        errorp("field_update: null pointer");
        return NULL_POINTER;
    }
    field->offset = 0;
    field->size = 0;
    field ->full_size = 0;
    field->last_modification_date = (uint32_t) time(NULL); 

    return SUCCESS;
}

ErrorCode field_set_name(Field *field, char * name){
    if(!field || !(field->entry) || !(field->entry->group) || !(field->entry->group->archive)){
        errorp("field_set_name: null pointer");
        return NULL_POINTER;
    }
    ErrorCode status = SUCCESS;

    field->name = name;

    status +=field_update(field);
    status +=field_update_parents(field);

    return (status !=SUCCESS )? FAILURE:SUCCESS;
}

ErrorCode field_set_content(Field *field, FieldType type, void *data){
    if(!field || !(field->entry) || !(field->entry->group) || !(field->entry->group->archive)){
        errorp("field_set_content: null pointer");
        return NULL_POINTER;
    }
    ErrorCode status = SUCCESS;
    field->type = type;
    field->content = data;
    field->crc = field_crc(field);

    status += field_update(field);
    status += field_update_parents(field);

    return (status !=SUCCESS )? FAILURE:SUCCESS;
}

ErrorCode field_set_compression(Field *field, CompressionType compression_type){
    if(!field || !(field->entry) || !(field->entry->group) || !(field->entry->group->archive)){
        errorp("field_set_compression: null pointer");
        return NULL_POINTER;
    }
    ErrorCode status = SUCCESS;
    field->last_modification_date = (uint32_t) time(NULL);
    field->compression = compression_type;

    status+=field_update(field);
    status+=field_update_parents(field);
    return (status !=SUCCESS )? FAILURE:SUCCESS;
}

ErrorCode field_set_entry(Field *field, Entry *new_entry){
    if(!field || !(field->entry) || !(field->entry->group) || !(field->entry->group->archive)){
        errorp("field_set_entry: null pointer");
        return NULL_POINTER;
    }
    ErrorCode status = SUCCESS;
    vector_remove_value(field->entry->group->fields, field);
    vector_remove_value(field->entry->fields, field);
    field_update_parents(field);
    
    field->entry = new_entry;
    vector_push_back(field->entry->group->fields, field);

    status+=field_update(field);
    status+=field_update_parents(field);
    return (status !=SUCCESS )? FAILURE:SUCCESS;
}

ErrorCode field_delete(Field *field){
    if(!field || !(field->entry) || !(field->entry->group) || !(field->entry->group->archive)){
        errorp("field_set_entry: null pointer");
        return NULL_POINTER;
    }
    ErrorCode status = SUCCESS;
    // remove from archive
    vector_remove_value(field->entry->group->archive->fields, field);
    // remove from group.
    vector_remove_value(field->entry->group->fields, field);
    // remove from entry
    vector_remove_value(field->entry->fields, field);

    status+=field_update(field);
    status+=field_update_parents(field);
    field_destroy(field);

    return (status !=SUCCESS )? FAILURE:SUCCESS;
}