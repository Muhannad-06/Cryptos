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
    uint16_t group_id; /* #TODO: use uint32_t*/
    char* name;
    

} Group ;

/* #TODO: Functions*/

Group * group_create(Archive *archive, char *name);
void group_destroy(Group * group);

#endif