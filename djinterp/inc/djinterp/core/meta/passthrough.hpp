/*******************************************************************************
* djinterp [core]                                                passthrough.hpp
*
*   `passthrough_marker`: an inheritable empty base used as a structural
* opt-in for any pipeline that supports "passthrough" semantics - a type
* whose presence in some input stream should be preserved at its
* original position without further transformation.
*
*   Generic and intentionally narrow: no domain knowledge.  The unary
* trait `is_passthrough` has the shape expected by the partition
* engine in dtuple_wrap_partition.hpp's `_IsPassthrough` slot, so it
* can be passed directly without an adapter.
*
*   This module lives at the meta layer.  Subframeworks that adopt
* passthrough semantics should re-export under their own names (see
* e.g. option_passthrough.hpp for the options subframework).
*
*
* path:      /inc/djinterp/core/meta/passthrough.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.27
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    passthrough_marker          (the inheritable empty base)
      --------------------------------------------------------

II.   is_passthrough              (trait + variable)
      ----------------------------------------------

III.  Passthrough                 (C++20 concept analog)
      --------------------------------------------------
*/

#ifndef DJINTERP_META_PASSTHROUGH_HPP
#define DJINTERP_META_PASSTHROUGH_HPP 1


// THE MODULE FLOOR is C++14, and below it this header is EMPTY rather than
// an error (the brief's rule 5: a facility is absent from a level it
// cannot express). env is included first, as the floor reads it; the
// standard header below is C++11 and the body needs C++14.
//
//   C++14 rather than the framework floor of C++11, because is_passthrough_v
// below is a variable TEMPLATE and variable templates are C++14. The trait
// itself, is_passthrough<T>, is C++11 and would be usable at the framework
// floor -- but this header ships both, so the header's floor is the higher of
// the two. This module does not include option.hpp and so does not inherit
// that subframework's C++17 floor.
#include "../../env/env.h"  // D_ENV_LANG_IS_CPP14_OR_HIGHER
#if D_ENV_LANG_IS_CPP14_OR_HIGHER


// std
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"
#include "./type_utility.hpp"  // clean_t


NS_DJINTERP


// ===========================================================================
// I.   passthrough_marker
// ===========================================================================

// passthrough_marker
//   tag: empty inheritable base.  Any type inheriting from
// passthrough_marker is detected as a passthrough by
// `is_passthrough` / `is_passthrough_v`.  Imposes no further
// contract - the meaning of "passthrough" is defined by the
// consuming pipeline.
struct passthrough_marker
{};


// ===========================================================================
// II.  is_passthrough
// ===========================================================================

// is_passthrough
//   trait: true iff Type inherits from passthrough_marker
// (after cv-ref stripping).
//
//   The single-typename shape matches the unary trait template
// expected by dtuple_wrap_partition's `_IsPassthrough` slot, so
// this can be passed directly without an adapter:
//     partition_wrap_except_t<W, N, is_passthrough, _Source...>
template<typename Type>
struct is_passthrough
    : std::is_base_of<
          passthrough_marker,
          clean_t<Type>
      >
{};

template<typename Type>
D_CONSTEXPR bool is_passthrough_v = is_passthrough<Type>::value;


// ===========================================================================
// III. Passthrough
// ===========================================================================

#if defined(__cpp_concepts)

    // Passthrough
    //   concept: satisfied iff Type is a passthrough.
    // Parallels is_passthrough_v, in Capital-letter form per
    // the project's concept naming convention.
    template<typename Type>
    concept Passthrough = is_passthrough_v<Type>;

#endif


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER


#endif  // DJINTERP_META_PASSTHROUGH_HPP
