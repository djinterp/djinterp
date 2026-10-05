/*******************************************************************************
* djinterp [djinterp]                                                dsettings.c
*
* Settings registry:
*   Entries live in a flat array in registration order; names and aliases live
* in a separate lookup array kept sorted, searched by bisection.  Every value
* is stored as validated text, so a reader of any type sees exactly what was
* accepted and a listing can print it back unchanged.
*   An unknown name is refused with the closest registered name, if one is
* close.  A typo in a flag is the likeliest mistake anyone makes here, and
* "did you mean" costs a few comparisons against a list this short.
*
*
* path:      /src/djinterp/tools/dawk/dsettings.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.09.21
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dsettings.h"  // corresponding header
// std
#include <stdio.h>   // snprintf
#include <stdlib.h>  // calloc, realloc, free, strtol
#include <string.h>  // strcmp, strlen, strchr, memcpy


// D_SETTINGS_VALUE_MAX
//   constant: the longest value text accepted.
#define D_SETTINGS_VALUE_MAX 128


// d_internal_entry
//   struct: one registered setting and its current value.
struct d_internal_entry
{
    const struct d_setting_def*  def;
    char                         value[D_SETTINGS_VALUE_MAX];
    enum d_setting_source        source;
};

// d_internal_key
//   struct: one lookup row -- a name or an alias, and its entry.
struct d_internal_key
{
    const char*  key;
    size_t       entry;
};

struct d_settings
{
    struct d_internal_entry*  entries;
    size_t                    entry_count;
    size_t                    entry_capacity;

    struct d_internal_key*    keys;
    size_t                    key_count;
    size_t                    key_capacity;
};


static int
d_internal_key_compare(
    const void* _left,
    const void* _right
)
{
    return strcmp(((const struct d_internal_key*)_left)->key,
                  ((const struct d_internal_key*)_right)->key);
}


/*
d_internal_find
  Bisects the sorted lookup for a name or alias.  Returns the entry index, or
the entry count when the name is not registered.
*/
static size_t
d_internal_find(
    const struct d_settings* _settings,
    const char*              _name
)
{
    size_t low  = 0;
    size_t high = _settings->key_count;

    while (low < high)
    {
        const size_t middle = low + ((high - low) / 2u);
        const int    order  = strcmp(_settings->keys[middle].key, _name);

        if (order == 0)
        {
            return _settings->keys[middle].entry;
        }

        if (order < 0)
        {
            low = middle + 1u;
        }
        else
        {
            high = middle;
        }
    }

    return _settings->entry_count;
}


/*
d_internal_distance
  Edit distance between two short strings, capped: anything beyond three is
reported as four, since only close names are worth suggesting.
*/
static size_t
d_internal_distance(
    const char* _a,
    const char* _b
)
{
    const size_t n = strlen(_a);
    const size_t m = strlen(_b);

    if ((n > 63u) || (m > 63u))
    {
        return 4u;
    }

    size_t row[64];

    for (size_t j = 0; j <= m; ++j)
    {
        row[j] = j;
    }

    for (size_t i = 1; i <= n; ++i)
    {
        size_t diagonal = row[0];

        row[0] = i;

        for (size_t j = 1; j <= m; ++j)
        {
            const size_t above = row[j];
            const size_t cost  = (_a[i - 1u] == _b[j - 1u]) ? 0u : 1u;

            size_t best = diagonal + cost;

            if ((above + 1u) < best)
            {
                best = above + 1u;
            }

            if ((row[j - 1u] + 1u) < best)
            {
                best = row[j - 1u] + 1u;
            }

            row[j]   = best;
            diagonal = above;
        }
    }

    return (row[m] > 3u) ? 4u : row[m];
}


