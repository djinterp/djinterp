/*******************************************************************************
* djinterp [c/parse]                                              t_syntax_cpp.c
*
* The syntax items of C++ heads: template arguments, template heads, class
* heads with base lists, constructors' initializer lists, and the namespace
* macros a caller names (lane H, 11-05 (2); lane F's F22 and F23). dawk's
* fixtures are the acceptance tests; these are parse's own.
*   Each text is lexed as C++ with keywords resolved, grouped, and
* recognized, and the item beginning at a named token checked: its kind,
* statement, name, parameter count, flags and level.
*
*
* path:      /tests/djinterp/c/modules/parse/syntax/t_syntax_cpp.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.06
*                                                            revised: 2026.11.06
*******************************************************************************/
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <stdio.h>    // printf
#include <string.h>   // memset, strlen, strncmp
// djinterp
#include "../../../../../../inc/djinterp/c/modules/parse/lang/c/lex_c.h"          // d_lex_c_create
#include "../../../../../../inc/djinterp/c/modules/parse/lang/cpp/lex_cpp.h"      // d_lex_cpp_create
#include "../../../../../../inc/djinterp/c/modules/parse/lex/lex_scan.h"          // d_lex_tokenize
#include "../../../../../../inc/djinterp/c/modules/parse/source/source_reader.h"  // d_source_init
#include "../../../../../../inc/djinterp/c/modules/parse/syntax/syntax_group.h"   // d_syntax_group
#include "../../../../../../inc/djinterp/c/modules/parse/syntax/syntax_item.h"    // d_syntax_items_build_with
// re_std
#include "../../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t, uint16_t


// D_TESTS_CPP_TOKENS
//   constant: the most tokens a text here lexes into.
#define D_TESTS_CPP_TOKENS  512u

// D_TESTS_CPP_ANY
//   constant: a parameter count that is not checked.
#define D_TESTS_CPP_ANY     (-1)

// D_TESTS_CPP_LEX
//   constant: the lexer's options: comments kept and keywords resolved, as
// dawk lexes.
#define D_TESTS_CPP_LEX     (D_LEX_OPT_EMIT_COMMENTS | D_LEX_OPT_KEYWORDS)

// dj_tests_cpp_expect
//   struct: the item that begins at the `nth` token (from 0) spelled
// `first`, and what it must be: its kind and statement, the spelling of its
// name or NULL for none, how many parameters its group holds or
// D_TESTS_CPP_ANY, the flags it must have and must not, its level, and the
// spelling of a token it must reach to (its first) or NULL.
struct dj_tests_cpp_expect
{
    const char* first;
    uint32_t    nth;
    uint8_t     kind;
    uint8_t     statement;
    const char* name;
    int         params;
    uint16_t    set;
    uint16_t    clear;
    uint8_t     level;
    const char* reaches;
};

// dj_tests_cpp_run
//   struct: one text's recognition.
struct dj_tests_cpp_run
{
    const char*             text;
    struct dj_token         tokens[D_TESTS_CPP_TOKENS];
    size_t                  count;
    struct dj_syntax_groups groups;
    struct dj_syntax_items  items;
};

/*
d_tests_cpp_find
  The index of the `_nth` token (from 0) spelled `_spelling`, or the count
where there is none.
*/
static size_t
d_tests_cpp_find(
    const struct dj_tests_cpp_run* _run,
    const char*                    _spelling,
    uint32_t                       _nth
)
{
    const size_t length = strlen(_spelling);
    uint32_t     seen   = 0u;

    for (size_t i = 0u; i < _run->count; ++i)
    {
        if ( (_run->tokens[i].span.length == length) &&
             (strncmp(_run->text + _run->tokens[i].span.offset, _spelling,
                      length) == 0) )
        {
            if (seen == _nth)
            {
                return i;
            }

            ++seen;
        }
    }

    return _run->count;
}

