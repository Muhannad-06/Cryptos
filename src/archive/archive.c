#include "../../include/archive/archive.h"
#include "../../include/io/Bin_IO.h"
#include "../../include/utils/error.h"
#include "../../include/utils/crc.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* memory functions. */
Archive * archive_create(char *name, char *description){
    Archive * archive = malloc(sizeof(Archive));
    if(!archive) return NULL;
    
    archive->creation_date = (uint64_t) time(NULL);
    archive->directory_offset = sizeof(uint32_t) + sizeof(uint64_t); /* magic 4 bytes + the offset location section */
    archive->fp = malloc(sizeof(FILE)); // creating file pointer, opening the file and writing data is in archive_f
    archive->groups = vector_create();
    archive->entries = vector_create();
    archive->fields = vector_create();
    archive->name = name;
    archive->description = description;
    archive->version = ARCHIVE_VERSION /* current project version */ ; 
    archive->num_of_entries = 0;
    archive->num_of_groups = 0; 
    
    /* Creating the default group (root) */
    Group * root_group = group_create(archive, "root"); // increases num_of_groups by 1.

    /* archive size = archive header + directory size*/
    archive->size = MAGIC_SIZE + OFFSET_SIZE + directory_size(archive) ; 
    
    return archive;
}

Archive * archive_clean_history(Archive * archive){
    // #TODO
    return NULL;
}

/* archive format functions */

/* dynamic size calculation functions */

uint64_t group_header_size(Group *group){
    return MAGIC_SIZE +
        ID_SIZE +
        DATE_SIZE*2 +
        STRING_LENGTH_SIZE +
        (uint64_t) sizeof(group->name);
}

uint64_t entry_header_size(Entry *entry){
    return MAGIC_SIZE+
        ID_SIZE*2 +
        DATE_SIZE*2 +
        STRING_LENGTH_SIZE +
        (uint64_t) sizeof(entry->name);
}

uint64_t field_header_size(Field *field){
    return MAGIC_SIZE +
        ID_SIZE +
        TYPE_SIZE*2 +
        OFFSET_SIZE*3 +
        CRC_SIZE +
        DATE_SIZE*2 +
        STRING_LENGTH_SIZE +
        (uint64_t) sizeof(field->name);
}

uint64_t local_field_header_size(Field *field){
    return MAGIC_SIZE +
        ID_SIZE +
        TYPE_SIZE*2 +
        OFFSET_SIZE*2 +
        CRC_SIZE +
        DATE_SIZE*2 +
        STRING_LENGTH_SIZE +
        (uint64_t) sizeof(field->name);
}

uint64_t directory_size(Archive *archive){
    uint64_t group_headers_size= 0;
    uint64_t entry_headers_size= 0;
    uint64_t field_headers_size= 0;

    for(int i = 0; i<(archive->fields->size); i++){
        field_headers_size += field_header_size(archive->fields->data[i]);
    }

    for(int i = 0; i<(archive->groups->size); i++){
        group_headers_size += group_header_size(archive->groups->data[i]);
    }

    for(int i = 0; i<(archive->entries->size); i++){
        entry_headers_size += entry_header_size(archive->entries->data[i]);
    }

    return MAGIC_SIZE +
        FILE_VERSION_SIZE +
        OFFSET_SIZE +
        DATE_SIZE*2 +
        ID_SIZE*3 +
        STRING_LENGTH_SIZE*2 +
        (uint64_t) sizeof(archive->name) +
        (uint64_t) sizeof(archive->description) +
        group_headers_size +
        entry_headers_size +
        field_headers_size;
}

/* writing functions */

ErrorCode write_group_header(Group * group){
    int status = SUCCESS;
    Archive *archive = group->archive;
    FILE *fp = archive->fp;
    /* check if file is open and appendable. */
    if(fp == NULL){
        error("could not write group header for group: %s. file not open.", group->name);
        return FAILURE;
    }

        /* seek to the end of file and write magic number. */
    if(IO_enumSeek(fp, group->archive->size) != IO_enumWriteU32(group->archive->fp, MAGIC_NUMBER) != SUCCESS){
        error("could not write group header for group: %s. could not append to file.", group->name);
        return FAILURE;
    }
    
    /* writing header content */

    status += IO_enumWriteU32(fp, group->group_id); // writing group ID
    status += IO_enumWriteU32(fp, group->creation_date);
    status += IO_enumWriteU32(fp, group->last_modification_date);
    status += IO_enumWriteU16(fp, (uint16_t) (strlen(group->name) + 1));
    status += IO_enumWriteString(fp, group->name);
    
    return (status != SUCCESS)? FAILURE : SUCCESS; // return success or failure.
}

