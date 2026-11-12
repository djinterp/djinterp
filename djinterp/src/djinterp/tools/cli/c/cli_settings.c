/*******************************************************************************
* djinterp [tools]                                                cli_settings.c
*
* The command-line kit's settings, declared in cli_settings.h.
*   The kit's djin is made at its first use, over static storage: a run
* makes it before it starts any thread, as it parses before it works. The
* command line is walked as the parser will read it -- each option that
* takes the argument after it is given that argument, and each subcommand
* named is followed -- so that a setting is only ever taken where the parser
* would have read an option.
*
*
* path:      /src/djinterp/tools/cli/c/cli_settings.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/
#include "../../../../../inc/djinterp/tools/cli/c/cli_settings.h"  // corresponding header
// std
#include <errno.h>    // ENOENT, ENOMEM, ENOTDIR
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdio.h>    // snprintf
#include <stdlib.h>   // the hooks' free
#include <string.h>   // memchr, memcpy, strchr, strcmp, strcspn, strlen,
                      // strncmp
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_parser.h"  // dj_arg_parser
#include "../../../../../inc/djinterp/config/tools/cli/cfg_cli.h"  // the knobs
#include "../../../../../inc/djinterp/tools/cli/c/cli_io.h"  // d_cli_read_file
#include "../../../../../inc/djinterp/tools/cli/c/cli_plugin.h"  // dj_cli_plugin_versions
#include "./cli_internal.h"  // d_cli_internal_getenv
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // RE_STD_HAS_INT64

#if RE_STD_HAS_INT64

#include "../../../../../inc/djinterp/c/option/option_env.h"           // frames
#include "../../../../../inc/djinterp/c/option/option_env_registry.h"  // registry


//==============================================================================
// 1.  THE DJIN
//==============================================================================

// D_INTERNAL_CLI_SETTINGS_ROOM
//   constant: the room for a full name, a value or a path the kit composes
// or says, with its NUL; a longer one is cut short.
#define D_INTERNAL_CLI_SETTINGS_ROOM 256u

// dj_internal_cli_settings_storage
//   struct: the djin's storage: a binding per source for each of its few
// variables, and the text their values take.
D_OPTION_ENV_DECLARE(dj_internal_cli_settings_storage, 16u, 4u, 2048u);

// d_internal_cli_settings_rows
//   constant: the kit's variables, each defaulting to its knob.
static const struct dj_option_env_var d_internal_cli_settings_rows[] =
{
#if (D_INTERNAL_CLI_PLUGIN_VARIABLE == 1)
    D_OPTION_ENV_TEXT_KNOB("plugin_search", D_CFG_CLI_PLUGIN_SEARCH,
                           "the directories a command's module is looked "
                           "for in, after the run's and plugin_path's"),
#endif
    D_OPTION_ENV_BOOL_KNOB("plugin_strict_version",
                           D_CFG_CLI_PLUGIN_STRICT_VERSION,
                           "whether a module of another version of the "
                           "program is refused, or loaded with a warning")
};

#if (D_INTERNAL_CLI_PLUGIN_VARIABLE == 1)

// d_internal_cli_settings_path_row
//   constant: the program's own `plugin_path`, on the registry's root, read
// as PROGRAM_PLUGIN_PATH: directories looked in before the build's.
static const struct dj_option_env_var d_internal_cli_settings_path_row =
    D_OPTION_ENV_TEXT("plugin_path", NULL,
                      "the directories a command's module is looked for in "
                      "first, separated as PATH separates them");

#endif  // D_INTERNAL_CLI_PLUGIN_VARIABLE

// d_internal_cli_settings_frame, _room, _made, _started
//   variables: the djin, its storage, whether it is made, and whether the
// run's settings are started.
static struct dj_option_env                    d_internal_cli_settings_frame;
static struct dj_internal_cli_settings_storage d_internal_cli_settings_room;
static bool                                    d_internal_cli_settings_made;
static bool                                    d_internal_cli_settings_started;

/*
d_cli_settings_djin
  Made at the first call: the frame over its storage, named, its variables
declared.
*/
struct dj_option_env* d_cli_settings_djin(void)
{
    if (!d_internal_cli_settings_made)
    {
        d_internal_cli_settings_made =
            ( (D_OPTION_ENV_VIEW(&d_internal_cli_settings_frame,
                                 d_internal_cli_settings_room).is_ok) &&
              (d_option_env_set_name(&d_internal_cli_settings_frame,
                                     "cli").is_ok) &&
              (d_option_env_declare_all(
                   &d_internal_cli_settings_frame,
                   d_internal_cli_settings_rows,
                   (uint32_t)(sizeof(d_internal_cli_settings_rows) /
                              sizeof(d_internal_cli_settings_rows[0])))
                   .is_ok) );
    }

