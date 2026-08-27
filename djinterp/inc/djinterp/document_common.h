/******************************************************************************
* djinterp [c/util/document]                                 document_common.h
*
* THE HINT BAG.  A flat run of key/value pairs, borrowed, in canonical order.
*
* BORROWED, NOT OWNING, AND THAT IS THE REAL DIFFERENCE FROM THE C++ SIDE.
*   `djinterp::doc_attributes` is a `container_metadata<std::string,
* std::string>`: it owns its keys and values and grows itself.  This does
* neither.  `items` points into storage the caller already had -- a parse
* arena, a static array -- and the bag is a view over a contiguous run.
* Copying a `d_doc_attributes` copies three words and shares the run.
*   That divergence is structural rather than chosen: C has no owning string,
* and the parse arena exists precisely so that nothing here allocates.
*
* "CANONICAL" IS NOT A SECOND ORDERING, AND READING IT AS ONE IS THE TRAP.
*   It was described as though the C side sorted while the C++ side kept
* insertion order.  It does not.  `container_metadata::set` walks its vector,
* OVERWRITES IN PLACE when the key is already present, and appends otherwise
* -- so a repeated key keeps its FIRST position and takes its LAST value.
* Canonical form here is defined to be exactly that.
*   THIS IS NOT A PREFERENCE.  `layout_parse_parity.cpp` compares the two bags
* POSITIONALLY: `bags_agree` iterates the C++ container and indexes
* `items[i]` in step.  Any other order fails at the first repeated key.  The
* C++ container's behaviour is the specification; this implements it.
*
* CANONICALISE COMPACTS IN PLACE.  A run arrives as written, duplicates and
* all; the bag that comes back points at a PREFIX of that same run with the
* duplicates removed.  The caller then advances its arena by the bag's count
* rather than by what it wrote -- `layout_parse.c` does exactly that:
* `_arena->attr_used = _first + _out_bag->count;`.  Nothing is copied,
* nothing is freed.
*
* path:      /inc/djinterp/c/util/document/document_common.h
* link(s):   TBA
* author(s): TBA                                            created: 2026.08.23
******************************************************************************/

#ifndef DJINTERP_C_UTIL_DOCUMENT_COMMON_
#define DJINTERP_C_UTIL_DOCUMENT_COMMON_ 1

#include <stddef.h>
#include <stdint.h>
#include "../../djinterp.h"

D_EXTERN_C_BEGIN


// d_doc_attr
//   struct: one key/value pair.  BOTH BORROWED -- the strings live wherever
// the caller put them, and must outlive every bag that names them.
struct d_doc_attr
{
    const char* key;
    const char* value;
};

// d_doc_attributes
//   struct: a view over a contiguous, canonical run of pairs.
//
//   `reserved` MUST BE ZERO.  It is not slack to ignore: it makes the struct
// the same shape on 32- and 64-bit targets instead of whatever the compiler
// chose, and gives a later field somewhere to go without moving `count`.
struct d_doc_attributes
{
    const struct d_doc_attr* items;     // borrowed run, canonical
    uint32_t                 count;     // pairs in it
    uint32_t                 reserved;  // must be 0
};

// D_DOC_ATTRIBUTES_EMPTY
//   The empty bag: a NULL run and a zero count.  This is what the root of a
// parsed document carries, matching `doc_attributes()` at the same position
// on the C++ side.  An empty bag is NOT the absence of a bag -- every node
// has one.
#define D_DOC_ATTRIBUTES_EMPTY { (const struct d_doc_attr*)0, 0u, 0u }

// d_doc_attributes_canonicalise
//   Compacts `_run` in place to canonical form and returns a bag over it.
//
//   FIRST POSITION, LAST VALUE.  `a=1 b=2 a=3` becomes `a=3 b=2`: the repeat
// does not move `a` to the end, and it does not lose the later value.  That
// is `container_metadata::set` written out, and it is the ONLY ordering
// authority in this tier -- a caller that sorted a run before handing it over
// would break parity rather than help it.
//
//   `_run` IS WRITTEN THROUGH, necessarily: dropping a duplicate means moving
// what follows it down.  A caller needing the original order needs its own
// copy, and none in the corpus does.
//
// Parameter(s):
//   _run:    the pairs as written; compacted in place.  May be NULL iff
//            _count is 0.
//   _count:  how many were written.
// Return:
//   a bag over the canonical prefix of _run.
struct d_doc_attributes d_doc_attributes_canonicalise(struct d_doc_attr* _run,
                                                      uint32_t           _count);

// d_doc_attributes_is_canonical
//   Whether `_bag` holds no repeated key.
//
//   THE POINT IS THE INVARIANT, NOT THE QUERY.  A bag reaching the
// interpreter with a duplicate renders one hint twice and nobody notices.
// This is what an assertion or a parity body calls to say the invariant held,
// not something a caller is expected to branch on.
//
// Return:
//   1 when canonical, 0 otherwise.  An empty bag is canonical.
int32_t d_doc_attributes_is_canonical(const struct d_doc_attributes* _bag);

// d_doc_attributes_find
//   The value bound to `_key`, or NULL.  Linear, like the C++ `find` it
// mirrors: a hint bag holds a handful of entries and an index would cost more
// than it saved.
const char* d_doc_attributes_find(const struct d_doc_attributes* _bag,
                                  const char*                    _key);


D_EXTERN_C_END

#endif  // DJINTERP_C_UTIL_DOCUMENT_COMMON_
