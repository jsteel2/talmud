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
    TEND, TERROR, TNUM, TIDENT, TCHARS, TSTRING
} TokenType;

void tokenizer_reset(Tokenizer *t);
TokenType tokenizer_token(Tokenizer *t, void *value);

#endif