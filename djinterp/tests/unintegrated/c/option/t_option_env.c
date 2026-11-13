/*******************************************************************************
* djinterp [test]                                                 t_option_env.c
*
*   The machine environment's C harness (c/option/option_env.h): declaring,
* reading defaults, binding by value and by text, scopes, parent frames,
* scripts, getenv, writing values back as text, and the two compactions. A
* standalone program, as the other tests not yet in DTest are: it prints a
* line per group and one per failed check, ends with "N checks, M failed",
* and exits 0 only when nothing failed.
*
*   A MODEL CARRIES THE BINDINGS. d_tests_option_env_random binds, rebinds and
* unbinds random variables in random scopes of a child frame and its parent,
* by value and by text, through arenas small enough to compact over and over,
* and after every step reads every variable in every scope and holds it to a
* plain table that did the same.
*
*   The locale check runs under de_DE.UTF-8 and skips where it is not
* installed.
*
*
* path:      /tests/unintegrated/c/option/t_option_env.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.11.10
*******************************************************************************/
// std
#include <locale.h>   // setlocale, LC_NUMERIC
#include <stdbool.h>  // bool
#include <stdio.h>    // printf, snprintf
#include <stdlib.h>   // setenv, unsetenv
#include <string.h>   // memcmp, memcpy, strcmp, strlen
// djinterp
#include "../../../../inc/djinterp/c/option/option_env.h"  // the environment
// re_std
#include "../../../../inc/re_std/c/cstdint/dstdint.h"  // int64_t, INT64_MIN


// D_TESTS_OPTION_ENV_CHECK
//   macro: one check: counts it, and prints the expression and line on
// failure.
#define D_TESTS_OPTION_ENV_CHECK(expr)                                         \
    d_tests_option_env_check(((expr) ? 1 : 0), #expr, __LINE__)

// D_TESTS_OPTION_ENV_STATUS
//   macro: a result's status.
#define D_TESTS_OPTION_ENV_STATUS(result)                                      \
    d_tests_option_env_status(result)

static size_t   d_tests_option_env_checks;
static size_t   d_tests_option_env_failures;
static uint64_t d_tests_option_env_state = 0x2545F4914F6CDD1Du;

/*
d_tests_option_env_check
  Counts one check, and reports it when it failed.
*/
static bool
d_tests_option_env_check(
    int         _ok,
    const char* _expression,
    int         _line
)
{
    ++d_tests_option_env_checks;

    if (_ok == 0)
    {
        ++d_tests_option_env_failures;
        printf("    FAILED line %d: %s\n", _line, _expression);
    }

    return (_ok != 0);
}

/*
d_tests_option_env_status
  A result's status: OK, or its error.
*/
static int32_t
d_tests_option_env_status(
    struct dj_option_result _result
)
{
    return d_option_result_status(&_result);
}

/*
d_tests_option_env_next
  xorshift64*: a repeatable stream of random numbers.
*/
static uint64_t
d_tests_option_env_next(void)
{
    d_tests_option_env_state ^= d_tests_option_env_state >> 12;
    d_tests_option_env_state ^= d_tests_option_env_state << 25;
    d_tests_option_env_state ^= d_tests_option_env_state >> 27;

    return d_tests_option_env_state * 0x2545F4914F6CDD1Du;
}

// the harness's schema: one variable of each kind, and a second text
static const struct dj_option_env_name D_TESTS_STYLES[] =
{
    { "text",     0 },
    { "json",     1 },
    { "markdown", 2 }
};

static const struct dj_option_env_name D_TESTS_PARTS[] =
{
    { "none",     0x00 },
    { "language", 0x01 },
    { "compiler", 0x02 },
    { "os",       0x04 },
    { "posix",    0x08 },
    { "core",     0x07 },
    { "all",      0x0F }
};

static const struct dj_option_env_var D_TESTS_SCHEMA[] =
{
    D_OPTION_ENV_BOOL("show_title", 1, "whether the title is shown"),
    D_OPTION_ENV_INT("key_width", 24, "the key column"),
    D_OPTION_ENV_REAL("ratio", 0.5, "a real"),
    D_OPTION_ENV_TEXT("title", "machine", "the title"),
    D_OPTION_ENV_CHOICE("style", D_TESTS_STYLES, 0, "the style"),
    D_OPTION_ENV_FLAGS("sections", D_TESTS_PARTS, 0x07, "sections shown"),
    D_OPTION_ENV_TEXT("entry", NULL, "a template")
};

enum
{
    D_TESTS_SHOW_TITLE = 0,
    D_TESTS_KEY_WIDTH  = 1,
    D_TESTS_RATIO      = 2,
    D_TESTS_TITLE      = 3,
    D_TESTS_STYLE      = 4,
    D_TESTS_SECTIONS   = 5,
    D_TESTS_ENTRY      = 6,
    D_TESTS_VARS       = 7
};

static const char* const D_TESTS_SCOPES[] = { "language", "compiler" };

D_OPTION_ENV_DECLARE(dj_tests_root_storage, 32, 16, 512);
D_OPTION_ENV_DECLARE(dj_tests_child_storage, 16, 0, 96);

/*
d_tests_option_env_root
  A root frame over the harness's schema and scopes.
*/
static void
d_tests_option_env_root(
    struct dj_option_env*         _env,
    struct dj_tests_root_storage* _storage
)
{
    (void)D_OPTION_ENV_VIEW(_env, *_storage);
    (void)d_option_env_declare_all(_env,
                                   D_TESTS_SCHEMA,
                                   (uint32_t)(sizeof(D_TESTS_SCHEMA) /
                                              sizeof(D_TESTS_SCHEMA[0])));
    (void)d_option_env_set_scopes(_env, D_TESTS_SCOPES, 2u);

    return;
}

/*
d_tests_option_env_text_is
  Whether a variable writes as the text expected.
*/
static int
d_tests_option_env_text_is(
    const struct dj_option_env* _env,
    uint32_t                    _scope,
    uint32_t                    _var,
    const char*                 _expected
)
{
    char         buffer[128];
    const size_t length = d_option_env_format(_env,
                                              _scope,
                                              _var,
                                              buffer,
                                              sizeof(buffer));

    if ( (length != strlen(_expected)) ||
         (strcmp(buffer, _expected) != 0) )
    {
        printf("    var %u scope %u: got [%s], expected [%s]\n",
               (unsigned)_var, (unsigned)_scope, buffer, _expected);

        return 0;
    }

    return 1;
}

/*
d_tests_option_env_declarations
  Tests the following:
  - a schema declares in order; names are found exactly
  - malformed declarations, duplicates and a full frame are refused
  - every default reads, a choice with its name
*/
static void
d_tests_option_env_declarations(void)
{
    static const struct dj_option_env_name NONE[1] = { { "x", 1 } };
    static const struct dj_option_env_var  BAD[] =
    {
        D_OPTION_ENV_INT("9lives", 0, NULL),
        D_OPTION_ENV_INT("has-dash", 0, NULL),
        D_OPTION_ENV_INT("", 0, NULL),
        D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_CHOICE, "nameless_choice", NULL,
                         0u, NULL, 0u, NULL, 0, INT64_MIN, INT64_MAX, 0.0,
                         NULL, NULL, NULL),
        D_OPTION_ENV_ROW(9, "bad_kind", NONE, 1u, NULL, 0u, NULL, 0,
                         INT64_MIN, INT64_MAX, 0.0, NULL, NULL, NULL),
        D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_INT, "bad_alias", NULL, 0u, NULL,
                         0u, NULL, 0, INT64_MIN, INT64_MAX, 0.0, "a-b", NULL,
                         NULL),
        D_OPTION_ENV_INT_RANGE("outside", 50, 0, 40, NULL),
        D_OPTION_ENV_CHOICE("strange_choice", NONE, 7, NULL),
        D_OPTION_ENV_FLAGS("strange_flags", NONE, 2, NULL),
        D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_BOOL, "two", NULL, 0u, NULL, 0u,
                         NULL, 2, INT64_MIN, INT64_MAX, 0.0, NULL, NULL,
                         NULL),
        D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_INT, "lost_list", NULL, 0u, NULL,
                         3u, NULL, 0, INT64_MIN, INT64_MAX, 0.0, NULL, NULL,
                         NULL),
        D_OPTION_ENV_INT("key_width", 0, NULL),
        D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_INT, "fresh", NULL, 0u, NULL, 0u,
                         NULL, 0, INT64_MIN, INT64_MAX, 0.0, "title", NULL,
                         NULL)
    };
    struct dj_tests_root_storage storage = D_OPTION_ENV_STORAGE_INIT;
    struct dj_option_env         env;
    struct dj_option_env_value   value;

    printf("  declarations and defaults\n");

    d_tests_option_env_root(&env, &storage);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_var_count(&env) == D_TESTS_VARS);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_find(&env, "style", 5u).payload.value
                             == D_TESTS_STYLE);
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_find(&env, "styl", 4u).is_ok);
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_find(&env, "STYLE", 5u).is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_find_scope(&env, "compiler", 8u)
                             .payload.value == 2u);

    for (size_t i = 0u; i < sizeof(BAD) / sizeof(BAD[0]); ++i)
    {
        const int32_t expected = (i >= 11u)
            ? (int32_t)D_OPTION_STATUS_KEY_DUPLICATE
            : (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT;

        D_TESTS_OPTION_ENV_CHECK(
            D_TESTS_OPTION_ENV_STATUS(d_option_env_declare(&env, &BAD[i]))
            == expected);
    }

    // the defaults, each as its kind and as text
    D_TESTS_OPTION_ENV_CHECK(d_option_env_bool(&env, 0u, D_TESTS_SHOW_TITLE));
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&env, 0u, D_TESTS_KEY_WIDTH)
                             == 24);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_real(&env, 0u, D_TESTS_RATIO)
                             == 0.5);
    D_TESTS_OPTION_ENV_CHECK(strcmp(d_option_env_text(&env, 0u, D_TESTS_TITLE),
                                    "machine") == 0);
    D_TESTS_OPTION_ENV_CHECK(strcmp(d_option_env_text(&env, 0u, D_TESTS_STYLE),
                                    "text") == 0);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_text(&env, 0u, D_TESTS_ENTRY)
                             == NULL);
    D_TESTS_OPTION_ENV_CHECK( (d_option_env_get(&env, 2u, D_TESTS_SECTIONS,
                                                &value).is_ok) &&
                              (value.integer == 0x07)          &&
                              (value.bound == 0)               &&
                              (value.kind == D_OPTION_ENV_KIND_FLAGS) );
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
                                 d_option_env_get(&env, 3u, 0u, &value))
                             == (int32_t)D_OPTION_STATUS_KEY_ABSENT);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
                                 d_option_env_get(&env, 0u, 99u, &value))
                             == (int32_t)D_OPTION_STATUS_KEY_ABSENT);
    D_TESTS_OPTION_ENV_CHECK(strcmp(d_option_env_kind_name(
                                        D_OPTION_ENV_KIND_FLAGS),
                                    "flags") == 0);
    D_TESTS_OPTION_ENV_CHECK(strcmp(d_option_env_kind_name(0), "unknown")
                             == 0);

    return;
}

