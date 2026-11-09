/*******************************************************************************
* djinterp [c/test]                                                  test_cvar.h
*
* The test registry: DTest's config and metadata schema, as one table.
*   Each row is a key, its flag (a DTestConfigKey or a DTestMetadataFlag),
* whether it is a config or a metadata entry, the d_type_info its value is read
* by, the value and a help text. The registry looks rows up by key (aliases
* included) or by flag, and keeps every row's default so a value can be reset.
* It is a dj_registry over a static table, test_cvar.c.
*
*   A planned revision, not yet made: the union becomes a uint64_t carrier
* (a row storing `b = true` leaves seven bytes indeterminate, so two equal rows
* need not compare or hash equal), and `value_type` a dj_type_info16.
*
*
* path:      /inc/djinterp/c/modules/test/test_cvar.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.16
*                                                            revised: 2026.11.08
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KEYS
    ----
    1.  Config keys
         1.  DTestConfigKey
    2.  Metadata flags
         1.  DTestMetadataFlag
2.  REGISTRY ROW FLAGS
    ------------------
    1.  Row flags
         1.  DTestRegistryRowFlag
3.  VALUE UNION
    -----------
    1.  Value
         1.  dj_test_value
4.  ROW STRUCTURE
    -------------
    1.  Row
         1.  dj_test_registry_row
5.  PUBLIC API
    ----------
    1.  Initialization
    2.  Row access
    3.  Value access
    4.  Argument validation
6.  DEFAULT VALUE HELPERS
    ---------------------
7.  DEFAULT VALUES
    --------------
    1.  Defaults
         1.  D_TEST_DEFAULT_INDENT
         2.  D_TEST_DEFAULT_MAX_INDENT
         3.  D_TEST_DEFAULT_MAX_FAILURES
         4.  D_TEST_DEFAULT_TIMEOUT
8.  TYPED ACCESS MACROS
    -------------------
    1.  Row lookup
         1.  D_TEST_REGISTRY_GET
    2.  Values
         1.  D_TEST_REGISTRY_VALUE_PTR
         2.  D_TEST_REGISTRY_VALUE_BOOL
         3.  D_TEST_REGISTRY_VALUE_SIZE_T
         4.  D_TEST_REGISTRY_VALUE_UINT32
         5.  D_TEST_REGISTRY_VALUE_UINT16
         6.  D_TEST_REGISTRY_VALUE_INT32
    3.  Row fields
         1.  D_TEST_REGISTRY_HELP
         2.  D_TEST_REGISTRY_FLAG
         3.  D_TEST_REGISTRY_TYPE
9.  PREDICATE FUNCTIONS FOR FILTERED ITERATION
    ------------------------------------------
    1.  Row predicates
10. ITERATION MACROS
    ----------------
    1.  Iteration
         1.  D_TEST_REGISTRY_FOREACH
         2.  D_TEST_REGISTRY_FOREACH_CONFIG
         3.  D_TEST_REGISTRY_FOREACH_METADATA
*/

#ifndef DJINTERP_C_TEST_TEST_CVAR_H
#define DJINTERP_C_TEST_TEST_CVAR_H 1

// std
#include <stddef.h>  // size_t, NULL
// djinterp
#include "../../djinterp.h"                     // framework root
#include "../../container/registry/registry.h"  // dj_registry, its iterators
#include "../../meta/type_info.h"               // D_TYPE_INFO_*
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // int32_t, uint16_t,
                                                   // uint32_t


//==============================================================================
// 1.  KEYS
//==============================================================================
// A row's flag: the config keys count from 0, the metadata flags from 0x100,
// so the two never collide in a scan by flag.


