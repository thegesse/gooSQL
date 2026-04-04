#ifndef DATABASE_H
#define DATABASE_H
#include <stddef.h>
#include "../lang/Ast.h"

typedef struct {
    char *name;
    char *type;
} ColumnsSchema;

typedef struct  {
    char *name;
    ColumnsSchema *columns;
    size_t column_count;
} TableSchema;

typedef struct  {
    TableSchema *tables;
    size_t table_count;
} Database;

void db_init(Database *db);
void db_free(Database *db);
int execute_create_table(Database *db, ASTNode *node);
void db_print(Database *db);

#endif