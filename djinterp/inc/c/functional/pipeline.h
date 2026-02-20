/******************************************************************************
* djinterp [functional]                                           pipeline.h
*
* Function pipeline for chaining operations in the functional module.
*   Provides a pipeline struct that holds intermediate results and supports
* chainable map, filter, fold, for-each, take, and skip operations. Each
* operation accepts a void* _context parameter (may be NULL) that is
* forwarded to the callback.
*
* path:      \inc\functional\pipeline.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.09
******************************************************************************/

#ifndef DJINTERP_C_FUNCTIONAL_PIPELINE_
#define DJINTERP_C_FUNCTIONAL_PIPELINE_ 1

#include <stdlib.h>
#include "..\djinterp.h"
#include ".\functional_common.h"


// d_functional_pipeline
//   struct: holds intermediate results for a function pipeline.
// Operations return a new pipeline struct, allowing chaining. If an error
// occurs at any stage, subsequent operations are no-ops and the error_code
// is propagated through.
struct d_functional_pipeline
{
    void*  data;           // current data pointer
    size_t element_size;   // size of each element
    size_t count;          // number of elements
    bool   owns_data;      // whether pipeline owns the data
    int    error_code;     // error status (0 = success)
};

// i.    pipeline creation
struct d_functional_pipeline d_functional_pipeline_begin(void* _data, size_t _count, size_t _element_size);
struct d_functional_pipeline d_functional_pipeline_begin_copy(const void* _data, size_t _count, size_t _element_size);

// ii.   pipeline operations (chainable)
struct d_functional_pipeline d_functional_pipeline_map(struct d_functional_pipeline _pipe, fn_transformer _transform, void* _context);
struct d_functional_pipeline d_functional_pipeline_filter(struct d_functional_pipeline _pipe, fn_predicate _test, void* _context);
struct d_functional_pipeline d_functional_pipeline_fold(struct d_functional_pipeline _pipe, void* _initial, size_t _accumulator_size, fn_accumulator _combine, void* _context);
struct d_functional_pipeline d_functional_pipeline_for_each(struct d_functional_pipeline _pipe, fn_consumer _apply, void* _context);
struct d_functional_pipeline d_functional_pipeline_take(struct d_functional_pipeline _pipe, size_t _n);
struct d_functional_pipeline d_functional_pipeline_skip(struct d_functional_pipeline _pipe, size_t _n);

// iii.  pipeline finalization
void* d_functional_pipeline_end(struct d_functional_pipeline _pipe, size_t* _out_count);
void  d_functional_pipeline_free(struct d_functional_pipeline* _pipe);

// D_FUNCTIONAL_PIPELINE
//   macro: starts a function pipeline.
#define D_FUNCTIONAL_PIPELINE(data, count, size) \
    d_functional_pipeline_begin((data), (count), (size))

// D_FUNCTIONAL_PIPE
//   macro: applies an operation in a pipeline (alias for readability).
#define D_FUNCTIONAL_PIPE(pipe, op) op(pipe)


#endif  // DJINTERP_C_FUNCTIONAL_PIPELINE_
