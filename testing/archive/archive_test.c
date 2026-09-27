/* checking archive functionality. 
* NOTE: This test is for fields of TEXT type.
*/
#include <stdarg.h>
// #include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../include/types.h"
#include "../../include/utils/error.h"
#include "../../include/utils/assertion.h"
#include "../../include/archive/archive.h"
// #include "../../../include/archive/field.h"
#include "../../include/archive/entry.h"
// #include "../../../include/archive/archive.h"

FILE * fp_cmp;
FILE * fp_tst;
Archive * archive;
Archive *archive_read;
Group *group;
Entry * entry;
Field * field;
Group *group_read;
Entry *entry_read;
Field *field_read;

void setup(){
    fp_cmp = fopen("cmp_archive.arc", "rb");
    fp_tst = fopen("test_archive.arc", "wb+");

    if (!fp_tst) {
       error("couldn't open test file.");
    }
    if (!fp_cmp) {
        error("couldn't open compare file.");
    }
}

void clean() {
    fclose(fp_cmp);
    fclose(fp_tst);

    archive_destroy(archive);
    // archive_destroy(archive_read);
}

ErrorCode test_in_memory_operations(){
    /* Test for:
     * - archive create.
     * - change archive name & descritption.
     * - add {group,entry,field}.
     * - change {group,entry,field} name.
     * - change field content.
     * 
     * Things to check:
     * - modification dates.
     * - archive written status.
     * - vectors.
    */
    ErrorCode status = SUCCESS;
    status += assert("creating archive",
         (archive = archive_create("", "", fp_tst)) != NULL);
    status += assert("set archive name", archive_set_name(archive, "test_archive") == SUCCESS);
    status += assert("set archive description.", archive_set_description(archive, "test_archive_description") == SUCCESS);

    status += assert("checking archive name and description",
        strcmp(archive->name, "test_archive") == 0 && strcmp(archive->description, "test_archive_description") == 0);
    
    status += assert("create group", (group = group_create(archive, "test_group0")) != NULL);
    status += assert("create entry", (entry = entry_create(group, "test_entry0")) != NULL);
    status += assert("create field", (field = field_create(entry, TEXT, "test_field0")) != NULL);
    
    status += assert("vectors sized correctly", 
        archive->groups->size == 1 && 
        group->entries->size == 1 && group->fields->size == 1 &&
        entry->fields->size == 1 && entry->fields->size == 1);
    
    /* change {group,entry,field} name */
    status += assert("set group name", group_set_name(group, "test_group") == SUCCESS);
    status += assert("set entry name",entry_set_name(entry, "test_entry") == SUCCESS);
    status += assert("set field name",field_set_name(field, "test_field") == SUCCESS);

    status += assert("changing entity names", 
        strcmp(group->name, "test_group") == 0 && 
        strcmp(entry->name, "test_entry") == 0 && 
        strcmp(field->name, "test_field") == 0);
    
    char * field_content = "test_text";
    status += assert("set field content", field_set_content(field, TEXT, field_content) == SUCCESS);
    status += assert("check field content", strcmp((char *)field->content, field_content) == 0);

    // check if modification date is updated in all entities. The last modified entity is field and it is a child of all other entities.
    status += assert("check modification date is equal for all entities.",
         archive->last_modification_date == group->last_modification_date &&
         group->last_modification_date == entry->last_modification_date  &&
         entry->last_modification_date  == field->last_modification_date);

    // check archive written status
    status += assert("archive written status when archive is not written yet.", archive->written == 0);

    return (status != SUCCESS) ? FAILURE : SUCCESS;
}

ErrorCode test_search() {
    /* NOTE: in-memory operations test must work before this test */
    ErrorCode status = SUCCESS;

    // add dummy data to fill search tree.
    Field *dummy_field = field_create(entry, TEXT, "dummy_field1");
    field_set_content(dummy_field, TEXT ,"dummy_text");
    Entry *dummy_entry = entry_create(group, "dummy_entry1");
    Group *dummy_group = group_create(archive, "dummy_group1");

    // find group
    status += assert("check group is found by name", (Group *)archive_find_entity(archive, group->name)->entity == group);
    // find entry
    status += assert("check entry is found by name", (Entry *)archive_find_entity(archive, entry->name)->entity == entry);
    // find field
    status += assert("check field is found by name", (Field *)archive_find_entity(archive, field->name)->entity == field);
    status += assert("check field is found by content", (Field *)archive_find_entity(archive, field->content)->entity == field);

    field_delete(dummy_field);
    entry_delete(dummy_entry);
    group_delete(dummy_group);

    return (status != SUCCESS) ? FAILURE : SUCCESS;
}

