#ifndef DATABASE_H
#define DATABASE_H
#include <stddef.h>
#include "../lang/Ast.h"
#include "../lang/lexer.h"

typedef enum {
    COL_TYPE_INT,
    COL_TYPE_TEXT,
    COL_TYPE_FLOAT,
    COL_TYPE_UNKNOWN
} ColumnType;

typedef enum {
    VAL_INT,
    VAL_TEXT,
    VAL_FLOAT
} ValueType;

typedef struct {
    ValueType type;
    union {
        int i_val;
        double f_val;
        char *s_val;
    } as;
} Value;

typedef struct {
    Value *values;
    size_t value_count;
} Row;

typedef struct {
    char *name;
    ColumnType type;
} ColumnsSchema;

typedef struct  {
    char *name;
    ColumnsSchema *columns;
    size_t column_count;
    Row *rows;
    size_t row_count;
} Table;

typedef struct  {
    Table *tables;
    size_t table_count;
} Database;

void db_init(Database *db);
void db_free(Database *db);
int execute_create_table(Database *db, ASTNode *node);
int execute_insert(Database *db, ASTNode *node);
void db_print(Database *db);
int execute_select(Database *db, ASTNode *node);
int row_matches_where(const Table *table, const Row *row, ASTNode *expr);
int execute_drop_table(Database *db, ASTNode *node);

#endif
