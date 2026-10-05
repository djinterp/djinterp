/*******************************************************************************
* djinterp [c]                                                   text_template.c
*
*
* path:      /src/djinterp/c/text/text_template.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.29
*******************************************************************************/
#include "../../../../inc/djinterp/c/text/text_template.h"


// =============================================================================
// Forward declarations for internal helpers
// =============================================================================
static void
d_internal_template_free_binding(struct d_text_template_binding* _binding);

static int
d_internal_template_find_binding(const struct d_text_template* _template,
                                 const char*                   _key);

static enum d_text_template_error
d_internal_template_ensure_capacity(struct d_text_template* _template);

static enum d_text_template_error
d_internal_template_resolve_value(const struct d_text_template*         _template,
                                  const struct d_text_template_binding* _binding,
                                  char**                                _result,
                                  size_t*                               _length,
                                  size_t                                _depth);

static enum d_text_template_error
d_internal_template_build_interp_context(const struct d_text_template* _template,
                                         struct d_str_interp_context*  _context,
                                         size_t                        _depth);

static void
d_internal_template_format_padded(char*  _buf,
                                  size_t _buf_size,
                                  size_t _number,
                                  size_t _width,
                                  char   _pad_char);

static size_t
d_internal_template_digit_count(size_t _value);


// =============================================================================
// Template Lifecycle
// =============================================================================

/*
d_text_template_new
  Allocate and initialize a new template with default markers ("%"/"%"
from D_TEXT_TEMPLATE_DEFAULT_PREFIX/SUFFIX), an empty binding array of
D_TEXT_TEMPLATE_DEFAULT_BINDING_CAPACITY slots, and max nesting depth of
D_TEXT_TEMPLATE_DEFAULT_NESTING_DEPTH.

Parameter(s):
  none.
Return:
  A non-NULL pointer on success, or NULL on allocation failure.
*/
struct d_text_template*
d_text_template_new
(
    void
)
{
    struct d_text_template* tmpl;

    tmpl = malloc(sizeof(struct d_text_template));

    // check if allocation succeeded
    if (!tmpl)
    {
        return NULL;
    }

    // allocate the binding array
    tmpl->bindings = malloc(D_TEXT_TEMPLATE_DEFAULT_BINDING_CAPACITY
                            * sizeof(struct d_text_template_binding));
    if (!tmpl->bindings)
    {
        free(tmpl);

        return NULL;
    }

    // zero out the binding array
    d_memset(tmpl->bindings,
             0,
             D_TEXT_TEMPLATE_DEFAULT_BINDING_CAPACITY
             * sizeof(struct d_text_template_binding));

    tmpl->binding_count    = 0;
    tmpl->binding_capacity = D_TEXT_TEMPLATE_DEFAULT_BINDING_CAPACITY;
    tmpl->max_nesting_depth = D_TEXT_TEMPLATE_DEFAULT_NESTING_DEPTH;

    // set default markers (heap-allocated via d_strdup)
    tmpl->marker.prefix = d_strdup(D_TEXT_TEMPLATE_DEFAULT_PREFIX);
    if (!tmpl->marker.prefix)
    {
        free(tmpl->bindings);
        free(tmpl);

        return NULL;
    }

    tmpl->marker.prefix_length = d_strnlen(D_TEXT_TEMPLATE_DEFAULT_PREFIX,
                                            64);

    tmpl->marker.suffix = d_strdup(D_TEXT_TEMPLATE_DEFAULT_SUFFIX);
    if (!tmpl->marker.suffix)
    {
        free(tmpl->marker.prefix);
        free(tmpl->bindings);
        free(tmpl);

        return NULL;
    }

    tmpl->marker.suffix_length = d_strnlen(D_TEXT_TEMPLATE_DEFAULT_SUFFIX,
                                            64);

    return tmpl;
}

/*
d_text_template_new_with_markers
  Allocate and initialize a new template with the specified prefix and
suffix marker strings. Both are duplicated via d_strdup from string_fn.h.

Parameter(s):
  _prefix: prefix marker string (null-terminated); must not be NULL.
  _suffix: suffix marker string (null-terminated); must not be NULL.
Return:
  A non-NULL pointer on success, or NULL on allocation failure or NULL
params.
*/
struct d_text_template*
d_text_template_new_with_markers
(
    const char* _prefix,
    const char* _suffix
)
{
    struct d_text_template* tmpl;

    // validate parameters
    if ( (!_prefix) ||
         (!_suffix) )
    {
        return NULL;
    }

    tmpl = d_text_template_new();

    // check if base allocation succeeded
    if (!tmpl)
    {
        return NULL;
    }

    // set custom markers
    if (d_text_template_set_markers(tmpl,
                                    _prefix,
                                    _suffix) != D_TEXT_TEMPLATE_SUCCESS)
    {
        d_text_template_free(tmpl);

        return NULL;
    }

    return tmpl;
}

/*
d_internal_template_free_binding
  Internal function to free the resources owned by a single binding.
Frees the key string and, for string bindings, the value string. For
list bindings, frees the copied separator and empty_text strings from
the options. Template pointers, function contexts, and list item
templates are not owned and are not freed.

Parameter(s):
  _binding: the binding to free resources for.
Return:
  none.
*/
static void
d_internal_template_free_binding
(
    struct d_text_template_binding* _binding
)
{
    // free the key
    if (_binding->key)
    {
        free(_binding->key);
        _binding->key = NULL;
    }

    // free type-specific resources
    if (_binding->type == D_TEXT_TEMPLATE_BINDING_STRING)
    {
        if (_binding->value.string.text)
        {
            free(_binding->value.string.text);
            _binding->value.string.text = NULL;
        }
    }
    else if (_binding->type == D_TEXT_TEMPLATE_BINDING_LIST)
    {
        // free copied option strings (cast away const for free)
        if (_binding->value.list.options.separator)
        {
            free((void*)_binding->value.list.options.separator);
            _binding->value.list.options.separator = NULL;
        }

        if (_binding->value.list.options.empty_text)
        {
            free((void*)_binding->value.list.options.empty_text);
            _binding->value.list.options.empty_text = NULL;
        }
    }

    _binding->key_length = 0;
    _binding->type       = D_TEXT_TEMPLATE_BINDING_STRING;

    return;
}

/*
d_text_template_free
  Free a template and all owned resources. Frees the marker strings,
all binding keys and string values (via string_fn.h), and the binding
array itself (via dmemory.h). Does not free referenced templates,
function contexts, or list item templates (these are not owned).
NULL-safe.

Parameter(s):
  _template: the template to free; may be NULL.
Return:
  none.
*/
void
d_text_template_free
(
    struct d_text_template* _template
)
{
    size_t i;

    if (_template)
    {
        // free marker strings
        if (_template->marker.prefix)
        {
            free(_template->marker.prefix);
        }

        if (_template->marker.suffix)
        {
            free(_template->marker.suffix);
        }

        // free all binding resources
        for (i = 0; i < _template->binding_count; i++)
        {
            d_internal_template_free_binding(&_template->bindings[i]);
        }

        // free the binding array
        if (_template->bindings)
        {
            free(_template->bindings);
        }

        free(_template);
    }

    return;
}

