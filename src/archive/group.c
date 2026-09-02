#include "../../include/archive/archive.h"
#include <stdint.h>
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