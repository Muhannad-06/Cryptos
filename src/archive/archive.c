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
#include <time.h>

/* memory functions. */
Archive * archive_create(char *name, char *description, FILE *fp){
    Archive * archive = malloc(sizeof(Archive));
    if(!archive) {
        error("archive_create: couldn't allocate memory for archive");
    }
    
    archive->creation_date = (uint64_t) time(NULL);
    archive->directory_offset = 0; /* 0 = sentinel meaning "no directory written yet / no previous directory" */
    archive->fp = fp; // the archive's backing file, provided by the caller.
    archive->groups = vector_create();
    archive->entries = vector_create();
    archive->fields = vector_create();
    archive->fields_updated = vector_create();
    archive->name = name;
    archive->description = description;
    archive->version = ARCHIVE_VERSION /* current project version */ ; 
    archive->entries->size = 0;
    archive->groups->size = 0;

    /* archive size is initially 0 until the archive is written. */
    archive->size = 0; 
    
    /* initialize num of changes. */
    archive->num_of_changes = 0; // initially 0 until it's written.
    
    // initializing text search tree.
    archive->search_tree = bst_create();
    
    return archive;
}

// update search tree's data.
ErrorCode search_tree_gen(Archive * archive) {
    // ErrorCode status = SUCCESS;

    /* clean old search tree */
    bst_destroy(archive->search_tree);
    archive->search_tree = bst_create();

    // group data.
    for (size_t i = 0; i < archive->groups->size; i++) {
        Group * group = vector_at(archive->groups, i);
        if (!group) {
            error("search tree gen: null group pointer");
        }
        TNode *node = bst_insert(archive->search_tree, group->name, group, NAME, GROUP);
        if (!node) {
            // error("search tree gen: couldn't insert group: %s", group->name);
        }
    }
    // entry data.
    for (size_t i = 0; i < archive->entries->size; i++) {
        Entry * entry = vector_at(archive->entries, i);
        if (!entry) {
            error("search tree gen: null entry pointer");
        }
        TNode *node = bst_insert(archive->search_tree, entry->name, entry, NAME, ENTRY);
        if (!node) {
            // error("search tree gen: couldn't insert entry: %s", entry->name);
        }
    }
    // field data.
    for (size_t i = 0;  i < archive->fields->size; i++) {
        Field * field = vector_at(archive->fields, i);
        if (!field) {
            error("search tree gen: null field pointer");
        }
        TNode *node = bst_insert(archive->search_tree, field->name, field, NAME, FIELD);
        if (!node) {
            // error("search tree gen: couldn't insert field: %s", field->name);
        }
        if (field->type == TEXT) {
            node = bst_insert(archive->search_tree, field->content, field, CONTENT, FIELD);
            if (!node) {
                // error("search tree gen: couldn't insert field: %s", field->name);
            }
        }
    }

    // return (status == SUCCESS) ? SUCCESS : FAILURE;
    return SUCCESS;
}

// find text function
TNode *archive_find_entity(Archive *archive, char *entity_name) {
    if (!archive || !entity_name) {
        error("find_entity: null pointer");
    }

    TNode *node = bst_find(archive->search_tree, entity_name);
    if (!node) {
        errorp("archive_find_entity: \"%s\" not found", entity_name);
        return NULL;
    }
    if (node->entity_type != FIELD && node->entity_type != ENTRY && node->entity_type != GROUP) {
        error("find_entity: invalid entity type");
    }
    
    return node; /* NOTE: this function can return null if text is not found */
}

char *archive_find_entity_str(Archive *archive, char *entity_name) {
    TNode *node = archive_find_entity(archive, entity_name);
    if (!node) {
        return NULL; /* NOTE: this function can return null if text is not found */
    }
    if (node->entity_type == FIELD) {
        return field_to_string(node->entity);
    } else if (node->entity_type == ENTRY) {
        return entry_to_string(node->entity);
    } else if (node->entity_type == GROUP) {
        return group_to_string(node->entity);
    }
    return NULL;
}

