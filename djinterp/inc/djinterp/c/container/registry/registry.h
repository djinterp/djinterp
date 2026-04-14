/******************************************************************************
* djinterp [container]                                              registry.h
*
*   A generic registry for storing and retrieving user-defined row structures
* by string key. The core is a simple array of user-defined structs with a
* separate sorted lookup array containing all keys and aliases for binary
* search access.
*   The user defines their own row structure; the first member MUST be a 
* `const char*` key used for lookup.
*
* 
* path:      \inc\container\registry\registry.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.16
******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_REGISTRY_
#define DJINTERP_C_CONTAINER_REGISTRY_ 1

#include <ctype.h>   // tolower
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "..\..\djinterp.h"
#include "..\..\dmemory.h"
#include "..\container.h"


/******************************************************************************
 * CONSTANTS AND DEFAULT VALUES
 *****************************************************************************/

#ifndef D_REGISTRY_DEFAULT_CAPACITY
    // D_REGISTRY_DEFAULT_CAPACITY
    //   constant: the default initial capacity for a new registry.
    #define D_REGISTRY_DEFAULT_CAPACITY 32
#endif  // D_REGISTRY_DEFAULT_CAPACITY

#ifndef D_REGISTRY_GROWTH_FACTOR
    // D_REGISTRY_GROWTH_FACTOR
    //   constant: multiplicative factor when growing registry capacity.
    #define D_REGISTRY_GROWTH_FACTOR 2
#endif  // D_REGISTRY_GROWTH_FACTOR


/******************************************************************************
 * KEY EXTRACTION
 *****************************************************************************/

// D_REGISTRY_ROW_KEY
//   macro: extracts the key from a row pointer, assuming the first member of
// the row struct is a `const char*`.
#define D_REGISTRY_ROW_KEY(row_ptr) \
    (*((const char* const*)(row_ptr)))


/******************************************************************************
 * FLAG DEFINITIONS
 *****************************************************************************/

// DRegistryFlag
//   enum: flags for registry-wide configuration and behavior.
enum DRegistryFlag
{
    D_REGISTRY_FLAG_NONE             = 0x00,
    D_REGISTRY_FLAG_CASE_INSENSITIVE = 0x01,  // key comparisons ignore case
    D_REGISTRY_FLAG_OWNS_ROWS        = 0x02,  // registry frees row memory
    D_REGISTRY_FLAG_STATIC_ROWS      = 0x04,  // rows are static, never free
    D_REGISTRY_FLAG_SORTED           = 0x08,  // rows maintained in key order
    D_REGISTRY_FLAG_FROZEN           = 0x10   // no modifications allowed
};

// D_REGISTRY_FLAG_DEFAULT
//   constant: default flags for newly created registries.
#define D_REGISTRY_FLAG_DEFAULT  D_REGISTRY_FLAG_NONE


/******************************************************************************
 * FLAG MANIPULATION MACROS
 *****************************************************************************/

// D_REGISTRY_HAS_FLAG
//   macro: checks if the registry has a specific flag set.
#define D_REGISTRY_HAS_FLAG(registry, flag) \
    (((registry)->flags & (flag)) == (flag))

// D_REGISTRY_SET_FLAG
//   macro: sets a specific flag on the registry.
#define D_REGISTRY_SET_FLAG(registry, flag) \
    ((registry)->flags |= (flag))

// D_REGISTRY_CLEAR_FLAG
//   macro: clears a specific flag on the registry.
#define D_REGISTRY_CLEAR_FLAG(registry, flag) \
    ((registry)->flags &= ~(flag))


/******************************************************************************
 * SEMANTIC CHECK MACROS
 *****************************************************************************/

// D_REGISTRY_IS_CASE_INSENSITIVE
//   macro: checks if the registry uses case-insensitive key comparisons.
#define D_REGISTRY_IS_CASE_INSENSITIVE(registry) \
    D_REGISTRY_HAS_FLAG(registry, D_REGISTRY_FLAG_CASE_INSENSITIVE)

