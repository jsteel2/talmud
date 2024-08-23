#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "compiler.h"

#define HANDLE(x) if (!(x)) return false
#define MODRM(mod, reg, rm) (((mod) << 6) | ((reg) << 3) | (rm))

bool compiler_init(Compiler *c)
{
    c->outbin = NULL;
    return map_init(&c->idents);
}

void compiler_free(Compiler *c)
{
    map_free(&c->idents);
    free(c->outbin);

    if (c->cur.type == TIDENT || c->cur.type == TSTRING || c->cur.type == TCHARS) free(c->cur.value);
    if (c->next.type == TIDENT || c->next.type == TSTRING || c->next.type == TCHARS) free(c->next.value);
}

TokenType compiler_advance(Compiler *c)
{
    if (c->next.type == TEND) return die(&c->t, "Unexpected EOF");
    c->cur = c->next;
    c->next.type = tokenizer_token(&c->t, &c->next.value);
    return c->cur.type;
}

bool compiler_match(Compiler *c, TokenType t)
{
    return t == c->next.type ? compiler_advance(c) : false;
}

TokenType compiler_matches(Compiler *c, TokenType *toks)
{
    for (int i = 0; toks[i]; i++)
    {
        if (toks[i] == c->next.type) return compiler_advance(c);
    }
    return false;
}

bool compiler_consume(Compiler *c, TokenType t)
{
    return compiler_match(c, t) ? true : die(&c->t, "Unexpected Token %d", c->cur.type);
}

bool compiler_end(Compiler *c)
{
    return c->next.type == TEND;
}

bool compiler_emit8(Compiler *c, uint8_t byte)
{
    c->ip++;
    if (c->is_first_pass) return true;

    c->outbin[c->outbin_len++] = byte;
    if (c->outbin_len >= c->outbin_size)
    {
        c->outbin_size *= 2;
        c->outbin = realloc(c->outbin, c->outbin_size);
        if (!c->outbin) return false;
    }
    return true;
}

bool compiler_emit16(Compiler *c, uint16_t word)
{
    HANDLE(compiler_emit8(c, word & 0xff));
    return compiler_emit8(c, word >> 8);
}

int64_t compiler_relative(Compiler *c, size_t absolute, size_t opsize)
{
    return absolute - c->ip - opsize;
}

bool compiler_expr(Compiler *c, size_t *res);

bool compiler_dot(Compiler *c)
{
    if (!c->cur_label) return die(&c->t, "idk what message to put here");
    c->cur.value = realloc(c->cur.value, strlen(c->cur_label) + strlen(c->cur.value) + 1);
    strcat(c->cur.value, c->cur_label);
    return true;
}

bool compiler_primary(Compiler *c, size_t *res)
{
    switch (compiler_advance(c))
    {
        case TIP:
            *res = c->ip;
            break;
        case TORIGIN:
            *res = c->org;
            break;
        case TNUM:
            *res = (size_t)c->cur.value;
            break;
        case TCHARS:
            *res = 0;
            for (size_t i = 0; i < strlen(c->cur.value); i++)
            {
                *res <<= 8;
                *res |= ((char *)c->cur.value)[i];
            }
            free(c->cur.value);
            c->cur.value = NULL;
            break;
        case TLEFTPAREN:
            HANDLE(compiler_expr(c, res));
            HANDLE(compiler_consume(c, TRIGHTPAREN));
            break;
        case TDOT:
            HANDLE(compiler_consume(c, TIDENT));
            HANDLE(compiler_dot(c));
            __attribute__((fallthrough));
        case TIDENT:
            if (c->is_first_pass)
            {
                *res = 0;
                free(c->cur.value);
                c->cur.value = NULL;
                break;
            }
            if (!map_get(&c->idents, c->cur.value, res)) return die(&c->t, "undefined identifier %s", (char *)c->cur.value);
            free(c->cur.value);
            c->cur.value = NULL;
            break;
        default: return die(&c->t, "Unexpected token %d", c->cur.type);
    }

    return true;
}

bool compiler_unary(Compiler *c, size_t *res)
{
    if (c->is_const_expr)
    {
        int64_t e = 1;
        while (compiler_match(c, TMINUS)) e *= -1;
        HANDLE(compiler_primary(c, res));
        *res = (int64_t)*res * e;
        return true;
    }

    return die(&c->t, "compiler_unary: Unimplemented");
}

