#include <stdlib.h>
#include <stdarg.h>
#include "compiler.h"

#define HANDLE(x) if (!(x)) return false
#define MODRM(mod, reg, rm) (((mod) << 6) | ((reg) << 3) | (rm))

void compiler_init(Compiler *c)
{
    (void)c;
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

bool compiler_expr(Compiler *c, size_t *res);

bool compiler_primary(Compiler *c, size_t *res)
{
    switch (compiler_advance(c))
    {
        case TNUM: *res = (size_t)c->cur.value; break;
        case TLEFTPAREN: HANDLE(compiler_expr(c, res)); HANDLE(compiler_consume(c, TRIGHTPAREN)); break;
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

int compiler_regx(Compiler *c, TokenType begin, TokenType end)
{
    if ((c->next.type < begin && c->next.type > end) || !compiler_advance(c)) return -1;

    return c->cur.type - begin;
}

#define compiler_reg16(c) compiler_regx(c, TAX, TDI)
#define compiler_seg(c) compiler_regx(c, TES, TDS)

bool compiler_grp1(Compiler *c, uint8_t base, uint8_t reg)
{
    (void)reg;
    int d, s;
    if ((d = compiler_reg16(c)) >= 0)
    {
        HANDLE(compiler_consume(c, TCOMMA));
        if ((s = compiler_reg16(c)) >= 0)
        {
            HANDLE(compiler_emit8(c, base + 3));
            HANDLE(compiler_emit8(c, MODRM(0b11, d, s)));
            return true;
        }
    }
    return die(&c->t, "compiler_grp1: Unimplemented");
}

bool compiler_mov(Compiler *c)
{
    int d, s;

    if ((d = compiler_seg(c)) >= 0)
    {
        HANDLE(compiler_consume(c, TCOMMA));
        if ((s = compiler_reg16(c)) >= 0)
        {
            HANDLE(compiler_emit8(c, 0x8E));
            HANDLE(compiler_emit8(c, MODRM(0b11, d, s)));
            return true;
        }
    }

    return die(&c->t, "compiler_mov: Unimplemented");
}

bool compiler_statement(Compiler *c)
{
    switch (compiler_advance(c))
    {
        case TORG: HANDLE(compiler_const_expr(c, &c->org)); c->ip = c->org; break;
        case TCLI: HANDLE(compiler_emit8(c, 0xFA)); break;
        case TMOV: HANDLE(compiler_mov(c)); break;
        case TSTI: HANDLE(compiler_emit8(c, 0xFB)); break;
        case TUSE16: c->use32 = false; break;
        case TUSE32: c->use32 = true; break;
        case TXOR: HANDLE(compiler_grp1(c, 0x30, 6)); break;
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