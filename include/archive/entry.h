#ifndef ENTRY_H
#define ENTRY_H

#include <stdint.h>

typedef struct Group Group;
typedef struct Field Field;
typedef struct Vector Vector;

typedef struct Entry{
    uint32_t creation_date;
    uint32_t last_modification_date;
    Group * group;
    uint32_t entry_id;
    char* name;

    Vector *fields;

} Entry;


/* Functions */

Entry * entry_create(Group *group, char* name);
void entry_destroy(Entry * entry);

/* brief info about entry */
char * entry_to_string(Entry * entry);

/* mutators */
void entry_update_parents(Entry *entry);
void entry_set_name(Entry *entry, char *name);
void entry_set_group(Entry *entry, Group *new_group);
void entry_delete(Entry *entry);
#endif