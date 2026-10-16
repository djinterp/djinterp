/*******************************************************************************
* djinterp [test]                                              decode_literals.c
*
* Literal decoder driver: one literal per line of a file, lexed and decoded.
*
*
* path:      /tests/parse/decode_literals.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.22
*******************************************************************************/
// literal decoder driver: one literal per line of a file, lexed and decoded
// under a named dialect; prints one result line per input line, for
// diff_literal.py to compare against gcc
#include "../../inc/djinterp/parse/lang/c/lex_c.h"
#include "../../inc/djinterp/parse/lang/cpp/lex_cpp.h"
#include "../../inc/djinterp/parse/lex/lex_literal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned level_of(const char* s, int* cpp)
{
    static const struct { const char* n; unsigned l; int cpp; } t[] = {
        {"c99", D_LEX_C99, 0}, {"c11", D_LEX_C11, 0}, {"c17", D_LEX_C17, 0},
        {"c23", D_LEX_C23, 0}, {"c++11", D_LEX_CPP11, 1},
        {"c++14", D_LEX_CPP14, 1}, {"c++17", D_LEX_CPP17, 1},
        {"c++20", D_LEX_CPP20, 1}, {"c++23", D_LEX_CPP23, 1}};
    for (size_t i = 0; i < sizeof t / sizeof t[0]; ++i)
        if (!strcmp(s, t[i].n)) { *cpp = t[i].cpp; return t[i].l; }
    return 0;
}

int main(int argc, char** argv)
{
    int cpp = 0;
    if (argc != 3) { fprintf(stderr, "usage: %s std file\n", argv[0]); return 2; }
    const unsigned level = level_of(argv[1], &cpp);
    FILE* f = fopen(argv[2], "rb");
    if (!level || !f) return 2;
    char line[4096];
    unsigned number = 0;
    while (fgets(line, sizeof line, f)) {
        ++number;
        size_t n = strcspn(line, "\n");
        line[n] = 0;
        struct d_source s;
        d_source_init(&s, line, n, "<lit>");
        struct d_lexer* lx = cpp ? d_lex_cpp_create(&s, level, D_LEX_OPT_NONE)
                                 : d_lex_c_create(&s, level, D_LEX_OPT_NONE);
        struct d_parse_diag_sink sink;
        d_parse_diag_sink_init(&sink, NULL, 0u, NULL, 0u);
        d_lex_set_diagnostics(lx, &sink);
        struct d_token t, extra;
        int ok = d_lex_next(lx, &t);
        int more = d_lex_next(lx, &extra);
        struct d_lex_literal lit;
        memset(&lit, 0, sizeof lit);
        int good = ok && !more && d_lex_literal_decode_token(lx, NULL, &t, &sink, &lit)
                   && !d_parse_diag_failed(&sink);
        printf("%u %s %d %d %llx %a %u %u\n", number, good ? "OK" : "ERR",
               lit.category, lit.encoding, (unsigned long long)lit.integer,
               lit.floating, lit.suffix, lit.characters);
        d_lex_destroy(lx);
    }
    fclose(f);
    return 0;
}
