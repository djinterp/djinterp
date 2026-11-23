/*******************************************************************************
* djinterp [c][option]                                   t_option_env_registry.c
*
*   The djin registry's C harness (c/option/option_env_registry.h): the
* program's registry, ready before main; a registry of the harness's own,
* with a program-wide variable in its root and two djins beneath it; djins
* listed, found and refused; settings by full name from every source, by
* statement and by script, routed to the frame they name; the process's
* environment under the prefix and each djin's name; the command line's
* forms; did-you-mean over djins, scopes and variables; a machine's frame
* beneath its djin.
*   A standalone program, as the tests not yet in DTest are: a line per group,
* one per failed check, "N checks, M failed" at the end, and exit 0 only when
* nothing failed.
*
*
* path:      /tests/unintegrated/c/option/t_option_env_registry.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.19
*******************************************************************************/
// std
#include <stdbool.h>  // bool
#include <stdio.h>    // printf
#include <stdlib.h>   // setenv
#include <string.h>   // strcmp, strlen
// djinterp
#include "../../../../inc/djinterp/c/option/option_env.h"           // frames
#include "../../../../inc/djinterp/c/option/option_env_registry.h"  // tested
// re_std
#include "../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t


// D_TESTS_REGISTRY_CHECK
//   macro: one check: counts it, and prints the expression and line when it
// fails.
#define D_TESTS_REGISTRY_CHECK(expr)                                           \
    d_tests_registry_check(((expr) ? 1 : 0), #expr, __LINE__)

// D_TESTS_REGISTRY_STATUS
//   macro: a result's status: 0 when it is ok.
#define D_TESTS_REGISTRY_STATUS(result)                                        \
    ((result).is_ok ? 0 : (result).payload.error)

static unsigned long d_tests_registry_checks;
static unsigned long d_tests_registry_failures;

// the root's program-wide variable
static const struct dj_option_env_var D_TESTS_ROOT_SCHEMA[] =
{
    D_OPTION_ENV_BOOL("color", 0, "whether output may be colored")
};

// the printer djin's schema and scopes
static const struct dj_option_env_name D_TESTS_STYLES[] =
{
    { "text", 0 },
    { "json", 1 }
};

static const struct dj_option_env_var D_TESTS_PRINTER_SCHEMA[] =
{
    D_OPTION_ENV_CHOICE("style", D_TESTS_STYLES, 0, "the preset"),
    D_OPTION_ENV_INT_RANGE("key_width", 0, 0, 200, "the key column"),
    D_OPTION_ENV_TEXT("title", "report", "the title")
};

static const char* const D_TESTS_PRINTER_SCOPES[] = { "language", "compiler" };

// a scope of the root's, by the name a djin might have
static const char* const D_TESTS_ROOT_SCOPES[] = { "net" };

// the logger djin's schema
static const struct dj_option_env_var D_TESTS_LOGGER_SCHEMA[] =
{
    D_OPTION_ENV_INT_RANGE("level", 2, 0, 5, "the least level written"),
    D_OPTION_ENV_BOOL("json", 0, "whether records are written as JSON")
};

D_OPTION_ENV_DECLARE(dj_tests_djin_storage, 32, 8, 256);
D_OPTION_ENV_DECLARE(dj_tests_machine_storage, 16, 0, 64);

/*
d_tests_registry_check
  Counts one check, and reports it when it failed.
*/
static bool
d_tests_registry_check(
    int         _ok,
    const char* _expression,
    int         _line
)
{
    ++d_tests_registry_checks;

    // a failure says what and where
    if (_ok == 0)
    {
        ++d_tests_registry_failures;
        printf("    FAILED line %d: %s\n", _line, _expression);
    }

    return (_ok != 0);
}

/*
d_tests_registry_setenv
  Sets one variable of the process's environment, on any platform.
*/
static void
d_tests_registry_setenv(
    const char* _name,
    const char* _value
)
{
#if defined(_WIN32)
    (void)_putenv_s(_name, _value);
#else
    (void)setenv(_name, _value, 1);
#endif

    return;
}

/*
d_tests_registry_make
  A registry of the harness's own: the root's variable, and the two djins,
each its schema declared and registered.
*/
static void
d_tests_registry_make(
    struct dj_option_env_registry* _registry,
    struct dj_option_env*          _printer,
    struct dj_tests_djin_storage*  _printer_storage,
    struct dj_option_env*          _logger,
    struct dj_tests_djin_storage*  _logger_storage
)
{
    (void)d_option_env_registry_init(_registry, "tests");
    (void)d_option_env_declare_all(d_option_env_registry_frame(_registry),
                                   D_TESTS_ROOT_SCHEMA,
                                   1u);
    (void)D_OPTION_ENV_VIEW(_printer, *_printer_storage);
    (void)d_option_env_set_name(_printer, "printer");
    (void)D_OPTION_ENV_VIEW(_logger, *_logger_storage);
    (void)d_option_env_set_name(_logger, "logger");
    (void)d_option_env_registry_add(_registry, _printer);
    (void)d_option_env_registry_add(_registry, _logger);
    (void)d_option_env_declare_all(_printer, D_TESTS_PRINTER_SCHEMA, 3u);
    (void)d_option_env_set_scopes(_printer, D_TESTS_PRINTER_SCOPES, 2u);
    (void)d_option_env_declare_all(_logger, D_TESTS_LOGGER_SCHEMA, 2u);

    return;
}

/*
d_tests_registry_program
  Tests the following:
  - the program's registry is there before anything sets it up: its root
    named, its room the knobs', its list empty, its frame usable
*/
static void
d_tests_registry_program(void)
{
    struct dj_option_env_registry* const program = d_option_env_registry();

    printf("  the program's registry\n");

    D_TESTS_REGISTRY_CHECK(program != NULL);
    D_TESTS_REGISTRY_CHECK(program == d_option_env_registry());
    D_TESTS_REGISTRY_CHECK(strcmp(program->frame.name, "djinterp") == 0);
    D_TESTS_REGISTRY_CHECK(program->frame.set.capacity ==
                           D_CFG_OPTION_ENV_REGISTRY_BINDINGS);
    D_TESTS_REGISTRY_CHECK(program->frame.var_capacity ==
                           D_CFG_OPTION_ENV_REGISTRY_VARIABLES);
    D_TESTS_REGISTRY_CHECK(program->frame.text_capacity ==
                           D_CFG_OPTION_ENV_REGISTRY_TEXT);
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_count(program) == 0u);
    D_TESTS_REGISTRY_CHECK(d_option_env_set_scopes(&program->frame, NULL, 0u)
                           .is_ok);
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_at(program, 0u) == NULL);

    return;
}

