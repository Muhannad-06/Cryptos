#include "../../include/archive/archive.h"
#include "../../include/utils/error.h"
#include <stdint.h>
#include <time.h>

Entry * entry_create(Group* group, char* name){
    Entry * entry = malloc(sizeof(Entry));
    if(!entry)
        return NULL;
    
    entry->creation_date = (uint32_t) time(NULL);
    entry->last_modification_date = entry->creation_date;
    entry->group = group;
    entry->entry_id = ++(group->archive->num_of_entries); /* get the last entry's ID and increase it by 1. */
    entry->name = name;
    entry->fields = vector_create();
    
    // adding the entry to vectors
    vector_push_back(group->entries, entry);
    vector_push_back(group->archive->entries, entry);
    
    return entry;
}

void entry_destroy(Entry * entry){
    if(!entry)
        return;
    
    free(entry->name);
    
    /* Destroy all fields */
    for(int i = 0; i < entry->fields->size ; i++){
        field_destroy(entry->fields->data[i]);
    }

    vector_destroy(entry->fields); // destroy the vector
}

/* mutators */

void entry_update_parents(Entry *entry){
    entry->group->last_modification_date = entry->last_modification_date;

    entry->group->archive->last_modification_date = entry->last_modification_date;
    entry->group->archive->written = 0;
}

void entry_set_name(Entry *entry, char *name){
    entry->last_modification_date = (uint32_t) time(NULL);
    entry->name = name;
    
    entry_update_parents(entry);
}

void entry_set_group(Entry *entry, Group *new_group){
    if(!entry || !new_group){
        error("entry_set_group: null pointer.");
    }
    entry->last_modification_date = (uint32_t) time(NULL);

    vector_remove_value(entry->group->entries, entry);
    entry_update_parents(entry);

    entry->group = new_group;
    entry_update_parents(entry);
}

void entry_delete(Entry *entry){
    // removing entry from vectors.
    // remove from archive.
    entry->group->archive->num_of_entries--;
    vector_remove_value(entry->group->archive->entries, entry);

    //remove from group.
    vector_remove_value(entry->group->entries, entry);
    
    // removing entry's fields.
    Vector *fields_vector = entry->fields;
    for(int i = 0; i<fields_vector->size; i++){
        field_delete(vector_at(fields_vector, i));
    }
    
    entry->last_modification_date = (uint32_t) time(NULL);
    entry_update_parents(entry);
    entry_destroy(entry);
}