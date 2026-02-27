#include "../../../inc/c/text/str_interp.h"


/*
d_str_interp_context_new
  Creates a new interpolation context with default settings.

Parameter(s):
  none.
Return:
  A pointer to a newly allocated d_str_interp_context, or NULL if allocation
failed.
*/
struct d_str_interp_context*
d_str_interp_context_new
(
    void
)
{
    struct d_str_interp_context* context;

    context = malloc(sizeof(struct d_str_interp_context));

    // check if allocation succeeded
    if (!context)
    {
        return NULL;
    }

    // initialize context
    context->specifier_count = 0;

    // zero out the specifiers array
    memset(context->specifiers,
           0,
           sizeof(context->specifiers));

    return context;
}

/*
d_str_interp_context_free
  Frees an interpolation context and all associated memory including
specifier names and values.

Parameter(s):
  _context: the context to free; may be NULL.
Return:
  none.
*/
void
d_str_interp_context_free
(
    struct d_str_interp_context* _context
)
{
    size_t i;

    if (_context)
    {
        // free all specifier names and values
        for (i = 0; i < _context->specifier_count; i++)
        {
            if (_context->specifiers[i].name)
            {
                free(_context->specifiers[i].name);
            }

            if (_context->specifiers[i].value)
            {
                free(_context->specifiers[i].value);
            }
        }

        free(_context);
    }

    return;
}

/*
d_str_interp_context_clear
  Clears all specifiers from a context without freeing the context itself.

Parameter(s):
  _context: the context to clear; may be NULL.
Return:
  none.
*/
void
d_str_interp_context_clear
(
    struct d_str_interp_context* _context
)
{
    size_t i;

    if (_context)
    {
        // free all specifier names and values
        for (i = 0; i < _context->specifier_count; i++)
        {
            if (_context->specifiers[i].name)
            {
                free(_context->specifiers[i].name);
                _context->specifiers[i].name = NULL;
            }

            if (_context->specifiers[i].value)
            {
                free(_context->specifiers[i].value);
                _context->specifiers[i].value = NULL;
            }

            _context->specifiers[i].name_length  = 0;
            _context->specifiers[i].value_length = 0;
        }

        _context->specifier_count = 0;
    }

    return;
}

/*
d_internal_interp_find_specifier
  Internal function to find a specifier by name.

Parameter(s):
  _context: the context to search.
  _name:    the specifier name to find.
Return:
  The index of the specifier if found, or -1 if not found.
*/
static int
d_internal_interp_find_specifier
(
    const struct d_str_interp_context* _context,
    const char*                        _name
)
{
    size_t i;
    size_t name_len;

    name_len = strlen(_name);

    // search through all specifiers
    for (i = 0; i < _context->specifier_count; i++)
    {
        if ( (_context->specifiers[i].name_length == name_len) &&
             (memcmp(_context->specifiers[i].name,
                     _name,
                     name_len) == 0) )
        {
            return (int)i;
        }
    }

    return -1;
}

/*
d_internal_interp_find_specifier_n
  Internal function to find a specifier by name with explicit length.

Parameter(s):
  _context:     the context to search.
  _name:        the specifier name to find.
  _name_length: the length of the name string.
Return:
  The index of the specifier if found, or -1 if not found.
*/
static int
d_internal_interp_find_specifier_n
(
    const struct d_str_interp_context* _context,
    const char*                        _name,
    size_t                             _name_length
)
{
    size_t i;

    // search through all specifiers
    for (i = 0; i < _context->specifier_count; i++)
    {
        if ( (_context->specifiers[i].name_length == _name_length) &&
             (memcmp(_context->specifiers[i].name,
                     _name,
                     _name_length) == 0) )
        {
            return (int)i;
        }
    }

    return -1;
}

