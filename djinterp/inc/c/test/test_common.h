/******************************************************************************
* djinterp [test]                                                test_common.h
*
*   Common types and declarations shared across the test framework.
*   Includes the d_test_object tree node, the d_test_arg key-value
*   configuration system, the d_test_counter tracking structure, note
*   structures, metadata, and standard argument key enumerations.
*
*
* path:      \inc\test\test_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.16
******************************************************************************/

#ifndef DJINTERP_TEST_COMMON_
#define DJINTERP_TEST_COMMON_ 1

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../djinterp.h"
#include "../container/map/min_enum_map.h"


// D_TEST_PASS
//   definition: evaluates in an evaluation, assertion, test, etc. passing
// and having the desired outcome.
#define D_TEST_PASS D_SUCCESS

// D_TEST_FAIL
//   definition: evaluates in an evaluation, assertion, test, etc. failing
// to have the desired outcome.
#define D_TEST_FAIL D_FAILURE

#define D_KEYWORD_TEST_ASSERTION   assertion
#define D_KEYWORD_TEST_TEST_FN     test_fn
#define D_KEYWORD_TEST_TEST        test
#define D_KEYWORD_TEST_BLOCK       block
#define D_KEYWORD_TEST_MODULE      module

// D_TEST_OBJECT_LEAF
//   constant: indicates a leaf node (assertion) in the test tree.
#define D_TEST_OBJECT_LEAF     true

// D_TEST_OBJECT_INTERIOR
//   constant: indicates an interior node (group/block) in the test tree.
#define D_TEST_OBJECT_INTERIOR false

// D_TEST_ARG_NO_LIMIT
//   constant: sentinel value for unlimited capacity on D_TEST_ARG_MAX_*
// keys. Cast to intptr_t for storage in void*.
#define D_TEST_ARG_NO_LIMIT ((intptr_t)-1)

// d_test_id
//   type: unique identifier for test objects.
typedef unsigned long long d_test_id;

// fn_test
//   function pointer: function pointer for a boolean test that returns
// D_TEST_PASS or D_TEST_FAIL.
typedef bool (*fn_test)();

// fn_stage
//   function pointer: function pointer for test lifecycle stage hooks.
// takes a test pointer and returns success/failure.
typedef bool (*fn_stage)(struct d_test*);


/******************************************************************************
 * ARG CONVENIENCE MACROS
 *****************************************************************************/

// D_TEST_ARG_TRUE / D_TEST_ARG_FALSE
//   macros: canonical boolean representations for void* arg storage.
#define D_TEST_ARG_TRUE  ((void*)(intptr_t)1)
#define D_TEST_ARG_FALSE ((void*)(intptr_t)0)

// D_TEST_ARG_AS_BOOL
//   macro: reads a void* arg value as a bool.
#define D_TEST_ARG_AS_BOOL(_val) ((bool)(intptr_t)(_val))

// D_TEST_ARG_AS_SIZE
//   macro: reads a void* arg value as a size_t.
#define D_TEST_ARG_AS_SIZE(_val) ((size_t)(intptr_t)(_val))

// D_TEST_ARG_FROM_SIZE
//   macro: stores a size_t value as a void* arg.
#define D_TEST_ARG_FROM_SIZE(_val) ((void*)(intptr_t)(_val))

// D_TEST_ARG_AS_INTPTR
//   macro: reads a void* arg value as intptr_t.
#define D_TEST_ARG_AS_INTPTR(_val) ((intptr_t)(_val))

// D_TEST_ARG_FROM_INTPTR
//   macro: stores an intptr_t value as a void* arg.
#define D_TEST_ARG_FROM_INTPTR(_val) ((void*)(_val))

// D_TEST_ARG_AS_PTR
//   macro: casts a void* arg value to a typed pointer.
#define D_TEST_ARG_AS_PTR(_type, _val) ((_type*)(_val))


/******************************************************************************
 * SYMBOL DEFINITIONS
 *****************************************************************************/

