#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "compiler.h"

#define HANDLE(x) do { if (!(x)) return false; } while (false)
#define MODRM(mod, reg, rm) (((mod) << 6) | ((reg) << 3) | (rm))
#define SIB(scale, index, base) MODRM(scale, index, base)

bool compiler_init(Compiler *c)
{
    c->outbin = NULL;
    c->later_size = 1024;
    c->later = malloc(c->later_size * sizeof(size_t));
    HANDLE(c->later);
    HANDLE(map_init(&c->locals));
    HANDLE(map_init(&c->strings));
    return map_init(&c->idents);
}

void compiler_free(Compiler *c)
{
    map_free(&c->idents);
    map_free(&c->locals);
    map_free(&c->strings);
    free(c->outbin);
    free(c->later);

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

// ukno this could just be a comparison like in the regx functions
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
    return compiler_match(c, t) ? true : die(&c->t, "Unexpected Token %d", c->next.type);
}

bool compiler_end(Compiler *c)
{
    return c->next.type == TEND;
}

bool compiler_emit8(Compiler *c, size_t byte)
{
    c->ip++;
    if (c->is_first_pass) return true;

    c->outbin[c->outbin_len++] = byte;
    if (c->outbin_len >= c->outbin_size)
    {
        c->outbin_size *= 2;
        c->outbin = realloc(c->outbin, c->outbin_size);
        // FIXME: just malloc c->outbin_len after first pass
        if (!c->outbin) return false;
    }
    return true;
}

bool compiler_emit16(Compiler *c, size_t word)
{
    HANDLE(compiler_emit8(c, word & 0xff));
    return compiler_emit8(c, word >> 8);
}

bool compiler_emit32(Compiler *c, size_t dword)
{
    HANDLE(compiler_emit16(c, dword & 0xffff));
    return compiler_emit16(c, dword >> 16);
}

int64_t compiler_relative(Compiler *c, size_t absolute, size_t opsize)
{
    return absolute - c->ip - opsize;
}

bool compiler_expr(Compiler *c, size_t *res, Token t);
bool compiler_statement(Compiler *c);

bool compiler_sizeoverride(Compiler *c, int size)
{
    if ((size == 32 && !c->use32) || (size == 16 && c->use32)) HANDLE(compiler_emit8(c, 0x66));
    return true;
}

bool compiler_addroverride(Compiler *c, int addrsize)
{
    if ((addrsize == 32 && !c->use32) || (addrsize == 16 && c->use32)) HANDLE(compiler_emit8(c, 0x67));
    return true;
}

size_t compiler_getlater(Compiler *c)
{
    if (c->is_first_pass) return c->later_i++;

    return c->later[c->later_i++];
}

bool compiler_setlater(Compiler *c, size_t i, size_t x)
{
    if (!c->is_first_pass) return true;

    while (c->later_size < i)
    {
        c->later_size *= 2;
        c->later = realloc(c->later, c->later_size * sizeof(size_t));
        HANDLE(c->later);
    }

    c->later[i] = x;
    return true;
}

bool compiler_dot(Compiler *c)
{
    if (!c->cur_label) return die(&c->t, "idk what message to put here");
    HANDLE(compiler_consume(c, TIDENT));
    c->cur.value = realloc(c->cur.value, strlen(c->cur_label) + strlen(c->cur.value) + 1);
    strcat(c->cur.value, c->cur_label);
    return true;
}

bool compiler_call(Compiler *c)
{
    size_t s = compiler_getlater(c);
    HANDLE(compiler_emit8(c, 0x83));
    HANDLE(compiler_emit8(c, MODRM(0b11, 5, 4)));
    HANDLE(compiler_emit8(c, s * 4)); // SUB ESP, s * 4
    HANDLE(compiler_emit8(c, 0x50)); // PUSH EAX
    HANDLE(compiler_consume(c, TLEFTPAREN));
    size_t i = 1;
    while (!compiler_match(c, TRIGHTPAREN))
    {
        HANDLE(compiler_expr(c, NULL, (Token){0}));
        HANDLE(compiler_emit8(c, 0x89));
        HANDLE(compiler_emit8(c, MODRM(0b01, 0, 0b100)));
        HANDLE(compiler_emit8(c, SIB(0, 0b100, 0b100)));
        HANDLE(compiler_emit8(c, i++ * 4)); // MOV [ESP + i * 4], EAX
        if (!compiler_match(c, TCOMMA) && compiler_consume(c, TRIGHTPAREN)) break;
    }
    HANDLE(compiler_setlater(c, s, i - 1));
    HANDLE(compiler_emit8(c, 0x58)); // POP EAX
    HANDLE(compiler_emit8(c, 0xFF));
    HANDLE(compiler_emit8(c, MODRM(0b11, 2, 0))); // CALL EAX
    HANDLE(compiler_emit8(c, 0x83));
    HANDLE(compiler_emit8(c, MODRM(0b11, 0, 4)));
    HANDLE(compiler_emit8(c, s * 4)); // ADD ESP, s * 4
    return true;
}

bool compiler_index(Compiler *c, TokenType t, bool align, bool addr)
{
    HANDLE(compiler_consume(c, t));
    HANDLE(compiler_emit8(c, 0x50)); // PUSH EAX
    HANDLE(compiler_expr(c, NULL, (Token){0}));
    HANDLE(compiler_emit8(c, 0x5A)); // POP EDX
    HANDLE(compiler_consume(c, t + 1));
    if (!addr) addr = c->next.type >= TPLUSEQUALS && c->next.type <= TEQUALS;
    else addr = c->next.type != TLEFTBRACKET && c->next.type != TLEFTBRACE && c->next.type != TLEFTCHEVRON && c->next.type != TDOT;
    if (t == TLEFTBRACKET)
    {
        HANDLE(compiler_emit8(c, addr ? 0x8D : 0x8B));
        HANDLE(compiler_emit8(c, MODRM(0, 0, 0b100)));
        HANDLE(compiler_emit8(c, SIB(align ? 2 : 0, 0, 2))); // MOV/LEA EAX, [EAX + EDX * 4/1]
    }
    else if (t == TLEFTCHEVRON)
    {
        if (addr)
        {
            HANDLE(compiler_emit8(c, 0x8D));
        }
        else
        {
            HANDLE(compiler_emit8(c, 0x0F));
            HANDLE(compiler_emit8(c, 0xB7));
        }
        HANDLE(compiler_emit8(c, MODRM(0, 0, 0b100)));
        HANDLE(compiler_emit8(c, SIB(align ? 1 : 0, 0, 2))); // MOVZX/LEA EAX, WORD [EAX + EDX * 2/1]
    }
    else if (t == TLEFTBRACE)
    {
        if (addr)
        {
            HANDLE(compiler_emit8(c, 0x8D));
        }
        else
        {
            HANDLE(compiler_emit8(c, 0x0F));
            HANDLE(compiler_emit8(c, 0xB6));
        }
        HANDLE(compiler_emit8(c, MODRM(0, 0, 0b100)));
        HANDLE(compiler_emit8(c, SIB(0, 0, 2))); // MOVZX/LEA EAX, BYTE [EAX + EDX]
    }
    return true;
}

bool compiler_post(Compiler *c, bool only_index)
{
    bool align = true;
l:
    switch (c->next.type)
    {
        case TLEFTPAREN: return only_index ? false : compiler_call(c);
        case TDOT:
            HANDLE(compiler_advance(c));
            align = false;
            goto l;
        case TLEFTBRACKET:
        case TLEFTCHEVRON:
        case TLEFTBRACE:
            return compiler_index(c, c->next.type, align, only_index);
        default: return false;
    }

    return true;
}

