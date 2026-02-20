/******************************************************************************
* djinterp [test]                                                  test_cvar.h
*
*   Registry-based configuration + metadata schema for the DTest framework.
*   The test registry provides:
*     - string-key lookup (with aliases)
*     - default values for config + metadata keys
*     - d_type_info for interpreting each value
*   This module is intentionally small and uses the generic `d_registry`
* container. Storage and lookup tables live in test_registry.c.
*
* 
* path:      \inc\test\test_cvar.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.16
******************************************************************************/

#ifndef DJINTERP_TEST_CVAR_
#define DJINTERP_TEST_CVAR_ 1

#include <stddef.h>
#include <stdint.h>
#include "..\djinterp.h"
#include "..\container\registry\registry.h"
#include "..\meta\type_info.h"
#include ".\test_config.h"


/******************************************************************************
 * REGISTRY ROW FLAGS
 *****************************************************************************/

// DTestRegistryRowFlag
//   enum: flags for categorizing registry row entries.
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


/******************************************************************************
 * VALUE UNION
 *****************************************************************************/

// d_test_value
//   union: variant type for storing different value types in registry rows.
union d_test_value
{
    const void* ptr;
    size_t      z;
    uint32_t    u32;
    uint16_t    u16;
    int32_t     i32;
    bool        b;
};


/******************************************************************************
 * ROW STRUCTURE
 *****************************************************************************/

// d_test_registry_row
//   struct: row structure for the test configuration registry.
// First member MUST be the key string for registry lookup.
struct d_test_registry_row
{
    const char*        key;           // primary lookup key (MUST BE FIRST)
    uint32_t           flag;          // enum flag (DTestConfigKey or DTestMetadataFlag)
    uint16_t           command_flags; // row flags (DTestRegistryRowFlag)
    uint32_t           value_type;    // d_type_info for interpretation
    union d_test_value value;         // current/default value
    const char*        help;          // help text description
};


/******************************************************************************
 * PUBLIC API
 *****************************************************************************/

// initialization + access to underlying registry
void               d_test_registry_init(void);
struct d_registry* d_test_registry_registry(void);

// row access
struct d_test_registry_row* d_test_registry_find(const char* _key);
struct d_test_registry_row* d_test_registry_find_by_flag(uint32_t _flag);

// value access
bool               d_test_registry_set(uint32_t _flag, union d_test_value _value);
union d_test_value d_test_registry_get(uint32_t _flag);
void               d_test_registry_reset(uint32_t _flag);
void               d_test_registry_reset_all(void);

// arg validation
bool d_test_registry_is_valid_arg(const char* _key, 
                                  uint16_t    _required_command_flag);


/******************************************************************************
 * DEFAULT VALUE HELPERS
 *****************************************************************************/

union d_test_value d_test_registry_get_default(uint32_t _flag);
union d_test_value d_test_registry_get_default_by_key(const char* _key);

/******************************************************************************
 * TYPED ACCESS MACROS
 *****************************************************************************/

// D_TEST_REGISTRY_GET
//   macro: looks up a registry row by key string.
#define D_TEST_REGISTRY_GET(key) \
    D_REGISTRY_GET(d_test_registry_registry(), key, struct d_test_registry_row)

// D_TEST_REGISTRY_VALUE_PTR
//   macro: gets the pointer value from a registry row.
#define D_TEST_REGISTRY_VALUE_PTR(key) \
    (D_TEST_REGISTRY_GET(key) ? D_TEST_REGISTRY_GET(key)->value.ptr : NULL)

// D_TEST_REGISTRY_VALUE_BOOL
//   macro: gets the boolean value from a registry row.
#define D_TEST_REGISTRY_VALUE_BOOL(key) \
    (D_TEST_REGISTRY_GET(key) ? D_TEST_REGISTRY_GET(key)->value.b : false)

// D_TEST_REGISTRY_VALUE_SIZE_T
//   macro: gets the size_t value from a registry row.
#define D_TEST_REGISTRY_VALUE_SIZE_T(key) \
    (D_TEST_REGISTRY_GET(key) ? D_TEST_REGISTRY_GET(key)->value.z : 0)