void archive_destroy(Archive *archive){
    /* deleting all entities.
     * delete is safer than destroy as it removes pointers pointing to freed structs.
     * delete functions do not affect the file until write_archive is called.
     */
    for (size_t i = 0; i < archive->fields->size; i++) {
        field_delete(vector_at(archive->fields, i));
    }
    for (size_t i = 0; i < archive->entries->size; i++) {
        entry_delete(vector_at(archive->entries, i));
    }
    for(size_t i = 0; i<archive->groups->size; i++){
        group_delete(vector_at(archive->groups, i));
    }
    vector_destroy(archive->groups);
    vector_destroy(archive->entries);
    vector_destroy(archive->fields);
    vector_destroy(archive->fields_updated);
    // free(archive->name);
    // free(archive->description);
    free(archive);
}

Archive * archive_clean_history(Archive * archive){
    if(!archive){
        error("archive_clean_history: null pointer");
    }
    
    char * new_file_name = strcat(archive->name, "_clean");
    
    if(write_archive_clean(archive, new_file_name) ==FAILURE){
        error("archive_clean_history: couldn't write new archive.");
    }
    
    archive->fp = fopen(new_file_name, "wb+");
    return archive;
}


/* mutators */
ErrorCode archive_set_name(Archive *archive, char *name){
    if(!archive){
        errorp("archive_set_name: null pointer.");
        return NULL_POINTER;
    }
    archive->name = name;
    
    archive->written = 0;
    return SUCCESS;
}

ErrorCode archive_set_description(Archive *archive, char *description){
    if(!archive){
        error("archive_set_description: null pointer.");
        return NULL_POINTER;
    }
    archive->description = description;
    
    archive->written = 0;
    return SUCCESS;
}

char *archive_to_string(Archive *archive){
    char * string;
    asprintf(&string,
         "Archive \"%s\". version: %u. description: %s \n size: %u. number of groups: %u. number of entries: %u. number of fields: %u. \n number of changes: %u, is written: %u",
        archive->name, archive->version, archive->description, (unsigned int) archive->size, (unsigned int)archive->entries->size, (unsigned int)archive->groups->size, (unsigned int) archive->fields->size, archive->num_of_changes, archive->written);
    if(!string){
        error("archive_to_string: failed.");
    }
    // return strcat(strcat(archive->name, ", description: "), archive->description);
    return string;
    // #TODO: return a better format.
}

/* archive format functions */

/* Archive history functions */

