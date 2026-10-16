/*******************************************************************************
* djinterp [test]                                                 corpus_group.c
*
* Grouping over a corpus: balance, depth, and the files that are not balanced.
*
*
* path:      /tests/parse/corpus_group.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.22
*******************************************************************************/
// grouping over a corpus: balance, depth, and the files that are not balanced
#include "../../inc/djinterp/parse/lang/c/lex_c.h"
#include "../../inc/djinterp/parse/lang/cpp/lex_cpp.h"
#include "../../inc/djinterp/parse/syntax/syntax_group.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{
    int cpp = (argc > 1 && strcmp(argv[1], "--cpp") == 0);
    size_t files = 0, balanced = 0, tokens = 0, groups = 0, directives = 0;
    size_t unclosed = 0, unopened = 0, mismatched = 0, maxdepth = 0;
    static struct d_token tok[400000];
    for (int a = cpp ? 2 : 1; a < argc; ++a) {
        FILE* f = fopen(argv[a], "rb"); if (!f) continue;
        fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
        char* b = malloc((size_t)n+1); size_t g = fread(b,1,(size_t)n,f); fclose(f);
        struct d_source s; d_source_init(&s, b, g, argv[a]); d_source_skip_bom(&s);
        struct d_lexer* lx = cpp ? d_lex_cpp_create(&s, 0, D_LEX_OPT_KEYWORDS)
                                 : d_lex_c_create(&s, 0, D_LEX_OPT_KEYWORDS);
        size_t c = d_lex_tokenize(lx, tok, sizeof tok / sizeof tok[0]);
        d_lex_destroy(lx);
        struct d_syntax_groups gr;
        if (!d_syntax_group(tok, c, &gr)) { free(b); continue; }
        for (size_t i = 0; i < c; ++i) {
            if (d_syntax_is_opener(tok[i].kind) && gr.match[i] != D_SYNTAX_NONE) ++groups;
            if (d_syntax_is_directive_start(&tok[i])) ++directives;
            size_t d = 0; uint32_t p = gr.parent[i];
            while (p != D_SYNTAX_NONE) { ++d; p = gr.parent[p]; }
            if (d > maxdepth) maxdepth = d;
        }
        if (!gr.unclosed && !gr.unopened && !gr.mismatched) ++balanced;
        else printf("  unbalanced: %s  (unclosed %zu, unopened %zu, mismatched %zu)\n",
                    argv[a], gr.unclosed, gr.unopened, gr.mismatched);
        unclosed += gr.unclosed; unopened += gr.unopened; mismatched += gr.mismatched;
        tokens += c; ++files;
        d_syntax_groups_free(&gr); free(b);
    }
    printf("\nfiles %zu  balanced %zu  tokens %zu  bracket pairs %zu  directives %zu"
           "  max depth %zu\n", files, balanced, tokens, groups, directives, maxdepth);
    printf("unclosed %zu  unopened %zu  mismatched %zu\n", unclosed, unopened, mismatched);
    return 0;
}
