/*******************************************************************************
* djinterp [c/parse]                                               t_syntax_pp.c
*
* The syntax items' `pp_of`: for every token, how many preprocessor
* conditionals are open around it, the include guard excluded as an item's
* `pp_total` excludes it. dawk's checker, dcheck, reads it beside `item_of`
* and `level_of` (lane H, 11-05).
*   Each text is lexed as C, grouped, and recognized, and the count read at
* named tokens.
*
*
* path:      /tests/djinterp/c/modules/parse/syntax/t_syntax_pp.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.06
*******************************************************************************/
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <stdio.h>    // printf
#include <string.h>   // memset, strlen, strncmp
// djinterp
#include "../../../../../../inc/djinterp/c/modules/parse/lang/c/lex_c.h"          // d_lex_c_create
#include "../../../../../../inc/djinterp/c/modules/parse/lex/lex_scan.h"          // d_lex_tokenize
#include "../../../../../../inc/djinterp/c/modules/parse/source/source_reader.h"  // d_source_init
#include "../../../../../../inc/djinterp/c/modules/parse/syntax/syntax_group.h"   // d_syntax_group
#include "../../../../../../inc/djinterp/c/modules/parse/syntax/syntax_item.h"    // d_syntax_items_build
// re_std
#include "../../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t


// D_TESTS_PP_TOKENS
//   constant: the most tokens a text here lexes into.
#define D_TESTS_PP_TOKENS   256u

// D_TESTS_PP_ANY
//   constant: an expectation of an item's `pp_total` that is not checked.
#define D_TESTS_PP_ANY      0xFFu

// dj_tests_pp_expect
//   struct: a token, by its spelling and which of that spelling it is, the
// count it must carry, and the `pp_total` of the item it is in, or
// D_TESTS_PP_ANY.
struct dj_tests_pp_expect
{
    const char* spelling;
    uint32_t    nth;
    uint8_t     pp;
    uint8_t     total;
};

/*
d_tests_pp_find
  The index of the `_nth` token (from 0) spelled `_spelling`, or the count
where there is none.
*/
static size_t
d_tests_pp_find(
    const char*            _text,
    const struct dj_token* _tokens,
    size_t                 _count,
    const char*            _spelling,
    uint32_t               _nth
)
{
    const size_t length = strlen(_spelling);
    uint32_t     seen   = 0u;

    for (size_t i = 0u; i < _count; ++i)
    {
        if ( (_tokens[i].span.length == length) &&
             (strncmp(_text + _tokens[i].span.offset, _spelling,
                      length) == 0) )
        {
            if (seen == _nth)
            {
                return i;
            }

            ++seen;
        }
    }

    return _count;
}

/*
d_tests_pp_text
  Lexes, groups and recognizes a text, then holds each expected token to its
count.
*/
static bool
d_tests_pp_text(
    const char*                      _text,
    const struct dj_tests_pp_expect* _expect,
    size_t                           _expected
)
{
    static struct dj_token tokens[D_TESTS_PP_TOKENS];
    struct dj_source       source;
    struct dj_syntax_groups groups;
    struct dj_syntax_items  items;
    bool                    ok = true;

    memset(&groups, 0, sizeof(groups));
    memset(&items, 0, sizeof(items));
    d_source_init(&source, _text, strlen(_text), "t.h");

    struct dj_lexer* lexer = d_lex_c_create(&source, 0u, 0u);
    const size_t     count = (lexer)
                             ? d_lex_tokenize(lexer, tokens,
                                              D_TESTS_PP_TOKENS)
                             : 0u;

    d_lex_destroy(lexer);

    ok = ( (count > 0u)                                              &&
           (count < D_TESTS_PP_TOKENS)                               &&
           (d_syntax_group(tokens, count, &groups))                  &&
           (d_syntax_items_build(tokens, count, &groups, _text,
                                 &items))                            &&
           (items.pp_of) );

    for (size_t i = 0u; ( (ok) && (i < _expected) ); ++i)
    {
        const size_t at = d_tests_pp_find(_text, tokens, count,
                                          _expect[i].spelling,
                                          _expect[i].nth);

        const uint32_t item = (at < count) ? items.item_of[at]
                                           : D_SYNTAX_NONE;

        if ( (at == count)                                          ||
             (items.pp_of[at] != _expect[i].pp)                     ||
             ( (_expect[i].total != D_TESTS_PP_ANY) &&
               ( (item == D_SYNTAX_NONE) ||
                 (items.items[item].pp_total != _expect[i].total) ) ) )
        {
            printf("    `%s` (%lu): %d, not %d\n", _expect[i].spelling,
                   (unsigned long)_expect[i].nth,
                   (at == count) ? -1 : (int)items.pp_of[at],
                   (int)_expect[i].pp);
            ok = false;
        }
    }

    d_syntax_items_free(&items);
    d_syntax_groups_free(&groups);

    return ok;
}

