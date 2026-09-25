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
#include "../utils/bst.h"

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
#define MAGIC_NUMBER  0x33334D4D /* in file it is reversed by little endian to be MM33 */

/* functions to calculate size of parts that have variable length */
uint64_t group_header_size (Group * group);
uint64_t entry_header_size(Entry * entry);
uint64_t field_header_size(Field * field);
uint64_t local_field_header_size(Field * field);
uint64_t field_full_size(Field * field);
uint64_t directory_size(Archive * archive);

/* archive size calculation */
uint64_t archive_size(Archive * archive);
    

typedef struct Archive{

    uint64_t directory_offset;
    uint64_t prev_dir_offset; /* the offset of the previous directory. This field is used for file history. */
    uint64_t size; // the offset of the last byte.
    FILE *fp;
    Vector *groups;
    Vector *entries;
    Vector *fields;
    uint32_t creation_date;
    uint32_t last_modification_date;
    Vector *fields_updated; /* holds fields to be written */
    uint32_t num_of_changes; /* represents number of directories in the archive */
    /* #NOTE: num_of_changes should be increased before each archive write operation. 
    * It is increased in the write_archive function.
    */
    char* name;
    char* description;
    BST* search_tree;
    uint16_t version;
    uint8_t written; /* a boolean value indicates if this version or current changes is written into the archive */

} Archive;

/* Memory Functions*/

Archive * archive_create(char *name, char *description, FILE *fp);
void archive_destroy(Archive * archive);
ErrorCode search_tree_gen(Archive *archive);
TNode *archive_find_entity(Archive *archive, char *entity_name);
char *archive_find_entity_str(Archive *archive, char *entity_name);

/* brief info about archive */
char * archive_to_string(Archive * archive);

/*  Format functions */

/*  write functions. */
ErrorCode write_group_header(Group *group);  /* write group directory header. */
ErrorCode write_entry_header(Entry *entry);  /* write entry directory header. */
ErrorCode write_field_header(Field * field); /* write field directory header. */
ErrorCode write_directory(Archive * archive);
ErrorCode write_field_local_header(Field * field); /* write field local header. */
ErrorCode write_field(Field * field); /* write both local header & field content */
ErrorCode write_archive(Archive * archive); /* create or update archive content. */
ErrorCode write_archive_clean(Archive * archive, char *new_file_name); /* rewrite archive with clean history. */

/* #TODO: read functions. */
Group * read_group_header(Archive * archive, FILE *fp, uint64_t offset);
Group ** read_group_headers(Archive * archive, FILE *fp, uint64_t n_groups_offset, uint32_t *out_count); 
Entry * read_entry_header(Archive * archive, FILE *fp, uint64_t offset);
Entry ** read_entry_headers(Archive * archive, FILE *fp, uint64_t n_entries_offset, uint32_t *out_count);
Field * read_field_header(Archive * archive, FILE *fp, uint64_t offset);
Field ** read_field_headers(Archive * archive, FILE *fp, uint64_t n_fields_offset, uint32_t *out_count); 
ErrorCode read_directory(Archive *archive, FILE *fp, uint64_t directory_offset);
ErrorCode read_archive(Archive *archive, FILE *fp);
ErrorCode print_field_data(Field * field, FILE * stream); // print field data into stream.

/* History functions. */
Archive * archive_get_backward(Archive * archive, uint32_t steps);
Vector * archive_get_versions(Archive *archive);

/* mutators */
ErrorCode archive_set_name(Archive *archive, char *name);
ErrorCode archive_set_description(Archive *archive, char *description);
#endif