/*
d_str_interp_add_specifier
  Adds a new specifier to the context. If a specifier with the same name
already exists, its value is updated instead.

Parameter(s):
  _context: the context to add the specifier to.
  _name:    the name of the specifier (null-terminated).
  _value:   the value to substitute for this specifier (null-terminated).
Return:
  D_STR_INTERP_SUCCESS on success, or an appropriate error code:
  - D_STR_INTERP_ERROR_NULL_PARAM if any parameter is NULL,
  - D_STR_INTERP_ERROR_TOO_MANY_SPECIFIERS if limit reached,
  - D_STR_INTERP_ERROR_ALLOCATION_FAILED if memory allocation fails.
*/
enum d_str_interp_error
d_str_interp_add_specifier
(
    struct d_str_interp_context* _context,
    const char*                  _name,
    const char*                  _value
)
{
    size_t name_len;
    size_t value_len;

    // validate parameters
    if ( (!_context) ||
         (!_name)    ||
         (!_value) )
    {
        return D_STR_INTERP_ERROR_NULL_PARAM;
    }

    name_len  = strlen(_name);
    value_len = strlen(_value);

    return d_str_interp_add_specifier_n(_context,
                                        _name,
                                        name_len,
                                        _value,
                                        value_len);
}

/*
d_str_interp_add_specifier_n
  Adds a new specifier to the context with explicit name and value lengths.
If a specifier with the same name already exists, its value is updated
instead.

Parameter(s):
  _context:      the context to add the specifier to.
  _name:         the name of the specifier.
  _name_length:  the length of the name string.
  _value:        the value to substitute for this specifier.
  _value_length: the length of the value string.
Return:
  D_STR_INTERP_SUCCESS on success, or an appropriate error code:
  - D_STR_INTERP_ERROR_NULL_PARAM if any parameter is NULL,
  - D_STR_INTERP_ERROR_TOO_MANY_SPECIFIERS if limit reached,
  - D_STR_INTERP_ERROR_ALLOCATION_FAILED if memory allocation fails.
*/
enum d_str_interp_error
d_str_interp_add_specifier_n
(
    struct d_str_interp_context* _context,
    const char*                  _name,
    size_t                       _name_length,
    const char*                  _value,
    size_t                       _value_length
)
{
    struct d_str_interp_specifier* spec;
    char*                          name_copy;
    char*                          value_copy;
    int                            existing;

    // validate parameters
    if ( (!_context) ||
         (!_name)    ||
         (!_value) )
    {
        return D_STR_INTERP_ERROR_NULL_PARAM;
    }

    // check if specifier already exists
    existing = d_internal_interp_find_specifier_n(_context,
                                                  _name,
                                                  _name_length);
    if (existing >= 0)
    {
        return d_str_interp_update_specifier(_context,
                                             _context->specifiers[existing].name,
                                             _value);
    }

    // check if we have room for more specifiers
    if (_context->specifier_count >= D_STR_INTERP_MAX_SPECIFIERS)
    {
        return D_STR_INTERP_ERROR_TOO_MANY_SPECIFIERS;
    }

    // allocate and copy name
    name_copy = malloc(_name_length + 1);

    // check allocation
    if (!name_copy)
    {
        return D_STR_INTERP_ERROR_ALLOCATION_FAILED;
    }

    memcpy(name_copy, _name, _name_length);
    name_copy[_name_length] = '\0';

    // allocate and copy value
    value_copy = malloc(_value_length + 1);

    // check allocation
    if (!value_copy)
    {
        free(name_copy);

        return D_STR_INTERP_ERROR_ALLOCATION_FAILED;
    }

    memcpy(value_copy, _value, _value_length);
    value_copy[_value_length] = '\0';

    // add the specifier
    spec               = &_context->specifiers[_context->specifier_count];
    spec->name         = name_copy;
    spec->name_length  = _name_length;
    spec->value        = value_copy;
    spec->value_length = _value_length;

    _context->specifier_count++;

    return D_STR_INTERP_SUCCESS;
}

/*
d_str_interp_remove_specifier
  Removes a specifier from the context by name.

Parameter(s):
  _context: the context to remove the specifier from.
  _name:    the name of the specifier to remove.
Return:
  D_STR_INTERP_SUCCESS on success, or an appropriate error code:
  - D_STR_INTERP_ERROR_NULL_PARAM if any parameter is NULL,
  - D_STR_INTERP_ERROR_SPECIFIER_NOT_FOUND if specifier doesn't exist.
*/
enum d_str_interp_error
d_str_interp_remove_specifier
(
    struct d_str_interp_context* _context,
    const char*                  _name
)
{
    int    index;
    size_t i;

    // validate parameters
    if ( (!_context) ||
         (!_name) )
    {
        return D_STR_INTERP_ERROR_NULL_PARAM;
    }

    // find the specifier
    index = d_internal_interp_find_specifier(_context, _name);
    if (index < 0)
    {
        return D_STR_INTERP_ERROR_SPECIFIER_NOT_FOUND;
    }

    // free the name and value
    if (_context->specifiers[index].name)
    {
        free(_context->specifiers[index].name);
    }

    if (_context->specifiers[index].value)
    {
        free(_context->specifiers[index].value);
    }

    // shift remaining specifiers down
    for (i = (size_t)index; i < _context->specifier_count - 1; i++)
    {
        _context->specifiers[i] = _context->specifiers[i + 1];
    }

    // clear the last slot and decrement count
    memset(&_context->specifiers[_context->specifier_count - 1],
           0,
           sizeof(struct d_str_interp_specifier));
    _context->specifier_count--;

    return D_STR_INTERP_SUCCESS;
}

