#include <stdio.h>
#include "lang/lexer.h"
#include "lang/Parser.h"
#include "lang/Ast.h"
#include "runtime/Databse.h"
//ai written test case (note to self learn how to write test cases)
static int run_statement(Database *db, const char *label, const char *sql, int expected_success) {
    struct Parser parser = parser_init(sql);
    ASTNode *node = parse_statement(&parser);
    int success = 0;

    if (node != NULL) {
        switch (node->type) {
            case NODE_CREATE_TABLE_STMT:
                success = execute_create_table(db, node);
                break;
            case NODE_INSERT_STMT:
                success = execute_insert(db, node);
                break;
            case NODE_SELECT_STMT:
                success = execute_select(db, node);
                break;
            default:
                success = 0;
                break;
        }
    }

    printf("[%s] %s\n", success == expected_success ? "PASS" : "FAIL", label);
    printf("  SQL: %s\n", sql);
    printf("  Expected: %s | Got: %s\n",
           expected_success ? "success" : "failure",
           success ? "success" : "failure");

    if (node != NULL) {
        print_ast(node, 0);
    }

    free_node(node);
    return success == expected_success;
}

int main(void) {
    int passed = 0;
    int total = 0;
    Database db;

    init_keywords();
    db_init(&db);

    total++;
    passed += run_statement(&db, "Create table",
                            "CREATE TABLE users (id INT, name TEXT);", 1);

    total++;
    passed += run_statement(&db, "Insert valid row",
                            "INSERT INTO users VALUES (1, 'bob');", 1);

    total++;
    passed += run_statement(&db, "Select all rows",
                            "SELECT * FROM users;", 1);

    total++;
    passed += run_statement(&db, "Where by id matches",
                            "SELECT * FROM users WHERE id = 1;", 1);

    total++;
    passed += run_statement(&db, "Where by id misses",
                            "SELECT * FROM users WHERE id = 2;", 1);

    total++;
    passed += run_statement(&db, "Where by name matches",
                            "SELECT * FROM users WHERE name = 'bob';", 1);

    total++;
    passed += run_statement(&db, "Where by name misses",
                            "SELECT * FROM users WHERE name = 'alice';", 1);

    total++;
    passed += run_statement(&db, "Reject invalid type",
                            "CREATE TABLE bad (id ASDASD);", 0);

    total++;
    passed += run_statement(&db, "Reject duplicate table",
                            "CREATE TABLE users (id INT, name TEXT);", 0);

    total++;
    passed += run_statement(&db, "Reject wrong insert count",
                            "INSERT INTO users VALUES (1);", 0);

    total++;
    passed += run_statement(&db, "Reject wrong insert type",
                            "INSERT INTO users VALUES ('bob', 1);", 0);

    total++;
    passed += run_statement(&db, "Reject missing table",
                            "INSERT INTO missing VALUES (1, 'bob');", 0);

    printf("\nSummary: %d/%d tests passed\n", passed, total);
    printf("\nDatabase state after tests:\n");

    db_print(&db);
    db_free(&db);
    free_keywords();
    return 0;
}
