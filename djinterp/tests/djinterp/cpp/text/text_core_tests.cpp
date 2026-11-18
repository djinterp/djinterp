/*******************************************************************************
* djinterp [test]                                            text_core_tests.cpp
*
*   The module "text_core": its entry point, which opens a block per header
* and a test per group, and runs each group or skips it below its header's
* floor.
*
*
* path:      /tests/djinterp/cpp/text/text_core_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/
#include "text_core_tests.hpp"  // the suite's entry point, DTest's C face


/*
d_tests_text_core_template
  The block for text_template.hpp, C++17 and up.
*/
static void
d_tests_text_core_template(
    dj_test_suite* _suite
)
{
    static const char* const below = "below C++17, text_template.hpp's floor";

    (void)d_test_suite_block(_suite, "text_template.hpp");

    d_tests_text_run(_suite, "parser",
        "the placeholder parser's segments, keys and totals",
        D_TESTS_TEXT_AT_CPP17(d_tests_text_core_template_parser), below);
    d_tests_text_run(_suite, "dialect",
        "{ key } is trimmed, {{ and }} are escapes, a lone brace is literal, "
        "and the first } closes",
        D_TESTS_TEXT_AT_CPP17(d_tests_text_core_template_dialect), below);
    d_tests_text_run(_suite, "render",
        "inline bindings, the functor face and render_to's append; a missing "
        "key renders empty",
        D_TESTS_TEXT_AT_CPP17(d_tests_text_core_template_render), below);
    d_tests_text_run(_suite, "lookups",
        "any callable is a source, called once per key in order, and a "
        "temporary it returns lives through the append",
        D_TESTS_TEXT_AT_CPP17(d_tests_text_core_template_lookups), below);
    d_tests_text_run(_suite, "copies",
        "a copy or a move renders like its original after the original is "
        "gone; wide characters",
        D_TESTS_TEXT_AT_CPP17(d_tests_text_core_template_copies), below);
    d_tests_text_run(_suite, "pattern",
        "a {key} literal renders through DTextTemplate and parses back "
        "through make_curly_pattern",
        D_TESTS_TEXT_AT_CPP17(d_tests_text_core_template_pattern), below);

    return;
}


/*
d_tests_text_core_pattern
  The block for text_pattern.hpp, C++11 and up.
*/
static void
d_tests_text_core_pattern(
    dj_test_suite* _suite
)
{
    static const char* const below = "below C++11, text_pattern.hpp's floor";

    (void)d_test_suite_block(_suite, "text_pattern.hpp");

    d_tests_text_run(_suite, "compile",
        "segments, keys and markers; adjacent captures and empty markers are "
        "malformed; set_markers recompiles",
        D_TESTS_TEXT_AT_CPP11(d_tests_text_core_pattern_compile), below);
    d_tests_text_run(_suite, "escape",
        "an escaped prefix is literal; set_escape changes the escape, and "
        "'\\0' turns it off; the four-argument constructor",
        D_TESTS_TEXT_AT_CPP11(d_tests_text_core_pattern_escape), below);
    d_tests_text_run(_suite, "match",
        "the predicate and extractor faces: the whole input, lazy captures, "
        "and a failed match's result",
        D_TESTS_TEXT_AT_CPP11(d_tests_text_core_pattern_match), below);
    d_tests_text_run(_suite, "find",
        "the first occurrence, with its position and captures; none",
        D_TESTS_TEXT_AT_CPP11(d_tests_text_core_pattern_find), below);
    d_tests_text_run(_suite, "render",
        "the renderer and rewrite faces: bound and unbound captures, escapes, "
        "and input that doesn't match",
        D_TESTS_TEXT_AT_CPP11(d_tests_text_core_pattern_render), below);
    d_tests_text_run(_suite, "compose",
        "concat keeps the left's markers and escape; the four factories' "
        "markers",
        D_TESTS_TEXT_AT_CPP11(d_tests_text_core_pattern_compose), below);

    return;
}


/*
d_tests_text_core_all
  Binds the suite, opens the module, then each header's block and its
tests.
*/
void
d_tests_text_core_all(
    dj_test_suite* _suite
)
{
    d_tests_text_bind(_suite);

    (void)d_test_suite_module(_suite, "text_core");
    (void)d_test_suite_describe(_suite,
                                "text's core C++ headers: alignment, "
                                "templates and patterns");

    (void)d_test_suite_block(_suite, "text_align.hpp");
    d_tests_text_run(_suite, "alignment",
        "DTextAlignment's four values, in order, in one byte",
        D_TESTS_TEXT_AT_CPP11(d_tests_text_core_align),
        "below C++11, text_align.hpp's floor");

    d_tests_text_core_template(_suite);
    d_tests_text_core_pattern(_suite);

    d_tests_text_bind(NULL);

    return;
}
