/*******************************************************************************
* djinterp [c/parse]                                       settings_tests_rows.c
*
* Parse's settings suite: its fixtures, and the tests of both blocks, "rows"
* and "core"; where the group is compiled out, the same tests, skipped.
*
*
* path:      /tests/djinterp/c/modules/parse/settings/settings_tests_rows.c
* link(s):   /docs/djinterp/option/knob_and_option.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/
#include "./settings_tests.h"  // corresponding header


#if (D_INTERNAL_PARSE_SETTINGS == 1)


// classes and operators of the fixtures' signature
#define D_TESTS_SETTINGS_NAME   1u
#define D_TESTS_SETTINGS_MINUS  2u
#define D_TESTS_SETTINGS_NEG    1u

// D_TESTS_SETTINGS_TOKENS
//   constant: the most tokens a fixture's parse reads.
#define D_TESTS_SETTINGS_TOKENS 64u


//==============================================================================
// FIXTURES
//==============================================================================


/*
d_tests_settings_open
  Makes a registry of the test's own and registers the djin in it.
*/
bool
d_tests_settings_open(
    struct dj_option_env_registry* _registry
)
{
    return ( (d_option_env_registry_init(_registry, "djinterp").is_ok) &&
             (d_parse_register(_registry).is_ok) );
}

/*
d_tests_settings_close
  Takes the djin off its registry.
*/
void
d_tests_settings_close(void)
{
    (void)d_parse_unregister();

    return;
}

/*
d_tests_settings_putenv
  Sets one variable of the process's environment, or removes it for NULL.
*/
static void
d_tests_settings_putenv(
    const char* _name,
    const char* _value
)
{
#if defined(_WIN32)
    static char line[128];
    size_t      at = 0u;

    for (const char* c = _name; ( (*c) && (at < sizeof(line) - 2u) ); ++c)
    {
        line[at++] = *c;
    }

    line[at++] = '=';

    for (const char* c = (_value) ? _value : "";
         ( (*c) && (at < sizeof(line) - 1u) );
         ++c)
    {
        line[at++] = *c;
    }

    line[at] = '\0';
    (void)_putenv(line);
#else
    if (_value)
    {
        (void)setenv(_name, _value, 1);
    }
    else
    {
        (void)unsetenv(_name);
    }
#endif

    return;
}

/*
d_tests_settings_depth
  Parses `- - ... - a`, `_negations` signs deep, with the guard option given
(0 for the setting in force).
  Returns D_PARSE_EXPR_OK, or the error's code (D_PARSE_EXPR_DIAG_TOO_DEEP
past the guard), or -1 where the fixture could not be built.
*/
int
d_tests_settings_depth(
    uint32_t _negations,
    uint32_t _max_depth
)
{
    struct dj_parse_expr_op           ops[2];
    struct dj_parse_expr_role         roles[2];
    struct dj_parse_expr_node         nodes[D_TESTS_SETTINGS_TOKENS];
    uint32_t                          args[D_TESTS_SETTINGS_TOKENS];
    struct dj_parse_expr_signature    s;
    struct dj_parse_expr_term         t;
    struct dj_parse_expr_token        tokens[D_TESTS_SETTINGS_TOKENS];
    struct dj_parse_expr_array_source state;
    struct dj_parse_expr_source       source;
    struct dj_parse_expr_options      options;
    struct dj_parse_expr_result       result;
    uint32_t                          count = 0u;
    int                               got   = -1;

    memset(tokens, 0, sizeof(tokens));
    memset(&options, 0, sizeof(options));
    options.max_depth = _max_depth;

    for ( ;
          ( (count < _negations) && (count + 1u < D_TESTS_SETTINGS_TOKENS) );
          ++count)
    {
        tokens[count].cls    = D_TESTS_SETTINGS_MINUS;
        tokens[count].offset = count * 2u;
        tokens[count].length = 1u;
    }

    tokens[count].cls    = D_TESTS_SETTINGS_NAME;
    tokens[count].offset = count * 2u;
    tokens[count].length = 1u;
    ++count;

    // the caller's storage, so the fixture stands where the heap does not
    d_parse_expr_signature_init(&s, ops, 2u, roles, 2u);
    d_parse_expr_term_init(&t, nodes, D_TESTS_SETTINGS_TOKENS, args,
                           D_TESTS_SETTINGS_TOKENS);

    if ( (d_parse_expr_define_atom(&s, D_TESTS_SETTINGS_NAME, 0u))         &&
         (d_parse_expr_define_op(&s, D_TESTS_SETTINGS_NEG,
                                 D_PARSE_EXPR_FORM_PREFIX, 20,
                                 D_PARSE_EXPR_ASSOC_RIGHT, 0u, "-"))       &&
         (d_parse_expr_define_token(&s, D_TESTS_SETTINGS_MINUS,
                                    D_PARSE_EXPR_OPERAND,
                                    D_TESTS_SETTINGS_NEG, 0u))             &&
         (d_parse_expr_signature_seal(&s, NULL)) )
    {
        d_parse_expr_source_array(&source, &state, tokens, count,
                                  count * 2u);
        got = d_parse_expr_parse(&s, &source, &options, &t, NULL, &result);
        got = (got == D_PARSE_EXPR_OK) ? D_PARSE_EXPR_OK : (int)result.code;
    }

    d_parse_expr_term_release(&t);
    d_parse_expr_signature_release(&s);

    return got;
}

