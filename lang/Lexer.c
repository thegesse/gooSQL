#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include "lexer.h"

typedef struct {
    const char *word;
    int type;
} KeywordEntry;

// Small hash table
#define KEYWORD_HASH_SIZE 64

static KeywordEntry *keyword_table[KEYWORD_HASH_SIZE] = {NULL};


// none case sensitive
static uint32_t hash_keyword_ci(const char *str) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + tolower(c);
    }
    return hash % KEYWORD_HASH_SIZE;
}

//keywords
void init_keywords(void) {
    static const char *keywords[] = {
        "SELECT", "FROM", "WHERE", "INSERT", "INTO", "VALUES",
        "UPDATE", "SET", "DELETE", "CREATE", "TABLE", "DROP",
        "ALTER", "AND", "OR", "NOT", "NULL", "TRUE", "FALSE",
        "AS", "JOIN", "ON", "ORDER", "BY", "GROUP", "HAVING",
        "LIMIT", "OFFSET", "PRIMARY", "KEY", "FOREIGN", "REFERENCES",
        "INDEX", "UNIQUE", "DISTINCT", "ASC", "DESC", "BETWEEN",
        "LIKE", "IN", "EXISTS", "CASE", "WHEN", "THEN", "ELSE", "END",
        "COUNT", "SUM", "AVG", "MIN", "MAX", "LEFT", "RIGHT", "INNER", "OUTER",
        NULL
    };
    
    for (int i = 0; keywords[i]; i++) {
        uint32_t h = hash_keyword_ci(keywords[i]);
        while (keyword_table[h] != NULL) {
            h = (h + 1) % KEYWORD_HASH_SIZE;
        }
        keyword_table[h] = malloc(sizeof(KeywordEntry));
        keyword_table[h]->word = keywords[i];
        keyword_table[h]->type = Keyword;
    }
}

int lookup_keyword(const char *str) {
    uint32_t h = hash_keyword_ci(str);
    
    for (int i = 0; i < KEYWORD_HASH_SIZE; i++) {
        int idx = (h + i) % KEYWORD_HASH_SIZE;
        if (keyword_table[idx] == NULL) break; // Empty slot, not found
        
        if (strcasecmp(str, keyword_table[idx]->word) == 0) {
            return keyword_table[idx]->type; // Found
        }
    }
    return Id;
}

void free_keywords(void) {
    for (int i = 0; i < KEYWORD_HASH_SIZE; i++) {
        free(keyword_table[i]);
        keyword_table[i] = NULL;
    }
}


char peek(struct Lexer *lexer) {
    if (lexer->pos >= lexer->totalLength) {
        return '\0';
    }
    //dont advance just return the position, advance will be in peek next
    return lexer->src[lexer->pos];
}

char peekNext(struct Lexer *lexer) {
    if(lexer->pos +1 >= lexer->totalLength) {
        return '\0';
    }
    return lexer->src[lexer->pos + 1];
}


//actually advances the position of the cursor
char advance(struct Lexer *lexer) {
    if(lexer->pos >= lexer->totalLength) {
        return '\0';
    }
    char current = lexer->src[lexer->pos];
    lexer->pos++;
    //resets line and column(goes to the next one)
    if(current == '\n') {
        lexer->currentLine++;
        lexer->currentColumn = 1;
    } else {
        lexer->currentColumn++;
    }

    return current;
}

//reminder not equal to Eof, just end of user input. Eof would be when creating a token
int isAtEnd(struct Lexer *lexer) {
    return lexer->pos >= lexer->totalLength;
}


// helper to make tokens
struct Token makeToken(struct Lexer *lexer, int type, const char *text, size_t len) {
    struct Token tok;
    tok.type = type;
    tok.line = lexer->currentLine;
    tok.column = lexer->currentColumn - (int)len;
    
    tok.value = malloc(len + 1);
    strncpy(tok.value, text, len);
    tok.value[len] = '\0';
    
    return tok;
}