// D_REGISTRY_IS_FROZEN
//   macro: checks if the registry is frozen (read-only).
#define D_REGISTRY_IS_FROZEN(registry) \
    D_REGISTRY_HAS_FLAG(registry, D_REGISTRY_FLAG_FROZEN)

// D_REGISTRY_OWNS_ROWS
//   macro: checks if the registry owns (and will free) row memory.
#define D_REGISTRY_OWNS_ROWS(registry) \
    D_REGISTRY_HAS_FLAG(registry, D_REGISTRY_FLAG_OWNS_ROWS)


/******************************************************************************
 * FUNCTION POINTER TYPES
 *****************************************************************************/

// fn_registry_row_free
//   typedef: function pointer for freeing a user's row structure.
typedef void (*fn_registry_row_free)(void* _row);

// fn_registry_row_visitor
//   typedef: function pointer for visiting rows during iteration.
// Return false to stop iteration early.
typedef bool (*fn_registry_row_visitor)(void* _row, void* _context);

// fn_registry_row_predicate
//   typedef: function pointer for filtering rows.
// Returns true if the row matches the predicate.
typedef bool (*fn_registry_row_predicate)(const void* _row, const void* _context);


/******************************************************************************
 * CORE STRUCTURES
 *****************************************************************************/

// d_registry_lookup_entry
//   struct: internal entry in the lookup array mapping a key or alias string
// to a row index.
struct d_registry_lookup_entry
{
    const char* key;        // key or alias string
    size_t      row_index;  // index into the rows array
};

// d_registry
//   struct: the main registry container. Stores user-defined rows in a simple
// array with a separate sorted lookup array for binary search by key/alias.
struct d_registry
{
    void*                           rows;           // array of user row structs
    size_t                          row_size;       // sizeof(user row struct)
    size_t                          count;          // number of rows
    size_t                          capacity;       // allocated row capacity
    struct d_registry_lookup_entry* lookup;         // sorted key/alias array
    size_t                          lookup_count;   // entries in lookup
    size_t                          lookup_capacity;// allocated lookup capacity
    uint8_t                         flags;          // registry-wide flags
    fn_registry_row_free            row_free;       // optional row destructor
};

// d_registry_iterator
//   struct: iterator for traversing registry entries with optional filtering.
struct d_registry_iterator
{
    struct d_registry*        registry;     // registry being iterated
    size_t                    current;      // current row index
    fn_registry_row_predicate filter;       // optional filter predicate
    const void*               filter_ctx;   // filter context
};


/******************************************************************************
 * STATIC TABLE GENERATION MACROS
 *****************************************************************************/

// D_REGISTRY_ROW
//   macro: declares a single row entry for use in D_REGISTRY_TABLE.
// Usage: D_REGISTRY_ROW("key", value1, value2, ...)
#define D_REGISTRY_ROW(...) { __VA_ARGS__ }

// D_REGISTRY_TABLE
//   macro: declares and initializes a static array of row structs.
// Usage: 
//   D_REGISTRY_TABLE(my_rows, struct my_row_type,
//       D_REGISTRY_ROW("key1", val1, val2),
//       D_REGISTRY_ROW("key2", val3, val4)
//   );
#define D_REGISTRY_TABLE(name, row_type, ...)                               \
    static row_type name[] = { __VA_ARGS__ }

// D_REGISTRY_TABLE_COUNT
//   macro: gets the count of rows in a static table.
#define D_REGISTRY_TABLE_COUNT(table) \
    (sizeof(table) / sizeof((table)[0]))

// D_REGISTRY_LOOKUP_ENTRY
//   macro: declares a single lookup entry for use in D_REGISTRY_LOOKUP_TABLE.
#define D_REGISTRY_LOOKUP_ENTRY(key_str, idx) \
    { .key = (key_str), .row_index = (idx) }

