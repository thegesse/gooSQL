#include "Databse.h"
#include "../lang/Ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void db_init( Database *db) {
    db->tables = NULL;
    db->table_count = 0;
}

//helpers
ColumnType parse_column_type(const char *type_name) {
    if (type_name == NULL) {
        return COL_TYPE_UNKNOWN;
    }

    if (strcmp(type_name, "INT") == 0) {
        return COL_TYPE_INT;
    }
    if (strcmp(type_name, "FLOAT") == 0) {
        return COL_TYPE_FLOAT;
    }
    if (strcmp(type_name, "TEXT") == 0) {
        return COL_TYPE_TEXT;
    }
    return COL_TYPE_UNKNOWN;
}

const char *column_type_to_string(ColumnType type) {
    switch (type) {
        case COL_TYPE_INT:
            return "INT";
            break;
        case COL_TYPE_FLOAT:
            return "FLOAT";
            break;
        case COL_TYPE_TEXT:
            return "TEXT";
            break;
        case COL_TYPE_UNKNOWN:
            return "UNKNOWN";
            break;
       default:
            return "UNKNOWN";
    }
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

    //dupllicate table names
    for (size_t i = 0; i < db->table_count; i++) {
        if (strcmp(db->tables[i].name, node->data.create_table.table_name) == 0) {
            return 0; //if table exists
        }
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

        for (size_t j = 0; j<i; j++) {
            if (strcmp(table->columns[j].name, col->data.column_def.column_name) == 0) {
                return 0; //duplicate col
            }
        }
        table->columns[i].name = strdup(col->data.column_def.column_name);
        if (table->columns[i].name == NULL) {
            return 0;
        }

        ColumnType parsed_type = parse_column_type(col->data.column_def.type_name);
        if (parsed_type == COL_TYPE_UNKNOWN) {
            fprintf(stderr, "Error: unknown column type '%s'\n", col->data.column_def.type_name);
            return 0;
        }

        table->columns[i].type = parsed_type;
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
            printf("Column Name: %s, Type: %s\n", col->name, column_type_to_string(col->type));
        }
    }
}