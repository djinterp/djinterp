/*******************************************************************************
* djinterp [c][tools]                                             cli_settings.c
*
* The command-line kit's settings, declared in cli_settings.h.
*   The kit's djin is made at its first use, over static storage: a run
* makes it before it starts any thread, as it parses before it works. The
* command line's settings are the parser's to offer: its setting hook is
* offered each long option the parse does not know, where the parse reads
* it, so that a setting is only ever taken where an option could stand --
* in a response file, after an abbreviated option's value, never in a
* prefix command's arguments - by the parser's own rules.
*
*
* path:      /src/djinterp/tools/cli/c/cli_settings.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.21
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

// D_INTERNAL_CLI_PLUGIN_REFUSED
//   constant: the sources the plugin rows refuse. None where the caller may
// choose where modules come from (D_CFG_CLI_PLUGIN_VARIABLE 1); where it may
// not, every source the caller controls - the user's file, the environment
// and the command line - so that only the program and an override bind
// them (D_OPTION_ENV_REFUSES_CALLER, option_env's decision 69). A builder
// who trusts one of those defines D_OPTION_ENV_REFUSES_CALLER as its own set.
#if (D_INTERNAL_CLI_PLUGIN_VARIABLE == 1)
    #define D_INTERNAL_CLI_PLUGIN_REFUSED 0u
#else
    #define D_INTERNAL_CLI_PLUGIN_REFUSED D_OPTION_ENV_REFUSES_CALLER
#endif

// d_internal_cli_settings_unknown_names, _refusal_names
//   constants: the words `cli.unknown_settings` and `cli.shared_refusals`
// take (cfg_cli.h, 1.5).
static const struct dj_option_env_name d_internal_cli_settings_unknown_names[] =
{
    { "none",  D_CLI_UNKNOWN_SETTINGS_NONE  },
    { "typos", D_CLI_UNKNOWN_SETTINGS_TYPOS },
    { "all",   D_CLI_UNKNOWN_SETTINGS_ALL   },
    { "error", D_CLI_UNKNOWN_SETTINGS_ERROR }
};

static const struct dj_option_env_name d_internal_cli_settings_refusal_names[] =
{
    { "none",    D_CLI_SHARED_REFUSALS_NONE    },
    { "verbose", D_CLI_SHARED_REFUSALS_VERBOSE },
    { "warn",    D_CLI_SHARED_REFUSALS_WARN    },
    { "error",   D_CLI_SHARED_REFUSALS_ERROR   }
};

// d_internal_cli_settings_rows
//   constant: the kit's variables, each defaulting to its knob.
static const struct dj_option_env_var d_internal_cli_settings_rows[] =
{
    D_OPTION_ENV_ROW_REFUSING(D_OPTION_ENV_KIND_TEXT, "plugin_search", NULL,
                              0u, NULL, 0u, D_CFG_CLI_PLUGIN_SEARCH, 0,
                              INT64_MIN, INT64_MAX, 0.0, NULL,
                              "D_CFG_CLI_PLUGIN_SEARCH",
                              "the directories a command's module is looked "
                              "for in, after the run's and plugin_path's",
                              D_INTERNAL_CLI_PLUGIN_REFUSED),
    D_OPTION_ENV_BOOL_KNOB("plugin_strict_version",
                           D_CFG_CLI_PLUGIN_STRICT_VERSION,
                           "whether a module of another version of the "
                           "program is refused, or loaded with a warning"),
    D_OPTION_ENV_CHOICE_KNOB("unknown_settings",
                             d_internal_cli_settings_unknown_names,
                             D_CFG_CLI_UNKNOWN_SETTINGS,
                             "what is said of a line of the user's file "
                             "that names no setting here: none, typos, all "
                             "or error"),
    D_OPTION_ENV_CHOICE_KNOB("shared_refusals",
                             d_internal_cli_settings_refusal_names,
                             D_CFG_CLI_SHARED_REFUSALS,
                             "what is said when the user's file or "
                             "DJINTERP_ sets a setting that refuses it: "
                             "none, verbose, warn or error")
};

// d_internal_cli_settings_path_row
//   constant: the program's own `plugin_path`, on the registry's root, read
// as PROGRAM_PLUGIN_PATH: directories looked in before the build's.
static const struct dj_option_env_var d_internal_cli_settings_path_row =
    D_OPTION_ENV_ROW_REFUSING(D_OPTION_ENV_KIND_TEXT, "plugin_path", NULL, 0u,
                              NULL, 0u, NULL, 0, INT64_MIN, INT64_MAX, 0.0,
                              NULL, NULL,
                              "the directories a command's module is looked "
                              "for in first, separated as PATH separates "
                              "them",
                              D_INTERNAL_CLI_PLUGIN_REFUSED);

// d_internal_cli_settings_frame, _room, _made, _started
//   variables: the djin, its storage, whether it is made, and whether the
// run's settings are started.
static struct dj_option_env                    d_internal_cli_settings_frame;
static struct dj_internal_cli_settings_storage d_internal_cli_settings_room;
static bool                                    d_internal_cli_settings_made;
static bool                                    d_internal_cli_settings_started;

// D_INTERNAL_CLI_SETTINGS_HELD
//   constant: the most notes the run's start holds; past them, counted.
#define D_INTERNAL_CLI_SETTINGS_HELD 32u

// dj_internal_cli_settings_kind
//   enum: what a held note is about, which decides what says it.
enum dj_internal_cli_settings_kind
{
    D_INTERNAL_CLI_HELD_FOREIGN = 0,  // a djin not registered here
    D_INTERNAL_CLI_HELD_UNKNOWN = 1,  // a near miss, or a djin's unknown
    D_INTERNAL_CLI_HELD_SHARED  = 2,  // refused from a shared source
    D_INTERNAL_CLI_HELD_OWN     = 3,  // refused from the program's prefix
    D_INTERNAL_CLI_HELD_VALUE   = 4   // a value the setting does not take
};

// dj_internal_cli_settings_note
//   struct: a note the run's start held: its kind, its line in the user's
// file (0 for the environment), and its words.
struct dj_internal_cli_settings_note
{
    int  kind;
    long line;
    char text[D_INTERNAL_CLI_SETTINGS_ROOM];
};

// d_internal_cli_settings_notes, _count, _past, _path
//   variables: the held notes, how many, how many more there were, and the
// user's file's path, which a file's notes name.
static struct dj_internal_cli_settings_note
    d_internal_cli_settings_notes[D_INTERNAL_CLI_SETTINGS_HELD];
static size_t d_internal_cli_settings_count;
static size_t d_internal_cli_settings_past;
static char   d_internal_cli_settings_path[D_INTERNAL_CLI_SETTINGS_ROOM];

// d_internal_cli_settings_refusal, _refused_argument, _refused_size
//   variables: the words for the setting the hook last refused; the
// argument it was offered, as typed, by which the parse's error is known
// for the hook's, and its length; 0 when none is held.
static char   d_internal_cli_settings_refusal[D_INTERNAL_CLI_SETTINGS_ROOM *
                                              2u];
static char   d_internal_cli_settings_refused_argument[
                  D_INTERNAL_CLI_SETTINGS_ROOM];
static size_t d_internal_cli_settings_refused_size;

/*
d_internal_cli_settings_hold
  A note kept for d_cli_settings_report; past the room, counted.
*/
static void
d_internal_cli_settings_hold(
    int         _kind,
    long        _line,
    const char* _text
)
{
    struct dj_internal_cli_settings_note* note;

    if (d_internal_cli_settings_count >= D_INTERNAL_CLI_SETTINGS_HELD)
    {
        ++d_internal_cli_settings_past;

        return;
    }

    note       = &d_internal_cli_settings_notes[d_internal_cli_settings_count];
    note->kind = _kind;
    note->line = _line;
    (void)snprintf(note->text, sizeof(note->text), "%s", _text);
    ++d_internal_cli_settings_count;

    return;
}

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
d_internal_cli_settings_reason
  Why the registry's check refuses `_name` given `_value` from `_source`
