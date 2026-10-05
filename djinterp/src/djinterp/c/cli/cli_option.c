/*******************************************************************************
* djinterp [c]                                                      cli_option.c
*
*   Implementation of generic string-to-void* lookup and CLI parsing.
*
*
* path:      /src/djinterp/c/cli/cli_option.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.22
*                                                            revised: 2026.09.29
*******************************************************************************/
#include "../../../../inc/djinterp/c/cli/cli_option.h"

// std
#include <string.h>  // strlen


/******************************************************************************
 * I. `d_string_cli_entry` LOOKUP
 *****************************************************************************/

/*
d_cli_find
  Searches an array of `d_string_cli_entry` for an element whose key
matches _key via strcmp. Returns the matching entry's value pointer,
or NULL if not found. NULL keys in the array are skipped.

Parameter(s):
  _entries: array of `d_string_cli_entry` structs
  _count:   number of elements in the array
  _key:     string to search for
Return:
  the value pointer of the matching entry, or NULL.
*/
void*
d_cli_find
(
    const struct d_string_cli_entry* _entries,
    size_t                           _entries_count,
    const char*                      _key,
    size_t                           _key_length
)
{
    size_t i;

    if ( (!_entries)       ||
         (!_entries_count) ||
         (!_key)           ||
         (!_key_length) )
    {
        return NULL;
    }

    for (i = 0; i < _entries_count; i++)
    {
        const struct d_string_cli_entry* e = &_entries[i];

        if ( (e->key) &&
             (d_strcmp_n(e->key, e->key_length, _key, _key_length) == 0) )
        {
            return e->value;
        }
    }

    return NULL;
}


/******************************************************************************
 * II. GENERIC FIND BY FIELD
 *****************************************************************************/

/*
d_cli_find_by_field
  Searches an array of arbitrary structs for an element whose const char* field
at _field_offset matches _key via strcmp.

Strides through the array using _element_size, reads the
const char* at each element's byte offset, and compares.
NULL field values are skipped.

Parameter(s):
  _array:        base pointer to the array
  _count:        number of elements
  _element_size: sizeof one element
  _field_offset: byte offset of the const char* field (via offsetof)
  _key:          string to search for
Return:
  pointer to the matching element, or NULL.
*/
void*
d_cli_find_by_field
(
    const void* _array,
    size_t      _count,
    size_t      _element_size,
    size_t      _field_offset,
    const char* _key,
    size_t      _key_length
)
{
    size_t             i;
    const char*        base;
    const char* const* field_ptr;
    const char*        field_val;

    if ( (!_array)            ||
         (_count == 0)        ||
         (_element_size == 0) ||
         (!_key)              ||
         (_key_length == 0) )
    {
        return NULL;
    }

    base = (const char*)_array;

    for (i = 0; i < _count; i++)
    {
        field_ptr = (const char* const*)
                    ( base + D_ARRAY_TOTAL_SIZE(_element_size, i) +
                      _field_offset);

        field_val = *(field_ptr);

        if ( (field_val) &&
             (d_strcmp_n(field_val,
                         strlen(field_val),
                         _key,
                         _key_length) == 0) )
        {
            return (void*)(base + D_ARRAY_TOTAL_SIZE(_element_size, i));
        }
    }

    return NULL;
}


/******************************************************************************
 * III. CUSTOM LOOKUP
 *****************************************************************************/

/*
d_cli_find_with
  Delegates lookup to a user-supplied fn_cli_lookup.

Parameter(s):
  _array:   raw array base
  _count:   number of elements
  _key:     search key
  _lookup:  user-supplied lookup function
  _context: opaque context passed to the lookup function
Return:
  whatever the lookup function returns, or NULL on invalid input.
*/
const void*
d_cli_find_with
(
    const void*   _array,
    size_t        _count,
    const char*   _key,
    size_t        _key_length,
    fn_cli_lookup _lookup,
    void*         _context
)
{
    if ( (!_array)      ||
         (_count == 0)  ||
         (!_key)        ||
         (!_key_length) ||
         (!_lookup) )
    {
        return NULL;
    }

    return _lookup(_array, _count, _key, _key_length, _context);
}


/******************************************************************************
 * IV. ARGV PARSING
 *****************************************************************************/

/*
d_cli_parse
  Parses argc/argv against a `d_string_cli_entry` table. For each argv element
starting from index 1, looks up the entire token as a key in the entry table.
On match:
  - if parameter _on_match is non-NULL, invoke it with the matched key, value,
    next argv element (or NULL), and context.
  - if _on_match returns false, parsing stops immediately.
  - if _on_match is NULL, the match is silently consumed.
Unrecognized arguments are skipped.

Parameter(s):
  _entries:  array of `d_string_cli_entry` structs
  _count:    number of entries
  _argc:     argument count from main
  _argv:     argument vector from main
  _on_match: handler called per matched entry; NULL = silent consume
  _context:  opaque pointer passed to _on_match
Return:
  true if parsing completed, false if handler stopped it.
*/
bool
d_cli_parse
(
    const struct d_string_cli_entry* _entries,
    size_t                           _count,
    int                              _argc,
    char*                            _argv[],
    fn_cli_match                     _on_match,
    void*                            _context
)
{
    int         i;
    size_t      key_length;
    void*       value;
    const char* next_arg;

    if ( (!_entries) ||
         (!_argv)    ||
         (_count == 0) )
    {
        return true;
    }

    for (i = 1; i < _argc; i++)
    {
        if (!_argv[i])
        {
            continue;
        }

        key_length = strlen(_argv[i]);

        value = d_cli_find(_entries, _count, _argv[i], key_length);

        if (!value)
        {
            continue;
        }

        if (_on_match)
        {
            next_arg = ( (i + 1) < _argc)
                            ? _argv[i + 1]
                            : NULL;

            if (!_on_match(_argv[i], key_length, value, next_arg,
                           _context))
            {
                return false;
            }
        }
    }

    return true;
}