#if ( defined(D_EMOJIS) &&  \
      (D_EMOJIS == D_ENABLED) )
    #define D_EMOJI_CHECKMARK       "\xE2\x9C\x94"
    #define D_EMOJI_CROSS_MARK      "\xE2\x9D\x8C"
    #define D_EMOJI_PARTY_POPPER    "\xF0\x9F\x8E\x89"
    #define D_EMOJI_CLIPBOARD       "\xF0\x9F\x93\x8B"
    #define D_EMOJI_PAGE_FACING_UP  "\xF0\x9F\x93\x84"
    #define D_EMOJI_FILE_FOLDER     "\xF0\x9F\x93\x81"
    #define D_EMOJI_PACKAGE         "\xF0\x9F\x93\xA6"
    #define D_EMOJI_WARNING_SIGN    "\xE2\x9A\xA0"
    #define D_EMOJI_QUESTION_MARK   "\xE2\x9D\x93"

    // primary test symbols
    #define D_TEST_SYMBOL_PASS      D_EMOJI_CHECKMARK
    #define D_TEST_SYMBOL_FAIL      D_EMOJI_CROSS_MARK
    #define D_TEST_SYMBOL_SUCCESS   D_EMOJI_PARTY_POPPER
    #define D_TEST_SYMBOL_INFO      D_EMOJI_CLIPBOARD

    // tree structure symbols (emoji versions)
    #define D_TEST_SYMBOL_LEAF      D_EMOJI_PAGE_FACING_UP
    #define D_TEST_SYMBOL_INTERIOR  D_EMOJI_FILE_FOLDER
    #define D_TEST_SYMBOL_MODULE    D_EMOJI_PACKAGE
    #define D_TEST_SYMBOL_WARNING   D_EMOJI_WARNING_SIGN
    #define D_TEST_SYMBOL_UNKNOWN   D_EMOJI_QUESTION_MARK

#else
    // fallback to ASCII-only symbols
    #define D_TEST_SYMBOL_PASS      "[PASS]"
    #define D_TEST_SYMBOL_FAIL      "[FAIL]"
    #define D_TEST_SYMBOL_SUCCESS   "[SUCCESS]"
    #define D_TEST_SYMBOL_INFO      "[INFO]"

    // ASCII tree structure symbols
    #define D_TEST_SYMBOL_LEAF      "[LEAF]"
    #define D_TEST_SYMBOL_INTERIOR  "[GROUP]"
    #define D_TEST_SYMBOL_MODULE    "[MODULE]"
    #define D_TEST_SYMBOL_WARNING   "[WARNING]"
    #define D_TEST_SYMBOL_UNKNOWN   "[UNKNOWN]"

#endif  // D_IS_ENABLED(D_EMOJIS)


/******************************************************************************
 * ENUMERATIONS
 *****************************************************************************/

// DTestStage
//   enum: lifecycle stages for test execution hooks.
enum DTestStage
{
    D_TEST_STAGE_SETUP      = 0,
    D_TEST_STAGE_TEAR_DOWN  = 1,
    D_TEST_STAGE_ON_SUCCESS = 2,
    D_TEST_STAGE_ON_FAILURE = 3,
    D_TEST_STAGE_BEFORE     = 4,
    D_TEST_STAGE_AFTER      = 5
};

// DTestTypeFlag
//   enum: discriminator for d_test_type union, identifying the type of
// test tree node.
enum DTestTypeFlag
{
    D_TEST_TYPE_UNKNOWN    = 0,
    D_TEST_TYPE_ASSERT     = 1,
    D_TEST_TYPE_TEST_FN    = 2,
    D_TEST_TYPE_TEST       = 3,
    D_TEST_TYPE_TEST_BLOCK = 4,
    D_TEST_TYPE_MODULE     = 5
};

// DTestArgKey
//   enum: standard argument key identifiers for the d_test_arg key-value
// system. These are functional arguments that affect behavior.
// String-only labels belong in DTestMetadataKey instead.
//
// Values stored as void*; use D_TEST_ARG_AS_BOOL, D_TEST_ARG_AS_SIZE,
// D_TEST_ARG_AS_PTR, etc. to read typed values. Pointer-typed args
// are non-owning unless marked "(owned)" — owned pointers are freed
// by the cleanup function of the struct that holds the arg list.
enum DTestArgKey
{
    D_TEST_ARG_NONE                    = 0,

