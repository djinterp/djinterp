/*******************************************************************************
* djinterp [c/parse]                                           syntax_tests_pp.c
*
* The syntax items' suite, block "pp": `pp_of`, for every token how many
* preprocessor conditionals are open around it, the include guard excluded
* as an item's `pp_total` excludes it. dawk's checker reads it beside
* `item_of` and `level_of` (lane H, 11-05).
*
*
* path:      /tests/djinterp/c/modules/parse/syntax/syntax_tests_pp.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "./syntax_tests.h"  // corresponding header


/*
d_tests_syntax_pp_guarded
  A header inside its include guard.
  Tests the following:
  - a token inside only the guard counts 0, and so does its item's
    `pp_total`, as documented (it counted the guard until 11-05)
  - each conditional inside it counts 1 more
  - a conditional's own directive lines stand outside it
*/
bool
d_tests_syntax_pp_guarded(void)
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
    static const struct dj_tests_syntax_pp_expect EXPECT[] =
    {
        { "a", 0u, 0u, 0u }, { "b", 0u, 1u, 1u }, { "c", 0u, 2u, 2u },
        { "d", 0u, 2u, 2u }, { "X", 0u, 0u, D_TESTS_SYNTAX_PP_ANY },
        { "Y", 0u, 1u, D_TESTS_SYNTAX_PP_ANY },
        { "else", 0u, 1u, D_TESTS_SYNTAX_PP_ANY }
    };

    D_TEST_REQUIRE(d_tests_syntax_pp_text(TEXT, EXPECT,
                                          sizeof(EXPECT) / sizeof(EXPECT[0])));

    return true;
}

/*
d_tests_syntax_pp_unguarded
  A source with no guard: every conditional counts.
  Tests the following:
  - a token outside every conditional counts 0, before and after one, though
    the file opens with the conditional
  - inside one, 1
*/
bool
d_tests_syntax_pp_unguarded(void)
{
    static const char* const TEXT =
        "#ifdef X\n"
        "int a;\n"
        "#endif\n"
        "int b;\n"
        "#ifdef Y\n"
        "int c;\n"
        "#endif\n";
    static const struct dj_tests_syntax_pp_expect EXPECT[] =
    {
        { "a", 0u, 1u, 1u }, { "b", 0u, 0u, 0u }, { "c", 0u, 1u, 1u }
    };

    D_TEST_REQUIRE(d_tests_syntax_pp_text(TEXT, EXPECT,
                                          sizeof(EXPECT) / sizeof(EXPECT[0])));

    return true;
}

/*
d_tests_syntax_pp_directives
  A header of directives alone, and a source whose conditional follows an
`#include` (lane H's std fixtures `c99_directives.h` and `cpp98_guarded.cpp`).
  Tests the following:
  - the guard is excluded though no token but a directive's is inside it:
    an `#include` inside only the guard counts 0
  - an `#if` around the rest of a source, after an `#include`, is no guard:
    what it holds counts 1
*/
bool
d_tests_syntax_pp_directives(void)
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
    static const struct dj_tests_syntax_pp_expect IN_HEADER[] =
    {
        { "include", 0u, 0u, D_TESTS_SYNTAX_PP_ANY },
        { "include", 1u, 1u, D_TESTS_SYNTAX_PP_ANY }
    };
    static const struct dj_tests_syntax_pp_expect IN_SOURCE[] =
    {
        { "include", 0u, 0u, D_TESTS_SYNTAX_PP_ANY }, { "a", 0u, 1u, 1u }
    };

    D_TEST_REQUIRE(d_tests_syntax_pp_text(HEADER, IN_HEADER,
                       sizeof(IN_HEADER) / sizeof(IN_HEADER[0])));
    D_TEST_REQUIRE(d_tests_syntax_pp_text(SOURCE, IN_SOURCE,
                       sizeof(IN_SOURCE) / sizeof(IN_SOURCE[0])));

    return true;
}

/*
d_tests_syntax_pp_not_guards
  Three texts that open with `#ifndef` but are no include guard.
  Tests the following:
  - an `#else` at the guard's level makes it a conditional
  - a `#define` of another name does
  - so does a token after the closing `#endif`
*/
bool
d_tests_syntax_pp_not_guards(void)
{
    static const char* const WITH_ELSE =
        "#ifndef T_H\n#define T_H 1\nint a;\n#else\nint b;\n#endif\n";
    static const char* const OTHER_NAME =
        "#ifndef T_H\n#define U_H 1\nint a;\n#endif\n";
    static const char* const TAIL =
        "#ifndef T_H\n#define T_H 1\nint a;\n#endif\nint b;\n";
    static const struct dj_tests_syntax_pp_expect BOTH[] =
    {
        { "a", 0u, 1u, 1u }, { "b", 0u, 1u, 1u }
    };
    static const struct dj_tests_syntax_pp_expect ONE[] =
    {
        { "a", 0u, 1u, 1u }
    };
    static const struct dj_tests_syntax_pp_expect AFTER[] =
    {
        { "a", 0u, 1u, 1u }, { "b", 0u, 0u, 0u }
    };

    D_TEST_REQUIRE(d_tests_syntax_pp_text(WITH_ELSE, BOTH, 2u));
    D_TEST_REQUIRE(d_tests_syntax_pp_text(OTHER_NAME, ONE, 1u));
    D_TEST_REQUIRE(d_tests_syntax_pp_text(TAIL, AFTER, 2u));

    return true;
}
