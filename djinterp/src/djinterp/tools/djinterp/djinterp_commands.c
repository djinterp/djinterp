/*******************************************************************************
* djinterp [c][tools]                                        djinterp_commands.c
*
* The djinterp program's 0.8 preview, declared in djinterp_commands.h.
*   The program registers every core djin - parse's, desc's, the
* environment printer's and the log's - before it runs, and the kit starts
* the run's settings at the program's first parse, so a command's work finds
* the registry whole: those djins, the kit's, the root's variables, and every
* binding from the user's file, the environment and the command line. `list`
* and `describe` are the djin records' text writers (c/modules/interp/djin.h);
* `describe --format=json` runs their fact describers (djin_desc.h) through
* desc's JSON machine, and `describe DJIN COMMAND` describes one command.
* `get` writes each setting as a statement d_option_env_load reads back, with
* d_option_env_format_quoted, and, at `--show-origin`, where its value came
* from and what lies under it. `set` checks each setting as the file will
* bind it and binds it for the run, so that a name or a value the setting
* does not take is refused before anything is written, then writes the
* user's file whole, under each setting's canonical name, as the run reads
* the file: in every program's lines, or in `--section`'s program's. A line
* among them that sets the same name alone is replaced; the new statements
* go after the last statement of every program's first lines, before the
* first header, or of the program's last section, which is made at the
* file's end where it has none; and a line kept after them that sets the
* same name for a program that reads them is said, by its line, since that
* program reads it last. The headers are kept as the file's structure even
* in a run that reads it as one part (`cli.settings_sections` false), so
* that no other program's lines are taken for this one's. Results go
* through the kit's watched output
* (d_cli_program_watched), so that an output that refuses them is said with
* the system's reason.
*
*
* path:      /src/djinterp/tools/djinterp/djinterp_commands.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.25
*******************************************************************************/
#include "./djinterp_commands.h"  // corresponding header
// std
#include <errno.h>    // ENOENT, ENOTDIR, ENOMEM, errno
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdio.h>    // snprintf
#include <stdlib.h>   // the hooks' malloc and free
#include <string.h>   // memchr, memcmp, memcpy, strlen
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../inc/djinterp/c/desc/desc_machines.h"  // JSON
#include "../../../../inc/djinterp/c/desc/desc_options.h"  // desc's djin
#include "../../../../inc/djinterp/c/dversion.h"  // D_VERSION_STRING
#include "../../../../inc/djinterp/c/filesys/file_dir.h"  // d_dir_create_parents
#include "../../../../inc/djinterp/c/log/log.h"  // the log's djin
#include "../../../../inc/djinterp/c/modules/interp/djin.h"  // the records
#include "../../../../inc/djinterp/c/modules/interp/djin_desc.h"  // as facts
#include "../../../../inc/djinterp/c/modules/parse/settings.h"  // parse's djin
#include "../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_command.h"  // commands
#include "../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_option.h"  // options
#include "../../../../inc/djinterp/c/option/option_env.h"  // frames
#include "../../../../inc/djinterp/c/option/option_env_registry.h"  // registry
#include "../../../../inc/djinterp/c/util/sink_common.h"  // d_sink_emit
#include "../../../../inc/djinterp/c/util/ostream/ostream.h"  // dj_ostream
#include "../../../../inc/djinterp/config/tools/cli/cfg_cli.h"  // the kit's hooks
#include "../../../../inc/djinterp/env/env_printer.h"  // the printer's djin
#include "../../../../inc/djinterp/tools/cli/c/cli_help.h"  // the description
#include "../../../../inc/djinterp/tools/cli/c/cli_io.h"  // files
#include "../../../../inc/djinterp/tools/cli/c/cli_settings.h"  // the user's file


//==============================================================================
// 1.  THE PROGRAM'S DJINS
//==============================================================================

// D_INTERNAL_DJINTERP_ROOM
//   constant: the room for a line the program writes, a value, a name.
#define D_INTERNAL_DJINTERP_ROOM 512u

// D_INTERNAL_DJINTERP_DJINS
//   constant: the most djins the program's set holds.
#define D_INTERNAL_DJINTERP_DJINS 16u

// D_INTERNAL_DJINTERP_LAYERS
//   constant: the most layers of a setting `get --show-origin` shows: as
// many as the option core lists (option_env.c keeps the top 64), far more
// than a frame chain of the program's can bind - a binding from each of
// five sources, in two scopes, a frame. Past them the default's line comes
// from the row.
#define D_INTERNAL_DJINTERP_LAYERS 64u

// d_internal_djinterp_reserved
//   constant: the names the program keeps for its own commands and the
// kit's, and those of its manual's pages in sections 5 and 7, which a
// djin's page, djinterp-NAME(1), would otherwise share: djinterp-settings(5),
// djinterp-cli(7) and djinterp-language(7).
static const char* const d_internal_djinterp_reserved[] =
{
    "list", "describe", "get", "set", "help", "completion",
    "settings", "cli", "language", NULL
};

// d_internal_djinterp_room, d_internal_djinterp_set
//   variables: the program's djins: the room and the set over it.
static const struct dj_djin*
    d_internal_djinterp_room[D_INTERNAL_DJINTERP_DJINS];
static struct dj_djin_set d_internal_djinterp_set;

/*
d_internal_djinterp_put
  Text to the program's output, whole or said as a failure to write it, with
the reason the output gave.
*/
static bool
d_internal_djinterp_put(
    const struct dj_cli_program* _program,
    const char*                  _text,
    size_t                       _size
)
{
    struct dj_cli_watch       watch;
    const struct dj_pack_sink out = d_cli_program_watched(_program, &watch);

    if (d_sink_emit(out, _text, _size) == 1)
    {
        return true;
    }

    (void)d_cli_output_error(_program, &watch);

    return false;
}


//==============================================================================
// 2.  LIST AND DESCRIBE
//==============================================================================

/*
d_internal_djinterp_work_list
  The djins, then the frames no djin names.
*/
static int
d_internal_djinterp_work_list(
    struct dj_cli_program* _program,
    struct dj_arg_command* _command,
    void*                  _context
)
{
    struct dj_cli_watch       watch;
    const struct dj_pack_sink out = d_cli_program_watched(_program, &watch);

    (void)_command;
    (void)_context;

    if (d_djin_write_list(&d_internal_djinterp_set, out) != 0)
    {
        return d_cli_output_error(_program, &watch);
    }

    return D_CLI_EXIT_OK;
}