/*
d_tests_registry_djins
  Tests the following:
  - djins listed in order, found by name, their depth 1, the root their
    parent; the root's variable seen from each
  - refused: a duplicate name, no name, a frame with children, the root
    itself, a full list; a refused djin keeps its parent
  - a removed djin is let go and the list closes up
*/
static void
d_tests_registry_djins(void)
{
    static struct dj_option_env_registry registry;
    struct dj_tests_djin_storage         printer_storage =
                                             D_OPTION_ENV_STORAGE_INIT;
    struct dj_tests_djin_storage         logger_storage =
                                             D_OPTION_ENV_STORAGE_INIT;
    struct dj_tests_djin_storage         other_storage =
                                             D_OPTION_ENV_STORAGE_INIT;
    struct dj_tests_machine_storage      machine_storage =
                                             D_OPTION_ENV_STORAGE_INIT;
    struct dj_option_env                 printer;
    struct dj_option_env                 logger;
    struct dj_option_env                 other;
    struct dj_option_env                 machine;

    printf("  djins\n");

    d_tests_registry_make(&registry, &printer, &printer_storage, &logger,
                          &logger_storage);

    D_TESTS_REGISTRY_CHECK(d_option_env_registry_count(&registry) == 2u);
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_at(&registry, 1u) == &logger);
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_find(&registry, "printer",
                                                      7u) == &printer);
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_find(&registry, "print", 5u)
                           == NULL);
    D_TESTS_REGISTRY_CHECK( (printer.depth == 1u) &&
                            (printer.parent == &registry.frame) );
    D_TESTS_REGISTRY_CHECK(d_option_env_find(&logger, "color", 5u).is_ok);

    // what is refused
    (void)D_OPTION_ENV_VIEW(&other, other_storage);
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_add(&registry, &other)
                           .payload.error ==
                           (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    (void)d_option_env_set_name(&other, "printer");
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_add(&registry, &other)
                           .payload.error ==
                           (int32_t)D_OPTION_STATUS_KEY_DUPLICATE);
    (void)d_option_env_set_name(&other, "other");
    (void)D_OPTION_ENV_VIEW(&machine, machine_storage);
    (void)d_option_env_set_parent(&machine, &other);
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_add(&registry, &other)
                           .payload.error ==
                           (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_add(&registry,
                                                     &registry.frame)
                           .payload.error ==
                           (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    D_TESTS_REGISTRY_CHECK(other.parent == NULL);
    d_option_env_detach(&machine);
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_add(&registry, &other)
                           .payload.value == 2u);

    // removed: let go, and the list closes up
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_remove(&registry, &printer)
                           .payload.value == 0u);
    D_TESTS_REGISTRY_CHECK( (printer.parent == NULL) &&
                            (d_option_env_registry_count(&registry) == 2u) &&
                            (d_option_env_registry_at(&registry, 0u) ==
                             &logger) );
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_remove(&registry, &printer)
                           .payload.error ==
                           (int32_t)D_OPTION_STATUS_KEY_ABSENT);

    return;
}

/*
d_tests_registry_machine
  Tests the following:
  - a machine beneath the printer djin
*/
static void
d_tests_registry_machine(
    struct dj_tests_machine_storage* _machine_storage,
    struct dj_option_env*            _printer,
    struct dj_option_env*            _machine,
    uint32_t                         _style
)
{
    // a machine beneath the printer djin
    (void)D_OPTION_ENV_VIEW(_machine, *_machine_storage);
    D_TESTS_REGISTRY_CHECK(d_option_env_set_parent(_machine, _printer).is_ok);
    D_TESTS_REGISTRY_CHECK(strcmp(d_option_env_text(_machine, 0u, _style),
                                  "json") == 0);
    (void)d_option_env_put(_machine, "style=text");
    D_TESTS_REGISTRY_CHECK(strcmp(d_option_env_text(_machine, 0u, _style),
                                  "json") == 0);
    (void)d_option_env_put_from(_machine, "style=text",
                                D_OPTION_ENV_SOURCE_FILE);
    D_TESTS_REGISTRY_CHECK(strcmp(d_option_env_text(_machine, 0u, _style),
                                  "text") == 0);
    D_TESTS_REGISTRY_CHECK(strcmp(d_option_env_text(_printer, 0u, _style),
                                  "json") == 0);
    d_option_env_detach(_machine);

    return;
}