// 1.1    Config keys
//------------------------------------------------------------------------------
// 1.1.1
// DTestConfigKey
//   enum: the keys of the configuration settings, each with its value's type.
enum DTestConfigKey
{
    D_TEST_OPTIONS_ENABLED,          // bool: custom config is enabled
    D_TEST_OPTIONS_INDENT_MAX_LEVEL, // uint16_t: the deepest indentation
    D_TEST_OPTIONS_INDENT_STR,       // const char*: one level's indentation
    D_TEST_OPTIONS_MAX_FAILURES,     // size_t: failures before stopping
    D_TEST_OPTIONS_MESSAGE_FLAGS,    // uint32_t: packed message flags
    D_TEST_OPTIONS_NAME,             // const char*: test name (no row)
    D_TEST_OPTIONS_PRIORITY,         // int32_t: test priority level
    D_TEST_OPTIONS_SKIP,             // bool: the test is skipped
    D_TEST_OPTIONS_TIMEOUT_MS        // size_t: test timeout in milliseconds
};

// 1.2    Metadata flags
//------------------------------------------------------------------------------
// 1.2.1
// DTestMetadataFlag
//   enum: the keys of a test tree node's metadata, each with its value's type.
// Starts at 0x100, past every DTestConfigKey, so d_test_registry_get, _set and
// _reset, which scan by flag, cannot confuse the two.
enum DTestMetadataFlag
{
    D_TEST_METADATA_UNKNOWN = 0x100,
    D_TEST_METADATA_AUTHORS,            // char**: author(s)' name(s)
    D_TEST_METADATA_CATEGORY,           // char*:  test category
    D_TEST_METADATA_CUSTOM_DATA,        // void*:  arbitrary custom data
    D_TEST_METADATA_DATE_CREATED,       // size_t: creation timestamp
    D_TEST_METADATA_DATE_MODIFIED,      // size_t: modification timestamp
    D_TEST_METADATA_DESCRIPTION,        // char*:  node description
    D_TEST_METADATA_DEPENDENCIES,       // char**: array of dependencies
    D_TEST_METADATA_FRAMEWORK_NAME,     // char*:  name of framework
    D_TEST_METADATA_MODULE_NAME,        // char*:  name of module
    D_TEST_METADATA_SUBMODULE_NAME,     // char*:  name of submodule
    D_TEST_METADATA_NAME,               // char*:  name of test
    D_TEST_METADATA_NOTES,              // char**: note(s)
    D_TEST_METADATA_SKIP_REASON,        // char*:  reason for skipping
    D_TEST_METADATA_TAGS,               // char**: array of tags
    D_TEST_METADATA_TIMESTAMP_FORMAT,   // char*:  timestamp format string
    D_TEST_METADATA_VERSION,            // void*:  version object
    D_TEST_METADATA_VERSION_STRING      // char*:  version string
};


//==============================================================================
// 2.  REGISTRY ROW FLAGS
//==============================================================================


// 2.1    Row flags
//------------------------------------------------------------------------------
// 2.1.1
// DTestRegistryRowFlag
//   enum: a row's command flags: what kind of entry it is, and where it
// applies.
enum DTestRegistryRowFlag
{
    D_TEST_REGISTRY_FLAG_IS_REQUIRED = 1u << 0,
    D_TEST_REGISTRY_FLAG_IS_CONFIG   = 1u << 1,
    D_TEST_REGISTRY_FLAG_IS_METADATA = 1u << 2,
    D_TEST_REGISTRY_FLAG_TEST_FN     = 1u << 3,
    D_TEST_REGISTRY_FLAG_ASSERTS     = 1u << 4,
    D_TEST_REGISTRY_FLAG_TESTS       = 1u << 5,
    D_TEST_REGISTRY_FLAG_BLOCKS      = 1u << 6,
    D_TEST_REGISTRY_FLAG_MODULES     = 1u << 7,
    D_TEST_REGISTRY_FLAG_SESSION     = 1u << 8
};


//==============================================================================
// 3.  VALUE UNION
//==============================================================================