(D_OPTION_ENV_REASON_*): FRAME for a dotted name whose first part names no
djin, and that the root takes neither as a name nor as a scope; NONE where
the check passes.
*/
static int32_t
d_internal_cli_settings_reason(
    const char* _name,
    size_t      _size,
    const char* _value,
    int32_t     _source
)
{
    struct dj_option_env_checked checked;
    struct dj_option_result      result;

    memset(&checked, 0, sizeof(checked));
    checked.size   = (uint32_t)sizeof(checked);
    checked.reason = (int32_t)D_OPTION_ENV_REASON_NONE;

    if ( (_name == NULL) ||
         (d_option_env_registry_check_from(
              d_option_env_registry(), _name, _size,
              (_value != NULL) ? _value : "",
              (_value != NULL) ? strlen(_value) : 0u,
              _source, NULL, 0u, &result, &checked) != (size_t)-1) )
    {
        return (int32_t)D_OPTION_ENV_REASON_NONE;
    }

    return checked.reason;
}

/*
d_cli_settings_why
  What a binding's failure means, for a setting `_name` (of `_size` bytes)
given `_value`, or none: written into `_buffer`. A name that names nothing
is given the nearest that does, where there is one; one whose first part
names no djin, where none is near, says so.
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
        const char* const dot = (_name != NULL)
                                ? (const char*)memchr(_name, '.', _size)
                                : NULL;

        found = d_option_env_registry_suggest(d_option_env_registry(),
                                              _name, _size,
                                              near, sizeof(near));

        // no djin by the first part, and none near: the djin is said
        if ( (found == 0u) &&
             (dot != NULL) &&
             (d_internal_cli_settings_reason(
                  _name, _size, _value,
                  (int32_t)D_OPTION_ENV_SOURCE_FILE) ==
              (int32_t)D_OPTION_ENV_REASON_FRAME) )
        {
            (void)snprintf(_buffer, _capacity, "no djin is named '%.*s'",
                           (int)(dot - _name), _name);

            return;
        }

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
d_internal_cli_settings_kind_of
  What a statement's failure is about: a name no setting here has, from a
djin not registered here, or near one or a djin's that is, the two kinds
`cli.unknown_settings` tells apart (cfg_cli.h, 1.5.1); a source the setting
refuses; or a value it does not take. The words for a refused source go
into `_why`.
*/
static int
d_internal_cli_settings_kind_of(
    int32_t     _status,
    const char* _name,
    size_t      _size,
    const char* _value,
    char*       _why,
    size_t      _capacity
)
{
    char near[D_INTERNAL_CLI_SETTINGS_ROOM];

    // a djin not registered here: the registry's check says so, where no
    // name is near (option_env's reason FRAME)
    if (_status == (int32_t)D_OPTION_STATUS_KEY_ABSENT)
    {
        const size_t found = d_option_env_registry_suggest(
                                 d_option_env_registry(), _name, _size,
                                 near, sizeof(near));

        return ( (found == 0u) &&
                 (d_internal_cli_settings_reason(
                      _name, _size, _value,
                      (int32_t)D_OPTION_ENV_SOURCE_FILE) ==
                  (int32_t)D_OPTION_ENV_REASON_FRAME) )
               ? (int)D_INTERNAL_CLI_HELD_FOREIGN
               : (int)D_INTERNAL_CLI_HELD_UNKNOWN;
    }

    if ( (_status == (int32_t)D_OPTION_STATUS_POLICY_REJECTED) &&
         (_value != NULL) &&
         (d_internal_cli_settings_reason(
              _name, _size, _value, (int32_t)D_OPTION_ENV_SOURCE_FILE) ==
          (int32_t)D_OPTION_ENV_REASON_SOURCE) )
    {
        (void)snprintf(_why, _capacity,
                       "'%.*s' is not taken from the user's settings file",
                       (int)_size, _name);

        return (int)D_INTERNAL_CLI_HELD_SHARED;
    }

    return (int)D_INTERNAL_CLI_HELD_VALUE;
}

