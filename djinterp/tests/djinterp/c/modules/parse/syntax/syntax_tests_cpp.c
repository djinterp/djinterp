/*******************************************************************************
* djinterp [c/parse]                                          syntax_tests_cpp.c
*
* The syntax items' suite, block "cpp": the items of C++ heads, template
* arguments, template heads, class heads with base lists, constructors'
* initializer lists, and the namespace macros a caller names (lane H, 11-05
* (2); lane F's F22 and F23), and C's, which they leave as they were.
*   Each text is lexed as C++ with keywords resolved, grouped and recognized,
* and the item beginning at a named token checked: its kind, statement, name,
* parameter count, flags and level.
*
*
* path:      /tests/djinterp/c/modules/parse/syntax/syntax_tests_cpp.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.06
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "./syntax_tests.h"  // corresponding header


/*
d_tests_syntax_cpp_template_args
  Commas, `=` and parentheses among template arguments.
  Tests the following:
  - a parameter whose type has a comma among its arguments is one
    parameter (`contracts.hpp:41`)
  - a `>>` closes two lists, and a parameter after it is its own
  - a function named after a type with arguments is still the function,
    and `d_result<void>` is no `void` return
*/
bool
d_tests_syntax_cpp_template_args(void)
{
    static const char* const TEXT =
        "void take(const std::pair<int, Type>& _first, int _second);\n"
        "void deep(d_a<d_b<int, char>> _x, int _y);\n"
        "d_result<void>\n"
        "run(int _a)\n"
        "{\n"
        "    return d_ok();\n"
        "}\n";
    static const struct dj_tests_syntax_expect EXPECT[] =
    {
        { "void", 0u, D_SYNTAX_ITEM_FN_DECL, 0u, "take", 2,
          D_SYNTAX_ITEM_FLAG_RETURNS_VOID, 0u, 0u, NULL },
        { "int", 1u, D_SYNTAX_ITEM_PARAM, 0u, "_second", D_TESTS_SYNTAX_ANY,
          0u, 0u, 1u, NULL },
        { "void", 1u, D_SYNTAX_ITEM_FN_DECL, 0u, "deep", 2, 0u, 0u, 0u, NULL },
        { "d_result", 0u, D_SYNTAX_ITEM_FN_DEF, 0u, "run", 1, 0u,
          D_SYNTAX_ITEM_FLAG_RETURNS_VOID, 0u, NULL }
    };

    D_TEST_REQUIRE(d_tests_syntax_text(TEXT, true, NULL, EXPECT,
                                       sizeof(EXPECT) / sizeof(EXPECT[0])));

    return true;
}

/*
d_tests_syntax_cpp_template_heads
  A template head with defaults, before a struct and before a function.
  Tests the following:
  - a default type in the head is no initializer: the struct has its member
    body, and its member is a definition (`templates.hpp:21`)
  - so with a default that holds a `> >` (`templates.hpp:37`)
  - a function definition begins at its `template` (`templates.hpp:69`)
*/
bool
d_tests_syntax_cpp_template_heads(void)
{
    static const char* const TEXT =
        "template<typename Type = double>\n"
        "struct d_defaulted\n"
        "{\n"
        "    Type sum(int _count) const\n"
        "    {\n"
        "        return Type();\n"
        "    }\n"
        "};\n"
        "template<typename First, typename Second = d_p<d_p<int> > >\n"
        "struct d_nested\n"
        "{\n"
        "    int count(int _limit) const\n"
        "    {\n"
        "        return _limit;\n"
        "    }\n"
        "};\n"
        "template<typename Type = int>\n"
        "D_CONSTEXPR Type\n"
        "d_free_sum(int _count)\n"
        "{\n"
        "    return Type(_count);\n"
        "}\n";
    static const struct dj_tests_syntax_expect EXPECT[] =
    {
        { "template", 0u, D_SYNTAX_ITEM_DECLARATION, 0u, NULL,
          D_TESTS_SYNTAX_ANY, 0u, 0u, 0u, NULL },
        { "Type", 1u, D_SYNTAX_ITEM_FN_DEF, 0u, "sum", 1, 0u, 0u, 1u, NULL },
        { "template", 1u, D_SYNTAX_ITEM_DECLARATION, 0u, NULL,
          D_TESTS_SYNTAX_ANY, 0u, 0u, 0u, NULL },
        { "int", 2u, D_SYNTAX_ITEM_FN_DEF, 0u, "count", 1, 0u, 0u, 1u, NULL },
        { "template", 2u, D_SYNTAX_ITEM_FN_DEF, 0u, "d_free_sum", 1,
          D_SYNTAX_ITEM_FLAG_ENDS_RETURN, 0u, 0u, NULL }
    };

    D_TEST_REQUIRE(d_tests_syntax_text(TEXT, true, NULL, EXPECT,
                                       sizeof(EXPECT) / sizeof(EXPECT[0])));

    return true;
}