/*
d_tests_settings_id
  A row's id in the djin.
*/
static uint32_t
d_tests_settings_id(
    int _setting
)
{
    return d_parse_setting_id(_setting);
}


//==============================================================================
// BLOCK "rows"
//==============================================================================


/*
d_tests_settings_defaults
  The rows before anything binds them.
  Tests the following:
  - before the djin is registered, the settings in force are the knobs
  - registered and unbound, each row reads its knob, from DEFAULT, and names
    the knob it is
*/
bool
d_tests_settings_defaults(void)
{
    static struct dj_option_env_registry registry;
    struct dj_parse_expr_settings        in_force;
    struct dj_option_env_value           value;

    d_parse_expr_settings(&in_force);
    D_TEST_REQUIRE(in_force.max_depth == (uint32_t)D_CFG_PARSE_EXPR_MAX_DEPTH);
    D_TEST_REQUIRE(in_force.max_errors ==
                   (uint32_t)D_CFG_PARSE_EXPR_MAX_ERRORS);
    D_TEST_REQUIRE(in_force.error_order ==
                   (uint8_t)D_CFG_PARSE_EXPR_ERROR_ORDER);

    D_TEST_REQUIRE(d_tests_settings_open(&registry));

    const uint32_t depth = d_tests_settings_id(D_PARSE_SETTING_EXPR_MAX_DEPTH);
    const bool     read  = d_option_env_get(d_parse_djin(), D_OPTION_ENV_GLOBAL,
                                            depth, &value).is_ok;
    const struct dj_option_env_var* row = d_option_env_var_at(d_parse_djin(),
                                                              depth);

    d_parse_expr_settings(&in_force);
    d_tests_settings_close();

    D_TEST_REQUIRE(read);
    D_TEST_REQUIRE(value.integer == (int64_t)D_CFG_PARSE_EXPR_MAX_DEPTH);
    D_TEST_REQUIRE(value.source == D_OPTION_ENV_SOURCE_DEFAULT);
    D_TEST_REQUIRE( (row) && (row->knob) &&
                    (strcmp(row->knob, "D_CFG_PARSE_EXPR_MAX_DEPTH") == 0) );
    D_TEST_REQUIRE(in_force.max_depth == (uint32_t)D_CFG_PARSE_EXPR_MAX_DEPTH);

    return true;
}

