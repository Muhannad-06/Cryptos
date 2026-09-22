#include "../../include/archive/archive.h"
#include "../../include/utils/error.h"
#include <stdint.h>
#include <time.h>

Entry * entry_create(Group* group, char* name){
    if (!group || !name) {
        error("entry_create: null pointer.");
    }

    /* handle if name exists */
    if (bst_contains(group->archive->search_tree, name)) {
        error("entry_create: entry name already exists."); // this terminates the program if name already exists.
    }

    Entry * entry = malloc(sizeof(Entry));
    if(!entry)
        error("entry_create: couldn't allocate memory for entry: %s", name);

    entry->creation_date = (uint32_t) time(NULL);
    entry->last_modification_date = entry->creation_date;
    entry->group = group;
    entry->entry_id = (group->archive->entries->size); /* get the last entry's ID and increase it by 1. start with 0*/
    entry->name = name;
    entry->fields = vector_create();
    
    // adding the entry to vectors
    vector_push_back(group->entries, entry);
    vector_push_back(group->archive->entries, entry);
    
    entry_update_parents(entry);
    
    return entry;
}

ErrorCode entry_destroy(Entry *entry){
    if(!entry)
        return NULL_POINTER;
    // free(entry->name);
    ErrorCode status = SUCCESS;
    
    vector_destroy(entry->fields); // destroy the vector

    return (status != SUCCESS )? SUCCESS:FAILURE;
}

ErrorCode entry_delete(Entry *entry) {
    if (!entry || !entry->group || !entry->group->archive) {
        errorp("entry_delete: null pointer");
        return NULL_POINTER;
    }
    Archive *archive = entry->group->archive;
    Group *group = entry->group;

    ErrorCode status = SUCCESS;

    status += (vector_remove_value(archive->entries, entry)) == NULL; // if vector_remove_value returns null this means it failed.
    status += (vector_remove_value(group->entries, entry)) == NULL;

    /* delete all fields */
    Vector *fields_to_delete = vector_copy(entry->fields);
    for (size_t i = 0; i < fields_to_delete->size; i++) {
        status += field_delete(vector_at(fields_to_delete, i));
        /* deleting field alters entry->fields but doesn't touch fields_to_delete */
    }
    vector_destroy(fields_to_delete);

    status += entry_update(entry);
    status += entry_update_parents(entry);

    status += entry_destroy(entry);

    return (status !=SUCCESS)? FAILURE: SUCCESS;
}

char * entry_to_string(Entry * entry) {
    if(!entry) {
        error("entry_to_string: null pointer.");
    }
    char *string;
    asprintf(&string, "Entry ID: %u, Entry Name:%s, num of fields: %lu, parent group: %s", entry->entry_id, entry->name, entry->fields->size, entry->group->name);
    return string;
}

/* mutators */

ErrorCode entry_update(Entry *entry) {
    if(!entry || !entry->group || !entry->group->archive) {
        error("entry_update: null pointer.");
    }

    entry->last_modification_date = (uint32_t) time(NULL);
    return SUCCESS;
}

ErrorCode entry_update_parents(Entry *entry){
    if(!entry){
        error("entry_update_parents: null pointer");
    }
    entry->group->last_modification_date = entry->last_modification_date;

    entry->group->archive->last_modification_date = entry->last_modification_date;
    entry->group->archive->written = 0;
    search_tree_gen(entry->group->archive);
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