/*
d_tests_option_env_values
  Tests the following:
  - binding each kind by value, and reading it back, bound
  - a kind mismatch; a choice outside its values; flags outside their names
  - unbinding falls back to the default; unbinding twice is fine
  - every change moves the generation
*/
static void
d_tests_option_env_values(void)
{
    struct dj_tests_root_storage storage = D_OPTION_ENV_STORAGE_INIT;
    struct dj_option_env         env;
    struct dj_option_env_value   value;

    printf("  binding by value\n");

    d_tests_option_env_root(&env, &storage);

    const uint32_t generation = d_option_env_generation(&env);

    D_TESTS_OPTION_ENV_CHECK(d_option_env_set_bool(&env, 0u,
                                                   D_TESTS_SHOW_TITLE,
                                                   false).is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_generation(&env) != generation);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_set_int(&env, 0u, D_TESTS_KEY_WIDTH,
                                                  INT64_MIN).is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_set_real(&env, 0u, D_TESTS_RATIO,
                                                   -2.25).is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_set_text(&env, 0u, D_TESTS_TITLE,
                                                   "borrowed").is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_set_int(&env, 0u, D_TESTS_STYLE,
                                                  2).is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_set_int(&env, 0u, D_TESTS_SECTIONS,
                                                  0x09).is_ok);

    D_TESTS_OPTION_ENV_CHECK(!d_option_env_bool(&env, 0u, D_TESTS_SHOW_TITLE));
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&env, 0u, D_TESTS_KEY_WIDTH)
                             == INT64_MIN);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_real(&env, 0u, D_TESTS_RATIO)
                             == -2.25);
    D_TESTS_OPTION_ENV_CHECK(strcmp(d_option_env_text(&env, 0u, D_TESTS_TITLE),
                                    "borrowed") == 0);
    D_TESTS_OPTION_ENV_CHECK(strcmp(d_option_env_text(&env, 0u, D_TESTS_STYLE),
                                    "markdown") == 0);
    D_TESTS_OPTION_ENV_CHECK( (d_option_env_get(&env, 0u, D_TESTS_SECTIONS,
                                                &value).is_ok) &&
                              (value.integer == 0x09)          &&
                              (value.bound == 1) );

    // refusals
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_set_int(&env, 0u, D_TESTS_TITLE, 1))
        == (int32_t)D_OPTION_STATUS_VALUE_TYPE_MISMATCH);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_set_bool(&env, 0u, D_TESTS_KEY_WIDTH, true))
        == (int32_t)D_OPTION_STATUS_VALUE_TYPE_MISMATCH);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_set_int(&env, 0u, D_TESTS_STYLE, 7))
        == (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_set_int(&env, 0u, D_TESTS_SECTIONS, 0x10))
        == (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_set_text_copy(&env, 0u, D_TESTS_RATIO, "x", 1u))
        == (int32_t)D_OPTION_STATUS_VALUE_TYPE_MISMATCH);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_set_int(&env, 9u, D_TESTS_KEY_WIDTH, 1))
        == (int32_t)D_OPTION_STATUS_KEY_ABSENT);

    // unbinding
    D_TESTS_OPTION_ENV_CHECK(d_option_env_unset(&env, 0u,
                                                D_TESTS_KEY_WIDTH).is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_unset(&env, 0u,
                                                D_TESTS_KEY_WIDTH).is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&env, 0u, D_TESTS_KEY_WIDTH)
                             == 24);
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_is_bound(&env, 0u,
                                                    D_TESTS_KEY_WIDTH));
    D_TESTS_OPTION_ENV_CHECK(d_option_env_is_bound(&env, 0u, D_TESTS_RATIO));
    d_option_env_clear(&env);
    D_TESTS_OPTION_ENV_CHECK( (d_option_env_real(&env, 0u, D_TESTS_RATIO)
                               == 0.5) &&
                              (env.set.count == 0u) );

    return;
}

/*
d_tests_option_env_scopes_and_frames
  Tests the following:
  - a scoped binding is seen in its scope only, ahead of the global one
  - a child sees its parent's bindings through its own; its own global
    binding comes before the parent's scoped one
  - the generation sums the chain
  - a frame with a child cannot take a parent; a cycle is refused; a
    child declares while it has room; detaching counts the child off
*/
static void
d_tests_option_env_scopes_and_frames(void)
{
    struct dj_tests_root_storage  root_storage  = D_OPTION_ENV_STORAGE_INIT;
    struct dj_tests_child_storage child_storage = D_OPTION_ENV_STORAGE_INIT;
    struct dj_option_env          root;
    struct dj_option_env          child;

    printf("  scopes and frames\n");

    d_tests_option_env_root(&root, &root_storage);
    (void)D_OPTION_ENV_VIEW(&child, child_storage);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_set_parent(&child, &root).is_ok);

    // scopes, in the root
    (void)d_option_env_set_int(&root, 0u, D_TESTS_KEY_WIDTH, 30);
    (void)d_option_env_set_int(&root, 2u, D_TESTS_KEY_WIDTH, 40);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&root, 0u, D_TESTS_KEY_WIDTH)
                             == 30);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&root, 1u, D_TESTS_KEY_WIDTH)
                             == 30);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&root, 2u, D_TESTS_KEY_WIDTH)
                             == 40);

    // the child sees through, then overrides
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&child, 2u, D_TESTS_KEY_WIDTH)
                             == 40);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_var_count(&child) == D_TESTS_VARS);

    const uint32_t generation = d_option_env_generation(&child);

    (void)d_option_env_set_int(&child, 0u, D_TESTS_KEY_WIDTH, 50);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&child, 2u, D_TESTS_KEY_WIDTH)
                             == 50);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&root, 2u, D_TESTS_KEY_WIDTH)
                             == 40);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_generation(&child) != generation);

    const uint32_t before = d_option_env_generation(&child);

    (void)d_option_env_set_bool(&root, 0u, D_TESTS_SHOW_TITLE, false);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_generation(&child) != before);
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_bool(&child, 1u,
                                                D_TESTS_SHOW_TITLE));
    D_TESTS_OPTION_ENV_CHECK(d_option_env_root(&child) == &root);

    // a frame with a child takes no parent; a cycle is refused; a child
    // with no room declares nothing
    D_TESTS_OPTION_ENV_CHECK(root.child_count == 1u);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_set_parent(&root, &child))
        == (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_set_parent(&child, &child))
        == (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_declare(&child,
                                                  &D_TESTS_SCHEMA[0]).is_ok);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_declare(&child, &D_TESTS_SCHEMA[1]))
        == (int32_t)D_OPTION_STATUS_CAPACITY);
    d_option_env_detach(&child);
    D_TESTS_OPTION_ENV_CHECK( (root.child_count == 0u) &&
                              (child.depth == 0u) );

    return;
}

/*
d_tests_option_env_text_more
  Tests the following:
  - text, kept as it is, quoted with escapes
  - scopes, unsetting, and statements that are not
  - flags no greedy choice of names covers; a buffer too small
*/
static void
d_tests_option_env_text_more(
    struct dj_option_env* _env
)
{
    char small[6];

    // text, kept as it is, quoted with escapes
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(_env, "title =  two words ")
                             .is_ok);
    D_TESTS_OPTION_ENV_CHECK(strcmp(d_option_env_text(_env, 0u, D_TESTS_TITLE),
                                    "two words") == 0);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(_env,
                                 "entry=\"{a};\\t\\\"b\\\"\\x41\\n\"").is_ok);
    D_TESTS_OPTION_ENV_CHECK(strcmp(d_option_env_text(_env, 0u, D_TESTS_ENTRY),
                                    "{a};\t\"b\"A\n") == 0);
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_put(_env, "entry=\"\\q\"").is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(_env,
                                 "entry=\"\\\\\\'\\r|\\0hidden\"").is_ok);
    D_TESTS_OPTION_ENV_CHECK(strcmp(d_option_env_text(_env, 0u, D_TESTS_ENTRY),
                                    "\\'\r|") == 0);
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_put(_env, "entry=\"\\xZZ\"").is_ok);
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_put(_env, "entry=\"\\x4\"").is_ok);
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_put(_env, "entry=\"a\\\"").is_ok);
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_put(_env, "entry=\"open").is_ok);
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_put(_env, "entry=\"a\" b").is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(_env, "key_width=\" 12 \"")
                             .is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(_env, 0u, D_TESTS_KEY_WIDTH)
                             == 12);

    // scopes, unsetting, and statements that are not
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(_env, "compiler.key_width=7")
                             .is_ok);
    D_TESTS_OPTION_ENV_CHECK( (d_option_env_int(_env, 2u, D_TESTS_KEY_WIDTH)
                               == 7) &&
                              (d_option_env_int(_env, 1u, D_TESTS_KEY_WIDTH)
                               == 12) );
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(_env, " -compiler.key_width ")
                             .is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(_env, 2u, D_TESTS_KEY_WIDTH)
                             == 12);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_put(_env, "linker.key_width=1"))
        == (int32_t)D_OPTION_STATUS_KEY_ABSENT);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_put(_env, "width=1"))
        == (int32_t)D_OPTION_STATUS_KEY_ABSENT);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_put(_env, "=1"))
        == (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_put(_env, "key_width 1"))
        == (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_put(_env, "-key_width=1"))
        == (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_put(_env, "key_width"))
        == (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);

    // flags no greedy choice of names covers: the rest as a number
    {
        static const struct dj_option_env_name OVERLAP[] =
        {
            { "low",  0x3 },
            { "high", 0x6 }
        };
        static const struct dj_option_env_var  MASKED[] =
        {
            D_OPTION_ENV_FLAGS("masked", OVERLAP, 0x7, NULL)
        };
        struct dj_tests_root_storage other = D_OPTION_ENV_STORAGE_INIT;
        struct dj_option_env         flags;

        (void)D_OPTION_ENV_VIEW(&flags, other);
        (void)d_option_env_declare_all(&flags, MASKED, 1u);
        D_TESTS_OPTION_ENV_CHECK(d_tests_option_env_text_is(&flags, 0u, 0u,
                                                            "low|0x4"));
        (void)d_option_env_put(&flags, "masked=");
        D_TESTS_OPTION_ENV_CHECK(d_tests_option_env_text_is(&flags, 0u, 0u,
                                                            "0"));
    }

    // a buffer too small: truncated, terminated, the whole length returned
    D_TESTS_OPTION_ENV_CHECK(d_option_env_format(_env, 0u, D_TESTS_TITLE,
                                                 small, sizeof(small)) == 9u);
    D_TESTS_OPTION_ENV_CHECK(strcmp(small, "two w") == 0);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_format(_env, 0u, D_TESTS_TITLE,
                                                 NULL, 0u) == 9u);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_format(_env, 0u, 99u, small,
                                                 sizeof(small)) == (size_t)-1);

    return;
}

