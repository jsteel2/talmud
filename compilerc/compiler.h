#ifndef __COMPILER_H
#define __COMPILER_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "tokenizer.h"
#include "map.h"

typedef struct
{
    Tokenizer t;
    uint8_t *outbin;
    size_t outbin_len;
    size_t outbin_size;
    bool is_first_pass;
    Token cur;
    Token next;
    size_t ip;
    size_t org;
    bool is_const_expr;
    bool use32;
    Map idents;
    char *cur_label;
} Compiler;

bool compiler_init(Compiler *c);
void compiler_free(Compiler *c);
uint8_t *compile(Compiler *c, char *src, size_t *outbin_len);

#endif