// D_TEST_REGISTRY_VALUE_UINT32
//   macro: gets the uint32_t value from a registry row.
#define D_TEST_REGISTRY_VALUE_UINT32(key) \
    (D_TEST_REGISTRY_GET(key) ? D_TEST_REGISTRY_GET(key)->value.u32 : 0)

// D_TEST_REGISTRY_VALUE_UINT16
//   macro: gets the uint16_t value from a registry row.
#define D_TEST_REGISTRY_VALUE_UINT16(key) \
    (D_TEST_REGISTRY_GET(key) ? D_TEST_REGISTRY_GET(key)->value.u16 : 0)

// D_TEST_REGISTRY_VALUE_INT32
//   macro: gets the int32_t value from a registry row.
#define D_TEST_REGISTRY_VALUE_INT32(key) \
    (D_TEST_REGISTRY_GET(key) ? D_TEST_REGISTRY_GET(key)->value.i32 : 0)

// D_TEST_REGISTRY_HELP
//   macro: gets the help text from a registry row.
#define D_TEST_REGISTRY_HELP(key) \
    (D_TEST_REGISTRY_GET(key) ? D_TEST_REGISTRY_GET(key)->help : NULL)

// D_TEST_REGISTRY_FLAG
//   macro: gets the flag value from a registry row.
#define D_TEST_REGISTRY_FLAG(key) \
    (D_TEST_REGISTRY_GET(key) ? D_TEST_REGISTRY_GET(key)->flag : 0)

// D_TEST_REGISTRY_TYPE
//   macro: gets the value_type from a registry row.
#define D_TEST_REGISTRY_TYPE(key) \
    (D_TEST_REGISTRY_GET(key) ? D_TEST_REGISTRY_GET(key)->value_type : 0)


/******************************************************************************
 * PREDICATE FUNCTIONS FOR FILTERED ITERATION
 *****************************************************************************/

bool d_test_registry_is_config_row(const void* _row, const void* _context);
bool d_test_registry_is_metadata_row(const void* _row, const void* _context);
bool d_test_registry_is_required_row(const void* _row, const void* _context);


/******************************************************************************
 * ITERATION MACROS
 *****************************************************************************/

// D_TEST_REGISTRY_FOREACH
//   macro: iterates over all rows in the test registry.
#define D_TEST_REGISTRY_FOREACH(var_name) \
    D_REGISTRY_FOREACH(d_test_registry_registry(), \
                       struct d_test_registry_row, \
                       var_name)

// D_TEST_REGISTRY_FOREACH_CONFIG
//   macro: iterates over config rows only.
#define D_TEST_REGISTRY_FOREACH_CONFIG(var_name)                             \
    for (struct d_registry_iterator D_CONCAT(var_name, _iter) =              \
             d_registry_iterator_filtered(d_test_registry_registry(),        \
                                          d_test_registry_is_config_row,     \
                                          NULL);                             \
         d_registry_iterator_has_next(&D_CONCAT(var_name, _iter)); )         \
        for (struct d_test_registry_row* var_name =                          \
                 d_registry_iterator_next(&D_CONCAT(var_name, _iter));       \
             var_name != NULL;                                               \
             var_name = NULL)

// D_TEST_REGISTRY_FOREACH_METADATA
//   macro: iterates over metadata rows only.
#define D_TEST_REGISTRY_FOREACH_METADATA(var_name)                           \
    for (struct d_registry_iterator D_CONCAT(var_name, _iter) =              \
             d_registry_iterator_filtered(d_test_registry_registry(),        \
                                          d_test_registry_is_metadata_row,   \
                                          NULL);                             \
        d_registry_iterator_has_next(&D_CONCAT(var_name, _iter)); )          \
        for (struct d_test_registry_row* var_name =                          \
                 d_registry_iterator_next(&D_CONCAT(var_name, _iter));       \
             var_name != NULL;                                               \
             var_name = NULL)


#endif  // DJINTERP_TEST_CVAR_