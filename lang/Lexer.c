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