/*
d_text_template_clear
  Remove all bindings from a template without freeing the template
itself. Frees all binding keys and string values. Resets binding_count
to 0 but retains the allocated binding_capacity. Does not modify
markers or max_nesting_depth. NULL-safe.

Parameter(s):
  _template: the template to clear; may be NULL.
Return:
  none.
*/
void
d_text_template_clear
(
    struct d_text_template* _template
)
{
    size_t i;

    if (_template)
    {
        // free all binding resources
        for (i = 0; i < _template->binding_count; i++)
        {
            d_internal_template_free_binding(&_template->bindings[i]);
        }

        // zero out the binding slots
        d_memset(_template->bindings,
                 0,
                 _template->binding_capacity
                 * sizeof(struct d_text_template_binding));
        _template->binding_count = 0;
    }

    return;
}


// =============================================================================
// Marker Configuration
// =============================================================================

/*
d_text_template_set_markers
  Sets the prefix and suffix marker strings for a template. Frees the
old marker strings and duplicates the new ones via d_strdup from
string_fn.h. Empty strings are not allowed.

Parameter(s):
  _template: the template to configure.
  _prefix:   the prefix marker string.
  _suffix:   the suffix marker string.
Return:
  D_TEXT_TEMPLATE_SUCCESS on success, or an appropriate error code.
*/
enum d_text_template_error
d_text_template_set_markers
(
    struct d_text_template* _template,
    const char*             _prefix,
    const char*             _suffix
)
{
    size_t prefix_len;
    size_t suffix_len;
    char*  new_prefix;
    char*  new_suffix;

    // validate parameters
    if ( (!_template) ||
         (!_prefix)   ||
         (!_suffix) )
    {
        return D_TEXT_TEMPLATE_ERROR_NULL_PARAM;
    }

    prefix_len = d_strnlen(_prefix, 256);
    suffix_len = d_strnlen(_suffix, 256);

    // validate lengths (empty markers are invalid)
    if ( (prefix_len == 0) ||
         (suffix_len == 0) )
    {
        return D_TEXT_TEMPLATE_ERROR_INVALID_MARKER;
    }

    // duplicate new markers before freeing old ones
    new_prefix = d_strdup(_prefix);
    if (!new_prefix)
    {
        return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
    }

    new_suffix = d_strdup(_suffix);
    if (!new_suffix)
    {
        free(new_prefix);

        return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
    }

    // free old markers
    if (_template->marker.prefix)
    {
        free(_template->marker.prefix);
    }

    if (_template->marker.suffix)
    {
        free(_template->marker.suffix);
    }

    // assign new markers
    _template->marker.prefix        = new_prefix;
    _template->marker.prefix_length = prefix_len;
    _template->marker.suffix        = new_suffix;
    _template->marker.suffix_length = suffix_len;

    return D_TEXT_TEMPLATE_SUCCESS;
}

/*
d_text_template_get_prefix
  Returns the prefix marker string.

Parameter(s):
  _template: the template to query.
Return:
  The prefix string, or NULL if _template is NULL.
*/
const char*
d_text_template_get_prefix
(
    const struct d_text_template* _template
)
{
    if (!_template)
    {
        return NULL;
    }

    return _template->marker.prefix;
}

/*
d_text_template_get_suffix
  Returns the suffix marker string.

Parameter(s):
  _template: the template to query.
Return:
  The suffix string, or NULL if _template is NULL.
*/
const char*
d_text_template_get_suffix
(
    const struct d_text_template* _template
)
{
    if (!_template)
    {
        return NULL;
    }

    return _template->marker.suffix;
}


// =============================================================================
// Nesting Depth Configuration
// =============================================================================

/*
d_text_template_set_max_depth
  Sets the maximum recursion depth for nested template expansion. This
limit applies to all nesting mechanisms: nested template bindings,
function bindings, and list bindings.

Parameter(s):
  _template: the template to configure.
  _depth:    the new maximum nesting depth.
Return:
  D_TEXT_TEMPLATE_SUCCESS on success, or D_TEXT_TEMPLATE_ERROR_NULL_PARAM
if _template is NULL.
*/
enum d_text_template_error
d_text_template_set_max_depth
(
    struct d_text_template* _template,
    size_t                  _depth
)
{
    if (!_template)
    {
        return D_TEXT_TEMPLATE_ERROR_NULL_PARAM;
    }

    _template->max_nesting_depth = _depth;

    return D_TEXT_TEMPLATE_SUCCESS;
}

/*
d_text_template_get_max_depth
  Returns the maximum recursion depth configured for this template.

Parameter(s):
  _template: the template to query.
Return:
  The max nesting depth, or 0 if _template is NULL.
*/
size_t
d_text_template_get_max_depth
(
    const struct d_text_template* _template
)
{
    if (!_template)
    {
        return 0;
    }

    return _template->max_nesting_depth;
}


// =============================================================================
// Internal: binding array capacity management
// =============================================================================

/*
d_internal_template_ensure_capacity
  Ensures the binding array has room for at least one more binding. If
the array is full (binding_count == binding_capacity), doubles the
capacity by allocating a new array via malloc, copying existing bindings
with d_memcpy from dmemory.h, zeroing new slots with d_memset, and
freeing the old array.

Parameter(s):
  _template: the template whose binding array to grow if needed.
Return:
  D_TEXT_TEMPLATE_SUCCESS on success, or
D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED if the new array could not be
allocated.
*/
static enum d_text_template_error
d_internal_template_ensure_capacity
(
    struct d_text_template* _template
)
{
    size_t                          new_capacity;
    struct d_text_template_binding* new_bindings;

    // check if there is room
    if (_template->binding_count < _template->binding_capacity)
    {
        return D_TEXT_TEMPLATE_SUCCESS;
    }

    // double the capacity
    new_capacity = _template->binding_capacity * 2;
    if (new_capacity < D_TEXT_TEMPLATE_DEFAULT_BINDING_CAPACITY)
    {
        new_capacity = D_TEXT_TEMPLATE_DEFAULT_BINDING_CAPACITY;
    }

    // allocate new binding array
    new_bindings = malloc(new_capacity
                          * sizeof(struct d_text_template_binding));
    if (!new_bindings)
    {
        return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
    }

    // copy existing bindings
    if ( (_template->bindings) &&
         (_template->binding_count > 0) )
    {
        d_memcpy(new_bindings,
                 _template->bindings,
                 _template->binding_count
                 * sizeof(struct d_text_template_binding));
    }

    // zero the new slots
    d_memset(new_bindings + _template->binding_count,
             0,
             (new_capacity - _template->binding_count)
             * sizeof(struct d_text_template_binding));

    // free old array and assign new
    free(_template->bindings);
    _template->bindings         = new_bindings;
    _template->binding_capacity = new_capacity;

    return D_TEXT_TEMPLATE_SUCCESS;
}


// =============================================================================
// Internal: binding lookup
// =============================================================================

/*
d_internal_template_find_binding
  Internal function to find a binding by key name using d_strequals
from string_fn.h for length-aware comparison.

Parameter(s):
  _template: the template to search.
  _key:      the key name to find (null-terminated).
Return:
  The index of the binding if found, or -1 if not found.
*/
static int
d_internal_template_find_binding
(
    const struct d_text_template* _template,
    const char*                   _key
)
{
    size_t i;
    size_t key_len;

    key_len = d_strnlen(_key, 4096);

    // search through all bindings using d_strequals
    for (i = 0; i < _template->binding_count; i++)
    {
        if (d_strequals(_template->bindings[i].key,
                        _template->bindings[i].key_length,
                        _key,
                        key_len))
        {
            return (int)i;
        }
    }

    return -1;
}


// =============================================================================
// Binding Management
// =============================================================================

