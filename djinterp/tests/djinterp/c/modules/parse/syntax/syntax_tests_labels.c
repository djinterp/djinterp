/*******************************************************************************
* djinterp [c/parse]                                       syntax_tests_labels.c
*
* The syntax items' suite, block "labels": the levels of what a `case` or a
* `default` holds (F24). A braced block right after a label is the label's
* own body, on the label's level, its contents one in; a block further down
* the label's statements is one of them, a level in, as they are. dawk
* corrected the second until 11-11 (`d_internal_down_a_case`); parse gives
* both.
*
*
* path:      /tests/djinterp/c/modules/parse/syntax/syntax_tests_labels.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.10
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "./syntax_tests.h"  // corresponding header


/*
d_tests_syntax_case_blocks
  A switch whose labels hold blocks in each place a block can stand.
  Tests the following:
  - `case 1:` then `{`: the brace is on the label's level, its statements
    one in
  - a block after a case's first statement is a statement of the case: a
    level in, its contents two
  - a block after a braceless body is one too: the body is passed over, and
    the item before it is no label
  - `default:` then `{` is as `case 1:`
  - the labels and a case's plain statements keep their levels
*/
bool
d_tests_syntax_case_blocks(void)
{
    static const char* const TEXT =
        "int d_f(int _a)\n"
        "{\n"
        "    switch (_a)\n"
        "    {\n"
        "    case 1:\n"
        "    {\n"
        "        d_one();\n"
        "        break;\n"
        "    }\n"
        "    case 2:\n"
        "        d_two();\n"
        "        {\n"
        "            d_three();\n"
        "        }\n"
        "        break;\n"
        "    case 3:\n"
        "        if (_a)\n"
        "            d_five();\n"
        "        {\n"
        "            d_six();\n"
        "        }\n"
        "        break;\n"
        "    default:\n"
        "    {\n"
        "        d_four();\n"
        "    }\n"
        "    }\n"
        "    return 0;\n"
        "}\n";
    static const struct dj_tests_syntax_expect EXPECT[] =
    {
        { "case", 0u, D_SYNTAX_ITEM_LABEL, D_SYNTAX_STMT_CASE, NULL,
          D_TESTS_SYNTAX_ANY, 0u, 0u, 2u, NULL },
        { "{", 2u, D_SYNTAX_ITEM_STATEMENT, D_SYNTAX_STMT_COMPOUND, NULL,
          D_TESTS_SYNTAX_ANY, 0u, 0u, 2u, NULL },
        { "d_one", 0u, D_SYNTAX_ITEM_STATEMENT, D_SYNTAX_STMT_SIMPLE, NULL,
          D_TESTS_SYNTAX_ANY, 0u, 0u, 3u, NULL },
        { "d_two", 0u, D_SYNTAX_ITEM_STATEMENT, D_SYNTAX_STMT_SIMPLE, NULL,
          D_TESTS_SYNTAX_ANY, 0u, 0u, 3u, NULL },
        { "{", 3u, D_SYNTAX_ITEM_STATEMENT, D_SYNTAX_STMT_COMPOUND, NULL,
          D_TESTS_SYNTAX_ANY, 0u, 0u, 3u, NULL },
        { "d_three", 0u, D_SYNTAX_ITEM_STATEMENT, D_SYNTAX_STMT_SIMPLE, NULL,
          D_TESTS_SYNTAX_ANY, 0u, 0u, 4u, NULL },
        { "{", 4u, D_SYNTAX_ITEM_STATEMENT, D_SYNTAX_STMT_COMPOUND, NULL,
          D_TESTS_SYNTAX_ANY, 0u, 0u, 3u, NULL },
        { "d_six", 0u, D_SYNTAX_ITEM_STATEMENT, D_SYNTAX_STMT_SIMPLE, NULL,
          D_TESTS_SYNTAX_ANY, 0u, 0u, 4u, NULL },
        { "default", 0u, D_SYNTAX_ITEM_LABEL, D_SYNTAX_STMT_DEFAULT, NULL,
          D_TESTS_SYNTAX_ANY, 0u, 0u, 2u, NULL },
        { "{", 5u, D_SYNTAX_ITEM_STATEMENT, D_SYNTAX_STMT_COMPOUND, NULL,
          D_TESTS_SYNTAX_ANY, 0u, 0u, 2u, NULL },
        { "d_four", 0u, D_SYNTAX_ITEM_STATEMENT, D_SYNTAX_STMT_SIMPLE, NULL,
          D_TESTS_SYNTAX_ANY, 0u, 0u, 3u, NULL }
    };

    D_TEST_REQUIRE(d_tests_syntax_text(TEXT, false, NULL, EXPECT,
                                       sizeof(EXPECT) / sizeof(EXPECT[0])));

    return true;
}
