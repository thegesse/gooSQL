#ifndef LEXER_H
#define LEXER_H

#include<stddef.h>
#include<stdint.h>

//defines all tokens available, and starts at 128 to avoid ASCII errors, bc all values under 128 are for characters
enum {
    Num = 128, Id, Str, Keyword, Ne, Le, Ge, Eq, Unknown, Eof
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

struct Lexer lexer_new(const char *source);
struct Token nextToken(struct Lexer *lexer);
void freeToken(struct Token *tok);

void init_keywords(void);
void free_keywords(void);

const char* tokenTypeToString(int type);


#endif