// D_REGISTRY_LOOKUP_TABLE
//   macro: declares and initializes a static lookup array. Entries should be
// in sorted order by key for binary search.
// Usage:
//   D_REGISTRY_LOOKUP_TABLE(my_lookup,
//       D_REGISTRY_LOOKUP_ENTRY("alias1", 0),
//       D_REGISTRY_LOOKUP_ENTRY("key1", 0),
//       D_REGISTRY_LOOKUP_ENTRY("key2", 1)
//   );
#define D_REGISTRY_LOOKUP_TABLE(name, ...)                                  \
    static struct d_registry_lookup_entry name[] = { __VA_ARGS__ }

// D_REGISTRY_LOOKUP_COUNT
//   macro: gets the count of entries in a static lookup table.
#define D_REGISTRY_LOOKUP_COUNT(lookup) \
    (sizeof(lookup) / sizeof((lookup)[0]))

// D_REGISTRY_STATIC_INIT
//   macro: initializes a d_registry struct from static tables.
// Usage:
//   struct d_registry reg = D_REGISTRY_STATIC_INIT(
//       my_rows, sizeof(struct my_row), my_lookup
//   );
#define D_REGISTRY_STATIC_INIT(rows_table, row_sz, lookup_table)            \
    {                                                                       \
        .rows            = (void*)(rows_table),                             \
        .row_size        = (row_sz),                                        \
        .count           = D_REGISTRY_TABLE_COUNT(rows_table),              \
        .capacity        = D_REGISTRY_TABLE_COUNT(rows_table),              \
        .lookup          = (lookup_table),                                  \
        .lookup_count    = D_REGISTRY_LOOKUP_COUNT(lookup_table),           \
        .lookup_capacity = D_REGISTRY_LOOKUP_COUNT(lookup_table),           \
        .flags           = D_REGISTRY_FLAG_STATIC_ROWS,                     \
        .row_free        = NULL                                             \
    }

/******************************************************************************
 * AUTOMATIC LOOKUP GENERATION MACROS
 *****************************************************************************/

// D_INTERNAL_REGISTRY_KEY_ENTRY
//   macro: internal helper to generate a lookup entry from a row.
#define D_INTERNAL_REGISTRY_KEY_ENTRY(row_table, idx) \
    { .key = D_REGISTRY_ROW_KEY(&(row_table)[idx]), .row_index = (idx) }

