#include <stdio.h>
#include "lang/lexer.h"
#include "lang/Parser.h"
#include "lang/Ast.h"
#include "runtime/Databse.h"

int main(void) {
    init_keywords();
    Database db;
    db_init(&db);

    const char *sql = "CREATE TABLE users (id INT, name TEXT);";
    const char *insert = "INSERT INTO users VALUES (1, 'bob');";

    struct Parser parser = parser_init(sql);
    ASTNode *root = parse_statement(&parser);

    struct Parser parser2 = parser_init(insert);
    ASTNode *node = parse_statement(&parser2);

    if (root != NULL && execute_create_table(&db, root)) {
        printf("Create table success\n");
    } else {
        printf("Create table failed\n");
    }

    if (node != NULL && execute_insert(&db, node)) {
        printf("Insert success\n");
    } else {
        printf("Insert failed\n");
    }

    db_print(&db);

    print_ast(root, 0);
    print_ast(node, 0);

    free_node(root);
    free_node(node);
    return 0;
}