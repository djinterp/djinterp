/******************************************************************************
* djinterp [functional]                                           compose.h
*
* Function composition and partial application for the functional module.
*   Provides transformer composition, partial application helpers, convenience
* composition macros, and composable function template utilities.
*
* 
* path:      \inc\functional\compose.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.09
******************************************************************************/

#ifndef DJINTERP_C_FUNCTIONAL_COMPOSE_
#define DJINTERP_C_FUNCTIONAL_COMPOSE_ 1

#include <stdlib.h>
#include "..\djinterp.h"
#include ".\functional_common.h"

// convenience composition marcos

// D_FUNCTIONAL_THEN
//   macro: chains two operations sequentially (left-to-right composition).
// Usage: 
//   result = D_FUNCTIONAL_THEN(D_FUNCTIONAL_THEN(func1, func2), f3)(input);
#define D_FUNCTIONAL_THEN(func1, func2)                                     \
    d_functional_compose_##func1##_##func2                                  
                                                                            
// D_FUNCTIONAL_AND_THEN                                                    
//   macro: alias for D_FUNCTIONAL_THEN for readability.                    
#define D_FUNCTIONAL_AND_THEN(func1, func2)                                 \
    D_FUNCTIONAL_THEN(func1, func2)

// D_FUNCTIONAL_COMPOSE_FN
//   macro: creates a composition function from two transformer functions.
// This is a helper for creating named composition functions.
#define D_FUNCTIONAL_COMPOSE_FN(func_name,                                  \
                                func1,                                      \
                                func2,                                      \
                                type)                                       \
    D_INLINE bool                                                           \
    func_name                                                               \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        type temp;                                                          \
        (void)_context;                                                     \
                                                                            \
        if (!(func1)(_input, &temp, NULL))                                  \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        return (func2)(&temp, _output, NULL);                               \
    }

// D_FUNCTIONAL_MAP_THEN_FILTER
//   macro: convenient composition of map followed by filter.
#define D_FUNCTIONAL_MAP_THEN_FILTER(data,                                  \
                                     count,                                 \
                                     size,                                  \
                                     transform,                             \
                                     test)                                  \
    d_functional_filter(d_functional_map( (data),                           \
                                          NULL,                             \
                                          (count),                          \
                                          (size),                           \
                                          (transform),                      \
                                          NULL),                            \
                         NULL,                                              \
                         (count),                                           \
                         (size),                                            \
                         (test),                                            \
                         NULL)                               


/// composable function templates

// D_FUNCTIONAL_MAKE_COMPOSABLE
//   macro: wraps a user function to make it composable.
// Usage: D_FUNCTIONAL_MAKE_COMPOSABLE(my_func, int) creates
// d_my_func_wrapper.
#define D_FUNCTIONAL_MAKE_COMPOSABLE(func_name,                             \
                                     type)                                  \
    D_INLINE bool                                                           \
    d_##func_name##_wrapper(const void* _input,                             \
                            void*       _output,                            \
                            void*       _context)                           \
    {                                                                       \
        (void)_context;                                                     \
        *(type*)_output = func_name(*(const type*)_input);                  \
                                                                            \
        return true;                                                        \
    }                                                           

// D_FUNCTIONAL_MAKE_PREDICATE_FROM                             
//   macro: converts a boolean function to fn_predicate.         
#define D_FUNCTIONAL_MAKE_PREDICATE_FROM(func_name,                         \
                                         type)                              \
    D_INLINE bool                                                           \
    d_##func_name##_predicate                                               \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
                                                                            \
        return func_name(*(const type*)_element);                           \
    }

// D_FUNCTIONAL_LIFT
//   macro: lifts a typed function into void* space for composition.
// This is the generic version that works with any signature.
#define D_FUNCTIONAL_LIFT(func_name,                            \
                          wrapper_name)                         \
    const void* wrapper_name = (const void*)func_name


// d_composed_transformer
//   struct: composition of two transformers (f . g).
// context1 and context2 are forwarded to first and second respectively,
// and may be NULL.
struct d_composed_transformer
{
    fn_transformer first;     // applied first (g)
    fn_transformer second;    // applied second (f)
    void*          context1;  // context for first transformer
    void*          context2;  // context for second transformer
    size_t         temp_size; // size of intermediate result
    void*          temp_buf;  // temporary buffer for intermediate result
};

// i.    transformer composition
struct d_composed_transformer* d_functional_compose_new(fn_transformer _first, void* _context1, fn_transformer _second, void* _context2, size_t _temp_size);
bool                           d_functional_compose_apply(const struct d_composed_transformer* _composed, const void* _input, void* _output);
void                           d_functional_compose_free(struct d_composed_transformer* _composed);


// d_partial_consumer
//   struct: partially applied consumer with bound context.
struct d_partial_consumer
{
    fn_consumer consumer;
    void*       context;
};

// ii.   partial application operations
struct d_partial_consumer* d_functional_partial_consumer_new(fn_consumer _consumer, void* _context);
void                       d_functional_partial_consumer_apply(const struct d_partial_consumer* _partial, void* _element);
void                       d_functional_partial_consumer_free(struct d_partial_consumer* _partial);


#endif  // DJINTERP_C_FUNCTIONAL_COMPOSE_