/*******************************************************************************
* djinterp [test]                                        t_option_env_registry.c
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
*                                                            revised: 2026.11.04
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
  - not settings, and settings named badly
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
d_tests_registry_command_line
  Tests the following:
  - "--set NAME=VALUE", "--NAME=VALUE", "--NAME", "--no-NAME", each bound
    from the command line
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
        (char*)"-"
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
    d_tests_registry_command_line();
    d_tests_registry_suggestions();

    printf("%lu checks, %lu failed\n",
           d_tests_registry_checks,
           d_tests_registry_failures);

    return (d_tests_registry_failures == 0u) ? 0 : 1;
}
