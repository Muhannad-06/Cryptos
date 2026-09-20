#include "../../include/archive/archive.h"
#include <stdint.h>
#include "../../include/utils/error.h"
#include <time.h>

Group * group_create(Archive * archive, char *name){
    /* handle null pointer */
    if (!archive || !name) {
        error("group_create: null pointer");
    }

    Group * group = malloc(sizeof(Group));
    if(!group){
        error("could not allocate group, %s", name);
    }
    
    group->archive = archive;
    group->creation_date = (uint32_t) time(NULL);
    group->last_modification_date = group->creation_date;
    group->entries = vector_create();
    group->fields = vector_create();
    group->group_id = (archive->groups->size);
    group->name = name;
    
    // add the group to vectors
    vector_push_back(archive->groups, group);
    
    group_update_parents(group);
    
    return group;
}


ErrorCode group_destroy(Group *group){

    if(!group){
        errorp("group_delete: null pointer");
        return NULL_POINTER;
    }

    // destroying the group
    vector_destroy(group->entries);
    vector_destroy(group->fields);
    
    // free(group->name);
    free(group);
    return SUCCESS;
}

ErrorCode group_delete(Group *group) {
    if (!group) {
        errorp("group_delete: null pointer");
        return NULL_POINTER;
    }
    ErrorCode status = SUCCESS;

    status += (vector_remove_value(group->archive->groups, group)) == NULL; // if vector_remove_value returns null this means it failed.

    /* delete all entries which handles deleting fields */
    Vector * entries_to_delete = vector_copy(group->entries);
    for (size_t i=0; i<entries_to_delete->size; i++) {
        entry_delete(vector_at(entries_to_delete, i));
    }
    vector_destroy(entries_to_delete);

    status+= group_update(group);
    status+= group_update_parents(group);

    status += group_destroy(group);

    return (status !=SUCCESS)? FAILURE: SUCCESS;
}

char * group_to_string(Group * group) {
    if(!group) {
        error("group_to_string: null pointer.");
    }
    char *string;
    asprintf(&string, "Group ID: %u, Group Name:%s, num of fields: %lu, num of entries: %lu", group->group_id, group->name, group->fields->size, group->entries->size);
    return string;
}

/* mutators */

ErrorCode group_update_parents(Group *group){
    if(!group){
        errorp("group_update_parents: null pointer");
        return NULL_POINTER;
    }
    group->archive->last_modification_date = group->last_modification_date;
    group->archive->written = 0;
    search_tree_gen(group->archive);
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