// 3.1    Value
//------------------------------------------------------------------------------
// 3.1.1
// dj_test_value
//   union: a row's value; the row's `value_type` says which member holds it.
union dj_test_value
{
    const void* ptr;
    size_t      z;
    uint32_t    u32;
    uint16_t    u16;
    int32_t     i32;
    bool        b;
};


//==============================================================================
// 4.  ROW STRUCTURE
//==============================================================================


// 4.1    Row
//------------------------------------------------------------------------------
// 4.1.1
// dj_test_registry_row
//   struct: one row of the test registry. The key must be the first member,
// as dj_registry's lookup reads it there.
struct dj_test_registry_row
{
    const char*         key;           // primary lookup key (MUST BE FIRST)
    uint32_t            flag;          // DTestConfigKey or DTestMetadataFlag
    uint16_t            command_flags; // row flags (DTestRegistryRowFlag)
    uint32_t            value_type;    // d_type_info: how to read the value
    union dj_test_value value;         // current value
    const char*         help;          // help text
};


//==============================================================================
// 5.  PUBLIC API
//==============================================================================
// Every function makes d_test_registry_init's call first, so none needs it.


// 5.1    Initialization
//------------------------------------------------------------------------------
/**
 * @brief Builds the registry's lookup and its aliases, and records every row's
 *        default; the first call only.
 */
void                d_test_registry_init(void);

/**
 * @brief The registry itself, initialized, for dj_registry's own calls.
 *
 * @return the registry; never `NULL`.
 */
struct dj_registry* d_test_registry_registry(void);

// 5.2    Row access
//------------------------------------------------------------------------------
/**
 * @brief Finds a row by its key or an alias of it.
 *
 * @param[in] _key  the key; may be `NULL`.
 * @return the row, or `NULL` if `_key` is `NULL` or unknown.
 */
struct dj_test_registry_row* d_test_registry_find(const char* _key);

/**
 * @brief Finds a row by its flag.
 *
 * @param[in] _flag  a DTestConfigKey or a DTestMetadataFlag.
 * @return the row, or `NULL` if no row has `_flag`.
 */
struct dj_test_registry_row* d_test_registry_find_by_flag(uint32_t _flag);

// 5.3    Value access
//------------------------------------------------------------------------------
/**
 * @brief Sets a row's value.
 *
 * @param[in] _flag   the row's flag.
 * @param[in] _value  the value, read by the row's `value_type`.
 * @return `true` if set; `false` if the registry is frozen or no row has
 *         `_flag`.
 */
bool                d_test_registry_set(uint32_t            _flag,
                                        union dj_test_value _value);

/**
 * @brief Reads a row's value.
 *
 * @param[in] _flag  the row's flag.
 * @return the value, or a `NULL` pointer value if no row has `_flag`.
 */
union dj_test_value d_test_registry_get(uint32_t _flag);

/**
 * @brief Restores a row's value to its default.
 *
 * @param[in] _flag  the row's flag; a flag no row has does nothing.
 * @post Nothing changes while the registry is frozen.
 */
void                d_test_registry_reset(uint32_t _flag);

/**
 * @brief Restores every row's value to its default.
 *
 * @post Nothing changes while the registry is frozen.
 */
void                d_test_registry_reset_all(void);

// 5.4    Argument validation
//------------------------------------------------------------------------------
/**
 * @brief Whether a key names a row that carries a command flag.
 *
 * @param[in] _key                    the key or an alias; may be `NULL`.
 * @param[in] _required_command_flag  the DTestRegistryRowFlag bits, any of
 *                                    which will do.
 * @return `true` if `_key` names a row with any of those bits set.
 */
bool d_test_registry_is_valid_arg(const char* _key,
                                  uint16_t    _required_command_flag);


//==============================================================================
// 6.  DEFAULT VALUE HELPERS
//==============================================================================
// Declared but never defined: commented out until implemented (2026.09.30).
// union dj_test_value d_test_registry_get_default(uint32_t _flag);
// union dj_test_value d_test_registry_get_default_by_key(const char* _key);