Archive * archive_get_backward(Archive * archive, uint32_t steps){
    if(!archive){
        error("archive_get_backward: null pointer");
    }

    for(uint32_t i = 0; i < steps; i++){
        ErrorCode result = read_directory(archive, archive->fp, archive->prev_dir_offset);
        if(result != SUCCESS){
            error("archive_get_backward: couldn't get backward version by %u steps", steps);
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

    archive_versions[num_of_changes - 1] = archive;

    uint64_t prev_offset = archive->prev_dir_offset;
    for (uint32_t i = num_of_changes - 1; i-- > 0; ) {
        Archive *version = archive_create(NULL, NULL, archive->fp);
        if (!version || read_directory(version, archive->fp, prev_offset) != SUCCESS) {
            if (version) archive_destroy(version);
            /* free versions already built before this failure. */
            for (uint32_t j = i + 1; j < num_of_changes - 1; j++) {
                archive_destroy(archive_versions[j]);
            }
            free(archive_versions);
            error("archive_get_versions: got null pointer in version %u", i);
        }
        archive_versions[i] = version;
        prev_offset = version->prev_dir_offset;
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

/* NOTE: deprecated */
/* calculated total archive size in write functions instead... */
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

    for(size_t i = 0; i<versions->size; i++){
        Archive *curr_archive = vector_at(versions, i);
        // add directory size
        directories_size += directory_size(curr_archive);

        Vector *curr_fields = curr_archive->fields;
        for(size_t j = 0; j<curr_fields->size; j++){
            Field *curr_field = vector_at(curr_fields, j);

            TNode *curr_field_node = bst_find(all_fields, curr_field->name);
            if(curr_field_node != NULL){ 
                if(!curr_field_node->entity){
                    error("archive size: bst node with null pointer entity.");
                }

                Field *curr_field_counted = (Field *) curr_field_node->entity;

                if(curr_field->last_modification_date == curr_field_counted->last_modification_date){
                    continue;
                } else{
                    // field_destroy(curr_field_counted); // already freed in archive_destroy.
                    curr_field_node->entity = curr_field;
                    fields_size += field_full_size(curr_field);
                }

            } else {
                fields_size += field_full_size(curr_field);
                bst_insert(all_fields, curr_field->name, curr_field, NAME, FIELD);
            }
        }
    }

    // clean old versions structs.
    for(size_t i = 0; i < versions->size; i++){
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
        (uint64_t) strlen(group->name);
}

uint64_t entry_header_size(Entry *entry){
    if(!entry){
        error("entry_header_size: null pointer.");
    }
    return MAGIC_SIZE+
        ID_SIZE*2 +
        DATE_SIZE*2 +
        STRING_LENGTH_SIZE +
        (uint64_t) strlen(entry->name);
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
        (uint64_t) strlen(field->name);
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
        (uint64_t) strlen(field->name);
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

    for(size_t i = 0; i<(archive->fields->size); i++){
        field_headers_size +=  field_header_size(archive->fields->data[i]);
    }

    for(size_t i = 0; i<(archive->groups->size); i++){
        group_headers_size += group_header_size(archive->groups->data[i]);
    }

    for(size_t i = 0; i<(archive->entries->size); i++){
        entry_headers_size += entry_header_size(archive->entries->data[i]);
    }

    return MAGIC_SIZE +
        FILE_VERSION_SIZE +
        OFFSET_SIZE +
        DATE_SIZE*2 +
        ID_SIZE*3 +
        STRING_LENGTH_SIZE*2 +
        (uint64_t) strlen(archive->name) +
        (uint64_t) strlen(archive->description) +
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
        errorp("write_group_header: could not write group header for group: %s. file not open.", group->name);
        return FAILURE;
    }

    /* seek to the end of file and write magic number. */
    if (IO_enumSeek(fp, 0, SEEK_END) != SUCCESS || IO_enumWriteU32(group->archive->fp, MAGIC_NUMBER) != SUCCESS) {
        error("write_group_header: could not write group header for group: %s. could not append to file.", group->name);
        return FAILURE;
    }
    
    /* writing header content */

    status += IO_enumWriteU32(fp, group->group_id); // writing group ID
    status += IO_enumWriteU32(fp, group->creation_date);
    status += IO_enumWriteU32(fp, group->last_modification_date);
    status += IO_enumWriteU16(fp, (uint16_t) (strlen(group->name) /*+ 1*/)); // + 1 is for null character. null character is not written.
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
        errorp("could not write entry header for entry: %s. file not open.", entry->name);
        return FAILURE;
    }

    /* seek to the end of file and write magic number. */
    if (IO_enumSeek(fp, 0, SEEK_END) != SUCCESS || IO_enumWriteU32(fp, MAGIC_NUMBER) != SUCCESS) {
        errorp("could not write entry header for entry: %s. could not append to file.", entry->name);
        return FAILURE;
    }
    
    /* writing header content */    

    status += IO_enumWriteU32(fp, entry->group->group_id); // writing group ID
    status += IO_enumWriteU32(fp, entry->entry_id); // writing entry ID
    status += IO_enumWriteU32(fp, entry->creation_date);
    status += IO_enumWriteU32(fp, entry->last_modification_date);
    status += IO_enumWriteU16(fp, (uint16_t) (strlen(entry->name)));
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
        errorp("could not write field header for field: %s. file not open.", field->name);
        return FAILURE;
    }

    /* seek to the end of file and write magic number. */
    if (IO_enumSeek(fp, 0 , SEEK_END) != SUCCESS || IO_enumWriteU32(fp, MAGIC_NUMBER) != SUCCESS) {
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
    status += IO_enumWriteU16(fp, (uint16_t) (strlen(field->name)));
    status += IO_enumWriteString(fp, field->name);
    
    return (status != SUCCESS)? FAILURE : SUCCESS; // return success or failure.
}

ErrorCode write_directory(Archive *archive){
    if(!archive){
        errorp("write_directory: null pointer.");
        return NULL_POINTER;
    }

    if(archive->written){
        errorp("File already updated.");
        return FAILURE; 
    }

    int status = SUCCESS;
    FILE *fp = archive->fp;
    /* check if file is open and appendable. */
    if(fp == NULL){
        errorp("could not write directory: %s. file not open.", archive->name);
        return FAILURE;
    }

        /* seek to the end of file and write magic number. */
    if(IO_enumSeek(fp, 0 , SEEK_END) !=  SUCCESS){
        errorp("could not write file directory: %s. could not append to file.", archive->name);
        return FAILURE;
    }
    // initial position of the directory.
    uint64_t init_pos = IO_u64Tell(fp);

    if(IO_enumWriteU32(fp, MAGIC_NUMBER) != SUCCESS){
        errorp("write directory: couldn't write magic number to the file.");
        return FAILURE;
    }
    
    /* fields that should be updated before writing directory. 
    * - archive size. -> in write_field, write_field_local_header.
    * - archive num of changes. -> in write_archive.
    */

    /* writing directory header. */
    status += IO_enumWriteU16(fp, archive->version);
    /* save the size offset to correct it later. */
    uint64_t size_offset = IO_u64Tell(fp);
    status += IO_enumWriteU64(fp, 0); // place hold. 
    status += IO_enumWriteU32(fp, archive->creation_date);
    status += IO_enumWriteU32(fp, archive->last_modification_date);
    archive->num_of_changes++;
    status += IO_enumWriteU32(fp, archive->num_of_changes); // changes in write_directory.
    /* updating directory offset. */
    // writing prev directory offset
    status += IO_enumWriteU64(fp, archive->directory_offset);
    archive->prev_dir_offset = archive->directory_offset;
    // archive->directory_offset = archive->size + 1;
    archive->directory_offset = init_pos;

    /* writing groups headers. */
    status+= IO_enumWriteU32(fp, archive->groups->size);
    for(size_t i = 0; i < archive->groups->size; i++){
        status += write_group_header(vector_at(archive->groups, i));
    }
    /* writing entries headers. */
    status+= IO_enumWriteU32(fp, archive->entries->size);
    for(size_t i = 0; i < archive->entries->size; i++){
        status += write_entry_header(vector_at(archive->entries, i));
    }
    /* writing fields headers*/
    status+= IO_enumWriteU32(fp, archive->fields->size);
    for(size_t i = 0; i < archive->fields->size; i++){
        status += write_field_header(vector_at(archive->fields, i));
    }

    /* writing archive name */
    status+= IO_enumWriteU16(fp, strlen(archive->name)); // added 1 for the null character. null character is not written.
    status+= IO_enumWriteString(fp, archive->name); // added 1 for the null character.
    
    /* writing archive description */
    status+= IO_enumWriteU16(fp, strlen(archive->description)); // added 1 for the null character. null character is not written.
    status+= IO_enumWriteString(fp, archive->description);
    
    /* writing the directory's offset at the end of the file. */
    status += IO_enumWriteU64(fp, archive->directory_offset);

    /* update archive */
    /* indicate that file is new data is written */
    archive->written = 1;
    
    /* update size */
    uint64_t end_pos = IO_u64Tell(fp);
    archive->size += end_pos - init_pos;
    
    /* write new size to archive */
    IO_enumSeek(fp, size_offset, SEEK_SET);
    IO_enumWriteU64(fp, archive->size);
    IO_enumSeek(fp, 0, SEEK_END);

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
    if(IO_enumSeek(fp, 0, SEEK_END) != SUCCESS){
        error("could not write field local header for field: %s. could not append to file.", field->name);
        return FAILURE;
    }
    // initial position.
    uint64_t init_pos = IO_u64Tell(fp);
    field->offset = init_pos; // assigning field offset.
    // test writing magic byte.
    if(IO_enumWriteU32(fp, MAGIC_NUMBER) != SUCCESS){
        error("write_field_local_header: couldn't write magic number to file.");
        return FAILURE;
    }
    
    /* writing header content */

    status += IO_enumWriteU32(fp, field->entry->entry_id); // writing entry ID
    status += IO_enumWriteU16(fp, field->compression);
    field->compressed_size_offset = IO_u64Tell(fp);
    status += IO_enumWriteU64(fp, field->compressed_size);
    status += IO_enumWriteU64(fp, field->size);
    status += IO_enumWriteU32(fp, field_crc(field));
    status += IO_enumWriteU32(fp, field->creation_date);
    status += IO_enumWriteU32(fp, field->last_modification_date);
    status += IO_enumWriteU16(fp, field->type);
    status += IO_enumWriteU16(fp, (uint16_t) (strlen(field->name)) );
    status += IO_enumWriteString(fp, field->name);
    uint64_t end_pos = IO_u64Tell(fp);
    
    // add local header size to the field's full size.
    field->full_size += end_pos - init_pos;
    
    // manage archive size.
    archive->size += field->full_size;
    
    return (status != SUCCESS)? FAILURE : SUCCESS; // return success or failure.
}

ErrorCode write_field(Field * field){
    if(!field){
        error("write_field: null pointer.");
    }

    Archive *archive = field->entry->group->archive;
    FILE *fp = archive->fp;
    int status = write_field_local_header(field);

    uint64_t init_pos = IO_u64Tell(fp);
    if(field->compression == NON_COMPRESSED){
        if(field->type == TEXT || field->type == PASSWORD){
            status += IO_enumWriteString(fp, (char *) (field->content));
        } else if(field->type == BINARY){
            status += IO_enumWriteFile(fp, (FILE *) (field->content));
        }
    } else if(field->compression == DEFLATE){
        // #TODO: write with deflate algorithm.
    } else {
        errorp("write_field: could not write field. invalid compression method in field: %s", field->name);
        return FAILURE;
    }
    uint64_t end_pos = IO_u64Tell(fp);
    // manage field size.
    field->size = end_pos - init_pos;
    field->full_size += field->size;
    // update field.
    field->number_of_changes++; 
    // archive size management.
    archive->size += field->size; /* the local header size is added to archive size in write_field_local_header */
    /* writing the new size in the local header */
    field->compressed_size = field->size; // Assuming no compression for now
    IO_enumSeek(fp, field->compressed_size_offset, SEEK_SET);
    IO_enumWriteU64(fp, field->compressed_size);
    IO_enumWriteU64(fp, field->size);
    IO_enumSeek(fp, 0, SEEK_END);
    // remove from updated list.
    vector_remove_value(archive->fields_updated, field);

    return (status != SUCCESS)? FAILURE : SUCCESS; // return success or failure.
}

ErrorCode write_archive(Archive *archive){
    if(!archive){
        errorp("write_archive: null pointer.");
        return NULL_POINTER;
    }
    int status = SUCCESS;
    // iterate through all updated fields and write them.
    // copy vector (as the main vector alters during the process).
    Vector * fields_to_write = vector_copy(archive->fields_updated);
    for (size_t i = 0; i < fields_to_write->size ; i++) {
        write_field(vector_at(fields_to_write, i));
    }
    vector_destroy(fields_to_write);
    // write directory.
    status += write_directory(archive);
    
    archive->written = 1;
    // archive->num_of_changes++; // this is handled in write_directory.
    return (status != SUCCESS)? FAILURE : SUCCESS; // return success or failure.
}

ErrorCode write_archive_clean(Archive * archive, char *new_file_name){
    if (!archive) {
        errorp("write_archive_clean: null pointer");
        return NULL_POINTER;
    }
    int status = SUCCESS;
    // cleaning number of changes.
    uint32_t old_n_changes = archive->num_of_changes;
    archive->num_of_changes = 0;
    // write new archive with clean history.
    FILE *old_fp = archive->fp;
    FILE *new_fp = fopen(new_file_name, "wb+");
    if(!new_fp){
        errorp("write_archive_clean: couldn't open new file");
        return FAILURE;
    }
    archive->fp = new_fp;

    status += write_archive(archive);
    
    fclose(new_fp);
    archive->fp = old_fp;
    archive->num_of_changes = old_n_changes;
    
    return (status != SUCCESS)? FAILURE : SUCCESS; // return success or failure.
}


/* <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< Read functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> */

/* Helper function to expect a specific magic number 
*       Why Static? As it won't be used outside this file */
static ErrorCode expect_magic(FILE *fp, const char *context) {
    uint32_t magic = 0;
    if (IO_enumReadU32(fp, &magic) != SUCCESS) {
        errorp("expect_magic: could not read magic number for %s", context);
        return ERROR_READ_FAILED;
    }
    if (magic != MAGIC_NUMBER) {
        errorp("expect_magic: bad magic number for %s", context);
        return ERROR_READ_FAILED;
    }
    return SUCCESS;
}

static ErrorCode read_prefixed_string(FILE *fp, char **out_str) {
    uint16_t length = 0;

    if (!fp || !out_str) {
        return NULL_POINTER;
    }

    if (IO_enumReadU16(fp, &length) != SUCCESS) {
        errorp("read_prefixed_string: could not read string length.");
        return ERROR_READ_FAILED;
    }

    return IO_charReadString(fp, length, out_str);
}

static Group *archive_find_group(Archive *archive, uint32_t group_id) {
    if (!archive || !archive->groups) return NULL;

    for (size_t i = 0; i < archive->groups->size; i++) {
        Group *group = vector_at(archive->groups, i);
        if (group && group->group_id == group_id) {
            return group;
        }
    }
    return NULL;
}

static Entry *archive_find_entry(Archive *archive, uint32_t entry_id) {
    if (!archive || !archive->entries) return NULL;

    for (size_t i = 0; i < archive->entries->size; i++) {
        Entry *entry = vector_at(archive->entries, i);
        if (entry && entry->entry_id == entry_id) {
            return entry;
        }
    }
    return NULL;
}

static ErrorCode archive_reset(Archive *archive) {
    if (!archive) {
        errorp("archive_reset: null pointer");
        return NULL_POINTER;
    }

    for (size_t i = 0; i < archive->fields->size; i++) {
        field_destroy(vector_at(archive->fields, i));
    }
    for (size_t i = 0; i < archive->entries->size; i++) {
        entry_destroy(vector_at(archive->entries, i));
    }
    for (size_t i = 0; i < archive->groups->size; i++) {
        group_destroy(vector_at(archive->groups, i));
    }

    vector_destroy(archive->groups);
    vector_destroy(archive->entries);
    vector_destroy(archive->fields);

    archive->groups = vector_create();
    archive->entries = vector_create();
    archive->fields = vector_create();

    archive->written = 0;

    return SUCCESS;
}

Group *read_group_header(Archive *archive, FILE *fp, uint64_t offset)
{
    Group *group;
    uint32_t group_id = 0, creation = 0, last_mod = 0;
    char *name = NULL;
 
    if (!archive || !fp) {
        errorp("could not read group header. NULL pointer.");
        return NULL;
    }
 
    if (IO_enumSeek(fp, offset, SEEK_SET) != SUCCESS) {
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

Group ** read_group_headers(Archive * archive, FILE *fp, uint64_t n_groups_offset, uint32_t *out_count){
    IO_enumSeek(fp, n_groups_offset, SEEK_SET);

    uint32_t n = 0;
    if (IO_enumReadU32(fp, &n) != SUCCESS) {
        errorp("could not read the number of groups.");
        return NULL;
    }

    if (n == 0) {
        if (out_count) *out_count = 0;
        return NULL;
    }

    Group ** groups = malloc(sizeof(Group *) * n);
    
    for(uint32_t i = 0; i < n; i++){
        Group * group = read_group_header(archive, fp, IO_u64Tell(fp));
        groups[i] = group;
    }
 
    archive->groups->size = n;
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
 
    if (IO_enumSeek(fp, offset, SEEK_SET) != SUCCESS) {
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
 
    if (IO_enumSeek(fp, n_entries_offset, SEEK_SET) != SUCCESS) {
        errorp("could not seek to the number of entries.");
        return NULL;
    }
    if (IO_enumReadU32(fp, &n) != SUCCESS) {
        errorp("could not read the number of entries.");
        return NULL;
    }
    if (n == 0) {
        archive->groups->size = 0;
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
 
    archive->groups->size = n;
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
 
    if (!archive || !fp) return NULL;
 
    if (IO_enumSeek(fp, offset, SEEK_SET) != SUCCESS) return NULL;
    if (expect_magic(fp, "field header") != SUCCESS) return NULL;
 
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
 
    if (read_prefixed_string(fp, &name) != SUCCESS) return NULL;
 
    entry = archive_find_entry(archive, entry_id);
    if (!entry) {
        free(name);
        return NULL;
    }
 
    field = field_create(entry, type, name);
    if (!field) {
        free(name);
        return NULL;
    }
 
    field->name = name;
    field->compression = (CompressionType)compression;
    field->compressed_size = compressed_size;
    field->size = size;
    field->offset = field_offset; /* Map the read offset */
    field->crc = crc;
    field->creation_date = creation;
    field->last_modification_date = last_mod;
    field->type = (FieldType)type;
    field->content = NULL;
 
    if ((field->type == TEXT || field->type == PASSWORD) && field->size > 0) {
        uint64_t local_header_size = MAGIC_SIZE + ID_SIZE + TYPE_SIZE * 2 +
                                      OFFSET_SIZE * 2 + CRC_SIZE + DATE_SIZE * 2 +
                                      STRING_LENGTH_SIZE + strlen(field->name);
        uint64_t saved_pos = IO_u64Tell(fp);
        char *content = malloc((size_t) field->size + 1);
        if (content) {
            if (IO_enumSeek(fp, field->offset + local_header_size, SEEK_SET) == SUCCESS &&
                IO_enumReadBytes(fp, content, (size_t) field->size) == SUCCESS) {
                content[field->size] = '\0';
                field->content = content;
            } else {
                errorp("could not read content for field %s.", field->name);
                free(content);
            }
        }
        IO_enumSeek(fp, saved_pos, SEEK_SET);
    }
 
    return field;
}
 
Field **read_field_headers(Archive *archive, FILE *fp,
                           uint64_t n_fields_offset, uint32_t *out_count)
{
    Field **fields;
    uint32_t n = 0, i;
 
    if (out_count) *out_count = 0;
    if (!archive || !fp) return NULL;
 
    if (IO_enumSeek(fp, n_fields_offset, SEEK_SET) != SUCCESS) {
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
 
    if (IO_enumSeek(fp, directory_offset, SEEK_SET) != SUCCESS) {
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
 
    archive->name = name;
    archive->description = description;
 
    /* Set last, because group_create()/field_create() touch these while
     * the records are being parsed. */
    archive->groups->size = n_groups;
    archive->entries->size = n_entries;
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
    if (IO_enumSeek(fp, 0, SEEK_SET) != SUCCESS) {
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
    if (IO_u64FileSize(fp, &file_size) != SUCCESS) {
        errorp("could not read archive. could not get file size.");
        return ERROR_READ_FAILED;
    }
    if (file_size < MAGIC_SIZE + OFFSET_SIZE) {
        errorp("archive is too small to be valid (%llu bytes).",
               (unsigned long long)file_size);
        return ERROR_READ_FAILED;
    }
 
    if (IO_enumSeek(fp, file_size - OFFSET_SIZE, SEEK_SET) != SUCCESS)
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

ErrorCode print_field_data(Field * field, FILE * stream){
    ErrorCode status = SUCCESS;
    FILE * fp = field->entry->group->archive->fp; // archive file.
    // seek to field data & print it to stream.
    IO_enumSeek(fp, field->offset + local_field_header_size(field), SEEK_SET);
    IO_enumPrintFile(stream, IO_u64Tell(fp), field->size, fp);
    return status;
}