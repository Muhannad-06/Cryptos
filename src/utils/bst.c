#include "../../include/utils/bst.h"
#include "utils/error.h"
#include <string.h>
#include <stdio.h>

TNode *bst_node_create(char *text, void *entity, TextType text_type, EntityType entity_type) {
    TNode *node = (TNode *)malloc(sizeof(TNode));
    if (!node) return NULL;

    // Use strdup to take memory ownership of the string
    node->text = text ? strdup(text) : NULL;
    node->entity = entity;
    node->text_type = text_type;
    node->entity_type = entity_type;
    node->left = NULL;
    node->right = NULL;

    return node;
}

void bst_node_destroy(TNode *node) {
    if (!node) return;
    if (node->text) free(node->text);
    free(node);
}

BST *bst_create() {
    BST *bst = (BST *)malloc(sizeof(BST));
    if (!bst) return NULL;

    // Initialize the inline dummy root
    bst->root.text = NULL;
    bst->root.entity = NULL;
    bst->root.left = NULL;
    bst->root.right = NULL;

    return bst;
}

// Recursive helper to clean up nodes without leaking memory
static void _bst_destroy_recursive(TNode *node) {
    if (!node) return;
    _bst_destroy_recursive(node->left);
    _bst_destroy_recursive(node->right);
    bst_node_destroy(node);
}

void bst_destroy(BST* bst) {
    if (!bst) return;
    _bst_destroy_recursive(bst->root.left);
    _bst_destroy_recursive(bst->root.right);
    if (bst->root.text) free(bst->root.text);
    free(bst);
}

TNode *bst_find_parent(BST *bst, char *text) {
    if (!bst || !bst->root.text || !text) return NULL;

    TNode *parent = NULL;
    TNode *curr = &(bst->root);

    while (curr != NULL) {
        int comp = strcmp(text, curr->text);
        if (comp == 0) return parent; // Exact match found, return its actual parent
        
        parent = curr;
        if (comp < 0) curr = curr->left;
        else curr = curr->right;
    }
    
    return parent; // Return the node that *would* be the parent if inserted
}

TNode *bst_find(BST *bst, char *text) {
    if (!bst || !bst->root.text || !text) return NULL;

    TNode *curr = &(bst->root);
    while (curr != NULL) {
        int comp = strcmp(text, curr->text);
        if (comp == 0) return curr;
        else if (comp < 0) curr = curr->left;
        else curr = curr->right;
    }
    return NULL;
}

int bst_contains(BST *bst, char *text) {
    return bst_find(bst, text) != NULL ? 1 : 0;
}

TNode *bst_insert_node(BST *bst, TNode *node) {
    if (!bst || !node || !node->text) return NULL;

    // Handle an entirely empty tree
    if (bst->root.text == NULL) {
        bst->root.text = strdup(node->text);
        bst->root.entity = node->entity;
        bst->root.text_type = node->text_type;
        bst->root.entity_type = node->entity_type;
        bst->root.left = node->left;
        bst->root.right = node->right;
        
        bst_node_destroy(node); // The root took its data, safely free the wrapper
        return &(bst->root);
    }

    TNode *parent = NULL;
    TNode *curr = &(bst->root);
    int comp;

    while (curr != NULL) {
        parent = curr;
        comp = strcmp(node->text, curr->text);
        
        if (comp < 0) curr = curr->left;
        else if (comp > 0) curr = curr->right;
        else return NULL; // Duplicate node exists
    }

    if (comp < 0) parent->left = node;
    else parent->right = node;

    return node;
}

TNode *bst_insert(BST *bst, char *text, void *entity, TextType text_type, EntityType entity_type) {
    if (bst_contains(bst, text)) {
        // errorp("bst_insert: \"%s\" already exists in binary search tree", text);
        /* NOTE: now if entities have the same name in archive, only one is in search tree */
        return NULL;
    }
    TNode *node = bst_node_create(text, entity, text_type, entity_type);
    if (!node) return NULL;

    TNode *inserted = bst_insert_node(bst, node);
    if (!inserted) bst_node_destroy(node); // Prevents memory leak on duplicate keys

    return inserted;
}

ErrorCode bst_delete_node(BST *bst, TNode *node) {
    if (!bst || !node) return FAILURE;

    TNode *parent = NULL;
    TNode *curr = &(bst->root);

    // Track the parent down since TNode lacks a parent pointer
    while (curr != NULL && curr != node) {
        parent = curr;
        int comp = strcmp(node->text, curr->text);
        if (comp < 0) curr = curr->left;
        else curr = curr->right;
    }

    if (curr != node) return FAILURE;

    // Deletion Cases
    if (node->left == NULL) {
        if (parent == NULL) { // Deleting the inline root node
            if (node->right == NULL) {
                free(bst->root.text);
                bst->root.text = NULL;
                bst->root.entity = NULL;
            } else {
                TNode *temp = bst->root.right;
                free(bst->root.text);
                bst->root.text = temp->text; // Inherit ownership
                bst->root.entity = temp->entity;
                bst->root.text_type = temp->text_type;
                bst->root.entity_type = temp->entity_type;
                bst->root.left = temp->left;
                bst->root.right = temp->right;
                free(temp); 
            }
        } else if (parent->left == node) {
            parent->left = node->right;
            bst_node_destroy(node);
        } else {
            parent->right = node->right;
            bst_node_destroy(node);
        }
    } else if (node->right == NULL) {
        if (parent == NULL) { // Deleting the inline root node
            TNode *temp = bst->root.left;
            free(bst->root.text);
            bst->root.text = temp->text;
            bst->root.entity = temp->entity;
            bst->root.text_type = temp->text_type;
            bst->root.entity_type = temp->entity_type;
            bst->root.left = temp->left;
            bst->root.right = temp->right;
            free(temp);
        } else if (parent->left == node) {
            parent->left = node->left;
            bst_node_destroy(node);
        } else {
            parent->right = node->left;
            bst_node_destroy(node);
        }
    } else {
        // Node has two children: Find the in-order successor
        TNode *succParent = node;
        TNode *succ = node->right;
        while (succ->left != NULL) {
            succParent = succ;
            succ = succ->left;
        }

        // Swap string/payload data securely
        free(node->text);
        node->text = strdup(succ->text);
        node->entity = succ->entity;
        node->text_type = succ->text_type;
        node->entity_type = succ->entity_type;

        // Sever the successor
        if (succParent->left == succ) succParent->left = succ->right;
        else succParent->right = succ->right;

        bst_node_destroy(succ);
    }

    return SUCCESS;
}

ErrorCode bst_delete(BST *bst, char *text) {
    TNode *node = bst_find(bst, text);
    if (!node) return FAILURE;
    return bst_delete_node(bst, node);
}