/*
d_tests_registry_names
  Tests the following:
  - a full name reaches a djin's variable, scoped or not, and the root's;
    the frame a name addresses
  - sources by full name: the command line holds whatever came after it
  - a statement and a script, each statement to its own frame; a failing
    statement's offset; a comment
  - a machine's frame beneath a djin reads the djin's settings and binds
    its own
*/
static void
d_tests_registry_names(void)
{
    static struct dj_option_env_registry registry;
    struct dj_tests_djin_storage         printer_storage =
                                             D_OPTION_ENV_STORAGE_INIT;
    struct dj_tests_djin_storage         logger_storage =
                                             D_OPTION_ENV_STORAGE_INIT;
    struct dj_tests_machine_storage      machine_storage =
                                             D_OPTION_ENV_STORAGE_INIT;
    struct dj_option_env                 printer;
    struct dj_option_env                 logger;
    struct dj_option_env                 machine;
    size_t                               offset = 0u;

    printf("  settings by full name\n");

    d_tests_registry_make(&registry, &printer, &printer_storage, &logger,
                          &logger_storage);

    const uint32_t style = d_option_env_find(&printer, "style", 5u)
                               .payload.value;
    const uint32_t width = style + 1u;
    const uint32_t level = d_option_env_find(&logger, "level", 5u)
                               .payload.value;

    D_TESTS_REGISTRY_CHECK(d_option_env_registry_target(&registry,
                                                        "printer.style", 13u)
                           == &printer);
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_target(&registry, "color",
                                                        5u)
                           == &registry.frame);
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_assign(&registry,
        "printer.style", 13u, "json", 4u, D_OPTION_ENV_SOURCE_FILE).is_ok);
    D_TESTS_REGISTRY_CHECK(strcmp(d_option_env_text(&printer, 0u, style),
                                  "json") == 0);
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_assign(&registry,
        "printer.compiler.key_width", 26u, "40",
        2u, D_OPTION_ENV_SOURCE_FILE).is_ok);
    D_TESTS_REGISTRY_CHECK(d_option_env_int(&printer, (1u << 20) | 2u, width)
                           == 40);
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_assign(&registry, "color",
        5u, "yes", 3u, D_OPTION_ENV_SOURCE_PROGRAM).is_ok);
    D_TESTS_REGISTRY_CHECK(d_option_env_bool(&logger, 0u, 0u));
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_assign(&registry,
        "logger.levle", 12u, "1", 1u, D_OPTION_ENV_SOURCE_FILE)
        .payload.error == (int32_t)D_OPTION_STATUS_KEY_ABSENT);

    // the command line, then a file: the command line holds
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_put(&registry,
        "logger.level=4", D_OPTION_ENV_SOURCE_COMMAND_LINE).is_ok);
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_put(&registry,
        "logger.level=1", D_OPTION_ENV_SOURCE_FILE).is_ok);
    D_TESTS_REGISTRY_CHECK(d_option_env_int(&logger, 0u, level) == 4);
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_put(&registry,
        "-logger.level", D_OPTION_ENV_SOURCE_COMMAND_LINE).is_ok);
    D_TESTS_REGISTRY_CHECK(d_option_env_int(&logger, 0u, level) == 1);

    // a script across frames
    const struct dj_option_result done = d_option_env_registry_load(
        &registry,
        "# settings for two djins and the root\n"
        "printer.title = \"a; b\"\n"
        "logger.json; color=no\n"
        "printer.language.key_width=12",
        D_OPTION_ENV_SOURCE_FILE,
        &offset);

    D_TESTS_REGISTRY_CHECK( (done.is_ok) &&
                            (done.payload.value == 4u) );
    D_TESTS_REGISTRY_CHECK(strcmp(d_option_env_text(&printer, 0u, style + 2u),
                                  "a; b") == 0);
    D_TESTS_REGISTRY_CHECK(d_option_env_bool(&logger, 0u, level + 1u));
    D_TESTS_REGISTRY_CHECK(!d_option_env_bool(&printer, 0u, 0u));
    D_TESTS_REGISTRY_CHECK(d_option_env_int(&printer, (1u << 20) | 1u, width)
                           == 12);

    const struct dj_option_result failed = d_option_env_registry_load(
        &registry,
        "logger.level=3; printer.key_width=999; logger.level=5",
        D_OPTION_ENV_SOURCE_FILE,
        &offset);

    D_TESTS_REGISTRY_CHECK( (!failed.is_ok) &&
                            (offset == 16u) );
    D_TESTS_REGISTRY_CHECK(d_option_env_int(&logger, 0u, level) == 3);

    d_tests_registry_machine(&machine_storage, &printer, &machine, style);

    return;
}

