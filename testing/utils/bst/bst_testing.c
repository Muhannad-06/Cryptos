#include <stdio.h>
#include <string.h>
#include "../../../include/utils/bst.h"
#include "../../../include/utils/assertion.h"
#include "../../../include/types.h"

BST *bst;

void set_up() {
    bst = bst_create();
    
    /* 
     * Workaround: Manually set root data to avoid NULL comparisons on first insert. 
     * Uses strdup so the BST destruction logic can safely free it. 
     */
    bst->root.text = strdup("50");
    bst->root.text_type = NAME;
    bst->root.entity_type = FIELD;
}

void tear_down() {
    bst_destroy(bst);
}

ErrorCode test_insert_new_nodes() {
    set_up();
    TNode *left = bst_insert(bst, "25", NULL, NAME, FIELD);
    TNode *right = bst_insert(bst, "75", NULL, NAME, FIELD);

    int r1 = assert("Insert should return the newly created left node", left != NULL);
    int r2 = assert("Insert should return the newly created right node", right != NULL);
    int r3 = assert("Root's left child should be 25", strcmp(bst->root.left->text, "25") == 0);
    int r4 = assert("Root's right child should be 75", strcmp(bst->root.right->text, "75") == 0);

    tear_down();
    return (r1 == SUCCESS && r2 == SUCCESS && r3 == SUCCESS && r4 == SUCCESS) ? SUCCESS : FAILURE;
}

ErrorCode test_duplicate_insertion() {
    set_up();
    bst_insert(bst, "30", NULL, NAME, FIELD);
    
    /* Attempting to insert a duplicate triggers a NULL return in the C implementation */
    TNode *dup = bst_insert(bst, "30", NULL, NAME, FIELD);
    int r1 = assert("Duplicate insertion should fail and return NULL", dup == NULL);
    
    tear_down();
    return (r1 == SUCCESS) ? SUCCESS : FAILURE;
}

ErrorCode test_delete_leaf_node() {
    set_up();
    bst_insert(bst, "20", NULL, NAME, FIELD);
    
    int r1 = assert("Delete leaf node should return SUCCESS", bst_delete(bst, "20") == SUCCESS);
    int r2 = assert("The left node should be null after being deleted", bst->root.left == NULL);
    
    tear_down();
    return (r1 == SUCCESS && r2 == SUCCESS) ? SUCCESS : FAILURE;
}

ErrorCode test_delete_node_with_one_child() {
    set_up();
    bst_insert(bst, "30", NULL, NAME, FIELD);
    bst_insert(bst, "20", NULL, NAME, FIELD); /* "30" now has a left child ("20") */
    
    int r1 = assert("Delete node with one child should return SUCCESS", bst_delete(bst, "30") == SUCCESS);
    int r2 = assert("Child node should be promoted", strcmp(bst->root.left->text, "20") == 0);
    
    tear_down();
    return (r1 == SUCCESS && r2 == SUCCESS) ? SUCCESS : FAILURE;
}

ErrorCode test_delete_root_node() {
    set_up();
    bst_insert(bst, "30", NULL, NAME, FIELD);
    bst_insert(bst, "70", NULL, NAME, FIELD);
    bst_insert(bst, "60", NULL, NAME, FIELD);
    
    int r1 = assert("Should successfully delete the root", bst_delete(bst, "50") == SUCCESS);
    int r2 = assert("Root should be updated to the smallest right branch value", strcmp(bst->root.text, "60") == 0);
    
    tear_down();
    return (r1 == SUCCESS && r2 == SUCCESS) ? SUCCESS : FAILURE;
}

ErrorCode test_contains_unfound_node() {
    set_up();
    
    int r1 = assert("Should return false (0) for a non-existent value", bst_contains(bst, "999") == 0);
    
    tear_down();
    return (r1 == SUCCESS) ? SUCCESS : FAILURE;
}

int main() {
    printf("Running BST Module Tests for Simple Archive...\n");

    assert("** testInsertNewNodes", test_insert_new_nodes() == SUCCESS);
    assert("** testFindParentThrowsExceptionOnDuplicate", test_duplicate_insertion() == SUCCESS);
    assert("** testDeleteLeafNode", test_delete_leaf_node() == SUCCESS);
    assert("** testDeleteNodeWithOneChild", test_delete_node_with_one_child() == SUCCESS);
    assert("** testDeleteRootNode", test_delete_root_node() == SUCCESS);
    assert("** testContainsUnfoundNode", test_contains_unfound_node() == SUCCESS);

    printf("BST testing complete.\n");
    return 0;
}