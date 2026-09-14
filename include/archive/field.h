#ifndef FIELD_H
#define FIELD_H

#include "../types.h"
#include <stdint.h>
#include <stdlib.h>

typedef struct Entry Entry;
typedef struct Archive Archive;
typedef struct Group Group;

typedef enum : uint16_t
{
    TEXT = 0,
    PASSWORD = 1,
    BINARY = 2, // file
} FieldType;

typedef enum : uint16_t
{
    NON_COMPRESSED = 0,
    DEFLATE = 1,
} CompressionType;

// typedef enum : uint16_t
// {
//     NON_ENCRYPTED = 0,
//     ENCRYPTED = 1,
// } EncryptionType;

typedef struct Field{
    Entry * entry;

    uint64_t offset;
    uint64_t compressed_size; // 0 for file with no compression.
    uint64_t size;
    uint64_t full_size;
    uint32_t crc;
    uint32_t creation_date;
    uint32_t last_modification_date;
    uint32_t number_of_changes; // number of updates to the field.
    void * content; // should either string or file pointer.
    FieldType type; //
    CompressionType compression;

    char* name;
} Field ;

/* functions */

Field * field_create(Entry *entry, FieldType type, char* name);
void field_destroy(Field *field);

/* brief info about field */
char * field_to_string(Field * field);

/* format functions */
uint32_t field_crc(Field *field);

#endif

/* mutators */
ErrorCode field_update_parents(Field * field);
ErrorCode field_update(Field *field);
// void field_add(Entry *entry, FieldType type, char *name);
ErrorCode field_set_name(Field *field, char * name);
ErrorCode field_set_content(Field *field, FieldType type, void * data);
ErrorCode field_set_compression(Field *field, CompressionType compression_type);
// void field_set_encryption(Field *field, EncryptionType encryption_type);
ErrorCode field_set_entry(Field *field, Entry *new_entry);
// void field_set_group(Field *field, Group *new_group);  // changing entry is enough.
ErrorCode field_delete(Field *field);

/* #TODO: add encryption type section to:
* Archive structure.
* field struct.
* size functions.
* write & read functions.
*/