/*
d_internal_djinterp_json
  A description as JSON of the shape D_DJIN_DESC_SHAPE names, through a
stream over the watched output; 0, or what the output refused, said.
*/
static int
d_internal_djinterp_json(
    struct dj_cli_program* _program,
    fn_desc_describer      _describer,
    const void*            _object
)
{
    const struct dj_desc_machine* const machine = d_desc_json_machine();
    struct dj_cli_watch                 watch;
    struct dj_ostream                   os;
    struct dj_desc_json_state           state;
    struct dj_desc_sink                 sink;
    unsigned char                       room[D_INTERNAL_DJINTERP_ROOM];
    int32_t                             held;

    if (machine == NULL)
    {
        d_cli_report(_program, (int)D_CLI_SEVERITY_ERROR,
                     "this build has no JSON (D_CFG_DESC_MACHINES)");

        return D_CLI_EXIT_TROUBLE;
    }

    if ( (d_ostream_init(&os, d_cli_program_watched(_program, &watch),
                         NULL, 0) == 0) ||
         (d_ostream_set_buffer(&os, room, sizeof(room)) == 0) )
    {
        return d_cli_output_error(_program, &watch);
    }

    d_desc_json_init(&state, &os,
                     D_DESC_JSON_PRETTY | D_DESC_JSON_NEWLINE);
    d_desc_json_shape(&state, D_DJIN_DESC_SHAPE);
    d_desc_sink_init(&sink, machine, &state);
    (void)d_desc_run(&sink, _describer, _object);
    held = d_ostream_close(&os);

    return (held != 0) ? D_CLI_EXIT_OK
                       : d_cli_output_error(_program, &watch);
}

/*
d_internal_djinterp_format
  Whether describe writes JSON: --format's value, text by default; -1 for a
value it does not take, with the value.
*/
static int
d_internal_djinterp_format(
    struct dj_arg_command*      _command,
    const struct dj_pack_text** _value
)
{
    size_t                           count = 0u;
    const struct dj_pack_text* const value = d_arg_option_values(
        d_arg_command_option(_command, "--format"), &count);

    *_value = value;

    if (count == 0u)
    {
        return 0;
    }

    if ( (value->length == 4u) &&
         (memcmp(value->data, "json", 4u) == 0) )
    {
        return 1;
    }

    return ( (value->length == 4u) &&
             (memcmp(value->data, "text", 4u) == 0) ) ? 0 : -1;
}

/*
d_internal_djinterp_describe_command
  One of a djin's commands: its name and summary, as text or JSON.
*/
static int
d_internal_djinterp_describe_command(
    struct dj_cli_program*     _program,
    struct dj_arg_command*     _command,
    const struct dj_djin*      _djin,
    const struct dj_pack_text* _djin_name,
    const struct dj_pack_text* _name,
    int                        _json
)
{
    struct dj_djin_command_of of;
    char                      line[D_INTERNAL_DJINTERP_ROOM];
    int                       made;

    of.djin    = _djin;
    of.command = (_djin != NULL)
                 ? d_djin_command_find(_djin, _name->data, _name->length)
                 : NULL;

    if (of.command == NULL)
    {
        return d_cli_usage_error(_program, _command,
                                 "'%.*s' has no command '%.*s'",
                                 (int)_djin_name->length, _djin_name->data,
                                 (int)_name->length, _name->data);
    }

    if (_json != 0)
    {
        return d_internal_djinterp_json(_program, d_djin_command_describer,
                                        &of);
    }

    made = snprintf(line, sizeof(line), "%s %s\n  %s\n", _djin->name,
                    of.command->name,
                    (of.command->summary != NULL) ? of.command->summary : "");

    return ( (made > 0) &&
             ((size_t)made < sizeof(line)) &&
             (d_internal_djinterp_put(_program, line, (size_t)made)) )
           ? D_CLI_EXIT_OK
           : D_CLI_EXIT_TROUBLE;
}

/*
d_internal_djinterp_work_describe
  A djin's description, or, for a frame no djin names, its settings; one of
a djin's commands, given its name; as text, or as JSON at --format=json.
*/
static int
d_internal_djinterp_work_describe(
    struct dj_cli_program* _program,
    struct dj_arg_command* _command,
    void*                  _context
)
{
    size_t                      count  = 0u;
    const struct dj_pack_text*  name   = d_arg_option_values(
                                             d_arg_command_option(_command,
                                                                  "DJIN"),
                                             &count);
    size_t                      given  = 0u;
    const struct dj_pack_text*  which  = d_arg_option_values(
                                             d_arg_command_option(_command,
                                                                  "COMMAND"),
                                             &given);
    const struct dj_pack_text*  format = NULL;
    const int                   json   = d_internal_djinterp_format(_command,
                                                                    &format);
    const struct dj_djin*       djin;
    const struct dj_option_env* frame;
    struct dj_cli_watch         watch;
    const struct dj_pack_sink   out    = d_cli_program_watched(_program,
                                                               &watch);
    int                         wrote;

    (void)_context;

    if (json < 0)
    {
        return d_cli_usage_error(_program, _command,
                                 "'%.*s' is not a format: text or json",
                                 (int)format->length, format->data);
    }

    if (count == 0u)
    {
        return d_cli_usage_error(_program, _command, "DJIN is required");
    }

    djin  = d_djin_set_find(&d_internal_djinterp_set, name->data,
                            name->length);
    frame = (djin == NULL)
            ? d_option_env_registry_find(d_option_env_registry(),
                                         name->data, name->length)
            : NULL;

    if ( (djin == NULL) &&
         (frame == NULL) )
    {
        return d_cli_usage_error(_program, _command,
                                 "no djin is named '%.*s'",
                                 (int)name->length, name->data);
    }

    if (given != 0u)
    {
        return d_internal_djinterp_describe_command(_program, _command, djin,
                                                    name, which, json);
    }

    if (json != 0)
    {
        return (djin != NULL)
               ? d_internal_djinterp_json(_program, d_djin_describer, djin)
               : d_internal_djinterp_json(_program, d_djin_frame_describer,
                                          frame);
    }

    wrote = (djin != NULL)
            ? d_djin_write_description(djin, out)
            : d_djin_write_frame(frame, out);

    return (wrote == 0) ? D_CLI_EXIT_OK
                        : d_cli_output_error(_program, &watch);
}


//==============================================================================
// 3.  GET
//==============================================================================

// dj_internal_djinterp_setting
//   struct: a setting found: its frame, scope and variable, and its row.
struct dj_internal_djinterp_setting
{
    struct dj_option_env*           frame;
    uint32_t                        scope;
    uint32_t                        var;
    const struct dj_option_env_var* row;
};

/*
d_internal_djinterp_find
  A setting by its full name: in the frame the registry gives the name,
first as written, then, in a djin, after the djin's own name.
*/
static bool
d_internal_djinterp_find(
    const char*                          _name,
    size_t                               _size,
    struct dj_internal_djinterp_setting* _found
)
{
    struct dj_option_env_registry* const registry = d_option_env_registry();
    struct dj_option_env* const          frame    =
        d_option_env_registry_target(registry, _name, _size);
    const char* const                    dot      =
        (const char*)memchr(_name, '.', _size);
    struct dj_option_result              found;

    if (frame == NULL)
    {
        return false;
    }

    found = d_option_env_resolve(frame, _name, _size, &_found->scope);

    if ( (!found.is_ok) &&
         (dot != NULL) &&
         (frame != d_option_env_registry_frame(registry)) )
    {
        found = d_option_env_resolve(frame, dot + 1,
                                     _size - (size_t)(dot + 1 - _name),
                                     &_found->scope);
    }

    _found->frame = frame;
    _found->var   = (found.is_ok) ? (uint32_t)found.payload.value : 0u;
    _found->row   = (found.is_ok) ? d_option_env_var_at(frame, _found->var)
                                  : NULL;

    return (_found->row != NULL);
}

