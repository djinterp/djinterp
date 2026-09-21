/*******************************************************************************
* djinterp [dawk]                                                      dsource.h
*
* Pluggable record source:
*   A record source replaces RS splitting.  The interpreter's main loop asks
* for the next record and does not care whether the bytes came from a line of
* a file, a node of a tree, or anything else.  NR is the ordinal either way.
*   This is the first of the six seams described in dawk.h, lowered onto
* d_awk_interp because that is where the record loop lives.  A source yields
* bytes rather than values, so every field-splitting invariant is unchanged
* and a host cannot regress POSIX behaviour by installing one.
*   Under D_AWK_STRICT_POSIX installation is refused, so the conforming core
* stays independently verifiable.
*
* path:      /inc/djinterp/tools/dawk/dsource.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.20
*                                                            revised: 2026.09.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Hook types
         1.  d_awk_fn_next_record
         2.  d_awk_fn_release_source
    2.  Descriptor
         1.  d_awk_source
*/

#ifndef DJINTERP_TOOLS_DAWK_DSOURCE_H
#define DJINTERP_TOOLS_DAWK_DSOURCE_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t


//==============================================================================
// 1.  TYPES
//==============================================================================
// The callback a source supplies, the callback that releases it, and the
// descriptor an interpreter holds.


// 1.1    Hook types
//------------------------------------------------------------------------------
// 1.1.1
// d_awk_fn_next_record
//   type: yields the next record.  Returns true with `_out_text` and
// `_out_length` set, or false once the source is exhausted.  The buffer
// must stay valid until the following call or until release, whichever
// comes first; the interpreter copies before returning to the loop.
typedef bool
(*d_awk_fn_next_record)(void*        _user,
                        const char** _out_text,
                        size_t*      _out_length);

// 1.1.2
// d_awk_fn_release_source
//   type: releases whatever `_user` owns.  May be NULL when the source
// owns nothing.
typedef void
(*d_awk_fn_release_source)(void* _user);

// 1.2    Descriptor
//------------------------------------------------------------------------------
// 1.2.1
// d_awk_source
//   struct: a record source installed in place of RS splitting.  A source
// with a NULL `next_record` is rejected at installation rather than
// treated as an empty input, because the two are not the same and the
// silent reading is the one that hides a wiring bug.
struct d_awk_source
{
    void*                   user;         // opaque, passed to every callback
    d_awk_fn_next_record    next_record;  // yields the next record
    d_awk_fn_release_source release;      // releases `user`; may be NULL
};


#endif  // DJINTERP_TOOLS_DAWK_DSOURCE_H
