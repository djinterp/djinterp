/*******************************************************************************
* djinterp [c]                                                            djin.c
*
*   Djins (djin.h): their records checked, kept in a program's set, their
* settings' frames registered in the program's registry, and the set and a
* djin written out as text for the program's `list` and `describe`.
*   Nothing here allocates. The set lives in the caller's room, and the text
* goes straight to the caller's sink, a column at a time, so a writer needs
* only the widths it measures first.
*
*
* path:      /src/djinterp/c/modules/interp/djin.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#include "../../../../../inc/djinterp/c/modules/interp/djin.h"  // corresponding header
// std
#include <stdio.h>   // snprintf
#include <string.h>  // memcmp, memcpy, strchr, strcmp, strlen
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root

#if RE_STD_HAS_INT64


// D_INTERNAL_DJIN_DEFAULT_ROOM, D_INTERNAL_DJIN_NAME_ROOM
//   constant: the most of a default's text, and of a setting's full name, a
// description shows, with its NUL; a longer one ends in "...".
#define D_INTERNAL_DJIN_DEFAULT_ROOM 96u
#define D_INTERNAL_DJIN_NAME_ROOM    128u

// d_internal_djin_status_names
//   constant: the statuses' names, by value.
static const char* const d_internal_djin_status_names[] =
{
    "ok", "invalid", "duplicate", "reserved", "full", "settings"
};


//==============================================================================
// 1.  CHECKING A RECORD
//==============================================================================

/*
d_internal_djin_is_word
  Whether `_text` is a lowercase letter followed by lowercase letters,
digits and `_extra`, the one other character the kind of name takes: '_'
for a djin's name, which is an identifier, and '-' for a command's.
*/
static bool
d_internal_djin_is_word(
    const char* _text,
    char        _extra
)
{
    const char* at;

    if ( (!_text) ||
         (_text[0] < 'a') ||
         (_text[0] > 'z') )
    {
        return false;
    }

    for (at = _text + 1; *at; ++at)
    {
        if ( (!((*at >= 'a') && (*at <= 'z'))) &&
             (!((*at >= '0') && (*at <= '9'))) &&
             (*at != _extra) )
        {
            return false;
        }
    }

    return true;
}

/*
d_internal_djin_is_line
  Whether `_text` is one line of text: present, not empty, no line break.
*/
static bool
d_internal_djin_is_line(
    const char* _text
)
{
    return ( (_text)                   &&
             (_text[0] != '\0')        &&
             (!strchr(_text, '\n'))    &&
             (!strchr(_text, '\r')) );
}

/*
d_internal_djin_commands_valid
  Whether a djin's commands are in shape: each record as large as this
build's, a command name, a one-line summary, and no name twice.
*/
static bool
d_internal_djin_commands_valid(
    const struct dj_djin* _djin
)
{
    size_t i;
    size_t j;

    if ( (_djin->command_count > 0) &&
         (!_djin->commands) )
    {
        return false;
    }

    for (i = 0; i < _djin->command_count; ++i)
    {
        const struct dj_djin_command* command = &_djin->commands[i];

        if ( (command->size < (uint32_t)sizeof(struct dj_djin_command)) ||
             (!d_internal_djin_is_word(command->name, '-'))              ||
             (!d_internal_djin_is_line(command->summary)) )
        {
            return false;
        }

        for (j = 0; j < i; ++j)
        {
            if (strcmp(_djin->commands[j].name, command->name) == 0)
            {
                return false;
            }
        }
    }

    return true;
}

/*
d_internal_djin_record_valid
  Whether a djin's record is in shape: this ABI, at least as large as this
build's record, a name, a version, a one-line summary, its commands, and a
settings frame, if any, named as the djin.
*/
static bool
d_internal_djin_record_valid(
    const struct dj_djin* _djin
)
{
    return ( (_djin->abi == D_DJIN_ABI)                              &&
             (_djin->size >= (uint32_t)sizeof(struct dj_djin))       &&
             (d_internal_djin_is_word(_djin->name, '_'))             &&
             (d_internal_djin_is_line(_djin->version))               &&
             (d_internal_djin_is_line(_djin->summary))               &&
             (d_internal_djin_commands_valid(_djin))                 &&
             ( (!_djin->settings) ||
               ( (_djin->settings->name) &&
                 (strcmp(_djin->settings->name, _djin->name) == 0) ) ) );
}