void skipWhiteSpace(struct Lexer *lexer) {
    while(!isAtEnd(lexer)) {
        char c = peek(lexer);
        
        // Handle spaces, tabs, newlines first
        if(c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            advance(lexer);
            continue;
        }
        // Handle SQL comments --
        if(c == '-' && peekNext(lexer) == '-') {
            while(!isAtEnd(lexer) && peek(lexer) != '\n') {
                advance(lexer);
            }
            continue;
        }
        break;
    }
}

struct Token parseNumbers(struct Lexer *lexer) {
    size_t start = lexer->pos;
    int startCol = lexer->currentColumn;

    while(isdigit(peek(lexer))) advance(lexer);

    if(peek(lexer) == '.' && isdigit(peekNext(lexer))) {
        advance(lexer);
        while(isdigit(peek(lexer))) advance(lexer);
    }

    size_t len = lexer->pos - start;
    struct Token tok = makeToken(lexer, Num, lexer->src + start, len);
    tok.column = startCol;
    return tok;
}

struct Token parseIdentifier(struct Lexer *lexer) {
    size_t start = lexer->pos;
    int startCol = lexer->currentColumn;

    while(isalnum(peek(lexer)) || peek(lexer) == '_') {
        advance(lexer);
    }

    size_t len = lexer->pos - start;
    char *text = strndup(lexer->src + start, len);

    int type = lookup_keyword(text);

    struct Token tok = makeToken(lexer, type, text, len);
    tok.column = startCol;
    free(text);
    return tok;
}

struct Token parseString(struct Lexer *lexer) {
    char quote = advance(lexer); //consume quotes
    size_t start = lexer->pos;
    int startCol = lexer->currentColumn;

    while(!isAtEnd(lexer) && peek(lexer) != quote){
        if(peek(lexer) == '\\') advance(lexer); //escape
        advance(lexer);
    }
    //handles error for undetermined string
    if(isAtEnd(lexer)) {
        return makeToken(lexer, Unknown, "undetermined string", 19);
    }

    size_t len = lexer->pos - start;
    struct Token tok = makeToken(lexer, Str, lexer->src + start, len);
    tok.column = startCol;

    advance(lexer);
    return tok;
}

struct Token nextToken(struct Lexer *lexer) {
    skipWhiteSpace(lexer);

    if(isAtEnd(lexer)) {
        struct Token tok = makeToken(lexer, Eof, "", 0);
        return tok;
    }

    char c = peek(lexer);
    int startCol = lexer->currentColumn;

    //handling numbers
    if (isdigit(c)) {
        return parseNumbers(lexer);
    }

    //handle Identifiers
    if(isalpha(c) || c == '_') {
        return parseIdentifier(lexer);
    }

    //handle Strings
    if(c == '\'' || c == '"') {
        return parseString(lexer);
    }

    // operations
    char next = peekNext(lexer);
    if(c == '!' && next == '=') {
        //advance twice to consume both characters
        advance(lexer);
        advance(lexer);
        struct Token tok = makeToken(lexer, Ne, "!=", 2);
        tok.column = startCol;
        return tok;
    }

    if(c == '<' && next == '=') {
        advance(lexer);
        advance(lexer);
        struct Token tok = makeToken(lexer, Le, "<=", 2);
        tok.column = startCol;
        return tok;
    }

    if(c == '>' && next == '=') {
        advance(lexer);
        advance(lexer);
        struct Token tok = makeToken(lexer, Ge, ">=", 2);
        return tok;
    }

    if(c == '=' && next == '=') {
        advance(lexer);
        advance(lexer);
        struct Token tok = makeToken(lexer, Eq, "==", 2);
        return tok;
    }

    advance(lexer);
    struct Token tok = makeToken(lexer, c, &c, 1);
    tok.column = startCol;
    return tok;
}

void freeToken(struct Token *tok) {
    if (tok == NULL) {
        return;
    }

    free(tok->value);
    tok->value = NULL;
}