bool compiler_binary(Compiler *c, size_t *res, bool (*fn)(Compiler *c, size_t *res), TokenType *toks)
{
    TokenType t;
    size_t r;
    HANDLE(fn(c, res));

    if (c->is_const_expr)
    {
        while ((t = compiler_matches(c, toks)))
        {
            HANDLE(fn(c, &r));
            switch (t)
            {
                case TSHIFTRIGHT: *res >>= r; break;
                case TSHIFTLEFT: *res <<= r; break;
                case TBITWISEXOR: *res ^= r; break;
                case TBITWISEAND: *res &= r; break;
                case TBITWISEOR: *res |= r; break;
                case TPLUS: *res += r; break;
                case TMINUS: *res -= r; break;
                case TMODULOU: *res %= r; break;
                case TMODULOS: *res = (int64_t)*res % (int64_t)r; break;
                case TSLASHU: *res /= r; break;
                case TSLASHS: *res = (int64_t)*res / (int64_t)r; break;
                case TSTARU: *res *= r; break;
                case TSTARS: *res = (int64_t)*res * (int64_t)r; break;
                case TGREATERTHANU: *res = *res > r; break;
                case TGREATERTHANS: *res = (int64_t)*res > (int64_t)r; break;
                case TGREATEREQUALSU: *res = *res >= r; break;
                case TGREATEREQUALSS: *res = (int64_t)*res >= (int64_t)r; break;
                case TLESSTHANU: *res = *res < r; break;
                case TLESSTHANS: *res = (int64_t)*res < (int64_t)r; break;
                case TLESSEQUALSU: *res = *res <= r; break;
                case TLESSEQUALSS: *res = (int64_t)*res <= (int64_t)r; break;
                case TEQUALS: *res = *res == r; break;
                case TNOTEQUALS: *res = *res != r; break;
                default: return die(&c->t, "compiler_binary: Unimplemented");
            }
        }
        return true;
    }

    return die(&c->t, "compiler_binary: Unimplemented");
}

bool compiler_factor(Compiler *c, size_t *res)
{
    return compiler_binary(c, res, compiler_unary, (TokenType[]){TMODULOU, TMODULOS, TSLASHU, TSLASHS, TSTARU, TSTARS, 0});
}

bool compiler_term(Compiler *c, size_t *res)
{
    return compiler_binary(c, res, compiler_factor, (TokenType[]){TPLUS, TMINUS, 0});
}

bool compiler_bitwise(Compiler *c, size_t *res)
{
    return compiler_binary(c, res, compiler_term, (TokenType[]){TSHIFTRIGHT, TSHIFTLEFT, TBITWISEXOR, TBITWISEOR, TBITWISEAND, 0});
}

bool compiler_comparison(Compiler *c, size_t *res)
{
    return compiler_binary(c, res, compiler_bitwise, (TokenType[]){TGREATERTHANU, TGREATERTHANS, TLESSTHANU, TLESSTHANS, TGREATEREQUALSU, TGREATEREQUALSS, TLESSEQUALSU, TLESSEQUALSS, 0});
}

bool compiler_equality(Compiler *c, size_t *res)
{
    return compiler_binary(c, res, compiler_comparison, (TokenType[]){TEQUALS, TNOTEQUALS, 0});
}

bool compiler_logical_and(Compiler *c, size_t *res)
{
    return compiler_binary(c, res, compiler_equality, (TokenType[]){TLOGICALAND, 0});
}

bool compiler_logical_or(Compiler *c, size_t *res)
{
    return compiler_binary(c, res, compiler_logical_and, (TokenType[]){TLOGICALOR, 0});
}

bool compiler_ternary(Compiler *c, size_t *res)
{
    size_t l, m, r;
    HANDLE(compiler_logical_or(c, &l));

    if (c->is_const_expr)
    {
        if (compiler_match(c, TQMARK))
        {
            HANDLE(compiler_ternary(c, &m));
            HANDLE(compiler_consume(c, TCOLON));
            HANDLE(compiler_ternary(c, &r));
            if (l) *res = m;
            else *res = r;
            return true;
        }
        *res = l;
        return true;
    }
    
    return die(&c->t, "compiler_ternary: Unimplemented");
}

