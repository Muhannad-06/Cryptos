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