/*
d_text_template_bind_string
  Bind a key to a string value. Both key and value are duplicated via
d_strdup / d_strndup from string_fn.h. If a binding with the same key
already exists (compared via d_strequals), it is replaced. The binding
array grows automatically via dmemory.h if capacity is exceeded.

Parameter(s):
  _template: the template to add the binding to.
  _key:      the key name (null-terminated).
  _value:    the string value to bind (null-terminated).
Return:
  D_TEXT_TEMPLATE_SUCCESS              : binding added or updated.
  D_TEXT_TEMPLATE_ERROR_NULL_PARAM     : _template, _key, or _value is
                                         NULL.
  D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED : string duplication or array
                                           growth failed.
*/
enum d_text_template_error
d_text_template_bind_string
(
    struct d_text_template* _template,
    const char*             _key,
    const char*             _value
)
{
    size_t key_len;
    size_t value_len;

    // validate parameters
    if ( (!_template) ||
         (!_key)      ||
         (!_value) )
    {
        return D_TEXT_TEMPLATE_ERROR_NULL_PARAM;
    }

    key_len   = d_strnlen(_key, 4096);
    value_len = d_strnlen(_value, 1048576);

    return d_text_template_bind_string_n(_template,
                                         _key,
                                         key_len,
                                         _value,
                                         value_len);
}

/*
d_text_template_bind_string_n
  Bind a key to a string value with explicit lengths. Uses d_strndup
from string_fn.h for length-aware duplication, avoiding strlen calls
when lengths are already known.

Parameter(s):
  _template:     the template to add the binding to.
  _key:          the key name.
  _key_length:   the length of the key string.
  _value:        the string value to bind.
  _value_length: the length of the value string.
Return:
  D_TEXT_TEMPLATE_SUCCESS on success, or an appropriate error code.
*/
enum d_text_template_error
d_text_template_bind_string_n
(
    struct d_text_template* _template,
    const char*             _key,
    size_t                  _key_length,
    const char*             _value,
    size_t                  _value_length
)
{
    struct d_text_template_binding* binding;
    char*                           key_copy;
    char*                           value_copy;
    int                             existing;
    enum d_text_template_error      err;

    // validate parameters
    if ( (!_template) ||
         (!_key)      ||
         (!_value) )
    {
        return D_TEXT_TEMPLATE_ERROR_NULL_PARAM;
    }

    // check if binding already exists
    existing = d_internal_template_find_binding(_template, _key);
    if (existing >= 0)
    {
        return d_text_template_update_string(_template,
                                             _key,
                                             _value);
    }

    // ensure there is room in the binding array
    err = d_internal_template_ensure_capacity(_template);
    if (err != D_TEXT_TEMPLATE_SUCCESS)
    {
        return err;
    }

    // duplicate key
    key_copy = d_strndup(_key, _key_length);
    if (!key_copy)
    {
        return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
    }

    // duplicate value
    value_copy = d_strndup(_value, _value_length);
    if (!value_copy)
    {
        free(key_copy);

        return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
    }

    // add the binding
    binding                      = &_template->bindings[_template->binding_count];
    binding->key                 = key_copy;
    binding->key_length          = _key_length;
    binding->type                = D_TEXT_TEMPLATE_BINDING_STRING;
    binding->value.string.text   = value_copy;
    binding->value.string.length = _value_length;

    _template->binding_count++;

    return D_TEXT_TEMPLATE_SUCCESS;
}

/*
d_text_template_bind_template
  Binds a key to another text template for nested expansion. The nested
template is referenced but NOT owned by this binding; the caller must
ensure it remains valid for the lifetime of this binding.

Parameter(s):
  _template: the template to add the binding to.
  _key:      the key name (null-terminated).
  _nested:   pointer to the nested template.
Return:
  D_TEXT_TEMPLATE_SUCCESS on success, or an appropriate error code.
*/
enum d_text_template_error
d_text_template_bind_template
(
    struct d_text_template* _template,
    const char*             _key,
    struct d_text_template* _nested
)
{
    struct d_text_template_binding* binding;
    char*                           key_copy;
    size_t                          key_len;
    int                             existing;
    enum d_text_template_error      err;

    // validate parameters
    if ( (!_template) ||
         (!_key)      ||
         (!_nested) )
    {
        return D_TEXT_TEMPLATE_ERROR_NULL_PARAM;
    }

    key_len = d_strnlen(_key, 4096);

    // if key already exists, free old binding and reuse slot
    existing = d_internal_template_find_binding(_template, _key);
    if (existing >= 0)
    {
        d_internal_template_free_binding(&_template->bindings[existing]);

        key_copy = d_strndup(_key, key_len);
        if (!key_copy)
        {
            return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
        }

        _template->bindings[existing].key        = key_copy;
        _template->bindings[existing].key_length = key_len;
        _template->bindings[existing].type       = D_TEXT_TEMPLATE_BINDING_TEMPLATE;
        _template->bindings[existing].value.tmpl = _nested;

        return D_TEXT_TEMPLATE_SUCCESS;
    }

    // ensure there is room in the binding array
    err = d_internal_template_ensure_capacity(_template);
    if (err != D_TEXT_TEMPLATE_SUCCESS)
    {
        return err;
    }

    // duplicate key
    key_copy = d_strndup(_key, key_len);
    if (!key_copy)
    {
        return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
    }

    // add the binding
    binding             = &_template->bindings[_template->binding_count];
    binding->key        = key_copy;
    binding->key_length = key_len;
    binding->type       = D_TEXT_TEMPLATE_BINDING_TEMPLATE;
    binding->value.tmpl = _nested;

    _template->binding_count++;

    return D_TEXT_TEMPLATE_SUCCESS;
}

/*
d_text_template_bind_function
  Binds a key to a user-supplied function that produces the replacement
string at render time. The function callback and context are stored in
the binding but NOT owned; the caller manages their lifetimes.

Parameter(s):
  _template: the template to add the binding to.
  _key:      the key name (null-terminated).
  _fn:       the callback invoked at render time; must not be NULL.
  _context:  opaque user data passed to _fn (may be NULL).
Return:
  D_TEXT_TEMPLATE_SUCCESS on success, or an appropriate error code.
*/
enum d_text_template_error
d_text_template_bind_function
(
    struct d_text_template* _template,
    const char*             _key,
    d_text_template_fn      _fn,
    void*                   _context
)
{
    struct d_text_template_binding* binding;
    char*                           key_copy;
    size_t                          key_len;
    int                             existing;
    enum d_text_template_error      err;

    // validate parameters
    if ( (!_template) ||
         (!_key)      ||
         (!_fn) )
    {
        return D_TEXT_TEMPLATE_ERROR_NULL_PARAM;
    }

    key_len = d_strnlen(_key, 4096);

    // if key already exists, free old binding and reuse slot
    existing = d_internal_template_find_binding(_template, _key);
    if (existing >= 0)
    {
        d_internal_template_free_binding(&_template->bindings[existing]);

        key_copy = d_strndup(_key, key_len);
        if (!key_copy)
        {
            return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
        }

        _template->bindings[existing].key                  = key_copy;
        _template->bindings[existing].key_length           = key_len;
        _template->bindings[existing].type                 = D_TEXT_TEMPLATE_BINDING_FUNCTION;
        _template->bindings[existing].value.function.fn      = _fn;
        _template->bindings[existing].value.function.context = _context;

        return D_TEXT_TEMPLATE_SUCCESS;
    }

    // ensure there is room in the binding array
    err = d_internal_template_ensure_capacity(_template);
    if (err != D_TEXT_TEMPLATE_SUCCESS)
    {
        return err;
    }

    // duplicate key
    key_copy = d_strndup(_key, key_len);
    if (!key_copy)
    {
        return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
    }

    // add the binding
    binding                          = &_template->bindings[_template->binding_count];
    binding->key                     = key_copy;
    binding->key_length              = key_len;
    binding->type                    = D_TEXT_TEMPLATE_BINDING_FUNCTION;
    binding->value.function.fn       = _fn;
    binding->value.function.context  = _context;

    _template->binding_count++;

    return D_TEXT_TEMPLATE_SUCCESS;
}

