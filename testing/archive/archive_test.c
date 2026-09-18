/* checking archive functionality. 
* NOTE: This test is for fields of TEXT type.
*/
#include <stdarg.h>
// #include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../../include/types.h"
#include "../../../include/utils/error.h"
#include "../../../include/utils/assertion.h"
#include "../../../include/archive/archive.h"
// #include "../../../include/archive/field.h"
#include "../../../include/archive/entry.h"
// #include "../../../include/archive/archive.h"

FILE * fp_cmp;
FILE * fp_tst;
Archive * archive;
Group *group;
Entry * entry;
Field * field;

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

    // field_destroy(field);
    archive_destroy(archive);
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

    /* #TODO: testing search feature */
    
    return (status != SUCCESS) ? FAILURE : SUCCESS;
}

ErrorCode test_search() {
    ErrorCode status = SUCCESS;

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
    ErrorCode status = SUCCESS;
    
    return (status != SUCCESS) ? FAILURE : SUCCESS;
}

ErrorCode test_history() {
    ErrorCode status = SUCCESS;

    return (status != SUCCESS) ? FAILURE : SUCCESS;
}

int main(){
    setup();

    printf("\n**** Checking in-memory operations ****\n");
    assert("**** check in-memory operations ****", test_in_memory_operations() == SUCCESS);

    printf("\n**** Checking archive writing ****\n");
    assert("**** check archive writing ****", test_write() == SUCCESS);

    clean();
}