/*
d_internal_cli_settings_load
  The file's statements, through the registry, from FILE: at each failure,
its line held and the rest read on, from where the failing statement ends.
*/
static void
d_internal_cli_settings_load(
    const char* _script
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
        const char*                   given;
        int                           kind;

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

        given = d_internal_cli_settings_value_at(_script, at, end, value,
                                                 sizeof(value));
        d_cli_settings_why(d_option_result_status(&done), name, size, given,
                           why, sizeof(why));
        kind  = d_internal_cli_settings_kind_of(d_option_result_status(&done),
                                                name, size, given, why,
                                                sizeof(why));
        d_internal_cli_settings_hold(kind, line, why);

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
        (void)snprintf(d_internal_cli_settings_path,
                       sizeof(d_internal_cli_settings_path), "%s", path);
        d_internal_cli_settings_load(text);
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

// d_internal_cli_settings_shared, _own
//   constants: which prefix an import's report is told of, as its context.
static const int d_internal_cli_settings_shared = 1;
static const int d_internal_cli_settings_own    = 0;

/*
d_internal_cli_settings_reported
  An import's report: a value the environment set that a setting refused,
held, a refused source by the prefix it came under; an empty one passed
over.
*/
static void
d_internal_cli_settings_reported(
    void*                              _context,
    const struct dj_option_env_report* _report
)
{
    const bool shared = (*(const int*)_context != 0);
    char       why[D_INTERNAL_CLI_SETTINGS_ROOM * 2u];
    char       said[D_INTERNAL_CLI_SETTINGS_ROOM];

    // a variable set empty is taken as one not set, as `NAME= program`
    // means, and is not said
    if ( (_report->value == NULL) ||
         (_report->value[0] == '\0') )
    {
        return;
    }

    if (_report->reason == (int32_t)D_OPTION_ENV_REASON_SOURCE)
    {
        (void)snprintf(why, sizeof(why),
                       "%s: '%s' is not taken from the environment",
                       _report->variable, _report->setting);
        d_internal_cli_settings_hold(
            (shared) ? (int)D_INTERNAL_CLI_HELD_SHARED
                     : (int)D_INTERNAL_CLI_HELD_OWN,
            0L, why);

        return;
    }

    // the words a file's statement is said with, after the variable's name
    d_cli_settings_why(d_option_result_status(&_report->result),
                       _report->setting, strlen(_report->setting),
                       _report->value, said, sizeof(said));
    (void)snprintf(why, sizeof(why), "%s: %s", _report->variable, said);
    d_internal_cli_settings_hold((int)D_INTERNAL_CLI_HELD_VALUE, 0L, why);

    return;
}

/*
d_cli_settings_start
  Once: the djin and the program's own variable, the environment under the
shared prefix and then the program's own, and the user's file, each failure
held for d_cli_settings_report.
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

    (void)d_option_env_declare(d_option_env_registry_frame(registry),
                               &d_internal_cli_settings_path_row);

    // the shared prefix, then the program's own, whose binding wins
    d_internal_cli_settings_prefix((_program != NULL) ? _program->name
                                                      : NULL,
                                   prefix, sizeof(prefix));
    (void)d_option_env_registry_import_each(
              registry, "DJINTERP_", d_internal_cli_settings_reported,
              (void*)&d_internal_cli_settings_shared);

    if ( (prefix[0] != '\0') &&
         (strcmp(prefix, "DJINTERP_") != 0) )
    {
        (void)d_option_env_registry_import_each(
                  registry, prefix, d_internal_cli_settings_reported,
                  (void*)&d_internal_cli_settings_own);
    }

    return d_internal_cli_settings_read_file(_program);
}

/*
d_internal_cli_settings_shaped
  Whether an argument is shaped as a setting: `--set`, or a long option
whose name - after `no-`, before `=` - holds a '.'. No option of the
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
d_internal_cli_settings_words
  What is wrong with a setting the command line named badly, as the kit says
it: the option as typed, and why. `_argv[_at]` is the option offered;
`--set` names its setting in the argument after it.
*/
static void
d_internal_cli_settings_words(
    int                _argc,
    const char* const* _argv,
    int                _at,
    int32_t            _status,
    char*              _buffer,
    size_t             _capacity
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
        (void)snprintf(_buffer, _capacity, "--set: NAME=VALUE is required");

        return;
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
        (void)snprintf(_buffer, _capacity,
                       "%s: '%s' is not a bool: give it a value, as "
                       "--%s=VALUE",
                       _argv[_at], text, text);

        return;
    }

    d_cli_settings_why(_status, text, size,
                       (text[size] == '=') ? (text + size + 1u) : NULL,
                       why, sizeof(why));
    (void)snprintf(_buffer, _capacity, "%.*s: %s",
                   (int)strcspn(_argv[_at], "="), _argv[_at], why);

    return;
}