/*
d_tests_option_env_text
  Tests the following:
  - each kind's reader: bool words, integers in every base and at the
    extremes, reals in C's form, choices by name and by value, flags by
    names and numbers, absolute and relative
  - what each refuses
  - scoped names; "name" alone; "-name"
  - writing each back as text, and the round trip
*/
static void
d_tests_option_env_text(void)
{
    struct dj_tests_root_storage storage = D_OPTION_ENV_STORAGE_INIT;
    struct dj_option_env         env;

    printf("  binding by text and writing as text\n");

    d_tests_option_env_root(&env, &storage);

    // bools
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&env, "show_title = OFF").is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_tests_option_env_text_is(&env, 0u,
                                                        D_TESTS_SHOW_TITLE,
                                                        "false"));
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&env, "show_title=Yes").is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_bool(&env, 0u, D_TESTS_SHOW_TITLE));
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&env, "show_title=n").is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&env, "show_title").is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_bool(&env, 0u, D_TESTS_SHOW_TITLE));
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_put(&env, "show_title=maybe").is_ok);

    // integers
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&env, "key_width=0x1F").is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&env, 0u, D_TESTS_KEY_WIDTH)
                             == 31);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&env, "key_width=-0b101").is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&env, 0u, D_TESTS_KEY_WIDTH)
                             == -5);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&env, "key_width=0o17").is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&env, 0u, D_TESTS_KEY_WIDTH)
                             == 15);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&env,
                                 "key_width=-9223372036854775808").is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_tests_option_env_text_is(&env, 0u,
                                 D_TESTS_KEY_WIDTH, "-9223372036854775808"));
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_put(&env,
                                 "key_width=9223372036854775808").is_ok);
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_put(&env, "key_width=12a").is_ok);
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_put(&env, "key_width=0x").is_ok);
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_put(&env, "key_width=").is_ok);

    // reals
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&env, "ratio=1.5e2").is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_real(&env, 0u, D_TESTS_RATIO)
                             == 150.0);
    D_TESTS_OPTION_ENV_CHECK(d_tests_option_env_text_is(&env, 0u,
                                                        D_TESTS_RATIO, "150"));
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&env, "ratio=0.1").is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_tests_option_env_text_is(&env, 0u,
                                 D_TESTS_RATIO, "0.10000000000000001"));
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&env, "ratio=-inf").is_ok);
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_put(&env, "ratio=1,5").is_ok);
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_put(&env, "ratio=1e999").is_ok);
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_put(&env, "ratio=0x1p3").is_ok);

    // choices
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&env, "style=JSON").is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_tests_option_env_text_is(&env, 0u,
                                                        D_TESTS_STYLE, "json"));
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&env, "style=2").is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_tests_option_env_text_is(&env, 0u,
                                                        D_TESTS_STYLE,
                                                        "markdown"));
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_put(&env, "style=yaml").is_ok);
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_put(&env, "style=3").is_ok);

    // flags: absolute, relative, numbers; written greedily
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&env,
                                              "sections=core,-os").is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&env, 0u, D_TESTS_SECTIONS)
                             == 0x03);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&env, "sections=+posix").is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_tests_option_env_text_is(&env, 0u,
                                 D_TESTS_SECTIONS, "language|compiler|posix"));
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&env,
                                              "sections=all -compiler").is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&env, 0u, D_TESTS_SECTIONS)
                             == 0x0D);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&env, "sections=0x0F").is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_tests_option_env_text_is(&env, 0u,
                                                        D_TESTS_SECTIONS,
                                                        "all"));
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&env, "sections=").is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_tests_option_env_text_is(&env, 0u,
                                                        D_TESTS_SECTIONS,
                                                        "none"));
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_put(&env, "sections=core,bogus")
                             .is_ok);
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_put(&env, "sections=0x10").is_ok);
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_put(&env, "sections=core,+").is_ok);

    d_tests_option_env_text_more(&env);

    return;
}

/*
d_tests_option_env_scripts
  Tests the following:
  - statements split at ';' and line breaks, comments to the line's end,
    quoted ';' and '#' kept
  - the count of statements done; the first failure stops it, with its
    offset, and what came before stays done
*/
static void
d_tests_option_env_scripts(void)
{
    struct dj_tests_root_storage storage = D_OPTION_ENV_STORAGE_INIT;
    struct dj_option_env         env;
    size_t                       offset = 0u;

    printf("  scripts\n");

    d_tests_option_env_root(&env, &storage);

    const struct dj_option_result done = d_option_env_load(
        &env,
        "# a comment line\n"
        "style=json; key_width = 30   # the key column\n"
        "\n;;title=\"a;b\\\";#c\"\n"
        "  -show_title\r\n"
        "language.key_width=10",
        &offset);

    D_TESTS_OPTION_ENV_CHECK( (done.is_ok) &&
                              (done.payload.value == 5u) );
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&env, 0u, D_TESTS_KEY_WIDTH)
                             == 30);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&env, 1u, D_TESTS_KEY_WIDTH)
                             == 10);
    D_TESTS_OPTION_ENV_CHECK(strcmp(d_option_env_text(&env, 0u, D_TESTS_TITLE),
                                    "a;b\";#c") == 0);
    D_TESTS_OPTION_ENV_CHECK(strcmp(d_option_env_text(&env, 0u, D_TESTS_STYLE),
                                    "json") == 0);

    const struct dj_option_result failed = d_option_env_load(
        &env,
        "key_width=40; style=bogus; key_width=50",
        &offset);

    D_TESTS_OPTION_ENV_CHECK( (!failed.is_ok) &&
                              (offset == 14u) );
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&env, 0u, D_TESTS_KEY_WIDTH)
                             == 40);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_load(&env, "", NULL).payload.value
                             == 0u);

    return;
}

/*
d_tests_option_env_environment
  Tests the following:
  - getenv under a prefix, names upper-cased
  - a value refused is reported, and the rest still read
*/
static void
d_tests_option_env_environment(void)
{
    struct dj_tests_root_storage storage = D_OPTION_ENV_STORAGE_INIT;
    struct dj_option_env         env;

    printf("  the process environment\n");

    d_tests_option_env_root(&env, &storage);

#if defined(_WIN32)
    (void)_putenv_s("DJTEST_KEY_WIDTH", "33");
    (void)_putenv_s("DJTEST_STYLE", "markdown");
    (void)_putenv_s("DJTEST_RATIO", "oops");
#else
    (void)setenv("DJTEST_KEY_WIDTH", "33", 1);
    (void)setenv("DJTEST_STYLE", "markdown", 1);
    (void)setenv("DJTEST_RATIO", "oops", 1);
#endif

    const struct dj_option_result result = d_option_env_import(&env,
                                                              "DJTEST_");

    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(result)
                             == (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&env, 0u, D_TESTS_KEY_WIDTH)
                             == 33);
    D_TESTS_OPTION_ENV_CHECK(strcmp(d_option_env_text(&env, 0u, D_TESTS_STYLE),
                                    "markdown") == 0);

#if defined(_WIN32)
    (void)_putenv_s("DJTEST_RATIO", "");
#else
    (void)unsetenv("DJTEST_RATIO");
#endif

    const struct dj_option_result clean = d_option_env_import(&env, "DJTEST_");

    D_TESTS_OPTION_ENV_CHECK( (clean.is_ok) &&
                              (clean.payload.value == 2u) );

    return;
}

/*
d_tests_option_env_locale
  Tests the following:
  - under a locale whose decimal point is ',', reals are still read and
    written in C's form, and the locale's own form is refused
*/
static void
d_tests_option_env_locale(void)
{
    struct dj_tests_root_storage storage = D_OPTION_ENV_STORAGE_INIT;
    struct dj_option_env         env;

    printf("  reals under de_DE.UTF-8\n");

    if (setlocale(LC_NUMERIC, "de_DE.UTF-8") == NULL)
    {
        printf("    (skipped: the locale is not installed)\n");

        return;
    }

    d_tests_option_env_root(&env, &storage);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&env, "ratio=2.75").is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_real(&env, 0u, D_TESTS_RATIO)
                             == 2.75);
    D_TESTS_OPTION_ENV_CHECK(d_tests_option_env_text_is(&env, 0u,
                                                        D_TESTS_RATIO,
                                                        "2.75"));
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_put(&env, "ratio=2,75").is_ok);
    (void)setlocale(LC_NUMERIC, "C");

    return;
}

/*
d_tests_option_env_arena
  Tests the following:
  - rebinding text over and over compacts the arena instead of filling it
  - a copy whose source is another binding's text survives the compaction
    its own allocation runs
  - text no compaction can make room for is BUFFER_TOO_SMALL, and the
    binding it would have replaced is kept
*/
static void
d_tests_option_env_arena(void)
{
    struct dj_tests_root_storage  root_storage  = D_OPTION_ENV_STORAGE_INIT;
    struct dj_tests_child_storage child_storage = D_OPTION_ENV_STORAGE_INIT;
    struct dj_option_env          root;
    struct dj_option_env          child;
    char                          text[64];
    char                          big[200];

    printf("  the text arena\n");

    d_tests_option_env_root(&root, &root_storage);
    (void)D_OPTION_ENV_VIEW(&child, child_storage);
    (void)d_option_env_set_parent(&child, &root);

    // 96 bytes of arena, rebound 500 times
    for (int i = 0; i < 500; ++i)
    {
        const int length = snprintf(text, sizeof(text), "title number %d", i);

        D_TESTS_OPTION_ENV_CHECK(d_option_env_set_text_copy(&child, 0u,
                                                            D_TESTS_TITLE,
                                                            text,
                                                            (size_t)length)
                                 .is_ok);
    }

    D_TESTS_OPTION_ENV_CHECK(strcmp(d_option_env_text(&child, 0u,
                                                      D_TESTS_TITLE),
                                    "title number 499") == 0);

    // a copy of a copy, from inside the arena, near full
    (void)d_option_env_set_text_copy(&child, 1u, D_TESTS_ENTRY,
                                     "0123456789012345678901234567890123456",
                                     37u);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_set_text_copy(
        &child, 2u, D_TESTS_ENTRY,
        d_option_env_text(&child, 1u, D_TESTS_ENTRY) + 10, 27u).is_ok);
    D_TESTS_OPTION_ENV_CHECK(strcmp(d_option_env_text(&child, 2u,
                                                      D_TESTS_ENTRY),
                                    "012345678901234567890123456") == 0);
    D_TESTS_OPTION_ENV_CHECK(strcmp(d_option_env_text(&child, 0u,
                                                      D_TESTS_TITLE),
                                    "title number 499") == 0);

    // a copy whose source moves in the compaction its own room needs
    d_option_env_clear(&child);
    (void)d_option_env_put(&child,
                           "title=AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
    (void)d_option_env_put(&child,
                           "language.entry="
                           "BBBBBCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");
    (void)d_option_env_put(&child, "title=short_one");
    D_TESTS_OPTION_ENV_CHECK(child.text_used == 92u);

    const uint32_t generation = d_option_env_generation(&child);

    D_TESTS_OPTION_ENV_CHECK(d_option_env_set_text_copy(
        &child, 2u, D_TESTS_ENTRY,
        d_option_env_text(&child, 1u, D_TESTS_ENTRY) + 5, 35u).is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_generation(&child) != generation);
    D_TESTS_OPTION_ENV_CHECK(strcmp(d_option_env_text(&child, 2u,
                                                      D_TESTS_ENTRY),
                                    "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC")
                             == 0);
    D_TESTS_OPTION_ENV_CHECK(strcmp(d_option_env_text(&child, 1u,
                                                      D_TESTS_ENTRY),
                                    "BBBBBCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC")
                             == 0);
    D_TESTS_OPTION_ENV_CHECK(strcmp(d_option_env_text(&child, 0u,
                                                      D_TESTS_TITLE),
                                    "short_one") == 0);

    // a source that died with its binding is refused, not read
    const char* const dead = d_option_env_text(&child, 2u, D_TESTS_ENTRY);

    (void)d_option_env_unset(&child, 2u, D_TESTS_ENTRY);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_set_text_copy(&child, 0u, D_TESTS_TITLE, dead, 35u))
        == (int32_t)D_OPTION_STATUS_BUFFER_TOO_SMALL);

    // too big even compacted
    memset(big, 'x', sizeof(big));
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_set_text_copy(&child, 0u, D_TESTS_TITLE, big,
                                   sizeof(big)))
        == (int32_t)D_OPTION_STATUS_BUFFER_TOO_SMALL);
    D_TESTS_OPTION_ENV_CHECK(strcmp(d_option_env_text(&child, 0u,
                                                      D_TESTS_TITLE),
                                    "short_one") == 0);

    return;
}

