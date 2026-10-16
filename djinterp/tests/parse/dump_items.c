/*******************************************************************************
* djinterp [test]                                                   dump_items.c
*
* Print the items of a source, indented by item nesting, for inspection.
*
*
* path:      /tests/parse/dump_items.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.22
*******************************************************************************/
// print the items of a source, indented by item nesting, for inspection
#include "../../inc/djinterp/parse/lang/c/lex_c.h"
#include "../../inc/djinterp/parse/lang/cpp/lex_cpp.h"
#include "../../inc/djinterp/parse/syntax/syntax_item.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void spell(const char* text, const struct d_token* t, char* out, size_t n)
{
    size_t len = t->span.length < n - 1 ? t->span.length : n - 1;
    memcpy(out, text + t->span.offset, len); out[len] = 0;
    for (size_t i = 0; i < len; ++i) if (out[i] == '\n') out[i] = ' ';
}

int main(int argc, char** argv)
{
    int cpp = (argc > 2 && !strcmp(argv[2], "--cpp"));
    FILE* f = fopen(argv[1], "rb"); fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    char* b = malloc((size_t)n + 1); size_t g = fread(b, 1, (size_t)n, f); fclose(f); b[g] = 0;
    struct d_source s; d_source_init(&s, b, g, argv[1]);
    struct d_lexer* lx = cpp ? d_lex_cpp_create(&s, 0, D_LEX_OPT_KEYWORDS | D_LEX_OPT_EMIT_COMMENTS)
                             : d_lex_c_create(&s, 0, D_LEX_OPT_KEYWORDS | D_LEX_OPT_EMIT_COMMENTS);
    static struct d_token tok[200000];
    size_t c = d_lex_tokenize(lx, tok, 200000); d_lex_destroy(lx);
    struct d_syntax_groups gr; d_syntax_group(tok, c, &gr);
    struct d_syntax_items it; d_syntax_items_build(tok, c, &gr, b, &it);
    for (size_t k = 0; k < it.count; ++k) {
        const struct d_syntax_item* x = &it.items[k];
        char first[40], name[40] = "";
        spell(b, &tok[x->first], first, sizeof first);
        if (x->name != D_SYNTAX_NONE) spell(b, &tok[x->name], name, sizeof name);
        printf("L%-4u lvl%u pp%u  %-11s %-8s %-18s first=%-12s%s%s%s%s%s%s%s%s\n",
            tok[x->first].span.line, x->level, x->pp_depth,
            d_syntax_item_kind_name(x->kind), d_syntax_statement_name(x->statement),
            name[0] ? name : "-", first,
            (x->flags & D_SYNTAX_ITEM_FLAG_RETURNS_VOID) ? " void" : "",
            (x->flags & D_SYNTAX_ITEM_FLAG_ENDS_RETURN) ? " ends-return" : "",
            (x->flags & D_SYNTAX_ITEM_FLAG_EMPTY_BODY) ? " empty" : "",
            (x->flags & D_SYNTAX_ITEM_FLAG_FN_POINTER) ? " fn-ptr" : "",
            (x->flags & D_SYNTAX_ITEM_FLAG_MACRO_HEAD) ? " macro" : "",
            (x->flags & D_SYNTAX_ITEM_FLAG_UNNAMED) ? " unnamed" : "",
            (x->flags & D_SYNTAX_ITEM_FLAG_BRACELESS) ? " braceless" : "",
            (x->flags & D_SYNTAX_ITEM_FLAG_UNTERMINATED) ? " UNTERMINATED" : "");
    }
    printf("\n%zu tokens, %zu items\n", c, it.count);
    d_syntax_items_free(&it); d_syntax_groups_free(&gr); free(b);
    return 0;
}
