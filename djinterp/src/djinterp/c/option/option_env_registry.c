/*******************************************************************************
* djinterp [c]                                             option_env_registry.c
*
*   The djin registry (option_env_registry.h): the program's root frame, its
* list of djins, and settings by full name routed to the frame they name.
*
*   A FULL NAME IS RESOLVED BY THE FRAME IT NAMES. The registry only picks the
* frame -- the djin a name's first part names, or the root -- and hands it
* the name whole: a named frame takes its own name off the front of what it
* resolves (option_env.c), so "env_printer.cpp_features.key_width" reads in
* the djin env_printer as "cpp_features.key_width" does.
*
*   THE PROGRAM'S REGISTRY IS INITIALISED STATICALLY. Its root frame points
* into its own storage, and those addresses are constants, so the whole
* object is a designated initialiser: there is no first call to race.
*
*
* path:      /src/djinterp/c/option/option_env_registry.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.12
*******************************************************************************/
#include "../../../../inc/djinterp/c/option/option_env_registry.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <string.h>  // memchr, memcmp, strchr, strlen, strncmp
// djinterp
#include "../../../../inc/djinterp/c/option/option_common.h"  // d_option_fail
#include "../../../../inc/djinterp/c/option/option_env.h"     // the frames
// re_std
#include "../../../../inc/re_std/c/cstdint/dstdint.h"  // int32_t, uint32_t


#if RE_STD_HAS_INT64

// D_INTERNAL_OPTION_ENV_REGISTRY_PREFIX
//   constant: the bytes a djin's environment prefix is built in.
#define D_INTERNAL_OPTION_ENV_REGISTRY_PREFIX  256u

// d_internal_option_env_program
//   variable: the program's registry, its root frame over its own storage,
// named "djinterp".
static struct dj_option_env_registry d_internal_option_env_program =
{
    .frame =
    {
        .set =
        {
            .options        =
                d_internal_option_env_program.storage.option_block,
            .values         = (unsigned char*)
                d_internal_option_env_program.storage.value_block,
            .count          = 0u,
            .capacity       = (uint32_t)D_CFG_OPTION_ENV_REGISTRY_BINDINGS,
            .value_used     = 0u,
            .value_capacity = (uint32_t)(sizeof(uint64_t) *
                                         D_CFG_OPTION_ENV_REGISTRY_BINDINGS)
        },
        .vars          = d_internal_option_env_program.storage.var_block,
        .name          = "djinterp",
        .text          = d_internal_option_env_program.storage.text_block,
        .text_capacity = (size_t)D_CFG_OPTION_ENV_REGISTRY_TEXT,
        .var_capacity  = (uint32_t)D_CFG_OPTION_ENV_REGISTRY_VARIABLES
    },
    .djin_count = 0u
};


//==============================================================================
// THE REGISTRY
//==============================================================================

/*
d_option_env_registry
  The static one.
*/
struct dj_option_env_registry*
d_option_env_registry(void)
{
    return &d_internal_option_env_program;
}

/*
d_option_env_registry_init
  A frame over the registry's own storage, named, with an empty list.
*/
struct dj_option_result
d_option_env_registry_init(
    struct dj_option_env_registry* _registry,
    const char*                    _name
)
{
    if (_registry == NULL)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    const struct dj_option_result viewed =
        D_OPTION_ENV_VIEW(&_registry->frame, _registry->storage);

    if (!viewed.is_ok)
    {
        return viewed;
    }

    _registry->djin_count = 0u;
    _registry->reserved   = 0u;

    return d_option_env_set_name(&_registry->frame, _name);
}

/*
d_option_env_registry_frame
  Its root.
*/
struct dj_option_env*
d_option_env_registry_frame(
    struct dj_option_env_registry* _registry
)
{
    return (_registry != NULL) ? &_registry->frame : NULL;
}

/*
d_option_env_registry_add
  The name is checked against the list before the frame is touched, so a
refused djin keeps its bindings and its parent.
*/
struct dj_option_result
d_option_env_registry_add(
    struct dj_option_env_registry* _registry,
    struct dj_option_env*          _djin
)
{
    if ( (_registry == NULL)     ||
         (_djin == NULL)         ||
         (_djin->name == NULL)   ||
         (_djin == &_registry->frame) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    if (d_option_env_registry_find(_registry,
                                   _djin->name,
                                   strlen(_djin->name)) != NULL)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_KEY_DUPLICATE);
    }

    if (_registry->djin_count >= D_CFG_OPTION_ENV_REGISTRY_DJINS)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_CAPACITY);
    }

    const struct dj_option_result attached =
        d_option_env_set_parent(_djin, &_registry->frame);

    if (!attached.is_ok)
    {
        return attached;
    }

    _registry->djins[_registry->djin_count] = _djin;

    return d_option_ok(_registry->djin_count++);
}

