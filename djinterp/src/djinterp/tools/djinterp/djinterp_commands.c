/*******************************************************************************
* djinterp [tools]                                           djinterp_commands.c
*
* The djinterp program's 0.8 preview, declared in djinterp_commands.h.
*   The kit starts the run's settings at the program's first parse, so a
* command's work finds the registry whole: the kit's djin, the root's
* variables, and every binding from the user's file, the environment and the
* command line. `list` and `describe` are the djin records' text writers
* (c/modules/interp/djin.h). `get` writes each setting as a statement
* d_option_env_load reads back, with d_option_env_format_quoted, and, at
* `--show-origin`, where its value came from and what lies under it. `set`
* binds each setting for the run first, so that a name or a value the
* setting does not take is refused before anything is written, then writes
* the user's file whole: a line that sets the same name alone is replaced,
* and each new statement goes at its end, where it is read last.
*
*
* path:      /src/djinterp/tools/djinterp/djinterp_commands.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
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
#include "../../../../inc/djinterp/c/dversion.h"  // D_VERSION_STRING
#include "../../../../inc/djinterp/c/filesys/file_dir.h"  // d_dir_create_parents
#include "../../../../inc/djinterp/c/modules/interp/djin.h"  // the records
#include "../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_command.h"  // commands
#include "../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_option.h"  // options
#include "../../../../inc/djinterp/c/option/option_env.h"  // frames
#include "../../../../inc/djinterp/c/option/option_env_registry.h"  // registry
#include "../../../../inc/djinterp/c/util/sink_common.h"  // d_sink_emit
#include "../../../../inc/djinterp/config/tools/cli/cfg_cli.h"  // the kit's hooks
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

// d_internal_djinterp_reserved
//   constant: the names the program keeps for its own commands, and the
// kit's.
static const char* const d_internal_djinterp_reserved[] =
{
    "list", "describe", "get", "set", "help", "completion", NULL
};

// d_internal_djinterp_room, d_internal_djinterp_set
//   variables: the program's djins: the room and the set over it.
static const struct dj_djin*
    d_internal_djinterp_room[D_INTERNAL_DJINTERP_DJINS];
static struct dj_djin_set d_internal_djinterp_set;

/*
d_internal_djinterp_put
  Text to the program's output, whole or said as a failure to write it.
*/
static bool
d_internal_djinterp_put(
    const struct dj_cli_program* _program,
    const char*                  _text,
    size_t                       _size
)
{
    if (d_sink_emit(_program->out, _text, _size) == 1)
    {
        return true;
    }

    (void)d_cli_file_error(_program, "write", "-", EIO);

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
    (void)_command;
    (void)_context;

    if (d_djin_write_list(&d_internal_djinterp_set, _program->out) != 0)
    {
        return d_cli_file_error(_program, "write", "-", EIO);
    }

    return D_CLI_EXIT_OK;
}

