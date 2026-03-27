#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

//note to self to seperate this into a C and header file when starting to add the AST and parser

//defines all tokens available, and starts at 128 to avoid ASCII errors, bc all values under 128 are for characters
enum {
    Num = 128, Id, Str, Keyword, Ne, Le, Ge, Unknown, Eof
};
struct Token {
    int type; //value of enum
    char *value; //token text
    int line;
    int column;
};

struct Lexer {
    const char *src;
    size_t pos;
    size_t totalLength;
    int currentLine;
    int currentColumn;
};

//add functions to let the code read through input such as peek, peekNext, advance, isAtEnd

char peek(struct Lexer *lexer) {
    if (lexer->pos >= lexer->totalLength) {
        return '\0';
    }
    //dont advance just return the position, advance will be in peek next
    return lexer->src[lexer->pos];
}

//for peek next last return would be return lexer->src[lexer->pos++];
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
        if(c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            advance(lexer);
        } else {
            break;
        }
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

    int type = Id;
    //add sql keywords: WHERE, SELECT ... too lazy to do it right now though

    struct Token tok = makeToken(lexer, type, text, len);
    tok.column = startCol;
    free(text);
    return tok;
}


struct Token parseString(struct Lexer *lexer) {
    char quote = advance(lexer); //consume quotes
    size_t start = lexer ->pos;
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

    advance(lexer);
    struct Token tok = makeToken(lexer, c, &c, 1);
    tok.column = startCol;
    return tok;
}
//make nextToken function == main function Ill call all the time