/*
d_internal_djinterp_source_word
  Where a value came from, in a word or two.
*/
static const char*
d_internal_djinterp_source_word(
    int32_t _source
)
{
    switch (_source)
    {
        case D_OPTION_ENV_SOURCE_PROGRAM:      return "program";
        case D_OPTION_ENV_SOURCE_FILE:         return "file";
        case D_OPTION_ENV_SOURCE_ENVIRONMENT:  return "environment";
        case D_OPTION_ENV_SOURCE_COMMAND_LINE: return "command line";
        case D_OPTION_ENV_SOURCE_OVERRIDE:     return "override";
        default:                               return "default";
    }
}

/*
d_internal_djinterp_value_text
  A value as a statement writes it: a bool as true or false, a real to the
last bit, a text in double quotes, a choice or a named integer by its name,
any other integer as itself; "unset" for a null text.
*/
static void
d_internal_djinterp_value_text(
    const struct dj_option_env_var* _row,
    const char*                     _text,
    int64_t                         _integer,
    double                          _real,
    char*                           _buffer,
    size_t                          _capacity
)
{
    const char* named = NULL;

    for (uint32_t i = 0u; (_row->names) && (i < _row->name_count); ++i)
    {
        named = (_row->names[i].value == _integer) ? _row->names[i].name
                                                    : named;
    }

    switch (_row->kind)
    {
        case D_OPTION_ENV_KIND_BOOL:
            (void)snprintf(_buffer, _capacity, "%s",
                           (_integer != 0) ? "true" : "false");
            break;
        case D_OPTION_ENV_KIND_REAL:
            (void)snprintf(_buffer, _capacity, "%.17g", _real);
            break;
        case D_OPTION_ENV_KIND_TEXT:
            if (_text != NULL)
            {
                (void)snprintf(_buffer, _capacity, "\"%s\"", _text);
            }
            else
            {
                (void)snprintf(_buffer, _capacity, "unset");
            }
            break;
        default:
            if (named != NULL)
            {
                (void)snprintf(_buffer, _capacity, "%s", named);
            }
            else
            {
                (void)snprintf(_buffer, _capacity, "%lld",
                               (long long)_integer);
            }
            break;
    }

    return;
}

/*
d_internal_djinterp_origins
  Under a setting's line, what lies under its value: each binding the one
in force stands over, and the default, with the knob it came from. Where the
frame's layers end with the default, that layer gives the default's line;
where they do not, the row gives it, so the lines are the same either way.
*/
static bool
d_internal_djinterp_origins(
    const struct dj_cli_program*               _program,
    const struct dj_internal_djinterp_setting* _setting
)
{
    struct dj_option_env_layer        layers[D_INTERNAL_DJINTERP_LAYERS];
    const struct dj_option_env_value* bottom = NULL;
    const size_t                      count  = d_option_env_layers(
                                                   _setting->frame,
                                                   _setting->scope,
                                                   _setting->var, layers,
                                                   D_INTERNAL_DJINTERP_LAYERS);
    const size_t                      shown  =
        (count == (size_t)-1)               ? 0u
      : (count < D_INTERNAL_DJINTERP_LAYERS) ? count
                                             : D_INTERNAL_DJINTERP_LAYERS;
    char                              value[D_INTERNAL_DJINTERP_ROOM];
    char                              line[D_INTERNAL_DJINTERP_ROOM * 2u];
    bool                              held   = true;
    int                               size;

    for (size_t i = 1u; (held) && (i < shown); ++i)
    {
        // the default's own layer is the last line, not one under it
        if (!layers[i].value.bound)
        {
            bottom = &layers[i].value;

            continue;
        }

        d_internal_djinterp_value_text(_setting->row,
                                       layers[i].value.text,
                                       layers[i].value.integer,
                                       layers[i].value.real,
                                       value, sizeof(value));
        size = snprintf(line, sizeof(line), "    under it   %s   (%s)\n",
                        value,
                        d_internal_djinterp_source_word(
                            layers[i].value.source));
        held = d_internal_djinterp_put(_program, line, (size_t)size);
    }

    if ( (held) &&
         (shown > 0u) )
    {
        d_internal_djinterp_value_text(
            _setting->row,
            (bottom != NULL) ? bottom->text    : _setting->row->text,
            (bottom != NULL) ? bottom->integer : _setting->row->integer,
            (bottom != NULL) ? bottom->real    : _setting->row->real,
            value, sizeof(value));
        size = snprintf(line, sizeof(line), "    default    %s   (%s)\n",
                        value,
                        (_setting->row->knob) ? _setting->row->knob
                                              : "default");
        held = d_internal_djinterp_put(_program, line, (size_t)size);
    }

    return held;
}

/*
d_internal_djinterp_get_one
  One setting's statement - a comment for a text that is unset, which
loads as nothing - and, at `--show-origin`, its source and what lies under
it.
*/
static bool
d_internal_djinterp_get_one(
    const struct dj_cli_program*               _program,
    const char*                                _name,
    size_t                                     _size,
    const struct dj_internal_djinterp_setting* _setting,
    bool                                       _origin
)
{
    struct dj_option_env_value in_force;
    char                       value[D_INTERNAL_DJINTERP_ROOM];
    char                       line[D_INTERNAL_DJINTERP_ROOM * 2u];
    const bool                 unset = ( (_setting->row->kind ==
                                          D_OPTION_ENV_KIND_TEXT) &&
                                         (d_option_env_text(
                                              _setting->frame,
                                              _setting->scope,
                                              _setting->var) == NULL) );
    int                        size;

    (void)d_option_env_format_quoted(_setting->frame, _setting->scope,
                                     _setting->var, value, sizeof(value));
    (void)d_option_env_get(_setting->frame, _setting->scope, _setting->var,
                           &in_force);

    size = (unset)
           ? snprintf(line, sizeof(line), "# %.*s is unset", (int)_size,
                      _name)
           : snprintf(line, sizeof(line), "%.*s = %s", (int)_size, _name,
                      value);
    size = (size > (int)sizeof(line) - 1) ? (int)sizeof(line) - 1 : size;

    if (_origin)
    {
        size += snprintf(line + size, sizeof(line) - (size_t)size,
                         "  (%s)",
                         (in_force.bound)
                             ? d_internal_djinterp_source_word(
                                   in_force.source)
                             : (_setting->row->knob) ? _setting->row->knob
                                                     : "default");
        size  = (size > (int)sizeof(line) - 2) ? (int)sizeof(line) - 2
                                               : size;
    }

    line[size++] = '\n';

    return ( (d_internal_djinterp_put(_program, line, (size_t)size)) &&
             ( (!_origin) ||
               (!in_force.bound) ||
               (d_internal_djinterp_origins(_program, _setting)) ) );
}