/*
d_internal_djinterp_work_describe
  A djin's description, or, for a frame no djin names, its settings.
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
    const struct dj_djin*       djin;
    const struct dj_option_env* frame;
    int                         wrote;

    (void)_context;

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

    wrote = (djin != NULL)
            ? d_djin_write_description(djin, _program->out)
            : d_djin_write_frame(frame, _program->out);

    return (wrote == 0) ? D_CLI_EXIT_OK
                        : d_cli_file_error(_program, "write", "-", EIO);
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
in force stands over, and the default, with the knob it came from.
*/
static bool
d_internal_djinterp_origins(
    const struct dj_cli_program*               _program,
    const struct dj_internal_djinterp_setting* _setting
)
{
    struct dj_option_env_layer layers[8];
    const size_t               count = d_option_env_layers(
                                           _setting->frame, _setting->scope,
                                           _setting->var, layers, 8u);
    const size_t               shown = (count == (size_t)-1) ? 0u
                                     : (count < 8u)          ? count
                                                             : 8u;
    char                       value[D_INTERNAL_DJINTERP_ROOM];
    char                       line[D_INTERNAL_DJINTERP_ROOM * 2u];
    bool                       held  = true;
    int                        size;

    for (size_t i = 1u; (held) && (i < shown); ++i)
    {
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
        d_internal_djinterp_value_text(_setting->row, _setting->row->text,
                                       _setting->row->integer,
                                       _setting->row->real,
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
  One setting's statement -- a comment for a text that is unset, which
loads as nothing -- and, at `--show-origin`, its source and what lies under
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

        bound = d_option_env_registry_assign(d_option_env_registry(),
                                             _items[i].data, name,
                                             equals + 1, size,
                                             D_OPTION_ENV_SOURCE_OVERRIDE);

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
d_internal_djinterp_sets_alone
  Whether a line of the file is one statement that sets `_name`, alone: its
name, then '=' after spaces, and no ';' or '#' after it.
*/
static bool
d_internal_djinterp_sets_alone(
    const char* _line,
    size_t      _length,
    const char* _name,
    size_t      _size
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
             (memchr(_line + at, ';', _length - at) == NULL) &&
             (memchr(_line + at, '#', _length - at) == NULL) );
}

/*
d_internal_djinterp_compose
  The file's new text into `_text`: each old line kept but one that sets a
name given alone, then each statement, as get writes it; how long it is, or
0 when it does not fit.
*/
static size_t
d_internal_djinterp_compose(
    const char*                _old,
    size_t                     _old_length,
    const struct dj_pack_text* _items,
    size_t                     _count,
    char*                      _text,
    size_t                     _capacity
)
{
    struct dj_internal_djinterp_setting setting;
    char                                value[D_INTERNAL_DJINTERP_ROOM];
    size_t                              used = 0u;

    for (size_t at = 0u; at < _old_length; )
    {
        const char* const end   = (const char*)memchr(_old + at, '\n',
                                                      _old_length - at);
        const size_t      line  = (end != NULL) ? (size_t)(end - _old) - at
                                                : _old_length - at;
        bool              drop  = false;

        for (size_t i = 0u; (!drop) && (i < _count); ++i)
        {
            const size_t name = (size_t)((const char*)memchr(
                                    _items[i].data, '=', _items[i].length) -
                                    _items[i].data);

            drop = d_internal_djinterp_sets_alone(_old + at, line,
                                                  _items[i].data, name);
        }

        if ( (!drop) &&
             (used + line + 1u < _capacity) )
        {
            memcpy(_text + used, _old + at, line);
            used         += line;
            _text[used++] = '\n';
        }
        else if (!drop)
        {
            return 0u;
        }

        at += line + 1u;
    }

    for (size_t i = 0u; i < _count; ++i)
    {
        const size_t name  = (size_t)((const char*)memchr(
                                 _items[i].data, '=', _items[i].length) -
                                 _items[i].data);
        int          wrote;

        (void)d_internal_djinterp_find(_items[i].data, name, &setting);
        (void)d_option_env_format_quoted(setting.frame, setting.scope,
                                         setting.var, value, sizeof(value));
        wrote = snprintf(_text + used, _capacity - used, "%.*s = %s\n",
                         (int)name, _items[i].data, value);

        if ( (wrote < 0) ||
             ((size_t)wrote >= _capacity - used) )
        {
            return 0u;
        }

        used += (size_t)wrote;
    }

    return used;
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
  Bound for the run first, every one, then the file written whole.
*/
static int
d_internal_djinterp_work_set(
    struct dj_cli_program* _program,
    struct dj_arg_command* _command,
    void*                  _context
)
{
    size_t                           count  = 0u;
    const struct dj_pack_text* const items  = d_arg_option_values(
        d_arg_command_option(_command, "SETTING"), &count);
    char                             path[D_INTERNAL_DJINTERP_ROOM];
    char*                            old    = NULL;
    size_t                           length = 0u;
    char*                            text;
    size_t                           room;
    size_t                           made;
    int                              failed;

    (void)_context;

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

    // the old text and a line's room for each statement, where that fits
    room = ( (count < ((size_t)-1 / D_INTERNAL_DJINTERP_ROOM) - 1u) &&
             (length < (size_t)-1 - (count + 1u) * D_INTERNAL_DJINTERP_ROOM) )
           ? length + (count + 1u) * D_INTERNAL_DJINTERP_ROOM
           : 0u;
    text = (room != 0u) ? (char*)D_INTERNAL_CLI_MALLOC(room) : NULL;
    made = (text != NULL)
           ? d_internal_djinterp_compose(old, length, items, count, text,
                                         room)
           : 0u;
    d_cli_free(old);

    failed = (text == NULL)   ? ENOMEM
           : (made == 0u)     ? ENOMEM
                              : d_internal_djinterp_folder(path);
    failed = (failed != 0) ? failed
                           : d_cli_write_file(path, text, made);
    D_INTERNAL_CLI_FREE(text);

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
    struct dj_arg_option*        names    = (get != NULL)
        ? d_arg_command_add_option(get, "NAME", "a setting's full name")
        : NULL;
    struct dj_arg_option*        items    = (set != NULL)
        ? d_arg_command_add_option(set, "SETTING",
                                   "NAME=VALUE: a setting's full name, and "
                                   "its value")
        : NULL;

    if ( (list == NULL) || (djin == NULL) || (names == NULL) ||
         (items == NULL) ||
         (d_arg_command_add_flag(get, "--show-origin",
                                 "say where each value came from, and "
                                 "what lies under it") == NULL) )
    {
        return false;
    }

    d_arg_option_set_required(djin, true);
    d_arg_option_set_expected_range(names, 0, D_ARG_EXPECTED_UNLIMITED);
    d_arg_option_set_expected_range(items, 1, D_ARG_EXPECTED_UNLIMITED);
    d_arg_option_set_required(items, true);

    return true;
}

/*
d_djinterp_program_make
  The kit's program, the djins' set, and the commands.
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

    return d_internal_djinterp_commands(_program);
}