/*
d_tests_cpp_spelled
  Whether a token, or D_SYNTAX_NONE, is spelled as expected, NULL meaning
none.
*/
static bool
d_tests_cpp_spelled(
    const struct dj_tests_cpp_run* _run,
    uint32_t                       _at,
    const char*                    _spelling
)
{
    if ( (_at == D_SYNTAX_NONE) || (!_spelling) )
    {
        return ( (_at == D_SYNTAX_NONE) && (!_spelling) );
    }

    return ( (_run->tokens[_at].span.length == strlen(_spelling)) &&
             (strncmp(_run->text + _run->tokens[_at].span.offset,
                      _spelling, strlen(_spelling)) == 0) );
}

/*
d_tests_cpp_params
  How many parameters an item's group holds: the items whose scope is it.
*/
static int
d_tests_cpp_params(
    const struct dj_tests_cpp_run* _run,
    const struct dj_syntax_item*   _item
)
{
    int count = 0;

    for (size_t k = 0u;
         ( (_item->params != D_SYNTAX_NONE) && (k < _run->items.count) );
         ++k)
    {
        if ( (_run->items.items[k].scope == _item->params) &&
             (_run->items.items[k].kind == D_SYNTAX_ITEM_PARAM) )
        {
            ++count;
        }
    }

    return count;
}

/*
d_tests_cpp_check
  Holds the item beginning at an expectation's token to it, and says what
differs.
*/
static bool
d_tests_cpp_check(
    const struct dj_tests_cpp_run*    _run,
    const struct dj_tests_cpp_expect* _expect
)
{
    const size_t at = d_tests_cpp_find(_run, _expect->first, _expect->nth);
    const struct dj_syntax_item* item = NULL;

    for (size_t k = 0u; ( (at < _run->count) && (k < _run->items.count) ); ++k)
    {
        if (_run->items.items[k].first == (uint32_t)at)
        {
            item = &_run->items.items[k];
            break;
        }
    }

    if ( (item)                                                  &&
         (item->kind == _expect->kind)                           &&
         (item->statement == _expect->statement)                 &&
         (d_tests_cpp_spelled(_run, item->name, _expect->name))  &&
         ( (_expect->params == D_TESTS_CPP_ANY) ||
           (d_tests_cpp_params(_run, item) == _expect->params) ) &&
         ((item->flags & _expect->set) == _expect->set)          &&
         ((item->flags & _expect->clear) == 0u)                  &&
         (item->level == _expect->level)                         &&
         ( (!_expect->reaches) ||
           (item->last >= (uint32_t)d_tests_cpp_find(_run, _expect->reaches,
                                                     0u)) ) )
    {
        return true;
    }

    printf("    `%s` (%lu): ", _expect->first, (unsigned long)_expect->nth);

    if (!item)
    {
        printf("no item begins there\n");
    }
    else
    {
        printf("%s %s, %d parameter(s), flags 0x%04x, level %u\n",
               d_syntax_item_kind_name(item->kind),
               d_syntax_statement_name(item->statement),
               d_tests_cpp_params(_run, item), (unsigned)item->flags,
               (unsigned)item->level);
    }

    return false;
}

/*
d_tests_cpp_text
  Lexes a text as C++ (or C), groups and recognizes it with the options
given, then holds each expectation to it.
*/
static bool
d_tests_cpp_text(
    const char*                       _text,
    bool                              _cpp,
    const struct dj_syntax_options*   _options,
    const struct dj_tests_cpp_expect* _expect,
    size_t                            _expected
)
{
    static struct dj_tests_cpp_run run;
    struct dj_source               source;
    bool                           ok = true;

    memset(&run, 0, sizeof(run));
    run.text = _text;
    d_source_init(&source, _text, strlen(_text), (_cpp) ? "t.hpp" : "t.h");

    struct dj_lexer* lexer = (_cpp)
                           ? d_lex_cpp_create(&source, 0u, D_TESTS_CPP_LEX)
                           : d_lex_c_create(&source, 0u, D_TESTS_CPP_LEX);

    run.count = (lexer) ? d_lex_tokenize(lexer, run.tokens,
                                         D_TESTS_CPP_TOKENS)
                        : 0u;
    d_lex_destroy(lexer);

    ok = ( (run.count > 0u)                                        &&
           (run.count < D_TESTS_CPP_TOKENS)                        &&
           (d_syntax_group(run.tokens, run.count, &run.groups))    &&
           (d_syntax_items_build_with(run.tokens, run.count,
                                      &run.groups, _text, _options,
                                      &run.items)) );

    for (size_t i = 0u; ( (ok) && (i < _expected) ); ++i)
    {
        ok = d_tests_cpp_check(&run, &_expect[i]);
    }

    d_syntax_items_free(&run.items);
    d_syntax_groups_free(&run.groups);

    return ok;
}