/*
d_internal_djinterp_get_frame
  Every setting a frame declares itself, named in full: the root's by their
names, a djin's after its own.
*/
static bool
d_internal_djinterp_get_frame(
    const struct dj_cli_program* _program,
    struct dj_option_env*        _frame,
    bool                         _root,
    bool                         _origin
)
{
    char                                name[D_INTERNAL_DJINTERP_ROOM];
    struct dj_internal_djinterp_setting setting;
    bool                                held = true;

    for (uint32_t k = 0u; (held) && (k < _frame->var_count); ++k)
    {
        const struct dj_option_env_var* const row = _frame->vars[k];
        const int size = (_root)
                         ? snprintf(name, sizeof(name), "%s", row->name)
                         : snprintf(name, sizeof(name), "%s.%s",
                                    _frame->name, row->name);

        if ( (size > 0) &&
             ((size_t)size < sizeof(name)) &&
             (d_internal_djinterp_find(name, (size_t)size, &setting)) )
        {
            held = d_internal_djinterp_get_one(_program, name,
                                               (size_t)size, &setting,
                                               _origin);
        }
    }

    return held;
}

/*
d_internal_djinterp_work_get
  The settings named, or every one: the root's, then each djin's.
*/
static int
d_internal_djinterp_work_get(
    struct dj_cli_program* _program,
    struct dj_arg_command* _command,
    void*                  _context
)
{
    struct dj_option_env_registry* const registry = d_option_env_registry();
    const bool                           origin   =
        (d_arg_option_count(d_arg_command_option(_command,
                                                 "--show-origin")) > 0u);
    size_t                               count    = 0u;
    const struct dj_pack_text* const     names    = d_arg_option_values(
        d_arg_command_option(_command, "NAME"), &count);
    struct dj_internal_djinterp_setting  setting;
    char                                 why[D_INTERNAL_DJINTERP_ROOM];
    bool                                 held     = true;
    int                                  status   = D_CLI_EXIT_OK;

    (void)_context;

    if (count == 0u)
    {
        held = d_internal_djinterp_get_frame(
                   _program, d_option_env_registry_frame(registry), true,
                   origin);

        for (uint32_t i = 0u;
             (held) && (i < d_option_env_registry_count(registry));
             ++i)
        {
            held = d_internal_djinterp_get_frame(
                       _program, d_option_env_registry_at(registry, i),
                       false, origin);
        }
    }

    for (size_t i = 0u; (held) && (i < count); ++i)
    {
        if (d_internal_djinterp_find(names[i].data, names[i].length,
                                     &setting))
        {
            held = d_internal_djinterp_get_one(_program, names[i].data,
                                               names[i].length, &setting,
                                               origin);
            continue;
        }

        d_cli_settings_why((int32_t)D_OPTION_STATUS_KEY_ABSENT,
                           names[i].data, names[i].length, NULL,
                           why, sizeof(why));
        d_cli_report(_program, (int)D_CLI_SEVERITY_ERROR, "%s", why);
        status = D_CLI_EXIT_TROUBLE;
    }

    return (held) ? status : D_CLI_EXIT_TROUBLE;
}


//==============================================================================
// 4.  SET
//==============================================================================

// D_INTERNAL_DJINTERP_LINE_*
//   constants: what a line of the user's file holds, to `set`: nothing but
// blanks; a comment alone; a statement, or more than one; a section's
// header, as the kit finds one (cli_settings.h, 1.4).
#define D_INTERNAL_DJINTERP_LINE_BLANK     0
#define D_INTERNAL_DJINTERP_LINE_COMMENT   1
#define D_INTERNAL_DJINTERP_LINE_STATEMENT 2
#define D_INTERNAL_DJINTERP_LINE_HEADER    3

// D_INTERNAL_DJINTERP_IN_*
//   constants: whose a line is, by the header above it: every program's,
// before the first header; every program's again, after `[*]`; one
// program's, after `[NAME]`; no program's, after a header the kit cannot
// read. A file read without sections is every program's, whole.
#define D_INTERNAL_DJINTERP_IN_LEADING 0
#define D_INTERNAL_DJINTERP_IN_EVERY   1
#define D_INTERNAL_DJINTERP_IN_SECTION 2
#define D_INTERNAL_DJINTERP_IN_NOBODY  3

// dj_internal_djinterp_place
//   struct: where `set` writes its statements: the section they are for, or
// every program's lines; the line of the old text they go before, or its
// end; and whether a header is to be written there first.
struct dj_internal_djinterp_place
{
    const char* section;    // the program's name, NUL-ended; NULL for
                            // every program's lines
    bool        sectioned;  // whether the file's headers are read
    size_t      at;         // a line's start in the old text, or its end
    bool        header;     // whether `[SECTION]` goes there first
};

// dj_internal_djinterp_lines
//   struct: a walk over the user's file, a line at a time: each line's kind,
// and whose it is.
struct dj_internal_djinterp_lines
{
    const char*                   text;       // the old text, NUL-ended
    size_t                        length;     // its length
    bool                          sectioned;  // whether headers are read
    bool                          found;      // whether `next` is a header
    struct dj_cli_settings_header next;       // the next header
    size_t                        at;         // the next line's start
    size_t                        start;      // the line's start
    size_t                        size;       // its length, '\n' aside
    long                          number;     // its number, from 1
    int                           kind;       // D_INTERNAL_DJINTERP_LINE_*
    int                           region;     // D_INTERNAL_DJINTERP_IN_*
    const char*                   name;       // its section's name
    size_t                        name_size;  // that name's length
};

/*
d_internal_djinterp_bind
  Each NAME=VALUE bound for the run, above every other source, each
refusal said; whether all were bound.
*/
static bool
d_internal_djinterp_bind(
    struct dj_cli_program*       _program,
    struct dj_arg_command*       _command,
    const struct dj_pack_text*   _items,
    size_t                       _count
)
{
    char why[D_INTERNAL_DJINTERP_ROOM];
    char value[D_INTERNAL_DJINTERP_ROOM];
    bool held = true;

    for (size_t i = 0u; i < _count; ++i)
    {
        const char* const equals = (const char*)memchr(_items[i].data, '=',
                                                       _items[i].length);
        const size_t      name   = (equals != NULL)
                                   ? (size_t)(equals - _items[i].data)
                                   : 0u;
        const size_t      size   = (equals != NULL)
                                   ? (_items[i].length - name - 1u)
                                   : 0u;
        struct dj_option_result bound;

        if ( (equals == NULL) ||
             (name == 0u) )
        {
            (void)d_cli_usage_error(_program, _command,
                                    "'%.*s' is not NAME=VALUE",
                                    (int)_items[i].length, _items[i].data);
            held = false;

            continue;
        }

        // checked as the file will bind it, then bound for the run
        bound = d_option_ok(0);

        if (d_option_env_registry_check_from(d_option_env_registry(),
                                             _items[i].data, name,
                                             equals + 1, size,
                                             D_OPTION_ENV_SOURCE_FILE, NULL,
                                             0u, &bound, NULL) != (size_t)-1)
        {
            bound = d_option_env_registry_assign(d_option_env_registry(),
                                                 _items[i].data, name,
                                                 equals + 1, size,
                                                 D_OPTION_ENV_SOURCE_OVERRIDE);
        }

        if (!bound.is_ok)
        {
            (void)snprintf(value, sizeof(value), "%.*s", (int)size,
                           equals + 1);
            d_cli_settings_why(d_option_result_status(&bound),
                               _items[i].data, name, value,
                               why, sizeof(why));
            d_cli_report(_program, (int)D_CLI_SEVERITY_ERROR, "%s", why);
            held = false;
        }
    }

    return held;
}