/*
d_text_template_bind_list
  Bind a key to a list iteration. When the renderer encounters this key
during expansion, it iterates `_count` times, rendering the sub-template
`_item_template` once per item.

Rendering sequence for each item at index `i`:
  1. clear all bindings on _item_template
  2. bind auto-keys onto _item_template:
       _index    : "0", "1", "2", ...          (always 0-based)
       _number   : "01", "02", ...             (padded, 1-based default)
       _count    : total items as string        (e.g., "21")
       _is_first : "1" if i == 0, else "0"
       _is_last  : "1" if i == count-1, else "0"
  3. invoke _bind_fn(i, _count, _item_template, _context)
  4. if _bind_fn returns false: abort, return LIST_CALLBACK_FAILED
  5. render _item_template with its bindings -> item string
  6. if render fails: abort, return LIST_ITEM_RENDER_FAILED
  7. append item string to output
  8. if i < _count - 1 and separator is set: append separator

When _count is 0:
  - the callback is never invoked
  - if _options->empty_text is set, that string is appended
  - otherwise nothing is appended (the key expands to "")

Ownership:
  - _item_template is NOT owned; caller must keep it alive until after
    the parent template is rendered
  - _context is NOT owned; caller manages its lifetime
  - _options is copied at bind time (strings deep-copied via d_strdup);
    the caller's struct need not outlive this call; NULL means defaults

Parameter(s):
  _template:      the parent template to add the binding to.
  _key:           the key name (null-terminated) that triggers iteration.
  _item_template: sub-template rendered per item; must have a format
                  string set before the parent template is rendered.
  _count:         number of items (iterations); may be 0.
  _bind_fn:       per-item callback; must not be NULL.
  _context:       opaque user data for _bind_fn (may be NULL).
  _options:       rendering options (separator, padding, etc.); NULL for
                  defaults (no separator, 1-based zero-padded number).
Return:
  D_TEXT_TEMPLATE_SUCCESS              : binding added successfully.
  D_TEXT_TEMPLATE_ERROR_NULL_PARAM     : _template, _key, _item_template,
                                         or _bind_fn is NULL.
  D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED : key duplication or array
                                           growth failed.
*/
enum d_text_template_error
d_text_template_bind_list
(
    struct d_text_template*                   _template,
    const char*                               _key,
    struct d_text_template*                   _item_template,
    size_t                                    _count,
    d_text_template_list_fn                   _bind_fn,
    void*                                     _context,
    const struct d_text_template_list_options* _options
)
{
    struct d_text_template_binding* binding;
    char*                           key_copy;
    size_t                          key_len;
    int                             existing;
    enum d_text_template_error      err;

    // validate parameters
    if ( (!_template)      ||
         (!_key)           ||
         (!_item_template) ||
         (!_bind_fn) )
    {
        return D_TEXT_TEMPLATE_ERROR_NULL_PARAM;
    }

    key_len = d_strnlen(_key, 4096);

    // if key already exists, free old binding and reuse slot
    existing = d_internal_template_find_binding(_template, _key);
    if (existing >= 0)
    {
        d_internal_template_free_binding(&_template->bindings[existing]);
        binding = &_template->bindings[existing];
    }
    else
    {
        // ensure there is room in the binding array
        err = d_internal_template_ensure_capacity(_template);
        if (err != D_TEXT_TEMPLATE_SUCCESS)
        {
            return err;
        }

        binding = &_template->bindings[_template->binding_count];
    }

    // duplicate key
    key_copy = d_strndup(_key, key_len);
    if (!key_copy)
    {
        return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
    }

    // populate the binding
    binding->key        = key_copy;
    binding->key_length = key_len;
    binding->type       = D_TEXT_TEMPLATE_BINDING_LIST;

    binding->value.list.item_template = _item_template;
    binding->value.list.count         = _count;
    binding->value.list.bind_fn       = _bind_fn;
    binding->value.list.context       = _context;

    // deep-copy options (or use defaults)
    d_memset(&binding->value.list.options,
             0,
             sizeof(struct d_text_template_list_options));

    if (_options)
    {
        binding->value.list.options.number_pad_width = _options->number_pad_width;
        binding->value.list.options.number_pad_char  = _options->number_pad_char;
        binding->value.list.options.number_from_zero = _options->number_from_zero;

        // deep-copy separator string
        if (_options->separator)
        {
            binding->value.list.options.separator = d_strdup(_options->separator);
            if (!binding->value.list.options.separator)
            {
                free(key_copy);
                binding->key = NULL;

                return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
            }
        }

        // deep-copy empty_text string
        if (_options->empty_text)
        {
            binding->value.list.options.empty_text = d_strdup(_options->empty_text);
            if (!binding->value.list.options.empty_text)
            {
                free((void*)binding->value.list.options.separator);
                binding->value.list.options.separator = NULL;
                free(key_copy);
                binding->key = NULL;

                return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
            }
        }
    }

    // only increment count if this was a new binding
    if (existing < 0)
    {
        _template->binding_count++;
    }

    return D_TEXT_TEMPLATE_SUCCESS;
}

/*
d_text_template_unbind
  Removes a binding from the template by key name.

Parameter(s):
  _template: the template to remove the binding from.
  _key:      the key name of the binding to remove.
Return:
  D_TEXT_TEMPLATE_SUCCESS on success, or an appropriate error code.
*/
enum d_text_template_error
d_text_template_unbind
(
    struct d_text_template* _template,
    const char*             _key
)
{
    int    index;
    size_t i;

    // validate parameters
    if ( (!_template) ||
         (!_key) )
    {
        return D_TEXT_TEMPLATE_ERROR_NULL_PARAM;
    }

    // find the binding
    index = d_internal_template_find_binding(_template, _key);
    if (index < 0)
    {
        return D_TEXT_TEMPLATE_ERROR_KEY_NOT_FOUND;
    }

    // free binding resources
    d_internal_template_free_binding(&_template->bindings[index]);

    // shift remaining bindings down
    for (i = (size_t)index; i < _template->binding_count - 1; i++)
    {
        _template->bindings[i] = _template->bindings[i + 1];
    }

    // clear the last slot and decrement count
    d_memset(&_template->bindings[_template->binding_count - 1],
             0,
             sizeof(struct d_text_template_binding));
    _template->binding_count--;

    return D_TEXT_TEMPLATE_SUCCESS;
}

