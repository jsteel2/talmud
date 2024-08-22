#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "tokenizer.h"

#define SLEN(x) x, (sizeof(x) - 1)

void tokenizer_init(Tokenizer *t, char *src)
{
    t->pos = 0;
    t->line = 0;
    t->col = 0;
    t->src = src;
}

TokenType die(Tokenizer *t, char *fmt, ...)
{
    fprintf(stderr, "at col %lu line %lu: ", t->col, t->line);
    va_list v;
    va_start(v, fmt);
    vfprintf(stderr, fmt, v);
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
    char *start = &t->src[t->pos];
    size_t len = 0;
    do
    {
        len++;
        t->pos++;
        t->col++;
    } while (isalnum(t->src[t->pos]) || t->src[t->pos] == '_');

    switch (start[0])
    {
        case 'A':
            return strncmp(start, SLEN("ADD")) == 0 ? TADD :
                strncmp(start, SLEN("AX")) == 0 ? TAX :
                strncmp(start, SLEN("AH")) == 0 ? TAH :
                strncmp(start, SLEN("AL")) == 0 ? TAL : TIDENT;
        case 'B':
            return strncmp(start, SLEN("BX")) == 0 ? TBX :
                strncmp(start, SLEN("BL")) == 0 ? TBL : TIDENT;
        case 'C':
            return strncmp(start, SLEN("CALL")) == 0 ? TCALL :
                strncmp(start, SLEN("CLI")) == 0 ? TCLI : TIDENT;
        case 'D':
            return strncmp(start, SLEN("DS")) == 0 ? TDS : TIDENT;
        case 'E':
            return strncmp(start, SLEN("ES")) == 0 ? TES : TIDENT;
        case 'I':
            return strncmp(start, SLEN("INT")) == 0 ? TINT : TIDENT;
        case 'M':
            return strncmp(start, SLEN("MOV")) == 0 ? TMOV : TIDENT;
        case 'O':
            return strncmp(start, SLEN("ORG")) == 0 ? TORG : TIDENT;
        case 'S':
            return strncmp(start, SLEN("STI")) == 0 ? TSTI :
                strncmp(start, SLEN("SUB")) == 0 ? TSUB :
                strncmp(start, SLEN("SHL")) == 0 ? TSHL :
                strncmp(start, SLEN("SS")) == 0 ? TSS :
                strncmp(start, SLEN("SI")) == 0 ? TSI : TIDENT;
        case 'U':
            return strncmp(start, SLEN("USE16")) == 0 ? TUSE16 :
                strncmp(start, SLEN("USE32")) == 0 ? TUSE32 : TIDENT;
        case 'X':
            return strncmp(start, SLEN("XOR")) == 0 ? TXOR : TIDENT;
        default:
            *value = strndup(start, len);
            return TIDENT;
    }
}

TokenType tokenizer_string_token(Tokenizer *t, char **value, TokenType type)
{
    t->pos++;
    t->col++;
    char *end = strchr(&t->src[t->pos], type == TSTRING ? '"' : '\'');
    size_t len = end - &t->src[t->pos];

    *value = malloc(len);

    for (size_t i = 0; i < len; i++)
    {
        t->col++;
        if (t->src[t->pos++] == '\\')
        {
            t->col++;
            switch (t->src[t->pos++])
            {
                case 'r': (*value)[i] = '\r'; break;
                case 'n': (*value)[i] = '\n'; break;
                case 't': (*value)[i] = '\t'; break;
                case 'b': (*value)[i] = '\b'; break;
                case '\'': (*value)[i] = '\''; break;
                case '"': (*value)[i] = '"'; break;
                case '\\': (*value)[i] = '\\'; break;
                default: return die(t, "unknown escape %c", t->src[t->pos - 1]);
            }
        }
        else
        {
            (*value)[i] = t->src[t->pos];
        }
    }

    t->col++;
    t->pos++;

    return type;
}

TokenType tokenizer_symbol_token(Tokenizer *t, void *value)
{
    (void)value;
    t->col++;
    switch (t->src[t->pos++])
    {
        case '*':
            if (t->src[t->pos++] == '!') return TSTARU;
            else if (t->src[t->pos - 1] == '$') return TSTARS;
            return TSTAR;
        case '/':
            if (t->src[t->pos++] == '!') return TSLASHU;
            else if (t->src[t->pos - 1] == '$') return TSLASHS;
            return die(t, "invalid symbol %c", t->src[t->pos - 1]);
        case '(': return TLEFTPAREN;
        case ')': return TRIGHTPAREN;
        case '-': return TMINUS;
        case '+': return TPLUS;
        case ',': return TCOMMA;
        default: return die(t, "unknown symbol %c", t->src[t->pos - 1]);
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