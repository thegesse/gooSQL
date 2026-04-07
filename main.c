#include <stdio.h>
#include "lang/lexer.h"
#include "lang/Parser.h"
#include "lang/Ast.h"
#include "runtime/Databse.h"
#include "storage/Storage.h"

#define DB_FILE "database.gsql"

static int execute_statement(Database *db, ASTNode *node, int *should_persist) {
    if (db == NULL || node == NULL || should_persist == NULL) {
        return 0;
    }

    *should_persist = 0;

    switch (node->type) {
        case NODE_CREATE_TABLE_STMT:
            *should_persist = 1;
            return execute_create_table(db, node);
        case NODE_INSERT_STMT:
            *should_persist = 1;
            return execute_insert(db, node);
        case NODE_SELECT_STMT:
            return execute_select(db, node);
        case NODE_DROP_TABLE_STMT:
            *should_persist = 1;
            return execute_drop_table(db, node);
        default:
            fprintf(stderr, "Error: unsupported statement\n");
            return 0;
    }
}

static int run_statement(Database *db, const char *label, const char *sql, int expected_success) {
    struct Parser parser = parser_init(sql);
    ASTNode *node = parse_statement(&parser);
    int success = 0;
    int should_persist = 0;

    if (node != NULL) {
        success = execute_statement(db, node, &should_persist);
        if (success && should_persist) {
            if (!db_save(db, DB_FILE)) {
                fprintf(stderr, "Error: failed to save database to %s\n", DB_FILE);
                success = 0;
            }
        }
    }

    printf("[%s] %s\n", success == expected_success ? "PASS" : "FAIL", label);
    printf("  SQL: %s\n", sql);
    printf("  Expected: %s | Got: %s\n",
           expected_success ? "success" : "failure",
           success ? "success" : "failure");

    free_node(node);
    return success == expected_success;
}

static int print_reloaded_snapshot(void) {
    Database restored;

    db_init(&restored);
    if (!db_load(&restored, DB_FILE)) {
        fprintf(stderr, "Error: failed to reload database from %s\n", DB_FILE);
        return 0;
    }

    printf("\nReloaded snapshot from %s:\n", DB_FILE);
    db_print(&restored);
    db_free(&restored);
    return 1;
}

int main(void) {
    int passed = 0;
    int total = 0;
    Database db;

    init_keywords();
    db_init(&db);

    if (remove(DB_FILE) != 0) {
        FILE *existing = fopen(DB_FILE, "rb");
        if (existing != NULL) {
            fclose(existing);
            fprintf(stderr, "Error: failed to reset %s\n", DB_FILE);
            db_free(&db);
            free_keywords();
            return 1;
        }
    }

    if (!db_load(&db, DB_FILE)) {
        fprintf(stderr, "Error: failed to load database from %s\n", DB_FILE);
        db_free(&db);
        free_keywords();
        return 1;
    }

    total++;
    passed += run_statement(&db, "Create table",
                            "CREATE TABLE users (id INT, name TEXT);", 1);

    total++;
    passed += run_statement(&db, "Insert first row",
                            "INSERT INTO users VALUES (1, 'bob');", 1);

    total++;
    passed += run_statement(&db, "Insert second row",
                            "INSERT INTO users VALUES (2, 'alice');", 1);

    total++;
    passed += run_statement(&db, "Select after inserts",
                            "SELECT * FROM users;", 1);

    total++;
    passed += run_statement(&db, "Reject duplicate table after save",
                            "CREATE TABLE users (id INT, name TEXT);", 0);

    if (!print_reloaded_snapshot()) {
        db_free(&db);
        free_keywords();
        return 1;
    }

    total++;
    passed += run_statement(&db, "Drop persisted table",
                            "DROP TABLE users;", 1);

    total++;
    passed += run_statement(&db, "Reject select after drop",
                            "SELECT * FROM users;", 0);

    if (!print_reloaded_snapshot()) {
        db_free(&db);
        free_keywords();
        return 1;
    }

    printf("\nSummary: %d/%d tests passed\n", passed, total);

    db_free(&db);
    free_keywords();
    return passed == total ? 0 : 1;
}
