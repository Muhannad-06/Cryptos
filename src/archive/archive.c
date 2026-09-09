#include "../../include/archive/archive.h"
#include "../../include/io/Bin_IO.h"
#include "../../include/utils/error.h"
#include "../../include/utils/crc.h"
#include "../../include/utils/bst.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

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
    
    /* initialize num of changes. */
    archive->num_of_changes = 1;
    
    return archive;
}

Archive * archive_clean_history(Archive * archive){
    if(!archive){
        return NULL;
    }
    // #TODO
    return NULL;
}


/* mutators */
void archive_set_name(Archive *archive, char *name){
    if(!archive){
        error("archive_set_name: null pointer.");
    }
    archive->name = name;
    
    archive->written = 0;
}

void archive_set_description(Archive *archive, char *description){
    if(!archive){
        error("archive_set_description: null pointer.");
    }
    archive->description = description;
    
    archive->written = 0;
}

char *archive_to_string(Archive *archive){
    return strcat(strcat(archive->name, ", description: "), archive->description);
    // #TODO: return a better format.
}

/* archive format functions */

/* Archive history functions */

Archive * archive_get_backward(Archive * archive, uint32_t steps){
    if(!archive){
        return NULL;
    }

    for(uint32_t i = 0; i < steps; i++){
        archive = read_directory(archive->fp, archive->prev_dir_offset);
        if(!archive){
            errorp("archive_get_backward: couldn't get backward version by %u steps", steps);
            return NULL;
        }
    }

    return archive;
}

Vector * archive_get_versions(Archive *archive){
    if(!archive){
        error("archive_get_versions: null pointer");
        // return NULL;
    }
    if(archive->num_of_changes == 0){
        error("archive_get_versions: archive has no versions.");
    }
    // all archive versions.
    uint32_t num_of_changes = archive->num_of_changes;
    Archive ** archive_versions = calloc((size_t)num_of_changes, sizeof(Archive *));
    if(!archive_versions){
        error("archive_get_versions: allocation for archive_versions failed.");
    }

    int c = archive->num_of_changes;
    while(c-->0){
        archive_versions[c] = archive;
        if(c > 0){
            archive = archive_get_backward(archive, 1);
            if(!archive){
                free(archive_versions);
                error("archive_get_versions: got null pointer in version %d", c);
            }
        }
    }
    
    Vector * vector = vector_create();
    if(!vector){
        error("versions vector creation failed.");
    }

    vector->size = num_of_changes;        
    vector->capacity = vector->size;
    vector->data = (void **) archive_versions;
    
    return vector;
}

/* dynamic size calculation functions */

uint64_t archive_size(Archive *archive){
    if(!archive){
        error("archive_size: null pointer.");
    }

    Vector * versions = archive_get_versions(archive);
    // calculate directories sizes over archive history.
    uint64_t directories_size = 0;
    // calculate field sizes (including local header.).
    uint64_t fields_size = 0;
    BST * all_fields = bst_create();
    if(!all_fields){
        error("archive_size: couldn't create all fields BST.");
    }

    for(uint32_t i = 0; i<versions->size; i++){
        Archive *curr_archive = vector_at(versions, i);
        // add directory size
        directories_size += directory_size(curr_archive);

        Vector *curr_fields = curr_archive->fields;
        for(uint32_t j = 0; j<curr_fields->size; j++){
            Field *curr_field = vector_at(curr_fields, j);

            TNode *curr_field_node = bst_find(curr_field->name);
            if(curr_field_node != NULL){ 
                if(!curr_field_node->entity){
                    error("archive size: bst node with null pointer entity.");
                }

                Field *curr_field_counted = (Field *) curr_field_node->entity;

                if(curr_field->last_modification_date == curr_field_counted->last_modification_date){
                    continue;
                } else{
                    // field_destroy(curr_field_counted); // already freed in archvie_destroy.
                    curr_field_node->entity = curr_field;
                    fields_size += field_full_size(curr_field);
                }

            } else {
                fields_size += field_full_size(curr_field);
                bst_insert(curr_field->name, curr_field, NAME, FIELD);
            }
        }
    }

    // clean old versions structs.
    for(uint32_t i = 0; i < versions->size; i++){
        Archive *curr_archive = vector_at(versions, i);
        // Protect the active archive from destruction
        if(curr_archive != archive){
            archive_destroy(curr_archive);
        }
    }
    
    vector_destroy(versions);
    bst_destroy(all_fields);
    
    return fields_size +
        directories_size;
}

uint64_t group_header_size(Group *group){
    if(!group){
        error("group_header_size: null pointer.");
    }
    return MAGIC_SIZE +
        ID_SIZE +
        DATE_SIZE*2 +
        STRING_LENGTH_SIZE +
        (uint64_t) sizeof(group->name);
}

