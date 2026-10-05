/*******************************************************************************
* djinterp [c]                                                   text_template.h
*
*   Marker-aware text templating engine built on top of `string_interp.h`.
* Supports configurable prefix/suffix marker strings (e.g., "{{" / "}}",
* "%" / "%", "${" / "}"), binding keys to plain string values, to other
* text templates for recursive (nested) expansion, to user-supplied
* functions for computed / dynamic content, or to list bindings that
* iterate a sub-template over a counted sequence with per-item callbacks.
*
*   List bindings enable iteration without adding loop syntax to templates.
* A key is bound to a sub-template, count, and per-item callback. During
* rendering, the renderer loops `count` times: clears the sub-template,
* injects auto-keys (_index, _number, _count, _is_first, _is_last),
* invokes the callback to bind domain-specific keys, renders the sub-
* template, and appends the result. An optional separator string is
* inserted between items. Nested list bindings compose naturally -- the
* callback for an outer list can itself bind an inner list on the sub-
* template -- with all nesting counted against the configurable depth
* limit.
*
*   No hardcoded limits on binding count, marker length, or key length.
* All strings are heap-allocated with explicit lengths (sized strings).
* The binding array grows dynamically. The maximum nesting depth is a
* per-template configurable field with a sensible default.
*
* Implementation dependencies:
*   dmemory.h   - d_memcpy, d_memset, d_memdup_s for binding array growth
*   string_fn.h - d_strdup, d_strndup, d_strequals, d_strnlen for sized
*                 string operations on markers, keys, and values
*
*
* path:      /inc/djinterp/c/text/text_template.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.26
*                                                            revised: 2026.09.29
*******************************************************************************/

#ifndef DJINTERP_C_TEXT_TEXT_TEMPLATE_H
#define DJINTERP_C_TEXT_TEXT_TEMPLATE_H 1

// std
#include <stddef.h>
// djinterp
#include "../djinterp.h"
#include "./str_interp.h"


// =============================================================================
// Default Values
// =============================================================================

// D_TEXT_TEMPLATE_DEFAULT_NESTING_DEPTH
//   default: initial maximum recursion depth for nested template expansion.
//   This limit is shared across all nesting mechanisms: nested template
//   bindings, function bindings, and list bindings. Each level of list
//   iteration that triggers a sub-template render counts as one depth.
//   Configurable per-template via d_text_template_set_max_depth().
#define D_TEXT_TEMPLATE_DEFAULT_NESTING_DEPTH 16

// D_TEXT_TEMPLATE_DEFAULT_BINDING_CAPACITY
//   default: initial allocation size (in slots) for the binding array.
//   The array grows automatically when this capacity is exceeded. This
//   is a performance hint, not a limit.
#define D_TEXT_TEMPLATE_DEFAULT_BINDING_CAPACITY 8

// D_TEXT_TEMPLATE_DEFAULT_PREFIX
//   default: prefix marker string used when no prefix is specified.
#define D_TEXT_TEMPLATE_DEFAULT_PREFIX "%"

// D_TEXT_TEMPLATE_DEFAULT_SUFFIX
//   default: suffix marker string used when no suffix is specified.
#define D_TEXT_TEMPLATE_DEFAULT_SUFFIX "%"


// =============================================================================
// Enumerations
// =============================================================================

// d_text_template_binding_type
//   enum: the type of value a binding maps to.
enum d_text_template_binding_type
{
    // binding maps a key to a plain string value
    D_TEXT_TEMPLATE_BINDING_STRING,

    // binding maps a key to another text template (nested)
    D_TEXT_TEMPLATE_BINDING_TEMPLATE,

    // binding maps a key to a user-supplied function that produces the
    // replacement string at render time
    D_TEXT_TEMPLATE_BINDING_FUNCTION,

    // binding maps a key to a list iteration: a sub-template is rendered
    // once per item, with per-item bindings supplied by a user callback
    D_TEXT_TEMPLATE_BINDING_LIST
};

