#include <stdio.h>
#include <stdlib.h>

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
    if(lexer->pos >= lexer->totalLength) {
        return '\0';
    }
    return lexer->src[lexer->pos++];
}


//actually advances the position of the cursor
char advance(struct Lexer *lexer) {
    if(lexer->pos >= lexer->totalLength) {
        return '\0';
    }
    char current = lexer->src[lexer->pos];
    lexer->pos++;
    //resets line and column(goes to the next one)
    if(current = '\n') {
        lexer->currentLine++;
        lexer->currentColumn = 0;
    } else {
        lexer->currentColumn++;
    }

    return current;
}

//reminder not equal to Eof, just end of user input. Eof would be when creating a token
int isAtEnd(struct Lexer *lexer) {
    return lexer->pos >= lexer->totalLength;
}

//next reminder to create the Token parser here or in another file, see how C structures work first