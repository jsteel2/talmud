#ifndef __COMPILER_H
#define __COMPILER_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "tokenizer.h"

typedef struct
{
    Tokenizer tokenizer;
    uint8_t *outbin;
    size_t outbin_len;
    size_t outbin_size;
    bool is_first_pass;
} Compiler;

void compiler_init(Compiler *c);
uint8_t *compile(Compiler *c, char *src, size_t *outbin_len);

#endif