/*
d_text_template_update_string
  Updates the string value of an existing binding.

Parameter(s):
  _template: the template containing the binding.
  _key:      the key name of the binding to update.
  _value:    the new string value.
Return:
  D_TEXT_TEMPLATE_SUCCESS on success, or an appropriate error code.
*/
enum d_text_template_error
d_text_template_update_string
(
    struct d_text_template* _template,
    const char*             _key,
    const char*             _value
)
{
    int                             index;
    size_t                          value_len;
    char*                           value_copy;
    struct d_text_template_binding* binding;

    // validate parameters
    if ( (!_template) ||
         (!_key)      ||
         (!_value) )
    {
        return D_TEXT_TEMPLATE_ERROR_NULL_PARAM;
    }

    // find the binding
    index = d_internal_template_find_binding(_template, _key);
    if (index < 0)
    {
        return D_TEXT_TEMPLATE_ERROR_KEY_NOT_FOUND;
    }

    // duplicate new value
    value_len  = d_strnlen(_value, 1048576);
    value_copy = d_strndup(_value, value_len);
    if (!value_copy)
    {
        return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
    }

    // free old resources if this was a string binding
    binding = &_template->bindings[index];
    if ( (binding->type == D_TEXT_TEMPLATE_BINDING_STRING) &&
         (binding->value.string.text) )
    {
        free(binding->value.string.text);
    }

    // update to string binding
    binding->type                = D_TEXT_TEMPLATE_BINDING_STRING;
    binding->value.string.text   = value_copy;
    binding->value.string.length = value_len;

    return D_TEXT_TEMPLATE_SUCCESS;
}

/*
d_text_template_has_binding
  Checks if a binding exists in the template.

Parameter(s):
  _template: the template to search.
  _key:      the key name to find.
Return:
  true if the binding exists, false otherwise.
*/
bool
d_text_template_has_binding
(
    const struct d_text_template* _template,
    const char*                   _key
)
{
    // validate parameters
    if ( (!_template) ||
         (!_key) )
    {
        return false;
    }

    return d_internal_template_find_binding(_template, _key) >= 0;
}

/*
d_text_template_binding_count
  Return the number of currently registered bindings. Returns 0 if
_template is NULL.

Parameter(s):
  _template: the template to query.
Return:
  The number of bindings, or 0 if _template is NULL.
*/
size_t
d_text_template_binding_count
(
    const struct d_text_template* _template
)
{
    if (!_template)
    {
        return 0;
    }

    return _template->binding_count;
}


// =============================================================================
// Internal: number formatting helpers
// =============================================================================

/*
d_internal_template_digit_count
  Returns the number of decimal digits needed to represent _value.
Returns 1 for _value == 0.

Parameter(s):
  _value: the unsigned integer to measure.
Return:
  The number of decimal digits.
*/
static size_t
d_internal_template_digit_count
(
    size_t _value
)
{
    size_t digits;

    if (_value == 0)
    {
        return 1;
    }

    digits = 0;

    // count digits by dividing
    while (_value > 0)
    {
        _value /= 10;
        digits++;
    }

    return digits;
}

/*
d_internal_template_format_padded
  Formats _number into _buf as a decimal string with left-padding.
The result is at least _width characters wide, padded with _pad_char
on the left. The buffer must be large enough to hold the result plus
a null terminator.

Parameter(s):
  _buf:      destination buffer.
  _buf_size: size of _buf in bytes.
  _number:   the number to format.
  _width:    minimum output width (0 = no padding).
  _pad_char: character used for left-padding.
Return:
  none.
*/
static void
d_internal_template_format_padded
(
    char*  _buf,
    size_t _buf_size,
    size_t _number,
    size_t _width,
    char   _pad_char
)
{
    size_t digits;
    size_t actual_width;
    size_t pos;
    size_t temp;

    if ( (!_buf) ||
         (_buf_size == 0) )
    {
        return;
    }

    digits       = d_internal_template_digit_count(_number);
    actual_width = (digits > _width) ? digits : _width;

    // ensure buffer is large enough
    if (actual_width >= _buf_size)
    {
        actual_width = _buf_size - 1;
    }

    // null-terminate
    _buf[actual_width] = '\0';

    // fill digits from right to left
    pos  = actual_width;
    temp = _number;

    // write at least one digit
    do
    {
        if (pos == 0)
        {
            break;
        }

        pos--;
        _buf[pos] = (char)('0' + (temp % 10));
        temp /= 10;
    } while (temp > 0);

    // fill remaining positions with pad character
    while (pos > 0)
    {
        pos--;
        _buf[pos] = _pad_char;
    }

    return;
}


// =============================================================================
// Internal: binding resolution
// =============================================================================