ErrorCode write_entry_header(Entry * entry){
    int status = SUCCESS;
    Archive *archive = entry->group->archive;
    FILE *fp = archive->fp;
    /* check if file is open and appendable. */
    if(fp == NULL){
        error("could not write entry header for entry: %s. file not open.", entry->name);
        return FAILURE;
    }

        /* seek to the end of file and write magic number. */
    if(IO_enumSeek(fp,archive->size) != IO_enumWriteU32(fp, MAGIC_NUMBER) != SUCCESS){
        error("could not write entry header for entry: %s. could not append to file.", entry->name);
        return FAILURE;
    }
    
    /* writing header content */    

    status += IO_enumWriteU32(fp, entry->group->group_id); // writing group ID
    status += IO_enumWriteU32(fp, entry->entry_id); // writing entry ID
    status += IO_enumWriteU32(fp, entry->creation_date);
    status += IO_enumWriteU32(fp, entry->last_modification_date);
    status += IO_enumWriteU16(fp, (uint16_t) (strlen(entry->name)+1));
    status += IO_enumWriteString(fp, entry->name);
    
    return (status != SUCCESS)? FAILURE : SUCCESS; // return success or failure.
}

ErrorCode write_field_header(Field * field){
    int status = SUCCESS;
    Archive *archive = field->entry->group->archive;
    FILE *fp = archive->fp;
    /* check if file is open and appendable. */
    if(fp == NULL){
        error("could not write field header for field: %s. file not open.", field->name);
        return FAILURE;
    }

        /* seek to the end of file and write magic number. */
    if(IO_enumSeek(fp,archive->size) != IO_enumWriteU32(fp, MAGIC_NUMBER) != SUCCESS){
        error("could not write field header for field: %s. could not append to file.", field->name);
        return FAILURE;
    }
    
    /* writing header content */    

    status += IO_enumWriteU32(fp, field->entry->entry_id); // writing entry ID
    status += IO_enumWriteU16(fp, field->compression);
    status += IO_enumWriteU64(fp, field->compressed_size);
    status += IO_enumWriteU64(fp, field->size);
    status += IO_enumWriteU64(fp, field->offset);
    status += IO_enumWriteU32(fp, field_crc(field));
    status += IO_enumWriteU32(fp, field->creation_date);
    status += IO_enumWriteU32(fp, field->last_modification_date);
    status += IO_enumWriteU16(fp, field->type);
    status += IO_enumWriteU16(fp, (uint16_t) (strlen(field->name)+1));
    status += IO_enumWriteString(fp, field->name);
    
    return (status != SUCCESS)? FAILURE : SUCCESS; // return success or failure.
}