/*
d_str_interp_update_specifier
  Updates the value of an existing specifier.

Parameter(s):
  _context: the context containing the specifier.
  _name:    the name of the specifier to update.
  _value:   the new value for the specifier.
Return:
  D_STR_INTERP_SUCCESS on success, or an appropriate error code:
  - D_STR_INTERP_ERROR_NULL_PARAM if any parameter is NULL,
  - D_STR_INTERP_ERROR_SPECIFIER_NOT_FOUND if specifier doesn't exist,
  - D_STR_INTERP_ERROR_ALLOCATION_FAILED if memory allocation fails.
*/
enum d_str_interp_error
d_str_interp_update_specifier
(
    struct d_str_interp_context* _context,
    const char*                  _name,
    const char*                  _value
)
{
    int                            index;
    size_t                         value_len;
    char*                          value_copy;
    struct d_str_interp_specifier* spec;

    // validate parameters
    if ( (!_context) ||
         (!_name)    ||
         (!_value) )
    {
        return D_STR_INTERP_ERROR_NULL_PARAM;
    }

    // find the specifier
    index = d_internal_interp_find_specifier(_context, _name);
    if (index < 0)
    {
        return D_STR_INTERP_ERROR_SPECIFIER_NOT_FOUND;
    }

    // allocate and copy new value
    value_len  = strlen(_value);
    value_copy = malloc(value_len + 1);

    // check allocation
    if (!value_copy)
    {
        return D_STR_INTERP_ERROR_ALLOCATION_FAILED;
    }

    // copy the new value
    memcpy(value_copy, _value, value_len + 1);

    // free old value and update
    spec = &_context->specifiers[index];
    if (spec->value)
    {
        free(spec->value);
    }

    spec->value        = value_copy;
    spec->value_length = value_len;

    return D_STR_INTERP_SUCCESS;
}

/*
d_str_interp_get_specifier
  Retrieves the value of a specifier by name.

Parameter(s):
  _context: the context to search.
  _name:    the name of the specifier to find.
Return:
  The value string if found, or NULL if not found or parameters are invalid.
*/
const char*
d_str_interp_get_specifier
(
    const struct d_str_interp_context* _context,
    const char*                        _name
)
{
    int index;

    // validate parameters
    if ( (!_context) ||
         (!_name) )
    {
        return NULL;
    }

    // find and return the specifier value
    index = d_internal_interp_find_specifier(_context, _name);
    if (index < 0)
    {
        return NULL;
    }

    return _context->specifiers[index].value;
}

/*
d_str_interp_has_specifier
  Checks if a specifier exists in the context.

Parameter(s):
  _context: the context to search.
  _name:    the name of the specifier to find.
Return:
  true if the specifier exists, false otherwise.
*/
bool
d_str_interp_has_specifier
(
    const struct d_str_interp_context* _context,
    const char*                        _name
)
{
    // validate parameters
    if ( (!_context) ||
         (!_name) )
    {
        return false;
    }

    return d_internal_interp_find_specifier(_context, _name) >= 0;
}

