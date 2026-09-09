#ifndef GROUP_H
#define GROUP_H

#include <stdint.h>

typedef struct Archive Archive;
typedef struct Entry Entry;
typedef struct Field Field;
typedef struct Vector Vector;


typedef struct Group{
    Archive * archive;
    uint32_t creation_date;
    uint32_t last_modification_date;
    Vector *entries;
    Vector *fields; /* This vector is added for easier iteration. */
    uint32_t group_id;
    char* name;
    
} Group ;

/* Functions */

Group * group_create(Archive *archive, char *name);
void group_destroy(Group * group);

/* brief info about group */
char * group_to_string(Group * group);

/* mutators */
void group_update_parents(Group *group);
void group_set_name(Group *group, char *name);
void group_delete(Group *group);

#endif