bool compiler_expr(Compiler *c, size_t *res)
{
    return compiler_ternary(c, res);
}

bool compiler_const_expr(Compiler *c, size_t *res)
{
    c->is_const_expr = true;
    bool ret = compiler_expr(c, res);
    c->is_const_expr = false;
    return ret;
}

bool compiler_regx(Compiler *c, size_t *res, TokenType begin, TokenType end)
{
    if (c->next.type < begin || c->next.type > end || !compiler_advance(c)) return false;

    *res = c->cur.type - begin;
    return true;
}

#define compiler_reg16(c, res) compiler_regx(c, res, TAX, TDI)
#define compiler_reg8(c, res) compiler_regx(c, res, TAL, TBH)
#define compiler_seg(c, res) compiler_regx(c, res, TES, TDS)

bool compiler_imm8(Compiler *c, size_t *imm)
{
    HANDLE(compiler_const_expr(c, imm));
    if (!c->is_first_pass && *imm > 0xff) return die(&c->t, "immediate too large");
    return true;
}

bool compiler_imm16(Compiler *c, size_t *imm)
{
    HANDLE(compiler_const_expr(c, imm));
    if (!c->is_first_pass && *imm > 0xffff) return die(&c->t, "immediate too large");
    return true;
}

bool compiler_mem(Compiler *c, int *size, uint8_t *modrm, size_t *disp)
{
    *disp = 0;
    *modrm = 0;
    if (compiler_match(c, TBYTE)) *size = 8;
    else if (compiler_match(c, TWORD)) *size = 16;
    else *size = 0;

    if (!compiler_match(c, TLEFTBRACKET)) return false;

    switch (c->next.type)
    {
        case TES:
        case TCS:
        case TSS:
        case TDS:
            HANDLE(compiler_emit8(c, 0x26 + (c->next.type - TES) * 8));
            HANDLE(compiler_advance(c));
            HANDLE(compiler_consume(c, TCOLON));
            break;
        default: break;
    }

    TokenType base = TINVALID;
    switch (c->next.type)
    {
        case TBP: *modrm = MODRM(0b10, 0, 0b110); break;
        case TBX: *modrm = MODRM(0, 0, 0b111); break;
        case TDI: *modrm = MODRM(0, 0, 0b101); break;
        case TSI: *modrm = MODRM(0, 0, 0b100); break;
        default: goto noadvance;
    }

    base = compiler_advance(c);
    HANDLE(base);
noadvance:

    if (base == TINVALID || compiler_match(c, TPLUS))
    {
        HANDLE(compiler_imm16(c, disp));
        if (base == TINVALID) *modrm = MODRM(0, 0, 0b110);
        else *modrm = MODRM(0b10, 0, *modrm);
    }

    HANDLE(compiler_consume(c, TRIGHTBRACKET));

    return true;
}

bool compiler_grp1(Compiler *c, uint8_t base, uint8_t reg)
{
    size_t d, s;
    if (compiler_reg16(c, &d))
    {
        HANDLE(compiler_consume(c, TCOMMA));
        if (compiler_reg16(c, &s))
        {
            HANDLE(compiler_emit8(c, base + 3));
            HANDLE(compiler_emit8(c, MODRM(0b11, d, s)));
            return true;
        }
        else if (compiler_imm16(c, &s))
        {
            HANDLE(compiler_emit8(c, 0x81));
            HANDLE(compiler_emit8(c, MODRM(0b11, reg, d)));
            HANDLE(compiler_emit16(c, s));
            return true;
        }
    }
    return die(&c->t, "compiler_grp1: Unimplemented");
}

bool compiler_grp2(Compiler *c, uint8_t reg)
{
    size_t d, s;

    if (compiler_reg16(c, &d))
    {
        HANDLE(compiler_consume(c, TCOMMA));
        if (compiler_imm8(c, &s))
        {
            HANDLE(compiler_emit8(c, 0xC1));
            HANDLE(compiler_emit8(c, MODRM(0b11, reg, d)));
            HANDLE(compiler_emit8(c, s));
            return true;
        }
    }

    return die(&c->t, "compiler_grp2: Unimplemented");
}

