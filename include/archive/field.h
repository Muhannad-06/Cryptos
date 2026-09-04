#ifndef FIELD_H
#define FIELD_H

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

typedef struct Field{
    Entry * entry;

    uint64_t offset;
    uint64_t compressed_size; // 0 for file with no compression.
    uint64_t size;
    uint32_t crc;
    uint32_t creation_date;
    uint32_t last_modification_date;
    uint32_t number_of_changes; // number of updates to the field.
    void * content; // should either string or file pointer.
    FieldType type; //
    CompressionType compression;

    char* name;
} Field ;

/* #TODO: functions */

Field * field_create(Entry *entry,uint64_t offset ,uint16_t type, char* name);
void field_destroy(Field *field);
uint32_t field_crc(Field *field);

#endif