/*
d_internal_djinterp_lines_begin
  A walk from the old text's first line.
*/
static void
d_internal_djinterp_lines_begin(
    struct dj_internal_djinterp_lines* _lines,
    const char*                        _text,
    size_t                             _length,
    bool                               _sectioned
)
{
    memset(_lines, 0, sizeof(*_lines));
    _lines->text      = (_text != NULL) ? _text : "";
    _lines->length    = (_text != NULL) ? _length : 0u;
    _lines->sectioned = _sectioned;
    _lines->region    = D_INTERNAL_DJINTERP_IN_LEADING;
    _lines->found     = (_sectioned) &&
                        (d_cli_settings_next_header(_lines->text, 0u,
                                                    &_lines->next));

    return;
}

/*
d_internal_djinterp_lines_next
  The walk on to the next line: its kind, and whose it is; a header's line
the first of its section's. False past the last.
*/
static bool
d_internal_djinterp_lines_next(
    struct dj_internal_djinterp_lines* _lines
)
{
    const char* const text = _lines->text;
    const char*       end;
    size_t            at;

    if (_lines->at >= _lines->length)
    {
        return false;
    }

    end            = (const char*)memchr(text + _lines->at, '\n',
                                         _lines->length - _lines->at);
    _lines->start  = _lines->at;
    _lines->size   = (end != NULL) ? (size_t)(end - text) - _lines->at
                                   : _lines->length - _lines->at;
    _lines->at    += _lines->size + 1u;
    _lines->number += 1;

    if ( (_lines->found) &&
         (_lines->next.at <= _lines->start + _lines->size) )
    {
        _lines->kind      = D_INTERNAL_DJINTERP_LINE_HEADER;
        _lines->name      = _lines->next.name;
        _lines->name_size = _lines->next.size;
        _lines->region    = (_lines->next.size == 0u)
                            ? D_INTERNAL_DJINTERP_IN_NOBODY
                            : ( (_lines->next.size == 1u) &&
                                (_lines->next.name[0] == '*') )
                              ? D_INTERNAL_DJINTERP_IN_EVERY
                              : D_INTERNAL_DJINTERP_IN_SECTION;
        _lines->found     = d_cli_settings_next_header(
                                text, _lines->next.end, &_lines->next);

        return true;
    }

    for (at = _lines->start;
         ( (at < _lines->start + _lines->size) &&
           ( (text[at] == ' ') || (text[at] == '\t') ||
             (text[at] == '\r') ) );
         ++at)
    {
    }

    _lines->kind = (at == _lines->start + _lines->size)
                   ? D_INTERNAL_DJINTERP_LINE_BLANK
                   : (text[at] == '#')
                     ? D_INTERNAL_DJINTERP_LINE_COMMENT
                     : D_INTERNAL_DJINTERP_LINE_STATEMENT;

    return true;
}

/*
d_internal_djinterp_ours
  Whether the walk's line is among those the statements are written for:
every program's, or the section's, as the place says.
*/
static bool
d_internal_djinterp_ours(
    const struct dj_internal_djinterp_lines* _lines,
    const struct dj_internal_djinterp_place* _place
)
{
    if (_place->section == NULL)
    {
        return ( (_lines->region == D_INTERNAL_DJINTERP_IN_LEADING) ||
                 (_lines->region == D_INTERNAL_DJINTERP_IN_EVERY) );
    }

    return ( (_lines->region == D_INTERNAL_DJINTERP_IN_SECTION) &&
             (d_cli_settings_section_reads(_lines->name, _lines->name_size,
                                           _place->section)) );
}

/*
d_internal_djinterp_place
  Where the statements go: in every program's lines before the first
header, or in the section's last, after its last statement; with none,
after the comments it begins with, else at its start. A section the file
does not have yet goes at the file's end, under a new header.
*/
static void
d_internal_djinterp_place(
    const char*                        _old,
    size_t                             _length,
    struct dj_internal_djinterp_place* _place
)
{
    struct dj_internal_djinterp_lines walk;
    const size_t                      none    = (size_t)-1;
    bool                              in      = (_place->section == NULL);
    bool                              any     = in;
    bool                              heading = true;
    size_t                            start   = 0u;
    size_t                            stated  = none;
    size_t                            comment = none;

    d_internal_djinterp_lines_begin(&walk, _old, _length,
                                    _place->sectioned);

    while (d_internal_djinterp_lines_next(&walk))
    {
        const size_t after = (walk.at < _length) ? walk.at : _length;

        if (walk.kind == D_INTERNAL_DJINTERP_LINE_HEADER)
        {
            // every program's first lines end at the first header
            in = (_place->section != NULL) &&
                 (d_internal_djinterp_ours(&walk, _place));

            if (in)
            {
                any     = true;
                heading = true;
                start   = after;
                stated  = none;
                comment = none;
            }

            continue;
        }

        if (!in)
        {
            continue;
        }

        stated  = (walk.kind == D_INTERNAL_DJINTERP_LINE_STATEMENT)
                  ? after : stated;
        heading = (heading) &&
                  (walk.kind == D_INTERNAL_DJINTERP_LINE_COMMENT);
        comment = (heading) ? after : comment;
    }

    _place->header = !any;
    _place->at     = (!any)            ? _length
                   : (stated != none)  ? stated
                   : (comment != none) ? comment
                                       : start;

    return;
}

/*
d_internal_djinterp_sets
  Whether a line of the file begins with a statement that sets `_name`: its
name, then '=' after blanks; `_alone`, one that holds nothing else, no ';'
or '#' after it.
*/
static bool
d_internal_djinterp_sets(
    const char* _line,
    size_t      _length,
    const char* _name,
    size_t      _size,
    bool        _alone
)
{
    size_t at = 0u;

    while ( (at < _length) &&
            ( (_line[at] == ' ') ||
              (_line[at] == '\t') ) )
    {
        ++at;
    }

    if ( (_length - at < _size) ||
         (memcmp(_line + at, _name, _size) != 0) )
    {
        return false;
    }

    at += _size;

    while ( (at < _length) &&
            ( (_line[at] == ' ') ||
              (_line[at] == '\t') ) )
    {
        ++at;
    }

    return ( (at < _length) &&
             (_line[at] == '=') &&
             ( (!_alone) ||
               ( (memchr(_line + at, ';', _length - at) == NULL) &&
                 (memchr(_line + at, '#', _length - at) == NULL) ) ) );
}

/*
d_internal_djinterp_canonical
  A setting's canonical full name, as the file is to hold it: its row's name,
after its djin's where it is a djin's; how long it is, or 0 when it does not
fit.
*/
static size_t
d_internal_djinterp_canonical(
    const struct dj_internal_djinterp_setting* _setting,
    char*                                      _name,
    size_t                                     _capacity
)
{
    const bool root = (_setting->frame ==
                       d_option_env_registry_frame(d_option_env_registry()));
    const int  made = (root)
                      ? snprintf(_name, _capacity, "%s", _setting->row->name)
                      : snprintf(_name, _capacity, "%s.%s",
                                 _setting->frame->name,
                                 _setting->row->name);

    return ( (made > 0) &&
             ((size_t)made < _capacity) ) ? (size_t)made : 0u;
}

