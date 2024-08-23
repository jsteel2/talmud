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
            if (strncmp(start, SLEN("ADD")) == 0) return TADD;
            if (strncmp(start, SLEN("AX")) == 0) return TAX;
            if (strncmp(start, SLEN("AH")) == 0) return TAH;
            if (strncmp(start, SLEN("AL")) == 0) return TAL;
            goto d;
        case 'B':
            if (strncmp(start, SLEN("BYTE")) == 0) return TBYTE;
            if (strncmp(start, SLEN("BX")) == 0) return TBX;
            if (strncmp(start, SLEN("BL")) == 0) return TBL;
            goto d;
        case 'C':
            if (strncmp(start, SLEN("CALL")) == 0) return TCALL;
            if (strncmp(start, SLEN("CLI")) == 0) return TCLI;
            if (strncmp(start, SLEN("CH")) == 0) return TCH;
            if (strncmp(start, SLEN("CL")) == 0) return TCL;
            goto d;
        case 'D':
            if (strncmp(start, SLEN("DS")) == 0) return TDS;
            if (strncmp(start, SLEN("DH")) == 0) return TDH;
            if (strncmp(start, SLEN("DI")) == 0) return TDI;
            if (strncmp(start, SLEN("DB")) == 0) return TDB;
            goto d;
        case 'E':
            if (strncmp(start, SLEN("ES")) == 0) return TES;
            goto d;
        case 'I':
            if (strncmp(start, SLEN("INT")) == 0) return TINT;
            goto d;
        case 'J':
            if (strncmp(start, SLEN("JMP")) == 0) return TJMP;
            if (strncmp(start, SLEN("JZ")) == 0) return TJZ;
            if (strncmp(start, SLEN("JB")) == 0) return TJB;
            goto d;
        case 'L':
            if (strncmp(start, SLEN("LODSB")) == 0) return TLODSB;
            goto d;
        case 'M':
            if (strncmp(start, SLEN("MOV")) == 0) return TMOV;
            goto d;
        case 'O':
            if (strncmp(start, SLEN("ORG")) == 0) return TORG;
            goto d;
        case 'P':
            if (strncmp(start, SLEN("PUSH")) == 0) return TPUSH;
            if (strncmp(start, SLEN("POP")) == 0) return TPOP;
            goto d;
        case 'R':
            if (strncmp(start, SLEN("RET")) == 0) return TRET;
            if (strncmp(start, SLEN("RB")) == 0) return TRB;
            goto d;
        case 'S':
            if (strncmp(start, SLEN("STI")) == 0) return TSTI;
            if (strncmp(start, SLEN("SUB")) == 0) return TSUB;
            if (strncmp(start, SLEN("SHL")) == 0) return TSHL;
            if (strncmp(start, SLEN("SS")) == 0) return TSS;
            if (strncmp(start, SLEN("SP")) == 0) return TSP;
            if (strncmp(start, SLEN("SI")) == 0) return TSI;
            goto d;
        case 'T':
            if (strncmp(start, SLEN("TEST")) == 0) return TTEST;
            goto d;
        case 'U':
            if (strncmp(start, SLEN("USE16")) == 0) return TUSE16;
            if (strncmp(start, SLEN("USE32")) == 0) return TUSE32;
            goto d;
        case 'X':
            if (strncmp(start, SLEN("XOR")) == 0) return TXOR;
            goto d;
        default: goto d;
    }

d:
    *value = strndup(start, len);
    return TIDENT;
}

TokenType tokenizer_string_token(Tokenizer *t, char **value, TokenType type)
{
    t->pos++;
    t->col++;
    char *end = strchr(&t->src[t->pos], type == TSTRING ? '"' : '\'');
    size_t len = end - &t->src[t->pos];

    *value = malloc(len + 1);

    size_t i = 0;
    while (&t->src[t->pos] < end)
    {
        t->col++;
        if (t->src[t->pos++] == '\\')
        {
            t->col++;
            switch (t->src[t->pos++])
            {
                case 'r': (*value)[i++] = '\r'; break;
                case 'n': (*value)[i++] = '\n'; break;
                case 't': (*value)[i++] = '\t'; break;
                case 'b': (*value)[i++] = '\b'; break;
                case '\'': (*value)[i++] = '\''; break;
                case '"': (*value)[i++] = '"'; break;
                case '\\': (*value)[i++] = '\\'; break;
                default: return die(t, "unknown escape %c", t->src[t->pos - 1]);
            }
        }
        else
        {
            (*value)[i++] = t->src[t->pos - 1];
        }
    }
    (*value)[i] = 0;

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
        case '$':
            if (t->src[t->pos++] == '$') return TORIGIN;
            return TIP;
        case '(': return TLEFTPAREN;
        case ')': return TRIGHTPAREN;
        case '[': return TLEFTBRACKET;
        case ']': return TRIGHTBRACKET;
        case '-': return TMINUS;
        case '+': return TPLUS;
        case ',': return TCOMMA;
        case '.': return TDOT;
        case ':': return TCOLON;
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