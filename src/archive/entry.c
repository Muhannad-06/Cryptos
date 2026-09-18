#include "../../include/archive/archive.h"
#include "../../include/utils/error.h"
#include <stdint.h>
#include <time.h>

Entry * entry_create(Group* group, char* name){
    if (!group || !name) {
        errorp("entry_create: null pointer.");
    }
    Entry * entry = malloc(sizeof(Entry));
    if(!entry)
        error("entry_create: couldn't allocate memory for entry: %s", name);

    entry->creation_date = (uint32_t) time(NULL);
    entry->last_modification_date = entry->creation_date;
    entry->group = group;
    entry->entry_id = (group->archive->num_of_entries)++; /* get the last entry's ID and increase it by 1. start with 0*/
    entry->name = name;
    entry->fields = vector_create();
    
    // adding the entry to vectors
    vector_push_back(group->entries, entry);
    vector_push_back(group->archive->entries, entry);
    
    entry_update_parents(entry);
    
    return entry;
}

void entry_destroy(Entry * entry){
    if(!entry)
        return;
    
    // free(entry->name);
    
    /* Destroy all fields */
    for(int i = 0; i < entry->fields->size ; i++){
        field_destroy(entry->fields->data[i]);
    }

    vector_destroy(entry->fields); // destroy the vector
}

/* mutators */

ErrorCode entry_update_parents(Entry *entry){
    if(!entry){
        error("entry_update_parents: null pointer");
    }
    entry->group->last_modification_date = entry->last_modification_date;

    entry->group->archive->last_modification_date = entry->last_modification_date;
    entry->group->archive->written = 0;
    return SUCCESS;
}

ErrorCode entry_set_name(Entry *entry, char *name){
    if(!entry){
        error("entry_set_name: null pointer");
    }
    ErrorCode status = SUCCESS; 
    entry->last_modification_date = (uint32_t) time(NULL);
    entry->name = name;
    
    status += entry_update_parents(entry);
    return (status !=SUCCESS )? FAILURE:SUCCESS;
}

ErrorCode entry_set_group(Entry *entry, Group *new_group){
    if(!entry || !new_group){
        error("entry_set_group: null pointer.");
    }
    ErrorCode status = SUCCESS;
    entry->last_modification_date = (uint32_t) time(NULL);

    vector_remove_value(entry->group->entries, entry);
    entry_update_parents(entry);

    entry->group = new_group;
    status +=entry_update_parents(entry);
    return (status !=SUCCESS )? FAILURE:SUCCESS;
}

ErrorCode entry_delete(Entry *entry){
    if(!entry){
        error("entry_delete: null pointer.");
    }
    ErrorCode status = SUCCESS;
    // removing entry from vectors.
    // remove from archive.
    entry->group->archive->num_of_entries--;
    vector_remove_value(entry->group->archive->entries, entry);

    //remove from group.
    vector_remove_value(entry->group->entries, entry);
    
    // removing entry's fields.
    Vector *fields_vector = entry->fields;
    for(int i = 0; i<fields_vector->size; i++){
        status += field_delete(vector_at(fields_vector, i));
    }
    
    entry->last_modification_date = (uint32_t) time(NULL);
    status += entry_update_parents(entry);
    entry_destroy(entry);
    return (status !=SUCCESS )? FAILURE:SUCCESS;
}