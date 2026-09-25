#ifndef FIELD_H
#define FIELD_H

#include "../types.h"
#include <stdint.h>
#include <stdlib.h>

typedef struct Entry Entry;
typedef struct Archive Archive;
typedef struct Group Group;

typedef enum
{
    TEXT = 0,
    PASSWORD = 1,
    BINARY = 2, // file
} FieldType;

typedef enum
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
    uint64_t compressed_size_offset; /* size section offset in the local header. required for writing the local header.
    * this variable is not needed in writing the field header in the directory as the field is already written and it's size is calculated. */
    uint32_t crc;
    uint32_t creation_date;
    uint32_t last_modification_date;
    uint32_t number_of_changes; // number of updates to the field.
    char* name;
    void * content; // should either string or file pointer.
    FieldType type; //
    CompressionType compression;

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