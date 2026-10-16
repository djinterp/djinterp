/*******************************************************************************
* djinterp [test]                                                 dump_lex_cpp.c
*
* Exercises `lex_cpp.h`.
*
*
* path:      /tests/parse/dump_lex_cpp.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.22
*******************************************************************************/
#include "../../inc/djinterp/parse/lang/cpp/lex_cpp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int c, char** v){
    FILE* f=fopen(v[1],"rb"); fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
    char* b=malloc((size_t)n+1); size_t g=fread(b,1,(size_t)n,f); fclose(f); b[g]=0;
    struct d_source s; d_source_init(&s,b,g,v[1]);
    struct d_lexer* lx=d_lex_cpp_create(&s,D_LEX_CPP23,D_LEX_OPT_KEYWORDS);
    struct d_token t; char sp[256];
    while(d_lex_next(lx,&t)){
        int want = (t.kind==D_TOKEN_RAW_STRING)||(t.flags&D_TOKEN_FLAG_USER_SUFFIX)
                 ||(t.flags&D_TOKEN_FLAG_DIGRAPH)||(t.flags&D_TOKEN_FLAG_SPLICED)
                 ||(t.kind==D_TOKEN_HEADER_NAME)||(t.kind==D_TOKEN_COMPARE);
        if(want && c>2){ d_token_spell(lx,&t,sp,sizeof sp);
            printf("  %-22s L%-3u %s\n", d_token_kind_name(t.kind), t.span.line, sp); }
    }
    d_lex_destroy(lx); free(b); return 0;
}
