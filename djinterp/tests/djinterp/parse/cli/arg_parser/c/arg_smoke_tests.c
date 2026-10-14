/*******************************************************************************
* djinterp [parse]                                             arg_smoke_tests.c
*
* The argument parser's smoke suite, declared in arg_smoke_tests.h.
*   Every expected transcript, message and help text here is CLI11 2.7.2's,
* for the same definitions and arguments, but where a section says
* otherwise: a definition CLI11 refuses by throwing is reported by the next
* parse until it is cleared, and a bound variable a value does not convert
* into keeps its value, where CLI11's holds whatever its conversion wrote
* before failing.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_smoke_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "./arg_smoke_tests.h"  // corresponding header
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <string.h>   // strcmp
// djinterp
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_parser.h"  // the parser
#include "../../../c/tests_section.h"  // d_tests_section
#include "./arg_tests_support.h"  // runs and transcripts


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// d_tests_arg_smoke_tag
//   struct: a callback's context: the run it notes in, and what it notes.
struct d_tests_arg_smoke_tag
{
    struct d_tests_arg_run* run;
    const char*             label;
};

// d_tests_arg_smoke_values
//   struct: the variables d_tests_arg_smoke_bound binds, one of each kind it
// checks.
struct d_tests_arg_smoke_values
{
    int         count;
    double      ratio;
    int         verbosity;
    const char* name;
    bool        on;
};

// d_tests_arg_smoke_hooks
//   struct: the contexts of the callbacks d_tests_arg_smoke_callbacks
// registers, each noting its own label.
struct d_tests_arg_smoke_hooks
{
    struct d_tests_arg_smoke_tag a;
    struct d_tests_arg_smoke_tag b;
    struct d_tests_arg_smoke_tag sub;
    struct d_tests_arg_smoke_tag complete;
    struct d_tests_arg_smoke_tag last;
};

/*
d_tests_arg_smoke_on_values
  An option's callback: notes its label and the first value it was given.
*/
static int
d_tests_arg_smoke_on_values(
    struct d_arg_option*      _option,
    const struct d_pack_text* _results,
    size_t                    _count,
    void*                     _context
)
{
    const struct d_tests_arg_smoke_tag* const tag = _context;

    (void)_option;

    // the first value, when there is one
    if (_count > 0u)
    {
        d_tests_arg_note(tag->run,
                         "%s %.*s",
                         tag->label,
                         (int)_results[0].length,
                         _results[0].data);
    }

    return D_ARG_OK;
}

/*
d_tests_arg_smoke_on_flag
  A flag's callback: notes its label.
*/
static int
d_tests_arg_smoke_on_flag(
    struct d_arg_option* _option,
    long                 _count,
    void*                _context
)
{
    const struct d_tests_arg_smoke_tag* const tag = _context;

    (void)_option;
    (void)_count;

    d_tests_arg_note(tag->run,
                     "%s",
                     tag->label);

    return D_ARG_OK;
}

/*
d_tests_arg_smoke_on_command
  A command's callback: notes its label.
*/
static int
d_tests_arg_smoke_on_command(
    struct d_arg_command* _command,
    void*                 _context
)
{
    const struct d_tests_arg_smoke_tag* const tag = _context;

    (void)_command;

    d_tests_arg_note(tag->run,
                     "%s",
                     tag->label);

    return D_ARG_OK;
}

/*
d_tests_arg_smoke_entry
  A main of the kind D_ARG_PARSE is written for: what the macro returns when
the parse ends the program, and 42 when the program goes on.
*/
static int
d_tests_arg_smoke_entry(
    struct d_arg_parser* _parser,
    int                  _argc,
    char**               _argv
)
{
    D_ARG_PARSE(_parser,
                _argc,
                _argv);

    return 42;
}

/*
d_tests_arg_smoke_ended
  Ends a parse as d_arg_parser_exit does, and compares the code and what
was written to each output with what was expected.
*/
static bool
d_tests_arg_smoke_ended(
    struct d_tests_arg_run* _run,
    int                     _status,
    int                     _code,
    const char*             _out,
    const char*             _err
)
{
    const int  code = d_arg_parser_exit(&_run->parser,
                                        _status);
    const bool out  = d_tests_arg_same("output",
                                       d_arg_buffer_text(&_run->out),
                                       _out);
    const bool err  = d_tests_arg_same("error output",
                                       d_arg_buffer_text(&_run->err),
                                       _err);

    return ( (code == _code) &&
             (out)           &&
             (err) );
}