/*
d_internal_interp_match_at
  Internal function to check whether any registered specifier name matches
the input string at the given position.

Parameter(s):
  _context:    the context containing specifiers to match against.
  _input:      the input string.
  _pos:        the position in the input to check.
  _input_len:  the total length of the input string.
Return:
  The index of the matching specifier, or -1 if none match. When multiple
specifiers could match at the same position, the longest match wins.
*/
static int
d_internal_interp_match_at
(
    const struct d_str_interp_context* _context,
    const char*                        _input,
    size_t                             _pos,
    size_t                             _input_len
)
{
    size_t remaining;
    int    best_index;
    size_t best_len;
    size_t i;

    remaining  = _input_len - _pos;
    best_index = -1;
    best_len   = 0;

    // check all specifiers for a match at this position
    for (i = 0; i < _context->specifier_count; i++)
    {
        // skip specifiers longer than remaining input
        if (_context->specifiers[i].name_length > remaining)
        {
            continue;
        }

        // prefer the longest matching specifier
        if (_context->specifiers[i].name_length <= best_len)
        {
            continue;
        }

        // check for match
        if (memcmp(&_input[_pos],
                   _context->specifiers[i].name,
                   _context->specifiers[i].name_length) == 0)
        {
            best_index = (int)i;
            best_len   = _context->specifiers[i].name_length;
        }
    }

    return best_index;
}

/*
d_str_interp_length
  Calculates the length of the interpolated string without performing the
substitution. Scans the input for exact occurrences of registered specifier
names and computes the total result length.

Parameter(s):
  _context: the interpolation context containing specifiers.
  _input:   the input string containing specifier names to replace.
  _length:  pointer to receive the calculated length (excluding null).
Return:
  D_STR_INTERP_SUCCESS on success, or an appropriate error code.
*/
enum d_str_interp_error
d_str_interp_length
(
    const struct d_str_interp_context* _context,
    const char*                        _input,
    size_t*                            _length
)
{
    size_t pos;
    size_t len;
    size_t input_len;
    int    spec_index;

    // validate parameters
    if ( (!_context) ||
         (!_input)   ||
         (!_length) )
    {
        return D_STR_INTERP_ERROR_NULL_PARAM;
    }

    // initialize
    pos       = 0;
    len       = 0;
    input_len = strlen(_input);

    // process input string
    while (pos < input_len)
    {
        // check for a specifier match at this position
        spec_index = d_internal_interp_match_at(_context,
                                                _input,
                                                pos,
                                                input_len);
        if (spec_index >= 0)
        {
            len += _context->specifiers[spec_index].value_length;
            pos += _context->specifiers[spec_index].name_length;

            continue;
        }

        // regular character
        len++;
        pos++;
    }

    *_length = len;

    return D_STR_INTERP_SUCCESS;
}

/*
d_str_interp
  Performs string interpolation by replacing all occurrences of registered
specifier names with their corresponding values.

Parameter(s):
  _context:     the interpolation context containing specifiers.
  _buffer:      destination buffer for the interpolated string.
  _buffer_size: size of the destination buffer in bytes.
  _input:       the input string containing specifier names to replace.
Return:
  D_STR_INTERP_SUCCESS on success, or an appropriate error code.
*/
enum d_str_interp_error
d_str_interp
(
    const struct d_str_interp_context* _context,
    char*                              _buffer,
    size_t                             _buffer_size,
    const char*                        _input
)
{
    size_t              pos;
    size_t              out_pos;
    size_t              input_len;
    size_t              required_len;
    int                 spec_index;
    enum d_str_interp_error err;

    // validate parameters
    if ( (!_context) ||
         (!_buffer)  ||
         (!_input) )
    {
        return D_STR_INTERP_ERROR_NULL_PARAM;
    }

    // check buffer size
    if (_buffer_size == 0)
    {
        return D_STR_INTERP_ERROR_BUFFER_TOO_SMALL;
    }

    // calculate required length
    err = d_str_interp_length(_context, _input, &required_len);
    if (err != D_STR_INTERP_SUCCESS)
    {
        return err;
    }

    // check if buffer is large enough
    if (required_len >= _buffer_size)
    {
        return D_STR_INTERP_ERROR_BUFFER_TOO_SMALL;
    }

    // initialize
    pos       = 0;
    out_pos   = 0;
    input_len = strlen(_input);

    // process input string
    while (pos < input_len)
    {
        // check for a specifier match at this position
        spec_index = d_internal_interp_match_at(_context,
                                                _input,
                                                pos,
                                                input_len);
        if (spec_index >= 0)
        {
            // copy specifier value
            memcpy(&_buffer[out_pos],
                   _context->specifiers[spec_index].value,
                   _context->specifiers[spec_index].value_length);
            out_pos += _context->specifiers[spec_index].value_length;
            pos     += _context->specifiers[spec_index].name_length;

            continue;
        }

        // regular character
        _buffer[out_pos++] = _input[pos++];
    }

    // null terminate
    _buffer[out_pos] = '\0';

    return D_STR_INTERP_SUCCESS;
}

