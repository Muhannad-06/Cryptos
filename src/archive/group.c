#include "../../include/archive/archive.h"
#include <stdint.h>
#include "../../include/utils/error.h"
#include <time.h>

Group * group_create(Archive * archive, char *name){
    Group * group = malloc(sizeof(Group));
    if(!group){
        return NULL;
    }
    
    group->archive = archive;
    group->creation_date = (uint32_t) time(NULL);
    group->last_modification_date = group->creation_date;
    group->entries = vector_create();
    group->fields = vector_create();
    group->group_id = ++(archive->num_of_groups);
    group->name = name;
    
    // add the group to vectors
    vector_push_back(archive->groups, group);
    
    group_update_parents(group);
    
    return group;
}


void group_destroy(Group *group){

    if(!group){
        return;
    }

    // destroying entries, entry_destroy handles destroying fields
    for(int i = 0; i < group->entries->size; i++){
        entry_destroy(group->entries->data[i]);
    }
    // destroying the group
    vector_destroy(group->entries);
    vector_destroy(group->fields);
    
    free(group->name);
    free(group);
}

/* mutators */

ErrorCode group_update_parents(Group *group){
    if(!group){
        errorp("group_update_parents: null pointer");
        return NULL_POINTER;
    }
    group->archive->last_modification_date = group->last_modification_date;
    group->archive->written = 0;
    return SUCCESS;
}

ErrorCode group_update(Group *group){
    if(!group){
        errorp("group_update: null pointer");
        return NULL_POINTER;
    }
    group->last_modification_date = (uint32_t) time(NULL);
    return SUCCESS;
}

ErrorCode group_set_name(Group *group, char *name){
    if (!group || !name) {
        errorp("group_set_name: null pointer");
        return NULL_POINTER;
    }
    ErrorCode status = SUCCESS;

    group->name = name;
    
    status+=group_update(group);
    status+=group_update_parents(group);
    return (status !=SUCCESS )? FAILURE:SUCCESS;
}

ErrorCode group_delete(Group *group){
    if (!group) {
        errorp("group_delete: null pointer");
        return NULL_POINTER;
    }
    ErrorCode status = SUCCESS;
    // remove from archive
    group->archive->num_of_groups--;
    vector_remove_value(group->archive->groups, group);
    
    // delete it's entries.
    for (int i=0; i<group->entries->size; i++) {
        entry_delete(vector_at(group->entries, i));
    }

    status+=group_update(group);
    status+=group_update_parents(group);
    group_destroy(group);
    return (status !=SUCCESS )? FAILURE:SUCCESS;
}