/*
d_tests_arg_smoke_bind
  Binds each of the variables to an option or a flag of its own, through
the built-in type descriptors.
*/
static void
d_tests_arg_smoke_bind(
    struct d_arg_command*            _app,
    struct d_tests_arg_smoke_values* _values
)
{
    (void)d_arg_command_add_option_bound(_app,
                                         "--count",
                                         &_values->count,
                                         d_arg_type_builtin(D_ARG_BUILTIN_INT),
                                         NULL);
    (void)d_arg_command_add_option_bound(
              _app,
              "--ratio",
              &_values->ratio,
              d_arg_type_builtin(D_ARG_BUILTIN_DOUBLE),
              NULL);
    (void)d_arg_command_add_flag_bound(_app,
                                       "-v",
                                       &_values->verbosity,
                                       d_arg_type_builtin(D_ARG_BUILTIN_INT),
                                       NULL);
    (void)d_arg_command_add_option_bound(
              _app,
              "--name",
              &_values->name,
              d_arg_type_builtin(D_ARG_BUILTIN_STRING),
              NULL);
    (void)d_arg_command_add_flag_bound(_app,
                                       "--on",
                                       &_values->on,
                                       d_arg_type_builtin(D_ARG_BUILTIN_BOOL),
                                       NULL);

    return;
}

/*
d_tests_arg_smoke_hook
  Registers a callback of each kind -- an option's, a flag's, a
subcommand's, and the root's parse-complete and final callbacks -- each with
its own context.
*/
static void
d_tests_arg_smoke_hook(
    struct d_tests_arg_run*         _run,
    struct d_arg_command*           _app,
    struct d_tests_arg_smoke_hooks* _hooks
)
{
    (void)d_arg_command_add_option_callback(_app,
                                            "--a",
                                            d_tests_arg_smoke_on_values,
                                            &_hooks->a,
                                            NULL,
                                            NULL);
    (void)d_arg_command_add_flag_callback(_app,
                                          "--b",
                                          d_tests_arg_smoke_on_flag,
                                          &_hooks->b,
                                          NULL,
                                          NULL);

    struct d_arg_command* const sub =
        d_tests_arg_watch_command(_run,
                                  "sub",
                                  d_arg_command_add_subcommand(_app,
                                                               "sub",
                                                               NULL));

    d_arg_command_set_callback(sub,
                               d_tests_arg_smoke_on_command,
                               &_hooks->sub,
                               NULL);
    d_arg_command_set_final_callback(_app,
                                     d_tests_arg_smoke_on_command,
                                     &_hooks->last,
                                     NULL);
    d_arg_command_set_parse_complete_callback(_app,
                                              d_tests_arg_smoke_on_command,
                                              &_hooks->complete,
                                              NULL);

    return;
}

/*
d_tests_arg_smoke_refuses
  Adds an option under a name already taken, and compares the definition
error it records with CLI11's OptionAlreadyAdded message.
*/
static bool
d_tests_arg_smoke_refuses(
    struct d_tests_arg_run* _run,
    struct d_arg_command*   _app
)
{
    static const char TAKEN[] =
        "cannot add '--alpha': long name '--alpha' already used by "
        "'--alpha'";
    const struct d_arg_error* const  error =
        d_arg_parser_definition_error(&_run->parser);
    const struct d_arg_option* const again =
        d_arg_command_add_option(_app,
                                 "--alpha",
                                 NULL);
    const bool                       same  =
        d_tests_arg_same("definition error",
                         d_pack_text_from_cstr(d_arg_error_message(error)),
                         TAKEN);

    return ( (again == NULL)                                          &&
             (error->status == (int)D_ARG_ERROR_OPTION_ALREADY_ADDED) &&
             (same) );
}

//==============================================================================
// 1.  THE COMMAND LINE
//==============================================================================

/*
d_tests_arg_smoke_mixed
  A flag, an option and a positional taking any number of values, with `--`.
  Tests the following:
  - a flag by its long name, and an option by its short name and a value
  - a positional takes each argument that is neither an option nor a value
  - after `--` every argument is positional, an option's name included, and
    the `--` itself is left over
*/
bool
d_tests_arg_smoke_mixed(void)
{
    static const char* const ARGS[] =
    {
        "--flag", "-o", "val", "a", "b", "--", "-c", "--flag"
    };
    static const char EXPECTED[] =
        "cmd app n=1 miss=[--] subs=[] order=[--flag|--opt|pos|pos|pos|pos]\n"
        "opt flag n=1 res=[true]\n"
        "opt opt n=1 res=[val]\n"
        "opt pos n=4 res=[a|b|-c|--flag]\n";
    struct d_tests_arg_run run = { 0 };
    struct d_arg_command*  app = d_tests_arg_begin(&run);

    (void)d_tests_arg_watch_option(&run,
                                   "flag",
                                   d_arg_command_add_flag(app,
                                                          "--flag,-f",
                                                          NULL));
    (void)d_tests_arg_watch_option(&run,
                                   "opt",
                                   d_arg_command_add_option(app,
                                                            "--opt,-o",
                                                            NULL));

    struct d_arg_option* const pos =
        d_tests_arg_watch_all(&run,
                              "pos",
                              d_arg_command_add_option(app,
                                                       "pos",
                                                       NULL));

    d_arg_option_set_expected(pos,
                              -1);

    return d_tests_arg_expect(&run,
                              ARGS,
                              sizeof(ARGS) / sizeof(ARGS[0]),
                              EXPECTED);
}