    return (d_internal_cli_settings_made)
           ? &d_internal_cli_settings_frame
           : NULL;
}


//==============================================================================
// 2.  WHAT WENT WRONG
//==============================================================================

/*
d_cli_settings_why
  What a binding's failure means, for a setting `_name` (of `_size` bytes)
given `_value`, or none: written into `_buffer`. A name that names nothing
is given the nearest that does, where there is one.
*/
void d_cli_settings_why(
    int32_t     _status,
    const char* _name,
    size_t      _size,
    const char* _value,
    char*       _buffer,
    size_t      _capacity
)
{
    char   near[D_INTERNAL_CLI_SETTINGS_ROOM];
    size_t found = 0u;
    int    shown = (_size < (size_t)D_INTERNAL_CLI_SETTINGS_ROOM)
                   ? (int)_size
                   : (int)D_INTERNAL_CLI_SETTINGS_ROOM;

    if (_status == (int32_t)D_OPTION_STATUS_KEY_ABSENT)
    {
        found = d_option_env_registry_suggest(d_option_env_registry(),
                                              _name, _size,
                                              near, sizeof(near));
        (void)snprintf(_buffer, _capacity,
                       ( (found > 0u) &&
                         (found < sizeof(near)) )
                       ? "no setting is named '%.*s'; did you mean '%s'?"
                       : "no setting is named '%.*s'%s",
                       shown, _name,
                       ( (found > 0u) &&
                         (found < sizeof(near)) ) ? near : "");
    }
    else if (_status == (int32_t)D_OPTION_STATUS_POLICY_REJECTED)
    {
        (void)snprintf(_buffer, _capacity,
                       "'%.*s' cannot be '%s' in this build",
                       shown, _name, (_value != NULL) ? _value : "");
    }
    else if (_value != NULL)
    {
        (void)snprintf(_buffer, _capacity,
                       "'%s' is not a value '%.*s' takes",
                       _value, shown, _name);
    }
    else
    {
        (void)snprintf(_buffer, _capacity,
                       "'%.*s' cannot be set so (%s)",
                       shown, _name, d_option_status_name(_status));
    }

    return;
}


//==============================================================================
// 3.  THE USER'S FILE
//==============================================================================

/*
d_cli_settings_file
  The XDG directory, or the home's .config; APPDATA on Windows.
*/
size_t d_cli_settings_file(
    char*  _buffer,
    size_t _capacity
)
{
    char* base   = NULL;
    int   length = -1;

#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    if ( (d_cli_internal_getenv("APPDATA", &base) == 0) &&
         (base != NULL) &&
         (base[0] != '\0') )
    {
        length = snprintf(_buffer, _capacity, "%s\\djinterp\\settings", base);
    }
#else
    if ( (d_cli_internal_getenv("XDG_CONFIG_HOME", &base) == 0) &&
         (base != NULL) &&
         (base[0] == '/') )
    {
        length = snprintf(_buffer, _capacity, "%s/djinterp/settings", base);
    }
    else
    {
        D_INTERNAL_CLI_FREE(base);
        base = NULL;

        if ( (d_cli_internal_getenv("HOME", &base) == 0) &&
             (base != NULL) &&
             (base[0] != '\0') )
        {
            length = snprintf(_buffer, _capacity,
                              "%s/.config/djinterp/settings", base);
        }
    }
