/*******************************************************************************
* djinterp [parse]                                                    arg_list.c
*
* The growable arrays the parser's records keep, declared in arg_internal.h
* (section 6).
*   One push, one search and one removal for each element type -- spans,
* owned strings, option and command pointers, unused arguments, validators
* -- each growing through d_arg_internal_grow, so that every list doubles
* from the same small start and a refused allocation leaves it as it was;
* and, for spans and owned strings, a reserve of room ahead.
*
*
* path:      /src/djinterp/parse/cli/arg_parser/c/arg_list.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./arg_internal.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // free
#include <string.h>  // memcpy, memmove
// djinterp
#include "../../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // lists, statuses
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator.h"  // d_arg_validator


/*
d_arg_internal_text_reserve
  Grows only when the request exceeds the capacity, so a reserve before a
run of pushes costs nothing when the room is there.
*/
int
d_arg_internal_text_reserve(
    struct d_arg_text_list* _list,
    size_t                  _count
)
{
    // there is room already
    if (_count <= _list->capacity)
    {
        return D_ARG_OK;
    }

    size_t      capacity = 0u;
    void* const items    = d_arg_internal_grow(_list->items,
                                               _list->capacity,
                                               _count,
                                               sizeof(*_list->items),
                                               &capacity);

    // check if memory allocation was successful
    if (!items)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    _list->items    = (struct d_pack_text*)items;
    _list->capacity = capacity;

    return D_ARG_OK;
}

/*
d_arg_internal_text_push
  A reserve of one more, then the span.
*/
int
d_arg_internal_text_push(
    struct d_arg_text_list* _list,
    struct d_pack_text      _text
)
{
    const int status = d_arg_internal_text_reserve(_list,
                                                   _list->count + 1u);

    // the list could not grow
    if (status != D_ARG_OK)
    {
        return status;
    }

    _list->items[_list->count] = _text;
    _list->count++;

    return D_ARG_OK;
}

/*
d_arg_internal_text_append
  A reserve of all the spans, then the spans in one copy, so the list grows
by all of them or by none.
*/
int
d_arg_internal_text_append(
    struct d_arg_text_list*       _list,
    const struct d_arg_text_list* _source,
    size_t                        _first,
    size_t                        _count
)
{
    const int status = d_arg_internal_text_reserve(_list,
                                                   _list->count + _count);

    // the spans, once there is room for all of them
    if ( (status == D_ARG_OK) &&
         (_count > 0u) )
    {
        memcpy(_list->items + _list->count,
               _source->items + _first,
               _count * sizeof(*_list->items));
        _list->count += _count;
    }

    return status;
}

/*
d_arg_internal_string_reserve
  Grows only when the request exceeds the capacity, as
d_arg_internal_text_reserve does.
*/
int
d_arg_internal_string_reserve(
    struct d_arg_string_list* _list,
    size_t                    _count
)
{
    // there is room already
    if (_count <= _list->capacity)
    {
        return D_ARG_OK;
    }

    size_t      capacity = 0u;
    void* const items    = d_arg_internal_grow(_list->items,
                                               _list->capacity,
                                               _count,
                                               sizeof(*_list->items),
                                               &capacity);

    // check if memory allocation was successful
    if (!items)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    _list->items    = (char**)items;
    _list->capacity = capacity;

    return D_ARG_OK;
}

