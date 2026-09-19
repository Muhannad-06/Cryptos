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
Archive * archive_create(char *name, char *description, FILE *fp){
    Archive * archive = malloc(sizeof(Archive));
    if(!archive) {
        error("archive_create: couldn't allocate memory for archive");
    }
    
    archive->directory_offset = 0; // initially 0 until it's written.
    archive->fp = fp;
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
    for (int i = 0; i < archive->groups->size; i++) {
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
    for (int i = 0; i < archive->entries->size; i++) {
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
    for (int i = 0;  i < (int)archive->fields->size; i++) {
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
    if (node->entity_type != FIELD && node->entity_type != ENTRY && node->entity_type != GROUP) {
        error("find_entity: invalid entity type");
    }
    if (!node) {
        errorp("archive_find_entity: \"%s\" not found", entity_name);
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
        archive->name, archive->version, archive->description, (unsigned int) archive->size, archive->entries->size, archive->groups->size, (unsigned int) archive->fields->size, archive->num_of_changes, archive->written);
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
        archive = read_directory(archive->fp, archive->prev_dir_offset);
        if(!archive){
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

    uint32_t c = archive->num_of_changes;
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

    for(uint32_t i = 0; i<versions->size; i++){
        Archive *curr_archive = vector_at(versions, i);
        // add directory size
        directories_size += directory_size(curr_archive);

        Vector *curr_fields = curr_archive->fields;
        for(uint32_t j = 0; j<curr_fields->size; j++){
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
        field_headers_size +=  field_header_size(archive->fields->data[i]);
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
        errorp("write_group_header: could not write group header for group: %s. file not open.", group->name);
        return FAILURE;
    }

        /* seek to the end of file and write magic number. */
    if(IO_enumSeek(fp, 0, SEEK_END) != IO_enumWriteU32(group->archive->fp, MAGIC_NUMBER) != SUCCESS){
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
    if(IO_enumSeek(fp, 0, SEEK_END) != IO_enumWriteU32(fp, MAGIC_NUMBER) != SUCCESS){
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
    if(IO_enumSeek(fp, 0 , SEEK_END) != IO_enumWriteU32(fp, MAGIC_NUMBER) != SUCCESS){
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
    for(int i = 0; i < archive->groups->size; i++){
        status += write_group_header(vector_at(archive->groups, i));
    }
    /* writing entries headers. */
    status+= IO_enumWriteU32(fp, archive->entries->size);
    for(int i = 0; i < archive->entries->size; i++){
        status += write_entry_header(vector_at(archive->entries, i));
    }
    /* writing fields headers*/
    status+= IO_enumWriteU32(fp, archive->fields->size);
    for(int i = 0; i < archive->fields->size; i++){
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


/* read functions */

Group * read_group_header(Archive * archive, FILE *fp, uint64_t offset){
    Group * group = group_create(archive,  "");
    /* #TODO: parse group data.*/

    return group;
}

Group ** read_group_headers(Archive * archive, FILE *fp, uint64_t n_groups_offset){
    /* seek to number of groups section. */
    IO_enumSeek(fp, n_groups_offset, SEEK_SET);

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
    IO_enumSeek(fp, directory_offset, SEEK_SET);

    Archive * archive = archive_create("", "", fp);
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

ErrorCode print_field_data(Field * field, FILE * stream){
    ErrorCode status = SUCCESS;
    FILE * fp = field->entry->group->archive->fp; // archive file.
    // seek to field data & print it to stream.
    IO_enumSeek(fp, field->offset + local_field_header_size(field), SEEK_SET);
    IO_enumPrintFile(stream, IO_u64Tell(fp), field->size, fp);
    return status;
}