/*
d_tests_syntax_cpp_class_heads
  Class heads with parentheses among their arguments and with base lists.
  Tests the following:
  - parentheses among a base's template arguments are no parameter list:
    the struct is a type, its member a definition (`classes.hpp:9`)
  - nor are those of a function type among a specialization's arguments
    (`classes.hpp:25`)
  - a macro's parentheses in a base list are none either
  - a class with a public base is a class (`initializers.hpp:10`)
  - a function returning a tag's type with arguments is still a function
*/
bool
d_tests_syntax_cpp_class_heads(void)
{
    static const char* const TEXT =
        "struct d_mixed\n"
        "    : d_under<d_has(Feat, 1)>\n"
        "    , d_over<d_has(Feat, 2)>\n"
        "{\n"
        "    int sum(int _a, int _b) { return _a + _b; }\n"
        "};\n"
        "struct d_traits<Return(Args...)>\n"
        "{\n"
        "    Return apply(Return _a, int _b) { return _a; }\n"
        "};\n"
        "struct d_macro_base : D_BASE(d_x, 2)\n"
        "{\n"
        "    int get(int _a) { return _a; }\n"
        "};\n"
        "class d_braced : public d_base<Type>\n"
        "{\n"
        "    int m_count;\n"
        "};\n"
        "struct d_node<int>*\n"
        "d_make(int _a, int _b)\n"
        "{\n"
        "    return 0;\n"
        "}\n";
    static const struct dj_tests_syntax_expect EXPECT[] =
    {
        { "struct", 0u, D_SYNTAX_ITEM_DECLARATION, 0u, NULL,
          D_TESTS_SYNTAX_ANY, 0u, 0u, 0u, NULL },
        { "int", 0u, D_SYNTAX_ITEM_FN_DEF, 0u, "sum", 2, 0u, 0u, 1u, NULL },
        { "struct", 1u, D_SYNTAX_ITEM_DECLARATION, 0u, NULL,
          D_TESTS_SYNTAX_ANY, 0u, 0u, 0u, NULL },
        { "Return", 1u, D_SYNTAX_ITEM_FN_DEF, 0u, "apply", 2, 0u, 0u, 1u,
          NULL },
        { "struct", 2u, D_SYNTAX_ITEM_DECLARATION, 0u, NULL,
          D_TESTS_SYNTAX_ANY, 0u, 0u, 0u, NULL },
        { "int", 4u, D_SYNTAX_ITEM_FN_DEF, 0u, "get", 1, 0u, 0u, 1u, NULL },
        { "class", 0u, D_SYNTAX_ITEM_DECLARATION, 0u, NULL,
          D_TESTS_SYNTAX_ANY, 0u, 0u, 0u, NULL },
        { "int", 6u, D_SYNTAX_ITEM_DECLARATION, 0u, "m_count",
          D_TESTS_SYNTAX_ANY, 0u, 0u, 1u, NULL },
        { "struct", 3u, D_SYNTAX_ITEM_FN_DEF, 0u, "d_make", 2, 0u, 0u, 0u,
          NULL }
    };

    D_TEST_REQUIRE(d_tests_syntax_text(TEXT, true, NULL, EXPECT,
                                       sizeof(EXPECT) / sizeof(EXPECT[0])));

    return true;
}