/*
d_tests_cpp_template_args
  Commas, `=` and parentheses among template arguments.
  Tests the following:
  - a parameter whose type has a comma among its arguments is one
    parameter (`contracts.hpp:41`)
  - a `>>` closes two lists, and a parameter after it is its own
  - a function named after a type with arguments is still the function,
    and `d_result<void>` is no `void` return
*/
static bool
d_tests_cpp_template_args(void)
{
    static const char* const TEXT =
        "void take(const std::pair<int, Type>& _first, int _second);\n"
        "void deep(d_a<d_b<int, char>> _x, int _y);\n"
        "d_result<void>\n"
        "run(int _a)\n"
        "{\n"
        "    return d_ok();\n"
        "}\n";
    static const struct dj_tests_cpp_expect EXPECT[] =
    {
        { "void", 0u, D_SYNTAX_ITEM_FN_DECL, 0u, "take", 2,
          D_SYNTAX_ITEM_FLAG_RETURNS_VOID, 0u, 0u, NULL },
        { "int", 1u, D_SYNTAX_ITEM_PARAM, 0u, "_second", D_TESTS_CPP_ANY,
          0u, 0u, 1u, NULL },
        { "void", 1u, D_SYNTAX_ITEM_FN_DECL, 0u, "deep", 2, 0u, 0u, 0u, NULL },
        { "d_result", 0u, D_SYNTAX_ITEM_FN_DEF, 0u, "run", 1, 0u,
          D_SYNTAX_ITEM_FLAG_RETURNS_VOID, 0u, NULL }
    };

    return d_tests_cpp_text(TEXT, true, NULL, EXPECT,
                            sizeof(EXPECT) / sizeof(EXPECT[0]));
}

/*
d_tests_cpp_template_heads
  A template head with defaults, before a struct and before a function.
  Tests the following:
  - a default type in the head is no initializer: the struct has its member
    body, and its member is a definition (`templates.hpp:21`)
  - so with a default that holds a `> >` (`templates.hpp:37`)
  - a function definition begins at its `template` (`templates.hpp:69`)
*/
static bool
d_tests_cpp_template_heads(void)
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
    static const struct dj_tests_cpp_expect EXPECT[] =
    {
        { "template", 0u, D_SYNTAX_ITEM_DECLARATION, 0u, NULL,
          D_TESTS_CPP_ANY, 0u, 0u, 0u, NULL },
        { "Type", 1u, D_SYNTAX_ITEM_FN_DEF, 0u, "sum", 1, 0u, 0u, 1u, NULL },
        { "template", 1u, D_SYNTAX_ITEM_DECLARATION, 0u, NULL,
          D_TESTS_CPP_ANY, 0u, 0u, 0u, NULL },
        { "int", 2u, D_SYNTAX_ITEM_FN_DEF, 0u, "count", 1, 0u, 0u, 1u, NULL },
        { "template", 2u, D_SYNTAX_ITEM_FN_DEF, 0u, "d_free_sum", 1,
          D_SYNTAX_ITEM_FLAG_ENDS_RETURN, 0u, 0u, NULL }
    };

    return d_tests_cpp_text(TEXT, true, NULL, EXPECT,
                            sizeof(EXPECT) / sizeof(EXPECT[0]));
}