/*
d_tests_arg_smoke_short_clusters
  Short names run together, with values attached and following.
  Tests the following:
  - `-ab` is two flags, and `-bac` two flags and an option, which takes the
    next argument as its value
  - `-cval` gives `-c` the rest of its argument
  - an argument nothing takes is an ExtrasError naming the command
*/
bool
d_tests_arg_smoke_short_clusters(void)
{
    static const char* const ARGS[] =
    {
        "-ab", "-cval", "-bac", "7", "x"
    };
    static const char EXPECTED[] =
        "ERR ExtrasError 109: prog: The following argument was not "
        "expected: x\n"
        "cmd app n=1 miss=[x] subs=[] order=[-a|-b|-c|-b|-a|-c]\n"
        "opt a n=2 res=[true|true]\n"
        "opt b n=2 res=[true|true]\n"
        "opt c n=2 res=[val|7]\n";
    struct d_tests_arg_run run = { 0 };
    struct d_arg_command*  app = d_tests_arg_begin(&run);

    (void)d_tests_arg_watch_all(&run,
                                "a",
                                d_arg_command_add_flag(app,
                                                       "-a",
                                                       NULL));
    (void)d_tests_arg_watch_all(&run,
                                "b",
                                d_arg_command_add_flag(app,
                                                       "-b",
                                                       NULL));
    (void)d_tests_arg_watch_all(&run,
                                "c",
                                d_arg_command_add_option(app,
                                                         "-c",
                                                         NULL));

    return d_tests_arg_expect(&run,
                              ARGS,
                              sizeof(ARGS) / sizeof(ARGS[0]),
                              EXPECTED);
}

/*
d_tests_arg_smoke_flag_values
  A flag with a negated name, given values, and a short flag repeated.
  Tests the following:
  - `--flag=false` and `--no-flag` both give "false"
  - a value given to the negated name is inverted: `--no-flag=0` is "true"
  - `-vvv` is the flag three times
*/
bool
d_tests_arg_smoke_flag_values(void)
{
    static const char* const ARGS[] =
    {
        "--flag=false", "--no-flag", "-vvv", "--no-flag=0", "--flag"
    };
    static const char EXPECTED[] =
        "cmd app n=1 miss=[] subs=[] "
        "order=[--flag|--flag|-v|-v|-v|--flag|--flag]\n"
        "opt flag n=4 res=[false|false|true|true]\n"
        "opt v n=3 res=[true|true|true]\n";
    struct d_tests_arg_run run = { 0 };
    struct d_arg_command*  app = d_tests_arg_begin(&run);

    (void)d_tests_arg_watch_all(&run,
                                "flag",
                                d_arg_command_add_flag(app,
                                                       "--flag,!--no-flag",
                                                       NULL));
    (void)d_tests_arg_watch_all(&run,
                                "v",
                                d_arg_command_add_flag(app,
                                                       "-v",
                                                       NULL));

    return d_tests_arg_expect(&run,
                              ARGS,
                              sizeof(ARGS) / sizeof(ARGS[0]),
                              EXPECTED);
}

/*
d_tests_arg_smoke_subcommands
  A subcommand used twice, left with `++` in between.
  Tests the following:
  - a subcommand's option is taken while the subcommand is parsed
  - `++` returns to the parent, whose flag is then taken
  - naming the subcommand again uses it again: it counts two uses, and is
    listed once among the parent's subcommands
*/
bool
d_tests_arg_smoke_subcommands(void)
{
    static const char* const ARGS[] =
    {
        "sub1", "--x", "1", "++", "--flag", "sub1", "--x", "2"
    };
    static const char EXPECTED[] =
        "cmd app n=1 miss=[] subs=[sub1] order=[--flag]\n"
        "cmd sub1 n=2 miss=[] subs=[] order=[--x|--x]\n"
        "opt flag n=1 res=[true]\n"
        "opt x n=2 res=[1|2]\n";
    struct d_tests_arg_run run = { 0 };
    struct d_arg_command*  app = d_tests_arg_begin(&run);

    (void)d_tests_arg_watch_option(&run,
                                   "flag",
                                   d_arg_command_add_flag(app,
                                                          "--flag",
                                                          NULL));

    struct d_arg_command* const sub =
        d_tests_arg_watch_command(&run,
                                  "sub1",
                                  d_arg_command_add_subcommand(app,
                                                               "sub1",
                                                               NULL));

    (void)d_tests_arg_watch_all(&run,
                                "x",
                                d_arg_command_add_option(sub,
                                                         "--x",
                                                         NULL));

    return d_tests_arg_expect(&run,
                              ARGS,
                              sizeof(ARGS) / sizeof(ARGS[0]),
                              EXPECTED);
}