/*
d_tests_syntax_cpp_initializers
  Constructors' initializer lists (`initializers.hpp:13`, `:24`).
  Tests the following:
  - the body is the brace after the last initializer, not the first
    initializer's brace: the constructor reaches its last initializer
  - an initializer list of one braced initializer before an empty body
  - an out-of-class constructor at file scope
*/
bool
d_tests_syntax_cpp_initializers(void)
{
    static const char* const TEXT =
        "class d_braced : public d_base<Type>\n"
        "{\n"
        "public:\n"
        "    d_braced(\n"
        "        const Type& _first\n"
        "    )\n"
        "        : d_base<Type>{_first}\n"
        "        , m_values{}\n"
        "        , m_count(0)\n"
        "    {\n"
        "        m_count = 1;\n"
        "    }\n"
        "\n"
        "    d_braced()\n"
        "        : m_values{}\n"
        "    {}\n"
        "};\n"
        "d_x::d_x(int _a)\n"
        "    : m_a(_a)\n"
        "{\n"
        "}\n";
    static const struct dj_tests_syntax_expect EXPECT[] =
    {
        { "d_braced", 1u, D_SYNTAX_ITEM_FN_DEF, 0u, "d_braced", 1, 0u,
          D_SYNTAX_ITEM_FLAG_EMPTY_BODY, 1u, "m_count" },
        { "m_count", 1u, D_SYNTAX_ITEM_STATEMENT, D_SYNTAX_STMT_SIMPLE, NULL,
          D_TESTS_SYNTAX_ANY, 0u, 0u, 2u, NULL },
        { "d_braced", 2u, D_SYNTAX_ITEM_FN_DEF, 0u, "d_braced", 0,
          D_SYNTAX_ITEM_FLAG_EMPTY_BODY, 0u, 1u, NULL },
        { "d_x", 0u, D_SYNTAX_ITEM_FN_DEF, 0u, "d_x", 1,
          D_SYNTAX_ITEM_FLAG_EMPTY_BODY, 0u, 0u, "m_a" }
    };

    D_TEST_REQUIRE(d_tests_syntax_text(TEXT, true, NULL, EXPECT,
                                       sizeof(EXPECT) / sizeof(EXPECT[0])));

    return true;
}

/*
d_tests_syntax_cpp_head_macros
  A macro's call between a template head and what it heads, as a C++20
`requires` is spelled for every level (lane H, 11-10: `net/client.hpp:328`).
  Tests the following:
  - before a class key, the call is the head's: the class is a type with its
    member body, not a function whose body is the class
  - before a return type, a name and its parameters, the call is the head's
    too: the function is named, with its own parameters
  - so before an operator
  - a templated constructor keeps its parameters though a macro's call
    follows them directly, `D_NOEXCEPT_IF(true)`
*/
bool
d_tests_syntax_cpp_head_macros(void)
{
    static const char* const TEXT =
        "template<typename Connector>\n"
        "D_INTERNAL_NET_REQUIRES_CONNECTOR(Connector)\n"
        "class client\n"
        "{\n"
        "public:\n"
        "    int count(int _limit) const\n"
        "    {\n"
        "        return _limit;\n"
        "    }\n"
        "};\n"
        "template<typename Connector>\n"
        "D_INTERNAL_NET_REQUIRES_CONNECTOR(Connector)\n"
        "io_error::value\n"
        "d_close(client<Connector>& _client, int _how)\n"
        "{\n"
        "    return 0;\n"
        "}\n"
        "template<typename Type>\n"
        "D_REQUIRES(Type)\n"
        "bool operator==(const Type& _a, const Type& _b)\n"
        "{\n"
        "    return true;\n"
        "}\n"
        "struct d_holder\n"
        "{\n"
        "    template<typename Other>\n"
        "    d_holder(const Other& _other) D_NOEXCEPT_IF(true)\n"
        "    {\n"
        "    }\n"
        "};\n";
    static const struct dj_tests_syntax_expect EXPECT[] =
    {
        { "template", 0u, D_SYNTAX_ITEM_DECLARATION, 0u, NULL,
          D_TESTS_SYNTAX_ANY, 0u, D_SYNTAX_ITEM_FLAG_FN_TYPE, 0u, NULL },
        { "public", 0u, D_SYNTAX_ITEM_LABEL, D_SYNTAX_STMT_ACCESS, NULL,
          D_TESTS_SYNTAX_ANY, 0u, 0u, 1u, NULL },
        { "int", 0u, D_SYNTAX_ITEM_FN_DEF, 0u, "count", 1, 0u, 0u, 1u, NULL },
        { "template", 1u, D_SYNTAX_ITEM_FN_DEF, 0u, "d_close", 2, 0u, 0u, 0u,
          NULL },
        { "template", 2u, D_SYNTAX_ITEM_FN_DEF, 0u, "operator", 2, 0u, 0u,
          0u, NULL },
        { "template", 3u, D_SYNTAX_ITEM_FN_DEF, 0u, "d_holder", 1,
          D_SYNTAX_ITEM_FLAG_EMPTY_BODY, 0u, 1u, NULL }
    };

    D_TEST_REQUIRE(d_tests_syntax_text(TEXT, true, NULL, EXPECT,
                                       sizeof(EXPECT) / sizeof(EXPECT[0])));

    return true;
}