bool compiler_primary(Compiler *c, size_t *res, Token t)
{
    size_t x;
    if (c->is_const_expr)
    {
        switch (compiler_advance(c))
        {
            case TIP:
                *res = c->ip;
                break;
            case TORIGIN:
                *res = c->org;
                break;
            case TPROGEND:
                *res = c->globals_end;
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
            case TSTRING:
                if (map_get(&c->strings, c->cur.value, &x))
                {
                    free(c->cur.value);
                    c->cur.value = NULL;
                }
                else
                {
                    HANDLE(map_set(&c->strings, c->cur.value, c->strings_offset));
                    x = c->strings_offset;
                    c->strings_offset += 8 + strlen(c->cur.value);
                }
                *res = c->prog_end + x - c->strings_offset;
                break;
            case TLEFTPAREN:
                HANDLE(compiler_expr(c, res, (Token){0}));
                HANDLE(compiler_consume(c, TRIGHTPAREN));
                break;
            case TDOT:
                HANDLE(compiler_dot(c));
                __attribute__((fallthrough));
            case TIDENT:
                if (c->is_first_pass)
                {
                    if (!map_get(&c->idents, c->cur.value, res)) *res = 0;
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

    bool is_label;

    switch (t.type ? t.type : compiler_advance(c))
    {
        case TLEFTPAREN:
            HANDLE(compiler_expr(c, res, (Token){0}));
            HANDLE(compiler_consume(c, TRIGHTPAREN));
            break;
        case TNUM:
            HANDLE(compiler_emit8(c, 0xB8));
            HANDLE(compiler_emit32(c, (size_t)c->cur.value)); // MOV EAX, c->cur.value
            break;
        case TLINE:
            HANDLE(compiler_emit8(c, 0xB8));
            HANDLE(compiler_emit32(c, c->t.line)); // MOV EAX, c->t.line
            break;
        case TFUNC:
            if (!map_get(&c->strings, c->cur_label, &x))
            {
                HANDLE(map_set(&c->strings, strdup(c->cur_label), c->strings_offset));
                x = c->strings_offset;
                c->strings_offset += 8 + strlen(c->cur_label);
            }
            HANDLE(compiler_emit8(c, 0xB8));
            HANDLE(compiler_emit32(c, c->prog_end + x - c->strings_offset)); // MOV EAX, c->prog_end + x - c->strings_offset
            break;
        case TCHARS:
            x = 0;
            for (size_t i = 0; i < strlen(c->cur.value); i++)
            {
                x <<= 8;
                x |= ((char *)c->cur.value)[i];
            }
            free(c->cur.value);
            c->cur.value = NULL;
            HANDLE(compiler_emit8(c, 0xB8));
            HANDLE(compiler_emit32(c, x)); // MOV EAX, x
            break;
        case TSTRING:
            if (map_get(&c->strings, c->cur.value, &x))
            {
                free(c->cur.value);
                c->cur.value = NULL;
            }
            else
            {
                HANDLE(map_set(&c->strings, c->cur.value, c->strings_offset));
                x = c->strings_offset;
                c->strings_offset += 8 + strlen(c->cur.value);
            }
            HANDLE(compiler_emit8(c, 0xB8));
            HANDLE(compiler_emit32(c, c->prog_end + x - c->strings_offset)); // MOV EAX, c->prog_end + x - c->strings_offset
            break;
        case TDOT:
            HANDLE(compiler_dot(c));
            __attribute__((fallthrough));
        case TIDENT:
            if (c->is_first_pass)
            {
                if (map_get(&c->locals, c->cur.value, &x)) c->ip += 6;
                else if (map_get2(&c->idents, c->cur.value, &x, &is_label) && !is_label && !(c->next.type <= TEQUALS && c->next.type >= TPLUSEQUALS)) c->ip += 6;
                else c->ip += 5;
                free(c->cur.value);
                c->cur.value = NULL;
                break;
            }
            if (map_get(&c->locals, c->cur.value, &x))
            {
                HANDLE(compiler_emit8(c, c->next.type <= TEQUALS && c->next.type >= TPLUSEQUALS ? 0x8D : 0x8B));
                HANDLE(compiler_emit8(c, MODRM(0b10, 0, 0b101)));
                HANDLE(compiler_emit32(c, x)); // MOV/LEA EAX, [EBP + x]
            }
            else if (map_get2(&c->idents, c->cur.value, &x, &is_label))
            {
                if (is_label || (c->next.type <= TEQUALS && c->next.type >= TPLUSEQUALS))
                {
                    HANDLE(compiler_emit8(c, 0xB8));
                    HANDLE(compiler_emit32(c, x)); // MOV EAX, x
                }
                else
                {
                    HANDLE(compiler_emit8(c, 0x8B));
                    HANDLE(compiler_emit8(c, MODRM(0, 0, 0b101)));
                    HANDLE(compiler_emit32(c, x)); // MOV EAX, [x]
                }
            }
            else
            {
                return die(&c->t, "undefined identifier %s", (char *)c->cur.value);
            }
            free(c->cur.value);
            c->cur.value = NULL;
            break;
        default: return die(&c->t, "Unexpected token %d", t.type ? t.type : c->cur.type);
    }

    while (compiler_post(c, false));

    return true;
}

bool compiler_unary(Compiler *c, size_t *res, Token t)
{
    if (c->is_const_expr)
    {
        int64_t e = 1;
        while (compiler_match(c, TMINUS)) e *= -1;
        HANDLE(compiler_primary(c, res, t));
        *res = (int64_t)*res * e;
        return true;
    }

    switch (t.type ? t.type : c->next.type)
    {
        case TBANG:
            if (!t.type) HANDLE(compiler_advance(c));
            HANDLE(compiler_unary(c, res, (Token){0}));
            HANDLE(compiler_emit8(c, 0x85));
            HANDLE(compiler_emit8(c, MODRM(0b11, 0, 0))); // TEST EAX, EAX
            HANDLE(compiler_emit8(c, 0xB8));
            HANDLE(compiler_emit32(c, 0)); // MOV EAX, 0
            HANDLE(compiler_emit8(c, 0x0F));
            HANDLE(compiler_emit8(c, 0x94));
            HANDLE(compiler_emit8(c, MODRM(0b11, 0, 0))); // SETZ AL
            break;
        case TMINUS:
            if (!t.type) HANDLE(compiler_advance(c));
            HANDLE(compiler_unary(c, res, (Token){0}));
            HANDLE(compiler_emit8(c, 0xF7));
            HANDLE(compiler_emit8(c, MODRM(0b11, 3, 0))); // NEG EAX
            break;
        case TBITWISENOT:
            if (!t.type) HANDLE(compiler_advance(c));
            HANDLE(compiler_unary(c, res, (Token){0}));
            HANDLE(compiler_emit8(c, 0xF7));
            HANDLE(compiler_emit8(c, MODRM(0b11, 2, 0))); // NOT EAX
            break;
        case TSTAR:
            // FIXME: ass
            if (!t.type) HANDLE(compiler_advance(c));
            HANDLE(compiler_unary(c, res, (Token){0}));
            HANDLE(compiler_emit8(c, 0x8B));
            HANDLE(compiler_emit8(c, MODRM(0, 0, 0))); // MOV EAX, [EAX]
            break;
        case TBITWISEAND:
            // FIXME: implement
            if (!t.type) HANDLE(compiler_advance(c));
            HANDLE(compiler_consume(c, TIDENT));
            size_t x;
            HANDLE(map_get(&c->locals, c->cur.value, &x));
            free(c->cur.value);
            c->cur.value = NULL;
            if (c->next.type == TLEFTBRACE || c->next.type == TLEFTBRACKET || c->next.type == TLEFTCHEVRON || c->next.type == TDOT)
            {
                HANDLE(compiler_emit8(c, 0x8B));
                HANDLE(compiler_emit8(c, MODRM(0b10, 0, 0b101)));
                HANDLE(compiler_emit32(c, x)); // MOV EAX, [EBP + x]
                while (compiler_post(c, true));
            }
            else
            {
                HANDLE(compiler_emit8(c, 0x8D));
                HANDLE(compiler_emit8(c, MODRM(0b10, 0, 0b101)));
                HANDLE(compiler_emit32(c, x)); // LEA EAX, [EBP + x]
            }
            break;
        default: return compiler_primary(c, res, t);
    }

    return true;
}

bool compiler_binary(Compiler *c, size_t *res, Token t2, bool (*fn)(Compiler *c, size_t *res, Token t2), TokenType *toks)
{
    TokenType t;
    size_t r;
    HANDLE(fn(c, res, t2));
    TokenType last = c->cur.type;

    if (c->is_const_expr)
    {
        while ((t = compiler_matches(c, toks)))
        {
            HANDLE(fn(c, &r, (Token){0}));
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
                case TEQUALSEQUALS: *res = *res == r; break;
                case TNOTEQUALS: *res = *res != r; break;
                default: return die(&c->t, "compiler_binary: Unimplemented");
            }
        }
        return true;
    }

    while ((t = compiler_matches(c, toks)))
    {
        if (t != TLOGICALAND && t != TLOGICALOR && t != TSLASHU && t != TMODULOU && t != TSLASHUEQUALS)
        {
            HANDLE(compiler_emit8(c, 0x50)); // PUSH EAX
            HANDLE(fn(c, NULL, (Token){0}));
            HANDLE(compiler_emit8(c, 0x5A)); // POP EDX
        }
        switch (t)
        {
            case TSTARU:
                HANDLE(compiler_emit8(c, 0xF7));
                HANDLE(compiler_emit8(c, MODRM(0b11, 4, 2))); // MUL EDX
                break;
            case TSLASHU:
                HANDLE(compiler_emit8(c, 0x50)); // PUSH EAX
                HANDLE(fn(c, NULL, t2));
                HANDLE(compiler_emit8(c, 0x91)); // XCHG EAX, ECX
                HANDLE(compiler_emit8(c, 0x58)); // POP EAX
                HANDLE(compiler_emit8(c, 0x99)); // CDQ
                HANDLE(compiler_emit8(c, 0xF7));
                HANDLE(compiler_emit8(c, MODRM(0b11, 6, 1))); // DIV ECX
                break;
            case TMODULOU:
                HANDLE(compiler_emit8(c, 0x50)); // PUSH EAX
                HANDLE(fn(c, NULL, t2));
                HANDLE(compiler_emit8(c, 0x91)); // XCHG EAX, ECX
                HANDLE(compiler_emit8(c, 0x58)); // POP EAX
                HANDLE(compiler_emit8(c, 0x99)); // CDQ
                HANDLE(compiler_emit8(c, 0xF7));
                HANDLE(compiler_emit8(c, MODRM(0b11, 6, 1))); // DIV ECX
                HANDLE(compiler_emit8(c, 0x92)); // XCHG EAX, EDX
                break;
            case TPLUS:
                HANDLE(compiler_emit8(c, 0x03));
                HANDLE(compiler_emit8(c, MODRM(0b11, 0, 2))); // ADD EAX, EDX
                break;
            case TMINUS:
                HANDLE(compiler_emit8(c, 0x92)); // XCHG EAX, EDX
                HANDLE(compiler_emit8(c, 0x2B));
                HANDLE(compiler_emit8(c, MODRM(0b11, 0, 2))); // SUB EAX, EDX
                break;
            case TNOTEQUALS:
                HANDLE(compiler_emit8(c, 0x3B));
                HANDLE(compiler_emit8(c, MODRM(0b11, 2, 0))); // CMP EDX, EAX
                HANDLE(compiler_emit8(c, 0xB8));
                HANDLE(compiler_emit32(c, 0)); // MOV EAX, 0
                HANDLE(compiler_emit8(c, 0x0F));
                HANDLE(compiler_emit8(c, 0x95));
                HANDLE(compiler_emit8(c, MODRM(0b11, 0, 0))); // SETNZ AL
                break;
            case TEQUALSEQUALS:
                HANDLE(compiler_emit8(c, 0x3B));
                HANDLE(compiler_emit8(c, MODRM(0b11, 2, 0))); // CMP EDX, EAX
                HANDLE(compiler_emit8(c, 0xB8));
                HANDLE(compiler_emit32(c, 0)); // MOV EAX, 0
                HANDLE(compiler_emit8(c, 0x0F));
                HANDLE(compiler_emit8(c, 0x94));
                HANDLE(compiler_emit8(c, MODRM(0b11, 0, 0))); // SETZ AL
                break;
            case TLESSTHANU:
                HANDLE(compiler_emit8(c, 0x3B));
                HANDLE(compiler_emit8(c, MODRM(0b11, 2, 0))); // CMP EDX, EAX
                HANDLE(compiler_emit8(c, 0xB8));
                HANDLE(compiler_emit32(c, 0)); // MOV EAX, 0
                HANDLE(compiler_emit8(c, 0x0F));
                HANDLE(compiler_emit8(c, 0x92));
                HANDLE(compiler_emit8(c, MODRM(0b11, 0, 0))); // SETB AL
                break;
            case TLESSTHANS:
                HANDLE(compiler_emit8(c, 0x3B));
                HANDLE(compiler_emit8(c, MODRM(0b11, 2, 0))); // CMP EDX, EAX
                HANDLE(compiler_emit8(c, 0xB8));
                HANDLE(compiler_emit32(c, 0)); // MOV EAX, 0
                HANDLE(compiler_emit8(c, 0x0F));
                HANDLE(compiler_emit8(c, 0x9C));
                HANDLE(compiler_emit8(c, MODRM(0b11, 0, 0))); // SETL AL
                break;
            case TGREATERTHANU:
                HANDLE(compiler_emit8(c, 0x3B));
                HANDLE(compiler_emit8(c, MODRM(0b11, 2, 0))); // CMP EDX, EAX
                HANDLE(compiler_emit8(c, 0xB8));
                HANDLE(compiler_emit32(c, 0)); // MOV EAX, 0
                HANDLE(compiler_emit8(c, 0x0F));
                HANDLE(compiler_emit8(c, 0x97));
                HANDLE(compiler_emit8(c, MODRM(0b11, 0, 0))); // SETA AL
                break;
            case TLESSEQUALSU:
                HANDLE(compiler_emit8(c, 0x3B));
                HANDLE(compiler_emit8(c, MODRM(0b11, 2, 0))); // CMP EDX, EAX
                HANDLE(compiler_emit8(c, 0xB8));
                HANDLE(compiler_emit32(c, 0)); // MOV EAX, 0
                HANDLE(compiler_emit8(c, 0x0F));
                HANDLE(compiler_emit8(c, 0x96));
                HANDLE(compiler_emit8(c, MODRM(0b11, 0, 0))); // SETBE AL
                break;
            case TGREATEREQUALSU:
                HANDLE(compiler_emit8(c, 0x3B));
                HANDLE(compiler_emit8(c, MODRM(0b11, 2, 0))); // CMP EDX, EAX
                HANDLE(compiler_emit8(c, 0xB8));
                HANDLE(compiler_emit32(c, 0)); // MOV EAX, 0
                HANDLE(compiler_emit8(c, 0x0F));
                HANDLE(compiler_emit8(c, 0x93));
                HANDLE(compiler_emit8(c, MODRM(0b11, 0, 0))); // SETAE AL
                break;
            case TBITWISEOR:
                HANDLE(compiler_emit8(c, 0x0B));
                HANDLE(compiler_emit8(c, MODRM(0b11, 0, 2))); // OR EAX, EDX
                break;
            case TBITWISEAND:
                HANDLE(compiler_emit8(c, 0x23));
                HANDLE(compiler_emit8(c, MODRM(0b11, 0, 2))); // AND EAX, EDX
                break;
            case TSHIFTRIGHT:
                HANDLE(compiler_emit8(c, 0x91)); // XCHG ECX, EAX
                HANDLE(compiler_emit8(c, 0xD3));
                HANDLE(compiler_emit8(c, MODRM(0b11, 5, 2))); // SHR EDX, CL
                HANDLE(compiler_emit8(c, 0x92)); // XCHG EAX, EDX
                break;
            case TSHIFTLEFT:
                HANDLE(compiler_emit8(c, 0x91)); // XCHG ECX, EAX
                HANDLE(compiler_emit8(c, 0xD3));
                HANDLE(compiler_emit8(c, MODRM(0b11, 4, 2))); // SHL EDX, CL
                HANDLE(compiler_emit8(c, 0x92)); // XCHG EAX, EDX
                break;
            case TLOGICALAND:
                HANDLE(compiler_emit8(c, 0x85));
                HANDLE(compiler_emit8(c, MODRM(0b11, 0, 0))); // TEST EAX, EAX
                size_t lfalse = compiler_getlater(c);
                HANDLE(compiler_emit8(c, 0x0F));
                HANDLE(compiler_emit8(c, 0x84));
                HANDLE(compiler_emit32(c, compiler_relative(c, lfalse, 4))); // JZ lfalse
                HANDLE(fn(c, NULL, t2));
                size_t lend = compiler_getlater(c);
                HANDLE(compiler_emit8(c, 0xE9));
                HANDLE(compiler_emit32(c, compiler_relative(c, lend, 4))); // JMP lend
                HANDLE(compiler_setlater(c, lfalse, c->ip));
                HANDLE(compiler_emit8(c, 0x33));
                HANDLE(compiler_emit8(c, MODRM(0b11, 0, 0))); // XOR EAX, EAX
                HANDLE(compiler_setlater(c, lend, c->ip));
                break;
            case TLOGICALOR:
                HANDLE(compiler_emit8(c, 0x85));
                HANDLE(compiler_emit8(c, MODRM(0b11, 0, 0))); // TEST EAX, EAX
                lend = compiler_getlater(c);
                HANDLE(compiler_emit8(c, 0x0F));
                HANDLE(compiler_emit8(c, 0x85));
                HANDLE(compiler_emit32(c, compiler_relative(c, lend, 4))); // JNZ lend
                HANDLE(fn(c, NULL, t2));
                HANDLE(compiler_setlater(c, lend, c->ip));
                break;
                // FIXME: check brace shit.. movzx,... whatevrrr
            case TEQUALS:
                HANDLE(compiler_emit8(c, last == TRIGHTBRACE ? 0x88 : 0x89));
                HANDLE(compiler_emit8(c, MODRM(0, 0, 2))); // MOV BYTE/DWORD [EDX], EAX
                break;
            case TPLUSEQUALS:
                HANDLE(compiler_emit8(c, last == TRIGHTBRACE ? 0x00 : 0x01));
                HANDLE(compiler_emit8(c, MODRM(0, 0, 2))); // ADD BYTE/DWORD [EDX], EAX
                if (last == TRIGHTBRACE)
                {
                    HANDLE(compiler_emit8(c, 0x0F));
                    HANDLE(compiler_emit8(c, 0xB7));
                    HANDLE(compiler_emit8(c, MODRM(0, 0, 2))); // MOVZX EAX, WORD [EDX]
                }
                else
                {
                    HANDLE(compiler_emit8(c, 0x8B));
                    HANDLE(compiler_emit8(c, MODRM(0, 0, 2))); // MOV EAX, [EDX]
                }
                break;
            case TMINUSEQUALS:
                HANDLE(compiler_emit8(c, 0x29));
                HANDLE(compiler_emit8(c, MODRM(0, 0, 2))); // SUB [EDX], EAX
                HANDLE(compiler_emit8(c, 0x8B));
                HANDLE(compiler_emit8(c, MODRM(0, 0, 2))); // MOV EAX, [EDX]
                break;
            case TSHIFTLEFTEQUALS:
                HANDLE(compiler_emit8(c, 0x91)); // XCHG ECX, EAX
                HANDLE(compiler_emit8(c, 0xD3));
                HANDLE(compiler_emit8(c, MODRM(0, 4, 2))); // SHL [EDX], CL
                HANDLE(compiler_emit8(c, 0x8B));
                HANDLE(compiler_emit8(c, MODRM(0, 0, 2))); // MOV EAX, [EDX]
                break;
            case TBITWISEOREQUALS:
                HANDLE(compiler_emit8(c, 0x09));
                HANDLE(compiler_emit8(c, MODRM(0, 0, 2))); // OR [EDX], EAX
                HANDLE(compiler_emit8(c, 0x8B));
                HANDLE(compiler_emit8(c, MODRM(0, 0, 2))); // MOV EAX, [EDX]
                break;
            case TBITWISEXOREQUALS:
                HANDLE(compiler_emit8(c, 0x31));
                HANDLE(compiler_emit8(c, MODRM(0, 0, 2))); // XOR [EDX], EAX
                HANDLE(compiler_emit8(c, 0x8B));
                HANDLE(compiler_emit8(c, MODRM(0, 0, 2))); // MOV EAX, [EDX]
                break;
            case TSTARUEQUALS:
                HANDLE(compiler_emit8(c, 0xF7));
                HANDLE(compiler_emit8(c, MODRM(0, 4, 2))); // MUL [EDX]
                HANDLE(compiler_emit8(c, 0x89));
                HANDLE(compiler_emit8(c, MODRM(0, 0, 2))); // MOV [EDX], EAX
                break;
            case TSLASHUEQUALS:
                HANDLE(compiler_emit8(c, 0x50)); // PUSH EAX
                HANDLE(fn(c, NULL, (Token){0}));
                HANDLE(compiler_emit8(c, 0x59)); // POP ECX
                HANDLE(compiler_emit8(c, 0x93)); // XCHG EAX, EBX
                HANDLE(compiler_emit8(c, 0x8B));
                HANDLE(compiler_emit8(c, MODRM(0, 0, 1))); // MOV EAX, [ECX]
                HANDLE(compiler_emit8(c, 0x99)); // CDQ
                HANDLE(compiler_emit8(c, 0xF7));
                HANDLE(compiler_emit8(c, MODRM(0b11, 6, 3))); // DIV EBX
                HANDLE(compiler_emit8(c, 0x89));
                HANDLE(compiler_emit8(c, MODRM(0, 0, 1))); // MOV [ECX], EAX
                break;
            default: return die(&c->t, "compiler_binary: Unimplemented");
        }
    }

    return true;
}

bool compiler_factor(Compiler *c, size_t *res, Token t)
{
    return compiler_binary(c, res, t, compiler_unary, (TokenType[]){TMODULOU, TMODULOS, TSLASHU, TSLASHS, TSTARU, TSTARS, 0});
}

bool compiler_term(Compiler *c, size_t *res, Token t)
{
    return compiler_binary(c, res, t, compiler_factor, (TokenType[]){TPLUS, TMINUS, 0});
}

bool compiler_bitwise(Compiler *c, size_t *res, Token t)
{
    return compiler_binary(c, res, t, compiler_term, (TokenType[]){TSHIFTRIGHT, TSHIFTLEFT, TBITWISEXOR, TBITWISEOR, TBITWISEAND, 0});
}

bool compiler_comparison(Compiler *c, size_t *res, Token t)
{
    return compiler_binary(c, res, t, compiler_bitwise, (TokenType[]){TGREATERTHANU, TGREATERTHANS, TLESSTHANU, TLESSTHANS, TGREATEREQUALSU, TGREATEREQUALSS, TLESSEQUALSU, TLESSEQUALSS, 0});
}

bool compiler_equality(Compiler *c, size_t *res, Token t)
{
    return compiler_binary(c, res, t, compiler_comparison, (TokenType[]){TEQUALSEQUALS, TNOTEQUALS, 0});
}

bool compiler_logical_and(Compiler *c, size_t *res, Token t)
{
    return compiler_binary(c, res, t, compiler_equality, (TokenType[]){TLOGICALAND, 0});
}

bool compiler_logical_or(Compiler *c, size_t *res, Token t)
{
    return compiler_binary(c, res, t, compiler_logical_and, (TokenType[]){TLOGICALOR, 0});
}

bool compiler_ternary(Compiler *c, size_t *res, Token t)
{
    size_t l, m, r;
    HANDLE(compiler_logical_or(c, &l, t));

    if (c->is_const_expr)
    {
        if (compiler_match(c, TQMARK))
        {
            HANDLE(compiler_ternary(c, &m, t));
            HANDLE(compiler_consume(c, TCOLON));
            HANDLE(compiler_ternary(c, &r, t));
            if (l) *res = m;
            else *res = r;
            return true;
        }
        *res = l;
        return true;
    }

    if (compiler_match(c, TQMARK))
    {
        HANDLE(compiler_emit8(c, 0x85));
        HANDLE(compiler_emit8(c, MODRM(0b11, 0, 0))); // TEST EAX, EAX
        size_t lelse = compiler_getlater(c);
        HANDLE(compiler_emit8(c, 0x0F));
        HANDLE(compiler_emit8(c, 0x84));
        HANDLE(compiler_emit32(c, compiler_relative(c, lelse, 4))); // JZ lelse
        HANDLE(compiler_ternary(c, NULL, t));
        size_t lend = compiler_getlater(c);
        HANDLE(compiler_emit8(c, 0xE9));
        HANDLE(compiler_emit32(c, compiler_relative(c, lend, 4))); // JMP lend
        HANDLE(compiler_consume(c, TCOLON));
        HANDLE(compiler_setlater(c, lelse, c->ip));
        HANDLE(compiler_ternary(c, NULL, t));
        HANDLE(compiler_setlater(c, lend, c->ip));
    }

    return true;   
}

bool compiler_assign(Compiler *c, size_t *res, Token t)
{
    return compiler_binary(c, res, t, compiler_ternary, (TokenType[]){TEQUALS, TPLUSEQUALS, TMINUSEQUALS, TSHIFTLEFTEQUALS, TSHIFTRIGHTEQUALS, TBITWISEOREQUALS, TBITWISEXOREQUALS, TSTARUEQUALS, TSLASHUEQUALS, 0});
}

bool compiler_expr(Compiler *c, size_t *res, Token t)
{
    return compiler_assign(c, res, t);
}

bool compiler_const_expr(Compiler *c, size_t *res)
{
    c->is_const_expr = true;
    bool ret = compiler_expr(c, res, (Token){0});
    c->is_const_expr = false;
    return ret;
}

bool compiler_regx(Compiler *c, int64_t *res, TokenType begin, TokenType end)
{
    if (c->next.type < begin || c->next.type > end || !compiler_advance(c)) return false;

    *res = c->cur.type - begin;
    return true;
}

#define compiler_reg32(c, res) compiler_regx(c, res, TEAX, TEDI)
#define compiler_reg16(c, res) compiler_regx(c, res, TAX, TDI)
#define compiler_reg8(c, res) compiler_regx(c, res, TAL, TBH)
#define compiler_seg(c, res) compiler_regx(c, res, TES, TGS)
#define compiler_ctrl(c, res) compiler_regx(c, res, TCR0, TCR7)

bool compiler_imm8(Compiler *c, int64_t *imm)
{
    HANDLE(compiler_const_expr(c, (size_t *)imm));
    if (!c->is_first_pass && (*imm > 0xff || *imm < -0x80)) return die(&c->t, "immediate too large");
    return true;
}

bool compiler_imm16(Compiler *c, int64_t *imm)
{
    HANDLE(compiler_const_expr(c, (size_t *)imm));
    if (!c->is_first_pass && (*imm > 0xffff || *imm < -0x8000)) return die(&c->t, "immediate too large");
    return true;
}

bool compiler_imm32(Compiler *c, int64_t *imm)
{
    HANDLE(compiler_const_expr(c, (size_t *)imm));
    if (!c->is_first_pass && (*imm > 0xffffffff || *imm < -(int64_t)0x80000000)) return die(&c->t, "immediate too large %ld", *imm);
    return true;
}

bool compiler_mem(Compiler *c, int *size, int *addrsize, uint8_t *modrm, uint8_t *sib, int64_t *disp)
{
    size_t x;
    *disp = 0;
    *modrm = 0;
    *sib = 0;
    *addrsize = 16;
    if (compiler_match(c, TBYTE)) *size = 8;
    else if (compiler_match(c, TWORD)) *size = 16;
    else if (compiler_match(c, TDWORD)) *size = 32;
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
        case TEAX: *modrm = MODRM(0, 0, 0b000); *addrsize = 32; break;
        case TECX: *modrm = MODRM(0, 0, 0b001); *addrsize = 32; break;
        case TEDX: *modrm = MODRM(0, 0, 0b010); *addrsize = 32; break;
        case TEBX: *modrm = MODRM(0, 0, 0b011); *addrsize = 32; break;
        case TESP: *modrm = MODRM(0, 0, 0b100); *addrsize = 32; *sib = SIB(0, 0b100, 0b100); break;
        case TEBP: *modrm = MODRM(0b10, 0, 0b101); *addrsize = 32; break;
        case TESI: *modrm = MODRM(0, 0, 0b110); *addrsize = 32; break;
        case TEDI: *modrm = MODRM(0, 0, 0b111); *addrsize = 32; break;
        default: goto noadvance;
    }

    HANDLE(compiler_addroverride(c, *addrsize));

    base = compiler_advance(c);
    HANDLE(base);
noadvance:

    if (base == TINVALID && c->next.type == TIDENT && map_get(&c->locals, c->next.value, &x))
    {
        HANDLE(compiler_advance(c));
        base = TEBP;
        *modrm = MODRM(0b10, 0, 0b101);
        *addrsize = 32;
        HANDLE(compiler_addroverride(c, 32));
        *disp = x;
    }

    int64_t index;
    if (base == TINVALID || compiler_match(c, TPLUS))
    {
        if (base != TINVALID && *addrsize == 16 && (c->next.type == TSI || c->next.type == TDI))
        {
            HANDLE(compiler_advance(c));
            if (base == TBX) *modrm = MODRM(0, 0, c->cur.type - TSI);
            else if (base == TBP) *modrm = MODRM(0b10, 0, 0b010 + c->cur.type - TSI);
            if (!compiler_match(c, TPLUS)) goto end;
        }
        else if (base != TINVALID && *addrsize == 32 && compiler_reg32(c, &index))
        {
            size_t scale = 0;
            if (compiler_match(c, TSTAR))
            {
                HANDLE(compiler_consume(c, TNUM));
                if ((size_t)c->cur.value == 1) scale = 0;
                else if ((size_t)c->cur.value == 2) scale = 1;
                else if ((size_t)c->cur.value == 4) scale = 2;
                else if ((size_t)c->cur.value == 8) scale = 3;
                else return die(&c->t, "invalid scale");
            }
            *sib = SIB(scale, index, *modrm & 0b111);
            *modrm = (*modrm & 0b11111000) | 0b100;
            if (!compiler_match(c, TPLUS)) goto end;
        }
        HANDLE(compiler_const_expr(c, &x));
        *disp += x;
        if (base == TINVALID && c->use32) *addrsize = 32;
        else if (*addrsize == 16 && (*disp > 0xffff || *disp < -0x8000)) return die(&c->t, "immediate too large");
        if (base == TINVALID) *modrm = MODRM(0, 0, *addrsize == 16 ? 0b110 : 0b101);
        else *modrm = MODRM(0b10, 0, *modrm);
    }

end:
    HANDLE(compiler_consume(c, TRIGHTBRACKET));

    return true;
}

bool compiler_emitmem(Compiler *c, int64_t disp, uint8_t modrm, uint8_t sib, uint8_t reg, int addrsize)
{
    HANDLE(compiler_emit8(c, MODRM(0, reg, modrm)));
    if (addrsize == 32 && (modrm & 0b111) == 0b100) HANDLE(compiler_emit8(c, sib));
    if ((modrm & 0b111) == (addrsize == 16 ? 0b110 : 0b101) || (modrm >> 6) == 0b10) HANDLE(addrsize == 16 ? compiler_emit16(c, disp) : compiler_emit32(c, disp));
    return true;
}

bool compiler_grp1(Compiler *c, uint8_t base, uint8_t reg)
{
    int64_t d, s;
    int size, addrsize;
    uint8_t modrm, sib;
    int64_t disp;

    if (compiler_reg32(c, &d))
    {
        HANDLE(compiler_consume(c, TCOMMA));
        HANDLE(compiler_sizeoverride(c, 32));
        if (compiler_reg32(c, &s))
        {
            HANDLE(compiler_emit8(c, base + 3));
            HANDLE(compiler_emit8(c, MODRM(0b11, d, s)));
            return true;
        }
        else if (compiler_mem(c, &size, &addrsize, &modrm, &sib, &disp))
        {
            HANDLE(compiler_emit8(c, base + 3));
            HANDLE(compiler_emitmem(c, disp, modrm, sib, d, addrsize));
            return true;
        }
        else if (compiler_imm32(c, &s))
        {
            HANDLE(compiler_emit8(c, 0x81));
            HANDLE(compiler_emit8(c, MODRM(0b11, reg, d)));
            HANDLE(compiler_emit32(c, s));
            return true;
        }
    }
    else if (compiler_reg16(c, &d))
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
    else if (compiler_reg8(c, &d))
    {
        HANDLE(compiler_consume(c, TCOMMA));
        if (compiler_imm8(c, &s))
        {
            HANDLE(compiler_emit8(c, 0x80));
            HANDLE(compiler_emit8(c, MODRM(0b11, reg, d)));
            HANDLE(compiler_emit8(c, s));
            return true;
        }
    }
    else if (compiler_mem(c, &size, &addrsize, &modrm, &sib, &disp))
    {
        HANDLE(compiler_consume(c, TCOMMA));
        if (size == 8 && compiler_reg8(c, &s))
        {
            HANDLE(compiler_emit8(c, base));
            HANDLE(compiler_emitmem(c, disp, modrm, sib, s, addrsize));
            return true;
        }
        else if (size == 8 && compiler_imm8(c, &s))
        {
            HANDLE(compiler_emit8(c, 0x80));
            HANDLE(compiler_emitmem(c, disp, modrm, sib, reg, addrsize));
            HANDLE(compiler_emit8(c, s));
            return true;
        }
    }

    return die(&c->t, "compiler_grp1: Unimplemented");
}

bool compiler_grp2(Compiler *c, uint8_t reg)
{
    int64_t d, s;

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

bool compiler_grp3(Compiler *c, uint8_t reg)
{
    int64_t d;
    int size, addrsize;
    uint8_t modrm, sib;
    int64_t disp;

    if (compiler_reg32(c, &d))
    {
        HANDLE(compiler_sizeoverride(c, 32));
        HANDLE(compiler_emit8(c, 0xF7));
        HANDLE(compiler_emit8(c, MODRM(0b11, reg, d)));
        return true;
    }
    else if (compiler_mem(c, &size, &addrsize, &modrm, &sib, &disp))
    {
        if (size == 0) return die(&c->t, "compiler_grp3: Unknown operand size");
        HANDLE(compiler_sizeoverride(c, size));
        HANDLE(compiler_emit8(c, size == 8 ? 0xF6 : 0xF7));
        HANDLE(compiler_emitmem(c, disp, modrm, sib, reg, addrsize));
        return true;
    }

    return die(&c->t, "compiler_grp3: Unimplemented");
}

bool compiler_mov(Compiler *c)
{
    int64_t d, s;
    int size, addrsize;
    uint8_t modrm, sib;
    int64_t disp;

    if (compiler_reg32(c, &d))
    {
        HANDLE(compiler_consume(c, TCOMMA));
        if (compiler_ctrl(c, &s))
        {
            HANDLE(compiler_sizeoverride(c, 32));
            HANDLE(compiler_emit8(c, 0x0F));
            HANDLE(compiler_emit8(c, 0x20));
            HANDLE(compiler_emit8(c, MODRM(0b11, s, d)));
            return true;
        }
        else if (compiler_reg32(c, &s))
        {
            HANDLE(compiler_sizeoverride(c, 32));
            HANDLE(compiler_emit8(c, 0x8B));
            HANDLE(compiler_emit8(c, MODRM(0b11, d, s)));
            return true;
        }
        else if (compiler_mem(c, &size, &addrsize, &modrm, &sib, &disp))
        {
            HANDLE(compiler_sizeoverride(c, 32));
            if (size != 32 && size != 0) return die(&c->t, "operand size mismatch");
            HANDLE(compiler_emit8(c, 0x8B));
            HANDLE(compiler_emitmem(c, disp, modrm, sib, d, addrsize));
            return true;
        }
        else if (compiler_imm32(c, &s))
        {
            HANDLE(compiler_sizeoverride(c, 32));
            HANDLE(compiler_emit8(c, 0xC7));
            HANDLE(compiler_emit8(c, MODRM(0b11, 0, d)));
            HANDLE(compiler_emit32(c, s));
            return true;
        }
    }
    else if (compiler_reg16(c, &d))
    {
        HANDLE(compiler_sizeoverride(c, 16));
        HANDLE(compiler_consume(c, TCOMMA));
        if (compiler_reg16(c, &s))
        {
            HANDLE(compiler_emit8(c, 0x8B));
            HANDLE(compiler_emit8(c, MODRM(0b11, d, s)));
            return true;
        }
        else if (compiler_seg(c, &s))
        {
            HANDLE(compiler_emit8(c, 0x8C));
            HANDLE(compiler_emit8(c, MODRM(0b11, s, d)));
            return true;
        }
        else if (compiler_mem(c, &size, &addrsize, &modrm, &sib, &disp))
        {
            if (size != 16 && size != 0) return die(&c->t, "operand size mismatch");
            HANDLE(compiler_emit8(c, 0x8B));
            HANDLE(compiler_emitmem(c, disp, modrm, sib, d, addrsize));
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
        else if (compiler_mem(c, &size, &addrsize, &modrm, &sib, &disp))
        {
            if (size != 8 && size != 0) return die(&c->t, "operand size mismatch");
            HANDLE(compiler_emit8(c, 0x8A));
            HANDLE(compiler_emitmem(c, disp, modrm, sib, d, addrsize));
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
            HANDLE(compiler_sizeoverride(c, 16));
            HANDLE(compiler_emit8(c, 0x8E));
            HANDLE(compiler_emit8(c, MODRM(0b11, d, s)));
            return true;
        }
        else if (compiler_reg32(c, &s))
        {
            HANDLE(compiler_sizeoverride(c, 32));
            HANDLE(compiler_emit8(c, 0x8E));
            HANDLE(compiler_emit8(c, MODRM(0b11, d, s)));
            return true;
        }
    }
    else if (compiler_ctrl(c, &d))
    {
        HANDLE(compiler_consume(c, TCOMMA));
        HANDLE(compiler_reg32(c, &s));
        HANDLE(compiler_emit8(c, 0x0F));
        HANDLE(compiler_emit8(c, 0x22));
        HANDLE(compiler_emit8(c, MODRM(0b11, d, s)));
        return true;
    }
    else if (compiler_mem(c, &size, &addrsize, &modrm, &sib, &disp))
    {
        HANDLE(compiler_consume(c, TCOMMA));
        if ((size == 8 || size == 0) && compiler_reg8(c, &s))
        {
            HANDLE(compiler_emit8(c, 0x88));
            HANDLE(compiler_emitmem(c, disp, modrm, sib, s, addrsize));
            return true;
        }
        else if ((size == 16 || size == 0) && compiler_reg16(c, &s))
        {
            HANDLE(compiler_sizeoverride(c, 16));
            HANDLE(compiler_emit8(c, 0x89));
            HANDLE(compiler_emitmem(c, disp, modrm, sib, s, addrsize));
            return true;
        }
        else if ((size == 32 || size == 0) && compiler_reg32(c, &s))
        {
            HANDLE(compiler_sizeoverride(c, 32));
            HANDLE(compiler_emit8(c, 0x89));
            HANDLE(compiler_emitmem(c, disp, modrm, sib, s, addrsize));
            return true;
        }
        else if (size == 8 && compiler_imm8(c, &s))
        {
            HANDLE(compiler_emit8(c, 0xC6));
            HANDLE(compiler_emitmem(c, disp, modrm, sib, 0, addrsize));
            HANDLE(compiler_emit8(c, s));
            return true;
        }
        else if (size == 16 && compiler_imm16(c, &s))
        {
            HANDLE(compiler_sizeoverride(c, 16));
            HANDLE(compiler_emit8(c, 0xC7));
            HANDLE(compiler_emitmem(c, disp, modrm, sib, 0, addrsize));
            HANDLE(compiler_emit16(c, s));
            return true;
        }
        else if (size == 32 && compiler_imm32(c, &s))
        {
            HANDLE(compiler_sizeoverride(c, 32));
            HANDLE(compiler_emit8(c, 0xC7));
            HANDLE(compiler_emitmem(c, disp, modrm, sib, 0, addrsize));
            HANDLE(compiler_emit32(c, s));
            return true;
        }
    }

    return die(&c->t, "compiler_mov: Unimplemented");
}

bool compiler_jmp(Compiler *c, uint8_t op, uint8_t opfar, uint8_t reg)
{
    int64_t x, y;

    int size, addrsize;
    uint8_t modrm, sib;
    int64_t disp;

    bool (*imm)(Compiler *c, int64_t *x) = c->use32 ? compiler_imm32 : compiler_imm16;
    bool (*emit)(Compiler *c, size_t x) = c->use32 ? compiler_emit32 : compiler_emit16;

    if (compiler_reg32(c, &x))
    {
        HANDLE(compiler_sizeoverride(c, 32));
        HANDLE(compiler_emit8(c, 0xFF));
        HANDLE(compiler_emit8(c, MODRM(0b11, reg, x)));
        return true;
    }
    else if (compiler_mem(c, &size, &addrsize, &modrm, &sib, &disp))
    {
        HANDLE(compiler_emit8(c, 0xFF));
        HANDLE(compiler_emitmem(c, disp, modrm, sib, 4, addrsize));
        return true;
    }
    else if (imm(c, &x))
    {
        if (compiler_match(c, TCOLON))
        {
            HANDLE(compiler_emit8(c, opfar));
            HANDLE(imm(c, &y));
            HANDLE(emit(c, y));
            HANDLE(compiler_emit16(c, x));
            return true;
        }
        HANDLE(compiler_emit8(c, op));
        HANDLE(emit(c, compiler_relative(c, x, c->use32 ? 4 : 2)));
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
    int64_t x;
    HANDLE(compiler_imm8(c, &x));
    HANDLE(compiler_emit8(c, 0xCD));
    HANDLE(compiler_emit8(c, x));
    return true;
}

bool compiler_test(Compiler *c)
{
    int64_t d, s;

    if (compiler_reg32(c, &d))
    {
        HANDLE(compiler_sizeoverride(c, 32));
        HANDLE(compiler_consume(c, TCOMMA));
        if (compiler_reg32(c, &s))
        {
            HANDLE(compiler_emit8(c, 0x85));
            HANDLE(compiler_emit8(c, MODRM(0b11, d, s)));
            return true;
        }
    }
    else if (compiler_reg8(c, &d))
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

// this is bad, do better next time
bool compiler_ident(Compiler *c, bool set_cur)
{
    char *ident = c->cur.value;

    if (compiler_match(c, TCOLON))
    {
        HANDLE(map_set(&c->idents, ident, c->ip));
        if (set_cur) c->cur_label = ident;
        return true;
    }

    HANDLE(compiler_expr(c, NULL, c->cur));
    return compiler_consume(c, TSEMICOLON);
}

bool compiler_dx(Compiler *c, bool (*emit)(Compiler *c, size_t x), bool (*imm)(Compiler *c, int64_t *x))
{
    int64_t x;
l:
    if (compiler_match(c, TCHARS))
    {
        for (size_t i = 0; i < strlen(c->cur.value); i++)
        {
            HANDLE(emit(c, ((char *)c->cur.value)[i]));
        }
        free(c->cur.value);
        c->cur.value = NULL;
    }
    else
    {
        HANDLE(imm(c, &x));
        HANDLE(emit(c, x));
    }
    if (compiler_match(c, TCOMMA)) goto l;

    return true;
}

bool compiler_pushpop(Compiler *c, bool is_push)
{
    int64_t x;
    int size, addrsize;
    uint8_t modrm, sib;
    int64_t disp;

    if (compiler_reg32(c, &x))
    {
        HANDLE(compiler_sizeoverride(c, 32));
        HANDLE(compiler_emit8(c, (is_push ? 0x50 : 0x58) | x));
        return true;
    }
    else if (compiler_reg16(c, &x))
    {
        HANDLE(compiler_emit8(c, (is_push ? 0x50 : 0x58) | x));
        return true;
    }
    else if (compiler_seg(c, &x))
    {
        switch (c->cur.type)
        {
            case TES: x = 0x06; break;
            case TCS: x = 0x0E; break;
            case TSS: x = 0x16; break;
            case TDS: x = 0x1E; break;
            default: return die(&c->t, "unreachable");
        }
        HANDLE(compiler_emit8(c, x + (is_push ? 0 : 1)));
        return true;
    }
    else if (compiler_mem(c, &size, &addrsize, &modrm, &sib, &disp))
    {
        if (size != 16 && size != 0) return die(&c->t, "compiler_pushpop: Operand size unsupported");
        HANDLE(compiler_emit8(c, is_push ? 0xFF : 0x8F));
        HANDLE(compiler_emitmem(c, disp, modrm, sib, is_push ? 6 : 0, addrsize));
        return true;
    }
    else if (is_push && c->use32 ? compiler_imm32(c, &x) : compiler_imm16(c, &x))
    {
        HANDLE(compiler_emit8(c, 0x68));
        if (c->use32) HANDLE(compiler_emit32(c, x));
        else HANDLE(compiler_emit16(c, x));
        return true;
    }

    return die(&c->t, "compiler_pushpop: Unimplemented");
}

bool compiler_movzx(Compiler *c)
{
    int64_t d, s;
    int size, addrsize;
    uint8_t modrm, sib;
    int64_t disp;
    (void)s;

    if (compiler_reg32(c, &d))
    {
        HANDLE(compiler_sizeoverride(c, 32));
        HANDLE(compiler_emit8(c, 0x0F));
        HANDLE(compiler_consume(c, TCOMMA));
        if (compiler_mem(c, &size, &addrsize, &modrm, &sib, &disp))
        {
            if (size == 16) HANDLE(compiler_emit8(c, 0xB7));
            else if (size == 8) HANDLE(compiler_emit8(c, 0xB6));
            else return die(&c->t, "compiler_movzx: Operand size unsupported");
            HANDLE(compiler_emitmem(c, disp, modrm, sib, d, addrsize));
            return true;
        }
    }
    else if (compiler_reg16(c, &d))
    {
        HANDLE(compiler_emit8(c, 0x0F));
        HANDLE(compiler_consume(c, TCOMMA));
        HANDLE(compiler_emit8(c, 0xB6));
        if (compiler_mem(c, &size, &addrsize, &modrm, &sib, &disp))
        {
            if (size != 8 && size != 0) return die(&c->t, "compiler_movzx: Operand size unsupported");
            HANDLE(compiler_emitmem(c, disp, modrm, sib, d, addrsize));
            return true;
        }
    }

    return die(&c->t, "compiler_movzx: Unimplemented");
}

bool compiler_lea(Compiler *c, uint8_t op)
{
    int64_t d;
    int size, addrsize;
    uint8_t modrm, sib;
    int64_t disp;

    if (compiler_reg16(c, &d))
    {
        HANDLE(compiler_consume(c, TCOMMA));
        HANDLE(compiler_mem(c, &size, &addrsize, &modrm, &sib, &disp));
        if (size != 0 && size != 16) return die(&c->t, "compiler_lea: Operand size unsupported");
        HANDLE(compiler_emit8(c, op));
        HANDLE(compiler_emitmem(c, disp, modrm, sib, d, addrsize));
        return true;
    }
    else if (compiler_reg32(c, &d))
    {
        HANDLE(compiler_sizeoverride(c, 32));
        HANDLE(compiler_consume(c, TCOMMA));
        HANDLE(compiler_mem(c, &size, &addrsize, &modrm, &sib, &disp));
        if (size != 0 && size != 32) return die(&c->t, "compiler_lea: Operand size unsupported");
        HANDLE(compiler_emit8(c, op));
        HANDLE(compiler_emitmem(c, disp, modrm, sib, d, addrsize));
        return true;
    }

    return die(&c->t, "compiler_lea: Unimplemented");
}

bool compiler_xchg(Compiler *c)
{
    int64_t d, s;

    if (compiler_reg32(c, &d))
    {
        HANDLE(compiler_consume(c, TCOMMA));
        HANDLE(compiler_sizeoverride(c, 32));
        if (compiler_reg32(c, &s))
        {
            HANDLE(compiler_emit8(c, 0x87));
            HANDLE(compiler_emit8(c, MODRM(0b11, d, s)));
            return true;
        }
    }

    return die(&c->t, "compiler_xchg: Unimplemented");
}

bool compiler_imul(Compiler *c)
{
    int64_t d, s, imm;

    if (compiler_reg16(c, &d))
    {
        if (compiler_match(c, TCOMMA) && compiler_reg16(c, &s))
        {
            if (compiler_match(c, TCOMMA) && compiler_imm16(c, &imm))
            {
                HANDLE(compiler_emit8(c, 0x69));
                HANDLE(compiler_emit8(c, MODRM(0b11, d, s)));
                HANDLE(compiler_emit16(c, imm));
                return true;
            }
        }
    }

    return die(&c->t, "compiler_imul: Unimplemented");
}

bool compiler_incdec(Compiler *c, uint8_t op)
{
    int64_t x;
    
    if (compiler_reg32(c, &x))
    {
        HANDLE(compiler_sizeoverride(c, 32));
        HANDLE(compiler_emit8(c, op | x));
        return true;
    }
    else if (compiler_reg16(c, &x))
    {
        HANDLE(compiler_emit8(c, op | x));
        return true;
    }

    return die(&c->t, "compiler_incdec: Unimplemented");
}

bool compiler_in(Compiler *c)
{
    int64_t imm;

    if (compiler_match(c, TAL))
    {
        HANDLE(compiler_consume(c, TCOMMA));
        if (compiler_imm8(c, &imm))
        {
            HANDLE(compiler_emit8(c, 0xE4));
            HANDLE(compiler_emit8(c, imm));
            return true;
        }
    }

    return die(&c->t, "compiler_in: Unimplemented");
}

bool compiler_out(Compiler *c)
{
    int64_t imm;

    if (compiler_imm8(c, &imm))
    {
        HANDLE(compiler_consume(c, TCOMMA));
        if (compiler_match(c, TAL))
        {
            HANDLE(compiler_emit8(c, 0xE6));
            HANDLE(compiler_emit8(c, imm));
            return true;
        }
    }

    return die(&c->t, "compiler_out: Unimplemented");
}

bool compiler_lgdt(Compiler *c, uint8_t reg)
{
    int size, addrsize;
    uint8_t modrm, sib;
    int64_t disp;
    HANDLE(compiler_mem(c, &size, &addrsize, &modrm, &sib, &disp));
    HANDLE(compiler_emit8(c, 0x0F));
    HANDLE(compiler_emit8(c, 0x01));
    HANDLE(compiler_emitmem(c, disp, modrm, sib, reg, addrsize));
    return true;
}

bool compiler_include(Compiler *c)
{
    HANDLE(compiler_consume(c, TSTRING));

    fprintf(stderr, "including file %s\n", (char *)c->cur.value);
    FILE *f = fopen(c->cur.value, "r");
    HANDLE(f);

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *src = malloc(size + 1);
    HANDLE(fread(src, sizeof(char), size, f));
    src[size] = 0;

    Tokenizer old = c->t;
    tokenizer_init(&c->t, src);

    free(c->cur.value);
    c->cur.value = NULL;

    Token cur = c->cur;
    Token next = c->next;
    c->cur = (Token){TINVALID, NULL};
    c->next = (Token){TINVALID, NULL};
    HANDLE(compiler_advance(c));

    while (!compiler_end(c)) HANDLE(compiler_statement(c));

    c->t = old;
    c->cur = cur;
    c->next = next;
    free(src); // memleak if we return false before lol

    return true;
}

bool compiler_function(Compiler *c)
{
    HANDLE(compiler_consume(c, TIDENT));
    HANDLE(map_set(&c->idents, c->cur.value, c->ip));
    c->cur_label = c->cur.value;

    HANDLE(compiler_consume(c, TLEFTPAREN));
    size_t i = 4;
    while (!compiler_match(c, TRIGHTPAREN))
    {
        i += 4;
        HANDLE(compiler_consume(c, TIDENT));
        HANDLE(map_set(&c->locals, c->cur.value, i));
        if (!compiler_match(c, TCOMMA) && compiler_consume(c, TRIGHTPAREN)) break;
    }

    HANDLE(compiler_emit8(c, 0x55)); // PUSH EBP
    HANDLE(compiler_emit8(c, 0x8B));
    HANDLE(compiler_emit8(c, MODRM(0b11, 5, 4))); // MOV EBP, ESP

    size_t stacksub = 0;
    if (compiler_match(c, TLEFTPAREN))
    {
        while (compiler_match(c, TIDENT))
        {
            stacksub += 4;
            HANDLE(map_set(&c->locals, c->cur.value, -stacksub));
            if (compiler_match(c, TLEFTBRACKET))
            {
                HANDLE(compiler_const_expr(c, &i));
                HANDLE(compiler_consume(c, TRIGHTBRACKET));
                HANDLE(compiler_emit8(c, 0x8D));
                HANDLE(compiler_emit8(c, MODRM(0b10, 0, 0b101)));
                HANDLE(compiler_emit32(c, -stacksub - i * 4)); // LEA EAX, [EBP + -stacksub - i * 4]
                HANDLE(compiler_emit8(c, 0x89));
                HANDLE(compiler_emit8(c, MODRM(0b10, 0, 0b101)));
                HANDLE(compiler_emit32(c, -stacksub)); // MOV [EBP + -stacksub], EAX
                stacksub += i * 4;
            }
            else if (compiler_match(c, TLEFTBRACE))
            {
                HANDLE(compiler_const_expr(c, &i));
                HANDLE(compiler_consume(c, TRIGHTBRACE));
                HANDLE(compiler_emit8(c, 0x8D));
                HANDLE(compiler_emit8(c, MODRM(0b10, 0, 0b101)));
                HANDLE(compiler_emit32(c, -stacksub - i)); // LEA EAX, [EBP + -stacksub - i]
                HANDLE(compiler_emit8(c, 0x89));
                HANDLE(compiler_emit8(c, MODRM(0b10, 0, 0b101)));
                HANDLE(compiler_emit32(c, -stacksub)); // MOV [EBP + -stacksub], EAX
                stacksub += i;
            }
            if (compiler_match(c, TCOMMA)) continue;
            break;
        }
        HANDLE(compiler_consume(c, TRIGHTPAREN));
    }

    if (stacksub)
    {
        HANDLE(compiler_emit8(c, 0x81));
        HANDLE(compiler_emit8(c, MODRM(0b11, 5, 4)));
        HANDLE(compiler_emit32(c, stacksub)); // SUB ESP, stacksub
    }
    if (compiler_match(c, TLEFTBRACE))
    {
        while (!compiler_match(c, TRIGHTBRACE)) HANDLE(compiler_statement(c));
    }
    else
    {
        HANDLE(compiler_statement(c));
    }
    HANDLE(compiler_emit8(c, 0xC9)); // LEAVE
    HANDLE(compiler_emit8(c, 0xC3)); // RET

    map_free2(&c->locals);
    return true;
}

bool compiler_const(Compiler *c)
{
    size_t value;
    do
    {
        HANDLE(compiler_consume(c, TIDENT));
        char *ident = c->cur.value;
        HANDLE(compiler_consume(c, TEQUALS));
        HANDLE(compiler_const_expr(c, &value));
        map_set(&c->idents, ident, value);
    } while (compiler_match(c, TCOMMA));

    return compiler_consume(c, TSEMICOLON);
}

bool compiler_struct(Compiler *c)
{
    HANDLE(compiler_consume(c, TIDENT));
    char *name = c->cur.value; // also memleaks if we return before map_set

    HANDLE(compiler_consume(c, TEQUALS));

    size_t mul;
    size_t size = 0;
    do
    {
        if (c->next.type != TDOT && c->next.type != TLEFTBRACKET && c->next.type != TLEFTBRACE && c->next.type != TLEFTCHEVRON) HANDLE(compiler_const_expr(c, &mul));
        else mul = 1;

        if (compiler_match(c, TLEFTBRACKET))
        {
            HANDLE(compiler_consume(c, TIDENT));
            HANDLE(map_set(&c->idents, c->cur.value, size));
            size += 4 * mul;
            HANDLE(compiler_consume(c, TRIGHTBRACKET));
        }
        else if (compiler_match(c, TLEFTBRACE))
        {
            HANDLE(compiler_consume(c, TIDENT));
            HANDLE(map_set(&c->idents, c->cur.value, size));
            size += 1 * mul;
            HANDLE(compiler_consume(c, TRIGHTBRACE));
        }
        else if (compiler_match(c, TLEFTCHEVRON))
        {
            HANDLE(compiler_consume(c, TIDENT));
            HANDLE(map_set(&c->idents, c->cur.value, size));
            size += 2 * mul;
            HANDLE(compiler_consume(c, TRIGHTCHEVRON));
        }
        else
        {
            return die(&c->t, "Unexpected token %d", c->next.type);
        }
    } while (compiler_match(c, TCOMMA));

    HANDLE(map_set(&c->idents, name, size));
    return compiler_consume(c, TSEMICOLON);
}

bool compiler_return(Compiler *c)
{
    if (!compiler_match(c, TSEMICOLON)) 
    {
        HANDLE(compiler_expr(c, NULL, (Token){0}));
        HANDLE(compiler_consume(c, TSEMICOLON));
    }
    HANDLE(compiler_emit8(c, 0xC9)); // LEAVE
    HANDLE(compiler_emit8(c, 0xC3)); // RET

    return true;
}

bool compiler_if(Compiler *c)
{
    HANDLE(compiler_consume(c, TLEFTPAREN));
    HANDLE(compiler_expr(c, NULL, (Token){0}));
    HANDLE(compiler_consume(c, TRIGHTPAREN));

    size_t lelse = compiler_getlater(c);

    HANDLE(compiler_emit8(c, 0x85));
    HANDLE(compiler_emit8(c, MODRM(0b11, 0, 0))); // TEST EAX, EAX
    HANDLE(compiler_emit8(c, 0x0F));
    HANDLE(compiler_emit8(c, 0x84));
    HANDLE(compiler_emit32(c, compiler_relative(c, lelse, 4))); // JZ lelse

    if (compiler_match(c, TLEFTBRACE))
    {
        while (!compiler_match(c, TRIGHTBRACE)) HANDLE(compiler_statement(c));
    }
    else
    {
        HANDLE(compiler_statement(c));
    }

    if (compiler_match(c, TELSE))
    {
        size_t lend = compiler_getlater(c);
        HANDLE(compiler_emit8(c, 0xE9));
        HANDLE(compiler_emit32(c, compiler_relative(c, lend, 4))); // JMP lend
        HANDLE(compiler_setlater(c, lelse, c->ip));
        if (compiler_match(c, TLEFTBRACE))
        {
            while (!compiler_match(c, TRIGHTBRACE)) HANDLE(compiler_statement(c));
        }
        else
        {
            HANDLE(compiler_statement(c));
        }
        HANDLE(compiler_setlater(c, lend, c->ip));
    }
    else
    {
        HANDLE(compiler_setlater(c, lelse, c->ip));
    }

    return true;
}

bool compiler_while(Compiler *c)
{
    HANDLE(compiler_consume(c, TLEFTPAREN));
    size_t loop = c->ip;
    size_t old = c->cur_continue;
    c->cur_continue = loop;
    HANDLE(compiler_expr(c, NULL, (Token){0}));
    HANDLE(compiler_emit8(c, 0x85));
    HANDLE(compiler_emit8(c, MODRM(0b11, 0, 0))); // TEST EAX, EAX
    HANDLE(compiler_emit8(c, 0x0F));
    HANDLE(compiler_emit8(c, 0x84));
    size_t loopend = compiler_getlater(c);
    HANDLE(compiler_emit32(c, compiler_relative(c, loopend, 4))); // JZ loopend
    HANDLE(compiler_consume(c, TRIGHTPAREN));

    if (compiler_match(c, TLEFTBRACE))
    {
        while (!compiler_match(c, TRIGHTBRACE)) HANDLE(compiler_statement(c));
    }
    else
    {
        HANDLE(compiler_statement(c));
    }

    HANDLE(compiler_emit8(c, 0xE9));
    HANDLE(compiler_emit32(c, compiler_relative(c, loop, 4))); // JMP loop

    HANDLE(compiler_setlater(c, loopend, c->ip));
    c->cur_continue = old;
    return true;
}

bool compiler_setcc(Compiler *c, uint8_t op)
{
    int size, addrsize;
    uint8_t modrm, sib;
    int64_t disp;

    if (compiler_mem(c, &size, &addrsize, &modrm, &sib, &disp))
    {
        if (size != 0 && size != 8) return die(&c->t, "invalid operand size");
        HANDLE(compiler_emit8(c, 0x0F));
        HANDLE(compiler_emit8(c, op));
        HANDLE(compiler_emitmem(c, disp, modrm, sib, 0, addrsize));
        return true;
    }

    return die(&c->t, "Unimplemented");
}

bool compiler_global(Compiler *c)
{
    do
    {
        HANDLE(compiler_consume(c, TIDENT));
        map_set2(&c->idents, c->cur.value, c->globals_start + c->globals_offset, false);
        c->globals_offset += 4;
    } while (compiler_match(c, TCOMMA));
    return compiler_consume(c, TSEMICOLON);
}

bool compiler_continue(Compiler *c)
{
    HANDLE(compiler_emit8(c, 0xE9));
    HANDLE(compiler_emit32(c, compiler_relative(c, c->cur_continue, 4))); // JMP c->cur_continue
    return compiler_consume(c, TSEMICOLON);
}

bool compiler_do(Compiler *c)
{
    size_t loop = c->ip;

    size_t old = c->cur_continue;
    c->cur_continue = compiler_getlater(c);

    if (compiler_match(c, TLEFTBRACE))
    {
        while (!compiler_match(c, TRIGHTBRACE)) HANDLE(compiler_statement(c));
    }
    else
    {
        HANDLE(compiler_statement(c));
    }

    HANDLE(compiler_setlater(c, c->cur_continue, c->ip));
    c->cur_continue = old;

    HANDLE(compiler_consume(c, TWHILE));
    HANDLE(compiler_consume(c, TLEFTPAREN));
    HANDLE(compiler_expr(c, NULL, (Token){0}));
    HANDLE(compiler_consume(c, TRIGHTPAREN));
    HANDLE(compiler_emit8(c, 0x85));
    HANDLE(compiler_emit8(c, MODRM(0b11, 0, 0))); // TEST EAX, EAX
    HANDLE(compiler_emit8(c, 0x0F));
    HANDLE(compiler_emit8(c, 0x85));
    HANDLE(compiler_emit32(c, compiler_relative(c, loop, 4))); // JNZ loop
    return compiler_consume(c, TSEMICOLON);
}

int case_compar(const void *p, const void *q)
{
    Case *x = (Case *)p;
    Case *y = (Case *)q;
    return (x->i > y->i) - (x->i < y->i);
}

bool compiler_switch(Compiler *c)
{
    HANDLE(compiler_consume(c, TLEFTPAREN));
    HANDLE(compiler_expr(c, NULL, (Token){0}));
    HANDLE(compiler_consume(c, TRIGHTPAREN));

    size_t lowest_case = compiler_getlater(c);
    size_t highest_case = compiler_getlater(c);
    size_t jump_table = compiler_getlater(c);
    size_t default_case = compiler_getlater(c);
    c->cur_break = compiler_getlater(c);

    HANDLE(compiler_emit8(c, 0x3D));
    HANDLE(compiler_emit32(c, lowest_case)); // CMP EAX, lowest_case
    HANDLE(compiler_emit8(c, 0x0F));
    HANDLE(compiler_emit8(c, 0x82));
    HANDLE(compiler_emit32(c, compiler_relative(c, default_case, 4))); // JB default_case
    HANDLE(compiler_emit8(c, 0x3D));
    HANDLE(compiler_emit32(c, highest_case)); // CMP EAX, highest_case
    HANDLE(compiler_emit8(c, 0x0F));
    HANDLE(compiler_emit8(c, 0x87));
    HANDLE(compiler_emit32(c, compiler_relative(c, default_case, 4))); // JA default_case
    HANDLE(compiler_emit8(c, 0xFF));
    HANDLE(compiler_emit8(c, MODRM(0, 4, 0b100)));
    HANDLE(compiler_emit8(c, SIB(2, 0, 0b101)));
    HANDLE(compiler_emit32(c, jump_table - lowest_case * 4)); // JMP [EAX * 4 + jump_table - lowest_case * 4]

    // FIXME: save old
    c->cur_cases = malloc(sizeof(Case));
    c->cur_cases_len = 0;
    c->cur_default_case = 0;

    if (compiler_match(c, TLEFTBRACE))
    {
        while (!compiler_match(c, TRIGHTBRACE)) HANDLE(compiler_statement(c));
    }
    else
    {
        HANDLE(compiler_statement(c));
    }

    qsort(c->cur_cases, c->cur_cases_len, sizeof(Case), case_compar);
    HANDLE(compiler_setlater(c, lowest_case, c->cur_cases[0].i));
    HANDLE(compiler_setlater(c, highest_case, c->cur_cases[c->cur_cases_len - 1].i));
    HANDLE(compiler_setlater(c, jump_table, c->ip));

    size_t cas = 0;
    for (size_t i = 0; i < c->cur_cases_len; i++)
    {
        size_t x = c->cur_cases[i].i - c->cur_cases[0].i;
        while (cas < x)
        {
            HANDLE(compiler_emit32(c, default_case));
            cas++;
        }
        HANDLE(compiler_emit32(c, c->cur_cases[i].jmp));
        cas++;
    }

    HANDLE(compiler_setlater(c, default_case, c->cur_default_case ? c->cur_default_case : c->ip));
    HANDLE(compiler_setlater(c, c->cur_break, c->ip));

    free(c->cur_cases);

    return true;
}

bool compiler_case(Compiler *c)
{
    size_t x;
    HANDLE(compiler_const_expr(c, &x));

    c->cur_cases[c->cur_cases_len].i = x;
    c->cur_cases[c->cur_cases_len].jmp = c->ip;
    c->cur_cases = realloc(c->cur_cases, (++c->cur_cases_len + 1) * sizeof(Case));

    return compiler_consume(c, TCOLON);
}

bool compiler_break(Compiler *c)
{
    HANDLE(compiler_emit8(c, 0xE9));
    HANDLE(compiler_emit32(c, compiler_relative(c, c->cur_break, 4))); // JMP c->cur_break
    return compiler_consume(c, TSEMICOLON);
}

bool compiler_default(Compiler *c)
{
    c->cur_default_case = c->ip;
    return compiler_consume(c, TCOLON);
}

bool compiler_statement(Compiler *c)
{
    size_t x;
    switch (compiler_advance(c))
    {
        case TDB: HANDLE(compiler_dx(c, compiler_emit8, compiler_imm8)); break;
        case TDW: HANDLE(compiler_dx(c, compiler_emit16, compiler_imm16)); break;
        case TDD: HANDLE(compiler_dx(c, compiler_emit32, compiler_imm32)); break;
        case TORG: HANDLE(compiler_const_expr(c, &c->org)); c->ip = c->org; break;
        case TRB: HANDLE(compiler_const_expr(c, &x)); c->ip += x; break;
        case TADD: HANDLE(compiler_grp1(c, 0x00, 0)); break;
        case TADC: HANDLE(compiler_grp1(c, 0x10, 2)); break;
        case TAND: HANDLE(compiler_grp1(c, 0x20, 4)); break;
        case TCALL: HANDLE(compiler_jmp(c, 0xE8, 0x9A, 2)); break;
        case TCDQ: HANDLE(compiler_emit16(c, 0x9966)); break;
        case TCLI: HANDLE(compiler_emit8(c, 0xFA)); break;
        case TCLD: HANDLE(compiler_emit8(c, 0xFC)); break;
        case TCMPSB: HANDLE(compiler_emit8(c, 0xA6)); break;
        case TCMP: HANDLE(compiler_grp1(c, 0x38, 7)); break;
        case TCWDE: HANDLE(compiler_emit16(c, 0x9866)); break;
        case TDEC: HANDLE(compiler_incdec(c, 0x48)); break;
        case TDIV: HANDLE(compiler_grp3(c, 6)); break;
        case THLT: HANDLE(compiler_emit8(c, 0xF4)); break;
        case TIMUL: HANDLE(compiler_imul(c)); break;
        case TINC: HANDLE(compiler_incdec(c, 0x40)); break;
        case TIN: HANDLE(compiler_in(c)); break;
        case TINT: HANDLE(compiler_int(c)); break;
        case TJMP: HANDLE(compiler_jmp(c, 0xE9, 0xEA, 4)); break;
        case TJNZ: HANDLE(compiler_jmp8(c, 0x75)); break;
        case TJNB: HANDLE(compiler_jmp8(c, 0x73)); break;
        case TJB: HANDLE(compiler_jmp8(c, 0x72)); break;
        case TJZ: HANDLE(compiler_jmp8(c, 0x74)); break;
        case TLODSB: HANDLE(compiler_emit8(c, 0xAC)); break;
        case TLGDT: HANDLE(compiler_lgdt(c, 2)); break;
        case TLOOP: HANDLE(compiler_jmp8(c, 0xE2)); break;
        case TLEA: HANDLE(compiler_lea(c, 0x8D)); break;
        case TMOVZX: HANDLE(compiler_movzx(c)); break;
        case TMOVSW: HANDLE(compiler_emit8(c, 0xA5)); break;
        case TMOVSB: HANDLE(compiler_emit8(c, 0xA4)); break;
        case TMOV: HANDLE(compiler_mov(c)); break;
        case TMUL: HANDLE(compiler_grp3(c, 4)); break;
        case TOUT: HANDLE(compiler_out(c)); break;
        case TOR: HANDLE(compiler_grp1(c, 0x08, 1)); break;
        case TPUSHAD: HANDLE(compiler_emit16(c, 0x6066)); break;
        case TPUSHF: HANDLE(compiler_emit8(c, 0x9C)); break;
        case TPUSH: HANDLE(compiler_pushpop(c, true)); break;
        case TPOPF: HANDLE(compiler_emit8(c, 0x9D)); break;
        case TPOP: HANDLE(compiler_pushpop(c, false)); break;
        case TPOPAD: HANDLE(compiler_emit16(c, 0x6166)); break;
        case TRETF: HANDLE(compiler_emit8(c, 0xCB)); break;
        case TRET: HANDLE(compiler_emit8(c, 0xC3)); break;
        case TREP: HANDLE(compiler_emit8(c, 0xF3)); break;
        case TSTOSB: HANDLE(compiler_emit8(c, 0xAA)); break;
        case TSETB: HANDLE(compiler_setcc(c, 0x92)); break;
        case TSHL: HANDLE(compiler_grp2(c, 4)); break;
        case TSHR: HANDLE(compiler_grp2(c, 5)); break;
        case TSTI: HANDLE(compiler_emit8(c, 0xFB)); break;
        case TSUB: HANDLE(compiler_grp1(c, 0x28, 5)); break;
        case TTEST: HANDLE(compiler_test(c)); break;
        case TUSE16: c->use32 = false; break;
        case TUSE32: c->use32 = true; break;
        case TXCHG: HANDLE(compiler_xchg(c)); break;
        case TXOR: HANDLE(compiler_grp1(c, 0x30, 6)); break;
        case TDOT: HANDLE(compiler_dot(c)); HANDLE(compiler_ident(c, false)); break;
        case TIDENT: HANDLE(compiler_ident(c, true)); break;
        case TINCLUDE: HANDLE(compiler_include(c)); break;
        case TFUNCTION: HANDLE(compiler_function(c)); break;
        case TCONST: HANDLE(compiler_const(c)); break;
        case TRETURN: HANDLE(compiler_return(c)); break;
        case TIF: HANDLE(compiler_if(c)); break;
        case TWHILE: HANDLE(compiler_while(c)); break;
        case TDO: HANDLE(compiler_do(c)); break;
        case TSTRUCT: HANDLE(compiler_struct(c)); break;
        case TGLOBAL: HANDLE(compiler_global(c)); break;
        case TCONTINUE: HANDLE(compiler_continue(c)); break;
        case TSWITCH: HANDLE(compiler_switch(c)); break;
        case TCASE: HANDLE(compiler_case(c)); break;
        case TBREAK: HANDLE(compiler_break(c)); break;
        case TDEFAULT: HANDLE(compiler_default(c)); break;
        case TSEMICOLON: break;
        default: HANDLE(compiler_expr(c, NULL, c->cur)); HANDLE(compiler_consume(c, TSEMICOLON));
    }
    return true;
}

bool compiler_pass(Compiler *c, bool is_first_pass)
{
    c->is_first_pass = is_first_pass;
    c->later_i = 0;
    c->globals_offset = 0;

    c->cur = (Token){TINVALID, NULL};
    c->next = (Token){TINVALID, NULL};
    HANDLE(compiler_advance(c));

    c->ip = c->org = 0;
    c->use32 = true;
    c->cur_label = NULL;

    while (!compiler_end(c)) HANDLE(compiler_statement(c));

    // ew
    size_t n = 0;
    size_t x = c->ip;
    while (n != c->strings.len)
    {
        for (size_t i = 0; i < c->strings.size; i++)
        {
            if (!c->strings.entries[i].key) continue;
            if (c->strings.entries[i].value != c->ip - x) continue;
            n++;
            size_t l = strlen(c->strings.entries[i].key);
            HANDLE(compiler_emit32(c, l));
            HANDLE(compiler_emit32(c, c->ip + 4));
            for (size_t j = 0; j < l; j++)
            {
                HANDLE(compiler_emit8(c, c->strings.entries[i].key[j]));
            }
        }
    }

    return true;
}

uint8_t *compile(Compiler *c, char *src, size_t *outbin_len)
{
    c->outbin_size = 1024;
    c->outbin = malloc(c->outbin_size);
    c->outbin_len = 0;
    c->prog_end = 0;
    c->strings_offset = 0;

    tokenizer_init(&c->t, src);
    if (!compiler_pass(c, true)) 
    {
        free(c->outbin);
        c->outbin = NULL;
        return NULL;
    }
    c->globals_start = c->ip;
    c->globals_end = c->globals_start + c->globals_offset;
    c->prog_end = c->ip;
    tokenizer_init(&c->t, src);
    if (!compiler_pass(c, false)) 
    {
        free(c->outbin);
        c->outbin = NULL;
        map_free2(&c->strings);
        return NULL;
    }

    map_free2(&c->strings);
    *outbin_len = c->outbin_len;
    return c->outbin;
}