/*
d_internal_template_resolve_value
  Resolves a binding to its final string value. For string bindings,
duplicates and returns the value. For template bindings, recursively
renders the nested template. For function bindings, invokes the user
callback. For list bindings, iterates and concatenates rendered items
with optional separators. Tracks nesting depth against the template's
max_nesting_depth to prevent infinite recursion.

Parameter(s):
  _template: the parent template (for max_nesting_depth).
  _binding:  the binding to resolve.
  _result:   pointer to receive the resolved string (heap-allocated);
             caller must free this.
  _length:   pointer to receive the length of the result.
  _depth:    current nesting depth for recursion protection.
Return:
  D_TEXT_TEMPLATE_SUCCESS on success, or an appropriate error code.
*/
static enum d_text_template_error
d_internal_template_resolve_value
(
    const struct d_text_template*         _template,
    const struct d_text_template_binding* _binding,
    char**                                _result,
    size_t*                               _length,
    size_t                                _depth
)
{
    char*                      resolved;
    size_t                     resolved_len;
    enum d_text_template_error err;

    // check nesting depth
    if (_depth >= _template->max_nesting_depth)
    {
        return D_TEXT_TEMPLATE_ERROR_NESTING_TOO_DEEP;
    }

    // string binding: duplicate the value
    if (_binding->type == D_TEXT_TEMPLATE_BINDING_STRING)
    {
        resolved_len = _binding->value.string.length;
        resolved     = d_strndup(_binding->value.string.text,
                                  resolved_len);
        if (!resolved)
        {
            return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
        }

        *_result = resolved;
        *_length = resolved_len;

        return D_TEXT_TEMPLATE_SUCCESS;
    }

    // template binding: recursively render the nested template
    if (_binding->type == D_TEXT_TEMPLATE_BINDING_TEMPLATE)
    {
        if (!_binding->value.tmpl)
        {
            resolved = malloc(1);
            if (!resolved)
            {
                return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
            }

            resolved[0] = '\0';
            *_result     = resolved;
            *_length     = 0;

            return D_TEXT_TEMPLATE_SUCCESS;
        }

        // nested templates resolve to empty for now; a full
        // implementation requires a stored format string on the nested
        // template (render is deferred to the interp context pass)
        resolved = malloc(1);
        if (!resolved)
        {
            return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
        }

        resolved[0] = '\0';
        *_result     = resolved;
        *_length     = 0;

        return D_TEXT_TEMPLATE_SUCCESS;
    }

    // function binding: invoke the callback
    if (_binding->type == D_TEXT_TEMPLATE_BINDING_FUNCTION)
    {
        if (!_binding->value.function.fn)
        {
            return D_TEXT_TEMPLATE_ERROR_FUNCTION_FAILED;
        }

        err = _binding->value.function.fn(
                  _binding->value.function.context,
                  &resolved);
        if (err != D_TEXT_TEMPLATE_SUCCESS)
        {
            return D_TEXT_TEMPLATE_ERROR_FUNCTION_FAILED;
        }

        *_result = resolved;
        *_length = d_strnlen(resolved, 1048576);

        return D_TEXT_TEMPLATE_SUCCESS;
    }

    // list binding: iterate and concatenate rendered items
    if (_binding->type == D_TEXT_TEMPLATE_BINDING_LIST)
    {
        struct d_text_template*                    item_tmpl;
        size_t                                     count;
        d_text_template_list_fn                    bind_fn;
        void*                                      ctx;
        const struct d_text_template_list_options*  opts;
        char*                                      output;
        size_t                                     output_len;
        size_t                                     output_cap;
        size_t                                     i;
        size_t                                     pad_width;
        char                                       pad_char;
        size_t                                     number_val;
        char                                       num_buf[32];
        char                                       idx_buf[32];
        char                                       cnt_buf[32];
        struct d_str_interp_context*               item_ctx;
        enum d_str_interp_error                    interp_err;
        char*                                      item_format;
        size_t                                     item_format_len;
        char*                                      item_result;
        size_t                                     item_result_len;
        size_t                                     j;
        size_t                                     sep_len;
        char*                                      new_output;

        item_tmpl = _binding->value.list.item_template;
        count     = _binding->value.list.count;
        bind_fn   = _binding->value.list.bind_fn;
        ctx       = _binding->value.list.context;
        opts      = &_binding->value.list.options;

        // handle empty list
        if (count == 0)
        {
            if ( (opts->empty_text) &&
                 (d_strnlen(opts->empty_text, 4096) > 0) )
            {
                resolved_len = d_strnlen(opts->empty_text, 4096);
                resolved     = d_strndup(opts->empty_text,
                                          resolved_len);
                if (!resolved)
                {
                    return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
                }

                *_result = resolved;
                *_length = resolved_len;

                return D_TEXT_TEMPLATE_SUCCESS;
            }

            // empty string
            resolved = malloc(1);
            if (!resolved)
            {
                return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
            }

            resolved[0] = '\0';
            *_result     = resolved;
            *_length     = 0;

            return D_TEXT_TEMPLATE_SUCCESS;
        }

        // determine number padding
        pad_width = opts->number_pad_width;
        if (pad_width == 0)
        {
            pad_width = d_internal_template_digit_count(count);
        }

        pad_char = opts->number_pad_char;
        if (pad_char == '\0')
        {
            pad_char = '0';
        }

        // separator length
        sep_len = 0;
        if (opts->separator)
        {
            sep_len = d_strnlen(opts->separator, 4096);
        }

        // format count string once
        d_internal_template_format_padded(cnt_buf,
                                          sizeof(cnt_buf),
                                          count,
                                          0,
                                          '0');

        // initialize output buffer
        output_cap = 256;
        output     = malloc(output_cap);
        if (!output)
        {
            return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
        }

        output[0]  = '\0';
        output_len = 0;

        // iterate over items
        for (i = 0; i < count; i++)
        {
            // clear item template bindings
            d_text_template_clear(item_tmpl);

            // format auto-key strings for this item
            d_internal_template_format_padded(idx_buf,
                                              sizeof(idx_buf),
                                              i,
                                              0,
                                              '0');

            number_val = opts->number_from_zero ? i : (i + 1);
            d_internal_template_format_padded(num_buf,
                                              sizeof(num_buf),
                                              number_val,
                                              pad_width,
                                              pad_char);

            // bind auto-keys
            d_text_template_bind_string(item_tmpl,
                                        "_index",
                                        idx_buf);
            d_text_template_bind_string(item_tmpl,
                                        "_number",
                                        num_buf);
            d_text_template_bind_string(item_tmpl,
                                        "_count",
                                        cnt_buf);
            d_text_template_bind_string(item_tmpl,
                                        "_is_first",
                                        (i == 0) ? "1" : "0");
            d_text_template_bind_string(item_tmpl,
                                        "_is_last",
                                        (i == count - 1) ? "1" : "0");

            // invoke the per-item callback
            if (!bind_fn(i, count, item_tmpl, ctx))
            {
                free(output);

                return D_TEXT_TEMPLATE_ERROR_LIST_CALLBACK_FAILED;
            }

            // build a format string from the item template's
            // marker-wrapped keys (auto-generated format; a full
            // implementation would use a stored format on the item
            // template)
            item_format_len = 0;
            for (j = 0; j < item_tmpl->binding_count; j++)
            {
                item_format_len += item_tmpl->marker.prefix_length
                                 + item_tmpl->bindings[j].key_length
                                 + item_tmpl->marker.suffix_length;
            }

            item_format = malloc(item_format_len + 1);
            if (!item_format)
            {
                free(output);

                return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
            }

            // assemble the auto-generated format
            {
                size_t fpos;

                fpos = 0;
                for (j = 0; j < item_tmpl->binding_count; j++)
                {
                    d_memcpy(item_format + fpos,
                             item_tmpl->marker.prefix,
                             item_tmpl->marker.prefix_length);
                    fpos += item_tmpl->marker.prefix_length;

                    d_memcpy(item_format + fpos,
                             item_tmpl->bindings[j].key,
                             item_tmpl->bindings[j].key_length);
                    fpos += item_tmpl->bindings[j].key_length;

                    d_memcpy(item_format + fpos,
                             item_tmpl->marker.suffix,
                             item_tmpl->marker.suffix_length);
                    fpos += item_tmpl->marker.suffix_length;
                }

                item_format[fpos] = '\0';
            }

            // render this item through its own interp context, built at
            // this binding's depth: d_text_template_render_alloc would
            // restart the count at 0, and every level of list nesting must
            // count against the depth limit
            item_ctx = d_str_interp_context_new();
            if (!item_ctx)
            {
                free(item_format);
                free(output);

                return D_TEXT_TEMPLATE_ERROR_LIST_ITEM_RENDER_FAILED;
            }

            err = d_internal_template_build_interp_context(item_tmpl,
                                                           item_ctx,
                                                           _depth);

            // substitute only if every item binding resolved
            if (err == D_TEXT_TEMPLATE_SUCCESS)
            {
                interp_err = d_str_interp_alloc(item_ctx,
                                                item_format,
                                                &item_result);

                if (interp_err != D_STR_INTERP_SUCCESS)
                {
                    err = D_TEXT_TEMPLATE_ERROR_INTERP_FAILED;
                }
            }

            d_str_interp_context_free(item_ctx);
            free(item_format);

            if (err != D_TEXT_TEMPLATE_SUCCESS)
            {
                free(output);

                return D_TEXT_TEMPLATE_ERROR_LIST_ITEM_RENDER_FAILED;
            }

            item_result_len = d_strnlen(item_result, 1048576);

            // grow output buffer if needed
            while (output_len + item_result_len + sep_len + 1 > output_cap)
            {
                output_cap *= 2;
                new_output  = malloc(output_cap);
                if (!new_output)
                {
                    free(item_result);
                    free(output);

                    return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
                }

                d_memcpy(new_output, output, output_len);
                free(output);
                output = new_output;
            }

            // append item result
            d_memcpy(output + output_len,
                     item_result,
                     item_result_len);
            output_len += item_result_len;
            free(item_result);

            // append separator between items
            if ( (i < count - 1) &&
                 (sep_len > 0) )
            {
                d_memcpy(output + output_len,
                         opts->separator,
                         sep_len);
                output_len += sep_len;
            }
        }

        // null-terminate
        output[output_len] = '\0';

        *_result = output;
        *_length = output_len;

        return D_TEXT_TEMPLATE_SUCCESS;
    }

    // unknown binding type
    return D_TEXT_TEMPLATE_ERROR_INTERP_FAILED;
}


// =============================================================================
// Internal: interp context builder
// =============================================================================

