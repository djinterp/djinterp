/*******************************************************************************
* djinterp [test]                                               t_option_env.cpp
*
*   option_env's C++ face's harness (cpp/option/option_env.hpp), at every
* level from C++98: DOptionEnvView over a frame C made -- finding, typed
* reads, the value as text, binding from text and from sources by standing,
* did you mean, the status of a failure -- DOptionEnv as a root and as a
* child, which lets go of its parent when it goes, and DOptionEnvOverride:
* forced over every other source, given back in order, the frame's own
* OVERRIDE binding restored, a scoped one, one that fails changing nothing.
* A standalone program, as the other tests not yet in DTest are: it prints a
* line per group and one per failed check, ends with "N checks, M failed",
* and exits 0 only when nothing failed.
*
*
* path:      /tests/unintegrated/cpp/option/t_option_env.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.04
*******************************************************************************/
// std
#include <cstdio>   // std::printf
#include <cstring>  // std::strcmp
#include <string>   // std::string
// djinterp
#include "../../../../inc/djinterp/cpp/option/option_env.hpp"  // the C++ face

#if RE_STD_HAS_INT64


// D_TESTS_OPTION_ENV_CHECK
//   macro: one check: counts it, and prints the expression and line on
// failure.
#define D_TESTS_OPTION_ENV_CHECK(expr)                                         \
    check(((expr) ? true : false), #expr, __LINE__)

namespace
{

unsigned long g_checks   = 0;
unsigned long g_failures = 0;

// COLORS
//   constant: the `color` variable's names.
const struct dj_option_env_name COLORS[] =
{
    { "never",  0 },
    { "auto",   1 },
    { "always", 2 }
};

// SCHEMA
//   constant: the harness's variables.
const struct dj_option_env_var SCHEMA[] =
{
    D_OPTION_ENV_INT_RANGE("width", 80, 1, 1000, "the line width"),
    D_OPTION_ENV_CHOICE("color", COLORS, 1, "when to color"),
    D_OPTION_ENV_BOOL("verbose", 0, "talk more"),
    D_OPTION_ENV_REAL("ratio", 0.5, "a ratio"),
    D_OPTION_ENV_TEXT("title", "untitled", "the title")
};

// SCOPES
//   constant: the harness's one scope.
const char* const SCOPES[] =
{
    "narrow"
};

// check
//   function: counts one check, and reports it when it failed.
void check(
    bool        _ok,
    const char* _expression,
    int         _line
)
{
    ++g_checks;

    if (!_ok)
    {
        ++g_failures;
        std::printf("    FAILED line %d: %s\n", _line, _expression);
    }

    return;
}

// view
//   test: a frame C made, through DOptionEnvView: finding, typed reads, the
// value as text, binding by standing, did you mean, a failure's status.
void view()
{
    D_OPTION_ENV_DECLARE(storage_type, 16, 8, 256);

    static storage_type    storage;
    struct dj_option_env   frame;
    std::size_t            offset = 0u;

    std::printf("  a frame borrowed\n");

    D_TESTS_OPTION_ENV_CHECK(D_OPTION_ENV_VIEW(&frame, storage).is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_declare_all(&frame, SCHEMA, 5u)
                             .is_ok);
    D_TESTS_OPTION_ENV_CHECK(d_option_env_set_scopes(&frame, SCOPES, 1u)
                             .is_ok);

    djinterp::DOptionEnvView env(&frame);
    const uint32_t           width = env.find("width");

    D_TESTS_OPTION_ENV_CHECK(env.get() == &frame);
    D_TESTS_OPTION_ENV_CHECK( (width == 0u) &&
                              (env.find(std::string("title")) == 4u) &&
                              (env.find("nothing") == D_OPTION_ENV_NONE) &&
                              (env.find_scope("narrow") == 1u) &&
                              (env.find_scope("wide") == D_OPTION_ENV_NONE) );

    // the defaults, typed and as text
    D_TESTS_OPTION_ENV_CHECK( (env.integer(width) == 80) &&
                              (env.integer(env.find("color")) == 1) &&
                              (!env.boolean(env.find("verbose"))) &&
                              (env.real(env.find("ratio")) == 0.5) &&
                              (std::strcmp(env.text(env.find("title")),
                                           "untitled") == 0) );
    D_TESTS_OPTION_ENV_CHECK( (env.format(env.find("color")) == "auto") &&
                              (env.format(width) == "80") &&
                              (env.format(D_OPTION_ENV_NONE).empty()) );
    D_TESTS_OPTION_ENV_CHECK(env.source(width) ==
                             static_cast<int32_t>(D_OPTION_ENV_SOURCE_DEFAULT));

    // binding: by standing, whatever the order; scoped; a script
    const uint32_t before = env.generation();

    D_TESTS_OPTION_ENV_CHECK(env.set("width", "100",
                                     D_OPTION_ENV_SOURCE_FILE));
    D_TESTS_OPTION_ENV_CHECK(env.set("width", "120"));
    D_TESTS_OPTION_ENV_CHECK( (env.integer(width) == 100) &&
                              (env.source(width) ==
                               static_cast<int32_t>(
                                   D_OPTION_ENV_SOURCE_FILE)) &&
                              (env.generation() != before) );
    D_TESTS_OPTION_ENV_CHECK(env.put("narrow.width=40"));
    D_TESTS_OPTION_ENV_CHECK( (env.integer(width, env.find_scope("narrow")) ==
                               100) );
    D_TESTS_OPTION_ENV_CHECK(env.put("narrow.width=60",
                                     D_OPTION_ENV_SOURCE_COMMAND_LINE));
    D_TESTS_OPTION_ENV_CHECK( (env.integer(width, env.find_scope("narrow")) ==
                               60) &&
                              (env.integer(width) == 100) );
    D_TESTS_OPTION_ENV_CHECK(env.load("verbose; color=always; ratio=0.25",
                                      D_OPTION_ENV_SOURCE_FILE));
    D_TESTS_OPTION_ENV_CHECK( (env.boolean(env.find("verbose"))) &&
                              (env.format(env.find("color")) == "always") &&
                              (env.real(env.find("ratio")) == 0.25) );

    // a value as text, long enough to leave the stack
    const std::string long_title(100u, 't');

    D_TESTS_OPTION_ENV_CHECK(env.set("title", long_title.c_str()));
    D_TESTS_OPTION_ENV_CHECK(env.format(env.find("title")) == long_title);

    // failures: their status, and nothing bound
    D_TESTS_OPTION_ENV_CHECK( (!env.set("width", "2000")) &&
                              (env.status() ==
                               static_cast<int32_t>(
                                   D_OPTION_STATUS_INVALID_ARGUMENT)) &&
                              (env.integer(width) == 100) );
    D_TESTS_OPTION_ENV_CHECK( (!env.set("widht", "10")) &&
                              (env.status() ==
                               static_cast<int32_t>(
                                   D_OPTION_STATUS_KEY_ABSENT)) );
    D_TESTS_OPTION_ENV_CHECK( (!env.load("width=50; colour=never",
                                         D_OPTION_ENV_SOURCE_PROGRAM,
                                         &offset)) &&
                              (offset == 10u) );
    D_TESTS_OPTION_ENV_CHECK( (!env.set(D_NULLPTR, "1")) &&
                              (env.status() ==
                               static_cast<int32_t>(
                                   D_OPTION_STATUS_INVALID_ARGUMENT)) );
    D_TESTS_OPTION_ENV_CHECK( (env.suggest("widht") != D_NULLPTR) &&
                              (std::strcmp(env.suggest("widht"),
                                           "width") == 0) &&
                              (env.suggest("zzzzzz") == D_NULLPTR) );

    // unbinding: the next binding by standing holds
    D_TESTS_OPTION_ENV_CHECK(env.unset(width));
    D_TESTS_OPTION_ENV_CHECK(env.integer(width) == 80);

    return;
}

// owned
//   test: DOptionEnv as a root and as a child: the child sees the root's
// declarations and bindings, its own over them; it lets go when it goes.
void owned()
{
    std::printf("  a frame owned\n");

    djinterp::DOptionEnv<16, 8, 256> root;

    D_TESTS_OPTION_ENV_CHECK(root.status() ==
                             static_cast<int32_t>(D_OPTION_STATUS_OK));
    D_TESTS_OPTION_ENV_CHECK(root.declare_all(SCHEMA, 5u) == 0u);
    D_TESTS_OPTION_ENV_CHECK(root.declare(SCHEMA[0]) == D_OPTION_ENV_NONE);
    D_TESTS_OPTION_ENV_CHECK(root.status() ==
                             static_cast<int32_t>(
                                 D_OPTION_STATUS_KEY_DUPLICATE));
    D_TESTS_OPTION_ENV_CHECK(root.set_scopes(SCOPES, 1u));
    D_TESTS_OPTION_ENV_CHECK(root.set_name("harness"));
    D_TESTS_OPTION_ENV_CHECK(root.load("width=90; title=root"));

    {
        djinterp::DOptionEnv<8> child(*root.get());

        D_TESTS_OPTION_ENV_CHECK(child.status() ==
                                 static_cast<int32_t>(D_OPTION_STATUS_OK));
        D_TESTS_OPTION_ENV_CHECK(root.get()->child_count == 1u);
        D_TESTS_OPTION_ENV_CHECK( (child.find("width") == 0u) &&
                                  (child.integer(0u) == 90) );
        D_TESTS_OPTION_ENV_CHECK(child.put("width=70"));
        D_TESTS_OPTION_ENV_CHECK( (child.integer(0u) == 70) &&
                                  (root.integer(0u) == 90) );
        child.clear();
        D_TESTS_OPTION_ENV_CHECK(child.integer(0u) == 90);
    }

    D_TESTS_OPTION_ENV_CHECK(root.get()->child_count == 0u);
    root.clear();
    D_TESTS_OPTION_ENV_CHECK(root.integer(0u) == 80);

    return;
}

// overrides
//   test: DOptionEnvOverride forces over every other source and gives back
// in order; the frame's own OVERRIDE binding is restored; a scoped one; one
// that fails changes nothing.
void overrides()
{
    std::printf("  overrides\n");

    djinterp::DOptionEnv<16, 8, 256> env;

    (void)env.declare_all(SCHEMA, 5u);
    (void)env.set_scopes(SCOPES, 1u);
    D_TESTS_OPTION_ENV_CHECK(env.set("width", "100",
                                     D_OPTION_ENV_SOURCE_COMMAND_LINE));

    {
        djinterp::DOptionEnvOverride narrow(*env.get(), "width", "40");

        D_TESTS_OPTION_ENV_CHECK(narrow.ok());
        D_TESTS_OPTION_ENV_CHECK( (env.integer(0u) == 40) &&
                                  (env.source(0u) ==
                                   static_cast<int32_t>(
                                       D_OPTION_ENV_SOURCE_OVERRIDE)) );

        {
            djinterp::DOptionEnvOverride narrower(*env.get(), "width", "20");

            D_TESTS_OPTION_ENV_CHECK(env.integer(0u) == 20);
        }

        // the outer one's binding, restored
        D_TESTS_OPTION_ENV_CHECK( (env.integer(0u) == 40) &&
                                  (env.source(0u) ==
                                   static_cast<int32_t>(
                                       D_OPTION_ENV_SOURCE_OVERRIDE)) );
    }

    D_TESTS_OPTION_ENV_CHECK( (env.integer(0u) == 100) &&
                              (env.source(0u) ==
                               static_cast<int32_t>(
                                   D_OPTION_ENV_SOURCE_COMMAND_LINE)) );

    // scoped: the global value untouched
    {
        djinterp::DOptionEnvOverride scoped(*env.get(), "narrow.width", "30");

        D_TESTS_OPTION_ENV_CHECK( (scoped.ok()) &&
                                  (env.integer(0u, 1u) == 30) &&
                                  (env.integer(0u) == 100) );
    }

    D_TESTS_OPTION_ENV_CHECK(env.integer(0u, 1u) == 100);

    // text, long, over an OVERRIDE binding the frame made itself
    const std::string long_title(90u, 'x');

    D_TESTS_OPTION_ENV_CHECK(env.set("title", long_title.c_str(),
                                     D_OPTION_ENV_SOURCE_OVERRIDE));

    {
        djinterp::DOptionEnvOverride title(*env.get(), "title", "short");

        D_TESTS_OPTION_ENV_CHECK(std::strcmp(env.text(4u), "short") == 0);
    }

    D_TESTS_OPTION_ENV_CHECK( (env.format(4u) == long_title) &&
                              (env.source(4u) ==
                               static_cast<int32_t>(
                                   D_OPTION_ENV_SOURCE_OVERRIDE)) );

    // a failure changes nothing, and gives nothing back
    {
        djinterp::DOptionEnvOverride bad(*env.get(), "width", "wide");
        djinterp::DOptionEnvOverride none(*env.get(), "nothing", "1");
        djinterp::DOptionEnvOverride null(*env.get(), "width", D_NULLPTR);

        D_TESTS_OPTION_ENV_CHECK( (!bad.ok()) &&
                                  (bad.status() ==
                                   static_cast<int32_t>(
                                       D_OPTION_STATUS_INVALID_ARGUMENT)) &&
                                  (env.integer(0u) == 100) );
        D_TESTS_OPTION_ENV_CHECK( (!none.ok()) &&
                                  (none.status() ==
                                   static_cast<int32_t>(
                                       D_OPTION_STATUS_KEY_ABSENT)) );
        D_TESTS_OPTION_ENV_CHECK(!null.ok());
    }

    D_TESTS_OPTION_ENV_CHECK( (env.integer(0u) == 100) &&
                              (env.source(0u) ==
                               static_cast<int32_t>(
                                   D_OPTION_ENV_SOURCE_COMMAND_LINE)) );

    return;
}

}  // namespace


// main
//   function: every group, then the count.
int main()
{
    std::printf("t_option_env (C++%ld)\n", static_cast<long>(__cplusplus));

    view();
    owned();
    overrides();

    std::printf("%lu checks, %lu failed\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}

#else

// below the option core's floor the face is absent, and so is its harness
int main()
{
    std::printf("t_option_env: no 64-bit integers, nothing to test\n");

    return 0;
}

#endif  // RE_STD_HAS_INT64