ErrorCode write_directory(Archive *archive){

    if(archive->written){
        error("File already updated.");
        return FAILURE; 
    }

    int status = SUCCESS;
    FILE *fp = archive->fp;
    /* check if file is open and appendable. */
    if(fp == NULL){
        error("could not write drectory: %s. file not open.", archive->name);
        return FAILURE;
    }

        /* seek to the end of file and write magic number. */
    if(IO_enumSeek(fp,archive->size) != IO_enumWriteU32(fp, MAGIC_NUMBER) != SUCCESS){
        error("could not write file directory: %s. could not append to file.", archive->name);
        return FAILURE;
    }
    
    // /* updating archive size. */
    // archive->size = archive_calculate_size(archive);
    /* size updating should be handled before writing!!! */

    /* writing directory header. */
    status += IO_enumWriteU16(fp, archive->version);
    status += IO_enumWriteU64(fp, archive->size); // new size is calculated before writing... 
    status += IO_enumWriteU32(fp, archive->creation_date);
    status += IO_enumWriteU32(fp, archive->last_modification_date);
    status += IO_enumWriteU32(fp, archive->num_of_changes);
    /* updating directory offset. */
    status += IO_enumWriteU32(fp, archive->directory_offset);
    archive->prev_dir_offset = archive->directory_offset;
    archive->directory_offset = archive->size + 1;

    /* writing groups headers. */
    status+= IO_enumWriteU32(fp, archive->num_of_groups);
    for(int i = 0; i < archive->num_of_groups; i++){
        status += write_group_header(vector_at(archive->groups, i));
    }
    /* writing entries headers. */
    status+= IO_enumWriteU32(fp, archive->num_of_entries);
    for(int i = 0; i < archive->num_of_entries; i++){
        status += write_entry_header(vector_at(archive->entries, i));
    }
    /* writing fields headers*/
    status+= IO_enumWriteU32(fp, archive->fields->size);
    for(int i = 0; i < archive->fields->size; i++){
        status += write_field_header(vector_at(archive->fields, i));
    }

    /* writing archive name */
    status+= IO_enumWriteU16(fp, strlen(archive->name) + 1); // added 1 for the null character.
    status+= IO_enumWriteString(fp, archive->name); // added 1 for the null character.
    
    /* writing archive description */
    status+= IO_enumWriteU16(fp, strlen(archive->description) + 1); // added 1 for the null character.
    status+= IO_enumWriteString(fp, archive->description); // added 1 for the null character.
    
    /* writing the directory's offset at the end of the file. */
    status += IO_enumWriteU64(fp, archive->directory_offset);

    /* indicate that file is new data is written */
    archive->written = 1;

    return (status != SUCCESS)? FAILURE : SUCCESS; // return success or failure.
}

ErrorCode write_field_local_header(Field * field){
    int status = SUCCESS;
    Archive *archive = field->entry->group->archive;
    FILE *fp = archive->fp;
    /* check if file is open and appendable. */
    if(fp == NULL){
        error("could not write field local header for field: %s. file not open.", field->name);
        return FAILURE;
    }

        /* seek to the end of file and write magic number. */
    if(IO_enumSeek(fp,archive->size) != IO_enumWriteU32(fp, MAGIC_NUMBER) != SUCCESS){
        error("could not write field local header for field: %s. could not append to file.", field->name);
        return FAILURE;
    }
    
    /* writing header content */    

    status += IO_enumWriteU32(fp, field->entry->entry_id); // writing entry ID
    status += IO_enumWriteU16(fp, field->compression);
    status += IO_enumWriteU64(fp, field->compressed_size);
    status += IO_enumWriteU64(fp, field->size);
    status += IO_enumWriteU32(fp, field_crc(field));
    status += IO_enumWriteU32(fp, field->creation_date);
    status += IO_enumWriteU32(fp, field->last_modification_date);
    status += IO_enumWriteU16(fp, field->type);
    status += IO_enumWriteU16(fp, (uint16_t) (strlen(field->name)+1) );
    status += IO_enumWriteString(fp, field->name);
    
    return (status != SUCCESS)? FAILURE : SUCCESS; // return success or failure.
}

ErrorCode write_field(Field * field){ // #TODO: pass the archive pointer directly.
    // #TODO pass the archive pointer to the write header functions
    Archive *archive = field->entry->group->archive;
    FILE *fp = archive->fp;
    int status = write_field_header(field);

    if(field->compression == NON_COMPRESSED){
        if(field->type == TEXT || field->type == PASSWORD){
            status += IO_enumWriteString(fp, (char *) (field->content));
        } else if(field->type == BINARY){
            status += IO_enumWriteFile(fp, (FILE *) (field->content));
        }
    } else if(field->compression == DEFLATE){
        // #TODO: write with deflate algorithm.
    } else {
        error("could not write field. invalid compression method in field: %s", field->name);
        return FAILURE;
    }
    
    // remove from updated list.
    vector_remove_value(archive->fields_updated, field);

    return (status != SUCCESS)? FAILURE : SUCCESS; // return success or failure.
}