/*
d_tests_arg_smoke_bound
  Options and flags bound to variables of the built-in types.
  Tests the following:
  - an `int`, a `double` given after `=`, and text, which the variable
    points at in the parser's keeping
  - a flag bound to an `int` counts its uses: `-vvv` is 3
  - a flag bound to a `bool` sets it
*/
bool
d_tests_arg_smoke_bound(void)
{
    static const char* const ARGS[] =
    {
        "--count", "7", "--ratio=0.25", "-vvv", "--name", "two words", "--on"
    };
    static const char EXPECTED[] =
        "cmd app n=1 miss=[] subs=[] "
        "order=[--count|--ratio|-v|-v|-v|--name|--on]\n";
    struct d_tests_arg_run          run    = { 0 };
    struct d_arg_command*           app    = d_tests_arg_begin(&run);
    struct d_tests_arg_smoke_values values = { 0, 0.0, 0, NULL, false };

    d_tests_arg_smoke_bind(app,
                           &values);
    (void)d_tests_arg_parse(&run,
                            ARGS,
                            sizeof(ARGS) / sizeof(ARGS[0]));

    // read before the run ends, which frees the text `name` points at
    const bool converted = ( (values.count == 7)              &&
                             (values.ratio == 0.25)           &&
                             (values.verbosity == 3)          &&
                             (values.name != NULL)            &&
                             (strcmp(values.name,
                                     "two words") == 0)       &&
                             (values.on) );
    const bool reported  = d_tests_arg_matches(&run,
                                               EXPECTED);

    d_tests_arg_end(&run);

    return ( (converted) &&
             (reported) );
}

/*
d_tests_arg_smoke_callbacks
  The order callbacks run in once the command line is parsed.
  Tests the following:
  - options' callbacks first, in the order the options were made, not the
    order they were given
  - then the command's parse-complete callback, then its subcommands'
    callbacks, then its final callback
*/
bool
d_tests_arg_smoke_callbacks(void)
{
    static const char* const ARGS[] =
    {
        "--b", "--a", "1", "sub"
    };
    static const char EXPECTED[] =
        "a 1\n"
        "b\n"
        "complete\n"
        "sub\n"
        "final\n"
        "cmd app n=1 miss=[] subs=[sub] order=[--b|--a]\n"
        "cmd sub n=1 miss=[] subs=[] order=[]\n";
    struct d_tests_arg_run         run   = { 0 };
    struct d_arg_command*          app   = d_tests_arg_begin(&run);
    struct d_tests_arg_smoke_hooks hooks =
    {
        { &run, "a" },
        { &run, "b" },
        { &run, "sub" },
        { &run, "complete" },
        { &run, "final" }
    };

    d_tests_arg_smoke_hook(&run,
                           app,
                           &hooks);

    return d_tests_arg_expect(&run,
                              ARGS,
                              sizeof(ARGS) / sizeof(ARGS[0]),
                              EXPECTED);
}

//==============================================================================
// 2.  FAILURES
//==============================================================================

/*
d_tests_arg_smoke_required
  A required option not given.
  Tests the following:
  - the parse fails with RequiredError, exit code 106, "--x is required"
*/
bool
d_tests_arg_smoke_required(void)
{
    static const char EXPECTED[] =
        "ERR RequiredError 106: --x is required\n"
        "cmd app n=1 miss=[] subs=[] order=[]\n"
        "opt x n=0 res=[]\n";
    struct d_tests_arg_run     run = { 0 };
    struct d_arg_command*      app = d_tests_arg_begin(&run);
    struct d_arg_option* const x   =
        d_tests_arg_watch_option(&run,
                                 "x",
                                 d_arg_command_add_option(app,
                                                          "--x",
                                                          NULL));

    d_arg_option_set_required(x,
                              true);

    return d_tests_arg_expect(&run,
                              NULL,
                              0u,
                              EXPECTED);
}

/*
d_tests_arg_smoke_needs
  An option needing another that was not given.
  Tests the following:
  - the parse fails with RequiresError, exit code 107: "--a requires --b"
*/
bool
d_tests_arg_smoke_needs(void)
{
    static const char* const ARGS[] =
    {
        "--a"
    };
    static const char EXPECTED[] =
        "ERR RequiresError 107: --a requires --b\n"
        "cmd app n=1 miss=[] subs=[] order=[--a]\n"
        "opt a n=1 res=[true]\n"
        "opt b n=0 res=[]\n";
    struct d_tests_arg_run     run = { 0 };
    struct d_arg_command*      app = d_tests_arg_begin(&run);
    struct d_arg_option* const a   =
        d_tests_arg_watch_option(&run,
                                 "a",
                                 d_arg_command_add_flag(app,
                                                        "--a",
                                                        NULL));
    struct d_arg_option* const b   =
        d_tests_arg_watch_option(&run,
                                 "b",
                                 d_arg_command_add_flag(app,
                                                        "--b",
                                                        NULL));

    (void)d_arg_option_needs(a,
                             b);

    return d_tests_arg_expect(&run,
                              ARGS,
                              sizeof(ARGS) / sizeof(ARGS[0]),
                              EXPECTED);
}