/*
d_tests_registry_environment
  Tests the following:
  - the root's variables under the prefix; a djin's under the prefix and its
    name; a root variable set for one djin alone under that djin's prefix
*/
static void
d_tests_registry_environment(void)
{
    static struct dj_option_env_registry registry;
    struct dj_tests_djin_storage         printer_storage =
                                             D_OPTION_ENV_STORAGE_INIT;
    struct dj_tests_djin_storage         logger_storage =
                                             D_OPTION_ENV_STORAGE_INIT;
    struct dj_option_env                 printer;
    struct dj_option_env                 logger;

    printf("  the process environment\n");

    d_tests_registry_make(&registry, &printer, &printer_storage, &logger,
                          &logger_storage);
    d_tests_registry_setenv("DJREG_COLOR", "on");
    d_tests_registry_setenv("DJREG_PRINTER_STYLE", "json");
    d_tests_registry_setenv("DJREG_PRINTER_COLOR", "off");
    d_tests_registry_setenv("DJREG_LOGGER_LEVEL", "5");

    const struct dj_option_result imported =
        d_option_env_registry_import(&registry, "DJREG_");

    D_TESTS_REGISTRY_CHECK( (imported.is_ok) &&
                            (imported.payload.value == 4u) );
    D_TESTS_REGISTRY_CHECK(d_option_env_bool(&logger, 0u, 0u));
    D_TESTS_REGISTRY_CHECK(!d_option_env_bool(&printer, 0u, 0u));
    D_TESTS_REGISTRY_CHECK(d_option_env_source(&printer, 0u, 0u) ==
                           D_OPTION_ENV_SOURCE_ENVIRONMENT);
    D_TESTS_REGISTRY_CHECK(d_option_env_int(&logger, 0u,
                                            d_option_env_find(&logger,
                                                              "level", 5u)
                                                .payload.value) == 5);
    D_TESTS_REGISTRY_CHECK(strcmp(d_option_env_text(&printer, 0u,
                                      d_option_env_find(&printer, "style",
                                                        5u).payload.value),
                                  "json") == 0);

    return;
}

/*
d_tests_registry_bad_options
  Tests the following:
  - not settings, and settings named badly; at stays where it was
*/
static void
d_tests_registry_bad_options(
    struct dj_option_env_registry* _registry,
    char**                         _argv,
    int                            _argc
)
{
    struct dj_option_result result;
    int                     at = 0;

    // not settings, and settings named badly
    at = 6;
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_take_option(_registry, _argc,
                                                             _argv, &at,
                                                             NULL) == 0);
    D_TESTS_REGISTRY_CHECK(at == 6);
    at = 7;
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_take_option(_registry, _argc,
                                                             _argv, &at,
                                                             NULL) == 0);
    at = 8;
    D_TESTS_REGISTRY_CHECK( (d_option_env_registry_take_option(_registry,
                                                               _argc, _argv,
                                                               &at, &result)
                             == -1) &&
                            (result.payload.error ==
                             (int32_t)D_OPTION_STATUS_KEY_ABSENT) );
    at = 9;
    D_TESTS_REGISTRY_CHECK( (d_option_env_registry_take_option(_registry,
                                                               _argc, _argv,
                                                               &at, &result)
                             == -1) &&
                            (result.payload.error ==
                             (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT) );
    at = 10;
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_take_option(_registry, _argc,
                                                             _argv, &at,
                                                             NULL) == -1);
    D_TESTS_REGISTRY_CHECK(at == 10);
    at = 11;
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_take_option(_registry, _argc,
                                                             _argv, &at,
                                                             NULL) == 0);
    at = _argc;
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_take_option(_registry, _argc,
                                                             _argv, &at,
                                                             NULL) == 0);

    return;
}