ErrorCode write_archive(Archive *archive){
    int status = SUCCESS;
    // iterate through all updated fields and write them.
    // copy vector (as the main vector alters during the process).
    Vector fields_to_write = *vector_copy(archive->fields_updated);
    for (int i = 0; i < fields_to_write.size ; i++) {
        write_field(vector_at(&fields_to_write, i));
    }
    vector_destroy(&fields_to_write);
    // write directory.
    status += write_directory(archive);
    return (status != SUCCESS)? FAILURE : SUCCESS; // return success or failure.
}

ErrorCode write_archive_clean(Archive * archive, char *new_file_name){
    int status = SUCCESS;
    // #TODO: write new archive with clean history.
    return (status != SUCCESS)? FAILURE : SUCCESS; // return success or failure.
}


/* <<<<<<<<<<<<<<<<<<<<<<<<< Read functions >>>>>>>>>>>>>>>>>>>>>> */
Group *read_group_header(Archive *archive, FILE *fp, uint64_t offset)
{
    Group *group;
    uint32_t group_id = 0, creation = 0, last_mod = 0;
    char *name = NULL;
 
    if (!archive || !fp) {
        errorp("could not read group header. NULL pointer.");
        return NULL;
    }
 
    if (IO_enumSeek(fp, offset) != SUCCESS) {
        errorp("could not seek to group header at offset %llu.",
               (unsigned long long)offset);
        return NULL;
    }
 
    if (expect_magic(fp, "group header") != SUCCESS)
        return NULL;
 
    if (IO_enumReadU32(fp, &group_id)  != SUCCESS ||
        IO_enumReadU32(fp, &creation)  != SUCCESS ||
        IO_enumReadU32(fp, &last_mod)  != SUCCESS) {
        errorp("could not read group header body.");
        return NULL;
    }
 
    if (read_prefixed_string(fp, &name) != SUCCESS) {
        errorp("could not read group name.");
        return NULL;
    }
 
    /* group_create() registers the group in archive->groups and assigns
     * a fresh id; the id from disk overrides it. */
    group = group_create(archive, name);
    if (!group) {
        free(name);
        errorp("could not allocate group while reading.");
        return NULL;
    }
 
    group->group_id = group_id;
    group->creation_date = creation;
    group->last_modification_date = last_mod;
 
    return group;
}
 
Group **read_group_headers(Archive *archive, FILE *fp,
                           uint64_t n_groups_offset, uint32_t *out_count)
{
    Group **groups;
    uint32_t n = 0, i;
 
    if (out_count) *out_count = 0;
    if (!archive || !fp) return NULL;
 
    if (IO_enumSeek(fp, n_groups_offset) != SUCCESS) {
        errorp("could not seek to the number of groups.");
        return NULL;
    }
    if (IO_enumReadU32(fp, &n) != SUCCESS) {
        errorp("could not read the number of groups.");
        return NULL;
    }
    if (n == 0) {
        archive->num_of_groups = 0;
        return NULL;
    }
 
    groups = malloc(sizeof(Group *) * n);
    if (!groups) {
        errorp("could not allocate the groups array.");
        return NULL;
    }
 
    for (i = 0; i < n; i++) {
        /* each header is variable length, so the cursor position after
         * the previous record IS the offset of the next one. */
        Group *group = read_group_header(archive, fp, IO_u64Tell(fp));
        if (!group) {
            errorp("could not read group header #%u.", i);
            free(groups);
            return NULL;
        }
        groups[i] = group;
    }
 
    archive->num_of_groups = n;
    if (out_count) *out_count = n;
    return groups;
}
 
