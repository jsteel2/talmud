#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "tokenizer.h"

void tokenizer_reset(Tokenizer *t)
{
    t->pos = 0;
    t->line = 0;
    t->col = 0;
}

TokenType tokenizer_error(char *fmt, ...)
{
    va_list v;
    va_start(v, fmt);
    vfprintf(stderr, fmt, v); // print t->col and t->line aswell
    va_end(v);
    fputc('\n', stderr);
    return TERROR;
}

TokenType tokenizer_number_token(Tokenizer *t, uint64_t *value)
{
    char *endptr;
    *value = strtoll(&t->src[t->pos], &endptr, 0);
    t->pos += endptr - &t->src[t->pos];
    t->col += endptr - &t->src[t->pos];
    return TNUM;
}

TokenType tokenizer_ident_token(Tokenizer *t, char **value)
{
    size_t len = 0;
    do
    {
        len++;
        t->pos++;
        t->col++;
    } while (isalnum(t->src[t->pos]) || t->src[t->pos] == '_');
    *value = strndup(&t->src[t->pos], len);
    return TIDENT;
}

TokenType tokenizer_string_token(Tokenizer *t, char **value, TokenType type)
{
    t->pos++;
    t->col++;
    char *end = strchr(&t->src[t->pos], '"' ? type == TSTRING : '\'');
    size_t len = end - &t->src[t->pos];

    *value = malloc(len);

    for (int i = 0; i < len; i++)
    {
        t->col++;
        if (t->src[t->pos++] == '\\')
        {
            t->col++;
            switch (t->src[t->pos++])
            {
                case '\r': (*value)[i] = '\r'; break;
                case '\n': (*value)[i] = '\n'; break;
                case '\t': (*value)[i] = '\t'; break;
                case '\b': (*value)[i] = '\b'; break;
                case '\'': (*value)[i] = '\''; break;
                case '"': (*value)[i] = '"'; break;
                case '\\': (*value)[i] = '\\'; break;
                default: return tokenizer_error("unknown escape %c", t->src[t->pos - 1]);
            }
        }
        else
        {
            (*value)[i] = t->src[t->pos];
        }
    }

    return type;
}

TokenType tokenizer_symbol_token(Tokenizer *t, void *value)
{
    t->col++;
    switch (t->src[t->pos++])
    {
        default: return tokenizer_error("unknown symbol %c", t->src[t->pos - 1]);
    }
}

TokenType tokenizer_token(Tokenizer *t, void *value)
{
    if (t->src[t->pos] == 0) return TEND;

    while (isspace(t->src[t->pos]) || t->src[t->pos] == '#')
    {
        if (t->src[t->pos] == '#')
        {
            while (t->src[++t->pos] != '\n')
            {
                if (t->src[t->pos] == 0) return TEND;
            }
        }
        if (t->src[t->pos] == '\n')
        {
            t->col = 0;
            t->line++;
        }
        if (t->src[++t->pos] == 0) return TEND;
        t->col++;
    }

    if (isdigit(t->src[t->pos])) return tokenizer_number_token(t, value);
    if (isalpha(t->src[t->pos]) || t->src[t->pos] == '_') return tokenizer_ident_token(t,  value);
    if (t->src[t->pos] == '\'') return tokenizer_string_token(t, value, TCHARS);
    if (t->src[t->pos] == '"') return tokenizer_string_token(t, value, TSTRING);

    return tokenizer_symbol_token(t, value);
}