/*
d_tests_arg_smoke_excludes
  An option excluding another, the two given in the reverse order.
  Tests the following:
  - the parse fails with ExcludesError, exit code 108, named from the
    option that excludes: "--c excludes --d"
*/
bool
d_tests_arg_smoke_excludes(void)
{
    static const char* const ARGS[] =
    {
        "--d", "--c"
    };
    static const char EXPECTED[] =
        "ERR ExcludesError 108: --c excludes --d\n"
        "cmd app n=1 miss=[] subs=[] order=[--d|--c]\n"
        "opt c n=1 res=[true]\n"
        "opt d n=1 res=[true]\n";
    struct d_tests_arg_run     run = { 0 };
    struct d_arg_command*      app = d_tests_arg_begin(&run);
    struct d_arg_option* const c   =
        d_tests_arg_watch_option(&run,
                                 "c",
                                 d_arg_command_add_flag(app,
                                                        "--c",
                                                        NULL));
    struct d_arg_option* const d   =
        d_tests_arg_watch_option(&run,
                                 "d",
                                 d_arg_command_add_flag(app,
                                                        "--d",
                                                        NULL));

    (void)d_arg_option_excludes(c,
                                d);

    return d_tests_arg_expect(&run,
                              ARGS,
                              sizeof(ARGS) / sizeof(ARGS[0]),
                              EXPECTED);
}

/*
d_tests_arg_smoke_missing_value
  An option given as the last argument, with no value after it.
  Tests the following:
  - ArgumentMismatch, exit code 114, worded as CLI11 words it, two spaces
    included
  - the option holds no value
*/
bool
d_tests_arg_smoke_missing_value(void)
{
    static const char* const ARGS[] =
    {
        "--opt"
    };
    static const char EXPECTED[] =
        "ERR ArgumentMismatch 114: --opt: 1 required  missing\n"
        "cmd app n=1 miss=[] subs=[] order=[]\n"
        "opt opt n=0 res=[]\n";
    struct d_tests_arg_run run = { 0 };
    struct d_arg_command*  app = d_tests_arg_begin(&run);

    (void)d_tests_arg_watch_option(&run,
                                   "opt",
                                   d_arg_command_add_option(app,
                                                            "--opt",
                                                            NULL));

    return d_tests_arg_expect(&run,
                              ARGS,
                              sizeof(ARGS) / sizeof(ARGS[0]),
                              EXPECTED);
}

/*
d_tests_arg_smoke_conversion
  A value a bound `int` cannot take.
  Tests the following:
  - ConversionError, exit code 104: "Could not convert: --count = x7"
  - the failure message, with the help flag's hint
  - the variable keeps the value it had, where CLI11's holds whatever its
    conversion wrote before failing -- 0, for `x7` into an `int`
*/
bool
d_tests_arg_smoke_conversion(void)
{
    static const char* const ARGS[] =
    {
        "--count", "x7"
    };
    static const char EXPECTED[] =
        "ERR ConversionError 104: Could not convert: --count = x7\n"
        "cmd app n=1 miss=[] subs=[] order=[--count]\n"
        "opt count n=1 res=[x7]\n";
    static const char MESSAGE[] =
        "Could not convert: --count = x7\n"
        "Run with --help for more information.\n";
    struct d_tests_arg_run run   = { 0 };
    struct d_arg_command*  app   = d_tests_arg_begin(&run);
    int                    count = 5;

    (void)d_tests_arg_watch_option(
              &run,
              "count",
              d_arg_command_add_option_bound(
                  app,
                  "--count",
                  &count,
                  d_arg_type_builtin(D_ARG_BUILTIN_INT),
                  NULL));

    const int  status   = d_tests_arg_parse(&run,
                                            ARGS,
                                            sizeof(ARGS) / sizeof(ARGS[0]));
    const bool reported = d_tests_arg_matches(&run,
                                              EXPECTED);
    const bool ended    = d_tests_arg_smoke_ended(
                              &run,
                              status,
                              (int)D_ARG_EXIT_CONVERSION_ERROR,
                              "",
                              MESSAGE);

    d_tests_arg_end(&run);

    return ( (reported)    &&
             (ended)       &&
             (count == 5) );
}