/*
d_internal_validate
  Checks a value against its definition and writes the canonical text.
*/
static bool
d_internal_validate(
    const struct d_setting_def* _def,
    const char*                 _value,
    char*                       _out,
    char*                       _error,
    size_t                      _error_size
)
{
    if (strlen(_value) >= D_SETTINGS_VALUE_MAX)
    {
        (void)snprintf(_error, _error_size, "value for '%s' is too long",
                       _def->name);
        return false;
    }

    if (_def->kind == D_SETTING_BOOL)
    {
        static const char* const yes[] = { "true", "on", "yes", "1" };
        static const char* const no[]  = { "false", "off", "no", "0" };

        for (size_t at = 0; at < 4u; ++at)
        {
            if (strcmp(_value, yes[at]) == 0)
            {
                (void)memcpy(_out, "true", 5u);
                return true;
            }

            if (strcmp(_value, no[at]) == 0)
            {
                (void)memcpy(_out, "false", 6u);
                return true;
            }
        }

        (void)snprintf(_error, _error_size, "'%s' is a flag; '%s' is not "
                       "true or false", _def->name, _value);
        return false;
    }

    if (_def->kind == D_SETTING_INT)
    {
        char* end = NULL;

        const long number = strtol(_value, &end, 10);

        if ((end == _value) || (*end != '\0') ||
            (number < _def->minimum) || (number > _def->maximum))
        {
            (void)snprintf(_error, _error_size, "'%s' takes an integer from "
                           "%ld to %ld, not '%s'", _def->name, _def->minimum,
                           _def->maximum, _value);
            return false;
        }

        (void)snprintf(_out, D_SETTINGS_VALUE_MAX, "%ld", number);
        return true;
    }

    for (size_t at = 0; (_def->choices) && (_def->choices[at]); ++at)
    {
        if (strcmp(_value, _def->choices[at]) == 0)
        {
            (void)memcpy(_out, _value, strlen(_value) + 1u);
            return true;
        }
    }

    // a ruled-but-unbuilt value is refused by name, never silently ignored
    for (size_t at = 0; (_def->unbuilt) && (_def->unbuilt[at]); ++at)
    {
        if (strcmp(_value, _def->unbuilt[at]) == 0)
        {
            (void)snprintf(_error, _error_size, "'%s=%s' is ruled but not yet "
                           "built", _def->name, _value);
            return false;
        }
    }

    size_t used = (size_t)snprintf(_error, _error_size, "'%s' must be one of",
                                   _def->name);

    for (size_t at = 0; (_def->choices) && (_def->choices[at]) &&
                        (used < _error_size); ++at)
    {
        used += (size_t)snprintf(_error + used, _error_size - used, " %s",
                                 _def->choices[at]);
    }

    if (used < _error_size)
    {
        (void)snprintf(_error + used, _error_size - used, ", not '%s'",
                       _value);
    }

    return false;
}


struct d_settings*
d_settings_new(
    void
)
{
    return calloc(1u, sizeof(struct d_settings));
}


void
d_settings_free(
    struct d_settings* _settings
)
{
    if (_settings)
    {
        free(_settings->entries);
        free(_settings->keys);
        free(_settings);
    }

    return;
}


/*
d_internal_add_key
*/
static bool
d_internal_add_key(
    struct d_settings* _settings,
    const char*        _key,
    size_t             _entry
)
{
    if (_settings->key_count == _settings->key_capacity)
    {
        const size_t grown = (_settings->key_capacity == 0)
                           ? 16u
                           : (_settings->key_capacity * 2u);

        struct d_internal_key* const keys =
            realloc(_settings->keys, grown * sizeof(struct d_internal_key));

        if (!keys)
        {
            return false;
        }

        _settings->keys         = keys;
        _settings->key_capacity = grown;
    }

    _settings->keys[_settings->key_count].key   = _key;
    _settings->keys[_settings->key_count].entry = _entry;

    ++_settings->key_count;

    return true;
}


/*
d_settings_register
  Adds definitions and gives each its default.  A name or alias already taken
is refused: two components claiming one setting would each believe they
control it.
*/
bool
d_settings_register(
    struct d_settings*          _settings,
    const struct d_setting_def* _defs,
    size_t                      _count
)
{
    if ((!_settings) || ((!_defs) && (_count > 0)))
    {
        return false;
    }

    for (size_t at = 0; at < _count; ++at)
    {
        const struct d_setting_def* const def = &_defs[at];

        if ( (d_internal_find(_settings, def->name) != _settings->entry_count)
             ||
             ((def->alias) &&
              (d_internal_find(_settings, def->alias)
               != _settings->entry_count)) )
        {
            return false;
        }

        if (_settings->entry_count == _settings->entry_capacity)
        {
            const size_t grown = (_settings->entry_capacity == 0)
                               ? 16u
                               : (_settings->entry_capacity * 2u);

            struct d_internal_entry* const entries =
                realloc(_settings->entries,
                        grown * sizeof(struct d_internal_entry));

            if (!entries)
            {
                return false;
            }

            _settings->entries        = entries;
            _settings->entry_capacity = grown;
        }

        struct d_internal_entry* const entry =
            &_settings->entries[_settings->entry_count];

        char error[160];

        entry->def    = def;
        entry->source = D_SETTING_DEFAULT;

        // a default that fails its own definition is a registration bug
        if (!d_internal_validate(def, def->fallback, entry->value, error,
                                 sizeof(error)))
        {
            return false;
        }

        const size_t index = _settings->entry_count;

        ++_settings->entry_count;

        if ( (!d_internal_add_key(_settings, def->name, index)) ||
             ((def->alias) && (!d_internal_add_key(_settings, def->alias,
                                                   index))) )
        {
            return false;
        }

        qsort(_settings->keys, _settings->key_count,
              sizeof(struct d_internal_key), d_internal_key_compare);
    }

    return true;
}