ErrorCode test_write(){
    /* NOTE: in-memory operations test must work before this test */
    // resetting all dates to 0 in order to be compared with the reference file.
    archive->last_modification_date = archive->creation_date = 0;
    group->last_modification_date = group->creation_date = 0;
    entry->last_modification_date = entry->creation_date = 0;
    field->last_modification_date = field->creation_date = 0;

    ErrorCode status = SUCCESS;
    status += assert("writing archive", write_archive(archive) == SUCCESS);
    // modifying and writing again to have two directories
    group_set_name(group, "test_group1");
    entry_set_name(entry, "test_entry1");
    field_set_name(field, "test_field1");
    field_set_content(field, TEXT,"test_text1");

    archive->last_modification_date = archive->creation_date = 0;
    group->last_modification_date = group->creation_date = 0;
    entry->last_modification_date = entry->creation_date = 0;
    field->last_modification_date = field->creation_date = 0;

    status += assert("updating archive", write_archive(archive) == SUCCESS);
    status += assertFileEqual("check if file format is as expected", fp_cmp, fp_tst);

    return (status != SUCCESS) ? FAILURE : SUCCESS;
}

ErrorCode test_read(){
    /* NOTE: write test must work before this test */
    ErrorCode status = SUCCESS;
    archive_read = archive_create(archive->name, archive->description, fp_tst);

    if (archive_read == NULL) {
        errorp("archive_create returned null.");
        return FAILURE;
    }

    if (read_archive(archive_read, fp_tst) != SUCCESS) {
        errorp("archive read failed.");
        return FAILURE;
    }

    /* check all entities */
    /* archive */
    status += assert("check num of fields is equal", archive->fields->size == archive_read->fields->size);
    status += assert("check num of groups is equal", archive->groups->size == archive_read->groups->size && archive->groups->size == archive_read->groups->size);
    status += assert("check num of entries is equal", archive->entries->size == archive_read->entries->size && archive->entries->size == archive_read->entries->size);
    status += assert("check num of changes is equal", archive->num_of_changes == archive_read->num_of_changes);
    status += assert("check archive name", strcmp(archive->name, archive_read->name) == 0);
    status += assert("check archive description", strcmp(archive->description, archive_read->description) == 0);

    /* groups */
    group_read = (Group *) archive_find_entity(archive_read, group->name)->entity;
    status += assert("check if group exists", group_read != NULL); /* NOTE: this test depends on searching. */
    /* entries */
    entry_read = (Entry *) archive_find_entity(archive_read, entry->name)->entity;
    status += assert("check if entry exists", entry_read != NULL); /* NOTE: this test depends on searching. */
    /* fields */
    field_read = (Field *) archive_find_entity(archive_read, field->name)->entity;
    status += assert("check if field exists", field_read != NULL); /* NOTE: this test depends on searching. */
    status += assert("check if field content is same", strcmp(field->content, field_read->content) == 0);

    return (status != SUCCESS) ? FAILURE : SUCCESS;
}

ErrorCode test_history() {
    /* NOTE: read & write tests must work before this test */
    ErrorCode status = SUCCESS;
    // get versions vector.
    Vector *versions = archive_get_versions(archive);
    Archive *version0 = vector_at(versions, 0);

    // assert version data
    Group *old_group = vector_at(version0->groups, 0);
    Entry *old_entry = vector_at(version0->entries, 0);
    Field *old_field = vector_at(version0->fields, 0);
    assert("check group old name", strcmp(old_group->name, "test_group") == 0);
    assert("check entry old name", strcmp(old_entry->name, "test_entry") == 0);
    assert("check field old name", strcmp(old_field->name, "test_field") == 0);
    assert("check field old content", strcmp(old_field->content, "test_text") == 0);

    // clean
    archive_destroy(version0);
    vector_destroy(versions);
    return (status != SUCCESS) ? FAILURE : SUCCESS;
}

int main(){
    setup();

    printf("\n**** Checking in-memory operations ****\n");
    assert("**** check in-memory operations ****", test_in_memory_operations() == SUCCESS);

    printf("\n**** Checking search operations ****\n");
    assert("**** check search operations ****", test_search() == SUCCESS);

    printf("\n**** Checking archive writing ****\n");
    assert("**** check archive writing ****", test_write() == SUCCESS);

    printf("\n**** Checking archive read ****\n");
    assert("**** check archive read ****", test_read() == SUCCESS);

    printf("\n**** Checking history feature ****\n");
    assert("**** check history feature ****", test_history() == SUCCESS);

    clean();
}