/******************************************************************************
* djinterp [parse]                                                   storage.h
*
* The one growth policy the subframework's containers share.
*   Four containers here hold an array that may be caller-supplied or owned:
* the operator registry, the operand pool's two arrays, the instruction stream,
* and the diagnostic sink. Each had written the same twelve lines -- refuse if
* the storage is not ours, double until the requirement fits, reallocate, zero
* the tail -- and the fourth copy is the one that justifies extracting it.
*
*   WHAT IS NOT EXTRACTED, AND WHY. Not the containers themselves. A generic
* buffer carrying an element size would turn every access into a runtime
* multiply against a loaded stride, and d_parse_op_set_find is the innermost
* thing in the system: it must stay `&set->ops[code]`, a shift and a load
* against a typed array. So the containers keep their typed fields and share
* only the algorithm, which is where the duplication actually was.
*
*   The diagnostic sink deliberately does not use this: a full sink keeps
* tallying what it can no longer store, which is a better behaviour than
* growing without bound while something upstream misbehaves.
*
*   Requires: c/djinterp.h (qualifier kit) and config/parse/cfg_parse.h.
*
* path:      /inc/djinterp/parse/storage.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/

#ifndef DJINTERP_PARSE_STORAGE_
#define DJINTERP_PARSE_STORAGE_ 1

// std
#include <stdint.h>                     // uint32_t
// djinterp
#include "../c/djinterp.h"              // framework root
#include "../config/parse/cfg_parse.h"  // D_INTERNAL_PARSE_HEAP


#if (D_INTERNAL_PARSE_HEAP == 1)

D_EXTERN_C_BEGIN

//   Growth. Returns the storage to use, which may be the storage passed in,
// or NULL if the requirement cannot be met -- so a caller assigns the result
// back and checks it for NULL, with no pointer-to-pointer cast anywhere.
void*           d_parse_grow(void*     _data,
                             uint32_t* _capacity,
                             uint32_t  _needed,
                             uint32_t  _stride,
                             int       _owned);

D_EXTERN_C_END

#endif  // D_INTERNAL_PARSE_HEAP


#endif  // DJINTERP_PARSE_STORAGE_