#endif

    D_INTERNAL_CLI_FREE(base);

    return (length > 0) ? (size_t)length : 0u;
}

/*
d_internal_cli_settings_name_at
  The name of the statement at `_at` of a script: from its first character
not a space to before its '=' or its end, spaces after it left off. A '-'
that unbinds is part of it, as the statement wrote it.
*/
static size_t
d_internal_cli_settings_name_at(
    const char*  _script,
    size_t       _at,
    size_t       _end,
    const char** _name
)
{
    size_t from = _at;
    size_t to;

    while ( (from < _end) &&
            ( (_script[from] == ' ') ||
              (_script[from] == '\t') ) )
    {
        ++from;
    }

    to = from;

    while ( (to < _end) &&
            (_script[to] != '=') )
    {
        ++to;
    }

    while ( (to > from) &&
            ( (_script[to - 1u] == ' ') ||
              (_script[to - 1u] == '\t') ) )
    {
        --to;
    }

    *_name = _script + from;

    return to - from;
}

/*
d_internal_cli_settings_value_at
  The value of the statement from `_at` to `_end`, after its '=', spaces
at either end left off, into `_buffer`, cut short to its room; NULL for a
statement with no '='.
*/
static const char*
d_internal_cli_settings_value_at(
    const char* _script,
    size_t      _at,
    size_t      _end,
    char*       _buffer,
    size_t      _capacity
)
{
    const char* const equals = (const char*)memchr(_script + _at, '=',
                                                   _end - _at);
    size_t            from;
    size_t            to   = _end;

    if (equals == NULL)
    {
        return NULL;
    }

    from = (size_t)(equals - _script) + 1u;

    while ( (from < to) &&
            ( (_script[from] == ' ') ||
              (_script[from] == '\t') ) )
    {
        ++from;
    }

    while ( (to > from) &&
            ( (_script[to - 1u] == ' ') ||
              (_script[to - 1u] == '\t') ||
              (_script[to - 1u] == '\r') ) )
    {
        --to;
    }

    if ((to - from) >= _capacity)
    {
        to = from + _capacity - 1u;
    }

    memcpy(_buffer, _script + from, to - from);
    _buffer[to - from] = '\0';

    return _buffer;
}

/*
d_internal_cli_settings_load
  The file's statements, through the registry, from FILE: at each failure,
its line said and the rest read on, from where the failing statement ends.
*/
static void
d_internal_cli_settings_load(
    const struct dj_cli_program* _program,
    const char*                  _path,
    const char*                  _script
)
{
    size_t offset  = 0u;
    long   line    = 1;
    size_t counted = 0u;

    while (_script[offset] != '\0')
    {
        size_t                        failed = 0u;
        const struct dj_option_result done   = d_option_env_registry_load(
                                                   d_option_env_registry(),
                                                   _script + offset,
                                                   D_OPTION_ENV_SOURCE_FILE,
                                                   &failed);
        size_t                        at     = offset + failed;
        size_t                        end;
        const char*                   name   = NULL;
        size_t                        size;
        char                          value[D_INTERNAL_CLI_SETTINGS_ROOM];
        char                          why[D_INTERNAL_CLI_SETTINGS_ROOM];

        if (done.is_ok)
        {
            break;
        }

        // the failing statement's line
        for (; counted < at; ++counted)
        {
            line += (_script[counted] == '\n') ? 1 : 0;
        }

        end  = d_option_env_statement_end(_script, at);
        size = d_internal_cli_settings_name_at(_script, at, end, &name);

        d_cli_settings_why(d_option_result_status(&done),
                                    name, size,
                                    d_internal_cli_settings_value_at(
                                        _script, at, end,
                                        value, sizeof(value)),
                                    why, sizeof(why));
        d_cli_report_at(_program, (int)D_CLI_SEVERITY_WARNING, _path, line,
                        0L, "%s", why);

        // on from the end of the one that failed, and never back
        offset = (end > at) ? end : (at + 1u);
    }

    return;
}

