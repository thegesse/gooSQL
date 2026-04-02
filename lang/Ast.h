#ifndef AST_H
#define AST_H

#include<stdlib.h>
typedef enum {
    NODE_ID,
    NODE_NUM,
    NODE_STR,
    NODE_NULL,
    NODE_WILDCARD,
    NODE_BINARY,
    NODE_COLUMN_LIST,
    NODE_SELECT_STMT
} NodeType;

typedef struct ASTNode {
    NodeType type;
    int op;
    union {
        char* s_val;
        double d_val;
        struct {
            struct ASTNode* left;
            struct ASTNode* right;
        } binary;
        struct {
            struct ASTNode* current;
            struct ASTNode* next;
        } list;
        struct {
            struct ASTNode* select_list;
            char* table_name;
            struct ASTNode* where_clause;
        } select_stmt;
    } data;
} ASTNode;


ASTNode* create_node(NodeType type);
void free_node(ASTNode *node);
#endif
