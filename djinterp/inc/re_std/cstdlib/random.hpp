/******************************************************************************
* djinterp [re_std]                                                   random.hpp
*
* rand and srand (re-exports):
*   The C pseudo-random generator. Re-exported rather than reimplemented
* because the sequence is the runtime's, and code that seeds with srand
* and reads with a re_std::rand would otherwise be drawing from a
* different generator than the rest of the process.
*
*   KNOW WHAT THIS IS BEFORE USING IT:
*   rand() carries hidden global state, so it is not thread-safe; its
* quality is unspecified and is poor on several widely used runtimes; and
* the common `rand() % n` idiom is biased whenever n does not divide
* RAND_MAX + 1. RAND_MAX itself is only guaranteed to reach 32767.
*
*   None of that is fixable from here -- a better generator would be a
* different generator, and providing one under this name would break the
* interoperation that is the reason for re-exporting. The right answer is
* <random>'s engines and distributions, catalogued at priority 40 and not
* yet implemented.
*
*   RAND_MAX is a macro and therefore has no re_std:: spelling; including
* this header makes it available exactly as <cstdlib> does.
*
*
* path:      /inc/djinterp/re_std/cstdlib/random.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CSTDLIB_RANDOM_
#define DJINTERP_RE_STD_CSTDLIB_RANDOM_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstdlib>


NS_RESTD

    // rand
    //   function: next value in the runtime's pseudo-random sequence,
    // in [0, RAND_MAX]. Not thread-safe; quality unspecified.
    using ::std::rand;

    // srand
    //   function: seed the sequence. The same seed reproduces the same
    // sequence within one runtime, but not across runtimes.
    using ::std::srand;

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CSTDLIB_RANDOM_
