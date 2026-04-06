#ifndef AST_H
#define AST_H

typedef enum {
    NODE_ID,
    NODE_NUM,
    NODE_STR,
    NODE_NULL,
    NODE_WILDCARD,
    NODE_BINARY,
    NODE_COLUMN_LIST,
    NODE_SELECT_STMT,
    NODE_CREATE_TABLE_STMT,
    NODE_COLUMN_DEF,
    NODE_INSERT_STMT,
    NODE_DROP_TABLE_STMT,
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
        struct {
            char *table_name;
            struct ASTNode *columns;
        } create_table;
        struct {
            char *column_name;
            char *type_name;
        } column_def;
        struct {
            char *table_name;
            struct ASTNode *values;
            struct ASTNode *columns;
        } insert_stmt;
        struct {
            char *table_name;
        } drop_table_stmt;
    } data;
} ASTNode;


ASTNode* create_node(NodeType type);
void free_node(ASTNode *node);
void print_node(ASTNode *node);
void print_ast(ASTNode *node, int indent);
#endif