// D_REGISTRY_GEN_LOOKUP_<1-32>
//   macros: generate lookup entries for tables of specific sizes.
#define D_REGISTRY_GEN_LOOKUP_1(t)  D_INTERNAL_REGISTRY_KEY_ENTRY(t, 0)
#define D_REGISTRY_GEN_LOOKUP_2(t)  D_REGISTRY_GEN_LOOKUP_1(t),  D_INTERNAL_REGISTRY_KEY_ENTRY(t, 1)
#define D_REGISTRY_GEN_LOOKUP_3(t)  D_REGISTRY_GEN_LOOKUP_2(t),  D_INTERNAL_REGISTRY_KEY_ENTRY(t, 2)
#define D_REGISTRY_GEN_LOOKUP_4(t)  D_REGISTRY_GEN_LOOKUP_3(t),  D_INTERNAL_REGISTRY_KEY_ENTRY(t, 3)
#define D_REGISTRY_GEN_LOOKUP_5(t)  D_REGISTRY_GEN_LOOKUP_4(t),  D_INTERNAL_REGISTRY_KEY_ENTRY(t, 4)
#define D_REGISTRY_GEN_LOOKUP_6(t)  D_REGISTRY_GEN_LOOKUP_5(t),  D_INTERNAL_REGISTRY_KEY_ENTRY(t, 5)
#define D_REGISTRY_GEN_LOOKUP_7(t)  D_REGISTRY_GEN_LOOKUP_6(t),  D_INTERNAL_REGISTRY_KEY_ENTRY(t, 6)
#define D_REGISTRY_GEN_LOOKUP_8(t)  D_REGISTRY_GEN_LOOKUP_7(t),  D_INTERNAL_REGISTRY_KEY_ENTRY(t, 7)
#define D_REGISTRY_GEN_LOOKUP_9(t)  D_REGISTRY_GEN_LOOKUP_8(t),  D_INTERNAL_REGISTRY_KEY_ENTRY(t, 8)
#define D_REGISTRY_GEN_LOOKUP_10(t) D_REGISTRY_GEN_LOOKUP_9(t),  D_INTERNAL_REGISTRY_KEY_ENTRY(t, 9)
#define D_REGISTRY_GEN_LOOKUP_11(t) D_REGISTRY_GEN_LOOKUP_10(t), D_INTERNAL_REGISTRY_KEY_ENTRY(t, 10)
#define D_REGISTRY_GEN_LOOKUP_12(t) D_REGISTRY_GEN_LOOKUP_11(t), D_INTERNAL_REGISTRY_KEY_ENTRY(t, 11)
#define D_REGISTRY_GEN_LOOKUP_13(t) D_REGISTRY_GEN_LOOKUP_12(t), D_INTERNAL_REGISTRY_KEY_ENTRY(t, 12)
#define D_REGISTRY_GEN_LOOKUP_14(t) D_REGISTRY_GEN_LOOKUP_13(t), D_INTERNAL_REGISTRY_KEY_ENTRY(t, 13)
#define D_REGISTRY_GEN_LOOKUP_15(t) D_REGISTRY_GEN_LOOKUP_14(t), D_INTERNAL_REGISTRY_KEY_ENTRY(t, 14)
#define D_REGISTRY_GEN_LOOKUP_16(t) D_REGISTRY_GEN_LOOKUP_15(t), D_INTERNAL_REGISTRY_KEY_ENTRY(t, 15)
#define D_REGISTRY_GEN_LOOKUP_17(t) D_REGISTRY_GEN_LOOKUP_16(t), D_INTERNAL_REGISTRY_KEY_ENTRY(t, 16)
#define D_REGISTRY_GEN_LOOKUP_18(t) D_REGISTRY_GEN_LOOKUP_17(t), D_INTERNAL_REGISTRY_KEY_ENTRY(t, 17)
#define D_REGISTRY_GEN_LOOKUP_19(t) D_REGISTRY_GEN_LOOKUP_18(t), D_INTERNAL_REGISTRY_KEY_ENTRY(t, 18)
#define D_REGISTRY_GEN_LOOKUP_20(t) D_REGISTRY_GEN_LOOKUP_19(t), D_INTERNAL_REGISTRY_KEY_ENTRY(t, 19)
#define D_REGISTRY_GEN_LOOKUP_21(t) D_REGISTRY_GEN_LOOKUP_20(t), D_INTERNAL_REGISTRY_KEY_ENTRY(t, 20)
#define D_REGISTRY_GEN_LOOKUP_22(t) D_REGISTRY_GEN_LOOKUP_21(t), D_INTERNAL_REGISTRY_KEY_ENTRY(t, 21)
#define D_REGISTRY_GEN_LOOKUP_23(t) D_REGISTRY_GEN_LOOKUP_22(t), D_INTERNAL_REGISTRY_KEY_ENTRY(t, 22)
#define D_REGISTRY_GEN_LOOKUP_24(t) D_REGISTRY_GEN_LOOKUP_23(t), D_INTERNAL_REGISTRY_KEY_ENTRY(t, 23)
#define D_REGISTRY_GEN_LOOKUP_25(t) D_REGISTRY_GEN_LOOKUP_24(t), D_INTERNAL_REGISTRY_KEY_ENTRY(t, 24)
#define D_REGISTRY_GEN_LOOKUP_26(t) D_REGISTRY_GEN_LOOKUP_25(t), D_INTERNAL_REGISTRY_KEY_ENTRY(t, 25)
#define D_REGISTRY_GEN_LOOKUP_27(t) D_REGISTRY_GEN_LOOKUP_26(t), D_INTERNAL_REGISTRY_KEY_ENTRY(t, 26)
#define D_REGISTRY_GEN_LOOKUP_28(t) D_REGISTRY_GEN_LOOKUP_27(t), D_INTERNAL_REGISTRY_KEY_ENTRY(t, 27)
#define D_REGISTRY_GEN_LOOKUP_29(t) D_REGISTRY_GEN_LOOKUP_28(t), D_INTERNAL_REGISTRY_KEY_ENTRY(t, 28)
#define D_REGISTRY_GEN_LOOKUP_30(t) D_REGISTRY_GEN_LOOKUP_29(t), D_INTERNAL_REGISTRY_KEY_ENTRY(t, 29)
#define D_REGISTRY_GEN_LOOKUP_31(t) D_REGISTRY_GEN_LOOKUP_30(t), D_INTERNAL_REGISTRY_KEY_ENTRY(t, 30)
#define D_REGISTRY_GEN_LOOKUP_32(t) D_REGISTRY_GEN_LOOKUP_31(t), D_INTERNAL_REGISTRY_KEY_ENTRY(t, 31)

