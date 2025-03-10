#include <stdio.h>
#include <stdlib.h>
#include "compiler.h"

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("usage: %s <srcfile> <outfile>\n", argv[0]);
        return -1;
    }

    FILE *srcfile = fopen(argv[1], "r");
    if (!srcfile)
    {
        fprintf(stderr, "Could not open srcfile %s\n", argv[1]);
        return -1;
    }

    FILE *outfile = fopen(argv[2], "wb");
    if (!outfile)
    {
        fprintf(stderr, "Could not open outfile %s\n", argv[2]);
        return -1;
    }

    fseek(srcfile, 0, SEEK_END);
    size_t srcsize = ftell(srcfile);
    fseek(srcfile, 0, SEEK_SET);

    char *src = malloc(srcsize + 1);
    if (!fread(src, sizeof(char), srcsize, srcfile))
    {
        fprintf(stderr, "Could not read srcfile %s\n", argv[1]);
        return -1;
    }
    src[srcsize] = 0;

    Compiler compiler;
    if (!compiler_init(&compiler)) return -1;

    size_t outbin_size;
    uint8_t *outbin = compile(&compiler, src, &outbin_size);
    free(src);
    if (!outbin) return -1;
    fwrite(outbin, sizeof(uint8_t), outbin_size, outfile);
    compiler_free(&compiler);

    return 0;
}