// d_text_template_error
//   enum: error codes returned by text template functions.
enum d_text_template_error
{
    D_TEXT_TEMPLATE_SUCCESS = 0,
    D_TEXT_TEMPLATE_ERROR_NULL_PARAM,
    D_TEXT_TEMPLATE_ERROR_BUFFER_TOO_SMALL,
    D_TEXT_TEMPLATE_ERROR_KEY_NOT_FOUND,
    D_TEXT_TEMPLATE_ERROR_NESTING_TOO_DEEP,
    D_TEXT_TEMPLATE_ERROR_ALLOCATION_FAILED,
    D_TEXT_TEMPLATE_ERROR_INVALID_MARKER,
    D_TEXT_TEMPLATE_ERROR_INTERP_FAILED,
    D_TEXT_TEMPLATE_ERROR_CYCLE_DETECTED,
    D_TEXT_TEMPLATE_ERROR_FUNCTION_FAILED,

    // the per-item callback for a list binding returned false, aborting
    // iteration; the index at which the callback failed is not captured
    // here but can be tracked by the callback's own context
    D_TEXT_TEMPLATE_ERROR_LIST_CALLBACK_FAILED,

    // the sub-template render failed during list iteration; this may be
    // caused by a missing binding in the sub-template, a nesting depth
    // overflow, or any other render-time error within the item template
    D_TEXT_TEMPLATE_ERROR_LIST_ITEM_RENDER_FAILED
};


// =============================================================================
// Function Types
// =============================================================================

// d_text_template_fn
//   function pointer: a user-supplied callback invoked at render time for
// function bindings. The function receives an opaque context pointer and must
// allocate and return the replacement string through `_out_result`. The caller
// (the renderer) is responsible for freeing `*_out_result`.
//
// Parameter(s):
//   _context:    opaque user data (may be NULL)
//   _out_result: on success, set to a heap-allocated null-terminated string
//
// Return:
//   D_TEXT_TEMPLATE_SUCCESS on success, or an appropriate error code.
typedef enum d_text_template_error(*d_text_template_fn)(void*  _context,
                                                        char** _out_result);

// struct d_text_template
//   forward declaration: the list callback below takes a pointer to one,
// and a struct first named inside a parameter list has prototype scope --
// it would be a different type from the struct defined further down.
struct d_text_template;


// d_text_template_list_fn
//   function pointer: a per-item callback invoked during list binding
// rendering. Called once for each item (index 0 through _count - 1).
// The callback's job is to bind domain-specific keys onto `_item_template`;
// the renderer has already cleared any prior bindings and injected the
// auto-keys (_index, _number, _count, _is_first, _is_last) before this
// callback runs.
//
// Parameter(s):
//   _index:         0-based item index within the list
//   _count:         total number of items in the list
//   _item_template: the sub-template to bind keys onto; auto-keys are
//                   already bound, all other bindings from the previous
//                   iteration have been cleared
//   _context:       opaque user data (may be NULL)
//
// Return:
//   true on success (continue iteration), false to abort (the renderer
//   will return D_TEXT_TEMPLATE_ERROR_LIST_CALLBACK_FAILED).
//
// Note:
//   The callback must not free or reallocate `_item_template`. The callback
//   may bind string, template, function, or list bindings (for nested
//   iteration) onto `_item_template`. Bound values must remain valid until
//   the callback returns -- the renderer will render the sub-template
//   immediately after the callback.
typedef bool (*d_text_template_list_fn)(size_t                  _index,
                                        size_t                  _count,
                                        struct d_text_template* _item_template,
                                        void*                   _context);


// =============================================================================
// Structures
// =============================================================================

// d_text_template_marker_config
//   struct: configuration for the prefix and suffix marker strings used to
// delimit specifier keys within a template string.
//
//   Both prefix and suffix are heap-allocated sized strings. Ownership is
// held by the template struct; they are freed when the template is freed
// or when new markers are set. Implemented using d_strdup / d_strndup
// from string_fn.h.
struct d_text_template_marker_config
{
    // prefix string placed before a key (e.g., "%", "{{", "${")
    // heap-allocated; NULL only before initialization
    char*  prefix;

    // length of the prefix string (excluding null terminator)
    size_t prefix_length;