/*
d_tests_settings_sources
  A row bound from each source, by its full name.
  Tests the following:
  - a file's `parse.expr_max_depth = 12`
  - the environment's DJINTERP_PARSE_EXPR_MAX_ERRORS
  - the command line's `--parse.expr_error_order=structure_first`
  - the settings in force follow each
*/
bool
d_tests_settings_sources(void)
{
    static struct dj_option_env_registry registry;
    static char  name[]   = "t";
    static char  option[] = "--parse.expr_error_order=structure_first";
    char* const  argv[]   = { name, option, NULL };
    struct dj_parse_expr_settings in_force;
    struct dj_option_result       taken;
    size_t                        offset = 0u;
    int                           at     = 1;

    D_TEST_REQUIRE(d_tests_settings_open(&registry));

    const bool file = d_option_env_registry_load(&registry,
                          "parse.expr_max_depth = 12\n",
                          D_OPTION_ENV_SOURCE_FILE, &offset).is_ok;

    d_tests_settings_putenv("DJINTERP_PARSE_EXPR_MAX_ERRORS", "7");

    const bool environment =
        d_option_env_registry_import(&registry, "DJINTERP_").is_ok;
    const int  command     =
        d_option_env_registry_take_option(&registry, 2, argv, &at, &taken);

    d_tests_settings_putenv("DJINTERP_PARSE_EXPR_MAX_ERRORS", NULL);
    d_parse_expr_settings(&in_force);
    d_tests_settings_close();

    D_TEST_REQUIRE(file);
    D_TEST_REQUIRE(environment);
    D_TEST_REQUIRE(command == 1);
    D_TEST_REQUIRE(in_force.max_depth == 12u);
    D_TEST_REQUIRE(in_force.max_errors == 7u);
    D_TEST_REQUIRE(in_force.error_order ==
                   D_PARSE_EXPR_ORDER_STRUCTURE_FIRST);

    return true;
}

/*
d_tests_settings_standing
  Two sources on one row, and a forced value.
  Tests the following:
  - the command line holds over a file, whichever bound first
  - a forced value holds over both, and taking it off gives the command
    line's back
  - taking the command line's off gives the file's
*/
bool
d_tests_settings_standing(void)
{
    static struct dj_option_env_registry registry;
    struct dj_parse_expr_settings        s[4];
    struct dj_option_env_value           forced;

    D_TEST_REQUIRE(d_tests_settings_open(&registry));

    const uint32_t errors = d_tests_settings_id(
                                D_PARSE_SETTING_EXPR_MAX_ERRORS);

    memset(&forced, 0, sizeof(forced));
    forced.kind    = D_OPTION_ENV_KIND_INT;
    forced.integer = 9;

    (void)d_option_env_registry_put(&registry, "parse.expr_max_errors = 5",
                                    D_OPTION_ENV_SOURCE_COMMAND_LINE);
    (void)d_option_env_registry_put(&registry, "parse.expr_max_errors = 3",
                                    D_OPTION_ENV_SOURCE_FILE);
    d_parse_expr_settings(&s[0]);
    (void)d_option_env_set_value(d_parse_djin(), D_OPTION_ENV_GLOBAL, errors,
                                 D_OPTION_ENV_SOURCE_OVERRIDE, &forced);
    d_parse_expr_settings(&s[1]);
    (void)d_option_env_unset_from(d_parse_djin(), D_OPTION_ENV_GLOBAL,
                                  errors, D_OPTION_ENV_SOURCE_OVERRIDE);
    d_parse_expr_settings(&s[2]);
    (void)d_option_env_unset_from(d_parse_djin(), D_OPTION_ENV_GLOBAL,
                                  errors, D_OPTION_ENV_SOURCE_COMMAND_LINE);
    d_parse_expr_settings(&s[3]);
    d_tests_settings_close();

    D_TEST_REQUIRE(s[0].max_errors == 5u);
    D_TEST_REQUIRE(s[1].max_errors == 9u);
    D_TEST_REQUIRE(s[2].max_errors == 5u);
    D_TEST_REQUIRE(s[3].max_errors == 3u);

    return true;
}

