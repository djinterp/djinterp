/*******************************************************************************
* djinterp [test]                                                   corpus_lex.c
*
* Corpus driver: lex every file given and report totals and defects.
*
*
* path:      /tests/parse/corpus_lex.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.22
*******************************************************************************/
// corpus driver: lex every file given and report totals and defects
#include "../../inc/djinterp/parse/lang/c/lex_c.h"
#include "../../inc/djinterp/parse/lang/cpp/lex_cpp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{
    int cpp = (argc > 1 && strcmp(argv[1], "--cpp") == 0);
    int first = cpp ? 2 : 1;
    size_t files = 0, tokens = 0, bad_files = 0, bytes = 0;
    size_t kw = 0, id = 0, num = 0, str = 0, ch = 0, punct = 0, hdr = 0;
    size_t spliced = 0, digraph = 0, raw = 0, suffix = 0;

    for (int a = first; a < argc; ++a) {
        FILE* f = fopen(argv[a], "rb");
        if (!f) continue;
        fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
        if (n < 0) { fclose(f); continue; }
        char* buf = malloc((size_t)n + 1);
        if (!buf) { fclose(f); continue; }
        size_t got = fread(buf, 1, (size_t)n, f); fclose(f);
        buf[got] = 0;

        struct d_source s;
        d_source_init(&s, buf, got, argv[a]);
        d_source_skip_bom(&s);
        struct d_lexer* lx = cpp ? d_lex_cpp_create(&s, D_LEX_CPP23, D_LEX_OPT_KEYWORDS)
                                 : d_lex_c_create(&s, D_LEX_C23, D_LEX_OPT_KEYWORDS);
        struct d_parse_diagnostic first[1];
        struct d_parse_diag_sink  sink;
        d_parse_diag_sink_init(&sink, first, 1u, NULL, 0u);
        d_lex_set_diagnostics(lx, &sink);
        struct d_token t;
        while (d_lex_next(lx, &t)) {
            ++tokens;
            if (t.flags & D_TOKEN_FLAG_SPLICED) ++spliced;
            if (t.flags & D_TOKEN_FLAG_DIGRAPH) ++digraph;
            if (t.flags & D_TOKEN_FLAG_USER_SUFFIX) ++suffix;
            if (t.kind == D_TOKEN_RAW_STRING) ++raw;
            if (d_token_is_keyword(t.kind)) ++kw;
            else if (d_token_is_punctuator(t.kind)) ++punct;
            else if (t.kind == D_TOKEN_IDENTIFIER) ++id;
            else if (t.kind == D_TOKEN_NUMBER) ++num;
            else if (t.kind == D_TOKEN_STRING) ++str;
            else if (t.kind == D_TOKEN_CHARACTER) ++ch;
            else if (t.kind == D_TOKEN_HEADER_NAME) ++hdr;
        }
        if (d_parse_diag_failed(&sink)) {
            ++bad_files;
            if (bad_files <= 6)
                printf("  %s: %s at %u:%u\n", argv[a],
                       d_lex_error_text(first[0].code),
                       first[0].span.line, first[0].span.column);
        }
        d_lex_destroy(lx);
        free(buf); ++files; bytes += got;
    }
    printf("\nfiles %zu   bytes %zu   tokens %zu   files with a defect %zu\n",
           files, bytes, tokens, bad_files);
    printf("keyword %zu  identifier %zu  punctuator %zu  number %zu\n",
           kw, id, punct, num);
    printf("string %zu  character %zu  header-name %zu\n", str, ch, hdr);
    printf("spliced %zu  alternative-spelling %zu  raw-string %zu  ud-suffix %zu\n",
           spliced, digraph, raw, suffix);
    return 0;
}
