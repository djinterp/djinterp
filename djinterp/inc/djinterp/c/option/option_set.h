/*******************************************************************************
* djinterp [c]                                                      option_set.h
*
* The C face of the option_set module. Tier 1a -- C ERGONOMICS over the tier 0
* core, adding no semantics of its own.
*
* WHAT THIS BUYS OVER CALLING THE CORE DIRECTLY.
*   Three things, and nothing else.
*
*   ONE: storage that cannot fall out of step with its schema. A set declared
* from a schema table is sized by that table, so adding a key widens the
* storage in the same edit. Hand-sized storage is the failure this removes,
* and it is the failure that shows up as D_OPTION_STATUS_CAPACITY at run time on a
* machine that is not yours.
*
*   TWO: typed access. `d_option_set_get` takes a `void*` and a width, which is
* correct and unpleasant. The macros here take a typed lvalue and derive both,
* so the width can no longer disagree with the destination -- the mismatch the
* core is obliged to report as D_OPTION_STATUS_VALUE_TYPE_MISMATCH becomes one it
* never has to.
*
*   THREE: a loop that reads like a loop.
*
*   None of it computes anything the core does not. Delete this header and
* every program still expressible, just longer -- which is the test tier 1a
* has to pass.
*
* THE SCHEMA EMITTERS ARE WHERE THE C FACE EARNS ITS PLACE.
*   `D_OPTION_SET_DECLARE_SCHEMA` and `D_OPTION_SET_INIT_SCHEMA` turn one
* X-macro table into a storage type, a name table, and a populated set. The
* C++ face reads the SAME table -- a C macro expands under a C++ compiler,
* which is the direction that works, and is why the schema lives on this side
* rather than as a C++ pack that C could never see.
*
*
* path:      /inc/djinterp/c/option/option_set.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.30
*                                                            revised: 2026.10.03
*******************************************************************************/

#ifndef DJINTERP_C_OPTION_OPTION_SET_H
#define DJINTERP_C_OPTION_OPTION_SET_H 1

// std
#include <stddef.h>
// djinterp
// c
#include "../djinterp.h"
#include "./option_common.h"
#include "./option_set_common.h"
#include "./option.h"
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // int32_t, uint32_t

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


D_EXTERN_C_BEGIN

///////////////////////////////////////////////////////////////////////////////
///             I.    SCHEMA-SIZED STORAGE                                  ///
///////////////////////////////////////////////////////////////////////////////

// D_OPTION_SCHEMA_BYTES_ROW / D_OPTION_SCHEMA_VALUE_BYTES
//   macro: an UPPER BOUND on the value block a schema needs -- the sum of its
// value widths plus seven bytes of alignment slack per row.
//   An upper bound and not an exact size, deliberately. The set packs slots
// under their own alignment, so the exact figure depends on the order the rows
// are added and on the target's alignment rules, and a macro that claimed to
// know it would be wrong on some target and right on yours. Over-provisioning
// by at most seven bytes per key is the cheap correct answer; a set that
// reports D_OPTION_STATUS_VALUE_CAPACITY on a schema it was sized from would be the
// expensive one.
#define D_OPTION_SCHEMA_BYTES_ROW(name, key, KT, VT, dflt, flags)           \
    + (uint32_t)(sizeof(VT) + 7u)

#define D_OPTION_SCHEMA_VALUE_BYTES(SCHEMA)                                 \
    ((uint32_t)(0u SCHEMA(D_OPTION_SCHEMA_BYTES_ROW)))

// D_OPTION_SET_DECLARE_SCHEMA
//   macro: declare a storage type sized from a schema. Use at file or block
// scope, then instantiate and view it:
//
//     D_OPTION_SET_DECLARE_SCHEMA(window_storage, WINDOW_OPTIONS);
//     struct window_storage raw = D_OPTION_SET_STORAGE_INIT;
//     struct d_option_set   set = D_OPTION_SET_VIEW(raw);
//     D_OPTION_SET_INIT_SCHEMA(&set, WINDOW_OPTIONS);
//
//   Automatic storage throughout. No allocator is involved at any step, which
// is what lets this subframework be finished ahead of the C container
// substrate rather than behind it.
#define D_OPTION_SET_DECLARE_SCHEMA(name,                                   \
                                    SCHEMA)                                 \
    D_OPTION_SET_DECLARE(name,                                              \
                         D_OPTION_SCHEMA_COUNT(SCHEMA),                     \
                         D_OPTION_SCHEMA_VALUE_BYTES(SCHEMA))

// D_OPTION_SCHEMA_ADD_ROW
//   macro (internal): one row's contribution to D_OPTION_SET_INIT_SCHEMA.
// Adds the column, then seeds its default -- so a freshly initialised set
// already holds every default and D_OPTION_FLAG_ASSIGNED distinguishes what a
// caller has since changed.
//
//   The expansion CARRIES ITS OWN SEMICOLON, which is unusual and is why it is
// called out. X-macro rows are juxtaposed with no separator between them, so a
// row ending in `while (0)` and nothing else runs straight into the next row's
// `do`. Every emitter that expands to a statement has to terminate itself.
#define D_OPTION_SCHEMA_ADD_ROW(name, key, KT, VT, dflt, flags)             \
    do                                                                      \
    {                                                                       \
        if (D_OPTION_SCHEMA_IS_UNARY(flags))                                \
        {                                                                   \
            (void)d_option_set_add_unary(D_INTERNAL_OS_TARGET,              \
                                         D_OPTION_KEY(key),                 \
                                         D_OPTION_TYPE(KT));                \
        }                                                                   \
        else                                                                \
        {                                                                   \
            VT D_INTERNAL_OS_SEED = (VT)(dflt);                             \
                                                                            \
            (void)d_option_set_add(D_INTERNAL_OS_TARGET,                    \
                                   D_OPTION_KEY(key),                       \
                                   D_OPTION_TYPE(KT),                       \
                                   D_OPTION_TYPE(VT),                       \
                                   D_OPTION_SIZE(VT),                       \
                                   D_OPTION_ALIGN(VT));                     \
            (void)d_option_set_set(D_INTERNAL_OS_TARGET,                    \
                                   D_OPTION_KEY(key),                       \
                                   &D_INTERNAL_OS_SEED,                     \
                                   sizeof(VT));                             \
        }                                                                   \
    } while (0);

