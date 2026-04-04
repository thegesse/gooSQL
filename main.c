#include <stdio.h>
#include "lang/lexer.h"
#include "lang/Parser.h"
#include "lang/Ast.h"

int main(void) {
    init_keywords();

    const char *sql = "CREATE TABLE users (id INT, name TEXT);";
    struct Parser parser = parser_init(sql);
    ASTNode *root = parse_statement(&parser);

    if (root != NULL) {
        printf("Parse success\n");
    } else {
        printf("Parse failed\n");
    }
    print_ast(root, 0);
    free_keywords();
    return 0;
}