    // --- general keys (1 - 10) ---
    D_TEST_ARG_FAILURES                = 1,   // d_test_standalone_failure_list*
    D_TEST_ARG_ASSERTION_NUMBER        = 2,   // size_t via intptr_t
    D_TEST_ARG_TEST_NUMBER             = 3,   // size_t via intptr_t
    D_TEST_ARG_OUTPUT_FILE             = 4,   // const char* (filepath)
    D_TEST_ARG_CONTEXT                 = 5,   // void* (opaque user data)
    D_TEST_ARG_METADATA                = 6,   // d_test_metadata*
    D_TEST_ARG_MAX_MODULES             = 7,   // intptr_t (D_TEST_ARG_NO_LIMIT)
    D_TEST_ARG_MAX_FAILURES            = 8,   // intptr_t (D_TEST_ARG_NO_LIMIT)

    // --- counter keys (11 - 12) ---
    D_TEST_ARG_ASSERTION_COUNTER       = 11,  // d_test_counter*
    D_TEST_ARG_TEST_COUNTER            = 12,  // d_test_counter*

    // --- option keys (21 - 30) ---
    //   stored as D_TEST_ARG_TRUE / D_TEST_ARG_FALSE.
    D_TEST_ARG_OPT_NUMBER_ASSERTIONS   = 21,  // bool via intptr_t
    D_TEST_ARG_OPT_NUMBER_TESTS        = 22,  // bool via intptr_t
    D_TEST_ARG_OPT_GLOBAL_NUMBERING    = 23,  // bool via intptr_t
    D_TEST_ARG_OPT_SHOW_INFO           = 24,  // bool via intptr_t
    D_TEST_ARG_OPT_SHOW_MODULE_FOOTER  = 25,  // bool via intptr_t
    D_TEST_ARG_OPT_LIST_FAILURES       = 26,  // bool via intptr_t
    D_TEST_ARG_OPT_WAIT_FOR_INPUT      = 27,  // bool via intptr_t
    D_TEST_ARG_OPT_SHOW_NOTES          = 28,  // bool via intptr_t

    // --- result keys (31 - 40) ---
    D_TEST_ARG_ELAPSED_TIME            = 31,  // double* (owned)
    D_TEST_ARG_PASSED                  = 32,  // bool via intptr_t
    D_TEST_ARG_MODULES_TOTAL           = 33,  // size_t via intptr_t
    D_TEST_ARG_MODULES_PASSED          = 34,  // size_t via intptr_t
    D_TEST_ARG_MODULE_RESULTS          = 35,  // d_test_arg_list** (array, owned)
    D_TEST_ARG_MODULE_RESULT_COUNT     = 36,  // size_t via intptr_t

    // --- note keys (41 - 42) ---
    D_TEST_ARG_NOTES                   = 41,  // const d_test_note_section*
    D_TEST_ARG_NOTE_COUNT              = 42,  // size_t via intptr_t

    // --- tree/nesting keys (51 - 56) ---
    D_TEST_ARG_CHILDREN                = 51,  // d_test_object**
    D_TEST_ARG_CHILD_COUNT             = 52,  // size_t via intptr_t
    D_TEST_ARG_SUB_RUNNER              = 53,  // d_test_standalone_runner*
    D_TEST_ARG_PARENT_RUNNER           = 54,  // d_test_standalone_runner*
    D_TEST_ARG_DEPTH                   = 55,  // size_t via intptr_t
    D_TEST_ARG_RUNNER_ID               = 56,  // size_t via intptr_t

    // --- module registration keys (61 - 63) ---
    //   replaces inline module array fields on the runner.
    //   module_entry array is owned; freed during cleanup.
    D_TEST_ARG_MODULES                 = 61,  // d_test_standalone_module_entry*
    D_TEST_ARG_MODULE_COUNT            = 62,  // size_t via intptr_t
    D_TEST_ARG_MODULE_CAPACITY         = 63,  // size_t via intptr_t

