/*******************************************************************************
* djinterp [tools]                                                       dhost.h
*
* Host functions: what an extension adds to the language.
*   A host function is called from a program as a user function is, by name
* and with parentheses, and a function the program defines itself wins over
* one of the same name. So a program written for one extension reads as awk,
* and one that never calls a host function cannot tell whether any is there.
*   This is the builtin seam of the six dawk.h describes, lowered onto
* d_awk_interp as the record source is in dsource.h. Arguments are values and
* the result is a value: a host function takes no array and cannot assign to
* its arguments, so it cannot reach what the program has not handed it.
*   Under D_AWK_STRICT_POSIX installation is refused, so the conforming core
* stays independently verifiable.
*
* path:      /inc/djinterp/tools/dawk/dhost.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Hook types
         1.  d_awk_fn_host
    2.  Descriptor
         1.  d_awk_host_function
*/

#ifndef DJINTERP_TOOLS_DAWK_DHOST_H
#define DJINTERP_TOOLS_DAWK_DHOST_H 1

// std
#include <stdbool.h>   // bool
#include <stddef.h>    // size_t
// djinterp
#include "./dvalue.h"  // d_awk_value


//==============================================================================
// 1.  TYPES
//==============================================================================
// The callback a host function is, and the descriptor that names it.


// 1.1    Hook types
//------------------------------------------------------------------------------
// 1.1.1
// d_awk_fn_host
//   type: one host function.  `_args` holds `_count` values, already
// evaluated, in the order the call gave them; `_out` receives the result and
// arrives uninitialised.  Returns true, or false with a diagnostic of at most
// `_capacity` bytes in `_problem`, which stops the run as a runtime error.
// Failure is for a program error -- a call that cannot mean anything -- and
// an operation that can fail in the world, such as opening a file, reports
// its failure through `_out` instead, as getline does.
typedef bool
(*d_awk_fn_host)(void*                _user,
                 struct d_awk_value** _args,
                 size_t               _count,
                 struct d_awk_value*  _out,
                 char*                _problem,
                 size_t               _capacity);

// 1.2    Descriptor
//------------------------------------------------------------------------------
// 1.2.1
// d_awk_host_function
//   struct: one host function as an extension declares it.  A call outside
// `least` to `most` arguments is a runtime error naming the function, so an
// implementation may index its arguments without checking the count.
struct d_awk_host_function
{
    const char*   name;   // the name a program calls it by
    size_t        least;  // the fewest arguments it takes
    size_t        most;   // the most arguments it takes
    d_awk_fn_host call;   // the implementation
};


#endif  // DJINTERP_TOOLS_DAWK_DHOST_H
