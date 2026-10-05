/*******************************************************************************
* djinterp [c]                                                      cli_option.h
*
*   Convention-agnostic string-to-void* lookup and CLI argument parsing.
* The core primitive is `d_string_entry`: a key/value pair mapping a
* string to an opaque pointer. Parse performs exact-match lookup of each
* argv token against the entry table. No prefix stripping, no operator
* awareness. Platform-specific conventions (Unix `-`/`--` expansion,
* Windows `/`, etc.) belong in separate modules built on top of this one.
*
*   Keys are literal strings. Multiple keys may map to the same pointer:
*
*   struct d_string_entry opts[] = {
*       { "verbose", &verbose_flag },
*       { "v",       &verbose_flag },
*       { "help",    &help_flag },
*   };
*   D_CLI_PARSE(opts, argc, argv, my_handler, ctx);
*
*   For arbitrary structs with a string field, `d_cli_find_by_field`
* searches by byte offset, and `D_CLI_FIND` wraps it type-safely.
*
*
* path:      /inc/djinterp/c/cli/cli_option.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.22
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_C_CLI_CLI_OPTION_H
#define DJINTERP_C_CLI_CLI_OPTION_H 1

// std
#include <stddef.h>
#include <stdio.h>
// djinterp
#include "../djinterp.h"
#include "../string_fn.h"


// D_CLI_ENTRY
//   macro: initializes a d_string_entry.
#define D_CLI_ENTRY(_key, _value)                                           \
    {                                                                       \
        (_key),                                                             \
        (sizeof(_key) - 1),                                                 \
        (_value)                                                            \
    }

// D_CLI_COUNT
//   macro: number of elements in a statically-declared array.
#define D_CLI_COUNT(_array)                                                 \
    (D_ARRAY_STATIC_SIZE(_array))

// D_CLI_TOTAL_SIZE
//   macro: total byte size of a statically-declared array.
#define D_CLI_TOTAL_SIZE(_array)                                            \
    D_ARRAY_TOTAL_SIZE(sizeof((_array)[0]), D_CLI_COUNT(_array))

// D_CLI_FIND_ENTRY
//   macro: convenience for d_cli_find on a static array.
#define D_CLI_FIND_ENTRY(_array, _key, _key_length)                         \
    d_cli_find((_array), D_CLI_COUNT(_array), (_key), (_key_length))

// D_CLI_FIND
//   macro: type-safe wrapper for d_cli_find_by_field.
// returns (_type*) or NULL.
#define D_CLI_FIND(_type, _array, _count, _field, _key, _key_length)        \
    ( (_type*)d_cli_find_by_field(                                          \
        (_array), (_count),                                                 \
        sizeof(_type),                                                      \
        offsetof(_type, _field),                                            \
        (_key), (_key_length)) )

// D_CLI_FIND_IN
//   macro: like D_CLI_FIND but derives count from a static array.
#define D_CLI_FIND_IN(_type, _array, _field, _key, _key_length)             \
    D_CLI_FIND(_type, (_array), D_CLI_COUNT(_array), _field,                \
               (_key), (_key_length))

// D_CLI_PARSE
//   macro: convenience on a static array.
#define D_CLI_PARSE(_array, _argc, _argv, _handler, _context)               \
    d_cli_parse((_array), D_CLI_COUNT(_array),                              \
                (_argc), (_argv), (_handler), (_context))


// fn_cli_lookup
//   function pointer: custom lookup function. receives the array base,
// element count, the key to find, and an opaque context. returns the
// matching row as const void*, or NULL.
typedef const void* (*fn_cli_lookup)(const void* _array,
                                     size_t      _array_count,
                                     const char* _key,
                                     size_t      _key_length,
                                     void*       _context);

// fn_cli_match
//   function pointer: handler invoked when a CLI argument matches an
// entry during parsing. receives the matched key, its associated value
// pointer, the next argv element (NULL if none), and a user context.
// returns true to continue parsing, false to stop.
typedef bool (*fn_cli_match)(const char* _key,
                             size_t      _key_length,
                             void*       _value,
                             const char* _next_arg,
                             void*       _context);

// d_string_cli_entry
//   struct: maps a string key to an opaque value pointer.
struct d_string_cli_entry
{
    const char*  key;
    const size_t key_length;
    void*        value;
};


void*       d_cli_find(const struct d_string_cli_entry* _entries,
                       size_t                           _count,
                       const char*                      _key,
                       size_t                           _key_length);
void*       d_cli_find_by_field(const void* _array,
                                size_t      _count,
                                size_t      _element_size,
                                size_t      _field_offset,
                                const char* _key,
                                size_t      _key_length);
const void* d_cli_find_with(const void*   _array,
                            size_t        _count,
                            const char*   _key,
                            size_t        _key_length,
                            fn_cli_lookup _lookup,
                            void*         _context);
bool        d_cli_parse(const struct d_string_cli_entry* _entries,
                        size_t                           _count,
                        int                              _argc,
                        char*                            _argv[],
                        fn_cli_match                     _on_match,
                        void*                            _context);


#endif  // DJINTERP_C_CLI_CLI_OPTION_H
