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

    if (!table) {
        fprintf(stderr, "Error: table '%s' not found\n",
                node->data.insert_stmt.table_name);
        return 0;
    }

    // Count values
    size_t value_count = 0;
    for (ASTNode *cur = node->data.insert_stmt.values; cur; cur = cur->data.list.next) {
        value_count++;
    }

    ASTNode *columns = node->data.insert_stmt.columns;

    //no column list
    if (columns == NULL) {
        if (value_count != table->column_count) {
            fprintf(stderr, "Error: expected %zu values, got %zu\n",
                    table->column_count, value_count);
            return 0;
        }
    }

    //column list present
    int column_map[table->column_count];
    size_t column_count = 0;

    if (columns != NULL) {
        int used[table->column_count];
        memset(used, 0, sizeof(used));

        ASTNode *cur = columns;

        while (cur) {
            ASTNode *col_node = cur->data.list.current;

            if (!col_node || col_node->type != NODE_ID) {
                fprintf(stderr, "Error: invalid column in INSERT\n");
                return 0;
            }

            int found = -1;
            for (size_t i = 0; i < table->column_count; i++) {
                if (strcmp(table->columns[i].name, col_node->data.s_val) == 0) {
                    found = (int)i;
                    break;
                }
            }

            if (found == -1) {
                fprintf(stderr, "Error: column '%s' not found\n",
                        col_node->data.s_val);
                return 0;
            }

            if (used[found]) {
                fprintf(stderr, "Error: duplicate column '%s'\n",
                        col_node->data.s_val);
                return 0;
            }

            used[found] = 1;
            column_map[column_count++] = found;

            cur = cur->data.list.next;
        }

        // 🔥 KEY RULE: must provide ALL columns
        if (column_count != table->column_count) {
            fprintf(stderr,
                    "Error: must provide all columns (%zu), got %zu\n",
                    table->column_count, column_count);
            return 0;
        }

        if (column_count != value_count) {
            fprintf(stderr,
                    "Error: column count (%zu) != value count (%zu)\n",
                    column_count, value_count);
            return 0;
        }
    }

    // Allocate full row
    Row new_row;
    new_row.value_count = table->column_count;
    new_row.values = malloc(sizeof(Value) * table->column_count);
    if (!new_row.values) return 0;


    ASTNode *val_cur = node->data.insert_stmt.values;
    size_t i = 0;

    while (val_cur) {
        ASTNode *val_node = val_cur->data.list.current;

        int target_index;
        if (columns == NULL) {
            target_index = (int)i;
        } else {
            target_index = column_map[i];
        }

        ColumnType col_type = table->columns[target_index].type;
        Value *dest = &new_row.values[target_index];

        if (val_node->type == NODE_NUM) {
            if (col_type == COL_TYPE_INT) {
                dest->type = VAL_INT;
                dest->as.i_val = (int)val_node->data.d_val;
            } else if (col_type == COL_TYPE_FLOAT) {
                dest->type = VAL_FLOAT;
                dest->as.f_val = val_node->data.d_val;
            } else {
                fprintf(stderr, "Type mismatch for column '%s'\n",
                        table->columns[target_index].name);
                free(new_row.values);
                return 0;
            }
        } else if (val_node->type == NODE_STR) {
            if (col_type != COL_TYPE_TEXT) {
                fprintf(stderr, "Type mismatch for column '%s'\n",
                        table->columns[target_index].name);
                free(new_row.values);
                return 0;
            }
            dest->type = VAL_TEXT;
            dest->as.s_val = strdup(val_node->data.s_val);
        } else {
            fprintf(stderr, "Error: unsupported value\n");
            free(new_row.values);
            return 0;
        }

        val_cur = val_cur->data.list.next;
        i++;
    }


    Row *tmp = realloc(table->rows, sizeof(Row) * (table->row_count + 1));
    if (!tmp) {
        free(new_row.values);
        return 0;
    }

    table->rows = tmp;
    table->rows[table->row_count++] = new_row;

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

    int use_wildcard = 0;
    if (select_list != NULL) {
        if (select_list->type == NODE_WILDCARD) {
            use_wildcard = 1;
        } else if (select_list->type == NODE_COLUMN_LIST &&
                   select_list->data.list.current != NULL &&
                   select_list->data.list.current->type == NODE_WILDCARD) {
            use_wildcard = 1;
                   }
    }

    int selected_indexes[table->column_count];
    size_t selected_count = 0;
    if (!use_wildcard) {
        if (select_list->type == NODE_ID) {
            int found = -1;
            for (size_t i = 0; i < table->column_count; i++) {
                if (strcmp(table->columns[i].name, select_list->data.s_val) == 0) {
                    found = (int)i;
                    break;
                }
            }
            if (found == -1) {
                fprintf(stderr, "Error: col %s not found\n", select_list->data.s_val);
                return 0;
            }
            selected_indexes[selected_count++] = found;
        } else if (select_list->type == NODE_COLUMN_LIST) {
            ASTNode *curr = select_list;

            while (curr) {
                ASTNode *item = curr->data.list.current;

                if (item == NULL || item->type != NODE_ID) {
                    fprintf(stderr, "Error: invalid selected column in SELECT\n");
                    return 0;
                }

                int found = -1;
                for (size_t i = 0; i < table->column_count; i++) {
                    if (strcmp(table->columns[i].name, item->data.s_val) == 0) {
                        found = (int)i;
                        break;
                    }
                }

                if (found == -1) {
                    fprintf(stderr, "Error: col %s not found\n", item->data.s_val);
                    return 0;
                }

                selected_indexes[selected_count++] = found;
                curr = curr->data.list.next;
            }
        } else {
            fprintf(stderr, "Error: invalid selected column in SELECT\n");
            return 0;
        }
    }

    //print table
    printf("Table: %s\n", table->name);
    if (use_wildcard) {
        for (size_t i = 0; i < table->column_count; i++) {
            printf("%s", table->columns[i].name);
            if (i < table->column_count - 1) {
                printf(" | ");
            }
        }
    } else {
        for (size_t i = 0; i < selected_count; i++) {
            printf("%s", table->columns[selected_indexes[i]].name);
            if (i < selected_count - 1) {
                printf(" | ");
            }
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
        if (use_wildcard) {
            for (size_t i = 0; i < table->column_count; i++) {
                Value *value = &row->values[i];
                //1 line statements for my sanity
                switch (value->type) {
                    case VAL_INT: printf("%d", value->as.i_val); break;
                    case VAL_FLOAT: printf("%f", value->as.f_val); break;
                    case VAL_TEXT:
                        printf("%s", value->as.s_val ? value->as.s_val : "NULL");
                        break;
                }
                if (i < row->value_count - 1) printf(" | ");
            }
        } else {
            for (size_t i = 0; i < selected_count; i++) {
                Value *value = &row->values[selected_indexes[i]];

                switch (value->type) {
                    case VAL_INT: printf("%d", value->as.i_val); break;
                    case VAL_FLOAT: printf("%f", value->as.f_val); break;
                    case VAL_TEXT:
                        printf("%s", value->as.s_val ? value->as.s_val : "NULL");
                        break;
                }
                if (i < selected_count - 1) printf(" | ");
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