/*
d_internal_djinterp_named
  Which statement's setting a line of the file sets, as `_alone` asks
(d_internal_djinterp_sets): the name as given or as canonical; `_count`
for none.
*/
static size_t
d_internal_djinterp_named(
    const char*                _line,
    size_t                     _length,
    const struct dj_pack_text* _items,
    size_t                     _count,
    bool                       _alone
)
{
    struct dj_internal_djinterp_setting setting;
    char                                name[D_INTERNAL_DJINTERP_ROOM];

    for (size_t i = 0u; i < _count; ++i)
    {
        const size_t given = (size_t)((const char*)memchr(
                                 _items[i].data, '=', _items[i].length) -
                                 _items[i].data);
        const size_t size  = (d_internal_djinterp_find(_items[i].data, given,
                                                       &setting))
                             ? d_internal_djinterp_canonical(&setting, name,
                                                             sizeof(name))
                             : 0u;

        if ( (d_internal_djinterp_sets(_line, _length, _items[i].data, given,
                                       _alone)) ||
             ( (size != 0u) &&
               (d_internal_djinterp_sets(_line, _length, name, size,
                                         _alone)) ) )
        {
            return i;
        }
    }

    return _count;
}

/*
d_internal_djinterp_drops
  Whether the walk's line is one a statement replaces: among the lines the
statements are for, one that sets, alone, a setting given.
*/
static bool
d_internal_djinterp_drops(
    const struct dj_internal_djinterp_lines* _lines,
    const struct dj_internal_djinterp_place* _place,
    const struct dj_pack_text*               _items,
    size_t                                   _count
)
{
    return ( (_lines->kind == D_INTERNAL_DJINTERP_LINE_STATEMENT) &&
             (d_internal_djinterp_ours(_lines, _place)) &&
             (d_internal_djinterp_named(_lines->text + _lines->start,
                                        _lines->size, _items, _count,
                                        true) < _count) );
}

/*
d_internal_djinterp_statements
  The statements written at `_used` in `_text`, after the section's header
where the place says, under each setting's canonical name, as get writes
it; how long the text is then, or 0 when it does not fit.
*/
static size_t
d_internal_djinterp_statements(
    const struct dj_internal_djinterp_place* _place,
    const struct dj_pack_text*               _items,
    size_t                                   _count,
    char*                                    _text,
    size_t                                   _used,
    size_t                                   _capacity
)
{
    struct dj_internal_djinterp_setting setting;
    char                                value[D_INTERNAL_DJINTERP_ROOM];
    size_t                              used = _used;
    bool                                fits = true;
    int                                 wrote;

    if (_place->header)
    {
        // a blank line between the old text and the new section
        const bool apart = (used == 0u) ||
                           ( (_text[used - 1u] == '\n') &&
                             ( (used == 1u) ||
                               (_text[used - 2u] == '\n') ) );

        wrote = snprintf(_text + used, _capacity - used, "%s[%s]\n",
                         (apart) ? "" : "\n", _place->section);
        fits  = (wrote >= 0) && ((size_t)wrote < _capacity - used);
        used += (fits) ? (size_t)wrote : 0u;
    }

    for (size_t i = 0u; (fits) && (i < _count); ++i)
    {
        const size_t name = (size_t)((const char*)memchr(
                                _items[i].data, '=', _items[i].length) -
                                _items[i].data);
        char         canonical[D_INTERNAL_DJINTERP_ROOM];
        size_t       size;

        (void)d_internal_djinterp_find(_items[i].data, name, &setting);
        size  = d_internal_djinterp_canonical(&setting, canonical,
                                              sizeof(canonical));
        (void)d_option_env_format_quoted(setting.frame, setting.scope,
                                         setting.var, value, sizeof(value));
        wrote = (size != 0u)
                ? snprintf(_text + used, _capacity - used, "%s = %s\n",
                           canonical, value)
                : snprintf(_text + used, _capacity - used, "%.*s = %s\n",
                           (int)name, _items[i].data, value);
        fits  = (wrote >= 0) && ((size_t)wrote < _capacity - used);
        used += (fits) ? (size_t)wrote : 0u;
    }

    return (fits) ? used : 0u;
}

/*
d_internal_djinterp_compose
  The file's new text into `_text`: each old line kept but one a statement
replaces, and the statements where the place says; how long it is, or 0
when it does not fit.
*/
static size_t
d_internal_djinterp_compose(
    const char*                              _old,
    size_t                                   _old_length,
    const struct dj_internal_djinterp_place* _place,
    const struct dj_pack_text*               _items,
    size_t                                   _count,
    char*                                    _text,
    size_t                                   _capacity
)
{
    struct dj_internal_djinterp_lines walk;
    size_t                            used    = 0u;
    bool                              written = false;

    d_internal_djinterp_lines_begin(&walk, _old, _old_length,
                                    _place->sectioned);

    while (d_internal_djinterp_lines_next(&walk))
    {
        if ( (!written) &&
             (walk.start >= _place->at) )
        {
            used    = d_internal_djinterp_statements(_place, _items, _count,
                                                     _text, used, _capacity);
            written = true;

            if (used == 0u)
            {
                return 0u;
            }
        }

        if (d_internal_djinterp_drops(&walk, _place, _items, _count))
        {
            continue;
        }

        if (used + walk.size + 1u >= _capacity)
        {
            return 0u;
        }

        memcpy(_text + used, _old + walk.start, walk.size);
        used         += walk.size;
        _text[used++] = '\n';
    }

    return (written) ? used
                     : d_internal_djinterp_statements(_place, _items, _count,
                                                      _text, used,
                                                      _capacity);
}