/*
d_tests_registry_bare_forms
  Tests the following:
  - "--NAME" and "--no-NAME" on a TEXT setting are -1, INVALID_ARGUMENT,
    and bind nothing: the title stays its default (option_env's decision
    37)
*/
static void
d_tests_registry_bare_forms(
    struct dj_option_env_registry* _registry,
    struct dj_option_env*          _printer,
    char**                         _argv,
    int                            _argc
)
{
    struct dj_option_result result;
    const uint32_t          title =
        d_option_env_find(_printer, "title", 5u).payload.value;
    int                     at;

    for (at = 12; at <= 13; ++at)
    {
        const int start = at;

        D_TESTS_REGISTRY_CHECK( (d_option_env_registry_take_option(_registry,
                                                                   _argc,
                                                                   _argv, &at,
                                                                   &result)
                                 == -1) &&
                                (result.payload.error ==
                                 (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT) );
        D_TESTS_REGISTRY_CHECK(at == start);
        D_TESTS_REGISTRY_CHECK(strcmp(d_option_env_text(_printer, 0u, title),
                                      "report") == 0);
        D_TESTS_REGISTRY_CHECK(d_option_env_source(_printer, 0u, title) ==
                               D_OPTION_ENV_SOURCE_DEFAULT);
    }

    return;
}

/*
d_tests_registry_command_line
  Tests the following:
  - "--set NAME=VALUE", "--NAME=VALUE", "--NAME", "--no-NAME", each bound
    from the command line
  - at ends on the last argument read: "--set"'s value, or the argument
    itself
  - an operand, "--", and a bare "-" are not settings; an unknown name, a
    flag of a kind that is not a bool, and "--set" with no value are -1
*/
static void
d_tests_registry_command_line(void)
{
    static struct dj_option_env_registry registry;
    struct dj_tests_djin_storage         printer_storage =
                                             D_OPTION_ENV_STORAGE_INIT;
    struct dj_tests_djin_storage         logger_storage =
                                             D_OPTION_ENV_STORAGE_INIT;
    struct dj_option_env                 printer;
    struct dj_option_env                 logger;
    struct dj_option_result              result;
    char*                                argv[] =
    {
        (char*)"prog",
        (char*)"--set",
        (char*)"logger.level=1",
        (char*)"--printer.style=json",
        (char*)"--logger.json",
        (char*)"--no-color",
        (char*)"file.txt",
        (char*)"--",
        (char*)"--printer.stlye=json",
        (char*)"--logger.level",
        (char*)"--set",
        (char*)"-",
        (char*)"--printer.title",
        (char*)"--no-printer.title"
    };
    const int                            argc = (int)(sizeof(argv) /
                                                      sizeof(argv[0]));
    int                                  at   = 1;

    printf("  the command line\n");

    d_tests_registry_make(&registry, &printer, &printer_storage, &logger,
                          &logger_storage);
    (void)d_option_env_registry_put(&registry, "color=yes",
                                    D_OPTION_ENV_SOURCE_ENVIRONMENT);

    D_TESTS_REGISTRY_CHECK(d_option_env_registry_take_option(&registry, argc,
                                                             argv, &at,
                                                             &result) == 1);
    D_TESTS_REGISTRY_CHECK(at == 2);
    at = 3;
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_take_option(&registry, argc,
                                                             argv, &at,
                                                             &result) == 1);
    D_TESTS_REGISTRY_CHECK(at == 3);
    at = 4;
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_take_option(&registry, argc,
                                                             argv, &at,
                                                             NULL) == 1);
    at = 5;
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_take_option(&registry, argc,
                                                             argv, &at,
                                                             NULL) == 1);
    D_TESTS_REGISTRY_CHECK(d_option_env_int(&logger, 0u,
                                            d_option_env_find(&logger,
                                                              "level", 5u)
                                                .payload.value) == 1);
    D_TESTS_REGISTRY_CHECK(d_option_env_source(&printer, 0u,
                                               d_option_env_find(&printer,
                                                                 "style", 5u)
                                                   .payload.value)
                           == D_OPTION_ENV_SOURCE_COMMAND_LINE);
    D_TESTS_REGISTRY_CHECK(d_option_env_bool(&logger, 0u,
                                             d_option_env_find(&logger,
                                                               "json", 4u)
                                                 .payload.value));
    D_TESTS_REGISTRY_CHECK(!d_option_env_bool(&registry.frame, 0u, 0u));

    d_tests_registry_bad_options(&registry, argv, argc);
    d_tests_registry_bare_forms(&registry, &printer, argv, argc);

    return;
}

/*
d_tests_registry_suggestions
  Tests the following:
  - the failing part replaced, the rest kept: a djin, a variable in a djin,
    a scope in a djin, the root's variable
  - nothing near is 0, and a small buffer is cut and terminated
*/
static void
d_tests_registry_suggestions(void)
{
    static struct dj_option_env_registry registry;
    struct dj_tests_djin_storage         printer_storage =
                                             D_OPTION_ENV_STORAGE_INIT;
    struct dj_tests_djin_storage         logger_storage =
                                             D_OPTION_ENV_STORAGE_INIT;
    struct dj_option_env                 printer;
    struct dj_option_env                 logger;
    char                                 hint[64];
    char                                 small[8];

    printf("  did you mean\n");

    d_tests_registry_make(&registry, &printer, &printer_storage, &logger,
                          &logger_storage);

    D_TESTS_REGISTRY_CHECK( (d_option_env_registry_suggest(&registry,
                                 "printr.style", 12u, hint, sizeof(hint))
                             == 13u) &&
                            (strcmp(hint, "printer.style") == 0) );
    D_TESTS_REGISTRY_CHECK( (d_option_env_registry_suggest(&registry,
                                 "printer.stlye", 13u, hint, sizeof(hint))
                             > 0u) &&
                            (strcmp(hint, "printer.style") == 0) );
    D_TESTS_REGISTRY_CHECK( (d_option_env_registry_suggest(&registry,
                                 "printer.compilr.key_width", 25u, hint,
                                 sizeof(hint)) > 0u) &&
                            (strcmp(hint, "printer.compiler.key_width")
                             == 0) );
    D_TESTS_REGISTRY_CHECK( (d_option_env_registry_suggest(&registry,
                                 "printer.compiler.key_widht", 26u, hint,
                                 sizeof(hint)) > 0u) &&
                            (strcmp(hint, "printer.compiler.key_width")
                             == 0) );
    D_TESTS_REGISTRY_CHECK( (d_option_env_registry_suggest(&registry,
                                 "logr.levl", 9u, hint, sizeof(hint)) > 0u) &&
                            (strcmp(hint, "logger.level") == 0) );
    D_TESTS_REGISTRY_CHECK( (d_option_env_registry_suggest(&registry,
                                 "colour", 6u, hint, sizeof(hint)) > 0u) &&
                            (strcmp(hint, "color") == 0) );
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_suggest(&registry,
                               "nothing.here", 12u, hint, sizeof(hint))
                           == 0u);
    D_TESTS_REGISTRY_CHECK( (d_option_env_registry_suggest(&registry,
                                 "printr.style", 12u, small, sizeof(small))
                             == 13u) &&
                            (strcmp(small, "printer") == 0) );

    return;
}

// dj_tests_heard
//   struct: what the report heard: how many, and the names, joined.
struct dj_tests_heard
{
    unsigned count;
    char     names[128];
    char     settings[128];
};