// one binding in the model: whether it is bound, and to what
struct dj_tests_option_env_slot
{
    int     bound;
    int64_t integer;
    double  real;
    char    text[48];
};

// D_TESTS_SOURCES
//   constant: the sources a binding has, PROGRAM to OVERRIDE, and one more
// for the model's index 0, which DEFAULT holds and nothing binds.
#define D_TESTS_SOURCES  6

D_OPTION_ENV_DECLARE(dj_tests_random_root_storage, 48, 16, 512);
D_OPTION_ENV_DECLARE(dj_tests_random_child_storage, 24, 0, 96);

/*
d_tests_option_env_expect
  What the model says a variable reads as in a scope: the highest source
first; in it the child before the parent, and in each the scoped binding
before the global one; the default when nothing binds it.
*/
static const struct dj_tests_option_env_slot*
d_tests_option_env_expect(
    struct dj_tests_option_env_slot _model[2][3][D_TESTS_VARS]
                                          [D_TESTS_SOURCES],
    uint32_t                        _scope,
    uint32_t                        _var,
    int*                            _source
)
{
    for (int source = D_TESTS_SOURCES - 1; source >= 1; --source)
    {
        for (int frame = 0; frame < 2; ++frame)
        {
            if ( (_scope != 0u) &&
                 (_model[frame][_scope][_var][source].bound != 0) )
            {
                *_source = source;

                return &_model[frame][_scope][_var][source];
            }

            if (_model[frame][0][_var][source].bound != 0)
            {
                *_source = source;

                return &_model[frame][0][_var][source];
            }
        }
    }

    *_source = 0;

    return NULL;
}

/*
d_tests_option_env_random_bind
  One binding from a source, by the variable's own route: a statement for
key_width, a copy for text, a value for the rest.
*/
static struct dj_option_result
d_tests_option_env_random_bind(
    struct dj_option_env*            _frame,
    uint32_t                         _scope,
    uint32_t                         _var,
    int32_t                          _source,
    uint64_t                         _roll,
    struct dj_tests_option_env_slot* _next
)
{
    struct dj_option_env_value value;
    char                       statement[96];
    const char* const          scope = (_scope == 0u)
                                           ? ""
                                           : D_TESTS_SCOPES[_scope - 1u];
    const char* const          dot   = (_scope == 0u) ? "" : ".";

    memset(&value, 0, sizeof(value));
    value.kind = D_TESTS_SCHEMA[_var].kind;

    switch (_var)
    {
        case D_TESTS_KEY_WIDTH:
            _next->integer = (int64_t)(_roll % 1000u);
            snprintf(statement, sizeof(statement), "%s%skey_width=%d", scope,
                     dot, (int)_next->integer);

            return d_option_env_put_from(_frame, statement, _source);
        case D_TESTS_TITLE:
        case D_TESTS_ENTRY:
        {
            const int length = snprintf(_next->text, sizeof(_next->text),
                                        "%u-%.*s", (unsigned)(_roll % 997u),
                                        (int)(_roll % 20u),
                                        "abcdefghijklmnopqrstuvwxyz");

            snprintf(statement, sizeof(statement), "%s%s%s", scope, dot,
                     D_TESTS_SCHEMA[_var].name);

            return d_option_env_assign_from(_frame, statement,
                                            strlen(statement), _next->text,
                                            (size_t)length, _source);
        }
        case D_TESTS_RATIO:
            _next->real = (double)(_roll % 100u) / 4.0;
            value.real  = _next->real;
            break;
        case D_TESTS_STYLE:
            _next->integer = (int64_t)(_roll % 3u);
            value.integer  = _next->integer;
            break;
        case D_TESTS_SECTIONS:
            _next->integer = (int64_t)(_roll % 16u);
            value.integer  = _next->integer;
            break;
        default:
            _next->integer = (int64_t)(_roll % 2u);
            value.integer  = _next->integer;
            break;
    }

    return d_option_env_set_value(_frame, _scope, _var, _source, &value);
}

/*
d_tests_option_env_random_step
  One random change to one frame, from one source, made to the model too
when the frame took it. A frame may refuse for want of room -- the child has
fewer bindings and less arena than the model can ask of it -- and only for
that; the count of any other refusal is returned.
*/
static size_t
d_tests_option_env_random_step(
    struct dj_option_env*           _frames[2],
    struct dj_tests_option_env_slot _model[2][3][D_TESTS_VARS]
                                          [D_TESTS_SOURCES]
)
{
    const int      frame  = (int)(d_tests_option_env_next() % 2u);
    const uint32_t scope  = (uint32_t)(d_tests_option_env_next() % 3u);
    const uint32_t var    = (uint32_t)(d_tests_option_env_next() %
                                       D_TESTS_VARS);
    const int32_t  source = 1 + (int32_t)(d_tests_option_env_next() %
                                          (D_TESTS_SOURCES - 1u));
    struct dj_tests_option_env_slot* const slot =
        &_model[frame][scope][var][source];
    const uint64_t                         roll = d_tests_option_env_next();
    struct dj_tests_option_env_slot        next = *slot;

    // a quarter of the time, unbind this source; now and then, every one
    if ((roll % 4u) == 0u)
    {
        (void)d_option_env_unset_from(_frames[frame], scope, var, source);
        slot->bound = 0;

        return 0u;
    }

    if ((roll % 23u) == 1u)
    {
        (void)d_option_env_unset(_frames[frame], scope, var);

        for (int s = 0; s < D_TESTS_SOURCES; ++s)
        {
            _model[frame][scope][var][s].bound = 0;
        }

        return 0u;
    }

    const struct dj_option_result result =
        d_tests_option_env_random_bind(_frames[frame], scope, var, source,
                                       roll, &next);

    if (result.is_ok)
    {
        next.bound = 1;
        *slot      = next;

        return 0u;
    }

    return ( (result.payload.error ==
              (int32_t)D_OPTION_STATUS_CAPACITY) ||
             (result.payload.error ==
              (int32_t)D_OPTION_STATUS_VALUE_CAPACITY) ||
             (result.payload.error ==
              (int32_t)D_OPTION_STATUS_BUFFER_TOO_SMALL) ) ? 0u : 1u;
}

/*
d_tests_option_env_random_verify
  Every variable in every scope, read through the child, against the model:
its value, whether it is bound, and the source it came from.
*/
static size_t
d_tests_option_env_random_verify(
    const struct dj_option_env*     _child,
    struct dj_tests_option_env_slot _model[2][3][D_TESTS_VARS]
                                          [D_TESTS_SOURCES]
)
{
    size_t wrong = 0u;

    for (uint32_t scope = 0u; scope < 3u; ++scope)
    {
        for (uint32_t var = 0u; var < D_TESTS_VARS; ++var)
        {
            int                                          source   = 0;
            const struct dj_tests_option_env_slot* const expected =
                d_tests_option_env_expect(_model, scope, var, &source);
            const struct dj_option_env_var* const        decl =
                &D_TESTS_SCHEMA[var];
            struct dj_option_env_value                   value;
            int                                          ok;

            (void)d_option_env_get(_child, scope, var, &value);

            switch (decl->kind)
            {
                case D_OPTION_ENV_KIND_REAL:
                    ok = (value.real == ((expected != NULL) ? expected->real
                                                            : decl->real));
                    break;
                case D_OPTION_ENV_KIND_TEXT:
                {
                    const char* const want = (expected != NULL)
                                                 ? expected->text
                                                 : decl->text;

                    ok = ( (want == NULL)
                               ? (value.text == NULL)
                               : ( (value.text != NULL) &&
                                   (strcmp(value.text, want) == 0) ) );
                    break;
                }
                default:
                    ok = (value.integer == ((expected != NULL)
                                                ? expected->integer
                                                : decl->integer));
                    break;
            }

            if ( (ok == 0)                                       ||
                 (value.bound != ((expected != NULL) ? 1 : 0))   ||
                 (value.source != (int32_t)source) )
            {
                ++wrong;
            }
        }
    }

    return wrong;
}

/*
d_tests_option_env_random
  Tests the following:
  - 20000 random binds, rebinds and unbinds from every source over a parent
    and a child, with arenas and value blocks small enough to compact many
    times, each followed by a full read -- value, bound, source -- against
    the model
*/
static void
d_tests_option_env_random(void)
{
    static struct dj_tests_option_env_slot
                                         model[2][3][D_TESTS_VARS]
                                              [D_TESTS_SOURCES];
    struct dj_tests_random_root_storage  root_storage  =
                                             D_OPTION_ENV_STORAGE_INIT;
    struct dj_tests_random_child_storage child_storage =
                                             D_OPTION_ENV_STORAGE_INIT;
    struct dj_option_env                 root;
    struct dj_option_env                 child;
    struct dj_option_env*                frames[2];
    size_t                               wrong   = 0u;
    size_t                               first   = 0u;
    size_t                               refused = 0u;

    printf("  random bindings from every source against a model\n");

    (void)D_OPTION_ENV_VIEW(&root, root_storage);
    (void)d_option_env_declare_all(&root,
                                   D_TESTS_SCHEMA,
                                   (uint32_t)(sizeof(D_TESTS_SCHEMA) /
                                              sizeof(D_TESTS_SCHEMA[0])));
    (void)d_option_env_set_scopes(&root, D_TESTS_SCOPES, 2u);
    (void)D_OPTION_ENV_VIEW(&child, child_storage);
    (void)d_option_env_set_parent(&child, &root);
    frames[0] = &child;
    frames[1] = &root;
    memset(model, 0, sizeof(model));

    for (size_t i = 0u; i < 20000u; ++i)
    {
        refused += d_tests_option_env_random_step(frames, model);

        const size_t errors = d_tests_option_env_random_verify(&child, model);

        if ( (errors != 0u) &&
             (wrong == 0u) )
        {
            first = i;
        }

        wrong += errors;
    }

    if (wrong != 0u)
    {
        printf("    %lu wrong reads, the first at step %lu\n",
               (unsigned long)wrong, (unsigned long)first);
    }

    D_TESTS_OPTION_ENV_CHECK(wrong == 0u);
    D_TESTS_OPTION_ENV_CHECK(refused == 0u);
    D_TESTS_OPTION_ENV_CHECK(child.set.count <= 24u);

    return;
}