//==============================================================================
// 7.  DEFAULT VALUES
//==============================================================================


// 7.1    Defaults
//------------------------------------------------------------------------------
// 7.1.1
// D_TEST_DEFAULT_INDENT
//   constant: one level's indentation, the "indent-string" row's default.
#define D_TEST_DEFAULT_INDENT       "  "

// 7.1.2
// D_TEST_DEFAULT_MAX_INDENT
//   constant: the deepest indentation, the "max-indent" row's default.
#define D_TEST_DEFAULT_MAX_INDENT   ( (uint16_t)10 )

// 7.1.3
// D_TEST_DEFAULT_MAX_FAILURES
//   constant: failures before stopping, 0 for no limit; the "max-failures"
// row's default.
#define D_TEST_DEFAULT_MAX_FAILURES ( (size_t)0 )

// 7.1.4
// D_TEST_DEFAULT_TIMEOUT
//   constant: a test's timeout in milliseconds, the "timeout" row's default.
#define D_TEST_DEFAULT_TIMEOUT      ( (size_t)1000 )


//==============================================================================
// 8.  TYPED ACCESS MACROS
//==============================================================================
// Each looks a row up by key and reads one field; an unknown key reads NULL,
// false or 0.


// 8.1    Row lookup
//------------------------------------------------------------------------------
// 8.1.1
// D_TEST_REGISTRY_GET
//   macro: looks up a registry row by key string.
#define D_TEST_REGISTRY_GET(key)                                              \
    D_REGISTRY_GET(d_test_registry_registry(),                                \
                   key,                                                       \
                   struct dj_test_registry_row)

// 8.2    Values
//------------------------------------------------------------------------------
// 8.2.1
// D_TEST_REGISTRY_VALUE_PTR
//   macro: gets the pointer value from a registry row.
#define D_TEST_REGISTRY_VALUE_PTR(key)                                        \
    (D_TEST_REGISTRY_GET(key)                                                 \
        ? D_TEST_REGISTRY_GET(key)->value.ptr                                 \
        : NULL)

// 8.2.2
// D_TEST_REGISTRY_VALUE_BOOL
//   macro: gets the boolean value from a registry row.
#define D_TEST_REGISTRY_VALUE_BOOL(key)                                       \
    (D_TEST_REGISTRY_GET(key)                                                 \
        ? D_TEST_REGISTRY_GET(key)->value.b                                   \
        : false)

// 8.2.3
// D_TEST_REGISTRY_VALUE_SIZE_T
//   macro: gets the size_t value from a registry row.
#define D_TEST_REGISTRY_VALUE_SIZE_T(key)                                     \
    (D_TEST_REGISTRY_GET(key)                                                 \
        ? D_TEST_REGISTRY_GET(key)->value.z                                   \
        : 0)

// 8.2.4
// D_TEST_REGISTRY_VALUE_UINT32
//   macro: gets the uint32_t value from a registry row.
#define D_TEST_REGISTRY_VALUE_UINT32(key)                                     \
    (D_TEST_REGISTRY_GET(key)                                                 \
        ? D_TEST_REGISTRY_GET(key)->value.u32                                 \
        : 0)

// 8.2.5
// D_TEST_REGISTRY_VALUE_UINT16
//   macro: gets the uint16_t value from a registry row.
#define D_TEST_REGISTRY_VALUE_UINT16(key)                                     \
    (D_TEST_REGISTRY_GET(key)                                                 \
        ? D_TEST_REGISTRY_GET(key)->value.u16                                 \
        : 0)

// 8.2.6
// D_TEST_REGISTRY_VALUE_INT32
//   macro: gets the int32_t value from a registry row.
#define D_TEST_REGISTRY_VALUE_INT32(key)                                      \
    (D_TEST_REGISTRY_GET(key)                                                 \
        ? D_TEST_REGISTRY_GET(key)->value.i32                                 \
        : 0)