    // --- result backing storage keys (71 - 73) ---
    //   per-module counter/time arrays allocated during execute.
    //   owned by the runner; freed during cleanup.
    D_TEST_ARG_RESULT_ASSERTION_CTRS   = 71,  // d_test_counter* (array, owned)
    D_TEST_ARG_RESULT_TEST_CTRS        = 72,  // d_test_counter* (array, owned)
    D_TEST_ARG_RESULT_ELAPSED_TIMES    = 73,  // double* (array, owned)

    // --- user keys ---
    D_TEST_ARG_USER                    = 256
};

// DTestMetadataKey
//   enum: key identifiers for string-only metadata arguments. These do
// not affect functionality; they are labels/descriptions for display
// and reporting purposes. Arrays of d_test_metadata_arg keyed by these
// values must be sorted in ascending key order.
enum DTestMetadataKey
{
    D_TEST_META_NONE           = 0,
    D_TEST_META_NAME           = 1,
    D_TEST_META_MESSAGE        = 2,
    D_TEST_META_DESCRIPTION    = 3,
    D_TEST_META_CURRENT_MODULE = 4,
    D_TEST_META_SUITE_NAME     = 5,
    D_TEST_META_AUTHOR         = 6,
    D_TEST_META_VERSION        = 7,
    D_TEST_META_USER           = 256
};


/******************************************************************************
 * COUNTER FIELD BITFIELD
 *****************************************************************************/

// DTestCountField
//   enum: bitfield flags selecting which counter fields are active.
// Each flag corresponds to one slot in the counter's compact amount
// array. The slot index is not fixed; it is the popcount of all
// lower-order active flags in the counter's field mask.
enum DTestCountField
{
    D_TEST_COUNT_TOTAL     = (1 << 0),
    D_TEST_COUNT_PASSED    = (1 << 1),
    D_TEST_COUNT_FAILED    = (1 << 2),
    D_TEST_COUNT_REMAINING = (1 << 3)
};

// D_TEST_COUNT_FIELD_MAX
//   constant: maximum number of distinct field flags. This is the
// width of the field nibble, not the size of any particular counter's
// amount array (which is popcount(fields)).
#define D_TEST_COUNT_FIELD_MAX 4

#define D_TEST_COUNT_ALL                                                    \
    ( D_TEST_COUNT_TOTAL     |                                              \
      D_TEST_COUNT_PASSED    |                                              \
      D_TEST_COUNT_FAILED    |                                              \
      D_TEST_COUNT_REMAINING )

#define D_TEST_COUNT_STANDARD                                               \
    ( D_TEST_COUNT_TOTAL  |                                                 \
      D_TEST_COUNT_PASSED |                                                 \
      D_TEST_COUNT_FAILED )


/******************************************************************************
 * COUNTER SPEC ENCODING
 *****************************************************************************/

// A counter spec is a uint16_t that fully encodes what a d_test_counter
// requires in binary:
//
//   bits [0..7]   test_type   (DTestTypeFlag)
//   bits [8..11]  fields      (DTestCountField bitmask)
//   bits [12..15] reserved    (zero)
//
// From the spec alone, the init function can determine the test type,
// which fields are active, and how many amount slots to allocate
// (popcount of the field nibble).
//
//   layout diagram (MSB → LSB):
//
//     15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
//    ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
//    │ 0   0   0   0 │ R │ F │ P │ T │       test_type (0-255)       │
//    └───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘
//                      │   │   │   │
//                      │   │   │   └── D_TEST_COUNT_TOTAL
//                      │   │   └────── D_TEST_COUNT_PASSED
//                      │   └────────── D_TEST_COUNT_FAILED
//                      └────────────── D_TEST_COUNT_REMAINING
//
// The compact amount array has exactly popcount(fields) elements.
// A field flag's index into the array is popcount(fields & (flag-1)):
//
//   example: fields = T|P|F  (0b0111), 3 slots
//     TOTAL     → popcount(0b0111 & 0b0000) = 0
//     PASSED    → popcount(0b0111 & 0b0001) = 1
//     FAILED    → popcount(0b0111 & 0b0011) = 2
//     REMAINING → not active, access returns 0