/*
d_internal_djinterp_after
  Each line kept after the new statements that sets one of their settings
for a program that reads them, said as a warning by its line in the new
text: that program reads it last, so its value stands there.
*/
static void
d_internal_djinterp_after(
    const struct dj_cli_program*             _program,
    const char*                              _path,
    const char*                              _old,
    size_t                                   _old_length,
    const struct dj_internal_djinterp_place* _place,
    const struct dj_pack_text*               _items,
    size_t                                   _count
)
{
    struct dj_internal_djinterp_lines walk;
    char                              whose[D_INTERNAL_DJINTERP_ROOM];
    long                              dropped = 0;

    d_internal_djinterp_lines_begin(&walk, _old, _old_length,
                                    _place->sectioned);

    while (d_internal_djinterp_lines_next(&walk))
    {
        const bool section = (walk.region == D_INTERNAL_DJINTERP_IN_SECTION);
        const bool every   = (walk.region == D_INTERNAL_DJINTERP_IN_LEADING) ||
                             (walk.region == D_INTERNAL_DJINTERP_IN_EVERY);
        const bool reads   = (every) ||
                             ( (section) &&
                               ( (_place->section == NULL) ||
                                 (d_internal_djinterp_ours(&walk,
                                                           _place)) ) );
        size_t     which;
        size_t     name;

        if (d_internal_djinterp_drops(&walk, _place, _items, _count))
        {
            ++dropped;

            continue;
        }

        if ( (walk.kind != D_INTERNAL_DJINTERP_LINE_STATEMENT) ||
             (walk.start < _place->at) ||
             (!reads) )
        {
            continue;
        }

        which = d_internal_djinterp_named(_old + walk.start, walk.size,
                                          _items, _count, false);

        if (which == _count)
        {
            continue;
        }

        name = (size_t)((const char*)memchr(_items[which].data, '=',
                                            _items[which].length) -
                        _items[which].data);
        (void)( (section)
                ? snprintf(whose, sizeof(whose), "[%.*s]",
                           (int)walk.name_size, walk.name)
                : snprintf(whose, sizeof(whose), "every program") );
        // its number in the new text: the statements before it, a line each
        d_cli_report_at(_program, (int)D_CLI_SEVERITY_WARNING, _path,
                        walk.number - dropped + (long)_count, 0,
                        "this line sets '%.*s' too, and %s "
                        "reads it after the new one", (int)name,
                        _items[which].data, whose);
    }

    return;
}

/*
d_internal_djinterp_section
  `--section`'s program into `_name`, and into the place; `*`, or none, for
every program's lines. The file's headers are its structure, kept whether
or not this run reads them, so that a run that reads the file as one part
never takes another program's lines for its own. D_CLI_EXIT_OK; a usage
error, said, for a name no header can hold.
*/
static int
d_internal_djinterp_section(
    struct dj_cli_program*             _program,
    struct dj_arg_command*             _command,
    struct dj_internal_djinterp_place* _place,
    char*                              _name,
    size_t                             _capacity
)
{
    size_t                           count = 0u;
    const struct dj_pack_text* const value = d_arg_option_values(
        d_arg_command_option(_command, "--section"), &count);
    struct dj_cli_settings_header    header;
    char                             line[D_INTERNAL_DJINTERP_ROOM];
    const int                        made  = (count != 0u)
        ? snprintf(line, sizeof(line), "[%.*s]", (int)value->length,
                   value->data)
        : 0;

    memset(_place, 0, sizeof(*_place));
    _place->sectioned = true;

    if ( (count == 0u) ||
         ( (value->length == 1u) &&
           (value->data[0] == '*') ) )
    {
        return D_CLI_EXIT_OK;
    }

    if ( (value->length == 0u) ||
         (made <= 0) ||
         ((size_t)made >= sizeof(line)) ||
         (value->length >= _capacity) ||
         (!d_cli_settings_next_header(line, 0u, &header)) ||
         (header.size != value->length) )
    {
        return d_cli_usage_error(_program, _command,
                                 "--section: '%.*s' is not a program's "
                                 "name: letters, digits, '_', '-' and '.'; "
                                 "or *", (int)value->length, value->data);
    }

    memcpy(_name, value->data, value->length);
    _name[value->length] = '\0';
    _place->section      = _name;

    return D_CLI_EXIT_OK;
}

/*
d_internal_djinterp_folder
  The folder the user's file is in, made where it is not there yet.
*/
static int
d_internal_djinterp_folder(
    const char* _path
)
{
    char   folder[D_INTERNAL_DJINTERP_ROOM];
    size_t size = strlen(_path);

    while ( (size > 0u) &&
            (_path[size - 1u] != '/') &&
            (_path[size - 1u] != '\\') )
    {
        --size;
    }

    if ( (size <= 1u) ||
         (size >= sizeof(folder)) )
    {
        return 0;
    }

    memcpy(folder, _path, size - 1u);
    folder[size - 1u] = '\0';

    return (d_dir_create_parents(folder, 0755u) == 0) ? 0 : errno;
}

/*
d_internal_djinterp_work_set
  Bound for the run first, every one, then the file written whole: in
every program's lines, or in `--section`'s.
*/
static int
d_internal_djinterp_work_set(
    struct dj_cli_program* _program,
    struct dj_arg_command* _command,
    void*                  _context
)
{
    size_t                            count  = 0u;
    const struct dj_pack_text* const  items  = d_arg_option_values(
        d_arg_command_option(_command, "SETTING"), &count);
    struct dj_internal_djinterp_place place;
    char                              section[D_INTERNAL_DJINTERP_ROOM];
    char                              path[D_INTERNAL_DJINTERP_ROOM];
    char*                             old    = NULL;
    size_t                            length = 0u;
    char*                             text;
    size_t                            room;
    size_t                            made;
    int                               failed;

    (void)_context;

    failed = d_internal_djinterp_section(_program, _command, &place, section,
                                         sizeof(section));

    if (failed != D_CLI_EXIT_OK)
    {
        return failed;
    }

    if (!d_internal_djinterp_bind(_program, _command, items, count))
    {
        return D_CLI_EXIT_TROUBLE;
    }

    made = d_cli_settings_file(path, sizeof(path));

    if ( (made == 0u) ||
         (made >= sizeof(path)) )
    {
        d_cli_report(_program, (int)D_CLI_SEVERITY_ERROR,
                     "the system names no home for the settings file");

        return D_CLI_EXIT_TROUBLE;
    }

    failed = d_cli_read_file(path, &old, &length);

    if ( (failed != 0) &&
         (failed != ENOENT) &&
         (failed != ENOTDIR) )
    {
        return d_cli_file_error(_program, "read", path, failed);
    }

    // the old text, a header's line, and a line's room for each statement
    room = ( (count < ((size_t)-1 / D_INTERNAL_DJINTERP_ROOM) - 2u) &&
             (length < (size_t)-1 - (count + 2u) * D_INTERNAL_DJINTERP_ROOM) )
           ? length + (count + 2u) * D_INTERNAL_DJINTERP_ROOM
           : 0u;
    text = (room != 0u) ? (char*)D_INTERNAL_CLI_MALLOC(room) : NULL;
    d_internal_djinterp_place(old, length, &place);
    made = (text != NULL)
           ? d_internal_djinterp_compose(old, length, &place, items, count,
                                         text, room)
           : 0u;

    failed = (text == NULL)   ? ENOMEM
           : (made == 0u)     ? ENOMEM
                              : d_internal_djinterp_folder(path);
    failed = (failed != 0) ? failed
                           : d_cli_write_file(path, text, made);
    D_INTERNAL_CLI_FREE(text);

    if (failed == 0)
    {
        d_internal_djinterp_after(_program, path, old, length, &place, items,
                                  count);
    }

    d_cli_free(old);

    return (failed == 0) ? D_CLI_EXIT_OK
                         : d_cli_file_error(_program, "write", path, failed);
}


//==============================================================================
// 5.  THE PROGRAM
//==============================================================================