// D_REGISTRY_AUTO_LOOKUP
//   macro: generates an unsorted lookup table from a rows table.
// Note: the generated lookup is NOT sorted; call d_registry_sort_lookup()
// or use D_REGISTRY_DEFINE for automatic sorting at init.
// Usage:
//   struct d_registry_lookup_entry my_lookup[] = {
//       D_REGISTRY_AUTO_LOOKUP(my_rows, 5)
//   };
#define D_REGISTRY_AUTO_LOOKUP(rows_table, n) \
    D_CONCAT(D_REGISTRY_GEN_LOOKUP_, n)(rows_table)


/******************************************************************************
 * COMPLETE REGISTRY DEFINITION MACROS
 *****************************************************************************/

// D_REGISTRY_DEFINE
//   macro: defines a complete static registry with rows table, lookup table,
// and registry struct. The lookup table will need runtime sorting.
// Usage:
//   D_REGISTRY_DEFINE(my_registry, struct my_row,
//       D_REGISTRY_ROW("zebra", 1, 2),
//       D_REGISTRY_ROW("apple", 3, 4),
//       D_REGISTRY_ROW("mango", 5, 6)
//   );
//   // Then call: d_registry_rebuild_lookup(&my_registry);
#define D_REGISTRY_DEFINE(name, row_type, ...)                              \
    static row_type D_CONCAT(name, _rows)[] = { __VA_ARGS__ };              \
    static struct d_registry_lookup_entry D_CONCAT(name, _lookup)[          \
        D_REGISTRY_TABLE_COUNT(D_CONCAT(name, _rows))];                     \
    static struct d_registry name = {                                       \
        .rows            = (void*)D_CONCAT(name, _rows),                    \
        .row_size        = sizeof(row_type),                                \
        .count           = D_REGISTRY_TABLE_COUNT(D_CONCAT(name, _rows)),   \
        .capacity        = D_REGISTRY_TABLE_COUNT(D_CONCAT(name, _rows)),   \
        .lookup          = D_CONCAT(name, _lookup),                         \
        .lookup_count    = 0,                                               \
        .lookup_capacity = D_REGISTRY_TABLE_COUNT(D_CONCAT(name, _rows)),   \
        .flags           = D_REGISTRY_FLAG_STATIC_ROWS,                     \
        .row_free        = NULL                                             \
    }

// D_REGISTRY_DEFINE_WITH_ALIASES
//   macro: defines a registry with extra capacity for aliases in lookup.
#define D_REGISTRY_DEFINE_WITH_ALIASES(name, row_type, alias_extra, ...)    \
    static row_type D_CONCAT(name, _rows)[] = { __VA_ARGS__ };              \
    static struct d_registry_lookup_entry D_CONCAT(name, _lookup)[          \
        D_REGISTRY_TABLE_COUNT(D_CONCAT(name, _rows)) + (alias_extra)];     \
    static struct d_registry name = {                                       \
        .rows            = (void*)D_CONCAT(name, _rows),                    \
        .row_size        = sizeof(row_type),                                \
        .count           = D_REGISTRY_TABLE_COUNT(D_CONCAT(name, _rows)),   \
        .capacity        = D_REGISTRY_TABLE_COUNT(D_CONCAT(name, _rows)),   \
        .lookup          = D_CONCAT(name, _lookup),                         \
        .lookup_count    = 0,                                               \
        .lookup_capacity = D_REGISTRY_TABLE_COUNT(D_CONCAT(name, _rows))    \
                           + (alias_extra),                                 \
        .flags           = D_REGISTRY_FLAG_STATIC_ROWS,                     \
        .row_free        = NULL                                             \
    }