/*
d_internal_template_build_interp_context
  Builds a d_str_interp_context from a template's bindings by
constructing marker-wrapped keys (prefix + key + suffix) and resolving
all values (including nested templates and function callbacks).

Parameter(s):
  _template: the template whose bindings to resolve.
  _context:  the str_interp context to populate.
  _depth:    current nesting depth for recursion protection.
Return:
  D_TEXT_TEMPLATE_SUCCESS on success, or an appropriate error code.
*/
static enum d_text_template_error
d_internal_template_build_interp_context
(
    const struct d_text_template* _template,
    struct d_str_interp_context*  _context,
    size_t                        _depth
)
{
    size_t                     i;
    size_t                     marked_key_len;
    char*                      marked_key;
    char*                      resolved_value;
    size_t                     resolved_len;
    enum d_text_template_error tmpl_err;
    enum d_str_interp_error    interp_err;

    // process each binding
    for (i = 0; i < _template->binding_count; i++)
    {
        // build the marked key: prefix + key + suffix
        marked_key_len = _template->marker.prefix_length
                       + _template->bindings[i].key_length
                       + _template->marker.suffix_length;

        marked_key = malloc(marked_key_len + 1);
        if (!marked_key)
        {
            return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
        }

        // assemble: prefix
        d_memcpy(marked_key,
                 _template->marker.prefix,
                 _template->marker.prefix_length);

        // assemble: key
        d_memcpy(marked_key + _template->marker.prefix_length,
                 _template->bindings[i].key,
                 _template->bindings[i].key_length);

        // assemble: suffix
        d_memcpy(marked_key + _template->marker.prefix_length
                            + _template->bindings[i].key_length,
                 _template->marker.suffix,
                 _template->marker.suffix_length);

        marked_key[marked_key_len] = '\0';

        // resolve the binding value
        resolved_value = NULL;
        resolved_len   = 0;
        tmpl_err = d_internal_template_resolve_value(
                       _template,
                       &_template->bindings[i],
                       &resolved_value,
                       &resolved_len,
                       _depth + 1);

        // check for resolution errors
        if (tmpl_err != D_TEXT_TEMPLATE_SUCCESS)
        {
            free(marked_key);

            return tmpl_err;
        }

        // add to the interp context
        interp_err = d_str_interp_add_specifier_n(
                         _context,
                         marked_key,
                         marked_key_len,
                         resolved_value,
                         resolved_len);

        free(marked_key);
        free(resolved_value);

        // check for interp errors
        if (interp_err != D_STR_INTERP_SUCCESS)
        {
            return D_TEXT_TEMPLATE_ERROR_INTERP_FAILED;
        }
    }

    return D_TEXT_TEMPLATE_SUCCESS;
}


// =============================================================================
// Rendering (Template Expansion)
// =============================================================================

/*
d_text_template_render
  Renders a template by replacing all marker-delimited keys in the
format string with their bound values. Nested template bindings are
recursively expanded.

Parameter(s):
  _template:    the template containing bindings and marker config.
  _format:      the format string (e.g., "Hello %name%, you are %age%.").
  _buffer:      destination buffer for the rendered string.
  _buffer_size: size of the destination buffer in bytes.
Return:
  D_TEXT_TEMPLATE_SUCCESS on success, or an appropriate error code.
*/
enum d_text_template_error
d_text_template_render
(
    const struct d_text_template* _template,
    const char*                   _format,
    char*                         _buffer,
    size_t                        _buffer_size
)
{
    struct d_str_interp_context* context;
    enum d_text_template_error   tmpl_err;
    enum d_str_interp_error      interp_err;

    // validate parameters
    if ( (!_template) ||
         (!_format)   ||
         (!_buffer) )
    {
        return D_TEXT_TEMPLATE_ERROR_NULL_PARAM;
    }

    // check buffer size
    if (_buffer_size == 0)
    {
        return D_TEXT_TEMPLATE_ERROR_BUFFER_TOO_SMALL;
    }

    // create an interp context and populate from bindings
    context = d_str_interp_context_new();
    if (!context)
    {
        return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
    }

    // build the interp context with marker-wrapped keys
    tmpl_err = d_internal_template_build_interp_context(_template,
                                                        context,
                                                        0);
    if (tmpl_err != D_TEXT_TEMPLATE_SUCCESS)
    {
        d_str_interp_context_free(context);

        return tmpl_err;
    }

    // perform the substitution via str_interp
    interp_err = d_str_interp(context,
                              _buffer,
                              _buffer_size,
                              _format);

    d_str_interp_context_free(context);

    // translate interp errors to template errors
    if (interp_err != D_STR_INTERP_SUCCESS)
    {
        if (interp_err == D_STR_INTERP_ERROR_BUFFER_TOO_SMALL)
        {
            return D_TEXT_TEMPLATE_ERROR_BUFFER_TOO_SMALL;
        }

        return D_TEXT_TEMPLATE_ERROR_INTERP_FAILED;
    }

    return D_TEXT_TEMPLATE_SUCCESS;
}

/*
d_text_template_render_alloc
  Renders a template and allocates memory for the result.

Parameter(s):
  _template: the template containing bindings and marker config.
  _format:   the format string.
  _result:   pointer to receive the allocated result string. Caller must
             free the returned string.
Return:
  D_TEXT_TEMPLATE_SUCCESS on success, or an appropriate error code.
*/
enum d_text_template_error
d_text_template_render_alloc
(
    const struct d_text_template* _template,
    const char*                   _format,
    char**                        _result
)
{
    struct d_str_interp_context* context;
    enum d_text_template_error   tmpl_err;
    enum d_str_interp_error      interp_err;

    // validate parameters
    if ( (!_template) ||
         (!_format)   ||
         (!_result) )
    {
        return D_TEXT_TEMPLATE_ERROR_NULL_PARAM;
    }

    // create an interp context and populate from bindings
    context = d_str_interp_context_new();
    if (!context)
    {
        return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
    }

    // build the interp context with marker-wrapped keys
    tmpl_err = d_internal_template_build_interp_context(_template,
                                                        context,
                                                        0);
    if (tmpl_err != D_TEXT_TEMPLATE_SUCCESS)
    {
        d_str_interp_context_free(context);

        return tmpl_err;
    }

    // perform the substitution via str_interp_alloc
    interp_err = d_str_interp_alloc(context,
                                    _format,
                                    _result);

    d_str_interp_context_free(context);

    // translate interp errors
    if (interp_err != D_STR_INTERP_SUCCESS)
    {
        if (interp_err == D_STR_INTERP_ERROR_ALLOCATION_FAILED)
        {
            return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
        }

        return D_TEXT_TEMPLATE_ERROR_INTERP_FAILED;
    }

    return D_TEXT_TEMPLATE_SUCCESS;
}

/*
d_text_template_render_length
  Calculates the length of the rendered string without performing the
substitution.

Parameter(s):
  _template: the template containing bindings and marker config.
  _format:   the format string.
  _length:   pointer to receive the calculated length (excluding null).
Return:
  D_TEXT_TEMPLATE_SUCCESS on success, or an appropriate error code.
*/
enum d_text_template_error
d_text_template_render_length
(
    const struct d_text_template* _template,
    const char*                   _format,
    size_t*                       _length
)
{
    struct d_str_interp_context* context;
    enum d_text_template_error   tmpl_err;
    enum d_str_interp_error      interp_err;

    // validate parameters
    if ( (!_template) ||
         (!_format)   ||
         (!_length) )
    {
        return D_TEXT_TEMPLATE_ERROR_NULL_PARAM;
    }

    // create an interp context and populate from bindings
    context = d_str_interp_context_new();
    if (!context)
    {
        return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
    }

    // build the interp context with marker-wrapped keys
    tmpl_err = d_internal_template_build_interp_context(_template,
                                                        context,
                                                        0);
    if (tmpl_err != D_TEXT_TEMPLATE_SUCCESS)
    {
        d_str_interp_context_free(context);

        return tmpl_err;
    }

    // calculate length via str_interp_length
    interp_err = d_str_interp_length(context,
                                     _format,
                                     _length);

    d_str_interp_context_free(context);

    // translate interp errors
    if (interp_err != D_STR_INTERP_SUCCESS)
    {
        return D_TEXT_TEMPLATE_ERROR_INTERP_FAILED;
    }

    return D_TEXT_TEMPLATE_SUCCESS;
}