/*
d_str_interp_alloc
  Performs string interpolation and allocates memory for the result.

Parameter(s):
  _context: the interpolation context containing specifiers.
  _input:   the input string containing specifier names to replace.
  _result:  pointer to receive the allocated result string. Caller must
            free the returned string.
Return:
  D_STR_INTERP_SUCCESS on success, or an appropriate error code.
*/
enum d_str_interp_error
d_str_interp_alloc
(
    const struct d_str_interp_context* _context,
    const char*                        _input,
    char**                             _result
)
{
    size_t              len;
    char*               buffer;
    enum d_str_interp_error err;

    // validate parameters
    if ( (!_context) ||
         (!_input)   ||
         (!_result) )
    {
        return D_STR_INTERP_ERROR_NULL_PARAM;
    }

    // calculate required length
    err = d_str_interp_length(_context, _input, &len);
    if (err != D_STR_INTERP_SUCCESS)
    {
        return err;
    }

    // allocate buffer
    buffer = malloc(len + 1);

    // check allocation
    if (!buffer)
    {
        return D_STR_INTERP_ERROR_ALLOCATION_FAILED;
    }

    // perform interpolation
    err = d_str_interp(_context,
                       buffer,
                       len + 1,
                       _input);

    // check for errors
    if (err != D_STR_INTERP_SUCCESS)
    {
        free(buffer);

        return err;
    }

    *_result = buffer;

    return D_STR_INTERP_SUCCESS;
}

/*
d_str_interp_quick
  Performs a single interpolation without explicit context management.

Parameter(s):
  _buffer:      destination buffer for the interpolated string.
  _buffer_size: size of the destination buffer in bytes.
  _input:       the input string containing specifier names to replace.
  _names:       null-terminated array of specifier names.
  _values:      null-terminated array of specifier values.
Return:
  D_STR_INTERP_SUCCESS on success, or an appropriate error code.
*/
enum d_str_interp_error
d_str_interp_quick
(
    char*        _buffer,
    size_t       _buffer_size,
    const char*  _input,
    const char** _names,
    const char** _values
)
{
    struct d_str_interp_context* context;
    size_t                      i;
    enum d_str_interp_error     err;

    // validate parameters
    if ( (!_buffer) ||
         (!_input)  ||
         (!_names)  ||
         (!_values) )
    {
        return D_STR_INTERP_ERROR_NULL_PARAM;
    }

    // create context
    context = d_str_interp_context_new();
    if (!context)
    {
        return D_STR_INTERP_ERROR_ALLOCATION_FAILED;
    }

    // add all specifiers
    for (i = 0; _names[i] != NULL && _values[i] != NULL; i++)
    {
        err = d_str_interp_add_specifier(context,
                                         _names[i],
                                         _values[i]);

        // check for errors
        if (err != D_STR_INTERP_SUCCESS)
        {
            d_str_interp_context_free(context);

            return err;
        }
    }

    // perform interpolation
    err = d_str_interp(context,
                       _buffer,
                       _buffer_size,
                       _input);

    d_str_interp_context_free(context);

    return err;
}

/*
d_str_interp_error_string
  Returns a human-readable string describing the given error code.

Parameter(s):
  _error: the error code to describe.
Return:
  A constant string describing the error.
*/
const char*
d_str_interp_error_string
(
    enum d_str_interp_error _error
)
{
    switch (_error)
    {
        case D_STR_INTERP_SUCCESS:
            return "success";

        case D_STR_INTERP_ERROR_NULL_PARAM:
            return "null parameter";

        case D_STR_INTERP_ERROR_BUFFER_TOO_SMALL:
            return "buffer too small";

        case D_STR_INTERP_ERROR_SPECIFIER_NOT_FOUND:
            return "specifier not found";

        case D_STR_INTERP_ERROR_TOO_MANY_SPECIFIERS:
            return "too many specifiers";

        case D_STR_INTERP_ERROR_ALLOCATION_FAILED:
            return "memory allocation failed";

        default:
            return "unknown error";
    }
}
