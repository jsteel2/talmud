#include <stdio.h>
#include <stdlib.h>
#include "compiler.h"

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("usage: %s <srcfile> <outfile>\r\n", argv[0]);
        return -1;
    }

    FILE *srcfile = fopen(argv[1], "r");
    if (!srcfile)
    {
        printf("Could not open srcfile %s\r\n", argv[1]);
        return -1;
    }

    FILE *outfile = fopen(argv[2], "wb");
    if (!outfile)
    {
        printf("Could not open outfile %s\r\n", argv[2]);
        return -1;
    }

    fseek(srcfile, 0, SEEK_END);
    size_t srcsize = ftell(srcfile);
    fseek(srcfile, 0, SEEK_SET);

    char *src = malloc(srcsize + 1);
    fread(src, sizeof(char), srcsize, srcfile);
    src[srcsize] = 0;

    Compiler compiler;
    compiler_init(&compiler);

    size_t outbin_size;
    uint8_t *outbin = compile(&compiler, src, &outbin_size);
    free(src);
    fwrite(outbin, sizeof(uint8_t), outbin_size, outfile);
}