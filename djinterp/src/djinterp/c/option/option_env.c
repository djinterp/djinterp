/*******************************************************************************
* djinterp [c]                                                      option_env.c
*
*   An abstract machine's environment (option_env.h): frames of bindings over
* the option core, reading through scopes, sources and parents to the
* schema's defaults, and binding by value, by text, by script and from
* getenv.
*
*   A BINDING IS A CELL, AND THE CORE DOES THE WORK. Adding, finding,
* writing, removing and compacting a binding are d_option_set_add, _find,
* _set, _remove and _compact; what this unit adds is the key -- (scope <<
* 32) | (source << 28) | variable -- and the text arena, which the core has
* no call for. Text set as text lives in the arena, and since neither
* unbinding nor rebinding gives bytes back, the arena is compacted when it
* fills: every live string slides down in address order, and the generation
* moves so a machine holding pointers into the arena knows.
*
*   A VARIABLE'S ID NAMES ITS FRAME. Bits 20 to 27 are the declaring frame's
* depth and the bits below its index there, so an id is resolved by walking
* up to the frame at that depth, and a frame's declarations can grow while
* frames beneath it hold bindings of theirs.
*
*   TEXT IS READ IN THE C LOCALE'S FORM. Integers are read here, digit by
* digit, so a value means the same everywhere; a real is checked to be in C's
* form and then read by strtod with the locale's decimal point put in, and
* written by snprintf with it taken out.
*
*
* path:      /src/djinterp/c/option/option_env.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.11.04
*******************************************************************************/
#include "../../../../inc/djinterp/c/option/option_env.h"  // corresponding header
// std
#include <errno.h>   // errno, ERANGE
#include <locale.h>  // localeconv, struct lconv
#include <math.h>    // HUGE_VAL
#include <stdio.h>   // snprintf
#include <stdlib.h>  // getenv, strtod
#include <string.h>  // memchr, memcpy, memmove, strlen, strncmp
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"                  // bool
#include "../../../../inc/djinterp/c/meta/type_info.h"            // D_TYPE_*
#include "../../../../inc/djinterp/c/option/option_common.h"      // d_option_*
#include "../../../../inc/djinterp/c/option/option_set_common.h"  // the set
// re_std
#include "../../../../inc/re_std/c/cstdint/dstdint.h"  // int64_t, uint64_t,
                                                       // uintptr_t, INT64_MAX


#if RE_STD_HAS_INT64

// D_INTERNAL_OPTION_ENV_DEPTH_SHIFT
//   constant: where an id's depth starts.
#define D_INTERNAL_OPTION_ENV_DEPTH_SHIFT  20u

// D_INTERNAL_OPTION_ENV_LOCAL
//   constant: the mask of an id's index in its frame.
#define D_INTERNAL_OPTION_ENV_LOCAL        0x000FFFFFu

// D_INTERNAL_OPTION_ENV_ID
//   constant: the mask of a whole variable id: depth and index, 28 bits.
#define D_INTERNAL_OPTION_ENV_ID           0x0FFFFFFFu

// D_INTERNAL_OPTION_ENV_MAKE_ID
//   macro: an id from a frame's depth and an index in it.
#define D_INTERNAL_OPTION_ENV_MAKE_ID(depth, index)                            \
    ((((uint32_t)(depth)) << D_INTERNAL_OPTION_ENV_DEPTH_SHIFT) |              \
     ((uint32_t)(index)))

// D_INTERNAL_OPTION_ENV_KEY
//   macro: a binding's key: its scope in the high half; its source, then its
// variable's id, in the low.
#define D_INTERNAL_OPTION_ENV_KEY(scope, source, var)                          \
    ((((uint64_t)(scope)) << 32) |                                             \
     (((uint64_t)(source)) << 28) |                                            \
     ((uint64_t)(var)))

// D_INTERNAL_OPTION_ENV_KEY_VAR / _SCOPE / _SOURCE
//   macro: a key's parts.
#define D_INTERNAL_OPTION_ENV_KEY_VAR(key)                                     \
    ((uint32_t)((key) & D_INTERNAL_OPTION_ENV_ID))
#define D_INTERNAL_OPTION_ENV_KEY_SCOPE(key)                                   \
    ((uint32_t)((key) >> 32))
#define D_INTERNAL_OPTION_ENV_KEY_SOURCE(key)                                  \
    ((int32_t)(((key) >> 28) & 0x0Fu))

// D_INTERNAL_OPTION_ENV_SCRATCH
//   constant: the bytes a quoted value that is not TEXT, a real's text or an
// environment variable's name is read into.
#define D_INTERNAL_OPTION_ENV_SCRATCH  256

// D_INTERNAL_OPTION_ENV_NAME_MAX
//   constant: the longest name d_option_env_suggest measures.
#define D_INTERNAL_OPTION_ENV_NAME_MAX  63u

// KIND_NAMES
//   constant: the kinds' names, by value.
static const char* const KIND_NAMES[] =
{
    "unknown", "bool", "int", "real", "text", "choice", "flags"
};

// SOURCE_NAMES
//   constant: the sources' names, by value.
static const char* const SOURCE_NAMES[] =
{
    "default", "program", "file", "environment", "command line", "override"
};


//==============================================================================
// SMALL THINGS
//==============================================================================

/**
 * @brief Folds an ASCII letter to lower case, whatever the locale.
 *
 * @param[in] _c  the character.
 * @return the lower-case letter, or _c itself.
 */
static char
d_internal_option_env_lower(
    char _c
)
{
    return ( (_c >= 'A') &&
             (_c <= 'Z') ) ? (char)(_c - 'A' + 'a') : _c;
}

/**
 * @brief Whether a character is ASCII white space.
 *
 * @param[in] _c  the character.
 * @return 1 or 0.
 */
static int
d_internal_option_env_is_space(
    char _c
)
{
    return ( (_c == ' ')  ||
             (_c == '\t') ||
             (_c == '\r') ||
             (_c == '\n') ||
             (_c == '\f') ||
             (_c == '\v') ) ? 1 : 0;
}

/**
 * @brief Whether a character may be in a name.
 *
 * @param[in] _c  the character.
 * @return 1 or 0.
 */
static int
d_internal_option_env_is_name_char(
    char _c
)
{
    return ( ((_c >= 'a') && (_c <= 'z')) ||
             ((_c >= 'A') && (_c <= 'Z')) ||
             ((_c >= '0') && (_c <= '9')) ||
             (_c == '_') ) ? 1 : 0;
}

/**
 * @brief Whether a NUL-terminated text is an identifier: not empty, not
 *        starting with a digit, name characters only.
 *
 * @param[in] _text  the text; may be null.
 * @return 1 or 0.
 */
static int
d_internal_option_env_is_identifier(
    const char* _text
)
{
    if ( (_text == NULL)      ||
         (_text[0] == '\0')   ||
         ( (_text[0] >= '0') &&
           (_text[0] <= '9') ) )
    {
        return 0;
    }

    for (; *_text != '\0'; ++_text)
    {
        if (d_internal_option_env_is_name_char(*_text) == 0)
        {
            return 0;
        }
    }

    return 1;
}

/**
 * @brief Whether a NUL-terminated name equals a counted one, exactly or
 *        ignoring ASCII case.
 *
 * @param[in] _name    the NUL-terminated name; may be null, which equals
 *                     nothing.
 * @param[in] _text    the counted one.
 * @param[in] _size    its length.
 * @param[in] _folded  1 to ignore case.
 * @return 1 or 0.
 */
static int
d_internal_option_env_equal(
    const char* _name,
    const char* _text,
    size_t      _size,
    int         _folded
)
{
    size_t i = 0u;

    if (_name == NULL)
    {
        return 0;
    }

    for (; i < _size; ++i)
    {
        if ( (_name[i] == '\0') ||
             ( (_folded != 0)
                   ? (d_internal_option_env_lower(_name[i]) !=
                      d_internal_option_env_lower(_text[i]))
                   : (_name[i] != _text[i]) ) )
        {
            return 0;
        }
    }

    return (_name[i] == '\0') ? 1 : 0;
}

/**
 * @brief Trims ASCII white space from both ends of a counted text.
 *
 * @param[in,out] _text  the text; moved past leading space.
 * @param[in,out] _size  its length; shortened.
 */
static void
d_internal_option_env_trim(
    const char** _text,
    size_t*      _size
)
{
    while ( (*_size > 0u) &&
            (d_internal_option_env_is_space(**_text) != 0) )
    {
        ++*_text;
        --*_size;
    }

    while ( (*_size > 0u) &&
            (d_internal_option_env_is_space((*_text)[*_size - 1u]) != 0) )
    {
        --*_size;
    }

    return;
}

/**
 * @brief A frame's ancestor at a depth, or the frame itself.
 *
 * @param[in] _env    the frame.
 * @param[in] _depth  the depth.
 * @return the frame, or null for a depth below the frame's.
 */
static const struct dj_option_env*
d_internal_option_env_at_depth(
    const struct dj_option_env* _env,
    uint32_t                    _depth
)
{
    if (_depth > _env->depth)
    {
        return NULL;
    }

    for (uint32_t up = _env->depth - _depth; up > 0u; --up)
    {
        _env = _env->parent;
    }

    return _env;
}

/**
 * @brief A variable's declaration by id, from a frame that can see it.
 *
 * @param[in] _env  the frame.
 * @param[in] _var  the id.
 * @return the declaration, or null.
 */
static const struct dj_option_env_var*
d_internal_option_env_decl(
    const struct dj_option_env* _env,
    uint32_t                    _var
)
{
    if ((_var & ~D_INTERNAL_OPTION_ENV_ID) != 0u)
    {
        return NULL;
    }

    const struct dj_option_env* const frame =
        d_internal_option_env_at_depth(
            _env,
            _var >> D_INTERNAL_OPTION_ENV_DEPTH_SHIFT);
    const uint32_t                    index = _var &
                                              D_INTERNAL_OPTION_ENV_LOCAL;

    return ( (frame != NULL) &&
             (index < frame->var_count) ) ? frame->vars[index] : NULL;
}

/**
 * @brief Whether a frame can see a scope: the global one, or one an
 *        ancestor or the frame itself named.
 *
 * @param[in] _env    the frame.
 * @param[in] _scope  the scope's id.
 * @return 1 or 0.
 */
static int
d_internal_option_env_scope_ok(
    const struct dj_option_env* _env,
    uint32_t                    _scope
)
{
    if (_scope == D_OPTION_ENV_GLOBAL)
    {
        return 1;
    }

    const struct dj_option_env* const frame =
        d_internal_option_env_at_depth(
            _env,
            _scope >> D_INTERNAL_OPTION_ENV_DEPTH_SHIFT);
    const uint32_t                    index = _scope &
                                              D_INTERNAL_OPTION_ENV_LOCAL;

    return ( (frame != NULL)               &&
             (index >= 1u)                 &&
             (index <= frame->scope_count) &&
             ((_scope >> D_INTERNAL_OPTION_ENV_DEPTH_SHIFT) <=
              D_OPTION_ENV_DEPTH_MAX) ) ? 1 : 0;
}

/**
 * @brief Checks a scope and a variable, and gives the declaration.
 *
 * @param[in]  _env    the frame; may be null.
 * @param[in]  _scope  the scope.
 * @param[in]  _var    the variable.
 * @param[out] _decl   its declaration.
 * @return D_OPTION_STATUS_OK, INVALID_ARGUMENT or KEY_ABSENT.
 */