/*
d_internal_djinterp_commands
  The four commands, their operands and options.
*/
static bool
d_internal_djinterp_commands(
    struct dj_cli_program* _program
)
{
    struct dj_arg_command* const list     = d_cli_program_add_command(
        _program, NULL, "list", "list the djins, a line each",
        d_internal_djinterp_work_list, NULL, NULL);
    struct dj_arg_command* const describe = d_cli_program_add_command(
        _program, NULL, "describe", "describe a djin: its commands and "
        "settings", d_internal_djinterp_work_describe, NULL, NULL);
    struct dj_arg_command* const get      = d_cli_program_add_command(
        _program, NULL, "get", "print settings as statements that load "
        "back; every one, without a NAME", d_internal_djinterp_work_get,
        NULL, NULL);
    struct dj_arg_command* const set      = d_cli_program_add_command(
        _program, NULL, "set", "write settings to the user's settings file",
        d_internal_djinterp_work_set, NULL, NULL);
    struct dj_arg_option*        djin     = (describe != NULL)
        ? d_arg_command_add_option(describe, "DJIN", "the djin to describe")
        : NULL;
    struct dj_arg_option*        which    = (describe != NULL)
        ? d_arg_command_add_option(describe, "COMMAND",
                                   "one of the djin's commands, to describe "
                                   "it alone")
        : NULL;
    struct dj_arg_option*        format   = (describe != NULL)
        ? d_arg_command_add_option(describe, "--format",
                                   "text, or json: an object of the shape "
                                   D_DJIN_DESC_SHAPE)
        : NULL;
    struct dj_arg_option*        names    = (get != NULL)
        ? d_arg_command_add_option(get, "NAME", "a setting's full name")
        : NULL;
    struct dj_arg_option*        items    = (set != NULL)
        ? d_arg_command_add_option(set, "SETTING",
                                   "NAME=VALUE: a setting's full name, and "
                                   "its value")
        : NULL;
    struct dj_arg_option*        section  = (set != NULL)
        ? d_arg_command_add_option(set, "--section",
                                   "the program whose section of the file "
                                   "the settings are written in; * for "
                                   "every program's lines, as without it")
        : NULL;

    if ( (list == NULL) || (djin == NULL) || (which == NULL) ||
         (format == NULL) || (names == NULL) || (items == NULL) ||
         (section == NULL) ||
         (d_arg_command_add_flag(get, "--show-origin",
                                 "say where each value came from, and "
                                 "what lies under it") == NULL) )
    {
        return false;
    }

    d_arg_option_set_required(djin, true);
    d_arg_option_set_expected_range(which, 0, 1);
    (void)d_arg_option_set_type_name(format, "FORMAT");
    (void)d_arg_option_set_type_name(section, "PROGRAM");
    d_arg_option_set_expected_range(names, 0, D_ARG_EXPECTED_UNLIMITED);
    d_arg_option_set_expected_range(items, 1, D_ARG_EXPECTED_UNLIMITED);
    d_arg_option_set_required(items, true);

    return true;
}

/*
d_internal_djinterp_core
  Every core djin's frame in the program's registry: parse's, desc's, the
environment printer's and the log's, so that their settings are read from
the user's file, the environment and the command line, and listed. One
already there is kept.
*/
static bool
d_internal_djinterp_core(void)
{
    struct dj_option_result made[4];
    bool                    held = true;

    made[0] = d_parse_register(NULL);
    made[1] = d_desc_register(NULL);
    made[2] = d_env_printer_register(NULL);
    made[3] = d_log_register(NULL);

    for (size_t i = 0u; i < sizeof(made) / sizeof(made[0]); ++i)
    {
        held = ( (held) &&
                 ( (made[i].is_ok) ||
                   (d_option_result_status(&made[i]) ==
                    (int32_t)D_OPTION_STATUS_KEY_DUPLICATE) ) );
    }

    return held;
}

/*
d_internal_djinterp_help_json
  The program's own members of its description (cli_help.h): `djins`, each
record's name, version, summary and commands, then each frame no record
names, its settings alone, in the order `list` gives them; and `reserved`,
the names a djin's page may not take.
*/
static int
d_internal_djinterp_help_json(
    void*                        _context,
    const struct dj_cli_program* _program,
    struct dj_cli_json*          _json
)
{
    struct dj_option_env_registry* const registry = d_option_env_registry();
    const struct dj_djin_set* const      set      = &d_internal_djinterp_set;

    (void)_context;
    (void)_program;

    d_cli_json_member(_json, "djins");
    d_cli_json_array(_json);

    for (uint32_t i = 0u; i < set->count; ++i)
    {
        const struct dj_djin* const djin = set->items[i];

        d_cli_json_object(_json);
        d_cli_json_member(_json, "name");
        d_cli_json_string(_json, djin->name);
        d_cli_json_member(_json, "version");
        d_cli_json_string(_json, djin->version);
        d_cli_json_member(_json, "summary");
        d_cli_json_string(_json, djin->summary);
        d_cli_json_member(_json, "commands");
        d_cli_json_array(_json);

        for (size_t c = 0u; c < djin->command_count; ++c)
        {
            d_cli_json_object(_json);
            d_cli_json_member(_json, "name");
            d_cli_json_string(_json, djin->commands[c].name);
            d_cli_json_member(_json, "summary");
            d_cli_json_string(_json, djin->commands[c].summary);
            d_cli_json_end(_json);
        }

        d_cli_json_end(_json);
        d_cli_json_member(_json, "settings_only");
        d_cli_json_bool(_json, false);
        d_cli_json_end(_json);
    }

    for (uint32_t i = 0u; i < d_option_env_registry_count(registry); ++i)
    {
        const struct dj_option_env* const frame =
            d_option_env_registry_at(registry, i);

        if ( (frame->name != NULL) &&
             (d_djin_set_find(set, frame->name, strlen(frame->name)) ==
              NULL) )
        {
            d_cli_json_object(_json);
            d_cli_json_member(_json, "name");
            d_cli_json_string(_json, frame->name);
            d_cli_json_member(_json, "settings_only");
            d_cli_json_bool(_json, true);
            d_cli_json_end(_json);
        }
    }

    d_cli_json_end(_json);
    d_cli_json_member(_json, "reserved");
    d_cli_json_array(_json);

    for (size_t i = 0u; d_internal_djinterp_reserved[i] != NULL; ++i)
    {
        d_cli_json_string(_json, d_internal_djinterp_reserved[i]);
    }

    d_cli_json_end(_json);

    return D_ARG_OK;
}

/*
d_djinterp_program_make
  The kit's program, the core djins, the djins' set, the commands, and the
program's members of its description.
*/
bool d_djinterp_program_make(
    struct dj_cli_program* _program,
    struct dj_arg_parser*  _parser
)
{
    if (d_cli_program_init(_program, _parser, "djinterp", D_VERSION_STRING,
                           D_CLI_PROGRAM_COMMANDS) != D_ARG_OK)
    {
        return false;
    }

    (void)d_djin_set_init(&d_internal_djinterp_set,
                          d_internal_djinterp_room,
                          D_INTERNAL_DJINTERP_DJINS,
                          d_option_env_registry(),
                          d_internal_djinterp_reserved);
    d_cli_program_set_help_json(_program, d_internal_djinterp_help_json,
                                NULL);

    return ( (d_internal_djinterp_core()) &&
             (d_internal_djinterp_commands(_program)) );
}
