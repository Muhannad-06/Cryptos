#include "../archive_m/archive.h"
#include <stdint.h>

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

/* functions to calculate size of parts that have variable length */
uint64_t group_header_size (Group * group);
uint64_t entry_header_size(Entry * entry);
uint64_t field_header_size(Field * field);
uint64_t local_field_header_size(Field * field);
uint64_t field_size(Field * field);
uint64_t directory_size(Archive * archive);
    