// D_OPTION_SET_INIT_SCHEMA
//   macro: populate a viewed set from a schema, in declaration order, seeding
// every default. Declaration order is preserved because the parity oracle
// diffs line by line -- see option_set_common.h section I.
//
//   Note the seeding leaves D_OPTION_FLAG_ASSIGNED raised on every valued
// column, since a default IS a write. Clear it with
// `d_option_set_mark_pristine` when "has the caller touched this?" is the
// question being asked.
#define D_OPTION_SET_INIT_SCHEMA(set,                                       \
                                 SCHEMA)                                    \
    do                                                                      \
    {                                                                       \
        struct d_option_set* D_INTERNAL_OS_TARGET = (set);                  \
                                                                            \
        d_option_set_clear(D_INTERNAL_OS_TARGET);                           \
        SCHEMA(D_OPTION_SCHEMA_ADD_ROW)                                     \
        d_option_set_mark_pristine(D_INTERNAL_OS_TARGET);                   \
    } while (0)

// d_option_set_mark_pristine
//   function: lower D_OPTION_FLAG_ASSIGNED across the set, declaring its
// current contents to be the baseline. Called at the end of schema
// initialisation so that "assigned" thereafter means "changed by a caller".
void     d_option_set_mark_pristine(struct d_option_set* _set);

// d_option_set_is_pristine
//   function: whether no column has been assigned since the last
// `d_option_set_mark_pristine`. One walk over a flat table -- the same fold
// the archive core uses to answer the same question about its knobs, rather
// than a comparison against a second set that has to be constructed first.
bool     d_option_set_is_pristine(const struct d_option_set* _set);


///////////////////////////////////////////////////////////////////////////////
///             II.   TYPED ACCESS                                          ///
///////////////////////////////////////////////////////////////////////////////

// D_OPTION_GET
//   macro: read a key's value into a typed lvalue. The width comes from the
// destination, so it cannot disagree with it. Yields the
// `struct d_option_result` the core returned, so a formal failure is still
// distinguishable from a mechanical one.
//
//     struct d_option_result r = D_OPTION_GET(&set, win_opt_width, w);
#define D_OPTION_GET(set, key, dest)                                        \
    d_option_set_get((set),                                                 \
                     D_OPTION_KEY(key),                                     \
                     &(dest),                                               \
                     sizeof(dest))

// D_OPTION_SET_VALUE
//   macro: write a key's value from a typed lvalue. Named with the `_VALUE`
// suffix because `D_OPTION_SET` would read as the type and not the operation,
// and the two appear on adjacent lines often enough for that to matter.
#define D_OPTION_SET_VALUE(set, key, src)                                    \
    d_option_set_set((set),                                                 \
                     D_OPTION_KEY(key),                                      \
                     &(src),                                                 \
                     sizeof(src))

// D_OPTION_GET_OR
//   macro: read a key's value, or evaluate to `fallback` when the key is
// absent, unary, or of another width. The one accessor that discards the
// status, and it says so in its name -- every other spelling reports.
#define D_OPTION_GET_OR(set, key, dest, fallback)                            \
    ( (d_option_result_status_of(D_OPTION_GET((set), (key), (dest))) == D_OPTION_STATUS_OK) \
          ? (dest)                                                           \
          : (fallback) )

// d_option_result_status_of
//   function: the status carried by a result value, for use in expressions
// where taking its address is not possible. The pointer form is
// `d_option_status` in option_common.h.
int32_t  d_option_result_status_of(struct d_option_result _result);


///////////////////////////////////////////////////////////////////////////////
///             III.  ITERATION                                             ///
///////////////////////////////////////////////////////////////////////////////

// D_OPTION_SET_FOREACH
//   macro: walk a set in declaration order, binding `cell` to each
// `const struct d_option*` and `idx` to its index.
//
//     D_OPTION_SET_FOREACH(&set, cell, i)
//     {
//         printf("%u %llu\n", i, (unsigned long long)cell->key);
//     }
//
//   The callback form -- `d_option_set_for_each` with an `fn_option_visit` --
// remains the composable path and is what a fold should use. This is for the
// cases where writing a callback and its context struct is more machinery
// than the loop it replaces.
#define D_OPTION_SET_FOREACH(set, cell, idx)                                 \
    for (uint32_t idx = 0u; idx < d_option_set_size(set); ++idx)             \
        for (const struct d_option* cell =                                   \
                 d_option_set_at_const((set), idx);                          \
             cell != NULL;                                                   \
             cell = NULL)


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_OPTION_OPTION_SET_H