// D_TEST_COUNTER_SPEC_TYPE_BITS
//   constant: bit width of the test_type field in a counter spec.
#define D_TEST_COUNTER_SPEC_TYPE_BITS   8

// D_TEST_COUNTER_SPEC_TYPE_MASK
//   constant: bitmask isolating the test_type field.
#define D_TEST_COUNTER_SPEC_TYPE_MASK   ((uint16_t)0x00FF)

// D_TEST_COUNTER_SPEC_FIELD_SHIFT
//   constant: bit offset of the fields nibble in a counter spec.
#define D_TEST_COUNTER_SPEC_FIELD_SHIFT 8

// D_TEST_COUNTER_SPEC_FIELD_MASK
//   constant: bitmask isolating the fields nibble (post-shift).
#define D_TEST_COUNTER_SPEC_FIELD_MASK  ((uint16_t)0x000F)

// D_TEST_COUNTER_SPEC
//   macro: encodes a test type and field bitmask into a single
// uint16_t counter spec.
#define D_TEST_COUNTER_SPEC(_type, _fields)                                 \
    ((uint16_t)(                                                            \
        ( ((uint16_t)(_type))   & D_TEST_COUNTER_SPEC_TYPE_MASK )  |        \
        ( (((uint16_t)(_fields)) & D_TEST_COUNTER_SPEC_FIELD_MASK)          \
              << D_TEST_COUNTER_SPEC_FIELD_SHIFT )                          \
    ))

// D_TEST_COUNTER_SPEC_TYPE
//   macro: extracts the test_type from a counter spec.
#define D_TEST_COUNTER_SPEC_TYPE(_spec)                                     \
    ((uint16_t)((_spec) & D_TEST_COUNTER_SPEC_TYPE_MASK))

// D_TEST_COUNTER_SPEC_FIELDS
//   macro: extracts the field bitmask from a counter spec.
#define D_TEST_COUNTER_SPEC_FIELDS(_spec)                                   \
    ((uint16_t)(((_spec) >> D_TEST_COUNTER_SPEC_FIELD_SHIFT)                \
                & D_TEST_COUNTER_SPEC_FIELD_MASK))


/******************************************************************************
 * COUNTER SPEC FIELD INDEXING
 *****************************************************************************/

// d_test_popcount4
//   function (inline): population count of the lower 4 bits.
// Returns the number of active field slots in a counter.
static inline uint16_t
d_test_popcount4
(
    uint16_t _mask
)
{
    // isolate lower nibble and sum adjacent bit pairs
    _mask = _mask & 0x0Fu;
    _mask = (_mask & 0x05u) + ((_mask >> 1) & 0x05u);
    _mask = (_mask & 0x03u) + ((_mask >> 2) & 0x03u);

    return _mask;
}

// D_TEST_COUNTER_SPEC_COUNT
//   macro: number of amount slots required by a counter spec.
// This is the popcount of the spec's field nibble.
#define D_TEST_COUNTER_SPEC_COUNT(_spec)                                    \
    d_test_popcount4(D_TEST_COUNTER_SPEC_FIELDS(_spec))

// D_TEST_COUNTER_FIELD_COUNT
//   macro: number of active fields in a raw field bitmask.
#define D_TEST_COUNTER_FIELD_COUNT(_fields)                                 \
    d_test_popcount4((uint16_t)(_fields))

// D_TEST_COUNTER_FIELD_INDEX
//   macro: compact array index of a single field flag within an
// active-field bitmask. The index is the number of set bits in
// _fields that are lower than _flag.
//
//   _fields: the counter's active field bitmask
//   _flag:   a single DTestCountField flag (must be a power of 2)
//
// Precondition: (_fields & _flag) != 0. If the flag is not active,
// the index is meaningless.
#define D_TEST_COUNTER_FIELD_INDEX(_fields, _flag)                          \
    d_test_popcount4((uint16_t)((_fields) & ((_flag) - 1)))

