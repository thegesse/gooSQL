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
        Table *table = &db->tables[i];
        free(table->name);

        for (size_t j = 0; j < table->column_count; j++) {
            free(table->columns[j].name);
        }
        free(table->columns);

        for (size_t r = 0; r < table->row_count; r++) {
            Row *row = &table->rows[r];

            for (size_t v = 0; v < row->value_count; v++) {
                Value *val = &row->values[v];

                if (val->type == VAL_TEXT) {
                    free(val->as.s_val);
                }
            }
            free(row->values);
        }
        free(table->rows);
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

    Table *tmp = realloc(db->tables, sizeof(Table) * (db->table_count + 1));
    if (tmp == NULL) {
        return 0;
    }
    db->tables = tmp;

    Table *table = &db->tables[db->table_count];
    table->name = strdup(node->data.create_table.table_name);
    table->column_count = count;
    table->rows = NULL;
    table->row_count = 0;
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

int execute_insert(Database *db, ASTNode *node) {
    if (node == NULL || db == NULL || node->type != NODE_INSERT_STMT) {
        return 0;
    }
    Table *table = NULL;
    for (size_t i = 0; i < db->table_count; i++) {
        if (strcmp(db->tables[i].name, node->data.insert_stmt.table_name) == 0) {
            table = &db->tables[i];
            break;
        }
    }
    if (table == NULL) {
        fprintf(stderr, "Error: table '%s' not found\n",
                node->data.insert_stmt.table_name);
        return 0;
    }

    size_t value_count = 0;
    for (ASTNode *cur = node->data.insert_stmt.values; cur != NULL; cur = cur->data.list.next) {
        value_count++;
    }

    if (value_count != table->column_count) {
        fprintf(stderr, "Error: expected %zu values, got %zu\n",
                table->column_count, value_count);
        return 0;
    }

    Row new_row;
    new_row.values = malloc(sizeof(Value) * value_count);
    new_row.value_count = value_count;
    if (!new_row.values) {
        return 0;
    }

    size_t i = 0;
    for (ASTNode *cur = node->data.insert_stmt.values; cur != NULL; cur = cur->data.list.next, i++) {
        ASTNode *val_node = cur->data.list.current;
        ColumnType col_type = table->columns[i].type;
        Value *dest = &new_row.values[i];

        if (val_node->type == NODE_NUM) {
            if (col_type == COL_TYPE_INT) {
                dest->type = VAL_INT;
                dest->as.i_val = (int)val_node->data.d_val;
            } else if (col_type == COL_TYPE_FLOAT) {
                dest->type = VAL_FLOAT;
                dest->as.f_val = val_node->data.d_val;
            } else {
                fprintf(stderr, "Error: unsupported column type '%s'\n", table->columns[i].name);
                free(new_row.values);
                return 0;
            }
        } else if (val_node->type == NODE_STR) {
            if (col_type != COL_TYPE_TEXT) {
                fprintf(stderr, "Error: unsupported column type '%s'\n", table->columns[i].name);
                free(new_row.values);
                return 0;
            }
            dest->type = VAL_TEXT;
            dest->as.s_val = strdup(val_node->data.s_val);
        }
        else if (val_node->type == NODE_NULL) {
            fprintf(stderr, "Error: NULL values are not supported yet\n");
            free(new_row.values);
            return 0;
        }
        else {
            fprintf(stderr, "Error: unsupported value\n");
            free(new_row.values);
            return 0;
        }
    }
    Row *tmp = realloc(table->rows, sizeof(Row) * (table->row_count + 1));
    if (!tmp) {
        for (size_t j = 0; j < i; j++) {
            if (new_row.values[j].type == VAL_TEXT) {
                free(new_row.values[j].as.s_val);
            }
        }
        free(new_row.values);
        return 0;
    }
    table->rows = tmp;
    table->rows[table->row_count] = new_row;
    table->row_count++;
    return 1;
}

