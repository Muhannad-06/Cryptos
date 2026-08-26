#include "../../include/archive_m/archive.h"
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