/*
d_internal_djin_is_reserved
  Whether `_name` is one of the program's own command names.
*/
static bool
d_internal_djin_is_reserved(
    const char* const* _reserved,
    const char*        _name
)
{
    const char* const* at;

    for (at = _reserved; (at) && (*at); ++at)
    {
        if (strcmp(*at, _name) == 0)
        {
            return true;
        }
    }

    return false;
}


//==============================================================================
// 2.  THE SET
//==============================================================================

/*
d_djin_set_init
  An empty set over the caller's room.
*/
int32_t
d_djin_set_init(
    struct dj_djin_set*            _set,
    const struct dj_djin**         _room,
    uint32_t                       _capacity,
    struct dj_option_env_registry* _registry,
    const char* const*             _reserved
)
{
    if ( (!_set) ||
         ( (!_room) && (_capacity > 0) ) )
    {
        return D_DJIN_STATUS_INVALID;
    }

    _set->items    = _room;
    _set->capacity = _capacity;
    _set->count    = 0;
    _set->registry = _registry;
    _set->reserved = _reserved;

    return D_DJIN_STATUS_OK;
}

/*
d_internal_djin_register_settings
  Registers a djin's frame in the set's registry, unless it is there already;
a different frame of its name there is a duplicate.
*/
static int32_t
d_internal_djin_register_settings(
    struct dj_djin_set*   _set,
    const struct dj_djin* _djin
)
{
    const struct dj_option_env* present;
    struct dj_option_result     added;

    if (!_djin->settings)
    {
        return D_DJIN_STATUS_OK;
    }

    if (!_set->registry)
    {
        return D_DJIN_STATUS_SETTINGS;
    }

    present = d_option_env_registry_find(_set->registry,
                                         _djin->name,
                                         strlen(_djin->name));

    if (present)
    {
        return (present == _djin->settings) ? D_DJIN_STATUS_OK
                                            : D_DJIN_STATUS_DUPLICATE;
    }

    added = d_option_env_registry_add(_set->registry, _djin->settings);

    return (added.is_ok) ? D_DJIN_STATUS_OK
                         : D_DJIN_STATUS_SETTINGS;
}

/*
d_djin_register
  Checks, then registers the settings, then lists: the settings are the last
step that can fail, so a refused djin changes nothing.
*/
int32_t
d_djin_register(
    struct dj_djin_set*   _set,
    const struct dj_djin* _djin
)
{
    int32_t status;

    if ( (!_set)  ||
         (!_djin) ||
         (!d_internal_djin_record_valid(_djin)) )
    {
        return D_DJIN_STATUS_INVALID;
    }

    if (d_internal_djin_is_reserved(_set->reserved, _djin->name))
    {
        return D_DJIN_STATUS_RESERVED;
    }

    if (d_djin_set_find(_set, _djin->name, strlen(_djin->name)))
    {
        return D_DJIN_STATUS_DUPLICATE;
    }

    if (_set->count >= _set->capacity)
    {
        return D_DJIN_STATUS_FULL;
    }

    status = d_internal_djin_register_settings(_set, _djin);

    if (status != D_DJIN_STATUS_OK)
    {
        return status;
    }

    _set->items[_set->count] = _djin;
    ++_set->count;

    return D_DJIN_STATUS_OK;
}

/*
d_djin_status_name
  A table by value.
*/
const char*
d_djin_status_name(
    int32_t _status
)
{
    if ( (_status < D_DJIN_STATUS_OK) ||
         (_status > D_DJIN_STATUS_SETTINGS) )
    {
        return "unknown";
    }

    return d_internal_djin_status_names[_status];
}

/*
d_djin_set_count
  How many are registered; 0 for no set.
*/
uint32_t
d_djin_set_count(
    const struct dj_djin_set* _set
)
{
    return (_set) ? _set->count : 0u;
}

/*
d_djin_set_at
  The one at a place, or null.
*/
const struct dj_djin*
d_djin_set_at(
    const struct dj_djin_set* _set,
    uint32_t                  _index
)
{
    return ( (_set) && (_index < _set->count) ) ? _set->items[_index]
                                                : NULL;
}

