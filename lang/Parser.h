#ifndef PARSER_H
#define PARSER_H
#include <stdbool.h>
#include "lexer.h"
#include "Ast.h"

struct Parser {
    struct Lexer lexer;
    struct Token current_token;
};

struct Parser parser_init(const char *source);
void advance_token(struct Parser *p);
bool is_keyword(const struct Parser *p, const char *word);
bool match(struct Parser *p, int expected_type);
void expect(struct Parser *p, int expected_type);
ASTNode* parse_statement(struct Parser *p);

#endif
