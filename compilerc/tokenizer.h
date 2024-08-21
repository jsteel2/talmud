#ifndef __TOKENIZER_H
#define __TOKENIZER_H

#include <stdio.h>
#include <stdbool.h>

typedef struct
{
    char *src;
    size_t pos;
    size_t line;
    size_t col;
} Tokenizer;

typedef enum
{
    TERROR, TEND, TINVALID, TNUM, TIDENT, TCHARS, TSTRING,

    TORG,

    TLOGICALOR, TLOGICALAND,

    TQMARK, TCOLON, TEQUALS, TNOTEQUALS, TGREATERTHANU, TGREATERTHANS, TLESSTHANU, TLESSTHANS, TGREATEREQUALSU, TGREATEREQUALSS, TLESSEQUALSU, TLESSEQUALSS, TSHIFTRIGHT, TSHIFTLEFT, TBITWISEXOR, TBITWISEOR, TBITWISEAND, TPLUS, TMINUS, TMODULOU, TMODULOS, TSLASHU, TSLASHS, TSTARU, TSTARS, TSTAR, TLEFTPAREN, TRIGHTPAREN
} TokenType;

typedef struct
{
    TokenType type;
    void *value;
} Token;

void tokenizer_init(Tokenizer *t, char *src);
TokenType tokenizer_token(Tokenizer *t, void *value);
TokenType die(Tokenizer *t, char *fmt, ...);

#endif