    // suffix string placed after a key (e.g., "%", "}}", "}")
    // heap-allocated; NULL only before initialization
    char*  suffix;

    // length of the suffix string (excluding null terminator)
    size_t suffix_length;
};

// d_text_template_list_options
//   struct: configuration for list binding rendering behavior. Passed to
// `d_text_template_bind_list()`. All fields have sensible defaults when
// the struct is zero-initialized or when NULL is passed as the options
// parameter.
//
// Default behavior (NULL or zero-initialized):
//   - no separator between items
//   - empty string when count is 0
//   - _number starts at 1 (1-based)
//   - _number is zero-padded to the digit width of count
//   - pad character is '0'
struct d_text_template_list_options
{
    // string inserted between consecutive rendered items; NULL or empty
    // string means no separator is inserted
    const char* separator;

    // string rendered when the item count is 0; NULL means empty string;
    // this allows displaying "(no items)" or similar placeholder text
    // without requiring a conditional wrapper in the calling code
    const char* empty_text;

    // minimum width for the _number auto-key; 0 means auto-detect from
    // the digit width of count (e.g., count=21 gives width 2, so _number
    // is "01", "02", ... "21"); a fixed value overrides this
    size_t      number_pad_width;

    // character used for padding _number; default (when zero) is '0';
    // common alternatives: ' ' for space-padded numbers
    char        number_pad_char;

    // when true, _number starts at 0 instead of 1; _index is always
    // 0-based regardless of this setting; default (false) gives 1-based
    // numbering which is typical for display purposes
    bool        number_from_zero;
};


// d_text_template_binding
//   struct: a single binding that associates a key name with a string value,
// a nested text template, a user-supplied function, or a list iteration.
//
//   The key is a heap-allocated sized string (using d_strdup from
// string_fn.h). String values are also heap-allocated sized strings.
struct d_text_template_binding
{
    // the key name for this binding (heap-allocated via d_strdup)
    char*  key;

    // length of the key string (excluding null terminator)
    size_t key_length;

    // type of this binding (string, template, function, or list)
    enum d_text_template_binding_type type;

    // the bound value (interpreted according to `type`)
    union
    {
        // for D_TEXT_TEMPLATE_BINDING_STRING:
        //   heap-allocated sized string (via d_strndup from string_fn.h)
        struct
        {
            char*  text;
            size_t length;
        } string;

        // for D_TEXT_TEMPLATE_BINDING_TEMPLATE:
        //   pointer to another template (not owned; caller manages lifetime)
        struct d_text_template* tmpl;

        // for D_TEXT_TEMPLATE_BINDING_FUNCTION:
        //   callback + context (context not owned; caller manages lifetime)
        struct
        {
            d_text_template_fn fn;
            void*              context;
        } function;

        // for D_TEXT_TEMPLATE_BINDING_LIST:
        //   sub-template, count, per-item callback, context, and options
        //   (sub-template and context not owned; caller manages lifetimes)
        struct
        {
            // the item template rendered once per iteration; not owned
            // by the binding -- caller must keep it alive until after
            // the parent template is rendered
            struct d_text_template* item_template;

            // number of items to iterate; the callback is invoked
            // exactly this many times (0 through count - 1)
            size_t                  count;

            // per-item callback that binds domain-specific keys onto
            // item_template before each render pass
            d_text_template_list_fn bind_fn;

            // opaque context pointer passed to bind_fn on each call;
            // not owned by the binding
            void*                   context;

            // rendering options: separator, empty text, number padding;
            // these are copied from the options struct at bind time, so
            // the caller's options struct need not outlive the bind call
            struct d_text_template_list_options options;
        } list;
    } value;
};

// d_text_template
//   struct: a text template with marker configuration and a dynamically-
// allocated array of bindings that map keys to string values, nested
// templates, user-supplied functions, or list iterations. Uses a
// d_str_interp_context internally for the actual substitution pass.
//
//   The binding array grows automatically via d_memcpy / d_memdup_s from
// dmemory.h when capacity is exceeded. Key lookups use d_strequals from
// string_fn.h for length-aware comparison.
struct d_text_template
{
    // marker configuration (prefix/suffix sized strings, heap-allocated)
    struct d_text_template_marker_config marker;

