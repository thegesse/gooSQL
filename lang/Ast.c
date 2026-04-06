#include <stdio.h>
#include <stdlib.h>
#include "Ast.h"
#include "lexer.h"

#include <string.h>

//a bit barbaric but fuck it
static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        putchar(' ');
    }
}

ASTNode* create_node(NodeType type) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "malloc failed\n");
        return NULL;
    }
    //set all bites to 0 == memset
    memset(node, 0, sizeof(ASTNode));
    //then overwrite everything with the current type
    node->type = type;
    return node;
}

void free_node(ASTNode *node) {
    if (node == NULL) {
        return;
    }

    switch (node->type) {
        case NODE_ID:
            free(node->data.s_val);
            break;
        case NODE_STR:
            free(node->data.s_val);
            break;
        case NODE_NUM:
            break;
        case NODE_NULL:
            break;
        case NODE_WILDCARD:
            break;
        case NODE_BINARY:
            free_node(node->data.binary.left);
            free_node(node->data.binary.right);
            break;
        case NODE_COLUMN_LIST:
            free_node(node->data.list.current);
            free_node(node->data.list.next);
            break;
        case NODE_SELECT_STMT:
            free_node(node->data.select_stmt.select_list);
            free(node->data.select_stmt.table_name);
            free_node(node->data.select_stmt.where_clause);
            break;
        case NODE_CREATE_TABLE_STMT:
            free(node->data.create_table.table_name);
            free_node(node->data.create_table.columns);
            break;
        case NODE_COLUMN_DEF:
            free(node->data.column_def.column_name);
            free(node->data.column_def.type_name);
            break;
        case NODE_INSERT_STMT:
            free(node->data.insert_stmt.table_name);
            free_node(node->data.insert_stmt.values);
            free_node(node->data.insert_stmt.columns);
            break;
    }
    free(node);
}
//helper for operators
void print_operator(int op) {
    switch (op) {
        case '=':
            printf("=");
            break;
        case '<':
            printf("<");
            break;
        case '>':
            printf(">");
            break;
        case Ne:
            printf("!=");
            break;
        case Le:
            printf("<=");
            break;
        case Ge:
            printf(">=");
            break;
        default:
            printf("UNKNOWN_OP(%d)\n", op);
            break;
    }
}


//debug viewer for AST
void print_ast(ASTNode *node, int indent) {
    if (node == NULL) {
        print_indent(indent);
        printf("NULL\n");
        return;
    }

    switch (node->type) {
        case NODE_ID:
            print_indent(indent);
            printf("ID: %s\n", node->data.s_val);
            break;
        case NODE_STR:
            print_indent(indent);
            printf("STR: %s\n", node->data.s_val);
            break;
        case NODE_NUM:
            print_indent(indent);
            printf("NUM: %f\n", node->data.d_val);
            break;
        case NODE_NULL:
            print_indent(indent);
            printf("NULL\n");
            break;
        case NODE_WILDCARD:
            print_indent(indent);
            printf("*\n");
            break;
        case NODE_BINARY:
            print_indent(indent);
            printf("BINARY_OP: ");
            print_operator(node->op);
            printf("\n");

            //staircase indents
            print_indent(indent + 2);
            printf("LEFT:\n");
            print_ast(node->data.binary.left, indent + 4);

            print_indent(indent + 2);
            printf("RIGHT:\n");
            print_ast(node->data.binary.right, indent + 4);
            break;
        case NODE_COLUMN_LIST:
            print_ast(node->data.list.current, indent);
            printf("- ");
            print_ast(node->data.list.current, 0);
            if (node->data.list.next != NULL) {
                print_ast(node->data.list.next, indent);
            }
            break;
        case NODE_SELECT_STMT:
            print_indent(indent);
            //TODO replace later when adding create table or drop table statements
            printf("SELECT_STMT: \n");

            print_indent(indent + 2);
            printf("COLUMNS:\n");
            print_ast(node->data.select_stmt.select_list, indent + 4);

            print_indent(indent + 2);
            printf("TABLE: %s\n", node->data.select_stmt.table_name);

            if (node->data.select_stmt.where_clause != NULL) {
                print_indent(indent + 2);
                printf("WHERE_CLAUSE:\n");
                print_ast(node->data.select_stmt.where_clause, indent + 4);
            }
            break;
        case NODE_CREATE_TABLE_STMT:
            print_indent(indent);
            printf("CREATE_TABLE_STMT: \n");

            print_indent(indent + 2);
            printf("TABLE: %s\n", node->data.create_table.table_name);

            print_indent(indent + 4);
            printf("COLUMNS:\n");
            print_ast(node->data.create_table.columns, indent + 4);
            break;
        case NODE_COLUMN_DEF:
            print_indent(indent);
            printf("COLUMN_DEF: \n");
            print_indent(indent + 2);
            printf("column name %s\n", node->data.column_def.column_name);
            print_indent(indent + 4);
            printf("DEF: %s\n", node->data.column_def.type_name);
            break;
        case NODE_INSERT_STMT:
            print_indent(indent);
            printf("INSERT_STMT:\n");
            print_indent(indent + 2);
            printf("TABLE: %s\n", node->data.insert_stmt.table_name);
            if (node->data.insert_stmt.columns != NULL) {
                print_indent(indent + 2);
                printf("COLUMNS:\n");
                print_ast(node->data.insert_stmt.columns, indent + 4);
            }
            print_indent(indent + 2);
            printf("VALUES:\n");
            print_ast(node->data.insert_stmt.values, indent + 4);
            break;
    }
}

//switch cases bc no polymorphism (fuck manual polymorphism, fuck C, fuck this project)