/*
d_internal_djin_name_is
  Whether a NUL-terminated name is the `_size` bytes at `_name`.
*/
static bool
d_internal_djin_name_is(
    const char* _known,
    const char* _name,
    size_t      _size
)
{
    return ( (strlen(_known) == _size) &&
             (memcmp(_known, _name, _size) == 0) );
}

/*
d_djin_set_find
  The one of a name, by a walk: a program has a handful.
*/
const struct dj_djin*
d_djin_set_find(
    const struct dj_djin_set* _set,
    const char*               _name,
    size_t                    _size
)
{
    uint32_t i;

    if ( (!_set) ||
         (!_name) )
    {
        return NULL;
    }

    for (i = 0; i < _set->count; ++i)
    {
        if (d_internal_djin_name_is(_set->items[i]->name, _name, _size))
        {
            return _set->items[i];
        }
    }

    return NULL;
}

/*
d_djin_command_find
  A djin's command of a name, by a walk.
*/
const struct dj_djin_command*
d_djin_command_find(
    const struct dj_djin* _djin,
    const char*           _name,
    size_t                _size
)
{
    size_t i;

    if ( (!_djin) ||
         (!_name) )
    {
        return NULL;
    }

    for (i = 0; i < _djin->command_count; ++i)
    {
        if (d_internal_djin_name_is(_djin->commands[i].name, _name, _size))
        {
            return &_djin->commands[i];
        }
    }

    return NULL;
}


//==============================================================================
// 3.  AS TEXT
//==============================================================================

/*
d_internal_djin_put
  Writes `_size` bytes; whether the sink took them all (d_sink_emit).
*/
static bool
d_internal_djin_put(
    struct dj_pack_sink _sink,
    const char*         _text,
    size_t              _size
)
{
    return (d_sink_emit(_sink, _text, _size) == 1);
}

/*
d_internal_djin_cell
  Writes `_text`, then spaces to `_width`, then the two between columns;
the last column takes no padding.
*/
static bool
d_internal_djin_cell(
    struct dj_pack_sink _sink,
    const char*         _text,
    size_t              _width,
    bool                _is_last
)
{
    static const char spaces[] = "                ";
    size_t            length   = strlen(_text);
    size_t            pad      = (_width > length) ? (_width - length) : 0;

    if (!d_internal_djin_put(_sink, _text, length))
    {
        return false;
    }

    if (_is_last)
    {
        return d_internal_djin_put(_sink, "\n", 1);
    }

    for (pad += 2; pad > 0; )
    {
        size_t step = (pad < sizeof(spaces) - 1) ? pad : sizeof(spaces) - 1;

        if (!d_internal_djin_put(_sink, spaces, step))
        {
            return false;
        }

        pad -= step;
    }

    return true;
}

/*
d_internal_djin_named
  The name a value has among a row's names, or null.
*/
static const char*
d_internal_djin_named(
    const struct dj_option_env_var* _row,
    int64_t                         _value
)
{
    uint32_t i;

    for (i = 0; (_row->names) && (i < _row->name_count); ++i)
    {
        if (_row->names[i].value == _value)
        {
            return _row->names[i].name;
        }
    }

    return NULL;
}

/*
d_internal_djin_mark_cut
  Ends a text that snprintf cut short in "...": `_full` is what snprintf
answered for `_capacity`.
*/
static void
d_internal_djin_mark_cut(
    char*  _buffer,
    size_t _capacity,
    int    _full
)
{
    if ( (_full >= 0) &&
         ((size_t)_full >= _capacity) &&
         (_capacity > 4) )
    {
        memcpy(_buffer + _capacity - 4, "...", 4);
    }

    return;
}