/*
d_arg_internal_string_adopt
  A reserve of one more, then the string; the string freed when it cannot
be kept, so a caller's every path past this call is free of it.
*/
int
d_arg_internal_string_adopt(
    struct d_arg_string_list* _list,
    char*                     _string
)
{
    // a refused copy arrives as NULL
    if (!_string)
    {
        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    const int status = d_arg_internal_string_reserve(_list,
                                                     _list->count + 1u);

    // the list could not grow
    if (status != D_ARG_OK)
    {
        free(_string);

        return status;
    }

    _list->items[_list->count] = _string;
    _list->count++;

    return D_ARG_OK;
}

/*
d_arg_internal_string_copy
  The copy first, then the adoption, which frees it again on failure.
*/
int
d_arg_internal_string_copy(
    struct d_arg_string_list* _list,
    struct d_pack_text        _text,
    const char**              _copy
)
{
    char* const copy   = d_arg_internal_copy_text(_text);
    const int   status = d_arg_internal_string_adopt(_list,
                                                     copy);

    // report the copy only once the list holds it
    if ( (status == D_ARG_OK) &&
         (_copy != NULL) )
    {
        *_copy = copy;
    }

    return status;
}

/*
d_arg_internal_string_truncate
  Frees from the end back to the count, keeping the array.
*/
void
d_arg_internal_string_truncate(
    struct d_arg_string_list* _list,
    size_t                    _count
)
{
    // free the strings past the count
    while (_list->count > _count)
    {
        _list->count--;
        free(_list->items[_list->count]);
        _list->items[_list->count] = NULL;
    }

    return;
}

/*
d_arg_internal_find_member
  A linear search, each candidate compared without being copied.
*/
long
d_arg_internal_find_member(
    struct d_pack_text              _name,
    const struct d_arg_string_list* _list,
    bool                            _case,
    bool                            _underscore
)
{
    // compare against each string in turn
    for (size_t index = 0u; index < _list->count; ++index)
    {
        if (d_arg_internal_names_match(_name,
                                       d_arg_internal_text_of(
                                           _list->items[index]),
                                       _case,
                                       _underscore))
        {
            return (long)index;
        }
    }

    return -1L;
}

/*
d_arg_internal_option_push
  Grows by doubling, then appends the pointer.
*/
int
d_arg_internal_option_push(
    struct d_arg_option_list* _list,
    struct d_arg_option*      _option
)
{
    // grow when full
    if (_list->count == _list->capacity)
    {
        size_t      capacity = 0u;
        void* const items    = d_arg_internal_grow(_list->items,
                                                   _list->capacity,
                                                   _list->count + 1u,
                                                   sizeof(*_list->items),
                                                   &capacity);

        // check if memory allocation was successful
        if (!items)
        {
            return D_ARG_ERROR_OUT_OF_MEMORY;
        }

        _list->items    = (struct d_arg_option**)items;
        _list->capacity = capacity;
    }

    _list->items[_list->count] = _option;
    _list->count++;

    return D_ARG_OK;
}

/*
d_arg_internal_option_insert
  A set's insert over a list: lists here are short, and the order they keep
makes the errors reported from them reproducible where std::set's pointer
order is not.
*/
int
d_arg_internal_option_insert(
    struct d_arg_option_list* _list,
    struct d_arg_option*      _option
)
{
    // already there
    if (d_arg_internal_option_has(_list,
                                  _option))
    {
        return D_ARG_OK;
    }

    return d_arg_internal_option_push(_list,
                                      _option);
}

/*
d_arg_internal_option_has
  A linear search by pointer.
*/
bool
d_arg_internal_option_has(
    const struct d_arg_option_list* _list,
    const struct d_arg_option*      _option
)
{
    // look at each entry
    for (size_t index = 0u; index < _list->count; ++index)
    {
        if (_list->items[index] == _option)
        {
            return true;
        }
    }

    return false;
}

/*
d_arg_internal_option_remove
  Closes the gap with memmove, so the entries after keep their order.
*/
bool
d_arg_internal_option_remove(
    struct d_arg_option_list*  _list,
    const struct d_arg_option* _option
)
{
    // find the first occurrence
    for (size_t index = 0u; index < _list->count; ++index)
    {
        if (_list->items[index] == _option)
        {
            memmove(&_list->items[index],
                    &_list->items[index + 1u],
                    (_list->count - index - 1u) * sizeof(*_list->items));
            _list->count--;

            return true;
        }
    }

    return false;
}

/*
d_arg_internal_command_push
  As d_arg_internal_option_push, for commands.
*/
int
d_arg_internal_command_push(
    struct d_arg_command_list* _list,
    struct d_arg_command*      _command
)
{
    // grow when full
    if (_list->count == _list->capacity)
    {
        size_t      capacity = 0u;
        void* const items    = d_arg_internal_grow(_list->items,
                                                   _list->capacity,
                                                   _list->count + 1u,
                                                   sizeof(*_list->items),
                                                   &capacity);

        // check if memory allocation was successful
        if (!items)
        {
            return D_ARG_ERROR_OUT_OF_MEMORY;
        }

        _list->items    = (struct d_arg_command**)items;
        _list->capacity = capacity;
    }

    _list->items[_list->count] = _command;
    _list->count++;

    return D_ARG_OK;
}

/*
d_arg_internal_command_insert
  As d_arg_internal_option_insert, for commands.
*/
int
d_arg_internal_command_insert(
    struct d_arg_command_list* _list,
    struct d_arg_command*      _command
)
{
    // already there
    if (d_arg_internal_command_has(_list,
                                   _command))
    {
        return D_ARG_OK;
    }

    return d_arg_internal_command_push(_list,
                                       _command);
}

/*
d_arg_internal_command_has
  A linear search by pointer.
*/
bool
d_arg_internal_command_has(
    const struct d_arg_command_list* _list,
    const struct d_arg_command*      _command
)
{
    // look at each entry
    for (size_t index = 0u; index < _list->count; ++index)
    {
        if (_list->items[index] == _command)
        {
            return true;
        }
    }

    return false;
}

/*
d_arg_internal_command_remove
  As d_arg_internal_option_remove, for commands.
*/
bool
d_arg_internal_command_remove(
    struct d_arg_command_list*  _list,
    const struct d_arg_command* _command
)
{
    // find the first occurrence
    for (size_t index = 0u; index < _list->count; ++index)
    {
        if (_list->items[index] == _command)
        {
            memmove(&_list->items[index],
                    &_list->items[index + 1u],
                    (_list->count - index - 1u) * sizeof(*_list->items));
            _list->count--;

            return true;
        }
    }

    return false;
}

/*
d_arg_internal_extra_push
  Grows by doubling, then appends the argument and its classification.
*/
int
d_arg_internal_extra_push(
    struct d_arg_extra_list* _list,
    struct d_pack_text       _text,
    int                      _class
)
{
    // grow when full
    if (_list->count == _list->capacity)
    {
        size_t      capacity = 0u;
        void* const items    = d_arg_internal_grow(_list->items,
                                                   _list->capacity,
                                                   _list->count + 1u,
                                                   sizeof(*_list->items),
                                                   &capacity);

        // check if memory allocation was successful
        if (!items)
        {
            return D_ARG_ERROR_OUT_OF_MEMORY;
        }

        _list->items    = (struct d_arg_extra*)items;
        _list->capacity = capacity;
    }

    _list->items[_list->count].text       = _text;
    _list->items[_list->count].classifier = _class;
    _list->count++;

    return D_ARG_OK;
}

/**
 * @brief Makes room in a validator list for one more.
 *
 * @param[in,out] _list       the list.
 * @param[in,out] _validator  the validator that was to go in, released when
 *                            there is no room.
 * @return D_ARG_OK; D_ARG_ERROR_OUT_OF_MEMORY, the validator then released and
 *         the list unchanged.
 *
 * Grows a validator list to hold one more, releasing the validator that
 * was to go into it when it cannot -- the list was to take it over either
 * way.
 */
D_STATIC int
d_arg_internal_validator_room(
    struct d_arg_validator_list* _list,
    struct d_arg_validator*      _validator
)
{
    // there is room already
    if (_list->count < _list->capacity)
    {
        return D_ARG_OK;
    }

    size_t      capacity = 0u;
    void* const items    = d_arg_internal_grow(_list->items,
                                               _list->capacity,
                                               _list->count + 1u,
                                               sizeof(*_list->items),
                                               &capacity);

    // check if memory allocation was successful
    if (!items)
    {
        d_arg_validator_release(_validator);

        return D_ARG_ERROR_OUT_OF_MEMORY;
    }

    _list->items    = (struct d_arg_validator*)items;
    _list->capacity = capacity;

    return D_ARG_OK;
}

/*
d_arg_internal_validator_push
  The validator moves into the list by value; its owned parts move with it.
*/
int
d_arg_internal_validator_push(
    struct d_arg_validator_list* _list,
    struct d_arg_validator       _validator
)
{
    const int status = d_arg_internal_validator_room(_list,
                                                     &_validator);

    // the list could not grow
    if (status != D_ARG_OK)
    {
        return status;
    }

    _list->items[_list->count] = _validator;
    _list->count++;

    return D_ARG_OK;
}

/*
d_arg_internal_validator_insert
  CLI11's transform puts its validator first; the others move up by one.
*/
int
d_arg_internal_validator_insert(
    struct d_arg_validator_list* _list,
    struct d_arg_validator       _validator
)
{
    const int status = d_arg_internal_validator_room(_list,
                                                     &_validator);

    // the list could not grow
    if (status != D_ARG_OK)
    {
        return status;
    }

    memmove(&_list->items[1],
            &_list->items[0],
            _list->count * sizeof(*_list->items));
    _list->items[0] = _validator;
    _list->count++;

    return D_ARG_OK;
}

/*
d_arg_internal_option_list_release
  The array only: the options belong to their commands.
*/
void
d_arg_internal_option_list_release(
    struct d_arg_option_list* _list
)
{
    // nothing to free without a list
    if (_list != NULL)
    {
        free(_list->items);
        _list->items    = NULL;
        _list->count    = 0u;
        _list->capacity = 0u;
    }

    return;
}

/*
d_arg_internal_command_list_release
  The array only: the commands belong to their parents.
*/
void
d_arg_internal_command_list_release(
    struct d_arg_command_list* _list
)
{
    // nothing to free without a list
    if (_list != NULL)
    {
        free(_list->items);
        _list->items    = NULL;
        _list->count    = 0u;
        _list->capacity = 0u;
    }

    return;
}

/*
d_arg_internal_extra_list_release
  The array only: the arguments' bytes belong to `argv` or the pool.
*/
void
d_arg_internal_extra_list_release(
    struct d_arg_extra_list* _list
)
{
    // nothing to free without a list
    if (_list != NULL)
    {
        free(_list->items);
        _list->items    = NULL;
        _list->count    = 0u;
        _list->capacity = 0u;
    }

    return;
}

/*
d_arg_internal_validator_list_release
  Each validator owns its parts, so each is released before the array.
*/
void
d_arg_internal_validator_list_release(
    struct d_arg_validator_list* _list
)
{
    // nothing to free without a list
    if (_list == NULL)
    {
        return;
    }

    // release every validator, then the array
    for (size_t index = 0u; index < _list->count; ++index)
    {
        d_arg_validator_release(&_list->items[index]);
    }

    free(_list->items);
    _list->items    = NULL;
    _list->count    = 0u;
    _list->capacity = 0u;

    return;
}

/*
d_arg_internal_context_release
  Only an owned context is freed; a borrowed one is merely forgotten.
*/
void
d_arg_internal_context_release(
    void**            _context,
    d_arg_release_fn* _release
)
{
    // free an owned context
    if ( (*_release != NULL) &&
         (*_context != NULL) )
    {
        (*_release)(*_context);
    }

    *_context = NULL;
    *_release = NULL;

    return;
}

/*
d_arg_internal_context_replace
  The old context released when it is not the new one -- given again, it
is only kept -- then the new pair stored.
*/
void
d_arg_internal_context_replace(
    void**            _context,
    d_arg_release_fn* _release,
    void*             _new_context,
    d_arg_release_fn  _new_release
)
{
    // the old context goes, unless it is given again
    if (*_context != _new_context)
    {
        d_arg_internal_context_release(_context,
                                       _release);
    }

    *_context = _new_context;
    *_release = _new_release;

    return;
}