/*
d_tests_option_env_sources_unset
  Tests the following:
  - in one frame and one source, the scoped binding; across frames of one
    source, the inner frame's unscoped one before the outer's scoped one
  - unset takes every source's binding in the frame, and nothing else
  - DEFAULT and a source past OVERRIDE are refused; the sources' names
*/
static void
d_tests_option_env_sources_unset(
    struct dj_option_env*       _root,
    struct dj_option_env*       _child,
    struct dj_option_env_value* _value
)
{
    // in one frame and one source, the scoped binding; across frames of one
    // source, the inner frame's unscoped one before the outer's scoped one
    (void)d_option_env_set_int(_root, 1u, D_TESTS_SECTIONS, 0x01);
    (void)d_option_env_set_int(_root, 0u, D_TESTS_SECTIONS, 0x02);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(_child, 1u, D_TESTS_SECTIONS)
                             == 0x01);
    (void)d_option_env_set_int(_child, 0u, D_TESTS_SECTIONS, 0x04);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(_child, 1u, D_TESTS_SECTIONS)
                             == 0x04);

    // unset takes every source's binding in the frame, and nothing else
    (void)d_option_env_unset(_root, 0u, D_TESTS_KEY_WIDTH);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(_child, 0u, D_TESTS_KEY_WIDTH)
                             == 50);
    (void)d_option_env_unset(_child, 0u, D_TESTS_KEY_WIDTH);
    D_TESTS_OPTION_ENV_CHECK( (d_option_env_get(_child, 0u,
                                                D_TESTS_KEY_WIDTH,
                                                _value).is_ok)          &&
                              (_value->integer == 24)                     &&
                              (_value->bound == 0)                        &&
                              (_value->source ==
                               D_OPTION_ENV_SOURCE_DEFAULT)             &&
                              (_value->depth == D_OPTION_ENV_NONE) );

    // what no binding can come from
    _value->kind    = D_OPTION_ENV_KIND_INT;
    _value->integer = 1;
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_set_value(_child, 0u, D_TESTS_KEY_WIDTH,
                               D_OPTION_ENV_SOURCE_DEFAULT, _value))
        == (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_put_from(_child, "key_width=1", D_OPTION_ENV_SOURCES))
        == (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    D_TESTS_OPTION_ENV_CHECK(strcmp(d_option_env_source_name(
                                        D_OPTION_ENV_SOURCE_COMMAND_LINE),
                                    "command line") == 0);
    D_TESTS_OPTION_ENV_CHECK(strcmp(d_option_env_source_name(9), "unknown")
                             == 0);

    return;
}

/*
d_tests_option_env_sources
  Tests the following:
  - the source of the highest standing holds, whatever order the sources
    were applied in; among equals the inner frame, and in a frame the
    scoped binding
  - a source's binding removed gives back the next one's; unset removes
    every source's; DEFAULT and a source past OVERRIDE are refused
  - the value says its source and the binding frame's depth; the layers
    list every binding in the order they would take over
*/
static void
d_tests_option_env_sources(void)
{
    struct dj_tests_root_storage  root_storage  = D_OPTION_ENV_STORAGE_INIT;
    struct dj_tests_child_storage child_storage = D_OPTION_ENV_STORAGE_INIT;
    struct dj_option_env          root;
    struct dj_option_env          child;
    struct dj_option_env_value    value;
    struct dj_option_env_layer    layers[8];

    printf("  sources and standing\n");

    d_tests_option_env_root(&root, &root_storage);
    (void)D_OPTION_ENV_VIEW(&child, child_storage);
    (void)d_option_env_set_parent(&child, &root);

    // the command line first, the environment and a file after: it holds
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put_from(&root, "key_width=10",
        D_OPTION_ENV_SOURCE_COMMAND_LINE).is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put_from(&root, "key_width=20",
        D_OPTION_ENV_SOURCE_ENVIRONMENT).is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put_from(&root, "key_width=30",
        D_OPTION_ENV_SOURCE_FILE).is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_set_int(&root, 0u,
                                                  D_TESTS_KEY_WIDTH, 40)
                             .is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&child, 0u, D_TESTS_KEY_WIDTH)
                             == 10);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_source(&child, 0u,
                                                 D_TESTS_KEY_WIDTH)
                             == D_OPTION_ENV_SOURCE_COMMAND_LINE);

    // the child's program binding does not beat the parent's command line
    (void)d_option_env_set_int(&child, 0u, D_TESTS_KEY_WIDTH, 50);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&child, 0u, D_TESTS_KEY_WIDTH)
                             == 10);

    // an override beats it; removing the override gives it back
    (void)d_option_env_put_from(&child, "key_width=60",
                                D_OPTION_ENV_SOURCE_OVERRIDE);
    D_TESTS_OPTION_ENV_CHECK( (d_option_env_get(&child, 0u,
                                                D_TESTS_KEY_WIDTH,
                                                &value).is_ok)          &&
                              (value.integer == 60)                     &&
                              (value.source ==
                               D_OPTION_ENV_SOURCE_OVERRIDE)            &&
                              (value.depth == 1u) );
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put_from(&child, "-key_width",
        D_OPTION_ENV_SOURCE_OVERRIDE).is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&child, 0u, D_TESTS_KEY_WIDTH)
                             == 10);

    // the layers, the one in force first
    D_TESTS_OPTION_ENV_CHECK(d_option_env_layers(&child, 0u,
                                                 D_TESTS_KEY_WIDTH, layers,
                                                 8u) == 5u);
    D_TESTS_OPTION_ENV_CHECK( (layers[0].shown == 1)                      &&
                              (layers[0].value.integer == 10)             &&
                              (layers[1].value.source ==
                               D_OPTION_ENV_SOURCE_ENVIRONMENT)           &&
                              (layers[2].value.source ==
                               D_OPTION_ENV_SOURCE_FILE)                  &&
                              (layers[3].frame == &child)                 &&
                              (layers[3].value.integer == 50)             &&
                              (layers[4].frame == &root)                  &&
                              (layers[4].value.integer == 40)             &&
                              (layers[4].shown == 0) );
    D_TESTS_OPTION_ENV_CHECK(d_option_env_layers(&child, 0u,
                                                 D_TESTS_KEY_WIDTH, layers,
                                                 2u) == 5u);
    D_TESTS_OPTION_ENV_CHECK(layers[1].value.integer == 20);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_layers(&child, 0u, 99u, NULL, 0u)
                             == (size_t)-1);

    // the command line's removed: the environment's holds
    (void)d_option_env_unset_from(&root, 0u, D_TESTS_KEY_WIDTH,
                                  D_OPTION_ENV_SOURCE_COMMAND_LINE);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&child, 0u, D_TESTS_KEY_WIDTH)
                             == 20);

    d_tests_option_env_sources_unset(&root, &child, &value);

    return;
}

/*
d_tests_option_env_deepest
  Tests the following:
  - the deepest chain allowed, and one frame past it
*/
static void
d_tests_option_env_deepest(void)
{
    D_OPTION_ENV_DECLARE(dj_tests_link_storage, 1, 0, 0);
    static struct dj_tests_link_storage links[D_OPTION_ENV_DEPTH_MAX + 2u];
    static struct dj_option_env         chain[D_OPTION_ENV_DEPTH_MAX + 2u];
    const uint32_t                      past = D_OPTION_ENV_DEPTH_MAX + 1u;

    for (uint32_t i = 0u; i <= past; ++i)
    {
        (void)D_OPTION_ENV_VIEW(&chain[i], links[i]);

        // a chain as deep as allowed; the last frame is left a root
        if ( (i > 0u) &&
             (i < past) )
        {
            (void)d_option_env_set_parent(&chain[i], &chain[i - 1u]);
        }
    }

    D_TESTS_OPTION_ENV_CHECK(chain[D_OPTION_ENV_DEPTH_MAX].depth ==
                             D_OPTION_ENV_DEPTH_MAX);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_set_parent(&chain[past], &chain[D_OPTION_ENV_DEPTH_MAX]))
        == (int32_t)D_OPTION_STATUS_CAPACITY);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_root(&chain[D_OPTION_ENV_DEPTH_MAX])
                             == &chain[0]);

    return;
}

/*
d_tests_option_env_layered
  Tests the following:
  - a child declares variables and scopes of its own: their ids carry its
    depth, the root's are plain indices, and a name is found innermost
    first, an inner declaration hiding an outer one
  - the k-th visible variable, root first, and back; the declaring frame
  - a frame with declarations re-parented: its ids move with its depth, its
    bindings go, and its generation passes any it had
  - a frame's own name may lead a name it resolves
*/
static void
d_tests_option_env_layered(void)
{
    static const struct dj_option_env_var MINE[] =
    {
        D_OPTION_ENV_INT("depth_only", 7, "the child's own"),
        D_OPTION_ENV_TEXT("title", "inner", "hides the root's")
    };
    static const char* const              PARTS[] = { "phase" };
    D_OPTION_ENV_DECLARE(dj_tests_layer_storage, 16, 4, 64);
    struct dj_tests_root_storage  root_storage = D_OPTION_ENV_STORAGE_INIT;
    struct dj_tests_layer_storage mine_storage = D_OPTION_ENV_STORAGE_INIT;
    struct dj_option_env          root;
    struct dj_option_env          mine;

    printf("  frames that declare\n");

    d_tests_option_env_root(&root, &root_storage);
    (void)D_OPTION_ENV_VIEW(&mine, mine_storage);
    (void)d_option_env_set_parent(&mine, &root);

    const struct dj_option_result first =
        d_option_env_declare_all(&mine, MINE, 2u);
    const uint32_t depth_only = first.payload.value;
    const uint32_t title      = depth_only + 1u;

    D_TESTS_OPTION_ENV_CHECK( (first.is_ok) &&
                              (depth_only == (1u << 20)) );
    D_TESTS_OPTION_ENV_CHECK(d_option_env_set_scopes(&mine, PARTS, 1u)
                             .payload.value == ((1u << 20) | 1u));
    D_TESTS_OPTION_ENV_CHECK(d_option_env_find(&mine, "title", 5u)
                             .payload.value == title);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_find(&root, "title", 5u)
                             .payload.value == D_TESTS_TITLE);
    D_TESTS_OPTION_ENV_CHECK(strcmp(d_option_env_text(&mine, 0u, title),
                                    "inner") == 0);
    D_TESTS_OPTION_ENV_CHECK(strcmp(d_option_env_text(&mine, 0u,
                                                      D_TESTS_TITLE),
                                    "machine") == 0);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&root, 0u, depth_only) == 0);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&mine,
                                              "phase.depth_only=8").is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&mine, (1u << 20) | 1u,
                                              depth_only) == 8);
    D_TESTS_OPTION_ENV_CHECK(strcmp(d_option_env_scope_name(&mine,
                                        (1u << 20) | 1u), "phase") == 0);
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_put(&root,
                                               "phase.key_width=1").is_ok);

    // the visible variables, root first
    D_TESTS_OPTION_ENV_CHECK(d_option_env_var_count(&mine) ==
                             D_TESTS_VARS + 2u);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_var_id(&mine, 0u) == 0u);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_var_id(&mine, D_TESTS_VARS)
                             == depth_only);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_var_id(&mine, D_TESTS_VARS + 2u)
                             == D_OPTION_ENV_NONE);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_var_index(&mine, title)
                             == D_TESTS_VARS + 1u);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_var_index(&root, title)
                             == D_OPTION_ENV_NONE);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_var_frame(&mine, title) == &mine);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_var_frame(&mine, D_TESTS_STYLE)
                             == &root);

    // a frame's own name leads its names
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_set_name(&mine, "no.dots").is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_set_name(&mine, "djin").is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&mine,
                                              "djin.depth_only=9").is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&mine,
                                              "djin.language.key_width=3")
                             .is_ok);
    D_TESTS_OPTION_ENV_CHECK( (d_option_env_int(&mine, 0u, depth_only) == 9) &&
                              (d_option_env_int(&mine, 1u, D_TESTS_KEY_WIDTH)
                               == 3) );

    // re-parented: its own ids move with its depth, its bindings go
    const uint32_t before = d_option_env_generation(&mine);

    d_option_env_detach(&mine);
    D_TESTS_OPTION_ENV_CHECK( (mine.depth == 0u) &&
                              (mine.set.count == 0u) );
    D_TESTS_OPTION_ENV_CHECK(d_option_env_generation(&mine) > before);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_find(&mine, "depth_only", 10u)
                             .payload.value == 0u);
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_find(&mine, "style", 5u).is_ok);

    d_tests_option_env_deepest();

    return;
}

