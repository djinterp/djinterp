#include "../../../inc/c/text/text_template.h"


/*
d_text_template_new
  Creates a new text template with default marker configuration ("%"/"%").

Parameter(s):
  none.
Return:
  A pointer to a newly allocated d_text_template, or NULL if allocation
failed.
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

    // initialize bindings
    tmpl->binding_count = 0;
    memset(tmpl->bindings,
           0,
           sizeof(tmpl->bindings));

    // set default markers
    memcpy(tmpl->marker.prefix,
           D_TEXT_TEMPLATE_DEFAULT_PREFIX,
           sizeof(D_TEXT_TEMPLATE_DEFAULT_PREFIX));
    tmpl->marker.prefix_length = sizeof(D_TEXT_TEMPLATE_DEFAULT_PREFIX) - 1;

    memcpy(tmpl->marker.suffix,
           D_TEXT_TEMPLATE_DEFAULT_SUFFIX,
           sizeof(D_TEXT_TEMPLATE_DEFAULT_SUFFIX));
    tmpl->marker.suffix_length = sizeof(D_TEXT_TEMPLATE_DEFAULT_SUFFIX) - 1;

    return tmpl;
}

/*
d_text_template_new_with_markers
  Creates a new text template with the specified prefix and suffix marker
strings.

Parameter(s):
  _prefix: the prefix marker string (e.g., "{{", "${", "%").
  _suffix: the suffix marker string (e.g., "}}", "}", "%").
Return:
  A pointer to a newly allocated d_text_template, or NULL if allocation
failed or markers are invalid.
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
        free(tmpl);

        return NULL;
    }

    return tmpl;
}

/*
d_internal_template_free_binding
  Internal function to free the resources owned by a single binding.

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
    if (_binding->key)
    {
        free(_binding->key);
        _binding->key = NULL;
    }

    // free string value if applicable (template pointers are not owned)
    if (_binding->type == D_TEXT_TEMPLATE_BINDING_STRING)
    {
        if (_binding->value.string.text)
        {
            free(_binding->value.string.text);
            _binding->value.string.text = NULL;
        }
    }

    _binding->key_length = 0;
    _binding->type       = D_TEXT_TEMPLATE_BINDING_STRING;

    return;
}

/*
d_text_template_free
  Frees a text template and all owned resources. Does not free nested
template pointers (those are not owned).

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
        // free all binding resources
        for (i = 0; i < _template->binding_count; i++)
        {
            d_internal_template_free_binding(&_template->bindings[i]);
        }

        free(_template);
    }

    return;
}

/*
d_text_template_clear
  Clears all bindings from a template without freeing the template itself.
Marker configuration is preserved.

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

        memset(_template->bindings,
               0,
               sizeof(_template->bindings));
        _template->binding_count = 0;
    }

    return;
}

/*
d_text_template_set_markers
  Sets the prefix and suffix marker strings for a template.

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

    // validate parameters
    if ( (!_template) ||
         (!_prefix)   ||
         (!_suffix) )
    {
        return D_TEXT_TEMPLATE_ERROR_NULL_PARAM;
    }

    prefix_len = strlen(_prefix);
    suffix_len = strlen(_suffix);

    // validate lengths
    if ( (prefix_len == 0) ||
         (prefix_len >= D_TEXT_TEMPLATE_MAX_MARKER_LENGTH) ||
         (suffix_len == 0) ||
         (suffix_len >= D_TEXT_TEMPLATE_MAX_MARKER_LENGTH) )
    {
        return D_TEXT_TEMPLATE_ERROR_INVALID_MARKER;
    }

    // copy prefix
    memcpy(_template->marker.prefix, _prefix, prefix_len + 1);
    _template->marker.prefix_length = prefix_len;

    // copy suffix
    memcpy(_template->marker.suffix, _suffix, suffix_len + 1);
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

/*
d_internal_template_find_binding
  Internal function to find a binding by key name.

Parameter(s):
  _template: the template to search.
  _key:      the key name to find.
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

    key_len = strlen(_key);

    // search through all bindings
    for (i = 0; i < _template->binding_count; i++)
    {
        if ( (_template->bindings[i].key_length == key_len) &&
             (memcmp(_template->bindings[i].key,
                     _key,
                     key_len) == 0) )
        {
            return (int)i;
        }
    }

    return -1;
}

/*
d_text_template_bind_string
  Binds a key to a plain string value. If the key already exists, its value
is updated.

Parameter(s):
  _template: the template to add the binding to.
  _key:      the key name (null-terminated).
  _value:    the string value to bind (null-terminated).
Return:
  D_TEXT_TEMPLATE_SUCCESS on success, or an appropriate error code.
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

    key_len   = strlen(_key);
    value_len = strlen(_value);

    return d_text_template_bind_string_n(_template,
                                         _key,
                                         key_len,
                                         _value,
                                         value_len);
}

/*
d_text_template_bind_string_n
  Binds a key to a plain string value with explicit lengths.

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
        return d_text_template_update_string(_template, _key, _value);
    }

    // check if we have room
    if (_template->binding_count >= D_TEXT_TEMPLATE_MAX_BINDINGS)
    {
        return D_TEXT_TEMPLATE_ERROR_TOO_MANY_BINDINGS;
    }

    // allocate and copy key
    key_copy = malloc(_key_length + 1);
    if (!key_copy)
    {
        return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
    }

    memcpy(key_copy, _key, _key_length);
    key_copy[_key_length] = '\0';

    // allocate and copy value
    value_copy = malloc(_value_length + 1);
    if (!value_copy)
    {
        free(key_copy);

        return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
    }

    memcpy(value_copy, _value, _value_length);
    value_copy[_value_length] = '\0';

    // add the binding
    binding                    = &_template->bindings[_template->binding_count];
    binding->key               = key_copy;
    binding->key_length        = _key_length;
    binding->type              = D_TEXT_TEMPLATE_BINDING_STRING;
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

    // validate parameters
    if ( (!_template) ||
         (!_key)      ||
         (!_nested) )
    {
        return D_TEXT_TEMPLATE_ERROR_NULL_PARAM;
    }

    // if key already exists, free old binding and reuse slot
    existing = d_internal_template_find_binding(_template, _key);
    if (existing >= 0)
    {
        d_internal_template_free_binding(&_template->bindings[existing]);

        key_len  = strlen(_key);
        key_copy = malloc(key_len + 1);
        if (!key_copy)
        {
            return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
        }

        memcpy(key_copy, _key, key_len + 1);

        _template->bindings[existing].key        = key_copy;
        _template->bindings[existing].key_length = key_len;
        _template->bindings[existing].type       = D_TEXT_TEMPLATE_BINDING_TEMPLATE;
        _template->bindings[existing].value.tmpl = _nested;

        return D_TEXT_TEMPLATE_SUCCESS;
    }

    // check if we have room
    if (_template->binding_count >= D_TEXT_TEMPLATE_MAX_BINDINGS)
    {
        return D_TEXT_TEMPLATE_ERROR_TOO_MANY_BINDINGS;
    }

    // allocate and copy key
    key_len  = strlen(_key);
    key_copy = malloc(key_len + 1);
    if (!key_copy)
    {
        return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
    }

    memcpy(key_copy, _key, key_len + 1);

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
    memset(&_template->bindings[_template->binding_count - 1],
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

    // allocate and copy new value
    value_len  = strlen(_value);
    value_copy = malloc(value_len + 1);
    if (!value_copy)
    {
        return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
    }

    memcpy(value_copy, _value, value_len + 1);

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
d_internal_template_resolve_value
  Internal function that resolves a binding to its final string value.
For string bindings, returns the value directly. For template bindings,
recursively renders the nested template. Tracks nesting depth to prevent
infinite recursion.

Parameter(s):
  _binding:  the binding to resolve.
  _format:   for template bindings, the format string to render; may be
             NULL in which case a template binding resolves to an empty
             string.
  _result:   pointer to receive the resolved string (heap-allocated).
             Caller must free this.
  _length:   pointer to receive the length of the result.
  _depth:    current nesting depth for recursion protection.
Return:
  D_TEXT_TEMPLATE_SUCCESS on success, or an appropriate error code.
*/
static enum d_text_template_error
d_internal_template_resolve_value
(
    const struct d_text_template_binding* _binding,
    const char*                           _format,
    char**                                _result,
    size_t*                               _length,
    size_t                                _depth
)
{
    char*                      resolved;
    size_t                     resolved_len;
    enum d_text_template_error err;

    // check nesting depth
    if (_depth >= D_TEXT_TEMPLATE_MAX_NESTING_DEPTH)
    {
        return D_TEXT_TEMPLATE_ERROR_NESTING_TOO_DEEP;
    }

    // resolve based on binding type
    if (_binding->type == D_TEXT_TEMPLATE_BINDING_STRING)
    {
        // string binding: duplicate the value
        resolved_len = _binding->value.string.length;
        resolved     = malloc(resolved_len + 1);
        if (!resolved)
        {
            return D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED;
        }

        memcpy(resolved, _binding->value.string.text, resolved_len + 1);

        *_result = resolved;
        *_length = resolved_len;

        return D_TEXT_TEMPLATE_SUCCESS;
    }

    // template binding: recursively render
    if ( (!_binding->value.tmpl) ||
         (!_format) )
    {
        // no template or format, return empty string
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

    // render the nested template (the nested template uses its own format)
    err = d_text_template_render_alloc(_binding->value.tmpl,
                                       _format,
                                       &resolved);
    if (err != D_TEXT_TEMPLATE_SUCCESS)
    {
        return err;
    }

    *_result = resolved;
    *_length = strlen(resolved);

    return D_TEXT_TEMPLATE_SUCCESS;
}

/*
d_internal_template_build_interp_context
  Internal function that builds a d_str_interp_context from a template's
bindings by constructing marker-wrapped keys (prefix + key + suffix) and
resolving all values (including nested templates).

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
        memcpy(marked_key,
               _template->marker.prefix,
               _template->marker.prefix_length);

        // assemble: key
        memcpy(marked_key + _template->marker.prefix_length,
               _template->bindings[i].key,
               _template->bindings[i].key_length);

        // assemble: suffix
        memcpy(marked_key + _template->marker.prefix_length
                          + _template->bindings[i].key_length,
               _template->marker.suffix,
               _template->marker.suffix_length);

        marked_key[marked_key_len] = '\0';

        // resolve the binding value
        resolved_value = NULL;
        resolved_len   = 0;
        tmpl_err = d_internal_template_resolve_value(
                       &_template->bindings[i],
                       NULL,
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

/*
d_text_template_render
  Renders a template by replacing all marker-delimited keys in the format
string with their bound values. Nested template bindings are recursively
expanded.

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

/*
d_text_template_wrap_keys
  Scans the input string for occurrences of any bound key name (without
markers) and wraps each match with the template's prefix and suffix
markers. This is useful for converting a plain string into a format string
suitable for rendering.

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
    input_len = strlen(_input);

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

            if (memcmp(&_input[pos],
                       _template->bindings[i].key,
                       _template->bindings[i].key_length) == 0)
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
            memcpy(&_buffer[out_pos],
                   _template->marker.prefix,
                   _template->marker.prefix_length);
            out_pos += _template->marker.prefix_length;

            // emit key
            memcpy(&_buffer[out_pos],
                   _template->bindings[best_index].key,
                   best_len);
            out_pos += best_len;

            // emit suffix
            memcpy(&_buffer[out_pos],
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
    input_len     = strlen(_input);
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

        case D_TEXT_TEMPLATE_ERROR_TOO_MANY_BINDINGS:
            return "too many bindings";

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

        default:
            return "unknown error";
    }
}
