#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "../types.h"
#include "../utils/vector.h"
#include "group.h"
#include "entry.h"
#include"field.h"

/* Archive current version */
#define ARCHIVE_VERSION 0

/* Sizes of file elements in Bytes. */
#define MAGIC_SIZE 4    // size of the magic number `0x4D4D33`.
#define FILE_VERSION_SIZE 2 // size of the file version integer.
#define OFFSET_SIZE 8   // size of an offset or a size.
#define DATE_SIZE 4     // size of date (stored in unix format).
#define ID_SIZE 4    // size of (Group ID, Entry ID and number of groups, etc..)
#define STRING_LENGTH_SIZE 2    // size of (name length, comment length) or any string stored.
#define CRC_SIZE 4   // size of CRC
#define TYPE_SIZE 2 // size of (field type, compression type, etc...)

/* Boundaries */
#define MAX_CHANGES 4294967296 // maximum number of archive directories, for each change in archive a new directory is written.

/* Magic Number */
#define MAGIC_NUMBER  0x4D4D3333  /* MM33 */

/* functions to calculate size of parts that have variable length */
uint64_t group_header_size (Group * group);
uint64_t entry_header_size(Entry * entry);
uint64_t field_header_size(Field * field);
uint64_t local_field_header_size(Field * field);
uint64_t field_size(Field * field);
uint64_t directory_size(Archive * archive);
    

typedef struct Archive{

    uint64_t directory_offset;
    uint64_t prev_dir_offset; /* the offset of the previous directory. This field is used for file history. */
    uint64_t size; // the offset of the last byte.
    FILE *fp;
    Vector *groups;
    Vector *entries;
    Vector *fields;
    /* vectors to collect entities that have changes to be written. */
    Vector *groups_updated;
    // Vector *entries_updated;/* groups & entries are not necessary as there info would be written in the directory in each change. */
    // Vector *fields_updated;
    uint32_t num_of_changes; /* represents number of directories in the archive */
    char* name;
    char* description;
    uint16_t version;
    uint16_t num_of_entries;
    uint16_t num_of_groups;
    uint8_t written; /* a boolean value indicates if this version or current changes is written into the archive */

} Archive;

/* #TODO: Memory Functions*/

Archive * archive_create(char *name, char *description);
void archive_destroy(Archive * archive);

/* #TODO: Format functions */
ErrorCode write_group_header(Group * group); /* write group directory header. */
ErrorCode write_entry_header(Entry * entry); /* write entry directory header. */
ErrorCode write_field_header(Field * field); /* write field directory header. */
ErrorCode write_directory(Archive * archive);
ErrorCode write_field_local_header(Archive * archive, Field * field); /* write field local header. */
ErrorCode write_field(Archive * archive, Field * field); /* write both local header & field content */
ErrorCode write_archive(Archive * archive); /* create or update archive content. */
ErrorCode write_archive_clean(Archive * archive, char *new_file_name); /* rewrite archive with clean history. */

#endif