// FORMATS, LATER, BITS
//   constants: the rows test's names: a choice's, the values ruled but not
// built, a flag's.
static const struct dj_option_env_name FORMATS[] =
{
    { "text", 0 },
    { "json", 1 }
};

static const struct dj_option_env_name LATER[] =
{
    { "yaml", 2 },
    { "toml", 8 }
};

static const struct dj_option_env_name BITS[] =
{
    { "a", 1 },
    { "b", 2 }
};

/*
d_tests_option_env_rows
  Tests the following:
  - an INT's range, by value and by text, at its ends and past them
  - an alias finds the variable, binds it, and is read from getenv when the
    name is not set
  - a value ruled but not built is refused by name -- a choice's, a flag's
    -- and by value, as POLICY_REJECTED, where an unknown one is
    INVALID_ARGUMENT
  - a row whose default is a knob names the knob, and reads as the default
*/
static void
d_tests_option_env_rows(void)
{
#define D_TESTS_KNOB_WIDTH 33
    static const struct dj_option_env_var ROWS[] =
    {
        D_OPTION_ENV_INT_RANGE("depth", 5, 1, 9, "between 1 and 9"),
        D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_INT, "columns", NULL, 0u, NULL,
                         0u, NULL, 80, INT64_MIN, INT64_MAX, 0.0, "width",
                         NULL, "renamed from width"),
        D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_CHOICE, "format", FORMATS, 2u,
                         LATER, 1u, NULL, 0, INT64_MIN, INT64_MAX, 0.0, NULL,
                         NULL, "yaml is ruled, not built"),
        D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_FLAGS, "bits", BITS, 2u, LATER,
                         2u, NULL, 1, INT64_MIN, INT64_MAX, 0.0, NULL, NULL,
                         "toml's bit is ruled, not built"),
        D_OPTION_ENV_INT_KNOB("knob_width", D_TESTS_KNOB_WIDTH,
                              "from a knob")
    };
#undef D_TESTS_KNOB_WIDTH
    struct dj_tests_root_storage storage = D_OPTION_ENV_STORAGE_INIT;
    struct dj_option_env         env;
    struct dj_option_env_value   value;

    printf("  rows: ranges, aliases, unbuilt values, knobs\n");

    (void)D_OPTION_ENV_VIEW(&env, storage);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_declare_all(&env, ROWS, 5u).is_ok);

    // the range
    D_TESTS_OPTION_ENV_CHECK(d_option_env_set_int(&env, 0u, 0u, 9).is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_set_int(&env, 0u, 0u, 1).is_ok);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_set_int(&env, 0u, 0u, 10))
        == (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_put(&env, "depth=0"))
        == (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&env, 0u, 0u) == 1);

    // the alias
    D_TESTS_OPTION_ENV_CHECK(d_option_env_find(&env, "width", 5u)
                             .payload.value == 1u);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&env, "width=100").is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&env, 0u, 1u) == 100);
#if defined(_WIN32)
    (void)_putenv_s("DJROWS_WIDTH", "120");
#else
    (void)setenv("DJROWS_WIDTH", "120", 1);
#endif
    D_TESTS_OPTION_ENV_CHECK(d_option_env_import(&env, "DJROWS_")
                             .payload.value == 1u);
    D_TESTS_OPTION_ENV_CHECK( (d_option_env_get(&env, 0u, 1u, &value).is_ok) &&
                              (value.integer == 120)                       &&
                              (value.source ==
                               D_OPTION_ENV_SOURCE_ENVIRONMENT) );

    // ruled, not built: refused by name and by value, unlike the unknown
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_put(&env, "format=YAML"))
        == (int32_t)D_OPTION_STATUS_POLICY_REJECTED);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_set_int(&env, 0u, 2u, 2))
        == (int32_t)D_OPTION_STATUS_POLICY_REJECTED);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_put(&env, "format=xml"))
        == (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&env, "format=json").is_ok);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_put(&env, "bits=a,toml"))
        == (int32_t)D_OPTION_STATUS_POLICY_REJECTED);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_set_int(&env, 0u, 3u, 8))
        == (int32_t)D_OPTION_STATUS_POLICY_REJECTED);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_set_int(&env, 0u, 3u, 16))
        == (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&env, "bits=a|b").is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&env, 0u, 3u) == 3);

    // the knob: its value is the default, its name is kept
    D_TESTS_OPTION_ENV_CHECK( (d_option_env_get(&env, 0u, 4u, &value).is_ok) &&
                              (value.integer == 33)                        &&
                              (value.source == D_OPTION_ENV_SOURCE_DEFAULT) );
    D_TESTS_OPTION_ENV_CHECK(strcmp(d_option_env_var_at(&env, 4u)->knob,
                                    "D_TESTS_KNOB_WIDTH") == 0);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_var_at(&env, 0u)->knob == NULL);

    return;
}

// DEPTHS, NEVER
//   constants: an int's named value, outside its range, as lane F's depth
// guard has it; and an int's value ruled but not built.
static const struct dj_option_env_name DEPTHS[] =
{
    { "unbounded", (int64_t)0xFFFFFFFFu }
};

static const struct dj_option_env_name NEVER[] =
{
    { "auto", -1 }
};

// D_TESTS_NAMED_ROWS, D_TESTS_NAMED_BAD
//   constants: the rows the named-value tests declare -- lane F's depth
// guard, one whose knob is the name, one with an unbuilt name -- and two rows
// that cannot be declared.
#define D_TESTS_KNOB_DEPTH     1000
#define D_TESTS_KNOB_UNBOUNDED 0xFFFFFFFFu
static const struct dj_option_env_var D_TESTS_NAMED_ROWS[] =
{
    D_OPTION_ENV_INT_RANGE_KNOB_NAMED("max_depth", D_TESTS_KNOB_DEPTH, 1,
                                      65535, DEPTHS, "or unbounded"),
    D_OPTION_ENV_INT_RANGE_KNOB_NAMED("open_depth", D_TESTS_KNOB_UNBOUNDED,
                                      1, 65535, DEPTHS,
                                      "a knob that is a name"),
    D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_INT, "ruled_depth", DEPTHS, 1u, NEVER,
                     1u, NULL, 5, 1, 9, 0.0, NULL, NULL,
                     "auto is ruled, not built")
};
#undef D_TESTS_KNOB_DEPTH
#undef D_TESTS_KNOB_UNBOUNDED

static const struct dj_option_env_var D_TESTS_NAMED_BAD[] =
{
    D_OPTION_ENV_ROW(D_OPTION_ENV_KIND_INT, "no_names", NULL, 1u, NULL, 0u,
                     NULL, 5, 1, 9, 0.0, NULL, NULL, NULL),
    D_OPTION_ENV_INT_RANGE_NAMED("outside", 0, 1, 9, DEPTHS, NULL)
};

/*
d_tests_option_env_named_ints
  Tests the following:
  - an INT's named value is read by name, in any case, or by its number,
    outside the range, from text and by value
  - it is written back by its name, and d_option_env_text gives it; a value
    with no name has no text and is written as a number
  - the range still holds for every other value
*/
static void
d_tests_option_env_named_ints(void)
{
    const int64_t                unbounded = (int64_t)0xFFFFFFFFu;
    struct dj_tests_root_storage storage   = D_OPTION_ENV_STORAGE_INIT;
    struct dj_option_env         env;
    char                         text[32];

    printf("  named int values: text and values\n");

    (void)D_OPTION_ENV_VIEW(&env, storage);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_declare_all(&env,
                                                      D_TESTS_NAMED_ROWS,
                                                      3u).is_ok);

    // the default, then the name, in any case, written back by name
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&env, 0u, 0u) == 1000);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_text(&env, 0u, 0u) == NULL);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&env, "max_depth=UnBounded")
                             .is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&env, 0u, 0u) == unbounded);
    (void)d_option_env_format(&env, 0u, 0u, text, sizeof(text));
    D_TESTS_OPTION_ENV_CHECK(strcmp(text, "unbounded") == 0);
    D_TESTS_OPTION_ENV_CHECK( (d_option_env_text(&env, 0u, 0u) != NULL) &&
                              (strcmp(d_option_env_text(&env, 0u, 0u),
                                      "unbounded") == 0) );

    // the range holds for every other value; the name's number is taken
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&env, "max_depth=65535").is_ok);
    (void)d_option_env_format(&env, 0u, 0u, text, sizeof(text));
    D_TESTS_OPTION_ENV_CHECK(strcmp(text, "65535") == 0);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_text(&env, 0u, 0u) == NULL);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_put(&env, "max_depth=65536"))
        == (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_put(&env, "max_depth=0"))
        == (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_put(&env, "max_depth=unbound"))
        == (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&env, "max_depth=4294967295")
                             .is_ok);
    (void)d_option_env_format(&env, 0u, 0u, text, sizeof(text));
    D_TESTS_OPTION_ENV_CHECK(strcmp(text, "unbounded") == 0);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_set_int(&env, 0u, 0u, 12).is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_set_int(&env, 0u, 0u, unbounded)
                             .is_ok);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_set_int(&env, 0u, 0u, 70000))
        == (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);

    return;
}

