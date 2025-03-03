#ifndef __COMPILER_H
#define __COMPILER_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "tokenizer.h"
#include "map.h"

typedef struct
{
    size_t i, jmp;
} Case;

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
    Map locals;
    Map strings;
    size_t strings_offset;
    size_t globals_offset;
    size_t globals_start;
    size_t globals_end;
    char *cur_label;
    size_t prog_end;
    size_t *later;
    size_t later_size;
    size_t later_i;
    size_t cur_continue;
    Case *cur_cases;
    size_t cur_cases_len;
    size_t cur_default_case;
    size_t cur_break;
} Compiler;

bool compiler_init(Compiler *c);
void compiler_free(Compiler *c);
uint8_t *compile(Compiler *c, char *src, size_t *outbin_len);

#endif