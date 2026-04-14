/******************************************************************************
* djinterp [text]                                              string_interp.h
*
*   Pure key-value string interpolation engine. Maps named specifiers to
* replacement values and performs batch find-and-replace substitution on
* input strings. Marker-aware parsing (prefix/suffix delimiters, nesting)
* is provided by the higher-level `text_template` module.
*
* path:      /inc/text/c/text/string_interp.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.01.02
******************************************************************************/

#ifndef DJINTERP_C_TEXT_STRING_INTERP_
#define DJINTERP_C_TEXT_STRING_INTERP_

#include <stddef.h>
#include "../djinterp.h"
#include "../dmemory.h"
#include "../string_fn.h"


// =============================================================================
// Constants
// =============================================================================

// D_STR_INTERP_MAX_SPECIFIERS
//   constant: maximum number of specifiers that can be registered at once.
#define D_STR_INTERP_MAX_SPECIFIERS 32

// =============================================================================
// Enumerations
// =============================================================================

// d_str_interp_error
//   enum: error codes returned by string interpolation functions.
enum d_str_interp_error
{
    D_STR_INTERP_SUCCESS = 0,
    D_STR_INTERP_ERROR_NULL_PARAM,
    D_STR_INTERP_ERROR_BUFFER_TOO_SMALL,
    D_STR_INTERP_ERROR_SPECIFIER_NOT_FOUND,
    D_STR_INTERP_ERROR_TOO_MANY_SPECIFIERS,
    D_STR_INTERP_ERROR_ALLOCATION_FAILED
};

// =============================================================================
// Structures
// =============================================================================

// d_str_interp_specifier
//   struct: represents a single named specifier and its replacement value.
// Both the name and value are heap-allocated and owned by the specifier.
struct d_str_interp_specifier
{
    // the name of the specifier (e.g., "name", "description")
    char*  name;

    // length of the name string (cached for performance)
    size_t name_length;

    // the value to substitute for this specifier
    char*  value;

    // length of the value string (cached for performance)
    size_t value_length;
};

// d_str_interp_context
//   struct: holds the state for an interpolation operation including
// specifiers. Performs direct name-to-value substitution with no
// marker or delimiter awareness.
struct d_str_interp_context
{
    // array of registered specifiers
    struct d_str_interp_specifier specifiers[D_STR_INTERP_MAX_SPECIFIERS];

    // number of currently registered specifiers
    size_t                        specifier_count;
};

// =============================================================================
// Context Management
// =============================================================================

struct d_str_interp_context* d_str_interp_context_new(void);
void                         d_str_interp_context_free(struct d_str_interp_context* _context);
void                         d_str_interp_context_clear(struct d_str_interp_context* _context);

// =============================================================================
// Specifier Management
// =============================================================================

enum d_str_interp_error d_str_interp_add_specifier(struct d_str_interp_context* _context,
                                                   const char*                  _name,
                                                   const char*                  _value);
enum d_str_interp_error d_str_interp_add_specifier_n(struct d_str_interp_context* _context,
                                                     const char*                  _name,
                                                     size_t                       _name_length,
                                                     const char*                  _value,
                                                     size_t                       _value_length);
enum d_str_interp_error d_str_interp_remove_specifier(struct d_str_interp_context* _context,
                                                      const char*                  _name);
enum d_str_interp_error d_str_interp_update_specifier(struct d_str_interp_context* _context,
                                                      const char*                  _name,
                                                      const char*                  _value);
const char*             d_str_interp_get_specifier(const struct d_str_interp_context* _context,
                                                   const char*                        _name);
bool                    d_str_interp_has_specifier(const struct d_str_interp_context* _context,
                                                   const char*                        _name);

// =============================================================================
// Interpolation Functions
// =============================================================================

enum d_str_interp_error d_str_interp(const struct d_str_interp_context* _context,
                                     char*                              _buffer,
                                     size_t                             _buffer_size,
                                     const char*                        _input);
enum d_str_interp_error d_str_interp_alloc(const struct d_str_interp_context* _context,
                                           const char*                        _input,
                                           char**                             _result);
enum d_str_interp_error d_str_interp_length(const struct d_str_interp_context* _context,
                                            const char*                        _input,
                                            size_t*                            _length);

// =============================================================================
// Convenience Functions (Single-Call Interpolation)
// =============================================================================

enum d_str_interp_error d_str_interp_quick(char*        _buffer,
                                           size_t       _buffer_size,
                                           const char*  _input,
                                           const char** _names,
                                           const char** _values);

// =============================================================================
// Utility Functions
// =============================================================================

const char* d_str_interp_error_string(enum d_str_interp_error _error);


#endif // DJINTERP_C_TEXT_STRING_INTERP_