/*
d_tests_option_env_named_rules
  Tests the following:
  - a named value read from the environment holds over the program's
    binding, and carries its name
  - a knob may be the named value, and the row's default reads as it
  - an INT's unbuilt name is refused by name and by value
  - a row naming no array of names, or with a default it does not accept,
    is not declared
*/
static void
d_tests_option_env_named_rules(void)
{
    const int64_t                unbounded = (int64_t)0xFFFFFFFFu;
    struct dj_tests_root_storage storage   = D_OPTION_ENV_STORAGE_INIT;
    struct dj_option_env         env;
    struct dj_option_env_value   value;
    char                         text[32];

    printf("  named int values: sources, knobs, rules\n");

    (void)D_OPTION_ENV_VIEW(&env, storage);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_declare_all(&env,
                                                      D_TESTS_NAMED_ROWS,
                                                      3u).is_ok);

    // from the environment, over the program's binding
#if defined(_WIN32)
    (void)_putenv_s("DJNAMED_MAX_DEPTH", "unbounded");
#else
    (void)setenv("DJNAMED_MAX_DEPTH", "unbounded", 1);
#endif
    (void)d_option_env_set_int(&env, 0u, 0u, 12);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_import(&env, "DJNAMED_")
                             .payload.value == 1u);
    D_TESTS_OPTION_ENV_CHECK( (d_option_env_get(&env, 0u, 0u, &value).is_ok) &&
                              (value.integer == unbounded)                 &&
                              (value.text != NULL)                         &&
                              (strcmp(value.text, "unbounded") == 0)       &&
                              (value.source ==
                               D_OPTION_ENV_SOURCE_ENVIRONMENT) );

    // a knob that is the name: the default reads as it, and keeps the knob
    D_TESTS_OPTION_ENV_CHECK( (d_option_env_get(&env, 0u, 1u, &value).is_ok) &&
                              (value.integer == unbounded)                 &&
                              (value.source == D_OPTION_ENV_SOURCE_DEFAULT) );
    (void)d_option_env_format(&env, 0u, 1u, text, sizeof(text));
    D_TESTS_OPTION_ENV_CHECK(strcmp(text, "unbounded") == 0);
    D_TESTS_OPTION_ENV_CHECK( (d_option_env_var_at(&env, 1u) != NULL) &&
                              (strcmp(d_option_env_var_at(&env, 1u)->knob,
                                      "D_TESTS_KNOB_UNBOUNDED") == 0) );

    // ruled, not built: refused by name and by value, unlike the unknown
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_put(&env, "ruled_depth=AUTO"))
        == (int32_t)D_OPTION_STATUS_POLICY_REJECTED);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_set_int(&env, 0u, 2u, -1))
        == (int32_t)D_OPTION_STATUS_POLICY_REJECTED);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_set_int(&env, 0u, 2u, -2))
        == (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put(&env, "ruled_depth=unbounded")
                             .is_ok);

    // rows that cannot be declared
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_declare(&env,
                                                   &D_TESTS_NAMED_BAD[0])
                             .is_ok);
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_declare(&env,
                                                   &D_TESTS_NAMED_BAD[1])
                             .is_ok);

    return;
}

// D_TESTS_TRIP_ROWS
//   constant: a row of every kind, for writing what loads back.
static const struct dj_option_env_var D_TESTS_TRIP_ROWS[] =
{
    D_OPTION_ENV_BOOL("flag", 0, NULL),
    D_OPTION_ENV_INT("count", 0, NULL),
    D_OPTION_ENV_INT_RANGE_NAMED("depth", 1, 1, 9, DEPTHS, NULL),
    D_OPTION_ENV_REAL("ratio", 0.0, NULL),
    D_OPTION_ENV_TEXT("title", NULL, NULL),
    D_OPTION_ENV_CHOICE("format", FORMATS, 0, NULL),
    D_OPTION_ENV_FLAGS("bits", BITS, 0, NULL)
};

/*
d_tests_option_env_trip
  Writes a variable's value as d_option_env_format_quoted does, loads the
  statement into a second frame as a script, as a settings file is read,
  and says whether the same value came back: a real's bits, a text's bytes,
  the others' integers.
*/
static bool
d_tests_option_env_trip(
    const struct dj_option_env* _from,
    struct dj_option_env*       _to,
    uint32_t                    _var
)
{
    char                       statement[160];
    struct dj_option_env_value want;
    struct dj_option_env_value got;
    const char* const          name = d_option_env_var_at(_from, _var)->name;
    const size_t               size = strlen(name);
    const size_t               room = sizeof(statement) - size - 1u;

    memcpy(statement, name, size);
    statement[size] = '=';

    if ( (d_option_env_format_quoted(_from, 0u, _var, statement + size + 1u,
                                     room) >= room)                     ||
         (!d_option_env_load(_to, statement, NULL).is_ok)               ||
         (!d_option_env_get(_from, 0u, _var, &want).is_ok)              ||
         (!d_option_env_get(_to, 0u, _var, &got).is_ok) )
    {
        return false;
    }

    switch (want.kind)
    {
        case D_OPTION_ENV_KIND_REAL:
            return (memcmp(&want.real, &got.real, sizeof(double)) == 0);
        case D_OPTION_ENV_KIND_TEXT:
            return ( (want.text != NULL) &&
                     (got.text != NULL)  &&
                     (strcmp(want.text, got.text) == 0) );
        default:
            return (want.integer == got.integer);
    }
}

/*
d_tests_option_env_round_trip
  Tests the following:
  - what d_option_env_format_quoted writes loads back as the same value for
    every kind but TEXT: a bool, an int at both ends of int64_t, a named
    int, reals to the last bit, a choice, flags, and flags of none
*/
static void
d_tests_option_env_round_trip(void)
{
    static const double REALS[] = { 0.1, -2.5e-300, 1e300,
                                    123456789.123456789, -0.0 };
    struct dj_tests_root_storage from_storage = D_OPTION_ENV_STORAGE_INIT;
    struct dj_tests_root_storage to_storage   = D_OPTION_ENV_STORAGE_INIT;
    struct dj_option_env         from;
    struct dj_option_env         to;

    printf("  writing what loads back: every kind\n");

    (void)D_OPTION_ENV_VIEW(&from, from_storage);
    (void)D_OPTION_ENV_VIEW(&to, to_storage);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_declare_all(&from,
                                                      D_TESTS_TRIP_ROWS,
                                                      7u).is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_declare_all(&to, D_TESTS_TRIP_ROWS,
                                                      7u).is_ok);

    (void)d_option_env_set_bool(&from, 0u, 0u, true);
    D_TESTS_OPTION_ENV_CHECK(d_tests_option_env_trip(&from, &to, 0u));
    (void)d_option_env_set_int(&from, 0u, 1u, INT64_MIN);
    D_TESTS_OPTION_ENV_CHECK(d_tests_option_env_trip(&from, &to, 1u));
    (void)d_option_env_set_int(&from, 0u, 1u, INT64_MAX);
    D_TESTS_OPTION_ENV_CHECK(d_tests_option_env_trip(&from, &to, 1u));
    (void)d_option_env_put(&from, "depth=unbounded");
    D_TESTS_OPTION_ENV_CHECK(d_tests_option_env_trip(&from, &to, 2u));
    (void)d_option_env_set_int(&from, 0u, 2u, 7);
    D_TESTS_OPTION_ENV_CHECK(d_tests_option_env_trip(&from, &to, 2u));

    for (size_t i = 0u; i < sizeof(REALS) / sizeof(REALS[0]); ++i)
    {
        (void)d_option_env_set_real(&from, 0u, 3u, REALS[i]);
        D_TESTS_OPTION_ENV_CHECK(d_tests_option_env_trip(&from, &to, 3u));
    }

    (void)d_option_env_put(&from, "format=json");
    D_TESTS_OPTION_ENV_CHECK(d_tests_option_env_trip(&from, &to, 5u));
    (void)d_option_env_put(&from, "bits=a|b");
    D_TESTS_OPTION_ENV_CHECK(d_tests_option_env_trip(&from, &to, 6u));
    (void)d_option_env_set_int(&from, 0u, 6u, 0);
    D_TESTS_OPTION_ENV_CHECK(d_tests_option_env_trip(&from, &to, 6u));

    return;
}

/*
d_tests_option_env_round_trip_text
  Tests the following:
  - text loads back as it was whatever it holds: spaces inside or at an end,
    nothing, '#', ';', quotes, backslashes, line breaks, tabs, a control
    character, bytes past ASCII, a leading '=' or '-'
  - quotes only where they are needed: plain text, and text with a space
    inside, are written as d_option_env_format writes them
  - a null text writes nothing; a statement may have space around its '='
*/
static void
d_tests_option_env_round_trip_text(void)
{
    static const char* const TEXTS[] =
    {
        "plain",       "two words",  " leading",      "trailing ", "",
        "a # b",       "a; b",       "say \"hi\"",    "back\\slash",
        "line\nbreak", "tab\tin",    "\x01" "ctl",    "\xC3\xBC" "ber",
        "=equals",     "-dash"
    };
    struct dj_tests_root_storage from_storage = D_OPTION_ENV_STORAGE_INIT;
    struct dj_tests_root_storage to_storage   = D_OPTION_ENV_STORAGE_INIT;
    struct dj_option_env         from;
    struct dj_option_env         to;
    char                         text[64];

    printf("  writing what loads back: text\n");

    (void)D_OPTION_ENV_VIEW(&from, from_storage);
    (void)D_OPTION_ENV_VIEW(&to, to_storage);
    (void)d_option_env_declare_all(&from, D_TESTS_TRIP_ROWS, 7u);
    (void)d_option_env_declare_all(&to, D_TESTS_TRIP_ROWS, 7u);

    for (size_t i = 0u; i < sizeof(TEXTS) / sizeof(TEXTS[0]); ++i)
    {
        (void)d_option_env_set_text(&from, 0u, 4u, TEXTS[i]);
        D_TESTS_OPTION_ENV_CHECK(d_tests_option_env_trip(&from, &to, 4u));
    }

    // quotes only where they are needed
    (void)d_option_env_set_text(&from, 0u, 4u, "two words");
    (void)d_option_env_format_quoted(&from, 0u, 4u, text, sizeof(text));
    D_TESTS_OPTION_ENV_CHECK(strcmp(text, "two words") == 0);
    (void)d_option_env_set_text(&from, 0u, 4u, "say \"hi\"\n");
    (void)d_option_env_format_quoted(&from, 0u, 4u, text, sizeof(text));
    D_TESTS_OPTION_ENV_CHECK(strcmp(text, "\"say \\\"hi\\\"\\n\"") == 0);
    (void)d_option_env_format(&from, 0u, 4u, text, sizeof(text));
    D_TESTS_OPTION_ENV_CHECK(strcmp(text, "say \"hi\"\n") == 0);

    // a null text writes nothing; space around '='
    (void)d_option_env_unset(&from, 0u, 4u);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_format_quoted(&from, 0u, 4u, text,
                                                        sizeof(text)) == 0u);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_load(&to, "title = \" padded \"",
                                               NULL).is_ok);
    D_TESTS_OPTION_ENV_CHECK( (d_option_env_text(&to, 0u, 4u) != NULL) &&
                              (strcmp(d_option_env_text(&to, 0u, 4u),
                                      " padded ") == 0) );
    D_TESTS_OPTION_ENV_CHECK(d_option_env_load(&to, "count = 5", NULL).is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_int(&to, 0u, 1u) == 5);

    return;
}