Entry *read_entry_header(Archive *archive, FILE *fp, uint64_t offset)
{
    Entry *entry;
    Group *group;
    uint32_t group_id = 0, entry_id = 0, creation = 0, last_mod = 0;
    char *name = NULL;
 
    if (!archive || !fp) {
        errorp("could not read entry header. NULL pointer.");
        return NULL;
    }
 
    if (IO_enumSeek(fp, offset) != SUCCESS) {
        errorp("could not seek to entry header at offset %llu.",
               (unsigned long long)offset);
        return NULL;
    }
 
    if (expect_magic(fp, "entry header") != SUCCESS)
        return NULL;
 
    if (IO_enumReadU32(fp, &group_id) != SUCCESS ||
        IO_enumReadU32(fp, &entry_id) != SUCCESS ||
        IO_enumReadU32(fp, &creation) != SUCCESS ||
        IO_enumReadU32(fp, &last_mod) != SUCCESS) {
        errorp("could not read entry header body.");
        return NULL;
    }
 
    if (read_prefixed_string(fp, &name) != SUCCESS) {
        errorp("could not read entry name.");
        return NULL;
    }
 
    /* resolve the owning group; groups must be parsed first. */
    group = archive_find_group(archive, group_id);
    if (!group) {
        errorp("entry %u references unknown group %u.", entry_id, group_id);
        free(name);
        return NULL;
    }
 
    entry = entry_create(group, name);
    if (!entry) {
        free(name);
        errorp("could not allocate entry while reading.");
        return NULL;
    }
 
    entry->entry_id = entry_id;
    entry->creation_date = creation;
    entry->last_modification_date = last_mod;
 
    return entry;
}
 
Entry **read_entry_headers(Archive *archive, FILE *fp,
                           uint64_t n_entries_offset, uint32_t *out_count)
{
    Entry **entries;
    uint32_t n = 0, i;
 
    if (out_count) *out_count = 0;
    if (!archive || !fp) return NULL;
 
    if (IO_enumSeek(fp, n_entries_offset) != SUCCESS) {
        errorp("could not seek to the number of entries.");
        return NULL;
    }
    if (IO_enumReadU32(fp, &n) != SUCCESS) {
        errorp("could not read the number of entries.");
        return NULL;
    }
    if (n == 0) {
        archive->num_of_entries = 0;
        return NULL;
    }
 
    entries = malloc(sizeof(Entry *) * n);
    if (!entries) {
        errorp("could not allocate the entries array.");
        return NULL;
    }
 
    for (i = 0; i < n; i++) {
        Entry *entry = read_entry_header(archive, fp, IO_u64Tell(fp));
        if (!entry) {
            errorp("could not read entry header #%u.", i);
            free(entries);
            return NULL;
        }
        entries[i] = entry;
    }
 
    archive->num_of_entries = n;
    if (out_count) *out_count = n;
    return entries;
}
 
Field *read_field_header(Archive *archive, FILE *fp, uint64_t offset)
{
    Field *field;
    Entry *entry;
    uint32_t entry_id = 0, crc = 0, creation = 0, last_mod = 0;
    uint16_t compression = 0, type = 0;
    uint64_t compressed_size = 0, size = 0, field_offset = 0;
    char *name = NULL;
 
    if (!archive || !fp) {
        errorp("could not read field header. NULL pointer.");
        return NULL;
    }
 
    if (IO_enumSeek(fp, offset) != SUCCESS) {
        errorp("could not seek to field header at offset %llu.",
               (unsigned long long)offset);
        return NULL;
    }
 
    if (expect_magic(fp, "field header") != SUCCESS)
        return NULL;
 
    if (IO_enumReadU32(fp, &entry_id)        != SUCCESS ||
        IO_enumReadU16(fp, &compression)     != SUCCESS ||
        IO_enumReadU64(fp, &compressed_size) != SUCCESS ||
        IO_enumReadU64(fp, &size)            != SUCCESS ||
        IO_enumReadU64(fp, &field_offset)    != SUCCESS ||
        IO_enumReadU32(fp, &crc)             != SUCCESS ||
        IO_enumReadU32(fp, &creation)        != SUCCESS ||
        IO_enumReadU32(fp, &last_mod)        != SUCCESS ||
        IO_enumReadU16(fp, &type)            != SUCCESS) {
        errorp("could not read field header body.");
        return NULL;
    }
 
    if (read_prefixed_string(fp, &name) != SUCCESS) {
        errorp("could not read field name.");
        return NULL;
    }
 
    if (type != TEXT && type != PASSWORD && type != BINARY) {
        errorp("field '%s' has an unknown type %u.", name, type);
        free(name);
        return NULL;
    }
    if (compression != NON_COMPRESSED && compression != DEFLATE) {
        errorp("field '%s' has an unknown compression method %u.", name, compression);
        free(name);
        return NULL;
    }
 
    /* resolve the owning entry; entries must be parsed first. */
    entry = archive_find_entry(archive, entry_id);
    if (!entry) {
        errorp("field '%s' references unknown entry %u.", name, entry_id);
        free(name);
        return NULL;
    }
 
    /* field_create() registers the field in entry/group/archive vectors.
     * Its own offset bookkeeping is overwritten right after. */
    field = field_create(entry, field_offset, type, name);
    if (!field) {
        free(name);
        errorp("could not allocate field while reading.");
        return NULL;
    }
 
    field->name = name;
    field->offset = field_offset;
    field->compression = (CompressionType)compression;
    field->compressed_size = compressed_size;
    field->size = size;
    field->crc = crc;
    field->creation_date = creation;
    field->last_modification_date = last_mod;
    field->type = (FieldType)type;
    field->content = NULL;   /* content stays on disk until asked for. */
 
    return field;
}
 