/*
d_tests_registry_heard
  A report that joins every name it is told.
*/
static void
d_tests_registry_heard(
    void*                              _context,
    const struct dj_option_env_report* _report
)
{
    struct dj_tests_heard* const heard = (struct dj_tests_heard*)_context;
    const size_t                 used  = strlen(heard->names);
    const size_t                 more  = strlen(heard->settings);

    ++heard->count;
    (void)snprintf(heard->names + used, sizeof(heard->names) - used, "%s;",
                   _report->variable);
    (void)snprintf(heard->settings + more, sizeof(heard->settings) - more,
                   "%s;", _report->setting);

    return;
}

/*
d_tests_registry_import_each
  Tests the following:
  - every failure in the root and in each djin is reported under its full
    environment name, and the rest still bind
*/
static void
d_tests_registry_import_each(void)
{
    static struct dj_option_env_registry registry;
    struct dj_tests_djin_storage         printer_storage =
                                             D_OPTION_ENV_STORAGE_INIT;
    struct dj_tests_djin_storage         logger_storage =
                                             D_OPTION_ENV_STORAGE_INIT;
    struct dj_option_env                 printer;
    struct dj_option_env                 logger;
    struct dj_tests_heard                heard;

    printf("  the process environment, each failure reported\n");

    memset(&heard, 0, sizeof(heard));
    d_tests_registry_make(&registry, &printer, &printer_storage, &logger,
                          &logger_storage);
    d_tests_registry_setenv("DJEACHREG_COLOR", "maybe");
    d_tests_registry_setenv("DJEACHREG_PRINTER_KEY_WIDTH", "999");
    d_tests_registry_setenv("DJEACHREG_LOGGER_LEVEL", "4");

    const struct dj_option_result imported =
        d_option_env_registry_import_each(&registry, "DJEACHREG_",
                                          d_tests_registry_heard, &heard);

    D_TESTS_REGISTRY_CHECK(!imported.is_ok);
    D_TESTS_REGISTRY_CHECK(heard.count == 2u);
    D_TESTS_REGISTRY_CHECK(strcmp(heard.names,
                                  "DJEACHREG_COLOR;"
                                  "DJEACHREG_PRINTER_KEY_WIDTH;") == 0);
    // each setting by the full name the registry resolves
    D_TESTS_REGISTRY_CHECK(strcmp(heard.settings,
                                  "color;printer.key_width;") == 0);
    D_TESTS_REGISTRY_CHECK(d_option_env_int(&logger, 0u,
                                            d_option_env_find(&logger,
                                                              "level", 5u)
                                                .payload.value) == 4);

    return;
}

// dj_tests_registry_env
//   struct: an environment of the test's own, in name and value pairs.
struct dj_tests_registry_env
{
    const char* const* pairs;
    unsigned           count;
    unsigned           asked;
};

/*
d_tests_registry_reader
  A reader over the test's own environment, as fn_option_env_getenv reads.
*/
static int32_t
d_tests_registry_reader(
    void*       _context,
    const char* _name,
    char*       _value,
    size_t      _size,
    size_t*     _length
)
{
    struct dj_tests_registry_env* const env =
        (struct dj_tests_registry_env*)_context;

    ++env->asked;

    for (unsigned i = 0u; i < env->count; ++i)
    {
        if (strcmp(env->pairs[2u * i], _name) == 0)
        {
            const size_t size = strlen(env->pairs[2u * i + 1u]);

            *_length = size;

            if (size >= _size)
            {
                return -1;
            }

            memcpy(_value, env->pairs[2u * i + 1u], size + 1u);

            return 1;
        }
    }

    return 0;
}