// 8.3    Row fields
//------------------------------------------------------------------------------
// 8.3.1
// D_TEST_REGISTRY_HELP
//   macro: gets the help text from a registry row.
#define D_TEST_REGISTRY_HELP(key)                                             \
    (D_TEST_REGISTRY_GET(key)                                                 \
        ? D_TEST_REGISTRY_GET(key)->help                                      \
        : NULL)

// 8.3.2
// D_TEST_REGISTRY_FLAG
//   macro: gets the flag value from a registry row.
#define D_TEST_REGISTRY_FLAG(key)                                             \
    (D_TEST_REGISTRY_GET(key)                                                 \
        ? D_TEST_REGISTRY_GET(key)->flag                                      \
        : 0)

// 8.3.3
// D_TEST_REGISTRY_TYPE
//   macro: gets the value_type from a registry row.
#define D_TEST_REGISTRY_TYPE(key)                                             \
    (D_TEST_REGISTRY_GET(key)                                                 \
        ? D_TEST_REGISTRY_GET(key)->value_type                                \
        : 0)


//==============================================================================
// 9.  PREDICATE FUNCTIONS FOR FILTERED ITERATION
//==============================================================================


// 9.1    Row predicates
//------------------------------------------------------------------------------
// filters for d_registry_iterator_filtered: whether `_row`, a
// dj_test_registry_row, is a config, a metadata or a required row; `_context`
// is unused
bool d_test_registry_is_config_row(const void* _row,
                                   const void* _context);
bool d_test_registry_is_metadata_row(const void* _row,
                                     const void* _context);
bool d_test_registry_is_required_row(const void* _row,
                                     const void* _context);


//==============================================================================
// 10. ITERATION MACROS
//==============================================================================


// 10.1   Iteration
//------------------------------------------------------------------------------
// 10.1.1
// D_TEST_REGISTRY_FOREACH
//   macro: iterates over all rows in the test registry, declaring `var_name`.
#define D_TEST_REGISTRY_FOREACH(var_name)                                     \
    D_REGISTRY_FOREACH(d_test_registry_registry(),                            \
                       struct dj_test_registry_row,                           \
                       var_name)

// 10.1.2
// D_TEST_REGISTRY_FOREACH_CONFIG
//   macro: iterates over config rows only, declaring `var_name`.
#define D_TEST_REGISTRY_FOREACH_CONFIG(var_name)                              \
    for (struct dj_registry_iterator D_CONCAT(var_name, _iter) =              \
             d_registry_iterator_filtered(d_test_registry_registry(),         \
                                          d_test_registry_is_config_row,      \
                                          NULL);                              \
         d_registry_iterator_has_next(&D_CONCAT(var_name, _iter)); )          \
        for (struct dj_test_registry_row* var_name =                          \
                 d_registry_iterator_next(&D_CONCAT(var_name, _iter));        \
             var_name != NULL;                                                \
             var_name = NULL)

// 10.1.3
// D_TEST_REGISTRY_FOREACH_METADATA
//   macro: iterates over metadata rows only, declaring `var_name`.
#define D_TEST_REGISTRY_FOREACH_METADATA(var_name)                            \
    for (struct dj_registry_iterator D_CONCAT(var_name, _iter) =              \
             d_registry_iterator_filtered(d_test_registry_registry(),         \
                                          d_test_registry_is_metadata_row,    \
                                          NULL);                              \
         d_registry_iterator_has_next(&D_CONCAT(var_name, _iter)); )          \
        for (struct dj_test_registry_row* var_name =                          \
                 d_registry_iterator_next(&D_CONCAT(var_name, _iter));        \
             var_name != NULL;                                                \
             var_name = NULL)


#endif  // DJINTERP_C_TEST_TEST_CVAR_H