Field **read_field_headers(Archive *archive, FILE *fp,
                           uint64_t n_fields_offset, uint32_t *out_count)
{
    Field **fields;
    uint32_t n = 0, i;
 
    if (out_count) *out_count = 0;
    if (!archive || !fp) return NULL;
 
    if (IO_enumSeek(fp, n_fields_offset) != SUCCESS) {
        errorp("could not seek to the number of fields.");
        return NULL;
    }
    if (IO_enumReadU32(fp, &n) != SUCCESS) {
        errorp("could not read the number of fields.");
        return NULL;
    }
    if (n == 0)
        return NULL;
 
    fields = malloc(sizeof(Field *) * n);
    if (!fields) {
        errorp("could not allocate the fields array.");
        return NULL;
    }
 
    for (i = 0; i < n; i++) {
        Field *field = read_field_header(archive, fp, IO_u64Tell(fp));
        if (!field) {
            errorp("could not read field header #%u.", i);
            free(fields);
            return NULL;
        }
        fields[i] = field;
    }
 
    if (out_count) *out_count = n;
    return fields;
}

ErrorCode read_directory(Archive *archive, FILE *fp, uint64_t directory_offset)
{
    uint16_t version = 0;
    uint64_t archive_size = 0, prev_dir_offset = 0;
    uint32_t creation = 0, last_mod = 0, num_of_changes = 0;
    uint32_t n_groups = 0, n_entries = 0, n_fields = 0;
    char *name = NULL, *description = NULL;
    Group **groups = NULL;
    Entry **entries = NULL;
    Field **fields = NULL;
 
    if (!archive || !fp) {
        errorp("could not read directory. NULL pointer.");
        return NULL_POINTER;
    }
 
    if (IO_enumSeek(fp, directory_offset) != SUCCESS) {
        errorp("could not seek to the directory at offset %llu.",
               (unsigned long long)directory_offset);
        return ERROR_READ_FAILED;
    }
 
    if (expect_magic(fp, "directory") != SUCCESS)
        return ERROR_READ_FAILED;
 
    if (IO_enumReadU16(fp, &version)         != SUCCESS ||
        IO_enumReadU64(fp, &archive_size)    != SUCCESS ||
        IO_enumReadU32(fp, &creation)        != SUCCESS ||
        IO_enumReadU32(fp, &last_mod)        != SUCCESS ||
        IO_enumReadU32(fp, &num_of_changes)  != SUCCESS ||
        IO_enumReadU64(fp, &prev_dir_offset) != SUCCESS) {
        errorp("could not read the directory header.");
        return ERROR_READ_FAILED;
    }
 
    if (version > ARCHIVE_VERSION) {
        errorp("archive file version %u is newer than the supported version %u.",
               version, (unsigned)ARCHIVE_VERSION);
        return FAILURE;
    }
 
    /* Start from a clean object: the caller may hand us an archive that
     * still holds the default root group or an older directory. */
    archive_reset(archive);
 
    archive->fp = fp;
    archive->directory_offset = directory_offset;
    archive->prev_dir_offset = prev_dir_offset;
    archive->version = version;
    archive->creation_date = creation;
    archive->last_modification_date = last_mod;
    archive->num_of_changes = num_of_changes;
 
    /* The three sections are stored back to back, so after each call the
     * cursor already sits on the next "number of X" counter. */
    groups = read_group_headers(archive, fp, IO_u64Tell(fp), &n_groups);
    if (n_groups > 0 && !groups) {
        errorp("could not read the group headers.");
        return ERROR_READ_FAILED;
    }
 
    entries = read_entry_headers(archive, fp, IO_u64Tell(fp), &n_entries);
    if (n_entries > 0 && !entries) {
        errorp("could not read the entry headers.");
        free(groups);
        return ERROR_READ_FAILED;
    }
 
    fields = read_field_headers(archive, fp, IO_u64Tell(fp), &n_fields);
    if (n_fields > 0 && !fields) {
        errorp("could not read the field headers.");
        free(groups);
        free(entries);
        return ERROR_READ_FAILED;
    }
 
    /* the lookup arrays were only needed to detect failures. */
    free(groups);
    free(entries);
    free(fields);
 
    if (read_prefixed_string(fp, &name) != SUCCESS) {
        errorp("could not read the archive name.");
        return ERROR_READ_FAILED;
    }
    if (read_prefixed_string(fp, &description) != SUCCESS) {
        errorp("could not read the archive description.");
        free(name);
        return ERROR_READ_FAILED;
    }
 
    free(archive->name);
    free(archive->description);
    archive->name = name;
    archive->description = description;
 
    /* Set last, because group_create()/field_create() touch these while
     * the records are being parsed. */
    archive->num_of_groups = n_groups;
    archive->num_of_entries = n_entries;
    archive->size = archive_size;
    archive->written = 1;   /* nothing pending: we just loaded from disk. */
 
    return SUCCESS;
}

