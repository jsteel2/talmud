#include <stdlib.h>
#include "compiler.h"

void compiler_init(Compiler *c)
{

}

bool compiler_pass(Compiler *c, bool is_first_pass)
{
    c->is_first_pass = is_first_pass;
    return true;
}

uint8_t *compile(Compiler *c, char *src, size_t *outbin_len)
{
    c->outbin = malloc(1024);
    c->outbin_size = 1024;
    c->outbin_len = 0;

    c->tokenizer.src = src;

    tokenizer_reset(&c->tokenizer);
    if (!compiler_pass(c, true)) return NULL;
    tokenizer_reset(&c->tokenizer);
    if (!compiler_pass(c, false)) return NULL;

    *outbin_len = c->outbin_len;
    return c->outbin;
}