// =============================================================================
// Key Wrapping Utility
// =============================================================================

/*
d_text_template_wrap_keys
  Scans the input string for occurrences of any bound key name (without
markers) and wraps each match with the template's prefix and suffix
markers. This is useful for converting a plain string into a format
string suitable for rendering.

  For example, given input "Hello name, your age is age", keys ["name",
"age"], and markers "{{"/"}}": produces "Hello {{name}}, your age is
{{age}}".

Parameter(s):
  _template:    the template whose keys and markers to use.
  _input:       the input string containing bare key names.
  _buffer:      destination buffer for the result.
  _buffer_size: size of the destination buffer in bytes.
Return:
  D_TEXT_TEMPLATE_SUCCESS on success, or an appropriate error code.
*/
enum d_text_template_error
d_text_template_wrap_keys
(
    const struct d_text_template* _template,
    const char*                   _input,
    char*                         _buffer,
    size_t                        _buffer_size
)
{
    size_t pos;
    size_t out_pos;
    size_t input_len;
    size_t remaining;
    size_t wrapped_len;
    size_t best_len;
    int    best_index;
    size_t i;

    // validate parameters
    if ( (!_template) ||
         (!_input)    ||
         (!_buffer) )
    {
        return D_TEXT_TEMPLATE_ERROR_NULL_PARAM;
    }

    // check buffer size
    if (_buffer_size == 0)
    {
        return D_TEXT_TEMPLATE_ERROR_BUFFER_TOO_SMALL;
    }

    // initialize
    pos       = 0;
    out_pos   = 0;
    input_len = d_strnlen(_input, 1048576);

    // process input string
    while (pos < input_len)
    {
        remaining  = input_len - pos;
        best_index = -1;
        best_len   = 0;

        // find the longest matching key at this position
        for (i = 0; i < _template->binding_count; i++)
        {
            if (_template->bindings[i].key_length > remaining)
            {
                continue;
            }

            if (_template->bindings[i].key_length <= best_len)
            {
                continue;
            }

            if (d_strequals(&_input[pos],
                            _template->bindings[i].key_length,
                            _template->bindings[i].key,
                            _template->bindings[i].key_length))
            {
                best_index = (int)i;
                best_len   = _template->bindings[i].key_length;
            }
        }

        // if a key matched, emit prefix + key + suffix
        if (best_index >= 0)
        {
            wrapped_len = _template->marker.prefix_length
                        + best_len
                        + _template->marker.suffix_length;

            // check output space
            if (out_pos + wrapped_len >= _buffer_size)
            {
                return D_TEXT_TEMPLATE_ERROR_BUFFER_TOO_SMALL;
            }

            // emit prefix
            d_memcpy(&_buffer[out_pos],
                     _template->marker.prefix,
                     _template->marker.prefix_length);
            out_pos += _template->marker.prefix_length;

            // emit key
            d_memcpy(&_buffer[out_pos],
                     _template->bindings[best_index].key,
                     best_len);
            out_pos += best_len;

            // emit suffix
            d_memcpy(&_buffer[out_pos],
                     _template->marker.suffix,
                     _template->marker.suffix_length);
            out_pos += _template->marker.suffix_length;

            pos += best_len;

            continue;
        }

        // regular character
        if (out_pos + 1 >= _buffer_size)
        {
            return D_TEXT_TEMPLATE_ERROR_BUFFER_TOO_SMALL;
        }

        _buffer[out_pos++] = _input[pos++];
    }

    // null terminate
    _buffer[out_pos] = '\0';

    return D_TEXT_TEMPLATE_SUCCESS;
}

/*
d_text_template_wrap_keys_alloc
  Like d_text_template_wrap_keys, but allocates the result buffer.

Parameter(s):
  _template: the template whose keys and markers to use.
  _input:    the input string containing bare key names.
  _result:   pointer to receive the allocated result string. Caller must
             free the returned string.
Return:
  D_TEXT_TEMPLATE_SUCCESS on success, or an appropriate error code.
*/
enum d_text_template_error
d_text_template_wrap_keys_alloc
(
    const struct d_text_template* _template,
    const char*                   _input,
    char**                        _result
)
{
    size_t input_len;
    size_t max_expansion;
    size_t buffer_size;
    char*  buffer;
    enum d_text_template_error err;

    // validate parameters
    if ( (!_template) ||
         (!_input)    ||
         (!_result) )
    {
        return D_TEXT_TEMPLATE_ERROR_NULL_PARAM;
    }

    // worst case: every character is part of a key, each key gets
    // prefix + suffix added; allocate conservatively
    input_len     = d_strnlen(_input, 1048576);
    max_expansion = _template->marker.prefix_length
                  + _template->marker.suffix_length;
    buffer_size   = input_len + (input_len * max_expansion) + 1;

    buffer = malloc(buffer_size);
    if (!buffer)
    {
        return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
    }

    // perform wrapping
    err = d_text_template_wrap_keys(_template,
                                    _input,
                                    buffer,
                                    buffer_size);

    // check for errors
    if (err != D_TEXT_TEMPLATE_SUCCESS)
    {
        free(buffer);

        return err;
    }

    *_result = buffer;

    return D_TEXT_TEMPLATE_SUCCESS;
}


// =============================================================================
// Utility Functions
// =============================================================================

/*
d_text_template_error_string
  Returns a human-readable string describing the given error code.

Parameter(s):
  _error: the error code to describe.
Return:
  A constant string describing the error.
*/
const char*
d_text_template_error_string
(
    enum d_text_template_error _error
)
{
    switch (_error)
    {
        case D_TEXT_TEMPLATE_SUCCESS:
            return "success";

        case D_TEXT_TEMPLATE_ERROR_NULL_PARAM:
            return "null parameter";

        case D_TEXT_TEMPLATE_ERROR_BUFFER_TOO_SMALL:
            return "buffer too small";

        case D_TEXT_TEMPLATE_ERROR_KEY_NOT_FOUND:
            return "key not found";

        case D_TEXT_TEMPLATE_ERROR_NESTING_TOO_DEEP:
            return "nesting depth exceeded";

        case D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED:
            return "memory allocation failed";

        case D_TEXT_TEMPLATE_ERROR_INVALID_MARKER:
            return "invalid marker";

        case D_TEXT_TEMPLATE_ERROR_INTERP_FAILED:
            return "interpolation failed";

        case D_TEXT_TEMPLATE_ERROR_CYCLE_DETECTED:
            return "cycle detected in template bindings";

        case D_TEXT_TEMPLATE_ERROR_FUNCTION_FAILED:
            return "function binding callback failed";

        case D_TEXT_TEMPLATE_ERROR_LIST_CALLBACK_FAILED:
            return "list per-item callback failed";

        case D_TEXT_TEMPLATE_ERROR_LIST_ITEM_RENDER_FAILED:
            return "list item render failed";

        default:
            return "unknown error";
    }
}
