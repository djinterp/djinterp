/*******************************************************************************
* djinterp [c][tools]                                             cli_settings.c
*
* The command-line kit's settings, declared in cli_settings.h.
*   The kit's djin is made at its first use, over static storage: a run
* makes it before it starts any thread, as it parses before it works. The
* command line's settings are the parser's to offer: its setting hook is
* offered each long option the parse does not know, where the parse reads
* it, so that a setting is only ever taken where an option could stand:
* in a response file, after an abbreviated option's value, never in a
* prefix command's arguments, by the parser's own rules.
*   Which rows the djin declares is chosen when it is made, from the
* build's knobs, whether the run is privileged and the program's say: the
* rows are const, so nothing is skipped when a setting is read.
*
*
* path:      /src/djinterp/tools/cli/c/cli_settings.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.25
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
D_OPTION_ENV_DECLARE(dj_internal_cli_settings_storage, 32u, 8u, 2048u);

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

// D_INTERNAL_CLI_SETTINGS_FIXED
//   constant: the sources `cli.settings_sections` refuses: the user's file,
// which it says how to read, and the command line, read after the file.
#define D_INTERNAL_CLI_SETTINGS_FIXED                                          \
    ( D_OPTION_ENV_REFUSES(D_OPTION_ENV_SOURCE_FILE) |                         \
      D_OPTION_ENV_REFUSES(D_OPTION_ENV_SOURCE_COMMAND_LINE) )

// d_internal_cli_settings_unknown_names, _refusal_names, _djin_names
//   constants: the words `cli.unknown_settings`, `cli.shared_refusals` and
// `cli.unknown_djin` take (cfg_cli.h, 1.5).
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

static const struct dj_option_env_name d_internal_cli_settings_djin_names[] =
{
    { "reason", D_CLI_UNKNOWN_DJIN_REASON },
    { "lookup", D_CLI_UNKNOWN_DJIN_LOOKUP }
};

// D_INTERNAL_CLI_SETTINGS_PRIVILEGED_ROW
//   macro: `cli.privileged_refuse`, refusing `_refused`, where the build
// declares it (cfg_cli.h, 1.6.2); nothing where it does not.
#if (D_INTERNAL_CLI_PRIVILEGED_OPTION == 1)
    #define D_INTERNAL_CLI_SETTINGS_PRIVILEGED_ROW(_refused)                   \
        , D_OPTION_ENV_ROW_REFUSING(D_OPTION_ENV_KIND_BOOL,                    \
                                    "privileged_refuse", NULL, 0u, NULL, 0u,   \
                                    NULL,                                      \
                                    (D_CFG_CLI_PRIVILEGED_REFUSE != 0) ? 1 : 0,\
                                    INT64_MIN, INT64_MAX, 0.0, NULL,           \
                                    "D_CFG_CLI_PRIVILEGED_REFUSE",             \
                                    "whether a run with privileges its "       \
                                    "caller lacks refuses the caller the "     \
                                    "plugin settings",                         \
                                    (_refused))
#else
    #define D_INTERNAL_CLI_SETTINGS_PRIVILEGED_ROW(_refused)
#endif

// D_INTERNAL_CLI_SETTINGS_ROWS
//   macro: the kit's variables, each defaulting to its knob: the plugin
// rows refusing `_plugins`, and `cli.privileged_refuse` `_privileged`.
#define D_INTERNAL_CLI_SETTINGS_ROWS(_plugins, _privileged)                    \
{                                                                              \
    D_OPTION_ENV_ROW_REFUSING(D_OPTION_ENV_KIND_TEXT, "plugin_search", NULL,   \
                              0u, NULL, 0u, D_CFG_CLI_PLUGIN_SEARCH, 0,        \
                              INT64_MIN, INT64_MAX, 0.0, NULL,                 \
                              "D_CFG_CLI_PLUGIN_SEARCH",                       \
                              "the directories a command's module is looked "  \
                              "for in, after the run's and plugin_path's",     \
                              (_plugins)),                                     \
    D_OPTION_ENV_BOOL_KNOB("plugin_strict_version",                            \
                           D_CFG_CLI_PLUGIN_STRICT_VERSION,                    \
                           "whether a module of another version of the "      \
                           "program is refused, or loaded with a warning"),   \
    D_OPTION_ENV_CHOICE_KNOB("unknown_settings",                               \
                             d_internal_cli_settings_unknown_names,            \
                             D_CFG_CLI_UNKNOWN_SETTINGS,                       \
                             "what is said of a line of the user's file "      \
                             "that names no setting here: none, typos, all "   \
                             "or error"),                                      \
    D_OPTION_ENV_CHOICE_KNOB("shared_refusals",                                \
                             d_internal_cli_settings_refusal_names,            \
                             D_CFG_CLI_SHARED_REFUSALS,                        \
                             "what is said when the user's file or "           \
                             "DJINTERP_ sets a setting that refuses it: "      \
                             "none, verbose, warn or error"),                  \
    D_OPTION_ENV_CHOICE_KNOB("unknown_djin",                                   \
                             d_internal_cli_settings_djin_names,               \
                             D_CFG_CLI_UNKNOWN_DJIN,                           \
                             "how a name for a djin not registered here is "  \
                             "told: by the registry's reason, or by the "      \
                             "kit's lookup of its first part"),                \
    D_OPTION_ENV_ROW_REFUSING(D_OPTION_ENV_KIND_BOOL, "settings_sections",     \
                              NULL, 0u, NULL, 0u, NULL,                        \
                              (D_CFG_CLI_SETTINGS_SECTIONS != 0) ? 1 : 0,      \
                              INT64_MIN, INT64_MAX, 0.0, NULL,                 \
                              "D_CFG_CLI_SETTINGS_SECTIONS",                   \
                              "whether the user's file is read in sections, "  \
                              "[PROGRAM] the lines for one program alone",     \
                              D_INTERNAL_CLI_SETTINGS_FIXED)                   \
    D_INTERNAL_CLI_SETTINGS_PRIVILEGED_ROW(_privileged)                        \
}

// d_internal_cli_settings_rows, _closed, _trusted
//   constants: the rows of an ordinary run, which refuse what the build
// says; of a privileged run that refuses the caller the plugin settings
// (cfg_cli.h, 1.6); and of a privileged run whose program, or build,
// trusts the caller with them. In either privileged run only the program
// sets `cli.privileged_refuse`.
static const struct dj_option_env_var d_internal_cli_settings_rows[] =
    D_INTERNAL_CLI_SETTINGS_ROWS(D_INTERNAL_CLI_PLUGIN_REFUSED, 0u);

static const struct dj_option_env_var d_internal_cli_settings_rows_closed[] =
    D_INTERNAL_CLI_SETTINGS_ROWS(D_OPTION_ENV_REFUSES_CALLER,
                                 D_OPTION_ENV_REFUSES_CALLER);

static const struct dj_option_env_var d_internal_cli_settings_rows_trusted[] =
    D_INTERNAL_CLI_SETTINGS_ROWS(D_INTERNAL_CLI_PLUGIN_REFUSED,
                                 D_OPTION_ENV_REFUSES_CALLER);

// D_INTERNAL_CLI_SETTINGS_ROW_COUNT
//   constant: how many rows each of the three holds.
#define D_INTERNAL_CLI_SETTINGS_ROW_COUNT                                      \
    ((uint32_t)(sizeof(d_internal_cli_settings_rows) /                         \
                sizeof(d_internal_cli_settings_rows[0])))

// D_INTERNAL_CLI_SETTINGS_PATH_ROW
//   macro: the program's own `plugin_path`, on the registry's root, read as
// PROGRAM_PLUGIN_PATH: directories looked in before the build's, refusing
// `_refused`.
#define D_INTERNAL_CLI_SETTINGS_PATH_ROW(_refused)                             \
    D_OPTION_ENV_ROW_REFUSING(D_OPTION_ENV_KIND_TEXT, "plugin_path", NULL, 0u, \
                              NULL, 0u, NULL, 0, INT64_MIN, INT64_MAX, 0.0,    \
                              NULL, NULL,                                      \
                              "the directories a command's module is looked "  \
                              "for in first, separated as PATH separates "     \
                              "them",                                          \
                              (_refused))

// d_internal_cli_settings_path_row, _path_row_closed
//   constants: `plugin_path` as the build says, and as a privileged run that
// refuses the caller declares it.
static const struct dj_option_env_var d_internal_cli_settings_path_row =
    D_INTERNAL_CLI_SETTINGS_PATH_ROW(D_INTERNAL_CLI_PLUGIN_REFUSED);

static const struct dj_option_env_var d_internal_cli_settings_path_row_closed =
    D_INTERNAL_CLI_SETTINGS_PATH_ROW(D_OPTION_ENV_REFUSES_CALLER);

// d_internal_cli_settings_frame, _room, _made, _started
//   variables: the djin, its storage, whether it is made, and whether the
// run's settings are started.
static struct dj_option_env                    d_internal_cli_settings_frame;
static struct dj_internal_cli_settings_storage d_internal_cli_settings_room;
static bool                                    d_internal_cli_settings_made;
static bool                                    d_internal_cli_settings_started;

// d_internal_cli_settings_say, _privileged, _closed
//   variables: the program's say on a privileged run, a
// dj_cli_plugin_privileged; and, from when the djin is made, whether the run
// is privileged, and whether the plugin rows refuse the caller.
static int  d_internal_cli_settings_say = (int)D_CLI_PLUGIN_PRIVILEGED_DEFAULT;
static bool d_internal_cli_settings_privileged;
static bool d_internal_cli_settings_closed;

// D_INTERNAL_CLI_SETTINGS_HELD
//   constant: the most notes the run's start holds; past them, counted.
#define D_INTERNAL_CLI_SETTINGS_HELD 32u

// dj_internal_cli_settings_kind
//   enum: what a held note is about, which decides what says it.
enum dj_internal_cli_settings_kind
{
    D_INTERNAL_CLI_HELD_FOREIGN    = 0,  // a djin not registered here
    D_INTERNAL_CLI_HELD_UNKNOWN    = 1,  // a near miss, or a djin's unknown
    D_INTERNAL_CLI_HELD_SHARED     = 2,  // refused from a shared source
    D_INTERNAL_CLI_HELD_OWN        = 3,  // refused from the program's prefix
    D_INTERNAL_CLI_HELD_VALUE      = 4,  // a value the setting does not take
    D_INTERNAL_CLI_HELD_PRIVILEGED = 5   // a privileged run trusts the caller
};

// dj_internal_cli_settings_note
//   struct: a note the run's start held: its kind and words as each way of
// telling an unknown djin reads it (cfg_cli.h, 1.5.3), the two the same for
// any other note, so the way the run ends with decides; and its line in the
// user's file, 0 for the environment.
struct dj_internal_cli_settings_note
{
    int  kind[2];
    long line;
    char text[2][D_INTERNAL_CLI_SETTINGS_ROOM];
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
d_internal_cli_settings_hold_both
  A note kept for d_cli_settings_report, as each way of telling an unknown
djin reads it; past the room, counted.
*/
static void
d_internal_cli_settings_hold_both(
    int         _kind,
    int         _kind_lookup,
    long        _line,
    const char* _text,
    const char* _text_lookup
)
{
    struct dj_internal_cli_settings_note* note;

    if (d_internal_cli_settings_count >= D_INTERNAL_CLI_SETTINGS_HELD)
    {
        ++d_internal_cli_settings_past;

        return;
    }

    note          = &d_internal_cli_settings_notes[
                        d_internal_cli_settings_count];
    note->kind[0] = _kind;
    note->kind[1] = _kind_lookup;
    note->line    = _line;
    (void)snprintf(note->text[0], sizeof(note->text[0]), "%s", _text);
    (void)snprintf(note->text[1], sizeof(note->text[1]), "%s", _text_lookup);
    ++d_internal_cli_settings_count;

    return;
}