// D_TESTS_REFUSING_ROWS, D_TESTS_REFUSING_BAD
//   constants: a row that refuses the environment, as lane G's plugin search
// does when D_CFG_CLI_PLUGIN_VARIABLE is 0; one that refuses a file and the
// command line; and two rows that refuse what no row may.
static const struct dj_option_env_var D_TESTS_REFUSING_ROWS[] =
{
    D_OPTION_ENV_ROW_REFUSING(D_OPTION_ENV_KIND_TEXT, "plugin_search", NULL,
                              0u, NULL, 0u, "/usr/lib", 0, INT64_MIN,
                              INT64_MAX, 0.0, NULL, NULL, "where code loads",
                              D_OPTION_ENV_REFUSES(
                                  D_OPTION_ENV_SOURCE_ENVIRONMENT)),
    D_OPTION_ENV_ROW_REFUSING(D_OPTION_ENV_KIND_BOOL, "strict", NULL, 0u,
                              NULL, 0u, NULL, 1, INT64_MIN, INT64_MAX, 0.0,
                              NULL, NULL, NULL,
                              D_OPTION_ENV_REFUSES(D_OPTION_ENV_SOURCE_FILE) |
                              D_OPTION_ENV_REFUSES(
                                  D_OPTION_ENV_SOURCE_COMMAND_LINE))
};

static const struct dj_option_env_var D_TESTS_REFUSING_BAD[] =
{
    D_OPTION_ENV_ROW_REFUSING(D_OPTION_ENV_KIND_INT, "no_default", NULL, 0u,
                              NULL, 0u, NULL, 0, INT64_MIN, INT64_MAX, 0.0,
                              NULL, NULL, NULL,
                              D_OPTION_ENV_REFUSES(
                                  D_OPTION_ENV_SOURCE_DEFAULT)),
    D_OPTION_ENV_ROW_REFUSING(D_OPTION_ENV_KIND_INT, "no_source", NULL, 0u,
                              NULL, 0u, NULL, 0, INT64_MIN, INT64_MAX, 0.0,
                              NULL, NULL, NULL, D_OPTION_ENV_REFUSES(6))
};

/*
d_tests_option_env_refused
  Tests the following:
  - a source a row refuses fails by name, with POLICY_REJECTED, from the
    environment, a file, a statement and the command line alike, and leaves
    the value as it was
  - every other source still binds, an override among them
  - import reports the refusal, and reads the rest
  - a row that refuses DEFAULT, or a source past OVERRIDE, is not declared
*/
static void
d_tests_option_env_refused(void)
{
    struct dj_tests_root_storage storage = D_OPTION_ENV_STORAGE_INIT;
    struct dj_option_env         env;
    struct dj_option_result      result;

    printf("  sources a row refuses\n");

    (void)D_OPTION_ENV_VIEW(&env, storage);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_declare_all(&env,
                                                      D_TESTS_REFUSING_ROWS,
                                                      2u).is_ok);

    // the environment is refused, and import says so, reading the rest
#if defined(_WIN32)
    (void)_putenv_s("DJREFUSE_PLUGIN_SEARCH", "/tmp/elsewhere");
    (void)_putenv_s("DJREFUSE_STRICT", "false");
#else
    (void)setenv("DJREFUSE_PLUGIN_SEARCH", "/tmp/elsewhere", 1);
    (void)setenv("DJREFUSE_STRICT", "false", 1);
#endif
    result = d_option_env_import(&env, "DJREFUSE_");
    D_TESTS_OPTION_ENV_CHECK( (!result.is_ok) &&
                              (result.payload.error ==
                               (int32_t)D_OPTION_STATUS_POLICY_REJECTED) );
    D_TESTS_OPTION_ENV_CHECK(strcmp(d_option_env_text(&env, 0u, 0u),
                                    "/usr/lib") == 0);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_source(&env, 0u, 0u) ==
                             D_OPTION_ENV_SOURCE_DEFAULT);
    D_TESTS_OPTION_ENV_CHECK( (!d_option_env_bool(&env, 0u, 1u)) &&
                              (d_option_env_source(&env, 0u, 1u) ==
                               D_OPTION_ENV_SOURCE_ENVIRONMENT) );

    // every other source binds plugin_search
    D_TESTS_OPTION_ENV_CHECK(d_option_env_load_from(&env,
                                                    "plugin_search=/opt/f",
                                                    D_OPTION_ENV_SOURCE_FILE,
                                                    NULL).is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put_from(&env, "plugin_search=/c",
                                 D_OPTION_ENV_SOURCE_COMMAND_LINE).is_ok);
    D_TESTS_OPTION_ENV_CHECK( (strcmp(d_option_env_text(&env, 0u, 0u),
                                      "/c") == 0) &&
                              (d_option_env_source(&env, 0u, 0u) ==
                               D_OPTION_ENV_SOURCE_COMMAND_LINE) );

    // strict refuses a file and the command line, by name, value untouched
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_load_from(&env, "strict=true", D_OPTION_ENV_SOURCE_FILE,
                               NULL))
        == (int32_t)D_OPTION_STATUS_POLICY_REJECTED);
    D_TESTS_OPTION_ENV_CHECK(D_TESTS_OPTION_ENV_STATUS(
        d_option_env_put_from(&env, "strict=true",
                              D_OPTION_ENV_SOURCE_COMMAND_LINE))
        == (int32_t)D_OPTION_STATUS_POLICY_REJECTED);
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_bool(&env, 0u, 1u));
    D_TESTS_OPTION_ENV_CHECK(d_option_env_put_from(&env, "strict=true",
                                 D_OPTION_ENV_SOURCE_OVERRIDE).is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_bool(&env, 0u, 1u));

    // what no row may refuse
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_declare(&env,
                                                   &D_TESTS_REFUSING_BAD[0])
                             .is_ok);
    D_TESTS_OPTION_ENV_CHECK(!d_option_env_declare(&env,
                                                   &D_TESTS_REFUSING_BAD[1])
                             .is_ok);

    return;
}

/*
d_tests_option_env_suggestions
  Tests the following:
  - did you mean: a swap, a letter missing, one too many, one wrong, case;
    an alias; a scope's name when the scope is the part that fails
  - nothing for a name too far from every one, or a short name two away
*/
static void
d_tests_option_env_suggestions(void)
{
    struct dj_tests_root_storage storage = D_OPTION_ENV_STORAGE_INIT;
    struct dj_option_env         env;
    const char*                  hint = NULL;

    printf("  did you mean\n");

    d_tests_option_env_root(&env, &storage);

    hint = d_option_env_suggest(&env, "stlye", 5u);
    D_TESTS_OPTION_ENV_CHECK( (hint != NULL) &&
                              (strcmp(hint, "style") == 0) );
    hint = d_option_env_suggest(&env, "key_with", 8u);
    D_TESTS_OPTION_ENV_CHECK( (hint != NULL) &&
                              (strcmp(hint, "key_width") == 0) );
    hint = d_option_env_suggest(&env, "sectionss", 9u);
    D_TESTS_OPTION_ENV_CHECK( (hint != NULL) &&
                              (strcmp(hint, "sections") == 0) );
    hint = d_option_env_suggest(&env, "RATIO", 5u);
    D_TESTS_OPTION_ENV_CHECK( (hint != NULL) &&
                              (strcmp(hint, "ratio") == 0) );
    hint = d_option_env_suggest(&env, "compilr.title", 13u);
    D_TESTS_OPTION_ENV_CHECK( (hint != NULL) &&
                              (strcmp(hint, "compiler") == 0) );
    hint = d_option_env_suggest(&env, "compiler.titel", 14u);
    D_TESTS_OPTION_ENV_CHECK( (hint != NULL) &&
                              (strcmp(hint, "title") == 0) );
    D_TESTS_OPTION_ENV_CHECK(d_option_env_suggest(&env, "completely", 10u)
                             == NULL);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_suggest(&env, "xy", 2u) == NULL);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_suggest(NULL, "style", 5u) == NULL);

    return;
}

/*
d_tests_option_env_hot_path
  Tests the following:
  - a cache reads once, and again only when the chain's generation moves:
    a change in the frame or in its parent
  - a variable that does not exist caches as kind 0
*/
static void
d_tests_option_env_hot_path(void)
{
    struct dj_tests_root_storage  root_storage  = D_OPTION_ENV_STORAGE_INIT;
    struct dj_tests_child_storage child_storage = D_OPTION_ENV_STORAGE_INIT;
    struct dj_option_env          root;
    struct dj_option_env          child;
    struct dj_option_env_cache    cache   = D_OPTION_ENV_CACHE_INIT;
    struct dj_option_env_cache    missing = D_OPTION_ENV_CACHE_INIT;

    printf("  reading on a hot path\n");

    d_tests_option_env_root(&root, &root_storage);
    (void)D_OPTION_ENV_VIEW(&child, child_storage);
    (void)d_option_env_set_parent(&child, &root);

    D_TESTS_OPTION_ENV_CHECK(d_option_env_cached(&child, 0u,
                                                 D_TESTS_KEY_WIDTH, &cache)
                             ->integer == 24);

    // the cache is what is read, until the generation moves
    cache.value.integer = 999;
    D_TESTS_OPTION_ENV_CHECK(d_option_env_cached(&child, 0u,
                                                 D_TESTS_KEY_WIDTH, &cache)
                             ->integer == 999);
    (void)d_option_env_set_int(&root, 0u, D_TESTS_KEY_WIDTH, 31);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_cached(&child, 0u,
                                                 D_TESTS_KEY_WIDTH, &cache)
                             ->integer == 31);
    (void)d_option_env_set_int(&child, 0u, D_TESTS_KEY_WIDTH, 32);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_cached(&child, 0u,
                                                 D_TESTS_KEY_WIDTH, &cache)
                             ->integer == 32);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_cached(&child, 0u, 99u, &missing)
                             ->kind == 0);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_cached(&child, 0u, 0u, NULL)
                             == NULL);

    return;
}

/*
main
  Every group, then the count.
*/
int
main(void)
{
    printf("t_option_env\n");

    d_tests_option_env_declarations();
    d_tests_option_env_values();
    d_tests_option_env_scopes_and_frames();
    d_tests_option_env_text();
    d_tests_option_env_scripts();
    d_tests_option_env_environment();
    d_tests_option_env_locale();
    d_tests_option_env_arena();
    d_tests_option_env_sources();
    d_tests_option_env_layered();
    d_tests_option_env_rows();
    d_tests_option_env_named_ints();
    d_tests_option_env_named_rules();
    d_tests_option_env_round_trip();
    d_tests_option_env_round_trip_text();
    d_tests_option_env_refused();
    d_tests_option_env_suggestions();
    d_tests_option_env_hot_path();
    d_tests_option_env_random();

    printf("%lu checks, %lu failed\n",
           (unsigned long)d_tests_option_env_checks,
           (unsigned long)d_tests_option_env_failures);

    return (d_tests_option_env_failures == 0u) ? 0 : 1;
}