/*
d_tests_settings_named
  The depth guard's named value, and its range.
  Tests the following:
  - `unbounded`, in any case, is D_PARSE_EXPR_DEPTH_UNBOUNDED, in force too
  - it is written back by its name
  - 0 and 65536 are refused, and the row keeps its value
*/
bool
d_tests_settings_named(void)
{
    static struct dj_option_env_registry registry;
    struct dj_parse_expr_settings        in_force;
    char                                 text[32];

    D_TEST_REQUIRE(d_tests_settings_open(&registry));

    const uint32_t depth = d_tests_settings_id(D_PARSE_SETTING_EXPR_MAX_DEPTH);
    const bool     named =
        d_option_env_registry_put(&registry, "parse.expr_max_depth = UNBOUNDED",
                                  D_OPTION_ENV_SOURCE_PROGRAM).is_ok;
    const size_t   written = d_option_env_format(d_parse_djin(),
                                                 D_OPTION_ENV_GLOBAL, depth,
                                                 text, sizeof(text));
    const struct dj_option_result zero =
        d_option_env_registry_put(&registry, "parse.expr_max_depth = 0",
                                  D_OPTION_ENV_SOURCE_PROGRAM);
    const struct dj_option_result wide =
        d_option_env_registry_put(&registry, "parse.expr_max_depth = 65536",
                                  D_OPTION_ENV_SOURCE_PROGRAM);

    d_parse_expr_settings(&in_force);
    d_tests_settings_close();

    D_TEST_REQUIRE(named);
    D_TEST_REQUIRE( (written > 0u) && (strcmp(text, "unbounded") == 0) );
    D_TEST_REQUIRE( (!zero.is_ok) &&
                    (zero.payload.error == D_OPTION_STATUS_INVALID_ARGUMENT) );
    D_TEST_REQUIRE( (!wide.is_ok) &&
                    (wide.payload.error == D_OPTION_STATUS_INVALID_ARGUMENT) );
    D_TEST_REQUIRE(in_force.max_depth == D_PARSE_EXPR_DEPTH_UNBOUNDED);

    return true;
}

/*
d_tests_settings_suggest
  A misspelt row on the command line.
  Tests the following:
  - it is refused (-1), and the nearest name is the row's
*/
bool
d_tests_settings_suggest(void)
{
    static struct dj_option_env_registry registry;
    static char  name[]   = "t";
    static char  option[] = "--parse.expr_max_dept=3";
    char* const  argv[]   = { name, option, NULL };
    char         near[64];
    int          at = 1;

    memset(near, 0, sizeof(near));
    D_TEST_REQUIRE(d_tests_settings_open(&registry));

    const int    taken = d_option_env_registry_take_option(&registry, 2, argv,
                                                           &at, NULL);
    const size_t found = d_option_env_registry_suggest(&registry,
                                                       "parse.expr_max_dept",
                                                       19u, near,
                                                       sizeof(near));

    d_tests_settings_close();

    D_TEST_REQUIRE(taken == -1);
    D_TEST_REQUIRE( (found > 0u) &&
                    (strcmp(near, "parse.expr_max_depth") == 0) );

    return true;
}


//==============================================================================
// BLOCK "core"
//==============================================================================