// D_REGISTRY_INIT
//   macro: runtime initialization call for a D_REGISTRY_DEFINE'd registry.
// Populates and sorts the lookup table.
// Usage: D_REGISTRY_INIT(&my_registry);
#define D_REGISTRY_INIT(registry_ptr) \
    d_registry_rebuild_lookup(registry_ptr)


/******************************************************************************
 * ALIAS DEFINITION MACROS
 *****************************************************************************/

// D_REGISTRY_ALIAS
//   macro: defines an alias entry to be added at runtime.
// Usage: d_registry_add_alias(&reg, "primary_key", "my_alias");
#define D_REGISTRY_ALIAS(key, alias) \
    { .key = (alias), .row_index = 0 }  // row_index set by add_alias

// D_REGISTRY_ALIASES
//   macro: helper for defining multiple aliases for a key.
// Usage: 
//   const char* my_aliases[] = D_REGISTRY_ALIASES("alt1", "alt2", "alt3");
#define D_REGISTRY_ALIASES(...) { __VA_ARGS__, NULL }

/******************************************************************************
 * ROW ACCESS MACROS
 *****************************************************************************/

// D_REGISTRY_ROW_AT
//   macro: gets a pointer to the row at the specified index.
#define D_REGISTRY_ROW_AT(registry, index, row_type) \
    ((row_type*)((char*)(registry)->rows + ((index) * (registry)->row_size)))

// D_REGISTRY_GET
//   macro: looks up a row by key and casts to the specified type.
// Returns NULL if not found.
#define D_REGISTRY_GET(registry, key_str, row_type) \
    ((row_type*)d_registry_get(registry, key_str))

// D_REGISTRY_GET_VALUE
//   macro: looks up a row by key and returns a specific field value.
// Evaluates to the field's default/zero value if not found.
#define D_REGISTRY_GET_VALUE(registry, key_str, row_type, field)            \
    (D_REGISTRY_GET(registry, key_str, row_type)                            \
        ? D_REGISTRY_GET(registry, key_str, row_type)->field                \
        : (row_type){0}.field)

// D_REGISTRY_FOREACH
//   macro: iterates over all rows in the registry.
// Usage:
//   D_REGISTRY_FOREACH(&reg, struct my_row, row) {
//       printf("%s\n", row->key);
//   }
#define D_REGISTRY_FOREACH(registry, row_type, var_name)                    \
    for (size_t D_CONCAT(var_name, _idx) = 0;                               \
         D_CONCAT(var_name, _idx) < (registry)->count;                      \
         D_CONCAT(var_name, _idx)++)                                        \
        for (row_type* var_name = D_REGISTRY_ROW_AT(registry,               \
                                                    D_CONCAT(var_name, _idx),\
                                                    row_type);              \
             var_name != NULL;                                              \
             var_name = NULL)


/******************************************************************************
 * CONSTRUCTOR FUNCTIONS
 *****************************************************************************/

struct d_registry* d_registry_new(size_t _row_size);
struct d_registry* d_registry_new_with_capacity(size_t _row_size, size_t _capacity);
struct d_registry* d_registry_new_copy(const struct d_registry* _other);
struct d_registry* d_registry_new_from_array(const void* _rows, size_t _row_size, size_t _count);


/******************************************************************************
 * PRIMARY LOOKUP FUNCTION
 *****************************************************************************/

// d_registry_get
//   function: binary searches the lookup array for the key and returns the
// matching row, or NULL if not found. This is the primary access method.
void* d_registry_get(const struct d_registry* _registry, const char* _key);