/*
d_internal_cli_settings_hold
  A note either way of telling an unknown djin reads alike.
*/
static void
d_internal_cli_settings_hold(
    int         _kind,
    long        _line,
    const char* _text
)
{
    d_internal_cli_settings_hold_both(_kind, _kind, _line, _text, _text);

    return;
}

/*
d_internal_cli_settings_refusing
  Whether a privileged run refuses the caller the plugin settings: the
program's say, where it gave one and the build declares the setting; else
the knob.
*/
static bool
d_internal_cli_settings_refusing(void)
{
#if (D_INTERNAL_CLI_PRIVILEGED_OPTION == 1)
    if (d_internal_cli_settings_say == (int)D_CLI_PLUGIN_PRIVILEGED_REFUSE)
    {
        return true;
    }

    if (d_internal_cli_settings_say == (int)D_CLI_PLUGIN_PRIVILEGED_TRUST)
    {
        return false;
    }
#endif

    return (D_INTERNAL_CLI_PRIVILEGED_REFUSE == 1);
}

/*
d_cli_settings_djin
  Made at the first call: the frame over its storage, named, the rows the
run calls for declared (cfg_cli.h, 1.6).
*/
struct dj_option_env* d_cli_settings_djin(void)
{
    const struct dj_option_env_var* rows = d_internal_cli_settings_rows;