/*
d_internal_djin_default
  A row's default as text, in `_buffer`: a choice, a flag set or a named INT
by name, a bool as true or false, a text in double quotes, "unset" for none.
A text longer than the room ends in "...".
*/
static void
d_internal_djin_default(
    const struct dj_option_env_var* _row,
    char*                           _buffer,
    size_t                          _capacity
)
{
    const char* named = d_internal_djin_named(_row, _row->integer);
    int         full  = 0;

    switch (_row->kind)
    {
        case D_OPTION_ENV_KIND_BOOL:
            full = snprintf(_buffer, _capacity, "%s",
                            (_row->integer) ? "true" : "false");
            break;
        case D_OPTION_ENV_KIND_REAL:
            full = snprintf(_buffer, _capacity, "%.17g", _row->real);
            break;
        case D_OPTION_ENV_KIND_TEXT:
            full = (_row->text) ? snprintf(_buffer, _capacity, "\"%s\"",
                                           _row->text)
                                : snprintf(_buffer, _capacity, "unset");
            break;
        default:
            full = (named) ? snprintf(_buffer, _capacity, "%s", named)
                           : snprintf(_buffer, _capacity, "%lld",
                                      (long long)_row->integer);
            break;
    }

    d_internal_djin_mark_cut(_buffer, _capacity, full);

    return;
}

/*
d_internal_djin_setting_cells
  One setting's five cells: its full name in `_name`, ending in "..." when
longer than the room, the default in `_value`, and the kind, the knob and the
help as pointers.
*/
static void
d_internal_djin_setting_cells(
    const char*                     _frame,
    const struct dj_option_env_var* _row,
    char*                           _name,
    size_t                          _name_room,
    char*                           _value,
    const char**                    _cells
)
{
    int full = snprintf(_name, _name_room, "%s.%s", _frame, _row->name);

    d_internal_djin_mark_cut(_name, _name_room, full);
    d_internal_djin_default(_row, _value, D_INTERNAL_DJIN_DEFAULT_ROOM);

    _cells[0] = _name;
    _cells[1] = d_option_env_kind_name(_row->kind);
    _cells[2] = _value;
    _cells[3] = (_row->knob) ? _row->knob : "-";
    _cells[4] = (_row->help) ? _row->help : "";

    return;
}

/*
d_internal_djin_write_settings
  A frame's own settings, a line each, in five columns as wide as their
widest cells: a first pass measures, a second writes. A line ends at its last
cell with text, so no line ends in spaces.
*/
static bool
d_internal_djin_write_settings(
    const struct dj_option_env* _frame,
    struct dj_pack_sink         _sink
)
{
    char        name[D_INTERNAL_DJIN_NAME_ROOM];
    char        value[D_INTERNAL_DJIN_DEFAULT_ROOM];
    const char* cells[5];
    size_t      width[5] = { 0, 0, 0, 0, 0 };
    uint32_t    i;
    int         pass;
    int         c;
    int         last;

    if (_frame->var_count == 0)
    {
        return true;
    }

    if (!d_internal_djin_put(_sink, "\nsettings\n", 10))
    {
        return false;
    }

    for (pass = 0; pass < 2; ++pass)
    {
        for (i = 0; i < _frame->var_count; ++i)
        {
            d_internal_djin_setting_cells(_frame->name, _frame->vars[i],
                                          name, sizeof(name), value, cells);

            // a setting with no help ends at its knob, with no padding
            last = (cells[4][0] != '\0') ? 4 : 3;

            for (c = 0; c <= last; ++c)
            {
                if (pass == 0)
                {
                    size_t length = strlen(cells[c]);

                    width[c] = (length > width[c]) ? length : width[c];
                }
                else if ( ( (c == 0) &&
                            (!d_internal_djin_put(_sink, "  ", 2)) ) ||
                          (!d_internal_djin_cell(_sink, cells[c], width[c],
                                                 c == last)) )
                {
                    return false;
                }
            }
        }
    }

    return true;
}

/*
d_internal_djin_widths
  The widths of the list's name and version columns: over the djins, and
over the registry's frames no djin names, whose version is "-".
*/
static void
d_internal_djin_widths(
    const struct dj_djin_set* _set,
    size_t*                   _name,
    size_t*                   _version
)
{
    uint32_t i;
    uint32_t frames = (_set->registry)
                    ? d_option_env_registry_count(_set->registry) : 0u;

    *_name    = 0;
    *_version = 1;

    for (i = 0; i < _set->count; ++i)
    {
        size_t n = strlen(_set->items[i]->name);
        size_t v = strlen(_set->items[i]->version);

        *_name    = (n > *_name)    ? n : *_name;
        *_version = (v > *_version) ? v : *_version;
    }

    for (i = 0; i < frames; ++i)
    {
        const struct dj_option_env* frame =
            d_option_env_registry_at(_set->registry, i);
        size_t n = (frame->name) ? strlen(frame->name) : 0;

        *_name = (n > *_name) ? n : *_name;
    }

    return;
}