/*
d_tests_cpp_class_heads
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
static bool
d_tests_cpp_class_heads(void)
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
    static const struct dj_tests_cpp_expect EXPECT[] =
    {
        { "struct", 0u, D_SYNTAX_ITEM_DECLARATION, 0u, NULL,
          D_TESTS_CPP_ANY, 0u, 0u, 0u, NULL },
        { "int", 0u, D_SYNTAX_ITEM_FN_DEF, 0u, "sum", 2, 0u, 0u, 1u, NULL },
        { "struct", 1u, D_SYNTAX_ITEM_DECLARATION, 0u, NULL,
          D_TESTS_CPP_ANY, 0u, 0u, 0u, NULL },
        { "Return", 1u, D_SYNTAX_ITEM_FN_DEF, 0u, "apply", 2, 0u, 0u, 1u,
          NULL },
        { "struct", 2u, D_SYNTAX_ITEM_DECLARATION, 0u, NULL,
          D_TESTS_CPP_ANY, 0u, 0u, 0u, NULL },
        { "int", 4u, D_SYNTAX_ITEM_FN_DEF, 0u, "get", 1, 0u, 0u, 1u, NULL },
        { "class", 0u, D_SYNTAX_ITEM_DECLARATION, 0u, NULL,
          D_TESTS_CPP_ANY, 0u, 0u, 0u, NULL },
        { "int", 6u, D_SYNTAX_ITEM_DECLARATION, 0u, "m_count",
          D_TESTS_CPP_ANY, 0u, 0u, 1u, NULL },
        { "struct", 3u, D_SYNTAX_ITEM_FN_DEF, 0u, "d_make", 2, 0u, 0u, 0u,
          NULL }
    };

    return d_tests_cpp_text(TEXT, true, NULL, EXPECT,
                            sizeof(EXPECT) / sizeof(EXPECT[0]));
}

/*
d_tests_cpp_initializers
  Constructors' initializer lists (`initializers.hpp:13`, `:24`).
  Tests the following:
  - the body is the brace after the last initializer, not the first
    initializer's brace: the constructor reaches its last initializer
  - an initializer list of one braced initializer before an empty body
  - an out-of-class constructor at file scope
*/
static bool
d_tests_cpp_initializers(void)
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
    static const struct dj_tests_cpp_expect EXPECT[] =
    {
        { "d_braced", 1u, D_SYNTAX_ITEM_FN_DEF, 0u, "d_braced", 1, 0u,
          D_SYNTAX_ITEM_FLAG_EMPTY_BODY, 1u, "m_count" },
        { "m_count", 1u, D_SYNTAX_ITEM_STATEMENT, D_SYNTAX_STMT_SIMPLE, NULL,
          D_TESTS_CPP_ANY, 0u, 0u, 2u, NULL },
        { "d_braced", 2u, D_SYNTAX_ITEM_FN_DEF, 0u, "d_braced", 0,
          D_SYNTAX_ITEM_FLAG_EMPTY_BODY, 0u, 1u, NULL },
        { "d_x", 0u, D_SYNTAX_ITEM_FN_DEF, 0u, "d_x", 1,
          D_SYNTAX_ITEM_FLAG_EMPTY_BODY, 0u, 0u, "m_a" }
    };

    return d_tests_cpp_text(TEXT, true, NULL, EXPECT,
                            sizeof(EXPECT) / sizeof(EXPECT[0]));
}

