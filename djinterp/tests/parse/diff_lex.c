/*******************************************************************************
* djinterp [test]                                                     diff_lex.c
*
* Self-consistency differential: extents must tile the source, and the.
*
*
* path:      /tests/parse/diff_lex.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.22
*******************************************************************************/
// self-consistency differential: extents must tile the source, and the
// spelled token stream must re-lex to itself
#include "../../inc/djinterp/parse/lang/c/lex_c.h"
#include "../../inc/djinterp/parse/lang/cpp/lex_cpp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXTOK 400000
static struct d_token a[MAXTOK], b[MAXTOK];

static int g_cpp = 0;

static struct d_lexer* make(const struct d_source* s)
{
    return g_cpp ? d_lex_cpp_create(s, D_LEX_CPP23, D_LEX_OPT_KEYWORDS)
                 : d_lex_c_create(s, D_LEX_C23, D_LEX_OPT_KEYWORDS);
}

static size_t lex_all(const char* text, size_t n, struct d_token* out)
{
    struct d_source s; d_source_init(&s, text, n, "<d>");
    struct d_lexer* lx = make(&s);
    size_t c = 0;
    while (c < MAXTOK && d_lex_next(lx, &out[c])) ++c;
    d_lex_destroy(lx);
    return c;
}

int main(int argc, char** argv)
{
    size_t files=0, tiling=0, roundtrip=0, toks=0;
    int first = 1;
    if (argc > 1 && strcmp(argv[1], "--cpp") == 0) { g_cpp = 1; first = 2; }
    for (int i = first; i < argc; ++i) {
        FILE* f = fopen(argv[i], "rb"); if (!f) continue;
        fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
        char* buf = malloc((size_t)n+1); size_t got=fread(buf,1,(size_t)n,f);
        fclose(f); buf[got]=0;

        size_t na = lex_all(buf, got, a);
        toks += na;

        // 1. extents must be strictly increasing, non-overlapping, in range
        size_t prev_end = 0; int ok = 1;
        for (size_t k = 0; k < na; ++k) {
            if (a[k].span.offset < prev_end || a[k].span.offset + a[k].span.length > got) { ok = 0; break; }
            if (a[k].span.length == 0 && a[k].kind != D_TOKEN_END) { ok = 0; break; }
            prev_end = a[k].span.offset + a[k].span.length;
        }
        if (!ok) { ++tiling; if (tiling<=5) printf("  tiling: %s\n", argv[i]); }

        // 2. spell every token separated by one space; re-lex; kinds must match
        size_t cap = got*2 + na + 16; char* re = malloc(cap); size_t w = 0;
        struct d_source s; d_source_init(&s, buf, got, "<d>");
        struct d_lexer* lx = make(&s);
        struct d_token t;
        // a line start must survive respelling: a directive is line-sensitive
        int first = 1;
        while (d_lex_next(lx, &t)) {
            if (!first) re[w++] = (t.flags & D_TOKEN_FLAG_LINE_START) ? '\n' : ' ';
            first = 0;
            size_t need = d_token_spell(lx, &t, re+w, cap-w);
            w += need; if (w+2 >= cap) break;
        }
        d_lex_destroy(lx);
        re[w] = 0;

        size_t nb = lex_all(re, w, b);
        int same = (na == nb);
        for (size_t k = 0; same && k < na; ++k)
            if (a[k].kind != b[k].kind) same = 0;
        if (!same) {
            ++roundtrip;
            if (roundtrip<=5) {
                printf("  roundtrip: %s (%zu -> %zu)\n", argv[i], na, nb);
                for (size_t k=0;k<na&&k<nb;++k) if (a[k].kind!=b[k].kind) {
                    printf("    first at %zu: %s -> %s\n", k,
                        d_token_kind_name(a[k].kind), d_token_kind_name(b[k].kind));
                    break; }
            }
        }
        free(re); free(buf); ++files;
    }
    printf("\nfiles %zu  tokens %zu  tiling failures %zu  roundtrip failures %zu\n",
           files, toks, tiling, roundtrip);
    return (tiling||roundtrip)?1:0;
}