static int32_t
d_internal_option_env_check(
    const struct dj_option_env*      _env,
    uint32_t                         _scope,
    uint32_t                         _var,
    const struct dj_option_env_var** _decl
)
{
    if (_env == NULL)
    {
        return (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT;
    }

    *_decl = d_internal_option_env_decl(_env, _var);

    if ( (*_decl == NULL) ||
         (d_internal_option_env_scope_ok(_env, _scope) == 0) )
    {
        return (int32_t)D_OPTION_STATUS_KEY_ABSENT;
    }

    return (int32_t)D_OPTION_STATUS_OK;
}

/**
 * @brief The bytes a kind's binding takes, which is also its alignment.
 *
 * @param[in] _kind  the kind.
 * @return the size.
 */
static uint32_t
d_internal_option_env_size(
    int32_t _kind
)
{
    switch (_kind)
    {
        case D_OPTION_ENV_KIND_BOOL:
            return (uint32_t)sizeof(bool);
        case D_OPTION_ENV_KIND_REAL:
            return (uint32_t)sizeof(double);
        case D_OPTION_ENV_KIND_TEXT:
            return (uint32_t)sizeof(const char*);
        default:
            return (uint32_t)sizeof(int64_t);
    }
}

/**
 * @brief The type descriptor of a kind's binding.
 *
 * @param[in] _kind  the kind.
 * @return the descriptor.
 */
static dj_type_info16
d_internal_option_env_type(
    int32_t _kind
)
{
    switch (_kind)
    {
        case D_OPTION_ENV_KIND_BOOL:
            return (dj_type_info16)D_TYPE_BOOL_();
        case D_OPTION_ENV_KIND_REAL:
            return (dj_type_info16)D_TYPE_DOUBLE_();
        case D_OPTION_ENV_KIND_TEXT:
            return (dj_type_info16)D_TYPE_MAKE_PTR(
                       D_TYPE_SET_CONST(D_TYPE_CHAR_()), 1);
        default:
            return (dj_type_info16)D_TYPE_LLONG_();
    }
}

/**
 * @brief The kind of the variable a frame's cell binds.
 *
 * @param[in] _env   the frame that holds the cell.
 * @param[in] _cell  the cell.
 * @return the kind, or 0 for a variable it cannot see.
 */
static int32_t
d_internal_option_env_cell_kind(
    const struct dj_option_env* _env,
    const struct dj_option*     _cell
)
{
    const struct dj_option_env_var* const decl =
        d_internal_option_env_decl(_env,
                                   D_INTERNAL_OPTION_ENV_KEY_VAR(_cell->key));

    return (decl != NULL) ? decl->kind : 0;
}

/**
 * @brief The text a TEXT cell holds.
 *
 * @param[in] _env   the frame.
 * @param[in] _cell  the cell.
 * @return the pointer stored in it.
 */
static const char*
d_internal_option_env_cell_text(
    const struct dj_option_env* _env,
    const struct dj_option*     _cell
)
{
    const char* text = NULL;

    memcpy(&text, _env->set.values + _cell->value_offset, sizeof(text));

    return text;
}

/**
 * @brief Whether a source is one a binding can have: PROGRAM to OVERRIDE.
 *
 * @param[in] _source  the source.
 * @return 1 or 0.
 */
static int
d_internal_option_env_source_ok(
    int32_t _source
)
{
    return ( (_source > D_OPTION_ENV_SOURCE_DEFAULT) &&
             (_source < D_OPTION_ENV_SOURCES) ) ? 1 : 0;
}


//==============================================================================
// THE TEXT ARENA
//==============================================================================

/**
 * @brief Slides every live string in the arena down to its start, and
 *        follows one pointer the caller needs kept.
 *
 * @param[in,out] _env    the frame.
 * @param[in,out] _track  a pointer into the arena to follow, or null; set to
 *                        null when it is in no live string.
 *
 * The live strings are the frame's TEXT bindings that point into the arena,
 * moved in address order so each moves down. Two bindings may share a string;
 * both are pointed at its new place.
 */
static void
d_internal_option_env_compact_text(
    struct dj_option_env* _env,
    const char**          _track
)
{
    const uintptr_t low   = (uintptr_t)_env->text;
    const uintptr_t high  = low + _env->text_used;
    uintptr_t       done  = 0u;
    int             first = 1;
    size_t          mark  = 0u;
    int             kept  = 0;

    for (;;)
    {
        const char* next = NULL;

        // the live string with the lowest address not yet moved
        for (uint32_t i = 0u; i < _env->set.count; ++i)
        {
            const struct dj_option* const cell = &_env->set.options[i];
            const char* const             text =
                (d_internal_option_env_cell_kind(_env, cell) ==
                 D_OPTION_ENV_KIND_TEXT)
                    ? d_internal_option_env_cell_text(_env, cell)
                    : NULL;
            const uintptr_t               at   = (uintptr_t)text;

            if ( (text != NULL)                  &&
                 (at >= low)                     &&
                 (at < high)                     &&
                 ( (first != 0) || (at > done) ) &&
                 ( (next == NULL) ||
                   (at < (uintptr_t)next) ) )
            {
                next = text;
            }
        }

        if (next == NULL)
        {
            break;
        }

        const size_t length = strlen(next) + 1u;
        char* const  place  = _env->text + mark;

        // the tracked pointer moves with the string it is in
        if ( (_track != NULL)                           &&
             (*_track != NULL)                          &&
             ((uintptr_t)*_track >= (uintptr_t)next)    &&
             ((uintptr_t)*_track < (uintptr_t)next + length) )
        {
            *_track = place + ((uintptr_t)*_track - (uintptr_t)next);
            kept    = 1;
        }

        memmove(place, next, length);

        for (uint32_t i = 0u; i < _env->set.count; ++i)
        {
            const struct dj_option* const cell = &_env->set.options[i];

            if ( (d_internal_option_env_cell_kind(_env, cell) ==
                  D_OPTION_ENV_KIND_TEXT) &&
                 (d_internal_option_env_cell_text(_env, cell) == next) )
            {
                memcpy(_env->set.values + cell->value_offset,
                       &place,
                       sizeof(place));
            }
        }

        done  = (uintptr_t)next;
        first = 0;
        mark += length;
    }

    if ( (_track != NULL)            &&
         (*_track != NULL)           &&
         (kept == 0)                 &&
         ((uintptr_t)*_track >= low) &&
         ((uintptr_t)*_track < high) )
    {
        *_track = NULL;
    }

    _env->text_used = mark;
    ++_env->generation;

    return;
}

/**
 * @brief Room in the arena for some text and its NUL, compacting it first
 *        when it is full.
 *
 * @param[in,out] _env    the frame.
 * @param[in]     _size   the text's length.
 * @param[in,out] _track  as d_internal_option_env_compact_text.
 * @return the room, or null when there is none.
 */
static char*
d_internal_option_env_alloc(
    struct dj_option_env* _env,
    size_t                _size,
    const char**          _track
)
{
    if (_env->text_capacity - _env->text_used <= _size)
    {
        d_internal_option_env_compact_text(_env, _track);

        if (_env->text_capacity - _env->text_used <= _size)
        {
            return NULL;
        }
    }

    char* const room = _env->text + _env->text_used;

    _env->text_used += _size + 1u;

    return room;
}


//==============================================================================
// SETTING UP
//==============================================================================

/*
d_option_env_init
  The bindings are an option set over the caller's two blocks; nothing else
needs more than zeroing.
*/
struct dj_option_result
d_option_env_init(
    struct dj_option_env*             _env,
    struct dj_option*                 _options,
    unsigned char*                    _values,
    uint32_t                          _binding_capacity,
    uint32_t                          _value_capacity,
    const struct dj_option_env_var**  _vars,
    uint32_t                          _var_capacity,
    char*                             _text,
    size_t                            _text_capacity
)
{
    if ( (_env == NULL)                         ||
         ( (_binding_capacity != 0u) &&
           ( (_options == NULL) ||
             (_values == NULL) ) )              ||
         ( (_var_capacity != 0u) &&
           (_vars == NULL) )                    ||
         ( (_text_capacity != 0u) &&
           (_text == NULL) ) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    _env->set           = d_option_set_view(_options,
                                            _values,
                                            _binding_capacity,
                                            _value_capacity);
    _env->parent        = NULL;
    _env->vars          = _vars;
    _env->scopes        = NULL;
    _env->name          = NULL;
    _env->text          = _text;
    _env->text_capacity = _text_capacity;
    _env->text_used     = 0u;
    _env->var_count     = 0u;
    _env->var_capacity  = (_var_capacity > D_INTERNAL_OPTION_ENV_LOCAL)
                              ? D_INTERNAL_OPTION_ENV_LOCAL
                              : _var_capacity;
    _env->scope_count   = 0u;
    _env->generation    = 0u;
    _env->depth         = 0u;
    _env->child_count   = 0u;

    return d_option_ok(0u);
}

/**
 * @brief Whether a declaration is well formed: an identifier for a name and
 *        any alias, a known kind, names for a choice or flags, and a default
 *        the row itself accepts.
 *
 * @param[in] _var  the declaration.
 * @return 1 or 0.
 */
static int
d_internal_option_env_well_formed(
    const struct dj_option_env_var* _var
)
{
    if ( (d_internal_option_env_is_identifier(_var->name) == 0)     ||
         ( (_var->alias != NULL) &&
           (d_internal_option_env_is_identifier(_var->alias) == 0) ) ||
         (_var->kind < D_OPTION_ENV_KIND_BOOL)                       ||
         (_var->kind > D_OPTION_ENV_KIND_FLAGS)                      ||
         ( (_var->unbuilt == NULL) &&
           (_var->unbuilt_count != 0u) ) )
    {
        return 0;
    }

    switch (_var->kind)
    {
        case D_OPTION_ENV_KIND_BOOL:
            return ( (_var->integer == 0) ||
                     (_var->integer == 1) ) ? 1 : 0;
        case D_OPTION_ENV_KIND_INT:
            return ( (_var->minimum <= _var->integer) &&
                     (_var->integer <= _var->maximum) ) ? 1 : 0;
        case D_OPTION_ENV_KIND_CHOICE:
        {
            uint32_t i = 0u;

            while ( (_var->names != NULL)     &&
                    (i < _var->name_count)    &&
                    (_var->names[i].value != _var->integer) )
            {
                ++i;
            }

            return ( (_var->names != NULL) &&
                     (i < _var->name_count) ) ? 1 : 0;
        }
        case D_OPTION_ENV_KIND_FLAGS:
        {
            int64_t mask = 0;

            for (uint32_t i = 0u; ( (_var->names != NULL) &&
                                    (i < _var->name_count) ); ++i)
            {
                mask |= _var->names[i].value;
            }

            return ( (_var->names != NULL)     &&
                     (_var->name_count != 0u)  &&
                     ((_var->integer & ~mask) == 0) ) ? 1 : 0;
        }
        default:
            return 1;
    }
}

/**
 * @brief Whether a frame itself declares a name, as a name or an alias.
 *
 * @param[in] _env   the frame.
 * @param[in] _name  the name, NUL-terminated; may be null.
 * @return 1 or 0.
 */
static int
d_internal_option_env_declares(
    const struct dj_option_env* _env,
    const char*                 _name
)
{
    const size_t size = (_name != NULL) ? strlen(_name) : 0u;

    for (uint32_t i = 0u; ( (_name != NULL) && (i < _env->var_count) ); ++i)
    {
        if ( (d_internal_option_env_equal(_env->vars[i]->name,
                                          _name,
                                          size,
                                          0) != 0) ||
             (d_internal_option_env_equal(_env->vars[i]->alias,
                                          _name,
                                          size,
                                          0) != 0) )
        {
            return 1;
        }
    }

    return 0;
}

/*
d_option_env_declare
  A declaration is checked whole before it is kept, and its default against
itself: a default the row would refuse is a declaration's bug, found here
rather than at the first read.
*/
struct dj_option_result
d_option_env_declare(
    struct dj_option_env*           _env,
    const struct dj_option_env_var* _var
)
{
    if ( (_env == NULL) ||
         (_var == NULL) ||
         (d_internal_option_env_well_formed(_var) == 0) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    if ( (d_internal_option_env_declares(_env, _var->name) != 0) ||
         (d_internal_option_env_declares(_env, _var->alias) != 0) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_KEY_DUPLICATE);
    }

    if (_env->var_count >= _env->var_capacity)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_CAPACITY);
    }

    _env->vars[_env->var_count] = _var;
    ++_env->generation;

    return d_option_ok(D_INTERNAL_OPTION_ENV_MAKE_ID(_env->depth,
                                                     _env->var_count++));
}

