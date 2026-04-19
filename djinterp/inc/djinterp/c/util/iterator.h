/******************************************************************************
* djinterp [container]                                              iterator.h
*
* 
* 
*
* link:      TBA
* file:      \inc\container\iterator.h   
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.20
******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_ITERATOR_
#define DJINTERP_C_CONTAINER_ITERATOR_ 1

#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include "..\..\djinterp.h"
#include "..\..\dmemory.h"


#include <stdlib.h>
#include "..\djinterp.h"
#include "..\dmemory.h"


// fn_predicate
//   typedef: function pointer type for a boolean predicate over an element.
// Returns true if the element satisfies the condition, false otherwise.
typedef bool (*fn_predicate)(const void* _element);

// fn_predicate_context
//   typedef: function pointer type for a boolean predicate over an element
// with additional caller-supplied context.
typedef bool (*fn_predicate_context)(const void* _element,
                                     void*       _context);

// fn_map
//   typedef: function pointer type for a mapping function that transforms
// an element, writing the result into `_out`.  Returns true on success.
typedef bool (*fn_map)(const void* _element,
                       void*       _out);

// fn_map_context
//   typedef: function pointer type for a mapping function with additional
// caller-supplied context.
typedef bool (*fn_map_context)(const void* _element,
                               void*       _out,
                               void*       _context);


// d_iterator
//   struct: a generic, container-agnostic iterator.
//   `next` returns a pointer to the current element and advances the
// iterator, or NULL when all elements have been consumed.
//   `reset` restores the iterator to its initial position; returns true
// on success and false if the iterator does not support resetting.
//   `destroy` frees all memory owned by the iterator (including `state`).
// The iterator struct itself is NOT freed — the caller owns it.
struct d_iterator
{
    void*  state;                                        // opaque state
    size_t element_size;                                 // bytes per element
    void*  (*next)(struct d_iterator* _iterator);        // advance + yield
    bool   (*reset)(struct d_iterator* _iterator);       // rewind
    void   (*destroy)(struct d_iterator* _iterator);     // free state
};


// D_ITER_FOREACH
//   macro: ergonomic iteration loop.  Declares a pointer variable of the
// given type and iterates until `next` returns NULL.
//   Example:
//     struct d_iterator it = d_array_iterator(&arr, sizeof(int));
//     D_ITER_FOREACH(int, val, &it)
//     {
//         printf("%d\n", *val);
//     }
//     it.destroy(&it);
#define D_ITER_FOREACH(element_type, var_name, iter_ptr)                    \
    for (element_type* var_name =                                           \
             (element_type*)(iter_ptr)->next((iter_ptr));                   \
         var_name != NULL;                                                  \
         var_name =                                                         \
             (element_type*)(iter_ptr)->next((iter_ptr)))


// I.    core operations
void*    d_iterator_next(struct d_iterator* _iterator);
bool     d_iterator_reset(struct d_iterator* _iterator);
void     d_iterator_free(struct d_iterator* _iterator);

// II.   consumption functions
void     d_iterator_foreach(struct d_iterator* _iterator, fn_apply _fn);
void     d_iterator_foreach_context(struct d_iterator* _iterator, fn_apply_context _fn, void* _context);
size_t   d_iterator_count(struct d_iterator* _iterator);
bool     d_iterator_any(struct d_iterator* _iterator, fn_predicate _predicate);
bool     d_iterator_all(struct d_iterator* _iterator, fn_predicate _predicate);
void*    d_iterator_find(struct d_iterator* _iterator, fn_predicate _predicate);

// III.  combinator constructors
struct   d_iterator d_iterator_filter(struct d_iterator* _inner, fn_predicate _predicate);
struct   d_iterator d_iterator_filter_context(struct d_iterator* _inner, fn_predicate_context _predicate, void* _context);
struct   d_iterator d_iterator_map(struct d_iterator* _inner, fn_map _transform, size_t _out_element_size);
struct   d_iterator d_iterator_map_context(struct d_iterator* _inner, fn_map_context _transform, size_t _out_element_size, void* _context);


#endif	// DJINTERP_C_CONTAINER_BUFFER_TEXT_