    if (d_internal_cli_settings_made)
    {
        return &d_internal_cli_settings_frame;
    }

    d_internal_cli_settings_privileged = d_cli_internal_privileged();
    d_internal_cli_settings_closed     =
        ( (D_INTERNAL_CLI_PLUGIN_VARIABLE == 0) ||
          ( (d_internal_cli_settings_privileged) &&
            (d_internal_cli_settings_refusing()) ) );

    if (d_internal_cli_settings_privileged)
    {
        rows = (d_internal_cli_settings_refusing())
               ? d_internal_cli_settings_rows_closed
               : d_internal_cli_settings_rows_trusted;
    }

    memset(&d_internal_cli_settings_room, 0,
           sizeof(d_internal_cli_settings_room));
    d_internal_cli_settings_made =
        ( (D_OPTION_ENV_VIEW(&d_internal_cli_settings_frame,
                             d_internal_cli_settings_room).is_ok) &&
          (d_option_env_set_name(&d_internal_cli_settings_frame,
                                 "cli").is_ok) &&
          (d_option_env_declare_all(&d_internal_cli_settings_frame, rows,
                                    D_INTERNAL_CLI_SETTINGS_ROW_COUNT)
               .is_ok) );

    return (d_internal_cli_settings_made)
           ? &d_internal_cli_settings_frame
           : NULL;
}