/*
d_tests_pp_guarded
  A header inside its include guard.
  Tests the following:
  - a token inside only the guard counts 0, and so does its item's
    `pp_total`, as documented (it counted the guard until 11-05)
  - each conditional inside it counts 1 more
  - a conditional's own directive lines stand outside it
*/
static bool
d_tests_pp_guarded(void)
{
    static const char* const TEXT =
        "#ifndef T_H\n"
        "#define T_H 1\n"
        "int a;\n"
        "#if X\n"
        "int b;\n"
        "#if Y\n"
        "int c;\n"
        "#else\n"
        "int d;\n"
        "#endif\n"
        "#endif\n"
        "#endif\n";
    static const struct dj_tests_pp_expect EXPECT[] =
    {
        { "a", 0u, 0u, 0u }, { "b", 0u, 1u, 1u }, { "c", 0u, 2u, 2u },
        { "d", 0u, 2u, 2u }, { "X", 0u, 0u, D_TESTS_PP_ANY },
        { "Y", 0u, 1u, D_TESTS_PP_ANY }, { "else", 0u, 1u, D_TESTS_PP_ANY }
    };

    return d_tests_pp_text(TEXT, EXPECT, sizeof(EXPECT) / sizeof(EXPECT[0]));
}

/*
d_tests_pp_unguarded
  A source with no guard: every conditional counts.
  Tests the following:
  - a token outside every conditional counts 0, before and after one, though
    the file opens with the conditional
  - inside one, 1
*/
static bool
d_tests_pp_unguarded(void)
{
    static const char* const TEXT =
        "#ifdef X\n"
        "int a;\n"
        "#endif\n"
        "int b;\n"
        "#ifdef Y\n"
        "int c;\n"
        "#endif\n";
    static const struct dj_tests_pp_expect EXPECT[] =
    {
        { "a", 0u, 1u, 1u }, { "b", 0u, 0u, 0u }, { "c", 0u, 1u, 1u }
    };

    return d_tests_pp_text(TEXT, EXPECT, sizeof(EXPECT) / sizeof(EXPECT[0]));
}

/*
d_tests_pp_directives
  A header of directives alone, and a source whose conditional follows an
`#include` (lane H's std fixtures `c99_directives.h` and `cpp98_guarded.cpp`).
  Tests the following:
  - the guard is excluded though no token but a directive's is inside it:
    an `#include` inside only the guard counts 0
  - an `#if` around the rest of a source, after an `#include`, is no guard:
    what it holds counts 1
*/
static bool
d_tests_pp_directives(void)
{
    static const char* const HEADER =
        "/* c99 */\n"
        "#ifndef T_H\n"
        "#define T_H 1\n"
        "#include <uchar.h>\n"
        "#if X\n"
        "#include <stdalign.h>\n"
        "#endif\n"
        "#endif  // T_H\n";
    static const char* const SOURCE =
        "#include <vector>\n"
        "#if X\n"
        "int a;\n"
        "#endif\n";
    static const struct dj_tests_pp_expect IN_HEADER[] =
    {
        { "include", 0u, 0u, D_TESTS_PP_ANY },
        { "include", 1u, 1u, D_TESTS_PP_ANY }
    };
    static const struct dj_tests_pp_expect IN_SOURCE[] =
    {
        { "include", 0u, 0u, D_TESTS_PP_ANY }, { "a", 0u, 1u, 1u }
    };

    const bool header = d_tests_pp_text(HEADER, IN_HEADER,
                            sizeof(IN_HEADER) / sizeof(IN_HEADER[0]));
    const bool source = d_tests_pp_text(SOURCE, IN_SOURCE,
                            sizeof(IN_SOURCE) / sizeof(IN_SOURCE[0]));

    return ( (header) && (source) );
}

/*
d_tests_pp_not_guards
  Three texts that open with `#ifndef` but are no include guard.
  Tests the following:
  - an `#else` at the guard's level makes it a conditional
  - a `#define` of another name does
  - so does a token after the closing `#endif`
*/
static bool
d_tests_pp_not_guards(void)
{
    static const char* const WITH_ELSE =
        "#ifndef T_H\n#define T_H 1\nint a;\n#else\nint b;\n#endif\n";
    static const char* const OTHER_NAME =
        "#ifndef T_H\n#define U_H 1\nint a;\n#endif\n";
    static const char* const TAIL =
        "#ifndef T_H\n#define T_H 1\nint a;\n#endif\nint b;\n";
    static const struct dj_tests_pp_expect BOTH[] =
    {
        { "a", 0u, 1u, 1u }, { "b", 0u, 1u, 1u }
    };
    static const struct dj_tests_pp_expect ONE[] =
    {
        { "a", 0u, 1u, 1u }
    };
    static const struct dj_tests_pp_expect AFTER[] =
    {
        { "a", 0u, 1u, 1u }, { "b", 0u, 0u, 0u }
    };

    const bool with_else  = d_tests_pp_text(WITH_ELSE, BOTH, 2u);
    const bool other_name = d_tests_pp_text(OTHER_NAME, ONE, 1u);
    const bool tail       = d_tests_pp_text(TAIL, AFTER, 2u);

    return ( (with_else) && (other_name) && (tail) );
}

/*
main
  Runs the four sections.
*/
int
main(void)
{
    const bool guarded    = d_tests_pp_guarded();
    const bool unguarded  = d_tests_pp_unguarded();
    const bool directives = d_tests_pp_directives();
    const bool not_guards = d_tests_pp_not_guards();

    printf("  [%s] syntax_pp_guarded\n", (guarded) ? "PASS" : "FAIL");
    printf("  [%s] syntax_pp_unguarded\n", (unguarded) ? "PASS" : "FAIL");
    printf("  [%s] syntax_pp_directives\n", (directives) ? "PASS" : "FAIL");
    printf("  [%s] syntax_pp_not_guards\n", (not_guards) ? "PASS" : "FAIL");

    return ( (guarded) && (unguarded) && (directives) && (not_guards) )
           ? 0
           : 1;
}
