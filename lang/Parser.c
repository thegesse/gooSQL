#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Ast.h"
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
    return p->current_token.type == Keyword &&
           strcasecmp(p->current_token.value, word) == 0;
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
//helper for parse_expressions
bool is_binop(int type) {
    return type == Eq || type == Ne || type == Le ||
           type == Ge || type == '<' || type == '>';
}
// actual functuons

ASTNode* parse_primary(struct Parser *p) {
    if (p->current_token.type == Id) {
        ASTNode* node = malloc(sizeof(ASTNode));
        node->type = NODE_ID;
        node->data.s_val = strdup(p->current_token.value);
        advance_token(p);
        return node;
    }

    if (p->current_token.type == Num) {
        ASTNode* node = malloc(sizeof(ASTNode));
        node->type = NODE_NUM;
        node->data.d_val = atof(p->current_token.value);
        advance_token(p);
        return node;
    }

    if (p->current_token.type == Str) {
        ASTNode* node = malloc(sizeof(ASTNode));
        node->type = NODE_STR;
        node->data.s_val = strdup(p->current_token.value);
        advance_token(p);
        return node;
    }

    if (is_keyword(p,"NULL")) {
        ASTNode* node = malloc(sizeof(ASTNode));
        node->type = NODE_NULL;
        advance_token(p);
        return node;
    }

    //for error
    fprintf(stderr, "Syntax Error at %d:%d: Unexpected token '%s'\n",
            p->current_token.line,
            p->current_token.column,
            p->current_token.value ? p->current_token.value : "<null>");
    return NULL;
}

ASTNode* parse_expression(struct Parser *p) {
    ASTNode* left = parse_primary(p);
    if (left == NULL) {
        return NULL;
    }

    if (is_binop(p->current_token.type)) {
        int op_type = p->current_token.type;
        advance_token(p);
        ASTNode* right = parse_primary(p);
        if (right == NULL) {
            return NULL;
        }

        ASTNode* node = malloc(sizeof(ASTNode));
        node->type = NODE_BINARY;
        node->op = op_type;
        node->data.binary.left = left;
        node->data.binary.right = right;
        return node;
    }

    return left;
}

ASTNode* parse_select_list(struct Parser *p) {
    ASTNode* head = NULL;
    ASTNode* tail = NULL;

    while (true) {
        ASTNode* item = NULL;

        if (p->current_token.type == Id) {
            item = malloc(sizeof(ASTNode));
            item->type = NODE_ID;
            item->data.s_val = strdup(p->current_token.value);
            advance_token(p);
        } else if (p->current_token.type == '*') {
            item = malloc(sizeof(ASTNode));
            item->type = NODE_WILDCARD;
            advance_token(p);
        } else {
            //no more columns
            break;
        }

        ASTNode* list_node = malloc(sizeof(ASTNode));
        list_node->type = NODE_COLUMN_LIST;
        list_node->data.list.current = item;
        list_node->data.list.next = NULL;

        if (head == NULL) {
            head = list_node;
            tail = list_node;
        } else {
            tail->data.list.next = list_node;
            tail = list_node;
        }
        //consume the , and parse it
        if (p->current_token.type == ',') {
            advance_token(p);
        } else {
            // no , so finished
            break;
        }
    }

    return head;
}

ASTNode* parse_select_statement(struct Parser *p) {
    if (!is_keyword(p,"SELECT")) {
        fprintf(stderr, "Syntax Error: expected 'SELECT'\n");
        return NULL;
    }
    advance_token(p);
    ASTNode* columns = parse_select_list(p);

    if (columns == NULL) {
        fprintf(stderr, "Syntax Error: expected select list\n");
        return NULL;
    }

    if (!is_keyword(p,"FROM")) {
        fprintf(stderr, "Syntax Error: expected 'FROM'\n");
        return NULL;
    }
    advance_token(p);

    char* table = NULL;
    if (p->current_token.type == Id) {
        table = strdup(p->current_token.value);
        advance_token(p);
    } else {
        fprintf(stderr, "Error: expected table name\n");
        return NULL;
    }

    ASTNode* where = NULL;
    if (is_keyword(p,"WHERE")) {
        advance_token(p);
        where = parse_expression(p);
        if (where == NULL) {
            return NULL;
        }
    }

    if (p->current_token.type == ';') {
        advance_token(p);
    }

    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = NODE_SELECT_STMT;
    node->data.select_stmt.select_list = columns;
    node->data.select_stmt.table_name = table;
    node->data.select_stmt.where_clause = where;

    return node;
}

ASTNode* parse_statement(struct Parser *p) {
    if (is_keyword(p,"SELECT")) {
        return parse_select_statement(p);
    }

    fprintf(stderr, "Error: Unsupported statement starting with '%s'\n",
            p->current_token.value ? p->current_token.value : "Unknown");
    return NULL;
}