uint64_t entry_header_size(Entry *entry){
    if(!entry){
        error("entry_header_size: null pointer.");
    }
    return MAGIC_SIZE+
        ID_SIZE*2 +
        DATE_SIZE*2 +
        STRING_LENGTH_SIZE +
        (uint64_t) sizeof(entry->name);
}

uint64_t field_header_size(Field *field){
    if(!field){
        error("field_header_size: null pointer.");
    }
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
    if(!field){
        error("local_field_size: null pointer.");
    }
    return MAGIC_SIZE +
        ID_SIZE +
        TYPE_SIZE*2 +
        OFFSET_SIZE*2 +
        CRC_SIZE +
        DATE_SIZE*2 +
        STRING_LENGTH_SIZE +
        (uint64_t) sizeof(field->name);
}

uint64_t field_full_size(Field *field){
    if(!field){
        error("field_full_size: null pointer.");
    }
    return field->size + local_field_header_size(field);
}

uint64_t directory_size(Archive *archive){
    if(!archive){
        error("directory_size: null pointer");
    }
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
        field_headers_size +
        OFFSET_SIZE/* directory offset is added at the end of the directory. */;
}

/* writing functions */

ErrorCode write_group_header(Group * group){
    if(!group){
        errorp("write_group_header: null pointer.");
        return NULL_POINTER;
    }
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
    if(!entry){
        errorp("write_entry_header: null pointer.");
        return NULL_POINTER;
    }
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
    if(!field){
        errorp("write_field_header: null pointer.");
        return NULL_POINTER;
    }
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
    if(!archive){
        errorp("write_directory: null pointer.");
        return NULL_POINTER;
    }

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
    if(!field){
        errorp("write_field_local_header: null pointer.");
        return NULL_POINTER;
    }
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
    if(!field){
        errorp("write_field: null pointer.");
        return NULL_POINTER;
    }
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
    if(!archive){
        errorp("write_archive: null pointer.");
        return NULL_POINTER;
    }
    archive->num_of_changes++;
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
    
    archive->written = 1;
    return (status != SUCCESS)? FAILURE : SUCCESS; // return success or failure.
}

ErrorCode write_archive_clean(Archive * archive, char *new_file_name){
    if (!archive) {
        errorp("write_archive_clean: null pointer");
        return NULL_POINTER;
    }
    int status = SUCCESS;
    // #TODO: write new archive with clean history.
    return (status != SUCCESS)? FAILURE : SUCCESS; // return success or failure.
}


/* read functions */

Group * read_group_header(Archive * archive, FILE *fp, uint64_t offset){
    Group * group = group_create(archive,  "");
    /* #TODO: parse group data.*/

    return group;
}

Group ** read_group_headers(Archive * archive, FILE *fp, uint64_t n_groups_offset){
    /* seek to number of groups section. */
    IO_enumSeek(fp, n_groups_offset);

    uint32_t n;
    /* #TODO: parse number of groups into n */

    // groups array
    Group ** groups = malloc(sizeof(Group *) * n);
    
    /* parse groups data. */
    
    for(int i = 0; i < n; i++){
        Group * group = read_group_header(archive, fp, IO_u64Tell(fp));
        groups[i] = group;
        
        vector_push_back(archive->groups, group);
    }
    
    return groups;
}

Entry * read_entry_header(Archive * archive, FILE *fp, uint64_t offset) {

    return NULL;
}

Entry ** read_entry_headers(Archive * archive, FILE *fp, uint64_t n_entries_offset){

    return NULL;
}

Field * read_field_header(Archive * archive, FILE *fp, uint64_t offset){

    return NULL;
}

Entry ** read_field_headers(Archive * archive, FILE *fp, uint64_t n_fields_offset){

    return NULL;
}

Archive * read_directory(FILE *fp, uint64_t directory_offset){
    // seek to the directory.
    IO_enumSeek(fp, directory_offset);

    Archive * archive = archive_create("", "");
    // # TODO parse data into archive
    

    return archive;
}
Archive * read_archive(FILE *fp){
    uint64_t directory_offset;
    // #TODO seek to end of file and read directory offset.
    
    // read directory
    Archive * archive = read_directory(fp, directory_offset);

    return archive;
}

void print_field_data(Field * field, FILE * stream){
    FILE * fp = field->entry->group->archive->fp; // archive file.
    // seek to field data & print it to stream.
    IO_enumSeek(fp, field->offset + local_field_header_size(field));
    IO_enumPrintFile(stream, IO_u64Tell(fp), field->size, fp);
}