/*
d_cli_settings_privileged
  Kept until the djin is made, which reads it; too late after.
*/
bool d_cli_settings_privileged(
    int _say
)
{
    if ( (d_internal_cli_settings_made) ||
         ( (D_INTERNAL_CLI_PRIVILEGED_OPTION == 0) &&
           (_say != (int)D_CLI_PLUGIN_PRIVILEGED_DEFAULT) ) )
    {
        return false;
    }

    d_internal_cli_settings_say =
        ( (_say == (int)D_CLI_PLUGIN_PRIVILEGED_REFUSE) ||
          (_say == (int)D_CLI_PLUGIN_PRIVILEGED_TRUST) )
        ? _say
        : (int)D_CLI_PLUGIN_PRIVILEGED_DEFAULT;

    return true;
}

/*
d_cli_settings_closed
  The build's knob, or a privileged run that refuses the caller.
*/
bool d_cli_settings_closed(void)
{
    (void)d_cli_settings_djin();

    return d_internal_cli_settings_closed;
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
d_internal_cli_settings_choice_of
  A choice of the kit's djin, by name: `_fallback`, its knob, before the
settings are started or where the djin has no such row.
*/
static int64_t
d_internal_cli_settings_choice_of(
    const char* _name,
    int64_t     _fallback
);

/*
d_internal_cli_settings_flag
  A bool of the kit's djin, by name: `_fallback`, its knob, before the
settings are started or where the djin has no such row.
*/
static bool
d_internal_cli_settings_flag(
    const char* _name,
    bool        _fallback
);

/*
d_internal_cli_settings_foreign
  Whether `_name` (of `_size` bytes) names a djin not registered here, as
`_way` tells it (cfg_cli.h, 1.5.3): the registry's reason FRAME; or no djin
here having its first part. A name with no '.' names a root setting, never
a djin.
*/
static bool
d_internal_cli_settings_foreign(
    const char* _name,
    size_t      _size,
    const char* _value,
    int64_t     _way
)
{
    const char* const dot = (_name != NULL)
                            ? (const char*)memchr(_name, '.', _size)
                            : NULL;

    if (dot == NULL)
    {
        return false;
    }

    if (_way == D_CLI_UNKNOWN_DJIN_LOOKUP)
    {
        return (d_option_env_registry_find(d_option_env_registry(), _name,
                                           (size_t)(dot - _name)) == NULL);
    }

    return (d_internal_cli_settings_reason(_name, _size, _value,
                                           (int32_t)D_OPTION_ENV_SOURCE_FILE)
            == (int32_t)D_OPTION_ENV_REASON_FRAME);
}

/*
d_internal_cli_settings_source_word
  How a source is named where a setting refuses it.
*/
static const char*
d_internal_cli_settings_source_word(
    int32_t _source
)
{
    return (_source == (int32_t)D_OPTION_ENV_SOURCE_COMMAND_LINE)
           ? "the command line"
           : (_source == (int32_t)D_OPTION_ENV_SOURCE_ENVIRONMENT)
             ? "the environment"
             : "the user's settings file";
}

/*
d_internal_cli_settings_why_by
  d_cli_settings_why for a setting given from `_source`, an unknown djin
told `_way`: a refusal because the setting does not take that source says
so; a policy that refuses the value, that the build does.
*/
static void
d_internal_cli_settings_why_by(
    int32_t     _status,
    const char* _name,
    size_t      _size,
    const char* _value,
    int32_t     _source,
    int64_t     _way,
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

        // no djin by the first part, and none near: the djin is said
        if ( (found == 0u) &&
             (d_internal_cli_settings_foreign(_name, _size, _value, _way)) )
        {
            (void)snprintf(_buffer, _capacity, "no djin is named '%.*s'",
                           (int)((const char*)memchr(_name, '.', _size) -
                                 _name), _name);

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
    else if ( (_status == (int32_t)D_OPTION_STATUS_POLICY_REJECTED) &&
              (d_internal_cli_settings_reason(_name, _size, _value,
                                              _source) ==
               (int32_t)D_OPTION_ENV_REASON_SOURCE) )
    {
        (void)snprintf(_buffer, _capacity, "'%.*s' is not taken from %s",
                       shown, _name,
                       d_internal_cli_settings_source_word(_source));
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
    else if (_status == (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT)
    {
        // text with no '=' where a statement was to be
        (void)snprintf(_buffer, _capacity,
                       "'%.*s' is not a statement: NAME = VALUE",
                       shown, _name);
    }
    else
    {
        (void)snprintf(_buffer, _capacity,
                       "'%.*s' cannot be set so (%s)",
                       shown, _name, d_option_status_name(_status));
    }

    return;
}

/*
d_cli_settings_why
  As the user's file gives it, an unknown djin told as the run says now.
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
    d_internal_cli_settings_why_by(
        _status, _name, _size, _value, (int32_t)D_OPTION_ENV_SOURCE_FILE,
        d_internal_cli_settings_choice_of("unknown_djin",
                                          D_CFG_CLI_UNKNOWN_DJIN),
        _buffer, _capacity);

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
djin not registered here as `_way` tells it, or near one or a djin's that
is, the two kinds `cli.unknown_settings` tells apart (cfg_cli.h, 1.5.1); a
source the setting refuses; or a value it does not take.
*/
static int
d_internal_cli_settings_kind_of(
    int32_t     _status,
    const char* _name,
    size_t      _size,
    const char* _value,
    int64_t     _way
)
{
    char near[D_INTERNAL_CLI_SETTINGS_ROOM];

    // a djin not registered here, where no name is near
    if (_status == (int32_t)D_OPTION_STATUS_KEY_ABSENT)
    {
        const size_t found = d_option_env_registry_suggest(
                                 d_option_env_registry(), _name, _size,
                                 near, sizeof(near));

        return ( (found == 0u) &&
                 (d_internal_cli_settings_foreign(_name, _size, _value,
                                                  _way)) )
               ? (int)D_INTERNAL_CLI_HELD_FOREIGN
               : (int)D_INTERNAL_CLI_HELD_UNKNOWN;
    }

    if ( (_status == (int32_t)D_OPTION_STATUS_POLICY_REJECTED) &&
         (_value != NULL) &&
         (d_internal_cli_settings_reason(
              _name, _size, _value, (int32_t)D_OPTION_ENV_SOURCE_FILE) ==
          (int32_t)D_OPTION_ENV_REASON_SOURCE) )
    {
        return (int)D_INTERNAL_CLI_HELD_SHARED;
    }

    return (int)D_INTERNAL_CLI_HELD_VALUE;
}

/*
d_internal_cli_settings_header
  The section header at `_at`, which holds a '[', into `_header`: where its
line ends, and what its brackets hold; a `size` of 0 where the line is not
`[NAME]` or `[*]`, blanks and a comment after it allowed, NAME of letters,
digits, '_', '-' and '.'.
*/
static void
d_internal_cli_settings_header(
    const char*                    _text,
    size_t                         _at,
    struct dj_cli_settings_header* _header
)
{
    size_t end   = _at;
    size_t close = _at + 1u;
    bool   good  = true;

    while ( (_text[end] != '\0') &&
            (_text[end] != '\n') )
    {
        ++end;
    }

    while ( (close < end) &&
            (_text[close] != ']') )
    {
        const char c = _text[close];

        good = ( (good) &&
                 ( ( (c >= 'a') && (c <= 'z') ) ||
                   ( (c >= 'A') && (c <= 'Z') ) ||
                   ( (c >= '0') && (c <= '9') ) ||
                   (c == '_') || (c == '-') || (c == '.') ||
                   ( (c == '*') && (close == _at + 1u) &&
                     (_text[close + 1u] == ']') ) ) );
        ++close;
    }

    _header->at   = _at;
    _header->end  = end;
    _header->name = _text + _at + 1u;
    _header->size = ( (good) && (close < end) && (close > _at + 1u) )
                    ? (close - _at - 1u)
                    : 0u;

    // after the ']', only blanks and a comment
    for (size_t at = close + 1u; (_header->size > 0u) && (at < end); ++at)
    {
        if (_text[at] == '#')
        {
            break;
        }

        if ( (_text[at] != ' ') &&
             (_text[at] != '\t') &&
             (_text[at] != '\r') )
        {
            _header->size = 0u;
        }
    }

    return;
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
        struct dj_cli_settings_header header;
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
        char                          why[2][D_INTERNAL_CLI_SETTINGS_ROOM];
        const char*                   given;
        int                           kind[2];
        int32_t                       status;

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

        given  = d_internal_cli_settings_value_at(_script, at, end, value,
                                                  sizeof(value));
        status = d_option_result_status(&done);

        // each way of telling an unknown djin, the run's end to choose
        for (int way = 0; way < 2; ++way)
        {
            d_internal_cli_settings_why_by(status, name, size, given,
                                           (int32_t)D_OPTION_ENV_SOURCE_FILE,
                                           (int64_t)way, why[way],
                                           sizeof(why[way]));
            kind[way] = d_internal_cli_settings_kind_of(status, name, size,
                                                        given, (int64_t)way);
        }

        // a header, in a file read without sections
        header.size = 0u;

        if (_script[at] == '[')
        {
            d_internal_cli_settings_header(_script, at, &header);
        }

        if (header.size > 0u)
        {
            (void)snprintf(why[0], sizeof(why[0]), "'%.*s' is a section's "
                           "header, and this run reads the file without "
                           "sections (cli.settings_sections)",
                           (int)(header.size + 2u), _script + at);
            (void)memcpy(why[1], why[0], sizeof(why[1]));
            kind[0] = (int)D_INTERNAL_CLI_HELD_VALUE;
            kind[1] = kind[0];
        }

        d_internal_cli_settings_hold_both(kind[0], kind[1], line, why[0],
                                          why[1]);

        // on from the end of the one that failed, and never back
        offset = (end > at) ? end : (at + 1u);
    }

    return;
}

/*
d_internal_cli_settings_blank
  The text from `_from` to `_to` made blanks, its line breaks kept, so that
the statements after it are found on the lines they were written on.
*/
static void
d_internal_cli_settings_blank(
    char*  _text,
    size_t _from,
    size_t _to
)
{
    for (size_t at = _from; at < _to; ++at)
    {
        _text[at] = (_text[at] == '\n') ? '\n' : ' ';
    }

    return;
}

bool d_cli_settings_next_header(
    const char*                    _text,
    size_t                         _from,
    struct dj_cli_settings_header* _header
)
{
    size_t at = _from;

    if ( (_text == NULL) ||
         (_header == NULL) )
    {
        return false;
    }

    while (_text[at] != '\0')
    {
        const char c = _text[at];
        size_t     end;

        // between statements: blanks, line breaks, ';' and comments
        if ( (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n') ||
             (c == ';') )
        {
            ++at;

            continue;
        }

        if (c == '[')
        {
            d_internal_cli_settings_header(_text, at, _header);

            return true;
        }

        end = (c == '#') ? (at + strcspn(_text + at, "\n"))
                         : d_option_env_statement_end(_text, at);
        at  = (end > at) ? end : (at + 1u);
    }

    _header->at   = at;
    _header->end  = at;
    _header->name = _text + at;
    _header->size = 0u;

    return false;
}

bool d_cli_settings_sectioned(void)
{
    return d_internal_cli_settings_flag(
        "settings_sections", (D_INTERNAL_CLI_SETTINGS_SECTIONS == 1));
}

/*
d_internal_cli_settings_sections
  The user's file read in sections (cfg_cli.h, 1.5.4): each header, and the
text of each section another program's, made blanks; a header the kit cannot
read held as a note, the lines after it no program's until the next.
*/
static void
d_internal_cli_settings_sections(
    char*       _text,
    const char* _program
)
{
    struct dj_cli_settings_header header;
    size_t                        at    = 0u;
    bool                          mine  = true;
    bool                          found = true;
    long                          line  = 1;
    size_t                        count = 0u;

    while (found)
    {
        char said[D_INTERNAL_CLI_SETTINGS_ROOM];

        found = d_cli_settings_next_header(_text, at, &header);

        if (!mine)
        {
            d_internal_cli_settings_blank(_text, at, header.at);
        }

        if (!found)
        {
            break;
        }

        mine = (header.size > 0u) &&
               (d_cli_settings_section_reads(header.name, header.size,
                                             _program));

        if (header.size == 0u)
        {
            for (; count < header.at; ++count)
            {
                line += (_text[count] == '\n') ? 1 : 0;
            }

            (void)snprintf(said, sizeof(said),
                           "'%.*s' is not a section's header: [PROGRAM] or "
                           "[*]", (int)(header.end - header.at),
                           _text + header.at);
            d_internal_cli_settings_hold(
                (int)D_INTERNAL_CLI_HELD_UNKNOWN, line, said);
        }

        d_internal_cli_settings_blank(_text, header.at, header.end);
        at = header.end;
    }

    return;
}

/*
d_internal_cli_settings_read_file
  The user's file, read whole: one not there, or that the system names no
home for, is nothing; one that cannot be read otherwise is said, and passed
over. Read in sections where `cli.settings_sections` says so.
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

        if (d_internal_cli_settings_flag(
                "settings_sections", (D_INTERNAL_CLI_SETTINGS_SECTIONS == 1)))
        {
            d_internal_cli_settings_sections(
                text, (_program != NULL) ? _program->name : NULL);
        }

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
d_internal_cli_settings_privileged_said
  The program's say on a privileged run, bound as the program's own; and,
in a privileged run that trusts the caller with the plugin settings, a
note that says so, and whose word it is: the program's, or the build's
(cfg_cli.h, 1.6).
*/
static void
d_internal_cli_settings_privileged_said(void)
{
#if (D_INTERNAL_CLI_PRIVILEGED_OPTION == 1)
    if (d_internal_cli_settings_say != (int)D_CLI_PLUGIN_PRIVILEGED_DEFAULT)
    {
        (void)d_option_env_registry_load(
                  d_option_env_registry(),
                  (d_internal_cli_settings_say ==
                   (int)D_CLI_PLUGIN_PRIVILEGED_REFUSE)
                  ? "cli.privileged_refuse = true"
                  : "cli.privileged_refuse = false",
                  D_OPTION_ENV_SOURCE_PROGRAM, NULL);
    }
#endif

    // the caller's sources cannot set cli.privileged_refuse here: it is
    // the program's say, or the build's
    if ( (d_internal_cli_settings_privileged) &&
         (!d_internal_cli_settings_closed) )
    {
        d_internal_cli_settings_hold(
            (int)D_INTERNAL_CLI_HELD_PRIVILEGED, 0L,
            (d_internal_cli_settings_say ==
             (int)D_CLI_PLUGIN_PRIVILEGED_TRUST)
            ? "running with privileges its caller lacks, and taking the "
              "plugin settings from the caller, as the program says"
            : "running with privileges its caller lacks, and taking the "
              "plugin settings from the caller, as the build says "
              "(D_CFG_CLI_PRIVILEGED_REFUSE)");
    }

    return;
}

/*
d_internal_cli_settings_path_declare
  The root's `plugin_path` declared as `_row` says. Where a start before
this one, in the same process, declared the kit's other row, this one takes
its place: the two differ in the sources they refuse alone, and the root
keeps every declaration it is given (option_env.h, 2.1.2). A row of
another's by that name is left as it is.
*/
static void
d_internal_cli_settings_path_declare(
    struct dj_option_env*           _root,
    const struct dj_option_env_var* _row
)
{
    const struct dj_option_result found = d_option_env_find(
                                              _root, "plugin_path", 11u);
    uint32_t                      index;

    if (!found.is_ok)
    {
        (void)d_option_env_declare(_root, _row);

        return;
    }

    index = D_OPTION_ENV_ID_INDEX(found.payload.value);

    if ( (_root->depth == 0u) &&
         (index < _root->var_count) &&
         (_root->vars[index] != _row) &&
         ( (_root->vars[index] == &d_internal_cli_settings_path_row) ||
           (_root->vars[index] == &d_internal_cli_settings_path_row_closed) ) )
    {
        _root->vars[index] = _row;
        ++_root->generation;
    }

    return;
}

/*
d_cli_settings_start
  Once: the djin and the program's own variable, the program's say on a
privileged run, the environment under the shared prefix and then the
program's own, and the user's file, in its sections, each failure held for
d_cli_settings_report.
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

    d_internal_cli_settings_path_declare(
        d_option_env_registry_frame(registry),
        ( (d_internal_cli_settings_privileged) &&
          (d_internal_cli_settings_closed) )
        ? &d_internal_cli_settings_path_row_closed
        : &d_internal_cli_settings_path_row);
    d_internal_cli_settings_privileged_said();

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
    const char* value = "true";
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
        text  += 3;
        value  = "false";
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

    // a bool's form checked as the value it gives
    d_internal_cli_settings_why_by(
        _status, text, size, (text[size] == '=') ? (text + size + 1u) : value,
        (int32_t)D_OPTION_ENV_SOURCE_COMMAND_LINE,
        d_internal_cli_settings_choice_of("unknown_djin",
                                          D_CFG_CLI_UNKNOWN_DJIN),
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
d_internal_cli_settings_choice_of
  As declared above: the binding in force once the settings are started.
*/
static int64_t
d_internal_cli_settings_choice_of(
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
d_internal_cli_settings_flag
  As declared above: the binding in force once the settings are started.
*/
static bool
d_internal_cli_settings_flag(
    const char* _name,
    bool        _fallback
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

    return (found.is_ok) ? d_option_env_bool(djin, D_OPTION_ENV_GLOBAL,
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
d_internal_cli_settings_in_order
  The held notes in the order a reader meets them: the environment's and
the run's first, as held, then the file's by line, as held within a line.
*/
static void
d_internal_cli_settings_in_order(void)
{
    struct dj_internal_cli_settings_note* const notes =
        d_internal_cli_settings_notes;

    for (size_t i = 1u; i < d_internal_cli_settings_count; ++i)
    {
        struct dj_internal_cli_settings_note held = notes[i];
        size_t                               at   = i;

        while ( (at > 0u) &&
                (notes[at - 1u].line > held.line) )
        {
            notes[at] = notes[at - 1u];
            --at;
        }

        notes[at] = held;
    }

    return;
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
    const int64_t unknown  = d_internal_cli_settings_choice_of(
                                 "unknown_settings",
                                 D_CFG_CLI_UNKNOWN_SETTINGS);
    const int64_t refusals = d_internal_cli_settings_choice_of(
                                 "shared_refusals",
                                 D_CFG_CLI_SHARED_REFUSALS);
    const int     way      = (d_internal_cli_settings_choice_of(
                                  "unknown_djin", D_CFG_CLI_UNKNOWN_DJIN) ==
                              D_CLI_UNKNOWN_DJIN_LOOKUP) ? 1 : 0;
    const bool    verbose  = (d_cli_program_verbosity(_program) > 0);
    int           status   = D_CLI_EXIT_OK;

    d_internal_cli_settings_in_order();

    for (size_t i = 0u; i < d_internal_cli_settings_count; ++i)
    {
        const struct dj_internal_cli_settings_note* const note =
            &d_internal_cli_settings_notes[i];
        const int severity = d_internal_cli_settings_severity(note->kind[way],
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
                            note->line, 0L, "%s", note->text[way]);
        }
        else
        {
            d_cli_report(_program, severity, "%s", note->text[way]);
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
  The djin off the list and to be made again, as the next run calls for,
every binding gone, the program's say on a privileged run forgotten, and
the start due again.
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
    d_internal_cli_settings_made         = false;
    d_internal_cli_settings_say          =
        (int)D_CLI_PLUGIN_PRIVILEGED_DEFAULT;
    d_internal_cli_settings_started      = false;
    d_internal_cli_settings_count        = 0u;
    d_internal_cli_settings_past         = 0u;
    d_internal_cli_settings_refused_size = 0u;

    return;
}

void d_cli_settings_pretend_privileged(
    int _answer
)
{
    d_cli_internal_privileged_pretend(_answer);

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

bool d_cli_settings_privileged(
    int _say
)
{
    (void)_say;

    return false;
}

bool d_cli_settings_closed(void)
{
    return (D_INTERNAL_CLI_PLUGIN_VARIABLE == 0);
}

bool d_cli_settings_next_header(
    const char*                    _text,
    size_t                         _from,
    struct dj_cli_settings_header* _header
)
{
    (void)_text;

    if (_header != NULL)
    {
        _header->at   = _from;
        _header->end  = _from;
        _header->name = NULL;
        _header->size = 0u;
    }

    return false;
}

bool d_cli_settings_sectioned(void)
{
    return false;
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

void d_cli_settings_pretend_privileged(
    int _answer
)
{
    d_cli_internal_privileged_pretend(_answer);

    return;
}

#endif  // D_TESTING
#endif  // defined(D_TESTING)

#endif  // RE_STD_HAS_INT64


//==============================================================================
// IN EVERY BUILD
//==============================================================================

bool d_cli_settings_section_reads(
    const char* _name,
    size_t      _size,
    const char* _program
)
{
    if ( (_name != NULL) &&
         (_size == 1u) &&
         (_name[0] == '*') )
    {
        return true;
    }

    if ( (_name == NULL) ||
         (_program == NULL) ||
         (strlen(_program) != _size) )
    {
        return false;
    }

    for (size_t i = 0u; i < _size; ++i)
    {
        const char a = ( (_name[i] >= 'A') && (_name[i] <= 'Z') )
                       ? (char)(_name[i] - 'A' + 'a') : _name[i];
        const char b = ( (_program[i] >= 'A') && (_program[i] <= 'Z') )
                       ? (char)(_program[i] - 'A' + 'a') : _program[i];

        if (a != b)
        {
            return false;
        }
    }

    return true;
}
