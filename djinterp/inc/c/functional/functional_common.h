/******************************************************************************
* djinterp [functional]                                    functional_common.h
*
* Common types, macros, and utilities for the functional programming module.
*   Provides function pointer type definitions (predicates, transformers,
* consumers, producers, comparators, accumulators, reducers, generic
* callbacks), inline/composition macro helpers, type-generic utilities
* (C11+), user-defined typed function wrappers, and commonly used utility
* functions (identity, constant, comparison, null-checking).
*
* NAMING CONVENTIONS:
*   fn_predicate         - function returning bool, taking one argument
*   fn_transformer       - function transforming input to output
*   fn_consumer          - function consuming value, returning void
*   fn_producer          - function producing value, taking no input
*   fn_comparator        - function comparing two values
*   fn_accumulator       - function combining accumulated value with element
*
* path:      \inc\functional\functional_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.09
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_COMMON_
#define DJINTERP_FUNCTIONAL_COMMON_ 1

#include <stddef.h>
#include <string.h>
#include "..\djinterp.h"


///////////////////////////////////////////////////////////////////////////////
///             I.    PREDICATES AND COMPARATORS                            ///
///////////////////////////////////////////////////////////////////////////////

// fn_predicate
//   function pointer: function returning bool for a given element.
// Used for filtering, testing, and boolean operations.
// Note: `_context` may be NULL.
typedef bool (*fn_predicate)(const void* _element,
                             void*       _context);

// fn_binary_predicate
//   function pointer: function returning bool for two elements.
// Used for comparisons, equality checks, and binary tests.
// Note: `_context` may be NULL.
typedef bool (*fn_binary_predicate)(const void* _element1,
                                    const void* _element2,
                                    void*       _context);

// fn_function_comparator
//   function pointer: three-way comparison returning negative/zero/positive.
// Returns <0 if _element1 < _element2, 0 if equal, >0 if greater.
// Note: `_context` may be NULL.
typedef int (*fn_function_comparator)(const void* _element1,
                                      const void* _element2,
                                      void*       _context);


///////////////////////////////////////////////////////////////////////////////
///             II.   TRANSFORMERS AND MAPPERS                              ///
///////////////////////////////////////////////////////////////////////////////

// fn_transformer
//   function pointer: function transforming input to output.
// Output is written to _output parameter. Returns success status.
// Note: `_context` may be NULL.
typedef bool (*fn_transformer)(const void* _input,
                               void*       _output,
                               void*       _context);

// d_mapper
//   type alias: alias for fn_transformer (common terminology).
typedef fn_transformer d_mapper;


///////////////////////////////////////////////////////////////////////////////
///             III.  CONSUMERS AND PRODUCERS                               ///
///////////////////////////////////////////////////////////////////////////////

// fn_consumer
//   function pointer: function consuming a value without producing output.
// Used for side-effects like printing, logging, or updating state.
// Note: `_context` may be NULL.
typedef void (*fn_consumer)(void* _element,
                            void* _context);

// fn_consumer_const
//   function pointer: consumer that doesn't modify the element.
// Note: `_context` may be NULL.
typedef void (*fn_consumer_const)(const void* _element,
                                  void*       _context);

// fn_producer
//   function pointer: function producing a value with no input.
// Output is written to _output parameter. Returns success status.
// Note: `_context` may be NULL.
typedef bool (*fn_producer)(void* _output,
                            void* _context);


///////////////////////////////////////////////////////////////////////////////
///             IV.   ACCUMULATORS AND REDUCERS                             ///
///////////////////////////////////////////////////////////////////////////////

// fn_accumulator
//   function pointer: function combining accumulated value with new element.
// Used in fold/reduce operations. Output is written to _accumulated.
// Returns success status. _context may be NULL.
typedef bool (*fn_accumulator)(void*       _accumulated,
                               const void* _element,
                               void*       _context);

// fn_reducer
//   function pointer: function combining two elements into one.
// Used for reduction operations. Output is written to _result.
// _context may be NULL.
typedef bool (*fn_reducer)(const void* _element1,
                           const void* _element2,
                           void*       _result,
                           void*       _context);


///////////////////////////////////////////////////////////////////////////////
///             V.    GENERIC CALLBACKS AND OPERATIONS                      ///
///////////////////////////////////////////////////////////////////////////////

// fn_unary_operation
//   function pointer: operation taking one input and producing one output.
// _context may be NULL.
typedef bool (*fn_unary_operation)(const void* _input,
                                   void*       _output,
                                   void*       _context);

// fn_binary_operation
//   function pointer: operation taking two inputs and producing one output.
// _context may be NULL.
typedef bool (*fn_binary_operation)(const void* _input1,
                                    const void* _input2,
                                    void*       _output,
                                    void*       _context);

// fn_hasher
//   function pointer: function computing hash value for an element.
// _context may be NULL.
typedef size_t (*fn_hasher)(const void* _element,
                            void*       _context);

// fn_cloner
//   function pointer: function creating a deep copy of an element.
// Output is written to _destination. Returns success status.
// _context may be NULL.
typedef bool (*fn_cloner)(const void* _source,
                          void*       _destination,
                          void*       _context);


///////////////////////////////////////////////////////////////////////////////
///             VI.   INLINE AND COMPOSITION MACROS                         ///
///////////////////////////////////////////////////////////////////////////////

// D_FUNCTIONAL_LAMBDA
//   macro: creates a simple inline function-like expression.
// Note: this is a limited "lambda" that works in C89+.
#define D_FUNCTIONAL_LAMBDA(return_type,                                    \
                            param_type,                                     \
                            param_name,                                     \
                            body)                                           \
    D_INLINE return_type                                                    \
    D_CONCAT(d_lambda_, __LINE__)(param_type param_name)                    \
    body

// D_FUNCTIONAL_PREDICATE
//   macro: creates a simple predicate function.
// The generated function accepts and ignores a void* _context parameter.
#define D_FUNCTIONAL_PREDICATE(name,                                        \
                               param,                                       \
                               condition)                                   \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* param,                                                  \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
                                                                            \
        return (condition);                                                 \
    }

// D_FUNCTIONAL_TRANSFORMER
//   macro: creates a simple transformer function.
// The generated function accepts and ignores a void* _context parameter.
#define D_FUNCTIONAL_TRANSFORMER(name,                                      \
                                 input,                                     \
                                 output,                                    \
                                 body)                                      \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* input,                                                  \
        void*       output,                                                 \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
        body                                                                \
                                                                            \
        return true;                                                        \
    }

// D_FUNCTIONAL_CONSUMER
//   macro: creates a simple consumer function.
// The generated function accepts and ignores a void* _context parameter.
#define D_FUNCTIONAL_CONSUMER(name,                                         \
                              param,                                        \
                              body)                                         \
    D_INLINE void                                                           \
    name                                                                    \
    (   void* param,                                                        \
        void* _context                                                      \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
                                                                            \
        body                                                                \
    }

// D_FUNCTIONAL_COMPOSE
//   macro: composes two function calls.
#define D_FUNCTIONAL_COMPOSE(F, G, X) F(G(X))


///////////////////////////////////////////////////////////////////////////////
///             VII.  TYPE-GENERIC HELPERS (C11+)                           ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_LANG_IS_C11_OR_HIGHER

// D_FUNCTIONAL_CALL
//   macro: type-generic function call wrapper.
// Automatically handles casting for common types. Passes NULL context.
#define D_FUNCTIONAL_CALL(FN, ARG)                                          \
    _Generic((ARG),                                                         \
        int*:          FN((const void*)(ARG), NULL),                        \
        double*:       FN((const void*)(ARG), NULL),                        \
        char*:         FN((const void*)(ARG), NULL),                        \
        void*:         FN((const void*)(ARG), NULL),                        \
        const int*:    FN((const void*)(ARG), NULL),                        \
        const double*: FN((const void*)(ARG), NULL),                        \
        const char*:   FN((const void*)(ARG), NULL),                        \
        const void*:   FN((ARG), NULL),                                     \
        default:       FN((const void*)(ARG), NULL))

#endif  // D_ENV_LANG_IS_C11_OR_HIGHER


///////////////////////////////////////////////////////////////////////////////
///             VIII. USER-DEFINED FUNCTION WRAPPERS                        ///
///////////////////////////////////////////////////////////////////////////////

// D_FUNCTIONAL_DEFINE_PREDICATE
//   macro: defines a typed predicate wrapper for user types.
// Creates both a typed version and a void* wrapper for composition.
// The wrapper forwards _context to the typed version.
#define D_FUNCTIONAL_DEFINE_PREDICATE(name,                                 \
                                      type)                                 \
    bool name##_typed(const type* _element,                                 \
                      void*       _context);                                \
                                                                            \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        return name##_typed((const type*)_element,                          \
                            _context);                                      \
    }                                                                       \
                                                                            \
    bool name##_typed(const type * _element,                                \
                      void*       _context)

// D_FUNCTIONAL_DEFINE_TRANSFORMER
//   macro: defines a typed transformer wrapper for user types.
#define D_FUNCTIONAL_DEFINE_TRANSFORMER(name,                               \
                                        input_type,                         \
                                        output_type)                        \
    bool name##_typed(const input_type* _input,                             \
                      output_type*      _output,                            \
                      void*             _context);                          \
                                                                            \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        return name##_typed((const input_type*)_input,                      \
                            (output_type*)_output,                          \
                            _context);                                      \
    }                                                                       \
                                                                            \
    bool name##_typed(const input_type* _input,                             \
                      output_type*      _output,                            \
                      void*             _context)

// D_FUNCTIONAL_DEFINE_CONSUMER
//   macro: defines a typed consumer wrapper for user types.
#define D_FUNCTIONAL_DEFINE_CONSUMER(name,                                  \
                                     type)                                  \
    void name##_typed(type* _element,                                       \
                      void* _context);                                      \
                                                                            \
    D_INLINE void                                                           \
    name                                                                    \
    (                                                                       \
        void* _element,                                                     \
        void* _context                                                      \
    )                                                                       \
    {                                                                       \
        name##_typed((type*)_element, _context);                            \
    }                                                                       \
                                                                            \
    void name##_typed(type* _element,                                       \
                      void* _context)

// D_FUNCTIONAL_DEFINE_COMPARATOR
//   macro: defines a typed comparator wrapper for user types.
#define D_FUNCTIONAL_DEFINE_COMPARATOR(name,                                \
                                       type)                                \
    int name##_typed(const type* _a,                                        \
                     const type* _b,                                        \
                     void*       _context);                                 \
                                                                            \
    D_INLINE int                                                            \
    name                                                                    \
    (                                                                       \
        const void* _a,                                                     \
        const void* _b,                                                     \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        return name##_typed((const type*)_a,                                \
                            (const type*)_b,                                \
                            _context);                                      \
    }                                                                       \
                                                                            \
    int name##_typed(const type* _a,                                        \
                     const type* _b,                                        \
                     void*       _context)

// D_FUNCTIONAL_DEFINE_ACCUMULATOR
//   macro: defines a typed accumulator wrapper for user types.
#define D_FUNCTIONAL_DEFINE_ACCUMULATOR(name,                               \
                                        accumulator_type,                   \
                                        element_type)                       \
    bool name##_typed(accumulator_type*   _accumulated,                     \
                      const element_type* _element,                         \
                      void*               _context);                        \
                                                                            \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        void*       _accumulated,                                           \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        return name##_typed((accumulator_type*)_accumulated,                \
                            (const element_type*)_element,                  \
                            _context);                                      \
    }                                                                       \
                                                                            \
    bool name##_typed(accumulator_type*   _accumulated,                     \
                      const element_type* _element,                         \
                      void*               _context)


///////////////////////////////////////////////////////////////////////////////
///             IX.   UTILITY FUNCTIONS                                     ///
///////////////////////////////////////////////////////////////////////////////

// i.    identity functions
bool     d_functional_identity_transformer(const void* _input, void* _output, void* _context);
bool     d_functional_identity_predicate(const void* _element, void* _context);

// ii.   constant functions
bool     d_functional_constant_true(const void* _element, void* _context);
bool     d_functional_constant_false(const void* _element, void* _context);

// iii.  comparison utilities
int      d_functional_compare_int(const void* _a, const void* _b, void* _context);
int      d_functional_compare_size_t(const void* _a, const void* _b, void* _context);
int      d_functional_compare_double(const void* _a, const void* _b, void* _context);
bool     d_functional_equal_int(const void* _a, const void* _b, void* _context);
bool     d_functional_equal_size_t(const void* _a, const void* _b, void* _context);

// iv.   predicate utilities
bool     d_functional_is_null(const void* _element, void* _context);
bool     d_functional_is_not_null(const void* _element, void* _context);

///////////////////////////////////////////////////////////////////////////////
///             X.    HIGHER-ORDER FUNCTIONS                                ///
///////////////////////////////////////////////////////////////////////////////

// i.    map
bool     d_functional_map(const void* _input, void* _output, size_t _count, size_t _element_size, fn_transformer _transform, void* _context);
         
// ii.     fold
bool     d_functional_fold_left(const void* _input, size_t _count, size_t _element_size, void* _accumulator, fn_accumulator _combine, void* _context);
bool     d_functional_fold_right(const void* _input, size_t _count, size_t _element_size, void* _accumulator, fn_accumulator _combine, void* _context);
         
// iii.    iteration
void     d_functional_for_each(void* _input, size_t _count, size_t _element_size, fn_consumer _apply, void* _context);
void     d_functional_for_each_const(const void* _input, size_t _count, size_t _element_size, fn_consumer_const _apply, void* _context);
         
// iv.     quantifiers
bool     d_functional_any(const void* _input, size_t _count, size_t _element_size, fn_predicate _test, void* _context);
bool     d_functional_all(const void* _input, size_t _count, size_t _element_size, fn_predicate _test, void* _context);
bool     d_functional_none(const void* _input, size_t _count, size_t _element_size, fn_predicate _test, void* _context);
size_t   d_functional_count_if(const void* _input, size_t _count, size_t _element_size, fn_predicate _test, void* _context);
void*    d_functional_find_if(const void* _input, size_t _count, size_t _element_size, fn_predicate _test, void* _context);


#endif  // DJINTERP_FUNCTIONAL_COMMON_
