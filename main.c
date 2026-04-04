#include <stdio.h>
#include "lang/lexer.h"
#include "lang/Parser.h"
#include "lang/Ast.h"
#include "runtime/Databse.h"

int main(void) {
    init_keywords();
    Database db;
    db_init(&db);

    const char *sql = "CREATE TABLE test (id INT, name TEXT, age INT, extra ASDASD);";
    struct Parser parser = parser_init(sql);
    ASTNode *root = parse_statement(&parser);

    if (root != NULL) {
        printf("Parse success\n");
        if (execute_create_table(&db, root)) {
            printf("Create table success\n");
            db_print(&db);
        }
    } else {
        printf("Parse failed\n");
    }
    print_ast(root, 0);
    free_keywords();
    free_node(root);
    db_free(&db);
    return 0;
}