/*
d_internal_cli_settings_read_file
  The user's file, read whole: one not there, or that the system names no
home for, is nothing; one that cannot be read otherwise is said, and passed
over.
*/
static int
d_internal_cli_settings_read_file(
    const struct dj_cli_program* _program
)
{
    char   path[D_INTERNAL_CLI_SETTINGS_ROOM];
    char*  text   = NULL;
    size_t length = 0u;
    size_t named  = d_cli_settings_file(path, sizeof(path));
    int    failed;

    if ( (named == 0u) ||
         (named >= sizeof(path)) )
    {
        return D_CLI_EXIT_OK;
    }

    failed = d_cli_read_file(path, &text, &length);

    if (failed == ENOMEM)
    {
        d_cli_report(_program, (int)D_CLI_SEVERITY_ERROR, "out of memory");

        return D_CLI_EXIT_TROUBLE;
    }

    if ( (failed != 0) &&
         (failed != ENOENT) &&
         (failed != ENOTDIR) )
    {
        d_cli_report(_program, (int)D_CLI_SEVERITY_WARNING,
                     "cannot read '%s': %s", path,
                     d_cli_internal_error_text(failed));
    }

    if (failed == 0)
    {
        d_internal_cli_settings_load(_program, path, text);
    }

    d_cli_free(text);

    return D_CLI_EXIT_OK;
}


//==============================================================================
// 4.  A RUN'S SETTINGS
//==============================================================================

/*
d_internal_cli_settings_prefix
  The program's own prefix: its name upper-cased, each '-' an '_', and an
'_' after it; empty when it does not fit.
*/
static void
d_internal_cli_settings_prefix(
    const char* _name,
    char*       _buffer,
    size_t      _capacity
)
{
    size_t at = 0u;

    for (const char* each = (_name != NULL) ? _name : "";
         (*each != '\0') && ((at + 2u) < _capacity);
         ++each)
    {
        _buffer[at++] = ( (*each >= 'a') &&
                          (*each <= 'z') )
                        ? (char)(*each - 'a' + 'A')
                        : (*each == '-') ? '_' : *each;
    }

    _buffer[at++] = '_';
    _buffer[at]   = '\0';

    if ( (_name == NULL) ||
         (at != strlen(_name) + 1u) )
    {
        _buffer[0] = '\0';
    }

    return;
}

/*
d_cli_settings_start
  Once: the djin and the program's own variable, the environment under the
shared prefix and then the program's own, and the user's file.
*/
int d_cli_settings_start(
    const struct dj_cli_program* _program
)
{
    struct dj_option_env_registry* const registry = d_option_env_registry();
    struct dj_option_env* const          djin     = d_cli_settings_djin();
    char                                 prefix[D_INTERNAL_CLI_SETTINGS_ROOM];

    if (d_internal_cli_settings_started)
    {
        return D_CLI_EXIT_OK;
    }

    d_internal_cli_settings_started = true;

    // registered once a process, a second program's start finding it
    if ( (djin != NULL) &&
         (d_option_env_registry_find(registry, "cli", 3u) == NULL) )
    {
        (void)d_option_env_registry_add(registry, djin);
    }

#if (D_INTERNAL_CLI_PLUGIN_VARIABLE == 1)
    (void)d_option_env_declare(d_option_env_registry_frame(registry),
                               &d_internal_cli_settings_path_row);
#endif

    // the shared prefix, then the program's own, whose binding wins
    d_internal_cli_settings_prefix((_program != NULL) ? _program->name
                                                      : NULL,
                                   prefix, sizeof(prefix));
    (void)d_option_env_registry_import(registry, "DJINTERP_");

    if ( (prefix[0] != '\0') &&
         (strcmp(prefix, "DJINTERP_") != 0) )
    {
        (void)d_option_env_registry_import(registry, prefix);
    }

    return d_internal_cli_settings_read_file(_program);
}

