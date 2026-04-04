#include "Databse.h"
#include "../lang/Ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void db_init( Database *db) {
    db->tables = NULL;
    db->table_count = 0;
}

void db_free( Database *db) {
    if (!db || !db->tables) {
        return;
    }
    for (size_t i = 0; i < db->table_count; i++) {
        TableSchema *table = &db->tables[i];
        free(table->name);

        for (size_t j = 0; j < table->column_count; j++) {
            free(table->columns[j].name);
            free(table->columns[j].type);
        }
        free(table->columns);
    }
    free(db->tables);
    db->tables = NULL;
    db->table_count = 0;
}

int execute_create_table(Database *db, ASTNode *node) {
    if (node == NULL || db == NULL || node->type != NODE_CREATE_TABLE_STMT) {
        return 0;
    }
    size_t count = 0;
    for (ASTNode *current = node->data.create_table.columns; current != NULL; current = current->data.list.next) {
        count++;
    }

    TableSchema *tmp = realloc(db->tables, sizeof(TableSchema) * (db->table_count + 1));
    if (tmp == NULL) {
        return 0;
    }
    db->tables = tmp;

    TableSchema *table = &db->tables[db->table_count];
    table->name = strdup(node->data.create_table.table_name);
    table->column_count = count;
    table->columns = malloc(sizeof(ColumnsSchema) * count);

    if (table->name == NULL || table->columns == NULL) {
        free(table->name);
        free(table->columns);
        return 0;
    }

    size_t i = 0;
    for (ASTNode *current = node->data.create_table.columns; current != NULL; current = current->data.list.next) {
        ASTNode *col = current->data.list.current;
        table->columns[i].name = strdup(col->data.column_def.column_name);
        table->columns[i].type = strdup(col->data.column_def.type_name);

        if (table->columns[i].name == NULL || table->columns[i].type == NULL) {
            return 0;
        }
        i++;
    }
    db->table_count++;
    return 1;
}

//debug see if db storage works well
void db_print(Database *db) {
    if (!db || !db->tables) {
        return;
    }

    for (size_t i = 0; i < db->table_count; i++) {
        TableSchema *table = &db->tables[i];
        printf("\nTable Name: %s\n", table->name);

        for (size_t j = 0; j < table->column_count; j++) {
            ColumnsSchema *col = &table->columns[j];
            printf("Column Name: %s, Type: %s\n", col->name, col->type);
        }
    }
}