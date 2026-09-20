/* binary search tree that hold text and a pointer to entity that contain that text.*/

#ifndef BST_H
#define BST_H
#include <stdlib.h>
#include "../types.h"

typedef enum{
    GROUP,
    ENTRY,
    FIELD,
} EntityType;

typedef enum{
    NAME,
    CONTENT,
} TextType;

/* Node */
typedef struct TNode{
    char *text;
    void *entity;
    TextType text_type;
    EntityType entity_type;
    struct TNode *left;
    struct TNode *right;
} TNode;

/* Node functions. */
TNode *bst_node_create(char *text, void *entity,TextType text_type, EntityType entity_type);
void bst_node_destroy(TNode *node);

/* BST */
typedef struct{
    TNode root;
} BST;

BST *bst_create();
void bst_destroy(BST* bst);
TNode *bst_find_parent(BST* bst, char *text);
TNode *bst_find(BST* bst, char *text);
int bst_contains(BST* bst, char *text);
TNode *bst_insert_node(BST* bst, TNode *node);
TNode *bst_insert(BST* bst, char *text, void *entity,TextType text_type, EntityType entity_type);
ErrorCode bst_delete_node(BST* bst, TNode* node);
ErrorCode bst_delete(BST* bst, char *text);

#endif