bool
d_settings_set(
    struct d_settings*    _settings,
    const char*           _name,
    const char*           _value,
    enum d_setting_source _source,
    char*                 _error,
    size_t                _error_size
)
{
    if ((!_settings) || (!_name) || (!_value))
    {
        return false;
    }

    const size_t at = d_internal_find(_settings, _name);

    if (at == _settings->entry_count)
    {
        const char* nearest = NULL;
        size_t      best    = 3u;

        for (size_t key = 0; key < _settings->key_count; ++key)
        {
            const size_t distance =
                d_internal_distance(_name, _settings->keys[key].key);

            if (distance < best)
            {
                best    = distance;
                nearest = _settings->keys[key].key;
            }
        }

        if (nearest)
        {
            (void)snprintf(_error, _error_size, "unknown setting '%s'; did "
                           "you mean '%s'?", _name, nearest);
        }
        else
        {
            (void)snprintf(_error, _error_size, "unknown setting '%s'",
                           _name);
        }

        return false;
    }

    struct d_internal_entry* const entry = &_settings->entries[at];

    char canonical[D_SETTINGS_VALUE_MAX];

    if (!d_internal_validate(entry->def, _value, canonical, _error,
                             _error_size))
    {
        return false;
    }

    // a value yields only to a source of equal or higher standing
    if (_source >= entry->source)
    {
        (void)memcpy(entry->value, canonical, strlen(canonical) + 1u);
        entry->source = _source;
    }

    return true;
}


/*
d_settings_apply
  Applies a comma-separated list of name=value pairs, as the environment
supplies them.  The first bad pair stops the list and is reported.
*/
bool
d_settings_apply(
    struct d_settings*    _settings,
    const char*           _list,
    enum d_setting_source _source,
    char*                 _error,
    size_t                _error_size
)
{
    if ((!_settings) || (!_list))
    {
        return true;
    }

    const char* at = _list;

    while (*at)
    {
        while ((*at == ',') || (*at == ' '))
        {
            ++at;
        }

        if (*at == '\0')
        {
            break;
        }

        const char* end = at;

        while ((*end) && (*end != ','))
        {
            ++end;
        }

        char pair[2u * D_SETTINGS_VALUE_MAX];

        const size_t length = (size_t)(end - at);

        if (length >= sizeof(pair))
        {
            (void)snprintf(_error, _error_size, "setting too long");
            return false;
        }

        (void)memcpy(pair, at, length);

        pair[length] = '\0';

        char* const equals = strchr(pair, '=');

        if (!equals)
        {
            (void)snprintf(_error, _error_size, "'%s' is not name=value",
                           pair);
            return false;
        }

        *equals = '\0';

        if (!d_settings_set(_settings, pair, equals + 1, _source, _error,
                            _error_size))
        {
            return false;
        }

        at = end;
    }

    return true;
}


const char*
d_settings_text(const struct d_settings* _settings, const char* _name)
{
    const size_t at = _settings ? d_internal_find(_settings, _name) : 0u;

    return ((_settings) && (at < _settings->entry_count))
           ? _settings->entries[at].value : NULL;
}

long
d_settings_int(const struct d_settings* _settings, const char* _name)
{
    const char* const text = d_settings_text(_settings, _name);

    return text ? strtol(text, NULL, 10) : 0L;
}

bool
d_settings_bool(const struct d_settings* _settings, const char* _name)
{
    const char* const text = d_settings_text(_settings, _name);

    return ((text) && (strcmp(text, "true") == 0));
}

bool
d_settings_is(const struct d_settings* _settings, const char* _name,
              const char* _word)
{
    const char* const text = d_settings_text(_settings, _name);

    return ((text) && (_word) && (strcmp(text, _word) == 0));
}

enum d_setting_source
d_settings_source(const struct d_settings* _settings, const char* _name)
{
    const size_t at = _settings ? d_internal_find(_settings, _name) : 0u;

    return ((_settings) && (at < _settings->entry_count))
           ? _settings->entries[at].source : D_SETTING_DEFAULT;
}

