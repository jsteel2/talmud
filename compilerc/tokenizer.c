#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "tokenizer.h"

#define LEN(x) (sizeof(x) - 1)
#define SLEN(x) x, (LEN(x))

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
            if (LEN("ADD") == len && strncmp(start, SLEN("ADD")) == 0) return TADD;
            if (LEN("AND") == len && strncmp(start, SLEN("AND")) == 0) return TAND;
            if (LEN("AX") == len && strncmp(start, SLEN("AX")) == 0) return TAX;
            if (LEN("AH") == len && strncmp(start, SLEN("AH")) == 0) return TAH;
            if (LEN("AL") == len && strncmp(start, SLEN("AL")) == 0) return TAL;
            break;
        case 'B':
            if (LEN("BYTE") == len && strncmp(start, SLEN("BYTE")) == 0) return TBYTE;
            if (LEN("BX") == len && strncmp(start, SLEN("BX")) == 0) return TBX;
            if (LEN("BL") == len && strncmp(start, SLEN("BL")) == 0) return TBL;
            break;
        case 'C':
            if (LEN("CALL") == len && strncmp(start, SLEN("CALL")) == 0) return TCALL;
            if (LEN("CLI") == len && strncmp(start, SLEN("CLI")) == 0) return TCLI;
            if (LEN("CLD") == len && strncmp(start, SLEN("CLD")) == 0) return TCLD;
            if (LEN("CX") == len && strncmp(start, SLEN("CX")) == 0) return TCX;
            if (LEN("CH") == len && strncmp(start, SLEN("CH")) == 0) return TCH;
            if (LEN("CL") == len && strncmp(start, SLEN("CL")) == 0) return TCL;
            if (LEN("CS") == len && strncmp(start, SLEN("CS")) == 0) return TCS;
            break;
        case 'D':
            if (LEN("DS") == len && strncmp(start, SLEN("DS")) == 0) return TDS;
            if (LEN("DH") == len && strncmp(start, SLEN("DH")) == 0) return TDH;
            if (LEN("DL") == len && strncmp(start, SLEN("DL")) == 0) return TDL;
            if (LEN("DI") == len && strncmp(start, SLEN("DI")) == 0) return TDI;
            if (LEN("DB") == len && strncmp(start, SLEN("DB")) == 0) return TDB;
            break;
        case 'E':
            if (LEN("EBP") == len && strncmp(start, SLEN("EBP")) == 0) return TEBP;
            if (LEN("ES") == len && strncmp(start, SLEN("ES")) == 0) return TES;
            break;
        case 'I':
            if (LEN("INT") == len && strncmp(start, SLEN("INT")) == 0) return TINT;
            break;
        case 'J':
            if (LEN("JMP") == len && strncmp(start, SLEN("JMP")) == 0) return TJMP;
            if (LEN("JZ") == len && strncmp(start, SLEN("JZ")) == 0) return TJZ;
            if (LEN("JB") == len && strncmp(start, SLEN("JB")) == 0) return TJB;
            break;
        case 'L':
            if (LEN("LODSB") == len && strncmp(start, SLEN("LODSB")) == 0) return TLODSB;
            break;
        case 'M':
            if (LEN("MOVSW") == len && strncmp(start, SLEN("MOVSW")) == 0) return TMOVSW;
            if (LEN("MOV") == len && strncmp(start, SLEN("MOV")) == 0) return TMOV;
            break;
        case 'O':
            if (LEN("ORG") == len && strncmp(start, SLEN("ORG")) == 0) return TORG;
            break;
        case 'P':
            if (LEN("PUSHF") == len && strncmp(start, SLEN("PUSHF")) == 0) return TPUSHF;
            if (LEN("PUSH") == len && strncmp(start, SLEN("PUSH")) == 0) return TPUSH;
            if (LEN("POP") == len && strncmp(start, SLEN("POP")) == 0) return TPOP;
            break;
        case 'R':
            if (LEN("RETF") == len && strncmp(start, SLEN("RETF")) == 0) return TRETF;
            if (LEN("RET") == len && strncmp(start, SLEN("RET")) == 0) return TRET;
            if (LEN("REP") == len && strncmp(start, SLEN("REP")) == 0) return TREP;
            if (LEN("RB") == len && strncmp(start, SLEN("RB")) == 0) return TRB;
            break;
        case 'S':
            if (LEN("STI") == len && strncmp(start, SLEN("STI")) == 0) return TSTI;
            if (LEN("SUB") == len && strncmp(start, SLEN("SUB")) == 0) return TSUB;
            if (LEN("SHL") == len && strncmp(start, SLEN("SHL")) == 0) return TSHL;
            if (LEN("SS") == len && strncmp(start, SLEN("SS")) == 0) return TSS;
            if (LEN("SP") == len && strncmp(start, SLEN("SP")) == 0) return TSP;
            if (LEN("SI") == len && strncmp(start, SLEN("SI")) == 0) return TSI;
            break;
        case 'T':
            if (LEN("TEST") == len && strncmp(start, SLEN("TEST")) == 0) return TTEST;
            break;
        case 'U':
            if (LEN("USE16") == len && strncmp(start, SLEN("USE16")) == 0) return TUSE16;
            if (LEN("USE32") == len && strncmp(start, SLEN("USE32")) == 0) return TUSE32;
            break;
        case 'X':
            if (LEN("XOR") == len && strncmp(start, SLEN("XOR")) == 0) return TXOR;
            break;
        default: break;
    }

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