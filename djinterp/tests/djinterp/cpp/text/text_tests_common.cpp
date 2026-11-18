/*******************************************************************************
* djinterp [test]                                          text_tests_common.cpp
*
*   The check and the run that every suite of text's C++ headers shares:
* the suite checks are recorded against, as bound, a comparison of texts that
* notes both when they differ, and a test run or skipped below its header's
* floor.
*
*
* path:      /tests/djinterp/cpp/text/text_tests_common.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/
#include "text_tests_common.hpp"  // corresponding header
// std
#include <cstring>  // std::strcmp
#include <string>   // std::string


// g_suite
//   the suite that checks are recorded against; set by d_tests_text_bind.
static dj_test_suite* g_suite = NULL;


/*
d_tests_text_bind
  Binds the suite that checks are recorded against, or unbinds it.
*/
void
d_tests_text_bind(
    dj_test_suite* _suite
)
{
    g_suite = _suite;

    return;
}


/*
d_tests_text_check
  Records one check against the open test of the bound suite.
*/
void
d_tests_text_check(
    bool        _passed,
    const char* _expression,
    const char* _file,
    int         _line
)
{
    (void)d_test_suite_check(g_suite, _passed, _expression, _file, _line);

    return;
}


/*
d_tests_text_visible
  A text with its newlines and tabs written as \n and \t, for a note.
*/
static std::string
d_tests_text_visible(
    const char* _text
)
{
    std::string out;

    for (const char* c = _text; (c != NULL) && (*c != '\0'); ++c)
    {
        out += (*c == '\n') ? std::string("\\n")
             : (*c == '\t') ? std::string("\\t")
             :                std::string(1, *c);
    }

    return out;
}


/*
d_tests_text_same
  Compares two texts, noting both against the open test when they differ.
*/
bool
d_tests_text_same(
    const char* _got,
    const char* _expected
)
{
    const bool same = (_got != NULL) && (_expected != NULL) &&
                      (std::strcmp(_got, _expected) == 0);

    if (!same)
    {
        (void)d_test_suite_note(g_suite,
                                "got      [%s]",
                                d_tests_text_visible(_got).c_str());
        (void)d_test_suite_note(g_suite,
                                "expected [%s]",
                                d_tests_text_visible(_expected).c_str());
    }

    return same;
}


/*
d_tests_text_run
  Opens a test and describes it, then runs its group, or skips it with the
reason given when the group is NULL.
*/
void
d_tests_text_run(
    dj_test_suite* _suite,
    const char*    _name,
    const char*    _description,
    void           (*_group)(void),
    const char*    _skip_reason
)
{
    (void)d_test_suite_test(_suite, _name);
    (void)d_test_suite_describe(_suite, _description);

    if (_group != NULL)
    {
        _group();
    }
    else
    {
        (void)d_test_suite_skip(_suite, _skip_reason);
    }

    return;
}
