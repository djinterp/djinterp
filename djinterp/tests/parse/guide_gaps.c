/*******************************************************************************
* djinterp [test]                                                   guide_gaps.c
*
* Dogfood: measure the style guide's function tripwires on the bundle's own.
*
*
* path:      /tests/parse/guide_gaps.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.22
*******************************************************************************/
// dogfood: measure the style guide's function tripwires on the bundle's own
// sources with the item layer -- 60 lines, 4 levels of nesting, 5 parameters
#include "../../inc/djinterp/parse/lang/c/lex_c.h"
#include "../../inc/djinterp/parse/syntax/syntax_item.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char** argv)
{
    static struct d_token tok[400000];
    size_t fns = 0, over_len = 0, over_nest = 0, over_params = 0;
    for (int a = 1; a < argc; ++a) {
        FILE* f = fopen(argv[a], "rb"); if (!f) continue;
        fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
        char* b = malloc((size_t)n+1); size_t g = fread(b,1,(size_t)n,f); fclose(f); b[g]=0;
        struct d_source s; d_source_init(&s, b, g, argv[a]);
        struct d_lexer* lx = d_lex_c_create(&s, 0, D_LEX_OPT_KEYWORDS);
        size_t c = d_lex_tokenize(lx, tok, 400000); d_lex_destroy(lx);
        struct d_syntax_groups gr; d_syntax_group(tok, c, &gr);
        struct d_syntax_items it; d_syntax_items_build(tok, c, &gr, b, &it);
        for (size_t k = 0; k < it.count; ++k) {
            const struct d_syntax_item* x = &it.items[k];
            if (x->kind != D_SYNTAX_ITEM_FN_DEF || x->body == D_SYNTAX_NONE || x->name == D_SYNTAX_NONE) continue;
            ++fns;
            unsigned lines = tok[gr.match[x->body]].span.line - tok[x->first].span.line + 1;
            unsigned params = 0, nest = 0;
            for (size_t j = 0; j < it.count; ++j)
                if (it.items[j].kind == D_SYNTAX_ITEM_PARAM && it.items[j].scope == x->params &&
                    !(it.items[j].flags & D_SYNTAX_ITEM_FLAG_VOID)) ++params;
            for (uint32_t t = x->body + 1; t < gr.match[x->body]; ++t) {
                unsigned d = 0; uint32_t p = gr.parent[t];
                while (p != D_SYNTAX_NONE && p != x->body) { if (tok[p].kind == D_TOKEN_BRACE_OPEN) ++d; p = gr.parent[p]; }
                if (d > nest) nest = d;
            }
            char name[64]; size_t L = tok[x->name].span.length < 63 ? tok[x->name].span.length : 63;
            memcpy(name, b + tok[x->name].span.offset, L); name[L] = 0;
            if (lines > 60 || nest > 4 || params > 5) {
                printf("  %-36s %4u lines  nest %u  params %u   %s\n", name, lines, nest, params,
                       strrchr(argv[a], '/') + 1);
                over_len += lines > 60; over_nest += nest > 4; over_params += params > 5;
            }
        }
        d_syntax_items_free(&it); d_syntax_groups_free(&gr); free(b);
    }
    printf("\n%zu functions: %zu over 60 lines, %zu nested past 4, %zu with more than 5 parameters\n",
           fns, over_len, over_nest, over_params);
    return 0;
}
