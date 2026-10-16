/*******************************************************************************
* djinterp [test]                                                     fuzz_lex.c
*
* Fuzz the scanner and the literal decoder: random token soup, no crash, no.
*
*
* path:      /tests/parse/fuzz_lex.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.22
*******************************************************************************/
// fuzz the scanner and the literal decoder: random token soup, no crash, no
// runaway, and every literal token decoded under a sink.  Pass 1 is ASCII and
// its counts are the baseline's; pass 2 adds UTF-8, ill-formed bytes, UCNs and
// the C++23 escapes, over every dialect level.
#include "../../inc/djinterp/parse/lang/cpp/lex_cpp.h"
#include "../../inc/djinterp/parse/lang/c/lex_c.h"
#include "../../inc/djinterp/parse/lex/lex_literal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned s=12345u;
static unsigned rnd(void){ s=s*1103515245u+12345u; return s>>16; }
static const unsigned levels[] = { D_LEX_C89, D_LEX_C99, D_LEX_C11, D_LEX_C17, D_LEX_C23,
    D_LEX_CPP98, D_LEX_CPP11, D_LEX_CPP14, D_LEX_CPP17, D_LEX_CPP20, D_LEX_CPP23 };
static size_t run(const char* buf, size_t n, int cpp, unsigned level, size_t* diags)
{
    struct d_source src; d_source_init(&src,buf,n,"<f>");
    struct d_lexer* lx = cpp
        ? d_lex_cpp_create(&src,level,D_LEX_OPT_KEYWORDS|D_LEX_OPT_EMIT_COMMENTS)
        : d_lex_c_create(&src,level,D_LEX_OPT_KEYWORDS|D_LEX_OPT_EMIT_COMMENTS);
    struct d_parse_diagnostic items[4]; char text[256];
    struct d_parse_diag_sink sink; d_parse_diag_sink_init(&sink,items,4,text,sizeof text);
    d_lex_set_diagnostics(lx,&sink);
    struct d_token t; size_t guard=0, total=0;
    while(d_lex_next(lx,&t)){
        ++total;
        if(++guard>4096){printf("RUNAWAY\n");exit(1);}
        if(t.span.offset+t.span.length>n || t.end_line<t.span.line){printf("BAD SPAN\n");exit(1);}
        struct d_lex_literal lit; (void)d_lex_literal_decode_token(lx,NULL,&t,&sink,&lit);
    }
    for (int k = 0; k < D_PARSE_SEVERITY_COUNT; ++k) *diags += sink.tally[k];
    d_lex_destroy(lx);
    return total;
}
int main(void){
    static char buf[4096]; size_t total=0, diags=0;
    const char* soup="{}()[]<>;:.,?!+-*/%^&|=~\"'\\\n\t #ULRu80123abcxyz_?";
    for (int i=0;i<200000;++i){
        size_t n=rnd()%512;
        for(size_t k=0;k<n;++k) buf[k]=soup[rnd()%49];
        total += run(buf,n,(i&1),(i&1)?D_LEX_CPP23:D_LEX_C23,&diags);
    }
    printf("200000 inputs, %zu tokens, no crash, no runaway\n", total);
    static const char* const pieces[] = { "\xC3\xA9", "\xE2\x82\xAC", "\xF0\x9F\x98\x80",
        "\xCC\x81", "\xFF", "\x80", "\xED\xA0\x80", "\\u00e9", "\\U0001F600", "\\u{e9}",
        "\\N{A}", "\\x{41}", "\\o{7}", "\\x", "u8'", "'", "\"", "R\"(", ")\"", "0x1p",
        "1'0", "e+", "wb", "ull", "_km", "\\\n", "?\?/", "\n", " ", "a", "9", "." };
    const size_t kinds = sizeof pieces / sizeof pieces[0];
    size_t total2=0;
    for (int i=0;i<100000;++i){
        size_t n=0, want=rnd()%160;
        while (n < want) { const char* p = pieces[rnd()%kinds]; size_t l = strlen(p);
                           if (n + l >= sizeof buf) { break; }
                           memcpy(buf+n,p,l); n += l; }
        const unsigned which = rnd()%11;
        total2 += run(buf,n,which>=5,levels[which],&diags);
    }
    printf("100000 extended inputs, %zu tokens, %zu diagnostics, no crash, no runaway\n",
           total2, diags);
    return 0;
}