/******************************************************************************
 * ROW MANIPULATION FUNCTIONS
 *****************************************************************************/

// adding rows
bool   d_registry_add(struct d_registry* _registry, const void* _row);
bool   d_registry_add_rows(struct d_registry* _registry, const void* _rows, size_t _count);
bool   d_registry_set(struct d_registry* _registry, const char* _key, const void* _row);

// removing rows
bool   d_registry_remove(struct d_registry* _registry, const char* _key);
bool   d_registry_remove_at(struct d_registry* _registry, size_t _index);
void   d_registry_clear(struct d_registry* _registry);


/******************************************************************************
 * ALIAS FUNCTIONS
 *****************************************************************************/

bool   d_registry_add_alias(struct d_registry* _registry, const char* _key, const char* _alias);
bool   d_registry_add_aliases(struct d_registry* _registry, const char* _key, const char** _aliases, size_t _count);
bool   d_registry_remove_alias(struct d_registry* _registry, const char* _alias);
void   d_registry_clear_aliases(struct d_registry* _registry);
size_t d_registry_alias_count(const struct d_registry* _registry);


/******************************************************************************
 * QUERY FUNCTIONS
 *****************************************************************************/

bool    d_registry_contains(const struct d_registry* _registry, const char* _key);
ssize_t d_registry_index_of(const struct d_registry* _registry, const char* _key);
void*   d_registry_at(const struct d_registry* _registry, size_t _index);
size_t  d_registry_count(const struct d_registry* _registry);
size_t  d_registry_capacity(const struct d_registry* _registry);
bool    d_registry_is_empty(const struct d_registry* _registry);


/******************************************************************************
 * LOOKUP MAINTENANCE FUNCTIONS
 *****************************************************************************/

// d_registry_rebuild_lookup
//   function: clears and rebuilds the lookup table from all row keys.
// Call after bulk modifications or on static registries after definition.
void d_registry_rebuild_lookup(struct d_registry* _registry);

// d_registry_sort_lookup
//   function: sorts the lookup table for binary search. Called automatically
// by rebuild_lookup and after add operations.
void d_registry_sort_lookup(struct d_registry* _registry);


/******************************************************************************
 * ITERATOR FUNCTIONS
 *****************************************************************************/

struct d_registry_iterator d_registry_iterator_new(struct d_registry* _registry);
struct d_registry_iterator d_registry_iterator_filtered(struct d_registry* _registry, fn_registry_row_predicate _filter, const void* _context);

bool  d_registry_iterator_has_next(const struct d_registry_iterator* _iter);
void* d_registry_iterator_next(struct d_registry_iterator* _iter);
void  d_registry_iterator_reset(struct d_registry_iterator* _iter);

// callback-based iteration
void d_registry_foreach(struct d_registry* _registry, fn_registry_row_visitor _visitor, void* _context);
void d_registry_foreach_if(struct d_registry* _registry, fn_registry_row_predicate _predicate, const void* _pred_ctx, fn_registry_row_visitor _visitor, void* _visit_ctx);


/******************************************************************************
 * UTILITY FUNCTIONS
 *****************************************************************************/

bool   d_registry_reserve(struct d_registry* _registry, size_t _capacity);
bool   d_registry_reserve_lookup(struct d_registry* _registry, size_t _capacity);
bool   d_registry_shrink_to_fit(struct d_registry* _registry);
bool   d_registry_freeze(struct d_registry* _registry);
bool   d_registry_thaw(struct d_registry* _registry);

// get all keys (including aliases)
const char** d_registry_get_all_keys(const struct d_registry* _registry, size_t* _out_count);


/******************************************************************************
 * DESTRUCTOR FUNCTIONS
 *****************************************************************************/

void d_registry_free(struct d_registry* _registry);


/******************************************************************************
 * INTERNAL COMPARISON FUNCTION
 *****************************************************************************/
int d_registry_lookup_compare(const void* _a, const void* _b);
int d_registry_lookup_compare_nocase(const void* _a, const void* _b);


#endif  // DJINTERP_C_CONTAINER_REGISTRY_