ErrorCode read_archive(Archive *archive, FILE *fp)
{
    uint32_t magic = 0;
    uint64_t file_size = 0;
    uint64_t directory_offset = 0;
 
    if (fp == NULL || archive == NULL) {
        errorp("could not read archive. file pointer or archive pointer is NULL.");
        return NULL_POINTER;
    }
 
    /* the archive must start with MM33. Seek explicitly: 
     *   we cannot assume the caller left the cursor at the beginning */
    if (IO_enumSeek(fp, 0) != SUCCESS) {
        errorp("could not seek to the start of the archive");
        return ERROR_READ_FAILED;
    }
    if (IO_enumReadU32(fp, &magic) != SUCCESS) {
        errorp("could not read the archive magic number");
        return ERROR_READ_FAILED;
    }
    if (magic != MAGIC_NUMBER) {
        errorp("not a Cryptos archive (bad magic number)");
        return ERROR_READ_FAILED;
    }
 
    /* the last 8 bytes hold the offset of the newest directory. */
    if (IO_enumFileSize(fp, &file_size) != SUCCESS) {
        errorp("could not read archive. could not get file size.");
        return ERROR_READ_FAILED;
    }
    if (file_size < MAGIC_SIZE + OFFSET_SIZE) {
        errorp("archive is too small to be valid (%llu bytes).",
               (unsigned long long)file_size);
        return ERROR_READ_FAILED;
    }
 
    if (IO_enumSeek(fp, file_size - OFFSET_SIZE) != SUCCESS)
        return ERROR_READ_FAILED;
 
    if (IO_enumReadU64(fp, &directory_offset) != SUCCESS)
        return ERROR_READ_FAILED;
 
    if (directory_offset < MAGIC_SIZE || directory_offset >= file_size) {
        errorp("directory offset %llu is out of the file bounds.",
               (unsigned long long)directory_offset);
        return ERROR_READ_FAILED;
    }
 
    /* everything else lives in the directory. */
    return read_directory(archive, fp, directory_offset);
}

void print_field_data(Field * field, FILE * stream){
    FILE * fp = field->entry->group->archive->fp; // archive file.
    // seek to field data & print it to stream.
    IO_enumSeek(fp, field->offset + local_field_header_size(field));
    IO_enumPrintFile(stream, IO_u64Tell(fp), field->size, fp);
}