/*
d_cli_settings_hook
  The parser's offer: an argument not shaped as a setting is the parser's,
so that `--out` goes on to become `--output`; one that is goes to the
registry, which takes it with the argument `--set` names, or refuses it,
the refusal's words kept for d_cli_settings_refused.
*/
int d_cli_settings_hook(
    void*        _ctx,
    int          _argc,
    char* const* _argv,
    int*         _at,
    int32_t*     _status
)
{
    struct dj_option_env_registry* const registry =
        (_ctx != NULL) ? (struct dj_option_env_registry*)_ctx
                       : d_option_env_registry();
    struct dj_option_result              result   = d_option_ok(0u);
    int32_t                              taken;
    int                                  first;

    if ( (_argv == NULL) ||
         (_at == NULL) ||
         (*_at < 1) ||
         (*_at >= _argc) ||
         (!d_internal_cli_settings_shaped(_argv[*_at])) )
    {
        return 0;
    }

    first = *_at;
    taken = d_option_env_registry_take_option(registry, _argc, _argv, _at,
                                              &result);

    // refused: said once the parse has ended, by the option as typed
    if (taken < 0)
    {
        d_internal_cli_settings_words(_argc, (const char* const*)_argv,
                                      first, d_option_result_status(&result),
                                      d_internal_cli_settings_refusal,
                                      sizeof(d_internal_cli_settings_refusal));
        (void)snprintf(d_internal_cli_settings_refused_argument,
                       sizeof(d_internal_cli_settings_refused_argument),
                       "%s", _argv[first]);
        d_internal_cli_settings_refused_size = strlen(_argv[first]);

        if (_status != NULL)
        {
            *_status = d_option_result_status(&result);
        }
    }

    return (int)taken;
}