/*
d_tests_registry_scoped
  Tests the following:
  - the registry's reader is its root's, and every djin's import asks it
    rather than getenv
  - with D_CFG_OPTION_ENV_SCOPED_IMPORT, a djin's scope's settings come
    from its prefix, the scope and the variable, and a failure is reported
    by its full name, "printer.compiler.key_width"
*/
static void
d_tests_registry_scoped(void)
{
    static const char* const PAIRS[] =
    {
        "DJSREG_PRINTER_STYLE",              "json",
        "DJSREG_PRINTER_LANGUAGE_TITLE",     "le titre",
        "DJSREG_PRINTER_COMPILER_KEY_WIDTH", "500",
        "DJSREG_LOGGER_LEVEL",               "3"
    };
    static struct dj_option_env_registry registry;
    struct dj_tests_djin_storage         printer_storage =
                                             D_OPTION_ENV_STORAGE_INIT;
    struct dj_tests_djin_storage         logger_storage =
                                             D_OPTION_ENV_STORAGE_INIT;
    struct dj_option_env                 printer;
    struct dj_option_env                 logger;
    struct dj_tests_heard                heard;
    struct dj_tests_registry_env         env;
    struct dj_option_result              imported;
    uint32_t                             title;

    printf("  a reader of its own, and scopes from the environment\n");

    memset(&heard, 0, sizeof(heard));
    env.pairs = PAIRS;
    env.count = 4u;
    env.asked = 0u;
    d_tests_registry_make(&registry, &printer, &printer_storage, &logger,
                          &logger_storage);
    d_tests_registry_setenv("DJSREG_PRINTER_STYLE", "text");
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_set_getenv(&registry,
                               d_tests_registry_reader, &env).is_ok);
    D_TESTS_REGISTRY_CHECK(D_TESTS_REGISTRY_STATUS(
                               d_option_env_registry_set_getenv(NULL, NULL,
                                                                NULL)) ==
                           (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);

    imported = d_option_env_registry_import_each(&registry, "DJSREG_",
                                                 d_tests_registry_heard,
                                                 &heard);
    title    = d_option_env_find(&printer, "title", 5u).payload.value;
    D_TESTS_REGISTRY_CHECK(env.asked != 0u);
    D_TESTS_REGISTRY_CHECK(strcmp(d_option_env_text(&printer, 0u,
                                      d_option_env_find(&printer, "style",
                                                        5u).payload.value),
                                  "json") == 0);
    D_TESTS_REGISTRY_CHECK(d_option_env_int(&logger, 0u,
                                            d_option_env_find(&logger,
                                                              "level", 5u)
                                                .payload.value) == 3);

#if D_CFG_OPTION_ENV_SCOPED_IMPORT
    D_TESTS_REGISTRY_CHECK(!imported.is_ok);
    D_TESTS_REGISTRY_CHECK(strcmp(d_option_env_text(&printer,
                                      d_option_env_find_scope(&printer,
                                          "language", 8u).payload.value,
                                      title),
                                  "le titre") == 0);
    D_TESTS_REGISTRY_CHECK( (heard.count == 1u) &&
                            (strcmp(heard.names,
                                    "DJSREG_PRINTER_COMPILER_KEY_WIDTH;")
                             == 0) &&
                            (strcmp(heard.settings,
                                    "printer.compiler.key_width;") == 0) );
#else
    D_TESTS_REGISTRY_CHECK( (imported.is_ok) &&
                            (heard.count == 0u) );
    D_TESTS_REGISTRY_CHECK(strcmp(d_option_env_text(&printer, 0u, title),
                                  "report") == 0);
#endif  // D_CFG_OPTION_ENV_SCOPED_IMPORT

    // given back to getenv
    D_TESTS_REGISTRY_CHECK(d_option_env_registry_set_getenv(&registry, NULL,
                                                            NULL).is_ok);
    (void)d_option_env_registry_import(&registry, "DJSREG_");
    D_TESTS_REGISTRY_CHECK(strcmp(d_option_env_text(&printer, 0u,
                                      d_option_env_find(&printer, "style",
                                                        5u).payload.value),
                                  "text") == 0);

    return;
}

