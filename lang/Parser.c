#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Parser.h"

//helpers
struct Parser parser_init(const char *source) {
    struct Parser p;

    // init lexer (maybe should have made a helper func for this but hindsight is 20/20)
    p.lexer.src = source;
    p.lexer.pos = 0;
    p.lexer.totalLength = strlen(source);
    p.lexer.currentLine = 1;
    p.lexer.currentColumn = 1;

    //first token
    p.current_token = nextToken(&p.lexer);
    return p;
}

void advance_token(struct Parser *p) {
    //dont forget free, important, please for the love of god
    freeToken(&p->current_token);
    p->current_token = nextToken(&p->lexer);
}

bool is_keyword(const struct Parser *p, const char *word) {
    return p->current_token.type == Keyword && strcmp(p->current_token.value, word) == 0;
}

bool match(struct Parser *p, const int expected_type) {
    if (p->current_token.type == expected_type) {
        advance_token(p);
        return true;
    }
    return false;
}

//find a way to stop the parsing later on
void expect(struct Parser *p, const int expected_type) {
    if (p->current_token.type == expected_type) {
        advance_token(p);
    } else {
        fprintf(stderr, "Syntax error at %d:%d: expected token type %d, got %d (%s)\n",
                p->current_token.line,
                p->current_token.column,
                expected_type,
                p->current_token.type,
                p->current_token.value ? p->current_token.value : "<null>");
    }
}

// actual functuons