// D_TEST_COUNTER_FIELD_ACTIVE
//   macro: evaluates true if _flag is active in _fields.
#define D_TEST_COUNTER_FIELD_ACTIVE(_fields, _flag)                         \
    (((_fields) & (_flag)) != 0)


/******************************************************************************
 * PREDEFINED COUNTER SPECS
 *****************************************************************************/

// Standard presets: test_type | TOTAL + PASSED + FAILED (3 slots)
#define D_TEST_COUNTER_ASSERT_STD                                           \
    D_TEST_COUNTER_SPEC(D_TEST_TYPE_ASSERT, D_TEST_COUNT_STANDARD)

#define D_TEST_COUNTER_TEST_STD                                             \
    D_TEST_COUNTER_SPEC(D_TEST_TYPE_TEST, D_TEST_COUNT_STANDARD)

#define D_TEST_COUNTER_TEST_FN_STD                                          \
    D_TEST_COUNTER_SPEC(D_TEST_TYPE_TEST_FN, D_TEST_COUNT_STANDARD)

#define D_TEST_COUNTER_BLOCK_STD                                            \
    D_TEST_COUNTER_SPEC(D_TEST_TYPE_TEST_BLOCK, D_TEST_COUNT_STANDARD)

#define D_TEST_COUNTER_MODULE_STD                                           \
    D_TEST_COUNTER_SPEC(D_TEST_TYPE_MODULE, D_TEST_COUNT_STANDARD)

// Full presets: test_type | all 4 fields (4 slots)
#define D_TEST_COUNTER_ASSERT_ALL                                           \
    D_TEST_COUNTER_SPEC(D_TEST_TYPE_ASSERT, D_TEST_COUNT_ALL)

#define D_TEST_COUNTER_TEST_ALL                                             \
    D_TEST_COUNTER_SPEC(D_TEST_TYPE_TEST, D_TEST_COUNT_ALL)

// Minimal presets: test_type | TOTAL only (1 slot)
#define D_TEST_COUNTER_ASSERT_TOTAL                                         \
    D_TEST_COUNTER_SPEC(D_TEST_TYPE_ASSERT, D_TEST_COUNT_TOTAL)

#define D_TEST_COUNTER_TEST_TOTAL                                           \
    D_TEST_COUNTER_SPEC(D_TEST_TYPE_TEST, D_TEST_COUNT_TOTAL)


/******************************************************************************
 * METADATA STRUCTURES AND MACROS
 *****************************************************************************/

// d_test_metadata_arg
//   struct: a single metadata entry mapping a DTestMetadataKey to a
// string value. Arrays of these must be sorted by key in ascending order
// for binary search lookup.
struct d_test_metadata_arg
{
    uint16_t    key;
    const char* value;
};

// d_test_metadata
//   struct: a sorted array of metadata entries. Referenced by
// D_TEST_ARG_METADATA in a d_test_arg_list.
struct d_test_metadata
{
    const struct d_test_metadata_arg* entries;
    size_t                            count;
};

#define D_TEST_META_ENTRY(_key, _value)                                     \
    { (_key), (_value) }

#define D_TEST_META_ARRAY(_name, ...)                                       \
    struct d_test_metadata_arg _name[] = { __VA_ARGS__ }

#define D_TEST_META_COUNT(_arr)                                             \
    (sizeof(_arr) / sizeof((_arr)[0]))

#define D_TEST_META_INIT(_arr)                                              \
    { (_arr), D_TEST_META_COUNT(_arr) }

#define D_TEST_META_DECLARE(_meta_name,                                     \
                            _arr_name, ...)                                 \
    D_TEST_META_ARRAY(_arr_name, __VA_ARGS__);                              \
    struct d_test_metadata _meta_name =                                     \
        D_TEST_META_INIT(_arr_name)


/******************************************************************************
 * CONFIGURATION ARGUMENT STRUCTURES
 *****************************************************************************/

// d_test_arg
//   struct: key-value pair for module/block/test configuration arguments.
struct d_test_arg
{
    int16_t key;
    void*   value;
};

// d_test_arg_list
//   struct: collection of test arguments.
struct d_test_arg_list
{
    struct d_test_arg* args;
    size_t             count;
    size_t             capacity;
};