bool compiler_mov(Compiler *c)
{
    size_t d, s;
    int size;
    uint8_t modrm;
    size_t disp;

    if (compiler_reg16(c, &d))
    {
        HANDLE(compiler_consume(c, TCOMMA));
        if (compiler_reg16(c, &s))
        {
            HANDLE(compiler_emit8(c, 0x8B));
            HANDLE(compiler_emit8(c, MODRM(0b11, d, s)));
            return true;
        }
        else if (compiler_imm16(c, &s))
        {
            HANDLE(compiler_emit8(c, 0xC7));
            HANDLE(compiler_emit8(c, MODRM(0b11, 0, d)));
            HANDLE(compiler_emit16(c, s));
            return true;
        }
    }
    else if (compiler_reg8(c, &d))
    {
        HANDLE(compiler_consume(c, TCOMMA));
        if (compiler_reg8(c, &s))
        {
            HANDLE(compiler_emit8(c, 0x8A));
            HANDLE(compiler_emit8(c, MODRM(0b11, d, s)));
            return true;
        }
        else if (compiler_mem(c, &size, &modrm, &disp))
        {
            if (size != 8 && size != 0) return die(&c->t, "operand size mismatch");
            HANDLE(compiler_emit8(c, 0x8A));
            HANDLE(compiler_emit8(c, MODRM(0, d, modrm)));
            if (disp || (modrm >> 6) == 0b10) HANDLE(compiler_emit16(c, disp));
            return true;
        }
        else if (compiler_imm8(c, &s))
        {
            HANDLE(compiler_emit8(c, 0xC6));
            HANDLE(compiler_emit8(c, MODRM(0b11, 0, d)));
            HANDLE(compiler_emit8(c, s));
            return true;
        }
    }
    else if (compiler_seg(c, &d))
    {
        HANDLE(compiler_consume(c, TCOMMA));
        if (compiler_reg16(c, &s))
        {
            HANDLE(compiler_emit8(c, 0x8E));
            HANDLE(compiler_emit8(c, MODRM(0b11, d, s)));
            return true;
        }
    }
    else if (compiler_mem(c, &size, &modrm, &disp))
    {
        HANDLE(compiler_consume(c, TCOMMA));
        if (size == 8 && compiler_imm8(c, &s))
        {
            HANDLE(compiler_emit8(c, 0xC6));
            HANDLE(compiler_emit8(c, MODRM(0, 0, modrm)));
            if (disp || (modrm >> 6) == 0b10) HANDLE(compiler_emit16(c, disp));
            HANDLE(compiler_emit8(c, s));
            return true;
        }
    }

    return die(&c->t, "compiler_mov: Unimplemented");
}

bool compiler_jmp(Compiler *c, uint8_t op, uint8_t opfar, uint8_t reg)
{
    size_t x;
    (void)opfar;
    (void)reg;

    if (compiler_imm16(c, &x))
    {
        HANDLE(compiler_emit8(c, op));
        HANDLE(compiler_emit16(c, compiler_relative(c, x, 2)));
        return true;
    }

    return die(&c->t, "compiler_jmp: Unimplemented");
}

bool compiler_jmp8(Compiler *c, uint8_t op)
{
    int64_t x;
    HANDLE(compiler_const_expr(c, (size_t *)&x));
    x = compiler_relative(c, x, 2);
    if (!c->is_first_pass && (x > 0xff || x < -0x80)) return die(&c->t, "immediate too large");
    HANDLE(compiler_emit8(c, op));
    HANDLE(compiler_emit8(c, x));

    return true;
}

bool compiler_int(Compiler *c)
{
    size_t x;
    HANDLE(compiler_imm8(c, &x));
    HANDLE(compiler_emit8(c, 0xCD));
    HANDLE(compiler_emit8(c, x));
    return true;
}

bool compiler_test(Compiler *c)
{
    size_t d, s;

    if (compiler_reg8(c, &d))
    {
        HANDLE(compiler_consume(c, TCOMMA));
        if (compiler_reg8(c, &s))
        {
            HANDLE(compiler_emit8(c, 0x84));
            HANDLE(compiler_emit8(c, MODRM(0b11, d, s)));
            return true;
        }
    }

    return die(&c->t, "compiler_test: Unimplemented");
}