/*
d_cli_settings_refused
  The refusal the parse ended with, if the kit's hook made it - the
parser's message is the argument as typed - said as a usage error from
the command the parse failed in, and let go of.
*/
int d_cli_settings_refused(
    const struct dj_cli_program* _program
)
{
    const struct dj_arg_parser* const parser  = (_program != NULL)
                                                ? _program->parser
                                                : NULL;
    const char* const                 message = (parser != NULL)
                                                ? parser->error.message
                                                : NULL;
    const size_t                      kept    =
        strlen(d_internal_cli_settings_refused_argument);
    const struct dj_arg_command*      command;
    bool                              ours;

    ours = ( (message != NULL) &&
             (d_internal_cli_settings_refused_size > 0u) &&
             (parser->error.status == (int)D_ARG_ERROR_SETTING) &&
             (strlen(message) == d_internal_cli_settings_refused_size) &&
             (strncmp(message, d_internal_cli_settings_refused_argument,
                      kept) == 0) );

    d_internal_cli_settings_refused_size = 0u;

    if (!ours)
    {
        return D_CLI_EXIT_OK;
    }

    command = parser->error.command;

    return d_cli_usage_error(_program,
                             (command != &parser->command) ? command : NULL,
                             "%s", d_internal_cli_settings_refusal);
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
    const char* const search = d_internal_cli_settings_text(
                                   d_cli_settings_djin(), "plugin_search",
                                   D_CFG_CLI_PLUGIN_SEARCH);

    return (search != NULL) ? search : "";
}