/*
d_internal_cli_settings_shaped
  Whether an argument is shaped as a setting: `--set`, or a long option
whose name -- after `no-`, before `=` -- holds a '.'. No option of the
kit's is either (R5).
*/
static bool
d_internal_cli_settings_shaped(
    const char* _argument
)
{
    const char* name;
    size_t      size;

    if ( (strncmp(_argument, "--", 2u) != 0) ||
         (_argument[2] == '\0') )
    {
        return false;
    }

    if (strcmp(_argument, "--set") == 0)
    {
        return true;
    }

    name = _argument + 2;
    size = strcspn(name, "=");

    return (memchr(name, '.', size) != NULL);
}

/*
d_internal_cli_settings_said
  A setting the command line named badly, said as a usage error: the
option as typed, and what was wrong with it.
*/
static int
d_internal_cli_settings_said(
    const struct dj_cli_program* _program,
    const struct dj_arg_command* _command,
    int                          _argc,
    const char* const*           _argv,
    int                          _at,
    int32_t                      _status
)
{
    const bool  set   = (strcmp(_argv[_at], "--set") == 0);
    const char* text  = (!set) ? (_argv[_at] + 2)
                               : (_at + 1 < _argc) ? _argv[_at + 1]
                                                   : NULL;
    size_t      size;
    char        why[D_INTERNAL_CLI_SETTINGS_ROOM];

    // --set with no NAME=VALUE after it
    if ( (text == NULL) ||
         ( (set) &&
           (strchr(text, '=') == NULL) ) )
    {
        return d_cli_usage_error(_program, _command,
                                 "--set: NAME=VALUE is required");
    }

    // a bool's --no-NAME names NAME
    if ( (!set) &&
         (strncmp(text, "no-", 3u) == 0) &&
         (strchr(text, '=') == NULL) )
    {
        text += 3;
    }

    size = strcspn(text, "=");

    // --NAME or --no-NAME, which only a bool takes: the registry refuses
    // either, for a setting of another kind, as an argument it cannot take
    if ( (!set) &&
         (text[size] == '\0') &&
         (_status == (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT) )
    {
        return d_cli_usage_error(_program, _command,
                                 "%s: '%s' is not a bool: give it a value, "
                                 "as --%s=VALUE",
                                 _argv[_at], text, text);
    }

    d_cli_settings_why(_status, text, size,
                                (text[size] == '=') ? (text + size + 1u)
                                                    : NULL,
                                why, sizeof(why));

    return d_cli_usage_error(_program, _command, "%.*s: %s",
                             (int)strcspn(_argv[_at], "="), _argv[_at],
                             why);
}

/*
d_internal_cli_settings_value_next
  Whether an argument the parser reads as an option takes the argument
after it as its value: a long option with no '=' that takes a value, or a
group of short ones whose last takes a value and holds none. A subcommand
named is followed, into `*_command`.
*/
static bool
d_internal_cli_settings_value_next(
    const struct dj_arg_command** _command,
    const char*                   _argument
)
{
    char                        name[3] = { '-', '\0', '\0' };
    const struct dj_arg_option* option;

    if (strncmp(_argument, "--", 2u) == 0)
    {
        option = (strchr(_argument, '=') == NULL)
                 ? d_arg_command_option(*_command, _argument)
                 : NULL;

        return (d_arg_option_items_expected_min(option) > 0);
    }

    if ( (_argument[0] == '-') &&
         (_argument[1] != '\0') )
    {
        for (const char* each = _argument + 1; *each != '\0'; ++each)
        {
            name[1] = *each;
            option  = d_arg_command_option(*_command, name);

            if (d_arg_option_items_expected_min(option) > 0)
            {
                return (each[1] == '\0');
            }
        }

        return false;
    }

    // a word: a subcommand's name takes the walk into it
    {
        const struct dj_arg_command* const inner =
            d_arg_command_subcommand(*_command, _argument);

        *_command = (inner != NULL) ? inner : *_command;
    }

    return false;
}

/*
d_cli_settings_take
  The walk: past `--` everything is kept; a setting is offered to the
registry; an option that takes the next argument keeps it with itself.
*/
int d_cli_settings_take(
    const struct dj_cli_program* _program,
    int                          _argc,
    const char* const*           _argv,
    const char**                 _kept,
    int*                         _kept_count
)
{
    const struct dj_arg_command* command = &_program->parser->command;
    int                          kept    = 0;
    bool                         ended   = false;

    for (int at = 0; at < _argc; ++at)
    {
        const char* const        argument = _argv[at];
        const int                first    = at;
        int                      taken    = 0;
        struct dj_option_result  result   = d_option_ok(0u);

        if ( (at > 0) &&
             (!ended) &&
             (d_internal_cli_settings_shaped(argument)) )
        {
            taken = d_option_env_registry_take_option(
                        d_option_env_registry(), _argc,
                        (char* const*)_argv, &at, &result);
        }

        if (taken < 0)
        {
            *_kept_count = kept;

            // said by the option as typed, wherever the walk stopped
            return d_internal_cli_settings_said(
                       _program,
                       (command != &_program->parser->command) ? command
                                                               : NULL,
                       _argc, _argv, first,
                       d_option_result_status(&result));
        }

        if (taken > 0)
        {
            continue;
        }

        _kept[kept++] = argument;
        ended         = ( (ended) ||
                          (strcmp(argument, "--") == 0) );

        // the value an option before it takes is kept with it, unread
        if ( (at > 0) &&
             (!ended) &&
             (at + 1 < _argc) &&
             (d_internal_cli_settings_value_next(&command, argument)) )
        {
            _kept[kept++] = _argv[++at];
        }
    }

    _kept[kept]  = NULL;
    *_kept_count = kept;

    return D_CLI_EXIT_OK;
}


//==============================================================================
// 5.  WHAT THE KIT READS
//==============================================================================

/*
d_internal_cli_settings_text
  A text variable of the kit's djin, or of the registry's root, by name:
`_fallback` before the settings are started or where it is not declared.
*/
static const char*
d_internal_cli_settings_text(
    struct dj_option_env* _frame,
    const char*           _name,
    const char*           _fallback
)
{
    struct dj_option_result found;

    if ( (!d_internal_cli_settings_started) ||
         (_frame == NULL) )
    {
        return _fallback;
    }

    found = d_option_env_find(_frame, _name, strlen(_name));

    return (found.is_ok)
           ? d_option_env_text(_frame, D_OPTION_ENV_GLOBAL,
                               (uint32_t)found.payload.value)
           : _fallback;
}

/*
d_cli_settings_plugin_search
  `cli.plugin_search`, or the knob.
*/
const char* d_cli_settings_plugin_search(void)
{
#if (D_INTERNAL_CLI_PLUGIN_VARIABLE == 1)
    const char* const search = d_internal_cli_settings_text(
                                   d_cli_settings_djin(), "plugin_search",
                                   D_CFG_CLI_PLUGIN_SEARCH);

    return (search != NULL) ? search : "";
#else
    return D_CFG_CLI_PLUGIN_SEARCH;
#endif
}

/*
d_cli_settings_plugin_path
  The root's `plugin_path`.
*/
const char* d_cli_settings_plugin_path(void)
{
#if (D_INTERNAL_CLI_PLUGIN_VARIABLE == 1)
    return d_internal_cli_settings_text(
               d_option_env_registry_frame(d_option_env_registry()),
               "plugin_path", NULL);
#else
    return NULL;
#endif
}

/*
d_cli_settings_plugin_strict_version
  A binding from the file, the environment or the command line stands above
the program's own say, as each source stands above PROGRAM.
*/
bool d_cli_settings_plugin_strict_version(
    int _program_says
)
{
    struct dj_option_env* const djin = d_cli_settings_djin();
    struct dj_option_result     found;

    if ( (d_internal_cli_settings_started) &&
         (djin != NULL) )
    {
        found = d_option_env_find(djin, "plugin_strict_version", 21u);

        if ( (found.is_ok) &&
             (d_option_env_source(djin, D_OPTION_ENV_GLOBAL,
                                  (uint32_t)found.payload.value) >
              (int32_t)D_OPTION_ENV_SOURCE_PROGRAM) )
        {
            return d_option_env_bool(djin, D_OPTION_ENV_GLOBAL,
                                     (uint32_t)found.payload.value);
        }
    }

    return (_program_says == (int)D_CLI_PLUGIN_VERSIONS_REFUSE)
           ? true
           : (_program_says == (int)D_CLI_PLUGIN_VERSIONS_WARN)
             ? false
             : (D_INTERNAL_CLI_PLUGIN_STRICT_VERSION == 1);
}

#if defined(D_TESTING)
#if (D_TESTING == 1)

/*
d_cli_settings_reset
  The djin off the list, every binding gone, and the start due again.
*/
void d_cli_settings_reset(void)
{
    struct dj_option_env_registry* const registry = d_option_env_registry();

    if (d_internal_cli_settings_made)
    {
        (void)d_option_env_registry_remove(registry,
                                           &d_internal_cli_settings_frame);
        d_option_env_clear(&d_internal_cli_settings_frame);
    }

    d_option_env_clear(d_option_env_registry_frame(registry));
    d_internal_cli_settings_started = false;

    return;
}

#endif  // D_TESTING
#endif  // defined(D_TESTING)

#else  // !RE_STD_HAS_INT64

//==============================================================================
// WITHOUT THE OPTION CORE
//==============================================================================
//   No registry: the knobs, as built; nothing taken off a command line.

struct dj_option_env* d_cli_settings_djin(void)
{
    return NULL;
}

int d_cli_settings_start(
    const struct dj_cli_program* _program
)
{
    (void)_program;

    return D_CLI_EXIT_OK;
}

int d_cli_settings_take(
    const struct dj_cli_program* _program,
    int                          _argc,
    const char* const*           _argv,
    const char**                 _kept,
    int*                         _kept_count
)
{
    (void)_program;

    for (int at = 0; at < _argc; ++at)
    {
        _kept[at] = _argv[at];
    }

    _kept[_argc] = NULL;
    *_kept_count = _argc;

    return D_CLI_EXIT_OK;
}

size_t d_cli_settings_file(
    char*  _buffer,
    size_t _capacity
)
{
    (void)_buffer;
    (void)_capacity;

    return 0u;
}

void d_cli_settings_why(
    int32_t     _status,
    const char* _name,
    size_t      _size,
    const char* _value,
    char*       _buffer,
    size_t      _capacity
)
{
    (void)_status;
    (void)_value;

    (void)snprintf(_buffer, _capacity, "'%.*s' cannot be set",
                   (int)_size, _name);

    return;
}

const char* d_cli_settings_plugin_search(void)
{
    return D_CFG_CLI_PLUGIN_SEARCH;
}

const char* d_cli_settings_plugin_path(void)
{
    return NULL;
}

bool d_cli_settings_plugin_strict_version(
    int _program_says
)
{
    return (_program_says == (int)D_CLI_PLUGIN_VERSIONS_REFUSE)
           ? true
           : (_program_says == (int)D_CLI_PLUGIN_VERSIONS_WARN)
             ? false
             : (D_INTERNAL_CLI_PLUGIN_STRICT_VERSION == 1);
}

#if defined(D_TESTING)
#if (D_TESTING == 1)

void d_cli_settings_reset(void)
{
    return;
}

#endif  // D_TESTING
#endif  // defined(D_TESTING)

#endif  // RE_STD_HAS_INT64