/*
d_djin_write_list
  The djins, then the frames no djin names.
*/
int
d_djin_write_list(
    const struct dj_djin_set* _set,
    struct dj_pack_sink       _sink
)
{
    size_t   name_width;
    size_t   version_width;
    uint32_t frames;
    uint32_t i;

    if (!_set)
    {
        return -1;
    }

    d_internal_djin_widths(_set, &name_width, &version_width);

    for (i = 0; i < _set->count; ++i)
    {
        const struct dj_djin* djin = _set->items[i];

        if ( (!d_internal_djin_cell(_sink, djin->name, name_width, false))  ||
             (!d_internal_djin_cell(_sink, djin->version, version_width,
                                    false))                                 ||
             (!d_internal_djin_cell(_sink, djin->summary, 0, true)) )
        {
            return -1;
        }
    }

    frames = (_set->registry) ? d_option_env_registry_count(_set->registry)
                              : 0u;

    for (i = 0; i < frames; ++i)
    {
        const struct dj_option_env* frame =
            d_option_env_registry_at(_set->registry, i);

        if ( (!frame->name) ||
             (d_djin_set_find(_set, frame->name, strlen(frame->name))) )
        {
            continue;
        }

        if ( (!d_internal_djin_cell(_sink, frame->name, name_width, false)) ||
             (!d_internal_djin_cell(_sink, "-", version_width, false))      ||
             (!d_internal_djin_cell(_sink, "settings only", 0, true)) )
        {
            return -1;
        }
    }

    return 0;
}

/*
d_internal_djin_write_commands
  A djin's commands, a line each, the names in a column.
*/
static bool
d_internal_djin_write_commands(
    const struct dj_djin* _djin,
    struct dj_pack_sink   _sink
)
{
    size_t width = 0;
    size_t i;

    if (_djin->command_count == 0)
    {
        return true;
    }

    for (i = 0; i < _djin->command_count; ++i)
    {
        size_t n = strlen(_djin->commands[i].name);

        width = (n > width) ? n : width;
    }

    if (!d_internal_djin_put(_sink, "\ncommands\n", 10))
    {
        return false;
    }

    for (i = 0; i < _djin->command_count; ++i)
    {
        if ( (!d_internal_djin_put(_sink, "  ", 2))                       ||
             (!d_internal_djin_cell(_sink, _djin->commands[i].name, width,
                                    false))                               ||
             (!d_internal_djin_cell(_sink, _djin->commands[i].summary, 0,
                                    true)) )
        {
            return false;
        }
    }

    return true;
}

/*
d_djin_write_description
  The heading, the summary, the commands, the settings.
*/
int
d_djin_write_description(
    const struct dj_djin* _djin,
    struct dj_pack_sink   _sink
)
{
    if (!_djin)
    {
        return -1;
    }

    if ( (!d_internal_djin_cell(_sink, _djin->name, 0, false))             ||
         (!d_internal_djin_cell(_sink, _djin->version, 0, true))           ||
         (!d_internal_djin_put(_sink, "  ", 2))                            ||
         (!d_internal_djin_cell(_sink, _djin->summary, 0, true))           ||
         (!d_internal_djin_write_commands(_djin, _sink))                   ||
         ( (_djin->settings) &&
           (!d_internal_djin_write_settings(_djin->settings, _sink)) ) )
    {
        return -1;
    }

    return 0;
}

/*
d_djin_write_frame
  A frame's name, that it is settings only, and its settings.
*/
int
d_djin_write_frame(
    const struct dj_option_env* _frame,
    struct dj_pack_sink         _sink
)
{
    if ( (!_frame) ||
         (!_frame->name) )
    {
        return -1;
    }

    if ( (!d_internal_djin_cell(_sink, _frame->name, 0, true))            ||
         (!d_internal_djin_put(_sink, "  settings only\n", 16))           ||
         (!d_internal_djin_write_settings(_frame, _sink)) )
    {
        return -1;
    }

    return 0;
}


#endif  // RE_STD_HAS_INT64
