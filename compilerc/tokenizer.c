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

TokenType _die(char *file, int line, Tokenizer *t, char *fmt, ...)
{
    fprintf(stderr, "COMP(file %s, line %d) at col %lu line %lu: ", file, line, t->col, t->line);
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
            if (LEN("ADC") == len && strncmp(start, SLEN("ADC")) == 0) return TADC;
            if (LEN("AND") == len && strncmp(start, SLEN("AND")) == 0) return TAND;
            if (LEN("AX") == len && strncmp(start, SLEN("AX")) == 0) return TAX;
            if (LEN("AH") == len && strncmp(start, SLEN("AH")) == 0) return TAH;
            if (LEN("AL") == len && strncmp(start, SLEN("AL")) == 0) return TAL;
            break;
        case 'B':
            if (LEN("BYTE") == len && strncmp(start, SLEN("BYTE")) == 0) return TBYTE;
            if (LEN("BX") == len && strncmp(start, SLEN("BX")) == 0) return TBX;
            if (LEN("BL") == len && strncmp(start, SLEN("BL")) == 0) return TBL;
            if (LEN("BH") == len && strncmp(start, SLEN("BH")) == 0) return TBH;
            break;
        case 'C':
            if (LEN("CMPSB") == len && strncmp(start, SLEN("CMPSB")) == 0) return TCMPSB;
            if (LEN("CALL") == len && strncmp(start, SLEN("CALL")) == 0) return TCALL;
            if (LEN("CWDE") == len && strncmp(start, SLEN("CWDE")) == 0) return TCWDE;
            if (LEN("CDQ") == len && strncmp(start, SLEN("CDQ")) == 0) return TCDQ;
            if (LEN("CLI") == len && strncmp(start, SLEN("CLI")) == 0) return TCLI;
            if (LEN("CLD") == len && strncmp(start, SLEN("CLD")) == 0) return TCLD;
            if (LEN("CMP") == len && strncmp(start, SLEN("CMP")) == 0) return TCMP;
            if (LEN("CR0") == len && strncmp(start, SLEN("CR0")) == 0) return TCR0;
            if (LEN("CR1") == len && strncmp(start, SLEN("CR1")) == 0) return TCR1;
            if (LEN("CR2") == len && strncmp(start, SLEN("CR2")) == 0) return TCR2;
            if (LEN("CR3") == len && strncmp(start, SLEN("CR3")) == 0) return TCR3;
            if (LEN("CR4") == len && strncmp(start, SLEN("CR4")) == 0) return TCR4;
            if (LEN("CR5") == len && strncmp(start, SLEN("CR5")) == 0) return TCR5;
            if (LEN("CR6") == len && strncmp(start, SLEN("CR6")) == 0) return TCR6;
            if (LEN("CR7") == len && strncmp(start, SLEN("CR7")) == 0) return TCR7;
            if (LEN("CX") == len && strncmp(start, SLEN("CX")) == 0) return TCX;
            if (LEN("CH") == len && strncmp(start, SLEN("CH")) == 0) return TCH;
            if (LEN("CL") == len && strncmp(start, SLEN("CL")) == 0) return TCL;
            if (LEN("CS") == len && strncmp(start, SLEN("CS")) == 0) return TCS;
            break;
        case 'D':
            if (LEN("DWORD") == len && strncmp(start, SLEN("DWORD")) == 0) return TDWORD;
            if (LEN("DEC") == len && strncmp(start, SLEN("DEC")) == 0) return TDEC;
            if (LEN("DIV") == len && strncmp(start, SLEN("DIV")) == 0) return TDIV;
            if (LEN("DS") == len && strncmp(start, SLEN("DS")) == 0) return TDS;
            if (LEN("DX") == len && strncmp(start, SLEN("DX")) == 0) return TDX;
            if (LEN("DH") == len && strncmp(start, SLEN("DH")) == 0) return TDH;
            if (LEN("DL") == len && strncmp(start, SLEN("DL")) == 0) return TDL;
            if (LEN("DI") == len && strncmp(start, SLEN("DI")) == 0) return TDI;
            if (LEN("DB") == len && strncmp(start, SLEN("DB")) == 0) return TDB;
            if (LEN("DW") == len && strncmp(start, SLEN("DW")) == 0) return TDW;
            if (LEN("DD") == len && strncmp(start, SLEN("DD")) == 0) return TDD;
            break;
        case 'E':
            if (LEN("EAX") == len && strncmp(start, SLEN("EAX")) == 0) return TEAX;
            if (LEN("ECX") == len && strncmp(start, SLEN("ECX")) == 0) return TECX;
            if (LEN("EDX") == len && strncmp(start, SLEN("EDX")) == 0) return TEDX;
            if (LEN("EBX") == len && strncmp(start, SLEN("EBX")) == 0) return TEBX;
            if (LEN("EBP") == len && strncmp(start, SLEN("EBP")) == 0) return TEBP;
            if (LEN("ESP") == len && strncmp(start, SLEN("ESP")) == 0) return TESP;
            if (LEN("ESI") == len && strncmp(start, SLEN("ESI")) == 0) return TESI;
            if (LEN("EDI") == len && strncmp(start, SLEN("EDI")) == 0) return TEDI;
            if (LEN("ES") == len && strncmp(start, SLEN("ES")) == 0) return TES;
            break;
        case 'F':
            if (LEN("FS") == len && strncmp(start, SLEN("FS")) == 0) return TFS;
            break;
        case 'G':
            if (LEN("GS") == len && strncmp(start, SLEN("GS")) == 0) return TGS;
            break;
        case 'H':
            if (LEN("HLT") == len && strncmp(start, SLEN("HLT")) == 0) return THLT;
            break;
        case 'I':
            if (LEN("IMUL") == len && strncmp(start, SLEN("IMUL")) == 0) return TIMUL;
            if (LEN("INC") == len && strncmp(start, SLEN("INC")) == 0) return TINC;
            if (LEN("INT") == len && strncmp(start, SLEN("INT")) == 0) return TINT;
            if (LEN("IN") == len && strncmp(start, SLEN("IN")) == 0) return TIN;
            break;
        case 'J':
            if (LEN("JMP") == len && strncmp(start, SLEN("JMP")) == 0) return TJMP;
            if (LEN("JNZ") == len && strncmp(start, SLEN("JNZ")) == 0) return TJNZ;
            if (LEN("JNB") == len && strncmp(start, SLEN("JNB")) == 0) return TJNB;
            if (LEN("JZ") == len && strncmp(start, SLEN("JZ")) == 0) return TJZ;
            if (LEN("JB") == len && strncmp(start, SLEN("JB")) == 0) return TJB;
            break;
        case 'L':
            if (LEN("LODSB") == len && strncmp(start, SLEN("LODSB")) == 0) return TLODSB;
            if (LEN("LGDT") == len && strncmp(start, SLEN("LGDT")) == 0) return TLGDT;
            if (LEN("LOOP") == len && strncmp(start, SLEN("LOOP")) == 0) return TLOOP;
            if (LEN("LEA") == len && strncmp(start, SLEN("LEA")) == 0) return TLEA;
            break;
        case 'M':
            if (LEN("MOVZX") == len && strncmp(start, SLEN("MOVZX")) == 0) return TMOVZX;
            if (LEN("MOVSB") == len && strncmp(start, SLEN("MOVSB")) == 0) return TMOVSB;
            if (LEN("MOVSW") == len && strncmp(start, SLEN("MOVSW")) == 0) return TMOVSW;
            if (LEN("MOVSD") == len && strncmp(start, SLEN("MOVSD")) == 0) return TMOVSD;
            if (LEN("MOV") == len && strncmp(start, SLEN("MOV")) == 0) return TMOV;
            if (LEN("MUL") == len && strncmp(start, SLEN("MUL")) == 0) return TMUL;
            break;
        case 'O':
            if (LEN("ORG") == len && strncmp(start, SLEN("ORG")) == 0) return TORG;
            if (LEN("OUT") == len && strncmp(start, SLEN("OUT")) == 0) return TOUT;
            if (LEN("OR") == len && strncmp(start, SLEN("OR")) == 0) return TOR;
            break;
        case 'P':
            if (LEN("PUSHAD") == len && strncmp(start, SLEN("PUSHAD")) == 0) return TPUSHAD;
            if (LEN("PUSHF") == len && strncmp(start, SLEN("PUSHF")) == 0) return TPUSHF;
            if (LEN("POPAD") == len && strncmp(start, SLEN("POPAD")) == 0) return TPOPAD;
            if (LEN("PUSH") == len && strncmp(start, SLEN("PUSH")) == 0) return TPUSH;
            if (LEN("POPF") == len && strncmp(start, SLEN("POPF")) == 0) return TPOPF;
            if (LEN("POP") == len && strncmp(start, SLEN("POP")) == 0) return TPOP;
            break;
        case 'R':
            if (LEN("RETF") == len && strncmp(start, SLEN("RETF")) == 0) return TRETF;
            if (LEN("RET") == len && strncmp(start, SLEN("RET")) == 0) return TRET;
            if (LEN("REP") == len && strncmp(start, SLEN("REP")) == 0) return TREP;
            if (LEN("RB") == len && strncmp(start, SLEN("RB")) == 0) return TRB;
            break;
        case 'S':
            if (LEN("STOSB") == len && strncmp(start, SLEN("STOSB")) == 0) return TSTOSB;
            if (LEN("STOSW") == len && strncmp(start, SLEN("STOSW")) == 0) return TSTOSW;
            if (LEN("STOSD") == len && strncmp(start, SLEN("STOSD")) == 0) return TSTOSD;
            if (LEN("SETB") == len && strncmp(start, SLEN("SETB")) == 0) return TSETB;
            if (LEN("SETZ") == len && strncmp(start, SLEN("SETZ")) == 0) return TSETZ;
            if (LEN("STI") == len && strncmp(start, SLEN("STI")) == 0) return TSTI;
            if (LEN("STD") == len && strncmp(start, SLEN("STD")) == 0) return TSTD;
            if (LEN("SUB") == len && strncmp(start, SLEN("SUB")) == 0) return TSUB;
            if (LEN("SHL") == len && strncmp(start, SLEN("SHL")) == 0) return TSHL;
            if (LEN("SHR") == len && strncmp(start, SLEN("SHR")) == 0) return TSHR;
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
        case 'W':
            if (LEN("WORD") == len && strncmp(start, SLEN("WORD")) == 0) return TWORD;
            break;
        case 'X':
            if (LEN("XCHG") == len && strncmp(start, SLEN("XCHG")) == 0) return TXCHG;
            if (LEN("XOR") == len && strncmp(start, SLEN("XOR")) == 0) return TXOR;
            break;
        case 'a':
            if (LEN("and") == len && strncmp(start, SLEN("and")) == 0) return TLOGICALAND;
            break;
        case 'b':
            if (LEN("break") == len && strncmp(start, SLEN("break")) == 0) return TBREAK;
            break;
        case 'c':
            if (LEN("continue") == len && strncmp(start, SLEN("continue")) == 0) return TCONTINUE;
            if (LEN("const") == len && strncmp(start, SLEN("const")) == 0) return TCONST;
            if (LEN("case") == len && strncmp(start, SLEN("case")) == 0) return TCASE;
            break;
        case 'd':
            if (LEN("default") == len && strncmp(start, SLEN("default")) == 0) return TDEFAULT;
            if (LEN("do") == len && strncmp(start, SLEN("do")) == 0) return TDO;
            break;
        case 'e':
            if (LEN("else") == len && strncmp(start, SLEN("else")) == 0) return TELSE;
            if (LEN("enum") == len && strncmp(start, SLEN("enum")) == 0) return TENUM;
            break;
        case 'f':
            if (LEN("function") == len && strncmp(start, SLEN("function")) == 0) return TFUNCTION;
            break;
        case 'g':
            if (LEN("global") == len && strncmp(start, SLEN("global")) == 0) return TGLOBAL;
            break;
        case 'i':
            if (LEN("include") == len && strncmp(start, SLEN("include")) == 0) return TINCLUDE;
            if (LEN("if") == len && strncmp(start, SLEN("if")) == 0) return TIF;
            break;
        case 'o':
            if (LEN("or") == len && strncmp(start, SLEN("or")) == 0) return TLOGICALOR;
            break;
        case 'r':
            if (LEN("return") == len && strncmp(start, SLEN("return")) == 0) return TRETURN;
            break;
        case 's':
            if (LEN("struct") == len && strncmp(start, SLEN("struct")) == 0) return TSTRUCT;
            if (LEN("switch") == len && strncmp(start, SLEN("switch")) == 0) return TSWITCH;
            break;
        case 'w':
            if (LEN("while") == len && strncmp(start, SLEN("while")) == 0) return TWHILE;
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

    size_t size = 32;
    *value = malloc(size);

    size_t i = 0;
    while (t->src[t->pos] && t->src[t->pos] != (type == TCHARS ? '\'' : '"'))
    {
        if (i >= size) *value = realloc(*value, size = size * 2);
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
    if (i >= size) *value = realloc(*value, size + 1);
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
            if (t->src[t->pos++] == '!')
            {
                if (t->src[t->pos++] == '=') return TSTARUEQUALS;
                t->pos--;
                return TSTARU;
            }
            if (t->src[t->pos - 1] == '$') return TSTARS;
            t->pos--;
            return TSTAR;
        case '/':
            if (t->src[t->pos++] == '!')
            {
                if (t->src[t->pos++] == '=') return TSLASHUEQUALS;
                t->pos--;
                return TSLASHU;
            }
            if (t->src[t->pos - 1] == '$') return TSLASHS;
            return die(t, "invalid symbol %c", t->src[t->pos - 1]);
        case '%':
            if (t->src[t->pos++] == '!') return TMODULOU;
            if (t->src[t->pos - 1] == '$') return TMODULOS;
            return die(t, "invalid symbol %c", t->src[t->pos - 1]);
        case '<':
            if (t->src[t->pos++] == '!') return TLESSTHANU;
            if (t->src[t->pos - 1] == '$') return TLESSTHANS;
            if (t->src[t->pos - 1] == '<')
            {
                if (t->src[t->pos++] == '=') return TSHIFTLEFTEQUALS;
                t->pos--;
                return TSHIFTLEFT;
            }
            if (t->src[t->pos - 1] == '=')
            {
                if (t->src[t->pos++] == '!') return TLESSEQUALSU;
                if (t->src[t->pos - 1] == '$') return TLESSEQUALSS;
                return die(t, "invalid symbol %c", t->src[t->pos - 1]);
            }
            t->pos--;
            return TLEFTCHEVRON;
        case '>':
            if (t->src[t->pos++] == '!') return TGREATERTHANU;
            if (t->src[t->pos - 1] == '$') return TGREATERTHANS;
            if (t->src[t->pos - 1] == '>')
            {
                if (t->src[t->pos++] == '=') return TSHIFTRIGHTEQUALS;
                t->pos--;
                return TSHIFTRIGHT;
            }
            if (t->src[t->pos - 1] == '=')
            {
                if (t->src[t->pos++] == '!') return TGREATEREQUALSU;
                if (t->src[t->pos - 1] == '$') return TGREATEREQUALSS;
                return die(t, "invalid symbol %c", t->src[t->pos - 1]);
            }
            t->pos--;
            return TRIGHTCHEVRON;
        case '$':
            if (t->src[t->pos++] == '$') return TORIGIN;
            if (t->src[t->pos - 1] == '!') return TPROGEND;
            if (t->src[t->pos - 1] == '*') return TLINE;
            if (t->src[t->pos - 1] == '@') return TFUNC;
            t->pos--;
            return TIP;
        case '+':
            if (t->src[t->pos++] == '=') return TPLUSEQUALS;
            t->pos--;
            return TPLUS;
        case '-':
            if (t->src[t->pos++] == '=') return TMINUSEQUALS;
            t->pos--;
            return TMINUS;
        case '^':
            if (t->src[t->pos++] == '=') return TBITWISEXOREQUALS;
            t->pos--;
            return TBITWISEXOR;
        case '!':
            if (t->src[t->pos++] == '=') return TNOTEQUALS;
            t->pos--;
            return TBANG;
        case '|':
            if (t->src[t->pos++] == '=') return TBITWISEOREQUALS;
            t->pos--;
            return TBITWISEOR;
        case '&':
            if (t->src[t->pos++] == '=') return TBITWISEANDEQUALS;
            t->pos--;
            return TBITWISEAND;
        case '=':
            if (t->src[t->pos++] == '=') return TEQUALSEQUALS;
            t->pos--;
            return TEQUALS;
        case '~': return TBITWISENOT;
        case '(': return TLEFTPAREN;
        case ')': return TRIGHTPAREN;
        case '[': return TLEFTBRACKET;
        case ']': return TRIGHTBRACKET;
        case '{': return TLEFTBRACE;
        case '}': return TRIGHTBRACE;
        case ',': return TCOMMA;
        case '.': return TDOT;
        case ':': return TCOLON;
        case ';': return TSEMICOLON;
        case '?': return TQMARK;
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