/*******************************************************************************
* djinterp [test]                                                 corpus_items.c
*
* Items over a corpus: counts, recognizer failures, and the indentation.
*
*
* path:      /tests/parse/corpus_items.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.22
*******************************************************************************/
// items over a corpus: counts, recognizer failures, and the indentation
// convention measured against the structure's level and pp-depth
#include "../../inc/djinterp/parse/lang/c/lex_c.h"
#include "../../inc/djinterp/parse/lang/cpp/lex_cpp.h"
#include "../../inc/djinterp/parse/syntax/syntax_item.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{
    int cpp = (argc > 1 && !strcmp(argv[1], "--cpp"));
    size_t kinds[9] = {0}, unterminated = 0, files = 0, items = 0;
    size_t measured = 0, by_level = 0, by_level_pp = 0, by_neither = 0;
    size_t miss_label = 0, miss_braceless = 0, miss_pp = 0, miss_other = 0;
    size_t named_params = 0, fn_defs_void = 0, void_ends_return = 0;
    size_t aligned = 0, misaligned = 0;
    static struct d_token tok[400000];
    for (int a = cpp ? 2 : 1; a < argc; ++a) {
        FILE* f = fopen(argv[a], "rb"); if (!f) continue;
        fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
        char* b = malloc((size_t)n+1); size_t g = fread(b,1,(size_t)n,f); fclose(f); b[g]=0;
        struct d_source s; d_source_init(&s, b, g, argv[a]);
        struct d_lexer* lx = cpp ? d_lex_cpp_create(&s,0,D_LEX_OPT_KEYWORDS|D_LEX_OPT_EMIT_COMMENTS)
                                 : d_lex_c_create(&s,0,D_LEX_OPT_KEYWORDS|D_LEX_OPT_EMIT_COMMENTS);
        size_t c = d_lex_tokenize(lx, tok, 400000); d_lex_destroy(lx);
        struct d_syntax_groups gr; d_syntax_group(tok, c, &gr);
        struct d_syntax_items it;
        if (!d_syntax_items_build(tok, c, &gr, b, &it)) { printf("  build failed: %s\n", argv[a]); continue; }
        for (size_t k = 0; k < it.count; ++k) {
            const struct d_syntax_item* x = &it.items[k];
            ++kinds[x->kind];
            if (x->flags & D_SYNTAX_ITEM_FLAG_UNTERMINATED) {
                ++unterminated;
                if (unterminated <= 6) printf("  unterminated %-9s at %s:%u\n",
                    d_syntax_item_kind_name(x->kind), argv[a], tok[x->first].span.line);
            }
            if (x->kind == D_SYNTAX_ITEM_PARAM && x->name != D_SYNTAX_NONE) ++named_params;
            if (x->kind == D_SYNTAX_ITEM_FN_DEF && (x->flags & D_SYNTAX_ITEM_FLAG_RETURNS_VOID)) {
                ++fn_defs_void; if (x->flags & D_SYNTAX_ITEM_FLAG_ENDS_RETURN) ++void_ends_return; }
            // indentation: only items that begin a line, below file level
            const struct d_token* t = &tok[x->first];
            if (x->flags & D_SYNTAX_ITEM_FLAG_MISALIGNED) ++misaligned;
            if (x->flags & D_SYNTAX_ITEM_FLAG_ALIGNED) { ++aligned; continue; }
            if (!(t->flags & D_TOKEN_FLAG_LINE_START) || x->level == 0) continue;
            ++measured;
            unsigned want = 4u * x->level + 1u, want_pp = 4u * (x->level + x->pp_depth) + 1u;
            if (t->span.column == want) ++by_level;
            else if (x->pp_depth && t->span.column == want_pp) ++by_level_pp;
            else {
                ++by_neither;
                if (x->flags & D_SYNTAX_ITEM_FLAG_UNDER_LABEL) { ++miss_label;
                    static int sl = 0;
                    if (getenv("SHOW_LABEL") && sl++ < atoi(getenv("SHOW_LABEL")))
                        printf("  label %s:%u col %u want %u\n", argv[a], t->span.line, t->span.column, want); }
                else if (x->flags & D_SYNTAX_ITEM_FLAG_BRACELESS) ++miss_braceless;
                else if (x->pp_depth) ++miss_pp;
                else {
                    ++miss_other;
                    static int shown = 0;
                    if (getenv("SHOW_OTHER") && shown++ < atoi(getenv("SHOW_OTHER")))
                        printf("  other %s:%u col %u want %u lvl %u %s\n", argv[a], t->span.line,
                               t->span.column, want, x->level, d_syntax_item_kind_name(x->kind));
                }
            }
        }
        items += it.count; ++files;
        d_syntax_items_free(&it); d_syntax_groups_free(&gr); free(b);
    }
    printf("\nfiles %zu  items %zu  unterminated %zu\n", files, items, unterminated);
    for (unsigned k = 0; k < 9; ++k) printf("  %-12s %zu\n", d_syntax_item_kind_name(k), kinds[k]);
    printf("named params %zu   void fn-defs %zu, of which end with return %zu\n",
           named_params, fn_defs_void, void_ends_return);
    printf("aligned parameters %zu, of which misaligned %zu\n", aligned, misaligned);
    printf("\nindentation, items beginning a line below file level: %zu\n", measured);
    printf("  column == 4*level+1              %zu  (%.1f%%)\n", by_level, 100.0*by_level/measured);
    printf("  column == 4*(level+pp-depth)+1   %zu  (%.1f%%)\n", by_level_pp, 100.0*by_level_pp/measured);
    printf("  neither                          %zu  (%.1f%%)\n", by_neither, 100.0*by_neither/measured);
    printf("    under a case label %zu, braceless body %zu, inside #if %zu, other %zu\n",
           miss_label, miss_braceless, miss_pp, miss_other);
    return 0;
}