/*
d_tests_settings_parse
  A parse whose depth guard option is 0, and one that gives it.
  Tests the following:
  - with the row at 3, three signs read and four are too deep
  - an option of 10 holds over the row
*/
bool
d_tests_settings_parse(void)
{
    static struct dj_option_env_registry registry;

    D_TEST_REQUIRE(d_tests_settings_open(&registry));

    const bool bound =
        d_option_env_registry_put(&registry, "parse.expr_max_depth = 3",
                                  D_OPTION_ENV_SOURCE_PROGRAM).is_ok;
    const int  under = d_tests_settings_depth(2u, 0u);
    const int  over  = d_tests_settings_depth(4u, 0u);
    const int  given = d_tests_settings_depth(4u, 10u);

    d_tests_settings_close();

    if ( (under != D_PARSE_EXPR_OK) ||
         (over != (int)D_PARSE_EXPR_DIAG_TOO_DEEP) ||
         (given != D_PARSE_EXPR_OK) )
    {
        D_TEST_NOTE("under %d, over %d, given %d", under, over, given);
    }

    D_TEST_REQUIRE(bound);
    D_TEST_REQUIRE(under == D_PARSE_EXPR_OK);
    D_TEST_REQUIRE(over == (int)D_PARSE_EXPR_DIAG_TOO_DEEP);
    D_TEST_REQUIRE(given == D_PARSE_EXPR_OK);

    return true;
}

/*
d_tests_settings_unregister
  The djin taken off, and registered again.
  Tests the following:
  - off the registry, the settings in force are the knobs, though a row was
    bound
  - registered again, its rows read their knobs: a djin's own bindings go
    when it gains a parent
*/
bool
d_tests_settings_unregister(void)
{
    static struct dj_option_env_registry registry;
    struct dj_parse_expr_settings        s[3];

    D_TEST_REQUIRE(d_tests_settings_open(&registry));
    (void)d_option_env_registry_put(&registry, "parse.expr_max_errors = 2",
                                    D_OPTION_ENV_SOURCE_PROGRAM);
    d_parse_expr_settings(&s[0]);
    d_tests_settings_close();
    d_parse_expr_settings(&s[1]);

    D_TEST_REQUIRE(d_parse_register(&registry).is_ok);
    d_parse_expr_settings(&s[2]);
    d_tests_settings_close();

    D_TEST_REQUIRE(s[0].max_errors == 2u);
    D_TEST_REQUIRE(s[1].max_errors == (uint32_t)D_CFG_PARSE_EXPR_MAX_ERRORS);
    D_TEST_REQUIRE(s[2].max_errors == (uint32_t)D_CFG_PARSE_EXPR_MAX_ERRORS);

    return true;
}


#else


// D_TESTS_SETTINGS_WITHOUT
//   constant: the reason each test gives where the group is compiled out.
#define D_TESTS_SETTINGS_WITHOUT \
    "the settings group is compiled out (D_CFG_PARSE_SETTINGS 0)"

/*
d_tests_settings_open, d_tests_settings_close, d_tests_settings_depth
  The fixtures, where there is no group to fix: nothing opens, and no
parse is made.
*/
bool
d_tests_settings_open(
    struct dj_option_env_registry* _registry
)
{
    (void)_registry;

    return false;
}

void
d_tests_settings_close(void)
{
    return;
}

int
d_tests_settings_depth(
    uint32_t _negations,
    uint32_t _max_depth
)
{
    (void)_negations;
    (void)_max_depth;

    return -1;
}

/*
d_tests_settings_*
  Each test, skipped, saying why (F26).
*/
bool
d_tests_settings_defaults(void)
{
    D_TEST_SKIP(D_TESTS_SETTINGS_WITHOUT);
}

bool
d_tests_settings_sources(void)
{
    D_TEST_SKIP(D_TESTS_SETTINGS_WITHOUT);
}

bool
d_tests_settings_standing(void)
{
    D_TEST_SKIP(D_TESTS_SETTINGS_WITHOUT);
}

bool
d_tests_settings_named(void)
{
    D_TEST_SKIP(D_TESTS_SETTINGS_WITHOUT);
}

bool
d_tests_settings_suggest(void)
{
    D_TEST_SKIP(D_TESTS_SETTINGS_WITHOUT);
}

bool
d_tests_settings_parse(void)
{
    D_TEST_SKIP(D_TESTS_SETTINGS_WITHOUT);
}

bool
d_tests_settings_unregister(void)
{
    D_TEST_SKIP(D_TESTS_SETTINGS_WITHOUT);
}


#endif  // D_INTERNAL_PARSE_SETTINGS