/*
d_tests_arg_smoke_definition_error
  Definitions CLI11 refuses by throwing, recorded instead.
  Tests the following:
  - a short name of two letters is refused, with CLI11's BadNameString
    message, and a name already taken with its OptionAlreadyAdded message
  - each parse reports the error, without parsing, until it is cleared
  - once cleared, the parse goes ahead with the definitions that were made
*/
bool
d_tests_arg_smoke_definition_error(void)
{
    static const char* const ARGS[] =
    {
        "--alpha", "1"
    };
    static const char REFUSED[] =
        "ERR BadNameString 101: Long names strings require 2 dashes -ab\n"
        "cmd app n=0 miss=[] subs=[] order=[]\n"
        "opt alpha n=0 res=[]\n";
    static const char PARSED[] =
        "cmd app n=1 miss=[] subs=[] order=[--alpha]\n"
        "opt alpha n=1 res=[1]\n";
    struct d_tests_arg_run           run   = { 0 };
    struct d_arg_command*            app   = d_tests_arg_begin(&run);
    const struct d_arg_error* const  error =
        d_arg_parser_definition_error(&run.parser);
    const struct d_arg_option* const bad   = d_arg_command_add_option(app,
                                                                      "-ab",
                                                                      NULL);

    (void)d_tests_arg_watch_option(&run,
                                   "alpha",
                                   d_arg_command_add_option(app,
                                                            "--alpha",
                                                            NULL));
    (void)d_tests_arg_parse(&run,
                            ARGS,
                            sizeof(ARGS) / sizeof(ARGS[0]));

    const bool refused = d_tests_arg_matches(&run,
                                             REFUSED);

    d_arg_parser_clear_definition_error(&run.parser);

    const bool cleared = (error->status == (int)D_ARG_OK);
    const bool taken   = d_tests_arg_smoke_refuses(&run,
                                                   app);

    d_arg_parser_clear_definition_error(&run.parser);
    (void)d_tests_arg_parse(&run,
                            ARGS,
                            sizeof(ARGS) / sizeof(ARGS[0]));

    const bool parsed = d_tests_arg_matches(&run,
                                            PARSED);

    d_tests_arg_end(&run);

    return ( (bad == NULL) &&
             (refused)     &&
             (cleared)     &&
             (taken)       &&
             (parsed) );
}

//==============================================================================
// 3.  ENDING
//==============================================================================

/*
d_tests_arg_smoke_help
  The help flag, given after an option.
  Tests the following:
  - the parse ends with CallForHelp, after taking what came before it
  - d_arg_parser_exit returns 0 and writes CLI11's help to the output
*/
bool
d_tests_arg_smoke_help(void)
{
    static const char* const ARGS[] =
    {
        "--count", "2", "--help"
    };
    static const char EXPECTED[] =
        "ERR CallForHelp 0: This should be caught in your main function, "
        "see examples\n"
        "cmd app n=1 miss=[] subs=[] order=[--count|--help]\n"
        "opt count n=1 res=[2]\n";
    static const char HELP[] =
        "d\n"
        "\n"
        "\n"
        "prog [OPTIONS]\n"
        "\n"
        "\n"
        "OPTIONS:\n"
        "  -h,     --help              Print this help message and exit\n"
        "  -c,     --count             How many\n";
    struct d_tests_arg_run run = { 0 };
    struct d_arg_command*  app = d_tests_arg_begin(&run);

    (void)d_tests_arg_watch_option(&run,
                                   "count",
                                   d_arg_command_add_option(app,
                                                            "--count,-c",
                                                            "How many"));

    const int  status   = d_tests_arg_parse(&run,
                                            ARGS,
                                            sizeof(ARGS) / sizeof(ARGS[0]));
    const bool reported = d_tests_arg_matches(&run,
                                              EXPECTED);
    const bool ended    = d_tests_arg_smoke_ended(&run,
                                                  status,
                                                  (int)D_ARG_EXIT_SUCCESS,
                                                  HELP,
                                                  "");

    d_tests_arg_end(&run);

    return ( (reported) &&
             (ended) );
}

/*
d_tests_arg_smoke_version
  The version flag.
  Tests the following:
  - the parse ends with CallForVersion, the version as its message
  - d_arg_parser_exit returns 0 and writes the version and a newline
*/
bool
d_tests_arg_smoke_version(void)
{
    static const char* const ARGS[] =
    {
        "--version"
    };
    static const char EXPECTED[] =
        "ERR CallForVersion 0: 1.2.3\n"
        "cmd app n=1 miss=[] subs=[] order=[--version]\n";
    struct d_tests_arg_run run = { 0 };
    struct d_arg_command*  app = d_tests_arg_begin(&run);

    (void)d_arg_command_set_version_flag(app,
                                         "--version",
                                         "1.2.3",
                                         NULL);

    const int  status   = d_tests_arg_parse(&run,
                                            ARGS,
                                            sizeof(ARGS) / sizeof(ARGS[0]));
    const bool reported = d_tests_arg_matches(&run,
                                              EXPECTED);
    const bool ended    = d_tests_arg_smoke_ended(&run,
                                                  status,
                                                  (int)D_ARG_EXIT_SUCCESS,
                                                  "1.2.3\n",
                                                  "");

    d_tests_arg_end(&run);

    return ( (reported) &&
             (ended) );
}