/*
d_tests_registry_checking
  Tests the following:
  - "NAME=VALUE" by full name is checked in the frame the name addresses,
    as binding it from the source would, and its value written as a file
    needs it; nothing is bound
  - no '=', or no name, is INVALID_ARGUMENT; an unknown name KEY_ABSENT
  - the record names the frame the name addressed, found or not: a djin's
    unknown name is NAME there; a first part that names no djin, and that
    the root takes neither as its own name nor as a scope, is FRAME at the
    root; a root scope by that name takes the name, NAME when it lacks it
*/
static void
d_tests_registry_checking(void)
{
    static struct dj_option_env_registry registry;
    struct dj_tests_djin_storage         printer_storage =
                                             D_OPTION_ENV_STORAGE_INIT;
    struct dj_tests_djin_storage         logger_storage =
                                             D_OPTION_ENV_STORAGE_INIT;
    struct dj_option_env                 printer;
    struct dj_option_env                 logger;
    struct dj_option_result              result;
    char                                 buffer[32];
    const int32_t                        file = D_OPTION_ENV_SOURCE_FILE;

    printf("  checking a setting without binding it\n");

    d_tests_registry_make(&registry, &printer, &printer_storage, &logger,
                          &logger_storage);

    D_TESTS_REGISTRY_CHECK( (d_option_env_registry_check(&registry,
                                 "printer.style=json", 18u, file, buffer,
                                 sizeof(buffer), &result) == 4u) &&
                            (result.is_ok)                       &&
                            (strcmp(buffer, "json") == 0) );
    D_TESTS_REGISTRY_CHECK( (d_option_env_registry_check(&registry,
                                 "printer.language.title=a b ", 27u, file,
                                 buffer, sizeof(buffer), &result) == 6u) &&
                            (strcmp(buffer, "\"a b \"") == 0) );
    D_TESTS_REGISTRY_CHECK( (d_option_env_registry_check(&registry,
                                 "color=yes", 9u, file, buffer,
                                 sizeof(buffer), &result) == 4u) &&
                            (strcmp(buffer, "true") == 0) );
    D_TESTS_REGISTRY_CHECK( (d_option_env_registry_check(&registry,
                                 "logger.level=9", 14u, file, buffer,
                                 sizeof(buffer), &result) == (size_t)-1) &&
                            (D_TESTS_REGISTRY_STATUS(result) ==
                             (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT) &&
                            (buffer[0] == '\0') );
    D_TESTS_REGISTRY_CHECK( (d_option_env_registry_check(&registry,
                                 "printer.nothing=1", 17u, file, buffer,
                                 sizeof(buffer), &result) == (size_t)-1) &&
                            (D_TESTS_REGISTRY_STATUS(result) ==
                             (int32_t)D_OPTION_STATUS_KEY_ABSENT) );
    D_TESTS_REGISTRY_CHECK( (d_option_env_registry_check(&registry,
                                 "printer.style", 13u, file, buffer,
                                 sizeof(buffer), &result) == (size_t)-1) &&
                            (D_TESTS_REGISTRY_STATUS(result) ==
                             (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT) );
    D_TESTS_REGISTRY_CHECK( (d_option_env_registry_check(&registry,
                                 "=json", 5u, file, buffer, sizeof(buffer),
                                 &result) == (size_t)-1) &&
                            (D_TESTS_REGISTRY_STATUS(result) ==
                             (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT) );

    // the name and the value apart: the value never split again, and the
    // record says where the name resolved, and why a value is refused
    {
        struct dj_option_env_checked record;

        record.size = (uint32_t)sizeof(record);
        D_TESTS_REGISTRY_CHECK( (d_option_env_registry_check_from(&registry,
                                     "printer.language.title", 22u, "a;b",
                                     3u, file, buffer, sizeof(buffer),
                                     &result, &record) == 5u) &&
                                (strcmp(buffer, "\"a;b\"") == 0) &&
                                (record.frame == &printer)       &&
                                (record.scope != 0u)             &&
                                (record.var == result.payload.value) &&
                                (record.reason ==
                                 (int32_t)D_OPTION_ENV_REASON_NONE) );
        D_TESTS_REGISTRY_CHECK( (d_option_env_registry_check_from(&registry,
                                     "logger.level", 12u, "9", 1u, file,
                                     buffer, sizeof(buffer), &result,
                                     &record) == (size_t)-1) &&
                                (record.frame == &logger) &&
                                (record.reason ==
                                 (int32_t)D_OPTION_ENV_REASON_RANGE) );
        D_TESTS_REGISTRY_CHECK( (d_option_env_registry_check_from(&registry,
                                     "printer.nothing", 15u, "1", 1u, file,
                                     NULL, 0u, &result, &record) ==
                                 (size_t)-1) &&
                                (record.frame == &printer) &&
                                (record.var == D_OPTION_ENV_NONE) &&
                                (record.reason ==
                                 (int32_t)D_OPTION_ENV_REASON_NAME) );

        // no djin net, and nothing in the root by that name
        D_TESTS_REGISTRY_CHECK( (d_option_env_registry_check_from(&registry,
                                     "net.timeout", 11u, "5", 1u, file,
                                     NULL, 0u, &result, &record) ==
                                 (size_t)-1) &&
                                (D_TESTS_REGISTRY_STATUS(result) ==
                                 (int32_t)D_OPTION_STATUS_KEY_ABSENT) &&
                                (record.frame == &registry.frame) &&
                                (record.var == D_OPTION_ENV_NONE) &&
                                (record.reason ==
                                 (int32_t)D_OPTION_ENV_REASON_FRAME) );

        // the root's own name leads its own settings, known or not
        D_TESTS_REGISTRY_CHECK( (d_option_env_registry_check_from(&registry,
                                     "tests.color", 11u, "yes", 3u, file,
                                     buffer, sizeof(buffer), &result,
                                     &record) == 4u) &&
                                (record.frame == &registry.frame) &&
                                (record.reason ==
                                 (int32_t)D_OPTION_ENV_REASON_NONE) );
        D_TESTS_REGISTRY_CHECK( (d_option_env_registry_check_from(&registry,
                                     "tests.nothing", 13u, "1", 1u, file,
                                     NULL, 0u, &result, &record) ==
                                 (size_t)-1) &&
                                (record.frame == &registry.frame) &&
                                (record.reason ==
                                 (int32_t)D_OPTION_ENV_REASON_NAME) );

        // a root scope named net takes net's names: known, or NAME
        D_TESTS_REGISTRY_CHECK(d_option_env_set_scopes(&registry.frame,
                                   D_TESTS_ROOT_SCOPES, 1u).is_ok);
        D_TESTS_REGISTRY_CHECK( (d_option_env_registry_check_from(&registry,
                                     "net.timeout", 11u, "5", 1u, file,
                                     NULL, 0u, &result, &record) ==
                                 (size_t)-1) &&
                                (record.frame == &registry.frame) &&
                                (record.scope != D_OPTION_ENV_GLOBAL) &&
                                (record.var == D_OPTION_ENV_NONE) &&
                                (record.reason ==
                                 (int32_t)D_OPTION_ENV_REASON_NAME) );
        D_TESTS_REGISTRY_CHECK( (d_option_env_registry_check_from(&registry,
                                     "net.color", 9u, "yes", 3u, file,
                                     buffer, sizeof(buffer), &result,
                                     &record) == 4u) &&
                                (record.frame == &registry.frame) &&
                                (record.scope != D_OPTION_ENV_GLOBAL) &&
                                (record.reason ==
                                 (int32_t)D_OPTION_ENV_REASON_NONE) );
        D_TESTS_REGISTRY_CHECK(d_option_env_registry_check_from(NULL, "x",
                                   1u, "1", 1u, file, NULL, 0u, NULL,
                                   &record) == (size_t)-1);
    }

    // nothing was bound
    D_TESTS_REGISTRY_CHECK(d_option_env_source(&printer, 0u, 0u) ==
                           D_OPTION_ENV_SOURCE_DEFAULT);

    return;
}

/*
main
  Every group, then the count.
*/
int
main(void)
{
    printf("t_option_env_registry\n");

    d_tests_registry_program();
    d_tests_registry_djins();
    d_tests_registry_names();
    d_tests_registry_environment();
    d_tests_registry_import_each();
    d_tests_registry_scoped();
    d_tests_registry_checking();
    d_tests_registry_command_line();
    d_tests_registry_suggestions();

    printf("%lu checks, %lu failed\n",
           d_tests_registry_checks,
           d_tests_registry_failures);

    return (d_tests_registry_failures == 0u) ? 0 : 1;
}
