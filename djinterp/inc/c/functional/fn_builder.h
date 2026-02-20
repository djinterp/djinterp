/******************************************************************************
* djinterp [core]                                   functional\fn_builder.h
*
* Fluent builder pattern for constructing function chains.
*   Provides a builder struct that accumulates transformers and predicates,
* with fluent (chainable) operations for map, filter, and_then, and where.
*
* path:      \inc\functional\fn_builder.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.06
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_FN_BUILDER_
#define DJINTERP_FUNCTIONAL_FN_BUILDER_ 1

#include <stddef.h>
#include <stdlib.h>
#include "..\djinterp.h"
#include ".\functional_common.h"


// D_FN_BUILDER_INITIAL_CAPACITY
//   constant: initial capacity for builder arrays.
#define D_FN_BUILDER_INITIAL_CAPACITY 8


// struct d_fn_builder
//   struct: fluent builder for constructing function chains.
struct d_fn_builder
{
    fn_transformer* transforms;      // array of transformers
    fn_predicate*   predicates;      // array of predicates
    size_t         transform_count;
    size_t         predicate_count;
    size_t         capacity;
};

// i.    builder creation
struct d_fn_builder* d_fn_builder_new(void);

// ii.   fluent operations (return builder for chaining)
struct d_fn_builder* d_funtional_builder_map(struct d_fn_builder* _builder, fn_transformer _transform);
struct d_fn_builder* d_funtional_builder_filter(struct d_fn_builder* _builder, fn_predicate _test);
struct d_fn_builder* d_funtional_builder_and_then(struct d_fn_builder* _builder, fn_transformer _transform);
struct d_fn_builder* d_funtional_builder_where(struct d_fn_builder* _builder, fn_predicate _test);

// iii.  builder execution
bool d_fn_builder_execute(const struct d_fn_builder* _builder, const void* _input, size_t _count, size_t _element_size, void* _output, size_t* _out_count);

// iv.   builder cleanup
void d_fn_builder_free(struct d_fn_builder* _builder);


#endif  // DJINTERP_FUNCTIONAL_FN_BUILDER_