    // dynamically-allocated array of bindings (heap-allocated via
    // dmemory.h; grown by doubling when binding_count reaches
    // binding_capacity)
    struct d_text_template_binding* bindings;

    // number of currently registered bindings
    size_t                          binding_count;

    // allocated capacity of the bindings array (in number of slots);
    // starts at D_TEXT_TEMPLATE_DEFAULT_BINDING_CAPACITY and grows
    // automatically
    size_t                          binding_capacity;

    // maximum recursion depth for nested expansion; applies to all
    // nesting mechanisms (nested templates, function bindings, list
    // bindings); defaults to D_TEXT_TEMPLATE_DEFAULT_NESTING_DEPTH;
    // configurable via d_text_template_set_max_depth()
    size_t                          max_nesting_depth;
};


// =============================================================================
// Template Lifecycle
// =============================================================================
struct d_text_template* d_text_template_new(void);
struct d_text_template* d_text_template_new_with_markers(const char* _prefix, const char* _suffix);
void                    d_text_template_free(struct d_text_template* _template);
void                    d_text_template_clear(struct d_text_template* _template);


// =============================================================================
// Marker Configuration
// =============================================================================
enum d_text_template_error d_text_template_set_markers(struct d_text_template* _template, const char* _prefix, const char* _suffix);
const char*                d_text_template_get_prefix(const struct d_text_template* _template);
const char*                d_text_template_get_suffix(const struct d_text_template* _template);


// =============================================================================
// Nesting Depth Configuration
// =============================================================================
enum d_text_template_error d_text_template_set_max_depth(struct d_text_template* _template, size_t _depth);
size_t                     d_text_template_get_max_depth(const struct d_text_template* _template);


// =============================================================================
// Binding Management
// =============================================================================
enum d_text_template_error d_text_template_bind_string(struct d_text_template* _template, const char* _key, const char* _value);
enum d_text_template_error d_text_template_bind_string_n(struct d_text_template* _template, const char* _key, size_t _key_length, const char* _value, size_t _value_length);
enum d_text_template_error d_text_template_bind_template(struct d_text_template* _template, const char* _key, struct d_text_template* _nested);
enum d_text_template_error d_text_template_bind_function(struct d_text_template* _template, const char* _key, d_text_template_fn _fn, void* _context);
enum d_text_template_error d_text_template_bind_list(struct d_text_template*                    _template,
                                                     const char*                                _key,
                                                     struct d_text_template*                    _item_template,
                                                     size_t                                     _count,
                                                     d_text_template_list_fn                    _bind_fn,
                                                     void*                                      _context,
                                                     const struct d_text_template_list_options* _options
);
enum d_text_template_error d_text_template_unbind(struct d_text_template* _template, const char* _key);
enum d_text_template_error d_text_template_update_string(struct d_text_template* _template, const char* _key, const char* _value);
bool                       d_text_template_has_binding(const struct d_text_template* _template, const char* _key);
size_t                     d_text_template_binding_count(const struct d_text_template* _template);


// =============================================================================
// Rendering (Template Expansion)
// =============================================================================
enum d_text_template_error d_text_template_render(const struct d_text_template* _template, const char* _format, char* _buffer, size_t _buffer_size);
enum d_text_template_error d_text_template_render_alloc(const struct d_text_template* _template, const char* _format, char** _result);
enum d_text_template_error d_text_template_render_length(const struct d_text_template* _template, const char* _format, size_t* _length);


// =============================================================================
// Key Wrapping Utility
// =============================================================================
enum d_text_template_error d_text_template_wrap_keys(const struct d_text_template* _template, const char* _input, char* _buffer, size_t _buffer_size);
enum d_text_template_error d_text_template_wrap_keys_alloc(const struct d_text_template* _template, const char* _input, char** _result);


// =============================================================================
// Utility Functions
// =============================================================================
const char* d_text_template_error_string(enum d_text_template_error _error);


#endif  // DJINTERP_C_TEXT_TEXT_TEMPLATE_H
