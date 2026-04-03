#include <stdio.h>
#include "lang/lexer.h"
#include "lang/Parser.h"
#include "lang/Ast.h"

int main(void) {
    init_keywords();

    const char *sql = "SELECT name FROM users WHERE id = 10;";
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