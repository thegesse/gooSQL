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
    return type == '=' || type == Eq || type == Ne || type == Le ||
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

char *parse_type_name(struct Parser *p) {
    if (p->current_token.type == Id || p->current_token.type == Keyword) {
        char *type_name = strdup(p->current_token.value);
        advance_token(p);
        return type_name;
    } else {
        fprintf(stderr, "Syntax Error: expected type name\n");
        return NULL;
    }
}

ASTNode* parse_column_def(struct Parser *p) {
    if (p->current_token.type != Id) {
        fprintf(stderr, "Syntax Error: expected 'Id'\n");
        return NULL;
    }
    char* column_name = strdup(p->current_token.value);
    advance_token(p);

    char* type_name = parse_type_name(p);
    if (type_name == NULL) {
        free(column_name);
        return NULL;
    }

    ASTNode* node = create_node(NODE_COLUMN_DEF);
    if (node == NULL) {
        free(type_name);
        free(column_name);
        return NULL;
    }
    node->data.column_def.column_name = column_name;
    node->data.column_def.type_name = type_name;
    return node;
}

ASTNode* parse_column_def_list(struct Parser *p) {
    ASTNode* first_col = parse_column_def(p);
    if (first_col == NULL) {
        return NULL;
    }

    ASTNode* head = create_node(NODE_COLUMN_LIST);
    if (head == NULL) return NULL;

    head->data.list.current = first_col;
    head->data.list.next = NULL;
    ASTNode* current = head;

    while (p->current_token.type == ',') {
        advance_token(p);

        ASTNode* next_col = parse_column_def(p);
        if (next_col == NULL) {
            free(current->data.list.current);
            return NULL;
        }

        ASTNode* next_list_node = create_node(NODE_COLUMN_LIST);
        if (next_list_node == NULL) {
            free(current->data.list.current);
            return NULL;
        }

        next_list_node->data.list.current = next_col;
        next_list_node->data.list.next = NULL;
        current->data.list.next = next_list_node;
        current = next_list_node;
    }
    return head;
}

ASTNode* parse_create_table_statement(struct Parser *p) {
    if (!is_keyword(p,"CREATE")) {
        fprintf(stderr, "Syntax Error: expected 'CREATE'\n");
        return NULL;
    }
    advance_token(p);

     if (!is_keyword(p,"TABLE")) {
         fprintf(stderr, "Syntax Error: expected 'TABLE'\n");
         return NULL;
     }
    advance_token(p);

    if (p->current_token.type != Id) {
        fprintf(stderr, "Syntax Error: expected 'ID'\n");
        return NULL;
    }
    char* table_name = strdup(p->current_token.value);
    advance_token(p);

    if (p->current_token.type != '(') {
        fprintf(stderr, "Syntax Error: expected '('\n");
        free(table_name);
        return NULL;
    }
    advance_token(p);

    ASTNode* column_list = parse_column_def_list(p);
    if (column_list == NULL) {
        free(table_name);
        return NULL;
    }

    if (p->current_token.type != ')') {
        fprintf(stderr, "Syntax Error: expected ')'\n");
        free(table_name);
        free_node(column_list);
        return NULL;
    }
    advance_token(p);

    if (p->current_token.type == ';') {
        advance_token(p);
    }

    ASTNode* node = create_node(NODE_CREATE_TABLE_STMT);
    if (node == NULL) {
        free(table_name);
        return NULL;
    }
    node->data.create_table.table_name = table_name;
    node->data.create_table.columns = column_list;

    return node;
}

ASTNode* parse_value(struct Parser *p) {
    ASTNode *node = NULL;

    if (p->current_token.type == Num) {
        node = create_node(NODE_NUM);
        if (!node) return NULL;
        node->data.d_val = atof(p->current_token.value);
        advance_token(p);
        return node;
    }
    if (p->current_token.type == Str) {
        node = create_node(NODE_STR);
        if (!node) return NULL;
        node->data.s_val = strdup(p->current_token.value);
        advance_token(p);
        return node;
    }
    if (is_keyword(p, "NULL")) {
        node = create_node(NODE_NULL);
        if (!node) return NULL;
        advance_token(p);
        return node;
    }

    fprintf(stderr, "Syntax Error at %d:%d: expected value, got '%s'\n",
            p->current_token.line,
            p->current_token.column,
            p->current_token.value ? p->current_token.value : "<null>");
    return NULL;
}

ASTNode* parse_value_list(struct Parser *p) {
    ASTNode *first_val = parse_value(p);
    if (first_val == NULL) return NULL;

    ASTNode *head = create_node(NODE_COLUMN_LIST);
    if (head == NULL) return NULL;
    head->data.list.current = first_val;
    head->data.list.next = NULL;
    ASTNode* current = head;

    while (p->current_token.type == ',') {
        advance_token(p);
        ASTNode* next_val = parse_value(p);
        if (next_val == NULL) {
            free_node(head);
            return NULL;
        }
        ASTNode *next_node = create_node(NODE_COLUMN_LIST);
        if (!next_node) {
            free_node(head);
            free_node(next_val);
            return NULL;
        }
        next_node->data.list.current = next_val;
        next_node->data.list.next = NULL;
        current->data.list.next = next_node;
        current = next_node;
    }
    return head;
}

ASTNode* parse_insert_statement(struct Parser *p) {
    if (!is_keyword(p,"INSERT")) {
        fprintf(stderr, "Syntax Error: expected 'INSERT'\n");
        return NULL;
    }
    advance_token(p);

    if (!is_keyword(p,"INTO")) {
        fprintf(stderr, "Syntax Error: expected 'INTO'\n");
        return NULL;
    }
    advance_token(p);

    char* table_name = strdup(p->current_token.value);
    advance_token(p);
    if (!is_keyword(p,"VALUES")) {
        fprintf(stderr, "Syntax Error: expected 'VALUES'\n");
        return NULL;
    }
    advance_token(p);

    if (p->current_token.type != '(') {
        fprintf(stderr, "Syntax Error: expected '('\n");
        free(table_name);
        return NULL;
    }
    advance_token(p);

    ASTNode* value_list = parse_value_list(p);
    if (value_list == NULL) {
        free(table_name);
        return NULL;
    }

    if (p->current_token.type != ')') {
        fprintf(stderr, "Syntax Error: expected ')'\n");
        free_node(value_list);
        free(table_name);
        return NULL;
    }
    advance_token(p);

    if (p->current_token.type == ';') {
        advance_token(p);
    }

    ASTNode* node = create_node(NODE_INSERT_STMT);
    if (!node) {
        free(table_name);
        free_node(value_list);
        return NULL;
    }
    node->data.insert_stmt.table_name = table_name;
    node->data.insert_stmt.values = value_list;
    return node;
}

ASTNode* parse_statement(struct Parser *p) {
    if (is_keyword(p,"SELECT")) {
        return parse_select_statement(p);
    }
    if (is_keyword(p, "CREATE")) {
        return parse_create_table_statement(p);
    }
    if (is_keyword(p, "INSERT")) {
        return parse_insert_statement(p);
    }

    fprintf(stderr, "Error: Unsupported statement starting with '%s'\n",
            p->current_token.value ? p->current_token.value : "Unknown");
    return NULL;
}