/*
d_option_env_declare_all
  Row by row, stopping at the first refusal.
*/
struct dj_option_result
d_option_env_declare_all(
    struct dj_option_env*           _env,
    const struct dj_option_env_var* _schema,
    uint32_t                        _count
)
{
    if ( (_env == NULL) ||
         ( (_schema == NULL) &&
           (_count != 0u) ) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    const uint32_t first = D_INTERNAL_OPTION_ENV_MAKE_ID(_env->depth,
                                                         _env->var_count);

    for (uint32_t i = 0u; i < _count; ++i)
    {
        const struct dj_option_result result =
            d_option_env_declare(_env, &_schema[i]);

        if (!result.is_ok)
        {
            return result;
        }
    }

    return d_option_ok(first);
}

/*
d_option_env_set_scopes
  Borrowed, and checked only for the frame and the count: a scope's name is
matched as it is written.
*/
struct dj_option_result
d_option_env_set_scopes(
    struct dj_option_env* _env,
    const char* const*    _names,
    uint32_t              _count
)
{
    if ( (_env == NULL)                        ||
         (_count > D_INTERNAL_OPTION_ENV_LOCAL) ||
         ( (_names == NULL) &&
           (_count != 0u) ) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    _env->scopes      = _names;
    _env->scope_count = _count;
    ++_env->generation;

    return d_option_ok(D_INTERNAL_OPTION_ENV_MAKE_ID(_env->depth, 1u));
}

/*
d_option_env_set_parent
  A frame with children is refused: their ids are built on its depth. The
frame's own bindings go -- their ids may name the old chain's variables --
and its generation is set above the old chain's sum, so every sum read from
the new chain is above any a cache could hold.
*/
struct dj_option_result
d_option_env_set_parent(
    struct dj_option_env* _env,
    struct dj_option_env* _parent
)
{
    if ( (_env == NULL) ||
         (_env->child_count != 0u) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    for (const struct dj_option_env* f = _parent; f != NULL; f = f->parent)
    {
        if (f == _env)
        {
            return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
        }
    }

    if ( (_parent != NULL) &&
         (_parent->depth >= D_OPTION_ENV_DEPTH_MAX) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_CAPACITY);
    }

    const uint32_t before = d_option_env_generation(_env);

    if (_env->parent != NULL)
    {
        --_env->parent->child_count;
    }

    if (_parent != NULL)
    {
        ++_parent->child_count;
    }

    d_option_set_clear(&_env->set);
    _env->text_used  = 0u;
    _env->parent     = _parent;
    _env->depth      = (_parent != NULL) ? (_parent->depth + 1u) : 0u;
    _env->generation = before + 1u;

    return d_option_ok(0u);
}

/*
d_option_env_set_name
  Borrowed; an identifier, so a full name -- "djin.variable" -- can be read
back.
*/
struct dj_option_result
d_option_env_set_name(
    struct dj_option_env* _env,
    const char*           _name
)
{
    if ( (_env == NULL) ||
         ( (_name != NULL) &&
           (d_internal_option_env_is_identifier(_name) == 0) ) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    _env->name = _name;
    ++_env->generation;

    return d_option_ok(0u);
}

/*
d_option_env_clear
  The bindings and the arena both: nothing in the arena is live without a
binding.
*/
void
d_option_env_clear(
    struct dj_option_env* _env
)
{
    if (_env != NULL)
    {
        d_option_set_clear(&_env->set);
        _env->text_used = 0u;
        ++_env->generation;
    }

    return;
}

/*
d_option_env_detach
  set_parent with no parent; a frame with children of its own stays.
*/
void
d_option_env_detach(
    struct dj_option_env* _env
)
{
    if ( (_env != NULL) &&
         (_env->parent != NULL) )
    {
        (void)d_option_env_set_parent(_env, NULL);
    }

    return;
}

/*
d_option_env_generation
  The sum along the chain: any frame's change moves it, and sums of counters
that only rise never come back to a value they had.
*/
uint32_t
d_option_env_generation(
    const struct dj_option_env* _env
)
{
    uint32_t generation = 0u;

    for (; _env != NULL; _env = _env->parent)
    {
        generation += _env->generation;
    }

    return generation;
}

/*
d_option_env_root
  The frame with no parent.
*/
const struct dj_option_env*
d_option_env_root(
    const struct dj_option_env* _env
)
{
    if (_env == NULL)
    {
        return NULL;
    }

    while (_env->parent != NULL)
    {
        _env = _env->parent;
    }

    return _env;
}

/*
d_option_env_kind_name
  A table by value.
*/
const char*
d_option_env_kind_name(
    int32_t _kind
)
{
    if ( (_kind < D_OPTION_ENV_KIND_BOOL) ||
         (_kind > D_OPTION_ENV_KIND_FLAGS) )
    {
        return KIND_NAMES[0];
    }

    return KIND_NAMES[_kind];
}

/*
d_option_env_source_name
  A table by value.
*/
const char*
d_option_env_source_name(
    int32_t _source
)
{
    if ( (_source < D_OPTION_ENV_SOURCE_DEFAULT) ||
         (_source >= D_OPTION_ENV_SOURCES) )
    {
        return "unknown";
    }

    return SOURCE_NAMES[_source];
}


//==============================================================================
// FINDING
//==============================================================================

/*
d_option_env_var_count
  Every frame's own count, along the chain.
*/
uint32_t
d_option_env_var_count(
    const struct dj_option_env* _env
)
{
    uint32_t count = 0u;

    for (; _env != NULL; _env = _env->parent)
    {
        count += _env->var_count;
    }

    return count;
}

/*
d_option_env_var_id
  Counted from the root down, found from the frame up: each frame's own
variables are the last of those it can see.
*/
uint32_t
d_option_env_var_id(
    const struct dj_option_env* _env,
    uint32_t                    _index
)
{
    uint32_t total = d_option_env_var_count(_env);

    for (; _env != NULL; _env = _env->parent)
    {
        const uint32_t base = total - _env->var_count;

        if (_index >= total)
        {
            return D_OPTION_ENV_NONE;
        }

        if (_index >= base)
        {
            return D_INTERNAL_OPTION_ENV_MAKE_ID(_env->depth, _index - base);
        }

        total = base;
    }

    return D_OPTION_ENV_NONE;
}

/*
d_option_env_var_index
  The declaring frame's own index, after every variable its ancestors have.
*/
uint32_t
d_option_env_var_index(
    const struct dj_option_env* _env,
    uint32_t                    _var
)
{
    if ( (_env == NULL) ||
         (d_internal_option_env_decl(_env, _var) == NULL) )
    {
        return D_OPTION_ENV_NONE;
    }

    const struct dj_option_env* const frame =
        d_internal_option_env_at_depth(
            _env,
            _var >> D_INTERNAL_OPTION_ENV_DEPTH_SHIFT);

    return d_option_env_var_count(frame->parent) +
           (_var & D_INTERNAL_OPTION_ENV_LOCAL);
}

/*
d_option_env_var_at
  The declaring frame's row.
*/
const struct dj_option_env_var*
d_option_env_var_at(
    const struct dj_option_env* _env,
    uint32_t                    _var
)
{
    return (_env != NULL) ? d_internal_option_env_decl(_env, _var) : NULL;
}

/*
d_option_env_var_frame
  The ancestor at the id's depth, once the id is known to be good.
*/
const struct dj_option_env*
d_option_env_var_frame(
    const struct dj_option_env* _env,
    uint32_t                    _var
)
{
    if (d_option_env_var_at(_env, _var) == NULL)
    {
        return NULL;
    }

    return d_internal_option_env_at_depth(
               _env,
               _var >> D_INTERNAL_OPTION_ENV_DEPTH_SHIFT);
}

/*
d_option_env_scope_name
  The declaring frame's name for it.
*/
const char*
d_option_env_scope_name(
    const struct dj_option_env* _env,
    uint32_t                    _scope
)
{
    if ( (_env == NULL)                      ||
         (_scope == D_OPTION_ENV_GLOBAL)     ||
         (d_internal_option_env_scope_ok(_env, _scope) == 0) )
    {
        return NULL;
    }

    const struct dj_option_env* const frame =
        d_internal_option_env_at_depth(
            _env,
            _scope >> D_INTERNAL_OPTION_ENV_DEPTH_SHIFT);

    return frame->scopes[(_scope & D_INTERNAL_OPTION_ENV_LOCAL) - 1u];
}

/*
d_option_env_find
  Innermost frame first, by exact name, then alias: a frame's own name hides
an ancestor's. A linear search: names are found when text is read, not when
values are.
*/
struct dj_option_result
d_option_env_find(
    const struct dj_option_env* _env,
    const char*                 _name,
    size_t                      _size
)
{
    if ( (_env == NULL) ||
         (_name == NULL) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    for (const struct dj_option_env* f = _env; f != NULL; f = f->parent)
    {
        for (uint32_t i = 0u; i < f->var_count; ++i)
        {
            if ( (d_internal_option_env_equal(f->vars[i]->name,
                                              _name,
                                              _size,
                                              0) != 0) ||
                 (d_internal_option_env_equal(f->vars[i]->alias,
                                              _name,
                                              _size,
                                              0) != 0) )
            {
                return d_option_ok(D_INTERNAL_OPTION_ENV_MAKE_ID(f->depth,
                                                                 i));
            }
        }
    }

    return d_option_fail((int32_t)D_OPTION_STATUS_KEY_ABSENT);
}

/*
d_option_env_find_scope
  As d_option_env_find, over the frames' scope names.
*/
struct dj_option_result
d_option_env_find_scope(
    const struct dj_option_env* _env,
    const char*                 _name,
    size_t                      _size
)
{
    if ( (_env == NULL) ||
         (_name == NULL) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    for (const struct dj_option_env* f = _env; f != NULL; f = f->parent)
    {
        for (uint32_t i = 0u; i < f->scope_count; ++i)
        {
            if (d_internal_option_env_equal(f->scopes[i],
                                            _name,
                                            _size,
                                            0) != 0)
            {
                return d_option_ok(D_INTERNAL_OPTION_ENV_MAKE_ID(f->depth,
                                                                 i + 1u));
            }
        }
    }

    return d_option_fail((int32_t)D_OPTION_STATUS_KEY_ABSENT);
}

/**
 * @brief A name with the frame's own name and a '.' taken off its front,
 *        when it has them: "env_printer.style" in the frame env_printer.
 *
 * @param[in]     _env   the frame.
 * @param[in,out] _name  the name; moved past the qualifier.
 * @param[in,out] _size  its length; shortened.
 */
static void
d_internal_option_env_unqualify(
    const struct dj_option_env* _env,
    const char**                _name,
    size_t*                     _size
)
{
    const size_t own = (_env->name != NULL) ? strlen(_env->name) : 0u;

    if ( (own != 0u)                                    &&
         (*_size > own + 1u)                            &&
         ((*_name)[own] == '.')                         &&
         (memcmp(*_name, _env->name, own) == 0) )
    {
        *_name += own + 1u;
        *_size -= own + 1u;
    }

    return;
}

/*
d_option_env_resolve
  A frame's own name may lead; then a scope's, before the last '.'.
*/
struct dj_option_result
d_option_env_resolve(
    const struct dj_option_env* _env,
    const char*                 _name,
    size_t                      _size,
    uint32_t*                   _scope
)
{
    if ( (_env == NULL)  ||
         (_name == NULL) ||
         (_scope == NULL) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    d_internal_option_env_unqualify(_env, &_name, &_size);
    *_scope = D_OPTION_ENV_GLOBAL;

    const char* const dot = (const char*)memchr(_name, '.', _size);

    if (dot == NULL)
    {
        return d_option_env_find(_env, _name, _size);
    }

    const struct dj_option_result scope =
        d_option_env_find_scope(_env, _name, (size_t)(dot - _name));

    if (!scope.is_ok)
    {
        return scope;
    }

    *_scope = scope.payload.value;

    return d_option_env_find(_env,
                             dot + 1,
                             _size - (size_t)(dot - _name) - 1u);
}

/**
 * @brief The edit distance between a name and a counted text, ignoring
 *        ASCII case, a swap of two neighbours counting one, capped.
 *
 * @param[in] _name  the name, NUL-terminated.
 * @param[in] _text  the text.
 * @param[in] _size  its length.
 * @return the distance; 9 for either longer than 63 characters.
 *
 * The optimal string alignment distance, over three rows of a table rather
 * than all of it.
 */
static uint32_t
d_internal_option_env_distance(
    const char* _name,
    const char* _text,
    size_t      _size
)
{
    const size_t length = strlen(_name);
    uint32_t     rows[3][D_INTERNAL_OPTION_ENV_NAME_MAX + 2u];

    if ( (length > D_INTERNAL_OPTION_ENV_NAME_MAX) ||
         (_size > D_INTERNAL_OPTION_ENV_NAME_MAX) )
    {
        return 9u;
    }

    for (size_t j = 0u; j <= _size; ++j)
    {
        rows[1][j] = (uint32_t)j;
    }

    for (size_t i = 1u; i <= length; ++i)
    {
        uint32_t* const two  = rows[(i + 2u) % 3u];
        uint32_t* const one  = rows[i % 3u];
        uint32_t* const here = rows[(i + 1u) % 3u];
        const char      a    = d_internal_option_env_lower(_name[i - 1u]);

        here[0] = (uint32_t)i;

        for (size_t j = 1u; j <= _size; ++j)
        {
            const char     b    = d_internal_option_env_lower(_text[j - 1u]);
            const uint32_t cost = (a == b) ? 0u : 1u;
            uint32_t       best = one[j - 1u] + cost;

            best = (one[j] + 1u < best) ? (one[j] + 1u) : best;
            best = (here[j - 1u] + 1u < best) ? (here[j - 1u] + 1u) : best;

            // a swap of the two last characters of each
            if ( (i > 1u) &&
                 (j > 1u) &&
                 (a == d_internal_option_env_lower(_text[j - 2u])) &&
                 (d_internal_option_env_lower(_name[i - 2u]) == b) &&
                 (two[j - 2u] + 1u < best) )
            {
                best = two[j - 2u] + 1u;
            }

            here[j] = best;
        }
    }

    return rows[(length + 1u) % 3u][_size];
}

/**
 * @brief The nearest of a frame chain's variable names and aliases, or of
 *        its scope names, to a counted text.
 *
 * @param[in] _env     the frame.
 * @param[in] _text    the text.
 * @param[in] _size    its length.
 * @param[in] _scopes  1 for the scope names, 0 for the variables'.
 * @return the nearest name within the threshold, or null.
 */
static const char*
d_internal_option_env_nearest(
    const struct dj_option_env* _env,
    const char*                 _text,
    size_t                      _size,
    int                         _scopes
)
{
    const uint32_t limit = (_size <= 3u) ? 1u : 2u;
    const char*    best  = NULL;
    uint32_t       most  = limit + 1u;

    for (const struct dj_option_env* f = _env; f != NULL; f = f->parent)
    {
        const uint32_t count = (_scopes != 0) ? f->scope_count
                                              : (2u * f->var_count);

        for (uint32_t i = 0u; i < count; ++i)
        {
            const char* const name =
                (_scopes != 0)   ? f->scopes[i]
                : ((i % 2u) == 0u) ? f->vars[i / 2u]->name
                : f->vars[i / 2u]->alias;
            const uint32_t    distance =
                (name != NULL)
                    ? d_internal_option_env_distance(name, _text, _size)
                    : 9u;

            if (distance < most)
            {
                best = name;
                most = distance;
            }
        }
    }

    return best;
}

/*
d_option_env_distance
  The internal measure, for a caller that ranks names of its own.
*/
uint32_t
d_option_env_distance(
    const char* _name,
    const char* _text,
    size_t      _size
)
{
    if ( (_name == NULL) ||
         ( (_text == NULL) &&
           (_size != 0u) ) )
    {
        return 9u;
    }

    return d_internal_option_env_distance(_name, _text, _size);
}

/*
d_option_env_suggest
  The part of the name that fails is the one measured: the scope before a
'.', when it is unknown, else the variable.
*/
const char*
d_option_env_suggest(
    const struct dj_option_env* _env,
    const char*                 _name,
    size_t                      _size
)
{
    if ( (_env == NULL) ||
         (_name == NULL) )
    {
        return NULL;
    }

    d_internal_option_env_unqualify(_env, &_name, &_size);

    const char* const dot = (const char*)memchr(_name, '.', _size);

    if ( (dot != NULL) &&
         (!d_option_env_find_scope(_env,
                                   _name,
                                   (size_t)(dot - _name)).is_ok) )
    {
        return d_internal_option_env_nearest(_env,
                                             _name,
                                             (size_t)(dot - _name),
                                             1);
    }

    if (dot != NULL)
    {
        _size -= (size_t)(dot - _name) + 1u;
        _name  = dot + 1;
    }

    return d_internal_option_env_nearest(_env, _name, _size, 0);
}


//==============================================================================
// READING
//==============================================================================

/**
 * @brief Reads a cell into a value, by the variable's kind.
 *
 * @param[in]  _env    the frame that holds the cell.
 * @param[in]  _cell   the cell.
 * @param[out] _value  the value; its kind already set.
 */
static void
d_internal_option_env_read(
    const struct dj_option_env* _env,
    const struct dj_option*     _cell,
    struct dj_option_env_value* _value
)
{
    const unsigned char* const slot = _env->set.values + _cell->value_offset;

    switch (_value->kind)
    {
        case D_OPTION_ENV_KIND_BOOL:
        {
            bool flag = false;

            memcpy(&flag, slot, sizeof(flag));
            _value->integer = flag ? 1 : 0;
            break;
        }
        case D_OPTION_ENV_KIND_REAL:
            memcpy(&_value->real, slot, sizeof(_value->real));
            break;
        case D_OPTION_ENV_KIND_TEXT:
            memcpy(&_value->text, slot, sizeof(_value->text));
            break;
        default:
            memcpy(&_value->integer, slot, sizeof(_value->integer));
            break;
    }

    _value->bound  = 1;
    _value->source = D_INTERNAL_OPTION_ENV_KEY_SOURCE(_cell->key);
    _value->depth  = _env->depth;

    return;
}

/**
 * @brief Whether a cell bears on a variable in a scope: it binds the
 *        variable there, or globally.
 *
 * @param[in] _cell   the cell.
 * @param[in] _scope  the scope.
 * @param[in] _var    the variable.
 * @return 1 or 0.
 */
static int
d_internal_option_env_bears(
    const struct dj_option* _cell,
    uint32_t                _scope,
    uint32_t                _var
)
{
    const uint32_t scope = D_INTERNAL_OPTION_ENV_KEY_SCOPE(_cell->key);

    return ( (D_INTERNAL_OPTION_ENV_KEY_VAR(_cell->key) == _var) &&
             ( (scope == _scope) ||
               (scope == D_OPTION_ENV_GLOBAL) ) ) ? 1 : 0;
}

/**
 * @brief Whether one binding outranks another: higher standing; then, of
 *        equal standing, the deeper frame -- the inner one -- and in one
 *        frame the scoped binding.
 *
 * @param[in] _a        the one.
 * @param[in] _a_depth  its frame's depth.
 * @param[in] _b        the other.
 * @param[in] _b_depth  its frame's depth.
 * @return 1 when _a outranks _b.
 */
static int
d_internal_option_env_outranks(
    const struct dj_option* _a,
    uint32_t                _a_depth,
    const struct dj_option* _b,
    uint32_t                _b_depth
)
{
    const int32_t a_source = D_INTERNAL_OPTION_ENV_KEY_SOURCE(_a->key);
    const int32_t b_source = D_INTERNAL_OPTION_ENV_KEY_SOURCE(_b->key);

    if (a_source != b_source)
    {
        return (a_source > b_source) ? 1 : 0;
    }

    if (_a_depth != _b_depth)
    {
        return (_a_depth > _b_depth) ? 1 : 0;
    }

    return ( (D_INTERNAL_OPTION_ENV_KEY_SCOPE(_a->key) !=
              D_OPTION_ENV_GLOBAL) &&
             (D_INTERNAL_OPTION_ENV_KEY_SCOPE(_b->key) ==
              D_OPTION_ENV_GLOBAL) ) ? 1 : 0;
}

/**
 * @brief The binding in force for a variable in a scope, through the chain.
 *
 * @param[in]  _env    the frame.
 * @param[in]  _scope  the scope.
 * @param[in]  _var    the variable.
 * @param[out] _frame  the frame that holds it.
 * @return the cell, or null when nothing binds the variable there.
 */
static const struct dj_option*
d_internal_option_env_in_force(
    const struct dj_option_env*  _env,
    uint32_t                     _scope,
    uint32_t                     _var,
    const struct dj_option_env** _frame
)
{
    const struct dj_option* best = NULL;

    for (const struct dj_option_env* f = _env; f != NULL; f = f->parent)
    {
        for (uint32_t i = 0u; i < f->set.count; ++i)
        {
            const struct dj_option* const cell = &f->set.options[i];

            if ( (d_internal_option_env_bears(cell, _scope, _var) != 0) &&
                 ( (best == NULL) ||
                   (d_internal_option_env_outranks(cell,
                                                   f->depth,
                                                   best,
                                                   (*_frame)->depth) != 0) ) )
            {
                best    = cell;
                *_frame = f;
            }
        }
    }

    return best;
}

/**
 * @brief A variable's default as a value.
 *
 * @param[in]  _decl   the declaration.
 * @param[out] _value  the value.
 */
static void
d_internal_option_env_default(
    const struct dj_option_env_var* _decl,
    struct dj_option_env_value*     _value
)
{
    _value->text    = _decl->text;
    _value->integer = _decl->integer;
    _value->real    = _decl->real;
    _value->kind    = _decl->kind;
    _value->bound   = 0;
    _value->source  = D_OPTION_ENV_SOURCE_DEFAULT;
    _value->depth   = D_OPTION_ENV_NONE;

    return;
}

/**
 * @brief A choice's value with its name for text, where it has one.
 *
 * @param[in]     _decl   the declaration.
 * @param[in,out] _value  the value.
 */
static void
d_internal_option_env_name_choice(
    const struct dj_option_env_var* _decl,
    struct dj_option_env_value*     _value
)
{
    if (_decl->kind != D_OPTION_ENV_KIND_CHOICE)
    {
        return;
    }

    _value->text = NULL;

    for (uint32_t i = 0u; i < _decl->name_count; ++i)
    {
        if (_decl->names[i].value == _value->integer)
        {
            _value->text = _decl->names[i].name;
            break;
        }
    }

    return;
}

/*
d_option_env_get
  The binding in force, read by the declaration's kind; the default when
there is none. A choice's text is its name, found here so every reader has
it.
*/
struct dj_option_result
d_option_env_get(
    const struct dj_option_env* _env,
    uint32_t                    _scope,
    uint32_t                    _var,
    struct dj_option_env_value* _value
)
{
    const struct dj_option_env_var* decl  = NULL;
    const struct dj_option_env*     frame = NULL;

    if (_value == NULL)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    const int32_t status = d_internal_option_env_check(_env,
                                                       _scope,
                                                       _var,
                                                       &decl);

    if (status != (int32_t)D_OPTION_STATUS_OK)
    {
        return d_option_fail(status);
    }

    const struct dj_option* const cell =
        d_internal_option_env_in_force(_env, _scope, _var, &frame);

    d_internal_option_env_default(decl, _value);

    if (cell != NULL)
    {
        d_internal_option_env_read(frame, cell, _value);
    }

    d_internal_option_env_name_choice(decl, _value);

    return d_option_ok(_var);
}

/*
d_option_env_bool
  As d_option_env_get, one member.
*/
bool
d_option_env_bool(
    const struct dj_option_env* _env,
    uint32_t                    _scope,
    uint32_t                    _var
)
{
    struct dj_option_env_value value;

    return ( (d_option_env_get(_env, _scope, _var, &value).is_ok) &&
             (value.kind == D_OPTION_ENV_KIND_BOOL)              &&
             (value.integer != 0) );
}

/*
d_option_env_int
  As d_option_env_get, one member; 0 for REAL, TEXT and BOOL.
*/
int64_t
d_option_env_int(
    const struct dj_option_env* _env,
    uint32_t                    _scope,
    uint32_t                    _var
)
{
    struct dj_option_env_value value;

    if ( (!d_option_env_get(_env, _scope, _var, &value).is_ok) ||
         (value.kind == D_OPTION_ENV_KIND_BOOL)               ||
         (value.kind == D_OPTION_ENV_KIND_REAL)               ||
         (value.kind == D_OPTION_ENV_KIND_TEXT) )
    {
        return 0;
    }

    return value.integer;
}

/*
d_option_env_real
  As d_option_env_get, one member; 0 for the other kinds.
*/
double
d_option_env_real(
    const struct dj_option_env* _env,
    uint32_t                    _scope,
    uint32_t                    _var
)
{
    struct dj_option_env_value value;

    return ( (d_option_env_get(_env, _scope, _var, &value).is_ok) &&
             (value.kind == D_OPTION_ENV_KIND_REAL) )
               ? value.real
               : 0.0;
}

/*
d_option_env_text
  As d_option_env_get, one member; a choice's name; null for the rest.
*/
const char*
d_option_env_text(
    const struct dj_option_env* _env,
    uint32_t                    _scope,
    uint32_t                    _var
)
{
    struct dj_option_env_value value;

    return ( (d_option_env_get(_env, _scope, _var, &value).is_ok) &&
             ( (value.kind == D_OPTION_ENV_KIND_TEXT) ||
               (value.kind == D_OPTION_ENV_KIND_CHOICE) ) )
               ? value.text
               : NULL;
}

/*
d_option_env_source
  As d_option_env_get, the source alone.
*/
int32_t
d_option_env_source(
    const struct dj_option_env* _env,
    uint32_t                    _scope,
    uint32_t                    _var
)
{
    struct dj_option_env_value value;

    return d_option_env_get(_env, _scope, _var, &value).is_ok ? value.source
                                                              : -1;
}

/*
d_option_env_is_bound
  This frame's cells for the exact scope, any source.
*/
bool
d_option_env_is_bound(
    const struct dj_option_env* _env,
    uint32_t                    _scope,
    uint32_t                    _var
)
{
    const struct dj_option_env_var* decl = NULL;

    if (d_internal_option_env_check(_env, _scope, _var, &decl) !=
        (int32_t)D_OPTION_STATUS_OK)
    {
        return false;
    }

    for (uint32_t i = 0u; i < _env->set.count; ++i)
    {
        const uint64_t key = _env->set.options[i].key;

        if ( (D_INTERNAL_OPTION_ENV_KEY_VAR(key) == _var) &&
             (D_INTERNAL_OPTION_ENV_KEY_SCOPE(key) == _scope) )
        {
            return true;
        }
    }

    return false;
}

/*
d_option_env_scope_is_bound
  The scope is the high half of every key, so this is a scan of each frame's
cells; scope 0 is bound wherever anything is.
*/
bool
d_option_env_scope_is_bound(
    const struct dj_option_env* _env,
    uint32_t                    _scope
)
{
    for (const struct dj_option_env* f = _env; f != NULL; f = f->parent)
    {
        for (uint32_t i = 0u; i < f->set.count; ++i)
        {
            if (D_INTERNAL_OPTION_ENV_KEY_SCOPE(f->set.options[i].key) ==
                _scope)
            {
                return true;
            }
        }
    }

    return false;
}

/**
 * @brief Puts one layer into a list kept in rank order, the list's last
 *        falling off when it is full.
 *
 * @param[in,out] _layers    the list.
 * @param[in]     _count     the layers in it.
 * @param[in]     _capacity  its room.
 * @param[in]     _layer     the new one.
 * @param[in]     _cell      its cell, for its rank.
 * @param[in,out] _cells     the cells of the layers in the list.
 */
static void
d_internal_option_env_insert_layer(
    struct dj_option_env_layer*       _layers,
    size_t                            _count,
    size_t                            _capacity,
    const struct dj_option_env_layer* _layer,
    const struct dj_option*           _cell,
    const struct dj_option**          _cells
)
{
    size_t at = (_count < _capacity) ? _count : _capacity;

    // past every layer it does not outrank
    while ( (at > 0u) &&
            (d_internal_option_env_outranks(_cell,
                                            _layer->frame->depth,
                                            _cells[at - 1u],
                                            _layers[at - 1u].frame->depth)
             != 0) )
    {
        --at;
    }

    if (at >= _capacity)
    {
        return;
    }

    const size_t moved = ( (_count < _capacity) ? _count : (_capacity - 1u) )
                         - at;

    memmove(&_layers[at + 1u], &_layers[at], moved * sizeof(*_layers));
    memmove(&_cells[at + 1u], &_cells[at], moved * sizeof(*_cells));
    _layers[at] = *_layer;
    _cells[at]  = _cell;

    return;
}

/*
d_option_env_layers
  Every bearing cell of the chain, put in rank order as it is found; the
ranking is the one d_option_env_get applies, so the first is the one in
force. The cells of the listed layers are kept beside them, on the stack, so
ranks are compared exactly; past 64 layers the list keeps the top 64.
*/
size_t
d_option_env_layers(
    const struct dj_option_env* _env,
    uint32_t                    _scope,
    uint32_t                    _var,
    struct dj_option_env_layer* _layers,
    size_t                      _capacity
)
{
    const struct dj_option_env_var* decl = NULL;
    const struct dj_option*         cells[64];
    size_t                          count    = 0u;
    const size_t                    capacity = (_capacity < 64u) ? _capacity
                                                                 : 64u;

    if ( (d_internal_option_env_check(_env, _scope, _var, &decl) !=
          (int32_t)D_OPTION_STATUS_OK) ||
         ( (_layers == NULL) &&
           (_capacity != 0u) ) )
    {
        return (size_t)-1;
    }

    for (const struct dj_option_env* f = _env; f != NULL; f = f->parent)
    {
        for (uint32_t i = 0u; i < f->set.count; ++i)
        {
            const struct dj_option* const cell = &f->set.options[i];
            struct dj_option_env_layer    layer;

            if (d_internal_option_env_bears(cell, _scope, _var) == 0)
            {
                continue;
            }

            layer.frame = f;
            layer.scope = D_INTERNAL_OPTION_ENV_KEY_SCOPE(cell->key);
            layer.shown = 0;
            d_internal_option_env_default(decl, &layer.value);
            d_internal_option_env_read(f, cell, &layer.value);
            d_internal_option_env_name_choice(decl, &layer.value);
            d_internal_option_env_insert_layer(_layers, count, capacity,
                                               &layer, cell, cells);
            ++count;
        }
    }

    if ( (count > 0u) &&
         (capacity > 0u) )
    {
        _layers[0].shown = 1;
    }

    return count;
}


//==============================================================================
// BINDING BY VALUE
//==============================================================================

/**
 * @brief Whether a variable accepts an integer: an INT in its range, a
 *        CHOICE among its names, FLAGS among its bits; a value ruled but not
 *        built is refused by name.
 *
 * @param[in] _decl     the declaration.
 * @param[in] _integer  the value.
 * @return D_OPTION_STATUS_OK, INVALID_ARGUMENT or POLICY_REJECTED.
 */
static int32_t
d_internal_option_env_accepts(
    const struct dj_option_env_var* _decl,
    int64_t                         _integer
)
{
    int64_t mask    = 0;
    int64_t unbuilt = 0;

    if (_decl->kind == D_OPTION_ENV_KIND_INT)
    {
        return ( (_integer >= _decl->minimum) &&
                 (_integer <= _decl->maximum) )
                   ? (int32_t)D_OPTION_STATUS_OK
                   : (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT;
    }

    if ( (_decl->kind != D_OPTION_ENV_KIND_CHOICE) &&
         (_decl->kind != D_OPTION_ENV_KIND_FLAGS) )
    {
        return (int32_t)D_OPTION_STATUS_OK;
    }

    for (uint32_t i = 0u; i < _decl->name_count; ++i)
    {
        mask |= _decl->names[i].value;

        if ( (_decl->kind == D_OPTION_ENV_KIND_CHOICE) &&
             (_decl->names[i].value == _integer) )
        {
            return (int32_t)D_OPTION_STATUS_OK;
        }
    }

    for (uint32_t i = 0u; i < _decl->unbuilt_count; ++i)
    {
        unbuilt |= _decl->unbuilt[i].value;

        if ( (_decl->kind == D_OPTION_ENV_KIND_CHOICE) &&
             (_decl->unbuilt[i].value == _integer) )
        {
            return (int32_t)D_OPTION_STATUS_POLICY_REJECTED;
        }
    }

    if (_decl->kind == D_OPTION_ENV_KIND_CHOICE)
    {
        return (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT;
    }

    // flags: bits no name has, of which some an unbuilt name has
    if ((_integer & ~mask) == 0)
    {
        return (int32_t)D_OPTION_STATUS_OK;
    }

    return ((_integer & ~mask & unbuilt) != 0)
               ? (int32_t)D_OPTION_STATUS_POLICY_REJECTED
               : (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT;
}

/**
 * @brief Binds a value's bytes from a source: rewrites the frame's own cell
 *        for the source, or adds one, compacting the value block once if it
 *        is full.
 *
 * @param[in,out] _env     the frame.
 * @param[in]     _scope   the scope.
 * @param[in]     _var     the variable.
 * @param[in]     _source  the source.
 * @param[in]     _kind    the kind the caller is binding.
 * @param[in]     _bytes   the value.
 * @return the variable's id, or why not.
 */
static struct dj_option_result
d_internal_option_env_bind(
    struct dj_option_env* _env,
    uint32_t              _scope,
    uint32_t              _var,
    int32_t               _source,
    int32_t               _kind,
    const void*           _bytes
)
{
    const struct dj_option_env_var* decl = NULL;
    const int32_t status = d_internal_option_env_check(_env,
                                                       _scope,
                                                       _var,
                                                       &decl);

    if (status != (int32_t)D_OPTION_STATUS_OK)
    {
        return d_option_fail(status);
    }

    // INT carries CHOICE and FLAGS, whose storage it is
    if ( (decl->kind != _kind) &&
         ( (_kind != D_OPTION_ENV_KIND_INT) ||
           ( (decl->kind != D_OPTION_ENV_KIND_CHOICE) &&
             (decl->kind != D_OPTION_ENV_KIND_FLAGS) ) ) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_VALUE_TYPE_MISMATCH);
    }

    if (d_internal_option_env_source_ok(_source) == 0)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    const uint64_t key  = D_INTERNAL_OPTION_ENV_KEY(_scope, _source, _var);
    const uint32_t size = d_internal_option_env_size(decl->kind);

    if (!d_option_set_contains(&_env->set, key))
    {
        struct dj_option_result added =
            d_option_set_add(&_env->set,
                             key,
                             (dj_type_info16)D_TYPE_ULLONG_(),
                             d_internal_option_env_type(decl->kind),
                             size,
                             size);

        // the holes unbinding left are given back once
        if ( (!added.is_ok) &&
             (added.payload.error ==
              (int32_t)D_OPTION_STATUS_VALUE_CAPACITY) )
        {
            (void)d_option_set_compact(&_env->set);
            added = d_option_set_add(&_env->set,
                                     key,
                                     (dj_type_info16)D_TYPE_ULLONG_(),
                                     d_internal_option_env_type(decl->kind),
                                     size,
                                     size);
        }

        if (!added.is_ok)
        {
            return added;
        }
    }

    const struct dj_option_result written = d_option_set_set(&_env->set,
                                                              key,
                                                              _bytes,
                                                              (size_t)size);

    if (!written.is_ok)
    {
        return written;
    }

    ++_env->generation;

    return d_option_ok(_var);
}

/**
 * @brief Binds an integer -- INT, CHOICE or FLAGS -- from a source, once the
 *        variable accepts it.
 *
 * @param[in,out] _env      the frame.
 * @param[in]     _scope    the scope.
 * @param[in]     _var      the variable.
 * @param[in]     _source   the source.
 * @param[in]     _integer  the value.
 * @return as d_option_env_set_int.
 */
static struct dj_option_result
d_internal_option_env_bind_int(
    struct dj_option_env* _env,
    uint32_t              _scope,
    uint32_t              _var,
    int32_t               _source,
    int64_t               _integer
)
{
    const struct dj_option_env_var* const decl =
        (_env != NULL) ? d_internal_option_env_decl(_env, _var) : NULL;

    if (decl != NULL)
    {
        const int32_t status = d_internal_option_env_accepts(decl, _integer);

        if (status != (int32_t)D_OPTION_STATUS_OK)
        {
            return d_option_fail(status);
        }
    }

    return d_internal_option_env_bind(_env,
                                      _scope,
                                      _var,
                                      _source,
                                      D_OPTION_ENV_KIND_INT,
                                      &_integer);
}

/*
d_option_env_set_bool
  bool's own bytes, from the program.
*/
struct dj_option_result
d_option_env_set_bool(
    struct dj_option_env* _env,
    uint32_t              _scope,
    uint32_t              _var,
    bool                  _value
)
{
    return d_internal_option_env_bind(_env,
                                      _scope,
                                      _var,
                                      D_OPTION_ENV_SOURCE_PROGRAM,
                                      D_OPTION_ENV_KIND_BOOL,
                                      &_value);
}

/*
d_option_env_set_int
  A choice must be one of its values, flags only bits its names have, and an
int in its range.
*/
struct dj_option_result
d_option_env_set_int(
    struct dj_option_env* _env,
    uint32_t              _scope,
    uint32_t              _var,
    int64_t               _value
)
{
    return d_internal_option_env_bind_int(_env,
                                          _scope,
                                          _var,
                                          D_OPTION_ENV_SOURCE_PROGRAM,
                                          _value);
}

/*
d_option_env_set_real
  double's own bytes, from the program.
*/
struct dj_option_result
d_option_env_set_real(
    struct dj_option_env* _env,
    uint32_t              _scope,
    uint32_t              _var,
    double                _value
)
{
    return d_internal_option_env_bind(_env,
                                      _scope,
                                      _var,
                                      D_OPTION_ENV_SOURCE_PROGRAM,
                                      D_OPTION_ENV_KIND_REAL,
                                      &_value);
}

/*
d_option_env_set_text
  The pointer is the binding; the text is the caller's.
*/
struct dj_option_result
d_option_env_set_text(
    struct dj_option_env* _env,
    uint32_t              _scope,
    uint32_t              _var,
    const char*           _value
)
{
    return d_internal_option_env_bind(_env,
                                      _scope,
                                      _var,
                                      D_OPTION_ENV_SOURCE_PROGRAM,
                                      D_OPTION_ENV_KIND_TEXT,
                                      &_value);
}

/*
d_option_env_set_value
  The value's own kind decides which member is bound; INT goes through the
same acceptance as d_option_env_set_int.
*/
struct dj_option_result
d_option_env_set_value(
    struct dj_option_env*             _env,
    uint32_t                          _scope,
    uint32_t                          _var,
    int32_t                           _source,
    const struct dj_option_env_value* _value
)
{
    if ( (_value == NULL) ||
         (d_internal_option_env_source_ok(_source) == 0) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    switch (_value->kind)
    {
        case D_OPTION_ENV_KIND_BOOL:
        {
            const bool flag = (_value->integer != 0);

            return d_internal_option_env_bind(_env, _scope, _var, _source,
                                              D_OPTION_ENV_KIND_BOOL, &flag);
        }
        case D_OPTION_ENV_KIND_REAL:
            return d_internal_option_env_bind(_env, _scope, _var, _source,
                                              D_OPTION_ENV_KIND_REAL,
                                              &_value->real);
        case D_OPTION_ENV_KIND_TEXT:
            return d_internal_option_env_bind(_env, _scope, _var, _source,
                                              D_OPTION_ENV_KIND_TEXT,
                                              &_value->text);
        case D_OPTION_ENV_KIND_INT:
        case D_OPTION_ENV_KIND_CHOICE:
        case D_OPTION_ENV_KIND_FLAGS:
            return d_internal_option_env_bind_int(_env, _scope, _var, _source,
                                                  _value->integer);
        default:
            return d_option_fail((int32_t)D_OPTION_STATUS_VALUE_TYPE_MISMATCH);
    }
}

/**
 * @brief Binds a TEXT variable from a source to a copy of some text, kept in
 *        the frame's arena.
 *
 * @param[in,out] _env     the frame.
 * @param[in]     _scope   the scope.
 * @param[in]     _var     the variable.
 * @param[in]     _value   the text; need not end in a NUL.
 * @param[in]     _size    its length.
 * @param[in]     _source  the source.
 * @return as d_option_env_set_text_copy.
 *
 * The kind is checked before the arena is touched, so a refused copy costs no
 * room. The source may be in the arena itself -- another binding's text -- so
 * the compaction an allocation may run follows it.
 */
static struct dj_option_result
d_internal_option_env_text_copy(
    struct dj_option_env* _env,
    uint32_t              _scope,
    uint32_t              _var,
    const char*           _value,
    size_t                _size,
    int32_t               _source
)
{
    const struct dj_option_env_var* decl = NULL;

    if ( ( (_value == NULL) &&
           (_size != 0u) ) ||
         (d_internal_option_env_source_ok(_source) == 0) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    const int32_t status = d_internal_option_env_check(_env,
                                                       _scope,
                                                       _var,
                                                       &decl);

    if (status != (int32_t)D_OPTION_STATUS_OK)
    {
        return d_option_fail(status);
    }

    if (decl->kind != D_OPTION_ENV_KIND_TEXT)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_VALUE_TYPE_MISMATCH);
    }

    const char* source = _value;
    char* const room   = d_internal_option_env_alloc(_env, _size, &source);

    if ( (room == NULL) ||
         ( (_size != 0u) &&
           (source == NULL) ) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_BUFFER_TOO_SMALL);
    }

    if (_size != 0u)
    {
        memmove(room, source, _size);
    }

    room[_size] = '\0';

    return d_internal_option_env_bind(_env,
                                      _scope,
                                      _var,
                                      _source,
                                      D_OPTION_ENV_KIND_TEXT,
                                      &room);
}

/*
d_option_env_set_text_copy
  From the program.
*/
struct dj_option_result
d_option_env_set_text_copy(
    struct dj_option_env* _env,
    uint32_t              _scope,
    uint32_t              _var,
    const char*           _value,
    size_t                _size
)
{
    return d_internal_option_env_text_copy(_env,
                                           _scope,
                                           _var,
                                           _value,
                                           _size,
                                           D_OPTION_ENV_SOURCE_PROGRAM);
}

/*
d_option_env_unset
  Each source's cell for the scope and the variable, removed in order by the
core, which moves no value; the holes wait for a compaction.
*/
struct dj_option_result
d_option_env_unset(
    struct dj_option_env* _env,
    uint32_t              _scope,
    uint32_t              _var
)
{
    const struct dj_option_env_var* decl    = NULL;
    int                             removed = 0;
    const int32_t status = d_internal_option_env_check(_env,
                                                       _scope,
                                                       _var,
                                                       &decl);

    if (status != (int32_t)D_OPTION_STATUS_OK)
    {
        return d_option_fail(status);
    }

    for (int32_t source = D_OPTION_ENV_SOURCE_PROGRAM;
         source < D_OPTION_ENV_SOURCES;
         ++source)
    {
        removed |= d_option_set_remove(&_env->set,
                                       D_INTERNAL_OPTION_ENV_KEY(_scope,
                                                                 source,
                                                                 _var))
                       .is_ok ? 1 : 0;
    }

    if (removed != 0)
    {
        ++_env->generation;
    }

    return d_option_ok(_var);
}

/*
d_option_env_unset_from
  One source's cell.
*/
struct dj_option_result
d_option_env_unset_from(
    struct dj_option_env* _env,
    uint32_t              _scope,
    uint32_t              _var,
    int32_t               _source
)
{
    const struct dj_option_env_var* decl = NULL;
    const int32_t status = d_internal_option_env_check(_env,
                                                       _scope,
                                                       _var,
                                                       &decl);

    if (status != (int32_t)D_OPTION_STATUS_OK)
    {
        return d_option_fail(status);
    }

    if (d_internal_option_env_source_ok(_source) == 0)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    if (d_option_set_remove(&_env->set,
                            D_INTERNAL_OPTION_ENV_KEY(_scope, _source, _var))
            .is_ok)
    {
        ++_env->generation;
    }

    return d_option_ok(_var);
}


//==============================================================================
// READING TEXT
//==============================================================================

/**
 * @brief Reads a bool: 1, true, yes, on, y; 0, false, no, off, n.
 *
 * @param[in]  _text   the text, trimmed.
 * @param[in]  _size   its length.
 * @param[out] _value  the value.
 * @return 1, or 0 for text that is none of them.
 */
static int
d_internal_option_env_read_bool(
    const char* _text,
    size_t      _size,
    bool*       _value
)
{
    static const char* const YES[] = { "1", "true", "yes", "on", "y" };
    static const char* const NO[]  = { "0", "false", "no", "off", "n" };

    for (size_t i = 0u; i < sizeof(YES) / sizeof(YES[0]); ++i)
    {
        if (d_internal_option_env_equal(YES[i], _text, _size, 1) != 0)
        {
            *_value = true;

            return 1;
        }

        if (d_internal_option_env_equal(NO[i], _text, _size, 1) != 0)
        {
            *_value = false;

            return 1;
        }
    }

    return 0;
}

/**
 * @brief Reads an int64_t: a sign, then decimal, or 0x, 0o or 0b and digits
 *        of that base.
 *
 * @param[in]  _text   the text, trimmed.
 * @param[in]  _size   its length.
 * @param[out] _value  the value.
 * @return 1, or 0 for text that is not one, or one out of range.
 */
static int
d_internal_option_env_read_int(
    const char* _text,
    size_t      _size,
    int64_t*    _value
)
{
    size_t   at        = 0u;
    int      negative  = 0;
    uint64_t base      = 10u;
    uint64_t magnitude = 0u;

    if ( (_size > 0u) &&
         ( (_text[0] == '+') ||
           (_text[0] == '-') ) )
    {
        negative = (_text[0] == '-');
        ++at;
    }

    // a base prefix
    if ( (at + 1u < _size) &&
         (_text[at] == '0') )
    {
        const char prefix = d_internal_option_env_lower(_text[at + 1u]);

        base = (prefix == 'x') ? 16u
             : (prefix == 'o') ? 8u
             : (prefix == 'b') ? 2u
             : 10u;

        if (base != 10u)
        {
            at += 2u;
        }
    }

    if (at == _size)
    {
        return 0;
    }

    for (; at < _size; ++at)
    {
        const char c     = d_internal_option_env_lower(_text[at]);
        uint64_t   digit = 16u;

        if ( (c >= '0') &&
             (c <= '9') )
        {
            digit = (uint64_t)(c - '0');
        }
        else if ( (c >= 'a') &&
                  (c <= 'f') )
        {
            digit = (uint64_t)(c - 'a') + 10u;
        }

        // a digit of the base, and no overflow
        if ( (digit >= base) ||
             (magnitude > (UINT64_MAX - digit) / base) )
        {
            return 0;
        }

        magnitude = (magnitude * base) + digit;
    }

    const uint64_t limit = (negative != 0)
                               ? ((uint64_t)INT64_MAX + 1u)
                               : (uint64_t)INT64_MAX;

    if (magnitude > limit)
    {
        return 0;
    }

    *_value = (negative == 0)
                  ? (int64_t)magnitude
                  : ( (magnitude == limit)
                          ? INT64_MIN
                          : -(int64_t)magnitude );

    return 1;
}

/**
 * @brief Reads a double in the C locale's form, whatever the locale.
 *
 * @param[in]  _text   the text, trimmed.
 * @param[in]  _size   its length.
 * @param[out] _value  the value.
 * @return 1, or 0 for text that is not one, or one that overflows.
 *
 * The text is held to C's alphabet -- digits, a sign, '.', an exponent --
 * or to inf, infinity and nan, so a locale's own form ("1,5") is refused, and
 * then copied with the locale's decimal point in place of '.' for strtod.
 */
static int
d_internal_option_env_read_real(
    const char* _text,
    size_t      _size,
    double*     _value
)
{
    char                buffer[D_INTERNAL_OPTION_ENV_SCRATCH];
    const struct lconv* locale = localeconv();
    const char*         point  = ( (locale != NULL)                &&
                                   (locale->decimal_point != NULL) &&
                                   (locale->decimal_point[0] != '\0') )
                                     ? locale->decimal_point
                                     : ".";
    const size_t        point_size = strlen(point);
    const char*         body       = _text;
    size_t              body_size  = _size;
    size_t              at         = 0u;

    if ( (_size == 0u) ||
         (_size >= 64u) )
    {
        return 0;
    }

    if ( (body[0] == '+') ||
         (body[0] == '-') )
    {
        ++body;
        --body_size;
    }

    const int special =
        (d_internal_option_env_equal("inf", body, body_size, 1) != 0)      ||
        (d_internal_option_env_equal("infinity", body, body_size, 1) != 0) ||
        (d_internal_option_env_equal("nan", body, body_size, 1) != 0);

    for (size_t i = 0u; i < _size; ++i)
    {
        const char c = _text[i];

        // C's alphabet, or a special word as a whole
        if ( (special == 0) &&
             (!( ((c >= '0') && (c <= '9')) ||
                 (c == '+') || (c == '-')   ||
                 (c == '.') || (c == 'e')   ||
                 (c == 'E') )) )
        {
            return 0;
        }

        if (c == '.')
        {
            if (at + point_size >= sizeof(buffer))
            {
                return 0;
            }

            memcpy(buffer + at, point, point_size);
            at += point_size;
        }
        else
        {
            buffer[at++] = c;
        }
    }

    buffer[at] = '\0';

    char*     end   = NULL;
    const int saved = errno;

    errno = 0;

    const double value = strtod(buffer, &end);
    const int    range = ( (errno == ERANGE) &&
                           ( (value == HUGE_VAL) ||
                             (value == -HUGE_VAL) ) );

    errno = saved;

    if ( (end != buffer + at) ||
         (range != 0) )
    {
        return 0;
    }

    *_value = value;

    return 1;
}

/**
 * @brief Whether a counted text names one of a list of names, ignoring case.
 *
 * @param[in]  _names  the names; may be null when _count is 0.
 * @param[in]  _count  how many.
 * @param[in]  _text   the text.
 * @param[in]  _size   its length.
 * @param[out] _value  the value of the one it names.
 * @return 1, or 0 when it names none.
 */
static int
d_internal_option_env_names(
    const struct dj_option_env_name* _names,
    uint32_t                         _count,
    const char*                      _text,
    size_t                           _size,
    int64_t*                         _value
)
{
    for (uint32_t i = 0u; i < _count; ++i)
    {
        if (d_internal_option_env_equal(_names[i].name,
                                        _text,
                                        _size,
                                        1) != 0)
        {
            *_value = _names[i].value;

            return 1;
        }
    }

    return 0;
}

/**
 * @brief Reads a choice: one of its names, ignoring case, or a number; an
 *        unbuilt name is told apart, so it can be refused by name.
 *
 * @param[in]  _decl   the variable.
 * @param[in]  _text   the text, trimmed.
 * @param[in]  _size   its length.
 * @param[out] _value  the value.
 * @return D_OPTION_STATUS_OK, POLICY_REJECTED for an unbuilt name, or
 *         INVALID_ARGUMENT for text that is neither a name nor a number.
 */
static int32_t
d_internal_option_env_read_choice(
    const struct dj_option_env_var* _decl,
    const char*                     _text,
    size_t                          _size,
    int64_t*                        _value
)
{
    if (d_internal_option_env_names(_decl->names,
                                    _decl->name_count,
                                    _text,
                                    _size,
                                    _value) != 0)
    {
        return (int32_t)D_OPTION_STATUS_OK;
    }

    if (d_internal_option_env_names(_decl->unbuilt,
                                    _decl->unbuilt_count,
                                    _text,
                                    _size,
                                    _value) != 0)
    {
        return (int32_t)D_OPTION_STATUS_POLICY_REJECTED;
    }

    return (d_internal_option_env_read_int(_text, _size, _value) != 0)
               ? (int32_t)D_OPTION_STATUS_OK
               : (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT;
}

/**
 * @brief Reads one term of flags: a name, an unbuilt name or a number.
 *
 * @param[in]  _decl  the variable.
 * @param[in]  _text  the term.
 * @param[in]  _size  its length.
 * @param[out] _bits  its bits.
 * @return D_OPTION_STATUS_OK, POLICY_REJECTED or INVALID_ARGUMENT.
 */
static int32_t
d_internal_option_env_read_term(
    const struct dj_option_env_var* _decl,
    const char*                     _text,
    size_t                          _size,
    int64_t*                        _bits
)
{
    if (d_internal_option_env_names(_decl->names,
                                    _decl->name_count,
                                    _text,
                                    _size,
                                    _bits) != 0)
    {
        return (int32_t)D_OPTION_STATUS_OK;
    }

    if (d_internal_option_env_names(_decl->unbuilt,
                                    _decl->unbuilt_count,
                                    _text,
                                    _size,
                                    _bits) != 0)
    {
        return (int32_t)D_OPTION_STATUS_POLICY_REJECTED;
    }

    return (d_internal_option_env_read_int(_text, _size, _bits) != 0)
               ? (int32_t)D_OPTION_STATUS_OK
               : (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT;
}

/**
 * @brief Reads flags: terms -- a name or a number, each with an optional
 *        '+' or '-' -- separated by ',', '|' or space. Text starting with a
 *        sign edits the value in force; text without one starts from 0.
 *
 * @param[in]  _env    the frame.
 * @param[in]  _scope  the scope, for the value in force.
 * @param[in]  _var    the variable.
 * @param[in]  _decl   its declaration.
 * @param[in]  _text   the text, trimmed.
 * @param[in]  _size   its length.
 * @param[out] _value  the value.
 * @return D_OPTION_STATUS_OK, or why a term was refused; whether the bits
 *         are accepted is the binding's to say.
 */
static int32_t
d_internal_option_env_read_flags(
    const struct dj_option_env*     _env,
    uint32_t                        _scope,
    uint32_t                        _var,
    const struct dj_option_env_var* _decl,
    const char*                     _text,
    size_t                          _size,
    int64_t*                        _value
)
{
    int64_t value = 0;
    size_t  at    = 0u;

    if ( (_size > 0u) &&
         ( (_text[0] == '+') ||
           (_text[0] == '-') ) )
    {
        value = d_option_env_int(_env, _scope, _var);
    }

    while (at < _size)
    {
        char    sign = '+';
        int64_t bits = 0;

        // skip separators, then one term
        while ( (at < _size) &&
                ( (_text[at] == ',') ||
                  (_text[at] == '|') ||
                  (d_internal_option_env_is_space(_text[at]) != 0) ) )
        {
            ++at;
        }

        if (at == _size)
        {
            break;
        }

        if ( (_text[at] == '+') ||
             (_text[at] == '-') )
        {
            sign = _text[at++];
        }

        const size_t start = at;

        while ( (at < _size) &&
                (d_internal_option_env_is_name_char(_text[at]) != 0) )
        {
            ++at;
        }

        const int32_t status = (at == start)
            ? (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT
            : d_internal_option_env_read_term(_decl, _text + start,
                                              at - start, &bits);

        if (status != (int32_t)D_OPTION_STATUS_OK)
        {
            return status;
        }

        value = (sign == '-') ? (value & ~bits) : (value | bits);
    }

    *_value = value;

    return (int32_t)D_OPTION_STATUS_OK;
}

/**
 * @brief Binds a variable from text, read as its kind reads it, from a
 *        source.
 *
 * @param[in,out] _env     the frame.
 * @param[in]     _scope   the scope.
 * @param[in]     _var     the variable.
 * @param[in]     _value   the text.
 * @param[in]     _size    its length.
 * @param[in]     _source  the source.
 * @return as d_option_env_assign_from.
 *
 * Text other than TEXT is trimmed first; TEXT is kept as it is.
 */
static struct dj_option_result
d_internal_option_env_assign(
    struct dj_option_env* _env,
    uint32_t              _scope,
    uint32_t              _var,
    const char*           _value,
    size_t                _size,
    int32_t               _source
)
{
    const struct dj_option_env_var* const decl = d_option_env_var_at(_env,
                                                                     _var);
    const char*                           text    = _value;
    size_t                                size    = _size;
    struct dj_option_env_value            value;
    int32_t                               status  = 0;
    bool                                  flag    = false;

    if (decl->kind == D_OPTION_ENV_KIND_TEXT)
    {
        return d_internal_option_env_text_copy(_env, _scope, _var, text, size,
                                               _source);
    }

    d_internal_option_env_trim(&text, &size);
    d_internal_option_env_default(decl, &value);

    switch (decl->kind)
    {
        case D_OPTION_ENV_KIND_BOOL:
            status = (d_internal_option_env_read_bool(text, size, &flag) != 0)
                         ? (int32_t)D_OPTION_STATUS_OK
                         : (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT;
            value.integer = flag ? 1 : 0;
            break;
        case D_OPTION_ENV_KIND_INT:
            status = (d_internal_option_env_read_int(text, size,
                                                     &value.integer) != 0)
                         ? (int32_t)D_OPTION_STATUS_OK
                         : (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT;
            break;
        case D_OPTION_ENV_KIND_REAL:
            status = (d_internal_option_env_read_real(text, size,
                                                      &value.real) != 0)
                         ? (int32_t)D_OPTION_STATUS_OK
                         : (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT;
            break;
        case D_OPTION_ENV_KIND_CHOICE:
            status = d_internal_option_env_read_choice(decl, text, size,
                                                       &value.integer);
            break;
        default:
            status = d_internal_option_env_read_flags(_env, _scope, _var,
                                                      decl, text, size,
                                                      &value.integer);
            break;
    }

    return (status == (int32_t)D_OPTION_STATUS_OK)
               ? d_option_env_set_value(_env, _scope, _var, _source, &value)
               : d_option_fail(status);
}

/*
d_option_env_assign_from
  The name, with its scope, then the kind's reader.
*/
struct dj_option_result
d_option_env_assign_from(
    struct dj_option_env* _env,
    const char*           _name,
    size_t                _name_size,
    const char*           _value,
    size_t                _value_size,
    int32_t               _source
)
{
    uint32_t scope = D_OPTION_ENV_GLOBAL;

    if ( (_env == NULL)  ||
         (_name == NULL) ||
         ( (_value == NULL) &&
           (_value_size != 0u) ) ||
         (d_internal_option_env_source_ok(_source) == 0) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    const struct dj_option_result found =
        d_option_env_resolve(_env, _name, _name_size, &scope);

    if (!found.is_ok)
    {
        return found;
    }

    return d_internal_option_env_assign(_env,
                                        scope,
                                        found.payload.value,
                                        (_value != NULL) ? _value : "",
                                        _value_size,
                                        _source);
}

/*
d_option_env_assign
  From the program.
*/
struct dj_option_result
d_option_env_assign(
    struct dj_option_env* _env,
    const char*           _name,
    size_t                _name_size,
    const char*           _value,
    size_t                _value_size
)
{
    return d_option_env_assign_from(_env,
                                    _name,
                                    _name_size,
                                    _value,
                                    _value_size,
                                    D_OPTION_ENV_SOURCE_PROGRAM);
}


//==============================================================================
// STATEMENTS AND SCRIPTS
//==============================================================================

/**
 * @brief Reads one escape of a quoted value: the character after a
 *        backslash, or x and two hexadecimal digits.
 *
 * @param[in]     _text  the text between the quotes.
 * @param[in]     _size  its length.
 * @param[in,out] _at    the escape's first character, after the backslash;
 *                       moved to its last.
 * @param[out]    _out   the character it stands for.
 * @return 1, or 0 for an escape that is none.
 */
static int
d_internal_option_env_escape(
    const char* _text,
    size_t      _size,
    size_t*     _at,
    char*       _out
)
{
    int64_t code = 0;

    switch (_text[*_at])
    {
        case '"':  *_out = '"';  return 1;
        case '\\': *_out = '\\'; return 1;
        case '\'': *_out = '\''; return 1;
        case 'n':  *_out = '\n'; return 1;
        case 'r':  *_out = '\r'; return 1;
        case 't':  *_out = '\t'; return 1;
        case '0':  *_out = '\0'; return 1;
        case 'x':  break;
        default:   return 0;
    }

    // x: exactly two hexadecimal digits
    if (*_at + 2u >= _size)
    {
        return 0;
    }

    const char digits[5] = { '0', 'x', _text[*_at + 1u], _text[*_at + 2u],
                             '\0' };

    if (d_internal_option_env_read_int(digits, 4u, &code) == 0)
    {
        return 0;
    }

    *_out = (char)(unsigned char)code;
    *_at += 2u;

    return 1;
}

/**
 * @brief Reads a quoted value's escapes into a buffer.
 *
 * @param[in]  _text      the text between the quotes.
 * @param[in]  _size      its length.
 * @param[out] _out       where; it needs at most _size bytes.
 * @param[out] _out_size  the bytes written.
 * @return 1, or 0 for an escape that is none.
 */
static int
d_internal_option_env_unescape(
    const char* _text,
    size_t      _size,
    char*       _out,
    size_t*     _out_size
)
{
    size_t length = 0u;

    for (size_t i = 0u; i < _size; ++i)
    {
        char c = _text[i];

        // an escape: one character, or \xHH
        if ( (c == '\\') &&
             ( (++i == _size) ||
               (d_internal_option_env_escape(_text, _size, &i, &c) == 0) ) )
        {
            return 0;
        }

        _out[length++] = c;
    }

    *_out_size = length;

    return 1;
}

/**
 * @brief Binds a variable from a quoted value: the text between the quotes,
 *        its escapes read.
 *
 * @param[in,out] _env     the frame.
 * @param[in]     _scope   the scope.
 * @param[in]     _var     the variable.
 * @param[in]     _text    the text between the quotes.
 * @param[in]     _size    its length.
 * @param[in]     _source  the source.
 * @return as d_option_env_assign_from.
 *
 * A TEXT value is read straight into the arena, where it will live -- it
 * needs at most its quoted length -- and bound there; any other is read into
 * a buffer here and then read as its kind. The quoted text may itself be in
 * the arena, so the allocation follows it through a compaction.
 */
static struct dj_option_result
d_internal_option_env_quoted(
    struct dj_option_env* _env,
    uint32_t              _scope,
    uint32_t              _var,
    const char*           _text,
    size_t                _size,
    int32_t               _source
)
{
    const struct dj_option_env_var* const decl = d_option_env_var_at(_env,
                                                                     _var);
    size_t                                length = 0u;

    if (decl->kind == D_OPTION_ENV_KIND_TEXT)
    {
        const char* source = _text;
        char* const room   = d_internal_option_env_alloc(_env, _size, &source);

        if ( (room == NULL) ||
             ( (_size != 0u) &&
               (source == NULL) ) )
        {
            return d_option_fail((int32_t)D_OPTION_STATUS_BUFFER_TOO_SMALL);
        }

        // a refused escape leaves an empty string, reclaimed by compaction
        if (d_internal_option_env_unescape(source,
                                           _size,
                                           room,
                                           &length) == 0)
        {
            room[0] = '\0';

            return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
        }

        room[length] = '\0';

        return d_internal_option_env_bind(_env, _scope, _var, _source,
                                          D_OPTION_ENV_KIND_TEXT, &room);
    }

    char scratch[D_INTERNAL_OPTION_ENV_SCRATCH];

    if ( (_size > sizeof(scratch)) ||
         (d_internal_option_env_unescape(_text,
                                         _size,
                                         scratch,
                                         &length) == 0) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    return d_internal_option_env_assign(_env, _scope, _var, scratch, length,
                                        _source);
}

/**
 * @brief A statement whose name was found: "-name" removes the source's
 *        binding, "name" alone sets a bool, and "= value" assigns.
 *
 * @param[in,out] _env     the frame.
 * @param[in]     _text    the statement, trimmed.
 * @param[in]     _size    its length.
 * @param[in]     _at      where the name ended, past space.
 * @param[in]     _unset   1 when it began with '-'.
 * @param[in]     _scope   the name's scope.
 * @param[in]     _var     the name's variable.
 * @param[in]     _source  the source.
 * @return as d_option_env_put_from.
 */
static struct dj_option_result
d_internal_option_env_act(
    struct dj_option_env* _env,
    const char*           _text,
    size_t                _size,
    size_t                _at,
    int                   _unset,
    uint32_t              _scope,
    uint32_t              _var,
    int32_t               _source
)
{
    size_t at = _at;

    if (at == _size)
    {
        if (_unset != 0)
        {
            return d_option_env_unset_from(_env, _scope, _var, _source);
        }

        const bool yes = true;

        return (d_option_env_var_at(_env, _var)->kind ==
                D_OPTION_ENV_KIND_BOOL)
                   ? d_internal_option_env_bind(_env, _scope, _var, _source,
                                                D_OPTION_ENV_KIND_BOOL, &yes)
                   : d_option_fail(
                         (int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    if ( (_unset != 0) ||
         (_text[at] != '=') )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    do
    {
        ++at;
    } while ( (at < _size) &&
              (d_internal_option_env_is_space(_text[at]) != 0) );

    // an unquoted value is the rest; a quoted one must end the statement
    if ( (at == _size) ||
         (_text[at] != '"') )
    {
        return d_internal_option_env_assign(_env, _scope, _var, _text + at,
                                            _size - at, _source);
    }

    size_t close = at + 1u;

    while ( (close < _size) &&
            (_text[close] != '"') )
    {
        close += (_text[close] == '\\') ? 2u : 1u;
    }

    if (close != _size - 1u)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    return d_internal_option_env_quoted(_env, _scope, _var, _text + at + 1u,
                                        close - at - 1u, _source);
}

/**
 * @brief Carries out one statement, given as a counted text.
 *
 * @param[in,out] _env     the frame.
 * @param[in]     _text    the statement.
 * @param[in]     _size    its length.
 * @param[in]     _source  the source.
 * @return as d_option_env_put_from.
 */
static struct dj_option_result
d_internal_option_env_statement(
    struct dj_option_env* _env,
    const char*           _text,
    size_t                _size,
    int32_t               _source
)
{
    const char* text  = _text;
    size_t      size  = _size;
    size_t      at    = 0u;
    uint32_t    scope = D_OPTION_ENV_GLOBAL;

    d_internal_option_env_trim(&text, &size);

    const int unset = ( (size > 0u) &&
                        (text[0] == '-') ) ? 1 : 0;

    // the name: an identifier, with an optional "frame." and "scope."
    at = (size_t)unset;

    while ( (at < size) &&
            ( (d_internal_option_env_is_name_char(text[at]) != 0) ||
              (text[at] == '.') ) )
    {
        ++at;
    }

    const struct dj_option_result var =
        d_option_env_resolve(_env,
                             text + unset,
                             at - (size_t)unset,
                             &scope);

    while ( (at < size) &&
            (d_internal_option_env_is_space(text[at]) != 0) )
    {
        ++at;
    }

    if (!var.is_ok)
    {
        return (at == (size_t)unset)
                   ? d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT)
                   : var;
    }

    return d_internal_option_env_act(_env, text, size, at, unset, scope,
                                     var.payload.value, _source);
}

/*
d_option_env_put_from
  One statement: the whole string.
*/
struct dj_option_result
d_option_env_put_from(
    struct dj_option_env* _env,
    const char*           _statement,
    int32_t               _source
)
{
    if ( (_env == NULL)       ||
         (_statement == NULL) ||
         (d_internal_option_env_source_ok(_source) == 0) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    return d_internal_option_env_statement(_env,
                                           _statement,
                                           strlen(_statement),
                                           _source);
}

/*
d_option_env_put
  From the program.
*/
struct dj_option_result
d_option_env_put(
    struct dj_option_env* _env,
    const char*           _statement
)
{
    return d_option_env_put_from(_env,
                                 _statement,
                                 D_OPTION_ENV_SOURCE_PROGRAM);
}

/*
d_option_env_put_n_from
  A counted statement: the same parser, given the length.
*/
struct dj_option_result
d_option_env_put_n_from(
    struct dj_option_env* _env,
    const char*           _statement,
    size_t                _size,
    int32_t               _source
)
{
    if ( (_env == NULL) ||
         ( (_statement == NULL) &&
           (_size != 0u) ) ||
         (d_internal_option_env_source_ok(_source) == 0) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    return d_internal_option_env_statement(_env,
                                           (_statement != NULL) ? _statement
                                                                : "",
                                           _size,
                                           _source);
}

/*
d_option_env_statement_end
  A backslash inside quotes escapes the next character, so an escaped quote
does not end the quoted text.
*/
size_t
d_option_env_statement_end(
    const char* _script,
    size_t      _at
)
{
    if (_script == NULL)
    {
        return _at;
    }

    int quoted = 0;

    while ( (_script[_at] != '\0') &&
            ( (quoted != 0) ||
              ( (_script[_at] != ';')  &&
                (_script[_at] != '\n') &&
                (_script[_at] != '#') ) ) )
    {
        if ( (_script[_at] == '\\') &&
             (quoted != 0)          &&
             (_script[_at + 1u] != '\0') )
        {
            ++_at;
        }
        else if (_script[_at] == '"')
        {
            quoted = !quoted;
        }

        ++_at;
    }

    return _at;
}

/*
d_option_env_load_from
  A statement ends at ';', a line break or '#' outside quotes; a comment
runs to its line's end. Empty statements are skipped.
*/
struct dj_option_result
d_option_env_load_from(
    struct dj_option_env* _env,
    const char*           _script,
    int32_t               _source,
    size_t*               _error_offset
)
{
    size_t   at    = 0u;
    uint32_t count = 0u;

    if ( (_env == NULL)    ||
         (_script == NULL) ||
         (d_internal_option_env_source_ok(_source) == 0) )
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    while (_script[at] != '\0')
    {
        const size_t start = at;

        at = d_option_env_statement_end(_script, at);

        const char* text = _script + start;
        size_t      size = at - start;

        d_internal_option_env_trim(&text, &size);

        if (size > 0u)
        {
            const struct dj_option_result result =
                d_internal_option_env_statement(_env, text, size, _source);

            if (!result.is_ok)
            {
                if (_error_offset != NULL)
                {
                    *_error_offset = (size_t)(text - _script);
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
d_option_env_load
  From the program.
*/
struct dj_option_result
d_option_env_load(
    struct dj_option_env* _env,
    const char*           _script,
    size_t*               _error_offset
)
{
    return d_option_env_load_from(_env,
                                  _script,
                                  D_OPTION_ENV_SOURCE_PROGRAM,
                                  _error_offset);
}

/**
 * @brief An environment variable's text for a declared name: the prefix,
 *        then the name upper-cased.
 *
 * @param[in]  _prefix  the prefix.
 * @param[in]  _name    the name; may be null.
 * @param[out] _key     where; D_INTERNAL_OPTION_ENV_SCRATCH bytes.
 * @return getenv's text, or null when unset or too long to ask for.
 */
static const char*
d_internal_option_env_getenv(
    const char* _prefix,
    const char* _name,
    char*       _key
)
{
    const size_t length = strlen(_prefix);
    const size_t size   = (_name != NULL) ? strlen(_name) : 0u;

    if ( (_name == NULL) ||
         (length + size >= D_INTERNAL_OPTION_ENV_SCRATCH) )
    {
        return NULL;
    }

    memcpy(_key, _prefix, length);

    for (size_t c = 0u; c <= size; ++c)
    {
        _key[length + c] = ( (_name[c] >= 'a') &&
                             (_name[c] <= 'z') )
                               ? (char)(_name[c] - 'a' + 'A')
                               : _name[c];
    }

    return getenv(_key);
}

/*
d_option_env_import
  One getenv per visible variable that its name reaches -- a name an inner
declaration hides is the inner one's -- and one for its alias when the name
is unset. Every variable is tried; the first refusal is the one reported.
*/
struct dj_option_result
d_option_env_import(
    struct dj_option_env* _env,
    const char*           _prefix
)
{
    struct dj_option_result failure = d_option_ok(0u);
    uint32_t                count   = 0u;
    const char* const       prefix  = (_prefix != NULL) ? _prefix : "";
    char                    key[D_INTERNAL_OPTION_ENV_SCRATCH];

    if (_env == NULL)
    {
        return d_option_fail((int32_t)D_OPTION_STATUS_INVALID_ARGUMENT);
    }

    for (uint32_t k = 0u; k < d_option_env_var_count(_env); ++k)
    {
        const uint32_t                        var  = d_option_env_var_id(_env,
                                                                         k);
        const struct dj_option_env_var* const decl = d_option_env_var_at(_env,
                                                                          var);
        const struct dj_option_result         seen =
            d_option_env_find(_env, decl->name, strlen(decl->name));
        const char*                           value =
            d_internal_option_env_getenv(prefix, decl->name, key);

        if ( (!seen.is_ok) ||
             (seen.payload.value != var) )
        {
            continue;
        }

        if (value == NULL)
        {
            value = d_internal_option_env_getenv(prefix, decl->alias, key);
        }

        if (value != NULL)
        {
            const struct dj_option_result result =
                d_internal_option_env_assign(_env,
                                             D_OPTION_ENV_GLOBAL,
                                             var,
                                             value,
                                             strlen(value),
                                             D_OPTION_ENV_SOURCE_ENVIRONMENT);

            if (result.is_ok)
            {
                ++count;
            }
            else if (failure.is_ok)
            {
                failure = result;
            }
        }
    }

    return failure.is_ok ? d_option_ok(count) : failure;
}


//==============================================================================
// WRITING TEXT
//==============================================================================

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
d_internal_option_env_append(
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
 * @brief Writes an int64_t in decimal into a buffer of 24 bytes.
 *
 * @param[out] _buffer  the buffer.
 * @param[in]  _value   the value.
 * @return the digits' length.
 */
static size_t
d_internal_option_env_decimal(
    char    _buffer[24],
    int64_t _value
)
{
    char     digits[24];
    size_t   count     = 0u;
    size_t   length    = 0u;
    uint64_t magnitude = (_value < 0) ? ((uint64_t)0u - (uint64_t)_value)
                                      : (uint64_t)_value;

    do
    {
        digits[count++] = (char)('0' + (int)(magnitude % 10u));
        magnitude      /= 10u;
    } while (magnitude != 0u);

    if (_value < 0)
    {
        _buffer[length++] = '-';
    }

    while (count > 0u)
    {
        _buffer[length++] = digits[--count];
    }

    return length;
}

/**
 * @brief The name with the most bits a value still has, each of them in it.
 *
 * @param[in] _decl  the variable.
 * @param[in] _left  the bits still to name.
 * @return the name's index, or -1 when no name fits.
 */
static int32_t
d_internal_option_env_widest(
    const struct dj_option_env_var* _decl,
    int64_t                         _left
)
{
    int32_t best       = -1;
    int     best_count = 0;

    for (uint32_t i = 0u; i < _decl->name_count; ++i)
    {
        const int64_t bits  = _decl->names[i].value;
        int           count = 0;

        if ( (bits == 0) ||
             ((bits & ~_left) != 0) )
        {
            continue;
        }

        for (uint64_t b = (uint64_t)bits; b != 0u; b &= b - 1u)
        {
            ++count;
        }

        if (count > best_count)
        {
            best       = (int32_t)i;
            best_count = count;
        }
    }

    return best;
}

/**
 * @brief Writes flags as names joined by '|': the name with the most bits
 *        that the value still has, each time, and any bits no name covers
 *        as one hexadecimal number.
 *
 * @param[in]     _decl      the variable.
 * @param[in]     _value     the value.
 * @param[out]    _buffer    as d_option_env_format.
 * @param[in]     _capacity  its size.
 * @param[in,out] _length    the text's length so far.
 */
static void
d_internal_option_env_format_flags(
    const struct dj_option_env_var* _decl,
    int64_t                         _value,
    char*                           _buffer,
    size_t                          _capacity,
    size_t*                         _length
)
{
    static const char HEX[] = "0123456789abcdef";
    int64_t           left  = _value;
    const char*       zero  = "0";

    for (int first = 1; left != 0; first = 0)
    {
        const int32_t best = d_internal_option_env_widest(_decl, left);

        if (first == 0)
        {
            d_internal_option_env_append(_buffer, _capacity, _length, "|", 1u);
        }

        // bits no name covers, as one number
        if (best < 0)
        {
            char   digits[20];
            size_t count = 0u;

            for (uint64_t rest = (uint64_t)left; rest != 0u; rest >>= 4)
            {
                digits[count++] = HEX[rest & 0x0Fu];
            }

            d_internal_option_env_append(_buffer, _capacity, _length, "0x", 2u);

            while (count > 0u)
            {
                d_internal_option_env_append(_buffer, _capacity, _length,
                                             &digits[--count], 1u);
            }

            return;
        }

        d_internal_option_env_append(_buffer, _capacity, _length,
                                     _decl->names[best].name,
                                     strlen(_decl->names[best].name));
        left &= ~_decl->names[best].value;
    }

    // nothing set: a name for 0, or 0
    if (_value == 0)
    {
        for (uint32_t i = 0u; i < _decl->name_count; ++i)
        {
            if (_decl->names[i].value == 0)
            {
                zero = _decl->names[i].name;
                break;
            }
        }

        d_internal_option_env_append(_buffer, _capacity, _length, zero,
                                     strlen(zero));
    }

    return;
}

/**
 * @brief Writes a double so strtod reads it back exactly, with '.' for the
 *        locale's decimal point.
 *
 * @param[in]     _real      the value.
 * @param[out]    _buffer    as d_option_env_format.
 * @param[in]     _capacity  its size.
 * @param[in,out] _length    the text's length so far.
 * @return 1, or 0 when snprintf failed.
 */
static int
d_internal_option_env_format_real(
    double  _real,
    char*   _buffer,
    size_t  _capacity,
    size_t* _length
)
{
    char                number[64];
    const struct lconv* locale = localeconv();
    const char*         point  = ( (locale != NULL)                &&
                                   (locale->decimal_point != NULL) &&
                                   (locale->decimal_point[0] != '\0') )
                                     ? locale->decimal_point
                                     : ".";
    const size_t        point_size = strlen(point);
    const int           written    = snprintf(number,
                                              sizeof(number),
                                              "%.17g",
                                              _real);
    size_t              i          = 0u;

    if ( (written < 0) ||
         ((size_t)written >= sizeof(number)) )
    {
        return 0;
    }

    while (i < (size_t)written)
    {
        // the locale's point, written as C's
        if (strncmp(number + i, point, point_size) == 0)
        {
            d_internal_option_env_append(_buffer, _capacity, _length, ".", 1u);
            i += point_size;
        }
        else
        {
            d_internal_option_env_append(_buffer, _capacity, _length,
                                         number + i, 1u);
            ++i;
        }
    }

    return 1;
}

/*
d_option_env_format
  The kind's own spelling, through one bounded appender, so the length is
counted whether or not it fits.
*/
size_t
d_option_env_format(
    const struct dj_option_env* _env,
    uint32_t                    _scope,
    uint32_t                    _var,
    char*                       _buffer,
    size_t                      _capacity
)
{
    struct dj_option_env_value value;
    char                       number[24];
    size_t                     length = 0u;

    if ( ( (_buffer == NULL) &&
           (_capacity != 0u) ) ||
         (!d_option_env_get(_env, _scope, _var, &value).is_ok) )
    {
        return (size_t)-1;
    }

    const struct dj_option_env_var* const decl = d_option_env_var_at(_env,
                                                                     _var);

    switch (value.kind)
    {
        case D_OPTION_ENV_KIND_BOOL:
            d_internal_option_env_append(_buffer, _capacity, &length,
                                         (value.integer != 0) ? "true"
                                                              : "false",
                                         (value.integer != 0) ? 4u : 5u);
            break;
        case D_OPTION_ENV_KIND_REAL:
            if (d_internal_option_env_format_real(value.real, _buffer,
                                                  _capacity, &length) == 0)
            {
                return (size_t)-1;
            }
            break;
        case D_OPTION_ENV_KIND_TEXT:
            if (value.text != NULL)
            {
                d_internal_option_env_append(_buffer, _capacity, &length,
                                             value.text, strlen(value.text));
            }

            break;
        case D_OPTION_ENV_KIND_FLAGS:
            d_internal_option_env_format_flags(decl, value.integer, _buffer,
                                               _capacity, &length);
            break;
        default:
            // INT, and a CHOICE with no name for its value
            if ( (value.kind == D_OPTION_ENV_KIND_CHOICE) &&
                 (value.text != NULL) )
            {
                d_internal_option_env_append(_buffer, _capacity, &length,
                                             value.text, strlen(value.text));
            }
            else
            {
                d_internal_option_env_append(
                    _buffer, _capacity, &length, number,
                    d_internal_option_env_decimal(number, value.integer));
            }

            break;
    }

    if (_capacity != 0u)
    {
        _buffer[(length < _capacity) ? length : (_capacity - 1u)] = '\0';
    }

    return length;
}


//==============================================================================
// READING ON A HOT PATH
//==============================================================================

/*
d_option_env_cached
  One walk of the chain's counters; the read itself only when they moved. A
variable that does not exist leaves the value at kind 0, cached like any
other answer until the generation moves.
*/
const struct dj_option_env_value*
d_option_env_cached(
    const struct dj_option_env* _env,
    uint32_t                    _scope,
    uint32_t                    _var,
    struct dj_option_env_cache* _cache
)
{
    if (_cache == NULL)
    {
        return NULL;
    }

    const uint32_t generation = d_option_env_generation(_env);

    if ( (_cache->filled == 0) ||
         (_cache->generation != generation) )
    {
        if (!d_option_env_get(_env, _scope, _var, &_cache->value).is_ok)
        {
            memset(&_cache->value, 0, sizeof(_cache->value));
        }

        _cache->generation = generation;
        _cache->filled     = 1;
    }

    return &_cache->value;
}


#endif  // RE_STD_HAS_INT64