/*
d_tests_arg_smoke_failure_message
  CLI11's FailureMessage::help, for a failure in a subcommand's parse.
  Tests the following:
  - the message names the error, then gives the help of the last
    subcommand used, as CLI11's help() delegates to it
  - d_arg_parser_exit returns the error's code, 106
*/
bool
d_tests_arg_smoke_failure_message(void)
{
    static const char* const ARGS[] =
    {
        "s"
    };
    static const char EXPECTED[] =
        "ERR RequiredError 106: --x is required\n"
        "cmd app n=1 miss=[] subs=[s] order=[]\n"
        "opt x n=0 res=[]\n";
    static const char MESSAGE[] =
        "ERROR: RequiredError: --x is required\n"
        "A subcommand\n"
        "\n"
        "\n"
        "prog s [OPTIONS]\n"
        "\n"
        "\n"
        "OPTIONS:\n"
        "  -h,     --help              Print this help message and exit\n";
    struct d_tests_arg_run     run = { 0 };
    struct d_arg_command*      app = d_tests_arg_begin(&run);
    struct d_arg_option* const x   =
        d_tests_arg_watch_option(&run,
                                 "x",
                                 d_arg_command_add_option(app,
                                                          "--x",
                                                          "The x"));

    d_arg_option_set_required(x,
                              true);
    (void)d_arg_command_add_subcommand(app,
                                       "s",
                                       "A subcommand");
    d_arg_command_set_failure_message(app,
                                      d_arg_failure_help,
                                      NULL,
                                      NULL);

    const int  status   = d_tests_arg_parse(&run,
                                            ARGS,
                                            sizeof(ARGS) / sizeof(ARGS[0]));
    const bool reported = d_tests_arg_matches(&run,
                                              EXPECTED);
    const bool ended    = d_tests_arg_smoke_ended(
                              &run,
                              status,
                              (int)D_ARG_EXIT_REQUIRED_ERROR,
                              "",
                              MESSAGE);

    d_tests_arg_end(&run);

    return ( (reported) &&
             (ended) );
}

//==============================================================================
// 4.  ENTRY POINTS
//==============================================================================

/*
d_tests_arg_smoke_main
  main's arguments, for a root without a name.
  Tests the following:
  - d_arg_parser_parse takes main's `argv` as it is, without a cast
  - the root takes `argv[0]` whole as its name, as CLI11's does, and the
    parser keeps a copy of it as `program`
*/
bool
d_tests_arg_smoke_main(void)
{
    static const char EXPECTED[] =
        "cmd app n=1 miss=[] subs=[] order=[--n]\n"
        "opt n n=1 res=[3]\n";
    char                   program[] = "./tool";
    char                   name[]    = "--n";
    char                   value[]   = "3";
    char*                  argv[]    = { program, name, value, NULL };
    struct d_tests_arg_run run       = { 0 };
    struct d_arg_command*  app       = d_tests_arg_begin(&run);

    (void)d_arg_command_set_name(app,
                                 NULL);
    (void)d_tests_arg_watch_option(&run,
                                   "n",
                                   d_arg_command_add_option(app,
                                                            "--n",
                                                            NULL));
    d_tests_arg_record(&run,
                       d_arg_parser_parse(&run.parser,
                                          3,
                                          argv));

    const bool reported = d_tests_arg_matches(&run,
                                              EXPECTED);
    const bool named    = ( (strcmp(d_arg_command_name(app),
                                    "./tool") == 0)               &&
                            (run.parser.program != NULL)          &&
                            (strcmp(run.parser.program,
                                    "./tool") == 0) );

    d_tests_arg_end(&run);

    return ( (reported) &&
             (named) );
}

/*
d_tests_arg_smoke_parse_macro
  D_ARG_PARSE in a function written as main is.
  Tests the following:
  - a parse that goes on lets the function go on
  - a parse that fails returns its exit code from the function, after
    writing the failure message to the error output
*/
bool
d_tests_arg_smoke_parse_macro(void)
{
    static const char MESSAGE[] =
        "prog: The following argument was not expected: --bogus\n"
        "Run with --help for more information.\n";
    char                   program[] = "prog";
    char                   name[]    = "--n";
    char                   value[]   = "3";
    char                   bogus[]   = "--bogus";
    char*                  good[]    = { program, name, value, NULL };
    char*                  bad[]     = { program, bogus, NULL };
    struct d_tests_arg_run run       = { 0 };
    struct d_arg_command*  app       = d_tests_arg_begin(&run);

    (void)d_arg_command_add_option(app,
                                   "--n",
                                   NULL);

    const int  goes_on = d_tests_arg_smoke_entry(&run.parser,
                                                 3,
                                                 good);
    const int  ends    = d_tests_arg_smoke_entry(&run.parser,
                                                 2,
                                                 bad);
    const bool written = d_tests_arg_same("error output",
                                          d_arg_buffer_text(&run.err),
                                          MESSAGE);

    d_tests_arg_end(&run);

    return ( (goes_on == 42)                         &&
             (ends == (int)D_ARG_EXIT_EXTRAS_ERROR)  &&
             (written) );
}