/*
d_cli_settings_plugin_path
  The root's `plugin_path`.
*/
const char* d_cli_settings_plugin_path(void)
{
    return d_internal_cli_settings_text(
               d_option_env_registry_frame(d_option_env_registry()),
               "plugin_path", NULL);
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

/*
d_internal_cli_settings_choice
  A choice of the kit's djin, by name: `_fallback`, its knob, before the
settings are started.
*/
static int64_t
d_internal_cli_settings_choice(
    const char* _name,
    int64_t     _fallback
)
{
    struct dj_option_env* const djin = d_cli_settings_djin();
    struct dj_option_result     found;

    if ( (!d_internal_cli_settings_started) ||
         (djin == NULL) )
    {
        return _fallback;
    }

    found = d_option_env_find(djin, _name, strlen(_name));

    return (found.is_ok) ? d_option_env_int(djin, D_OPTION_ENV_GLOBAL,
                                            (uint32_t)found.payload.value)
                         : _fallback;
}

/*
d_internal_cli_settings_severity
  How a held note is said, by its kind and the two settings: a severity, or
-1 for not at all.
*/
static int
d_internal_cli_settings_severity(
    int     _kind,
    int64_t _unknown,
    int64_t _refusals,
    bool    _verbose
)
{
    switch (_kind)
    {
        case D_INTERNAL_CLI_HELD_FOREIGN:
            return (_unknown == D_CLI_UNKNOWN_SETTINGS_ERROR)
                   ? (int)D_CLI_SEVERITY_ERROR
                   : (_unknown == D_CLI_UNKNOWN_SETTINGS_ALL)
                     ? (int)D_CLI_SEVERITY_WARNING
                     : -1;
        case D_INTERNAL_CLI_HELD_UNKNOWN:
            return (_unknown == D_CLI_UNKNOWN_SETTINGS_ERROR)
                   ? (int)D_CLI_SEVERITY_ERROR
                   : (_unknown == D_CLI_UNKNOWN_SETTINGS_NONE)
                     ? -1
                     : (int)D_CLI_SEVERITY_WARNING;
        case D_INTERNAL_CLI_HELD_SHARED:
            return (_refusals == D_CLI_SHARED_REFUSALS_ERROR)
                   ? (int)D_CLI_SEVERITY_ERROR
                   : (_refusals == D_CLI_SHARED_REFUSALS_WARN)
                     ? (int)D_CLI_SEVERITY_WARNING
                     : ( (_refusals == D_CLI_SHARED_REFUSALS_VERBOSE) &&
                         (_verbose) )
                       ? (int)D_CLI_SEVERITY_NOTE
                       : -1;
        default:
            return (int)D_CLI_SEVERITY_WARNING;
    }
}

/*
d_cli_settings_report
  Each held note, as the two settings now say - the command line's among
them - once; an error among them makes the run's trouble.
*/
int d_cli_settings_report(
    const struct dj_cli_program* _program
)
{
    const int64_t unknown  = d_internal_cli_settings_choice(
                                 "unknown_settings",
                                 D_CFG_CLI_UNKNOWN_SETTINGS);
    const int64_t refusals = d_internal_cli_settings_choice(
                                 "shared_refusals",
                                 D_CFG_CLI_SHARED_REFUSALS);
    const bool    verbose  = (d_cli_program_verbosity(_program) > 0);
    int           status   = D_CLI_EXIT_OK;

    for (size_t i = 0u; i < d_internal_cli_settings_count; ++i)
    {
        const struct dj_internal_cli_settings_note* const note =
            &d_internal_cli_settings_notes[i];
        const int severity = d_internal_cli_settings_severity(note->kind,
                                                              unknown,
                                                              refusals,
                                                              verbose);

        status = (severity == (int)D_CLI_SEVERITY_ERROR) ? D_CLI_EXIT_TROUBLE
                                                        : status;

        if (severity < 0)
        {
            continue;
        }

        if (note->line > 0L)
        {
            d_cli_report_at(_program, severity, d_internal_cli_settings_path,
                            note->line, 0L, "%s", note->text);
        }
        else
        {
            d_cli_report(_program, severity, "%s", note->text);
        }
    }

    if (d_internal_cli_settings_past > 0u)
    {
        d_cli_report(_program, (int)D_CLI_SEVERITY_NOTE,
                     "%lu more failures in the settings were not kept",
                     (unsigned long)d_internal_cli_settings_past);
    }

    d_internal_cli_settings_count = 0u;
    d_internal_cli_settings_past  = 0u;

    return status;
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
    d_internal_cli_settings_started      = false;
    d_internal_cli_settings_count        = 0u;
    d_internal_cli_settings_past         = 0u;
    d_internal_cli_settings_refused_size = 0u;

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

int d_cli_settings_hook(
    void*        _ctx,
    int          _argc,
    char* const* _argv,
    int*         _at,
    int32_t*     _status
)
{
    (void)_ctx;
    (void)_argc;
    (void)_argv;
    (void)_at;
    (void)_status;

    return 0;
}

int d_cli_settings_refused(
    const struct dj_cli_program* _program
)
{
    (void)_program;

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

int d_cli_settings_report(
    const struct dj_cli_program* _program
)
{
    (void)_program;

    return D_CLI_EXIT_OK;
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