int row_matches_where(const Table *table, const Row *row, ASTNode *expr) {
    if (!table || !row || !expr) {
        return 0;
    }

    if (expr->type != NODE_BINARY) {
        return 0;
    }
    if (expr->op != '=' && expr->op != Eq) {
        return 0;
    }
    ASTNode *left = expr->data.binary.left;
    ASTNode *right = expr->data.binary.right;

    if (!left || left->type != NODE_ID || !right) {
        return 0;
    }

    //for col index
    int col_index = -1;
    for (size_t i = 0; i < table->column_count; i++) {
        if (strcmp(table->columns[i].name, left->data.s_val) == 0) {
            col_index = (int)i;
            break;
        }
    }
    if (col_index == -1) {
        return 0;//notfoundinnit
    }
    //rows
    Value *val = &row->values[col_index];
     if (right->type == NODE_NUM) {
         if (val->type == VAL_INT) {
             return val->as.i_val == (int)right->data.d_val;
         }
         if (val->type == VAL_FLOAT) {
             return val->as.f_val == right->data.d_val;
         }
         return 0;
     }
    if (right->type == NODE_STR) {
        if (val->type != VAL_TEXT || val->as.s_val == NULL) {
            return 0;
        }
        return strcmp(val->as.s_val, right->data.s_val) == 0;
    }
    return 0;
}

int execute_select(Database *db, ASTNode *node) {
    if (node == NULL || db == NULL || node->type != NODE_SELECT_STMT) {
        return 0;
    }
    Table *table = NULL;
    for (size_t i = 0; i < db->table_count; i++) {
        if (strcmp(db->tables[i].name, node->data.select_stmt.table_name) == 0) {
            table = &db->tables[i];
            break;
        }
    }
    if (table == NULL) {
        fprintf(stderr, "Error: table '%s' not found\n", node->data.select_stmt.table_name);
        return 0;
    }
    //SELECT * part
    ASTNode *select_list = node->data.select_stmt.select_list;

    if (select_list == NULL || select_list->data.list.current->type != NODE_WILDCARD) {
        //TODO remove this later when I can select objects aswell
        fprintf(stderr, "Error: only SELECT * is currently supported\n");
        return 0;
    }

    //print table
    printf("Table: %s\n", table->name);
    for (size_t i = 0; i < table->column_count; i++) {
        printf("%s", table->columns[i].name);
        if (i < table->column_count - 1) {
            printf(" | ");
        }
    }
    printf("\n");
    //rows
    if (table->row_count == 0) {
        printf("Rows: 0 \n");
        return 1;
    }
    for (size_t r = 0; r < table->row_count; r++) {
        Row *row = &table->rows[r];
        //check for where statement
        if (node->data.select_stmt.where_clause) {
            if (!row_matches_where(table, row, node->data.select_stmt.where_clause)) {
                continue;
            }
        }
        for (size_t i = 0; i < row->value_count; i++) {
            Value *value = &row->values[i];

            switch (value->type) {
                case VAL_INT:
                    printf("%d", value->as.i_val);
                    break;
                case VAL_FLOAT:
                    printf("%f", value->as.f_val);
                    break;
                case VAL_TEXT:
                    if (value->as.s_val != NULL) {
                        printf("%s", value->as.s_val);
                    } else {
                        printf("NULL");
                    }
                    break;
            }
            if (i < row->value_count - 1) {
                printf(" | ");
            }
        }
        printf("\n");
    }
    return 1;
}

//debug see if db storage works well
void db_print(Database *db) {
    if (!db || !db->tables) {
        return;
    }

    for (size_t i = 0; i < db->table_count; i++) {
        Table *table = &db->tables[i];
        printf("\nTable Name: %s\n", table->name);

        for (size_t j = 0; j < table->column_count; j++) {
            ColumnsSchema *col = &table->columns[j];
            printf("Column Name: %s, Type: %s\n", col->name, column_type_to_string(col->type));
        }
        if (table->row_count == 0) {
            printf("Rows: 0 \n");
        }
        for (size_t r = 0; r < table->row_count; r++) {
            Row *row = &table->rows[r];
            printf("Row %zu: ", r);

            for (size_t v = 0; v < row->value_count; v++) {
                Value *value = &row->values[v];
                switch (value->type) {
                    case VAL_INT:
                        printf("%d", value->as.i_val);
                        break;
                    case VAL_FLOAT:
                        printf("%f", value->as.f_val);
                        break;
                    case VAL_TEXT:
                        if (value->as.s_val) {
                            printf("%s", value->as.s_val);
                        } else {
                            printf("NULL");
                        }
                        break;
                    default:
                        printf("?");
                        break;
                }
                if (v < row->value_count - 1) {
                    printf(", ");
                }
            }
            printf("\n");
        }
    }
}