/*
d_tests_cpp_namespaces
  The namespace macros a caller names (`layout.hpp:5`, `namespaces.hpp:33`,
`source.cpp:6`).
  Tests the following:
  - each named macro is a block item of its own, `open` or `close`, and
    swallows nothing: the function after `NS_DJINTERP` begins at its type
  - `D_NAMESPACE(x)` takes its argument group as its `params`
  - with no options, the old reading stands: the function begins at the
    macro, as a C caller that names none still sees
*/
static bool
d_tests_cpp_namespaces(void)
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
    static const struct dj_tests_cpp_expect NAMED[] =
    {
        { "NS_DJINTERP", 0u, D_SYNTAX_ITEM_BLOCK, D_SYNTAX_STMT_OPEN, NULL,
          D_TESTS_CPP_ANY, D_SYNTAX_ITEM_FLAG_MACRO_HEAD, 0u, 0u, NULL },
        { "int", 0u, D_SYNTAX_ITEM_FN_DEF, 0u, "d_f", 1, 0u, 0u, 0u, NULL },
        { "NS_END", 0u, D_SYNTAX_ITEM_BLOCK, D_SYNTAX_STMT_CLOSE, NULL,
          D_TESTS_CPP_ANY, D_SYNTAX_ITEM_FLAG_MACRO_HEAD, 0u, 0u, NULL },
        { "D_NAMESPACE", 0u, D_SYNTAX_ITEM_BLOCK, D_SYNTAX_STMT_OPEN, NULL,
          0, D_SYNTAX_ITEM_FLAG_MACRO_HEAD, 0u, 0u, NULL },
        { "struct", 0u, D_SYNTAX_ITEM_DECLARATION, 0u, NULL,
          D_TESTS_CPP_ANY, 0u, 0u, 0u, NULL },
        { "NS_END", 1u, D_SYNTAX_ITEM_BLOCK, D_SYNTAX_STMT_CLOSE, NULL,
          D_TESTS_CPP_ANY, D_SYNTAX_ITEM_FLAG_MACRO_HEAD, 0u, 0u, NULL }
    };
    static const struct dj_tests_cpp_expect UNNAMED[] =
    {
        { "NS_DJINTERP", 0u, D_SYNTAX_ITEM_FN_DEF, 0u, "d_f", 1, 0u, 0u, 0u,
          NULL }
    };

    const bool named   = d_tests_cpp_text(TEXT, true, &OPTIONS, NAMED,
                             sizeof(NAMED) / sizeof(NAMED[0]));
    const bool unnamed = d_tests_cpp_text(TEXT, true, NULL, UNNAMED, 1u);

    return ( (named) && (unnamed) );
}

/*
d_tests_cpp_c_unchanged
  C, where a `<` after a name compares.
  Tests the following:
  - an initializer's comparisons leave the declarator alone: the first of
    two declarators is the name
  - a C function's parameters still split at their commas
*/
static bool
d_tests_cpp_c_unchanged(void)
{
    static const char* const TEXT =
        "static int d_x = D_A < D_B, d_y = D_C > D_D;\n"
        "int d_g(int _a, int _b)\n"
        "{\n"
        "    return _a < _b;\n"
        "}\n";
    static const struct dj_tests_cpp_expect EXPECT[] =
    {
        { "static", 0u, D_SYNTAX_ITEM_DECLARATION, 0u, "d_x",
          D_TESTS_CPP_ANY, 0u, 0u, 0u, NULL },
        { "int", 1u, D_SYNTAX_ITEM_FN_DEF, 0u, "d_g", 2, 0u, 0u, 0u, NULL }
    };

    return d_tests_cpp_text(TEXT, false, NULL, EXPECT,
                            sizeof(EXPECT) / sizeof(EXPECT[0]));
}

/*
main
  Runs the six sections.
*/
int
main(void)
{
    const bool args     = d_tests_cpp_template_args();
    const bool heads    = d_tests_cpp_template_heads();
    const bool classes  = d_tests_cpp_class_heads();
    const bool inits    = d_tests_cpp_initializers();
    const bool spaces   = d_tests_cpp_namespaces();
    const bool c_only   = d_tests_cpp_c_unchanged();

    printf("  [%s] syntax_cpp_template_args\n", (args) ? "PASS" : "FAIL");
    printf("  [%s] syntax_cpp_template_heads\n", (heads) ? "PASS" : "FAIL");
    printf("  [%s] syntax_cpp_class_heads\n", (classes) ? "PASS" : "FAIL");
    printf("  [%s] syntax_cpp_initializers\n", (inits) ? "PASS" : "FAIL");
    printf("  [%s] syntax_cpp_namespaces\n", (spaces) ? "PASS" : "FAIL");
    printf("  [%s] syntax_cpp_c_unchanged\n", (c_only) ? "PASS" : "FAIL");

    return ( (args) && (heads) && (classes) && (inits) && (spaces) &&
             (c_only) )
           ? 0
           : 1;
}