size_t
d_settings_count(const struct d_settings* _settings)
{
    return _settings ? _settings->entry_count : 0u;
}

const struct d_setting_def*
d_settings_def_at(const struct d_settings* _settings, size_t _at)
{
    return ((_settings) && (_at < _settings->entry_count))
           ? _settings->entries[_at].def : NULL;
}

const char*
d_settings_value_at(const struct d_settings* _settings, size_t _at)
{
    return ((_settings) && (_at < _settings->entry_count))
           ? _settings->entries[_at].value : NULL;
}

enum d_setting_source
d_settings_source_at(const struct d_settings* _settings, size_t _at)
{
    return ((_settings) && (_at < _settings->entry_count))
           ? _settings->entries[_at].source : D_SETTING_DEFAULT;
}

const char*
d_settings_source_name(enum d_setting_source _source)
{
    static const char* const names[] =
    {
        "default", "sheet", "environment", "command line"
    };

    return ((unsigned)_source < 4u) ? names[_source] : "?";
}


/*
d_settings_take_option
  Consumes one command-line setting: `--set name=value`, `--name=value`,
`--name` for true or `--no-name` for false.  Returns 1 when it consumed one,
0 when the argument is not a setting option -- a positional, `--`, or
`--settings`, which the caller handles -- and -1 when it named a setting
badly, with the reason in `_error`.  Both dawk and dcheck parse settings
through this, so the two commands cannot drift apart.
*/
int
d_settings_take_option(
    struct d_settings* _settings,
    int                _argc,
    char**             _argv,
    int*               _at,
    char*              _error,
    size_t             _error_size
)
{
    const char* const argument = _argv[*_at];

    if ( (strncmp(argument, "--", 2u) != 0) || (argument[2] == '\0') ||
         (strcmp(argument, "--settings") == 0) )
    {
        return 0;
    }

    const char* body  = argument + 2;
    const char* value = "true";

    if (strcmp(argument, "--set") == 0)
    {
        if ((*_at + 1) >= _argc)
        {
            (void)snprintf(_error, _error_size, "--set needs name=value");
            return -1;
        }

        body = _argv[++(*_at)];
    }

    const char* const equals = strchr(body, '=');

    if (equals)
    {
        value = equals + 1;
    }
    else if (strncmp(body, "no-", 3u) == 0)
    {
        body += 3;
        value = "false";
    }

    const size_t length = equals ? (size_t)(equals - body) : strlen(body);

    char name[D_SETTINGS_VALUE_MAX];

    if (length >= sizeof(name))
    {
        (void)snprintf(_error, _error_size, "option too long");
        return -1;
    }

    (void)memcpy(name, body, length);

    name[length] = '\0';

    return d_settings_set(_settings, name, value, D_SETTING_COMMAND_LINE,
                          _error, _error_size) ? 1 : -1;
}


/*
d_settings_describe
  Writes every registered setting: its value and where it came from, what it
does, what it accepts, and any older name it answers to.
*/
void
d_settings_describe(
    const struct d_settings* _settings,
    FILE*                    _out
)
{
    for (size_t at = 0; at < d_settings_count(_settings); ++at)
    {
        const struct d_setting_def* const def = d_settings_def_at(_settings,
                                                                  at);

        (void)fprintf(_out, "  %-20s = %-10s (%s)\n", def->name,
                      d_settings_value_at(_settings, at),
                      d_settings_source_name(d_settings_source_at(_settings,
                                                                  at)));
        (void)fprintf(_out, "  %-20s   %s\n", "", def->summary);

        if (def->kind == D_SETTING_BOOL)
        {
            (void)fprintf(_out, "  %-20s   accepts: true | false\n", "");
        }
        else if (def->kind == D_SETTING_INT)
        {
            (void)fprintf(_out, "  %-20s   accepts: %ld to %ld\n", "",
                          def->minimum, def->maximum);
        }
        else
        {
            (void)fprintf(_out, "  %-20s   accepts:", "");

            for (size_t c = 0; def->choices[c]; ++c)
            {
                (void)fprintf(_out, "%s %s", (c > 0) ? " |" : "",
                              def->choices[c]);
            }

            for (size_t u = 0; (def->unbuilt) && (def->unbuilt[u]); ++u)
            {
                (void)fprintf(_out, "  (%s: ruled, not built)",
                              def->unbuilt[u]);
            }

            (void)fprintf(_out, "\n");
        }

        if (def->alias)
        {
            (void)fprintf(_out, "  %-20s   also: %s\n", "", def->alias);
        }
    }

    return;
}