/*
d_option_env_registry_remove
  The later djins move down a place, so the list keeps its order.
*/
struct dj_option_result
d_option_env_registry_remove(
    struct dj_option_env_registry* _registry,
    struct dj_option_env*          _djin
)
{
    if ( (_registry == NULL) ||
         (_djin == NULL) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    for (uint32_t i = 0u; i < _registry->djin_count; ++i)
    {
        if (_registry->djins[i] == _djin)
        {
            d_option_env_detach(_djin);

            for (uint32_t j = i + 1u; j < _registry->djin_count; ++j)
            {
                _registry->djins[j - 1u] = _registry->djins[j];
            }

            --_registry->djin_count;

            return d_option_ok(i);
        }
    }

    return d_option_fail((int32_t)D_OPTION_STATUS_KEY_ABSENT);
}

/*
d_option_env_registry_count
  The list's length.
*/
uint32_t
d_option_env_registry_count(
    const struct dj_option_env_registry* _registry
)
{
    return (_registry != NULL) ? _registry->djin_count : 0u;
}

/*
d_option_env_registry_at
  The list, checked.
*/
struct dj_option_env*
d_option_env_registry_at(
    const struct dj_option_env_registry* _registry,
    uint32_t                             _index
)
{
    return ( (_registry != NULL) &&
             (_index < _registry->djin_count) ) ? _registry->djins[_index]
                                                : NULL;
}

/*
d_option_env_registry_find
  By exact name, as option_env finds a variable.
*/
struct dj_option_env*
d_option_env_registry_find(
    const struct dj_option_env_registry* _registry,
    const char*                          _name,
    size_t                               _size
)
{
    if ( (_registry == NULL) ||
         (_name == NULL) )
    {
        return NULL;
    }

    for (uint32_t i = 0u; i < _registry->djin_count; ++i)
    {
        const char* const name = _registry->djins[i]->name;

        if ( (strlen(name) == _size) &&
             (memcmp(name, _name, _size) == 0) )
        {
            return _registry->djins[i];
        }
    }

    return NULL;
}

/*
d_option_env_registry_target
  A name with no '.' is the root's; one whose first part is a djin's name is
that djin's; any other is the root's, whose scopes may be what it names.
*/
struct dj_option_env*
d_option_env_registry_target(
    struct dj_option_env_registry* _registry,
    const char*                    _name,
    size_t                         _size
)
{
    if ( (_registry == NULL) ||
         (_name == NULL) )
    {
        return NULL;
    }

    const char* const dot = (const char*)memchr(_name, '.', _size);

    if (dot != NULL)
    {
        struct dj_option_env* const djin =
            d_option_env_registry_find(_registry,
                                       _name,
                                       (size_t)(dot - _name));

        if (djin != NULL)
        {
            return djin;
        }
    }

    return &_registry->frame;
}


//==============================================================================
// SETTINGS BY FULL NAME
//==============================================================================

/**
 * @brief The frame a statement's name addresses: its name is what follows
 *        space and an optional '-', up to the first character a name has
 *        not.
 *
 * @param[in] _registry  the registry.
 * @param[in] _text      the statement.
 * @param[in] _size      its length.
 * @return the frame.
 */
static struct dj_option_env*
d_internal_option_env_registry_route(
    struct dj_option_env_registry* _registry,
    const char*                    _text,
    size_t                         _size
)
{
    size_t at = 0u;

    while ( (at < _size) &&
            ( (_text[at] == ' ')  ||
              (_text[at] == '\t') ||
              (_text[at] == '-') ) )
    {
        ++at;
    }

    const size_t start = at;

    while ( (at < _size) &&
            ( ((_text[at] >= 'a') && (_text[at] <= 'z')) ||
              ((_text[at] >= 'A') && (_text[at] <= 'Z')) ||
              ((_text[at] >= '0') && (_text[at] <= '9')) ||
              (_text[at] == '_')                         ||
              (_text[at] == '.') ) )
    {
        ++at;
    }

    return d_option_env_registry_target(_registry,
                                        _text + start,
                                        at - start);
}

/*
d_option_env_registry_assign
  The frame the name addresses, then option_env's assign.
*/
struct dj_option_result
d_option_env_registry_assign(
    struct dj_option_env_registry* _registry,
    const char*                    _name,
    size_t                         _name_size,
    const char*                    _value,
    size_t                         _value_size,
    int32_t                        _source
)
{
    struct dj_option_env* const target =
        d_option_env_registry_target(_registry, _name, _name_size);

    if (target == NULL)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    return d_option_env_assign_from(target,
                                    _name,
                                    _name_size,
                                    _value,
                                    _value_size,
                                    _source);
}

/*
d_option_env_registry_put
  The statement whole, to the frame its name addresses.
*/
struct dj_option_result
d_option_env_registry_put(
    struct dj_option_env_registry* _registry,
    const char*                    _statement,
    int32_t                        _source
)
{
    if ( (_registry == NULL) ||
         (_statement == NULL) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    const size_t size = strlen(_statement);

    return d_option_env_put_n_from(
               d_internal_option_env_registry_route(_registry,
                                                    _statement,
                                                    size),
               _statement,
               size,
               _source);
}

/*
d_option_env_registry_load
  option_env's splitting, each statement routed on its own: one script may
set the root's variables and several djins'.
*/
struct dj_option_result
d_option_env_registry_load(
    struct dj_option_env_registry* _registry,
    const char*                    _script,
    int32_t                        _source,
    size_t*                        _error_offset
)
{
    size_t   at    = 0u;
    uint32_t count = 0u;

    if ( (_registry == NULL) ||
         (_script == NULL) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    while (_script[at] != '\0')
    {
        const size_t start = at;

        at = d_option_env_statement_end(_script, at);

        // a statement of nothing but space is skipped
        size_t first = start;

        while ( (first < at) &&
                ( (_script[first] == ' ')  ||
                  (_script[first] == '\t') ||
                  (_script[first] == '\r') ) )
        {
            ++first;
        }

        if (first < at)
        {
            const struct dj_option_result result = d_option_env_put_n_from(
                d_internal_option_env_registry_route(_registry,
                                                     _script + first,
                                                     at - first),
                _script + first,
                at - first,
                _source);

            if (!result.is_ok)
            {
                if (_error_offset != NULL)
                {
                    *_error_offset = first;
                }

                return result;
            }

            ++count;
        }

        // a comment runs to its line's end
        if (_script[at] == '#')
        {
            while ( (_script[at] != '\0') &&
                    (_script[at] != '\n') )
            {
                ++at;
            }
        }

        if (_script[at] != '\0')
        {
            ++at;
        }
    }

    return d_option_ok(count);
}

/*
d_option_env_registry_import
  The root under the prefix, then each djin under the prefix, its name
upper-cased, and an underscore. A djin whose prefix would not fit is passed
over, as option_env passes over a name that would not.
*/
struct dj_option_result
d_option_env_registry_import(
    struct dj_option_env_registry* _registry,
    const char*                    _prefix
)
{
    char              prefix[D_INTERNAL_OPTION_ENV_REGISTRY_PREFIX];
    const char* const base   = (_prefix != NULL) ? _prefix : "";
    const size_t      length = strlen(base);

    if (_registry == NULL)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    struct dj_option_result failure = d_option_ok(0u);
    struct dj_option_result result  = d_option_env_import(&_registry->frame,
                                                          base);
    uint32_t                count   = result.is_ok ? result.payload.value : 0u;

    failure = result.is_ok ? failure : result;

    for (uint32_t i = 0u; i < _registry->djin_count; ++i)
    {
        const char* const name = _registry->djins[i]->name;
        const size_t      size = strlen(name);

        if (length + size + 1u >= sizeof(prefix))
        {
            continue;
        }

        memcpy(prefix, base, length);

        for (size_t c = 0u; c < size; ++c)
        {
            prefix[length + c] = ( (name[c] >= 'a') &&
                                   (name[c] <= 'z') )
                                     ? (char)(name[c] - 'a' + 'A')
                                     : name[c];
        }

        prefix[length + size]      = '_';
        prefix[length + size + 1u] = '\0';
        result = d_option_env_import(_registry->djins[i], prefix);
        count += result.is_ok ? result.payload.value : 0u;

        if ( (!result.is_ok) &&
             (failure.is_ok) )
        {
            failure = result;
        }
    }

    return failure.is_ok ? d_option_ok(count) : failure;
}

/**
 * @brief Whether a full name names a bool, for the command line's bare
 *        forms.
 *
 * @param[in] _registry  the registry.
 * @param[in] _name      the name; need not end in a NUL.
 * @param[in] _size      its length.
 * @return 1 for a bool; 0 for a setting of another kind; -1 for a name
 *         that names nothing, which binding then reports.
 */
static int
d_internal_option_env_registry_is_bool(
    struct dj_option_env_registry* _registry,
    const char*                    _name,
    size_t                         _size
)
{
    struct dj_option_env* const target =
        d_option_env_registry_target(_registry, _name, _size);
    struct dj_option_env_value  value;
    uint32_t                    scope = D_OPTION_ENV_GLOBAL;

    if (target == NULL)
    {
        return -1;
    }

    const struct dj_option_result found =
        d_option_env_resolve(target, _name, _size, &scope);

    if ( (!found.is_ok) ||
         (!d_option_env_get(target, scope, found.payload.value,
                            &value).is_ok) )
    {
        return -1;
    }

    return (value.kind == (int32_t)D_OPTION_ENV_KIND_BOOL) ? 1 : 0;
}

/*
d_option_env_registry_take_option
  dsettings' forms. A bare "--NAME" or "--no-NAME" is a bool's alone: a
setting of another kind is refused before anything is bound, since a TEXT
would take the words "true" and "false" as its text.
*/
int32_t
d_option_env_registry_take_option(
    struct dj_option_env_registry* _registry,
    int                            _argc,
    char* const*                   _argv,
    int*                           _at,
    struct dj_option_result*       _result
)
{
    struct dj_option_result result =
        d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);

    if ( (_registry == NULL)     ||
         (_argv == NULL)         ||
         (_at == NULL)           ||
         (*_at < 0)              ||
         (*_at >= _argc)         ||
         (_argv[*_at] == NULL)   ||
         (strncmp(_argv[*_at], "--", 2u) != 0) ||
         (_argv[*_at][2] == '\0') )
    {
        return 0;
    }

    const char* body  = _argv[*_at] + 2;
    const char* value = "true";

    // "--set NAME=VALUE" takes the next argument, which must have a value
    if (strcmp(_argv[*_at], "--set") == 0)
    {
        if ( (*_at + 1 >= _argc) ||
             (strchr(_argv[*_at + 1], '=') == NULL) )
        {
            if (_result != NULL)
            {
                *_result = result;
            }

            return -1;
        }

        body = _argv[++*_at];
    }

    const char* const equals = strchr(body, '=');

    if (equals != NULL)
    {
        value = equals + 1;
    }
    else if (strncmp(body, "no-", 3u) == 0)
    {
        body += 3;
        value = "false";
    }

    const size_t size = (equals != NULL) ? (size_t)(equals - body)
                                         : strlen(body);

    if ( (equals == NULL) &&
         (d_internal_option_env_registry_is_bool(_registry, body, size) == 0) )
    {
        if (_result != NULL)
        {
            *_result = result;
        }

        return -1;
    }

    result = d_option_env_registry_assign(_registry,
                                          body,
                                          size,
                                          value,
                                          strlen(value),
                                          D_OPTION_ENV_SOURCE_COMMAND_LINE);

    if (_result != NULL)
    {
        *_result = result;
    }

    return result.is_ok ? 1 : -1;
}

/**
 * @brief Appends text to a bounded buffer, counting all of it.
 *
 * @param[out]    _buffer    the buffer; may be null when _capacity is 0.
 * @param[in]     _capacity  its size.
 * @param[in,out] _length    the text's length so far.
 * @param[in]     _text      what to append.
 * @param[in]     _size      its length.
 */
static void
d_internal_option_env_registry_append(
    char*       _buffer,
    size_t      _capacity,
    size_t*     _length,
    const char* _text,
    size_t      _size
)
{
    for (size_t i = 0u; i < _size; ++i)
    {
        if (*_length + 1u < _capacity)
        {
            _buffer[*_length] = _text[i];
        }

        ++*_length;
    }

    return;
}

/**
 * @brief The djin whose name is nearest a text, within the distance a
 *        suggestion may be.
 *
 * @param[in] _registry  the registry.
 * @param[in] _text      the text.
 * @param[in] _size      its length.
 * @return the djin, or null.
 */
static struct dj_option_env*
d_internal_option_env_registry_nearest(
    const struct dj_option_env_registry* _registry,
    const char*                          _text,
    size_t                               _size
)
{
    const uint32_t        limit = (_size <= 3u) ? 1u : 2u;
    struct dj_option_env* best  = NULL;
    uint32_t              most  = limit + 1u;

    for (uint32_t i = 0u; i < _registry->djin_count; ++i)
    {
        const uint32_t distance =
            d_option_env_distance(_registry->djins[i]->name, _text, _size);

        if (distance < most)
        {
            best = _registry->djins[i];
            most = distance;
        }
    }

    return best;
}

/**
 * @brief A suggestion for a name inside one frame: the frame's suggestion
 *        put in place of the part that failed, the scope or the variable.
 *
 * @param[in]     _frame     the frame.
 * @param[in]     _name      the name, its frame's part taken off.
 * @param[in]     _size      its length.
 * @param[out]    _buffer    as d_option_env_registry_suggest.
 * @param[in]     _capacity  its size.
 * @param[in,out] _length    the suggestion's length so far.
 * @return 1, or 0 when the frame has nothing near.
 */
static int
d_internal_option_env_registry_within(
    const struct dj_option_env* _frame,
    const char*                 _name,
    size_t                      _size,
    char*                       _buffer,
    size_t                      _capacity,
    size_t*                     _length
)
{
    const char* const near = d_option_env_suggest(_frame, _name, _size);
    const char* const dot  = (const char*)memchr(_name, '.', _size);

    if (near == NULL)
    {
        return 0;
    }

    // a scope that fails is replaced, and the variable after it kept
    if ( (dot != NULL) &&
         (!d_option_env_find_scope(_frame,
                                   _name,
                                   (size_t)(dot - _name)).is_ok) )
    {
        d_internal_option_env_registry_append(_buffer, _capacity, _length,
                                              near, strlen(near));
        d_internal_option_env_registry_append(_buffer, _capacity, _length,
                                              dot,
                                              _size - (size_t)(dot - _name));

        return 1;
    }

    if (dot != NULL)
    {
        d_internal_option_env_registry_append(_buffer, _capacity, _length,
                                              _name,
                                              (size_t)(dot - _name) + 1u);
    }

    d_internal_option_env_registry_append(_buffer, _capacity, _length, near,
                                          strlen(near));

    return 1;
}

/*
d_option_env_registry_suggest
  The djin part first: a djin named exactly, or the nearest; then the rest
inside it. A name with no near djin is tried as the root's.
*/
size_t
d_option_env_registry_suggest(
    struct dj_option_env_registry* _registry,
    const char*                    _name,
    size_t                         _size,
    char*                          _buffer,
    size_t                         _capacity
)
{
    size_t length = 0u;

    if ( (_registry == NULL) ||
         (_name == NULL)     ||
         ( (_buffer == NULL) &&
           (_capacity != 0u) ) )
    {
        return 0u;
    }

    const char* const     dot  = (const char*)memchr(_name, '.', _size);
    const size_t          head = (dot != NULL) ? (size_t)(dot - _name) : 0u;
    struct dj_option_env* djin =
        (dot != NULL) ? d_option_env_registry_find(_registry, _name, head)
                      : NULL;

    if ( (dot != NULL) &&
         (djin == NULL) &&
         (!d_option_env_find_scope(&_registry->frame, _name, head).is_ok) )
    {
        djin = d_internal_option_env_registry_nearest(_registry, _name, head);
    }

    if (djin != NULL)
    {
        const char* const rest = dot + 1;
        const size_t      size = _size - head - 1u;

        d_internal_option_env_registry_append(_buffer, _capacity, &length,
                                              djin->name, strlen(djin->name));
        d_internal_option_env_registry_append(_buffer, _capacity, &length,
                                              ".", 1u);

        uint32_t scope = D_OPTION_ENV_GLOBAL;

        // the rest may be right already, when only the djin was wrong
        if (d_option_env_resolve(djin, rest, size, &scope).is_ok)
        {
            d_internal_option_env_registry_append(_buffer, _capacity,
                                                  &length, rest, size);
        }
        else if (d_internal_option_env_registry_within(djin, rest, size,
                                                         _buffer, _capacity,
                                                         &length) == 0)
        {
            length = 0u;
        }
    }
    else if (d_internal_option_env_registry_within(&_registry->frame, _name,
                                                     _size, _buffer,
                                                     _capacity, &length) == 0)
    {
        length = 0u;
    }

    if (_capacity != 0u)
    {
        _buffer[(length < _capacity) ? length : (_capacity - 1u)] = '\0';
    }

    return length;
}


#endif  // RE_STD_HAS_INT64