/*
d_tests_syntax_cpp_namespaces
  The namespace macros a caller names (`layout.hpp:5`, `namespaces.hpp:33`,
`source.cpp:6`).
  Tests the following:
  - each named macro is a block item of its own, `open` or `close`, and
    swallows nothing: the function after `NS_DJINTERP` begins at its type
  - `D_NAMESPACE(x)` takes its argument group as its `params`
  - with no options, the old reading stands: the function begins at the
    macro, as a C caller that names none still sees
*/
bool
d_tests_syntax_cpp_namespaces(void)
{
    static const char* const OPEN[]  = { "NS_DJINTERP", "D_NAMESPACE",
                                         NULL };
    static const char* const CLOSE[] = { "NS_END", NULL };
    static const struct dj_syntax_options OPTIONS = { OPEN, CLOSE };
    static const char* const TEXT =
        "NS_DJINTERP\n"
        "int d_f(int _a)\n"
        "{\n"
        "    return _a;\n"
        "}\n"
        "NS_END  // djinterp\n"
        "D_NAMESPACE(d_macro)\n"
        "\n"
        "    struct d_first\n"
        "    {};\n"
        "\n"
        "NS_END  // d_macro\n";
    static const struct dj_tests_syntax_expect NAMED[] =
    {
        { "NS_DJINTERP", 0u, D_SYNTAX_ITEM_BLOCK, D_SYNTAX_STMT_OPEN, NULL,
          D_TESTS_SYNTAX_ANY, D_SYNTAX_ITEM_FLAG_MACRO_HEAD, 0u, 0u, NULL },
        { "int", 0u, D_SYNTAX_ITEM_FN_DEF, 0u, "d_f", 1, 0u, 0u, 0u, NULL },
        { "NS_END", 0u, D_SYNTAX_ITEM_BLOCK, D_SYNTAX_STMT_CLOSE, NULL,
          D_TESTS_SYNTAX_ANY, D_SYNTAX_ITEM_FLAG_MACRO_HEAD, 0u, 0u, NULL },
        { "D_NAMESPACE", 0u, D_SYNTAX_ITEM_BLOCK, D_SYNTAX_STMT_OPEN, NULL,
          0, D_SYNTAX_ITEM_FLAG_MACRO_HEAD, 0u, 0u, NULL },
        { "struct", 0u, D_SYNTAX_ITEM_DECLARATION, 0u, NULL,
          D_TESTS_SYNTAX_ANY, 0u, 0u, 0u, NULL },
        { "NS_END", 1u, D_SYNTAX_ITEM_BLOCK, D_SYNTAX_STMT_CLOSE, NULL,
          D_TESTS_SYNTAX_ANY, D_SYNTAX_ITEM_FLAG_MACRO_HEAD, 0u, 0u, NULL }
    };
    static const struct dj_tests_syntax_expect UNNAMED[] =
    {
        { "NS_DJINTERP", 0u, D_SYNTAX_ITEM_FN_DEF, 0u, "d_f", 1, 0u, 0u, 0u,
          NULL }
    };

    D_TEST_REQUIRE(d_tests_syntax_text(TEXT, true, &OPTIONS, NAMED,
                                       sizeof(NAMED) / sizeof(NAMED[0])));
    D_TEST_REQUIRE(d_tests_syntax_text(TEXT, true, NULL, UNNAMED, 1u));

    return true;
}

/*
d_tests_syntax_cpp_c_unchanged
  C, where a `<` after a name compares.
  Tests the following:
  - an initializer's comparisons leave the declarator alone: the first of
    two declarators is the name
  - a C function's parameters still split at their commas
*/
bool
d_tests_syntax_cpp_c_unchanged(void)
{
    static const char* const TEXT =
        "static int d_x = D_A < D_B, d_y = D_C > D_D;\n"
        "int d_g(int _a, int _b)\n"
        "{\n"
        "    return _a < _b;\n"
        "}\n";
    static const struct dj_tests_syntax_expect EXPECT[] =
    {
        { "static", 0u, D_SYNTAX_ITEM_DECLARATION, 0u, "d_x",
          D_TESTS_SYNTAX_ANY, 0u, 0u, 0u, NULL },
        { "int", 1u, D_SYNTAX_ITEM_FN_DEF, 0u, "d_g", 2, 0u, 0u, 0u, NULL }
    };

    D_TEST_REQUIRE(d_tests_syntax_text(TEXT, false, NULL, EXPECT,
                                       sizeof(EXPECT) / sizeof(EXPECT[0])));

    return true;
}