bool compiler_ident(Compiler *c)
{
    char *ident = c->cur.value;

    if (compiler_match(c, TCOLON))
    {
        map_set(&c->idents, ident, c->ip);
        c->cur_label = ident;
        return true;
    }

    return die(&c->t, "compiler_ident: Unimplemented");
}

bool compiler_db(Compiler *c)
{
    size_t x;
l:
    if (compiler_match(c, TCHARS))
    {
        for (size_t i = 0; i < strlen(c->cur.value); i++)
        {
            HANDLE(compiler_emit8(c, ((char *)c->cur.value)[i]));
        }
        free(c->cur.value);
        c->cur.value = NULL;
    }
    else
    {
        HANDLE(compiler_imm8(c, &x));
        HANDLE(compiler_emit8(c, x));
    }
    if (compiler_match(c, TCOMMA)) goto l;

    return true;
}

bool compiler_pushpop(Compiler *c, bool is_push)
{
    uint8_t op = is_push ? 0x50 : 0x58;
    size_t x;

    if (compiler_reg16(c, &x))
    {
        compiler_emit8(c, op | x);
        return true;
    }

    return die(&c->t, "compiler_pushpop: Unimplemented");
}

bool compiler_statement(Compiler *c)
{
    size_t x;
    switch (compiler_advance(c))
    {
        case TDB: HANDLE(compiler_db(c)); break;
        case TORG: HANDLE(compiler_const_expr(c, &c->org)); c->ip = c->org; break;
        case TRB: HANDLE(compiler_const_expr(c, &x)); c->ip += x; break;
        case TADD: HANDLE(compiler_grp1(c, 0x00, 0)); break;
        case TCALL: HANDLE(compiler_jmp(c, 0xE8, 0x9A, 2)); break;
        case TCLI: HANDLE(compiler_emit8(c, 0xFA)); break;
        case TINT: HANDLE(compiler_int(c)); break;
        case TJMP: HANDLE(compiler_jmp(c, 0xE9, 0xEA, 4)); break;
        case TJB: HANDLE(compiler_jmp8(c, 0x73)); break;
        case TJZ: HANDLE(compiler_jmp8(c, 0x74)); break;
        case TLODSB: HANDLE(compiler_emit8(c, 0xAC)); break;
        case TMOV: HANDLE(compiler_mov(c)); break;
        case TPUSH: HANDLE(compiler_pushpop(c, true)); break;
        case TPOP: HANDLE(compiler_pushpop(c, false)); break;
        case TRET: HANDLE(compiler_emit8(c, 0xC3)); break;
        case TSHL: HANDLE(compiler_grp2(c, 4)); break;
        case TSTI: HANDLE(compiler_emit8(c, 0xFB)); break;
        case TSUB: HANDLE(compiler_grp1(c, 0x28, 5)); break;
        case TTEST: HANDLE(compiler_test(c)); break;
        case TUSE16: c->use32 = false; break;
        case TUSE32: c->use32 = true; break;
        case TXOR: HANDLE(compiler_grp1(c, 0x30, 6)); break;
        case TDOT: HANDLE(compiler_dot(c)); break;
        case TIDENT: HANDLE(compiler_ident(c)); break;
        default: return die(&c->t, "Unexpected token %d", c->cur.type);
    }
    return true;
}

bool compiler_pass(Compiler *c, bool is_first_pass)
{
    c->is_first_pass = is_first_pass;

    c->cur = (Token){TINVALID, NULL};
    c->next = (Token){TINVALID, NULL};
    HANDLE(compiler_advance(c));

    c->ip = c->org = 0;
    c->use32 = true;
    c->cur_label = NULL;

    while (!compiler_end(c)) HANDLE(compiler_statement(c));

    return true;
}

uint8_t *compile(Compiler *c, char *src, size_t *outbin_len)
{
    c->outbin_size = 1024;
    c->outbin = malloc(c->outbin_size);
    c->outbin_len = 0;

    tokenizer_init(&c->t, src);
    if (!compiler_pass(c, true)) 
    {
        free(c->outbin);
        return NULL;
    }
    tokenizer_init(&c->t, src);
    if (!compiler_pass(c, false)) 
    {
        free(c->outbin);
        return NULL;
    }

    *outbin_len = c->outbin_len;
    return c->outbin;
}