/*
d_tests_arg_smoke_line
  A command line held in one string, the program's name first.
  Tests the following:
  - the line is split as CLI11 splits it, a quoted value kept whole
  - the program's name names a root without one
*/
bool
d_tests_arg_smoke_line(void)
{
    static const char EXPECTED[] =
        "cmd app n=1 miss=[] subs=[] order=[--name|-v]\n"
        "opt name n=1 res=[two words]\n"
        "opt v n=1 res=[true]\n";
    struct d_tests_arg_run run = { 0 };
    struct d_arg_command*  app = d_tests_arg_begin(&run);

    (void)d_arg_command_set_name(app,
                                 NULL);
    (void)d_tests_arg_watch_option(&run,
                                   "name",
                                   d_arg_command_add_option(app,
                                                            "--name",
                                                            NULL));
    (void)d_tests_arg_watch_option(&run,
                                   "v",
                                   d_arg_command_add_flag(app,
                                                          "-v",
                                                          NULL));
    d_tests_arg_record(
        &run,
        d_arg_parser_parse_line(
            &run.parser,
            d_pack_text_from_cstr("./tool --name 'two words' -v"),
            true));

    const bool reported = d_tests_arg_matches(&run,
                                              EXPECTED);
    const bool named    = (strcmp(d_arg_command_name(app),
                                  "./tool") == 0);

    d_tests_arg_end(&run);

    return ( (reported) &&
             (named) );
}

/*
d_tests_arg_smoke_reparse
  One parser parsing two command lines.
  Tests the following:
  - the second parse forgets the first: an option given only the first
    time holds nothing, and one given both times holds the second value
*/
bool
d_tests_arg_smoke_reparse(void)
{
    static const char* const FIRST_ARGS[] =
    {
        "--x", "1", "--f"
    };
    static const char FIRST[] =
        "cmd app n=1 miss=[] subs=[] order=[--x|--f]\n"
        "opt x n=1 res=[1]\n"
        "opt f n=1 res=[true]\n";
    static const char* const SECOND_ARGS[] =
    {
        "--x", "2"
    };
    static const char SECOND[] =
        "cmd app n=1 miss=[] subs=[] order=[--x]\n"
        "opt x n=1 res=[2]\n"
        "opt f n=0 res=[]\n";
    struct d_tests_arg_run run = { 0 };
    struct d_arg_command*  app = d_tests_arg_begin(&run);

    (void)d_tests_arg_watch_option(&run,
                                   "x",
                                   d_arg_command_add_option(app,
                                                            "--x",
                                                            NULL));
    (void)d_tests_arg_watch_option(&run,
                                   "f",
                                   d_arg_command_add_flag(app,
                                                          "--f",
                                                          NULL));
    (void)d_tests_arg_parse(&run,
                            FIRST_ARGS,
                            sizeof(FIRST_ARGS) / sizeof(FIRST_ARGS[0]));

    const bool first = d_tests_arg_matches(&run,
                                           FIRST);

    (void)d_tests_arg_parse(&run,
                            SECOND_ARGS,
                            sizeof(SECOND_ARGS) / sizeof(SECOND_ARGS[0]));

    const bool second = d_tests_arg_matches(&run,
                                            SECOND);

    d_tests_arg_end(&run);

    return ( (first) &&
             (second) );
}

//==============================================================================
// 5.  THE SUITE
//==============================================================================

// d_tests_arg_smoke
//   constant: the suite: the command line, the failures, the ending, and the
// entry points, in the header's order.
const struct d_tests_section d_tests_arg_smoke[] =
{
    { "arg_smoke_mixed",              d_tests_arg_smoke_mixed              },
    { "arg_smoke_short_clusters",     d_tests_arg_smoke_short_clusters     },
    { "arg_smoke_flag_values",        d_tests_arg_smoke_flag_values        },
    { "arg_smoke_subcommands",        d_tests_arg_smoke_subcommands        },
    { "arg_smoke_bound",              d_tests_arg_smoke_bound              },
    { "arg_smoke_callbacks",          d_tests_arg_smoke_callbacks          },
    { "arg_smoke_required",           d_tests_arg_smoke_required           },
    { "arg_smoke_needs",              d_tests_arg_smoke_needs              },
    { "arg_smoke_excludes",           d_tests_arg_smoke_excludes           },
    { "arg_smoke_missing_value",      d_tests_arg_smoke_missing_value      },
    { "arg_smoke_conversion",         d_tests_arg_smoke_conversion         },
    { "arg_smoke_definition_error",   d_tests_arg_smoke_definition_error   },
    { "arg_smoke_help",               d_tests_arg_smoke_help               },
    { "arg_smoke_version",            d_tests_arg_smoke_version            },
    { "arg_smoke_failure_message",    d_tests_arg_smoke_failure_message    },
    { "arg_smoke_main",               d_tests_arg_smoke_main               },
    { "arg_smoke_parse_macro",        d_tests_arg_smoke_parse_macro        },
    { "arg_smoke_line",               d_tests_arg_smoke_line               },
    { "arg_smoke_reparse",            d_tests_arg_smoke_reparse            }
};

// d_tests_arg_smoke_count
//   constant: how many sections the table above holds in this build.
const size_t d_tests_arg_smoke_count =
    sizeof(d_tests_arg_smoke) / sizeof(d_tests_arg_smoke[0]);