// d_test_fn
//   struct: wrapper for a test function with optional arguments.
struct d_test_fn
{
    fn_test test_fn;
    void*   context;
};


/******************************************************************************
 * NOTE STRUCTURES
 *****************************************************************************/

// d_test_note_item
//   struct: represents a single note item for implementation notes.
struct d_test_note_item
{
    uint16_t    key;
    const char* message;
};

// d_test_note_section
//   struct: represents a section of notes (e.g., "CURRENT STATUS").
struct d_test_note_section
{
    const char*                    title;
    size_t                         count;
    const struct d_test_note_item* items;
};


/******************************************************************************
 * COUNTER STRUCTURE
 *****************************************************************************/

// d_test_counter
//   struct: configurable counter for tracking test/assertion progress.
// The amount array is heap-allocated with exactly popcount(fields)
// elements. Each active field flag maps to a compact index via
// D_TEST_COUNTER_FIELD_INDEX(fields, flag).
struct d_test_counter
{
    uint16_t test_type;
    uint16_t fields;
    size_t*  amount;
};


/******************************************************************************
 * TEST OBJECT
 *****************************************************************************/

// d_test_object
//   struct: tree node representing either an assertion (leaf) or a test
// group/block (interior). supports nested test organization.
// The context pointer is opaque and its interpretation is defined by
// the consuming module (e.g. test_standalone, dtest).
struct d_test_object
{
    bool                    is_leaf;
    bool                    result;
    void*                   context;
    struct d_test_arg_list* args;
};


/******************************************************************************
 * TEST OBJECT FUNCTIONS
 *****************************************************************************/

struct d_test_object* d_test_object_new(bool _is_leaf, bool _result, void* _context);
void                  d_test_object_free(struct d_test_object* _obj);
void                  d_test_object_set_args(struct d_test_object* _obj, struct d_test_arg_list* _args);


/******************************************************************************
 * TEST COUNTER FUNCTIONS
 *****************************************************************************/

int    d_test_counter_init(struct d_test_counter* _counter, uint16_t _spec);
void   d_test_counter_free(struct d_test_counter* _counter);
void   d_test_counter_reset(struct d_test_counter* _counter);
void   d_test_counter_increment(struct d_test_counter* _counter, uint16_t _field);
size_t d_test_counter_get(const struct d_test_counter* _counter, uint16_t _field);
void   d_test_counter_add(struct d_test_counter* _dest, const struct d_test_counter* _source);


/******************************************************************************
 * ARG LIST UTILITY FUNCTIONS
 *****************************************************************************/

struct d_test_arg_list* d_test_arg_list_new(size_t _capacity);
void                    d_test_arg_list_free(struct d_test_arg_list* _list);
int                     d_test_arg_list_set(struct d_test_arg_list* _list, int16_t _key, void* _value);
void*                   d_test_arg_list_get(const struct d_test_arg_list* _list, int16_t _key);


/******************************************************************************
 * ARG LIST TYPED CONVENIENCE FUNCTIONS
 *****************************************************************************/

// typed getters with defaults (avoid casting at every call site)
bool        d_test_arg_list_get_bool(const struct d_test_arg_list* _list, int16_t _key, bool _default);
size_t      d_test_arg_list_get_size(const struct d_test_arg_list* _list, int16_t _key, size_t _default);
intptr_t    d_test_arg_list_get_intptr(const struct d_test_arg_list* _list, int16_t _key, intptr_t _default);
const char* d_test_arg_list_get_str(const struct d_test_arg_list* _list, int16_t _key, const char* _default);


/******************************************************************************
 * METADATA UTILITY FUNCTIONS
 *****************************************************************************/

const char* d_test_metadata_get(const struct d_test_metadata* _meta, uint16_t _key);

// convenience: get metadata string from an arg_list's D_TEST_ARG_METADATA
const char* d_test_arg_list_get_meta(const struct d_test_arg_list* _list, uint16_t _meta_key, const char* _default);


#endif  // DJINTERP_TEST_COMMON_
