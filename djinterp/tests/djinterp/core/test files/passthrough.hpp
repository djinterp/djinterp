/******************************************************************************
* djinterp [meta]                                              passthrough.hpp
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
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.27
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    passthrough_marker          (the inheritable empty base)
II.   is_passthrough              (trait + variable)
III.  Passthrough                 (C++20 concept analog)
*/

#ifndef DJINTERP_META_PASSTHROUGH_
#define DJINTERP_META_PASSTHROUGH_ 1

// std
#include <type_traits>
// djinterp
#include "../djinterp.hpp"


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
//   trait: true iff _Type inherits from passthrough_marker
// (after cv-ref stripping).
//
//   The single-typename shape matches the unary trait template
// expected by dtuple_wrap_partition's `_IsPassthrough` slot, so
// this can be passed directly without an adapter:
//     partition_wrap_except_t<W, N, is_passthrough, _Source...>
template<typename _Type>
struct is_passthrough
    : std::is_base_of<
          passthrough_marker,
          clean_t<_Type>
      >
{};

template<typename _Type>
D_CONSTEXPR_INLINE bool is_passthrough_v = is_passthrough<_Type>::value;


// ===========================================================================
// III. Passthrough
// ===========================================================================

#if defined(__cpp_concepts)

    // Passthrough
    //   concept: satisfied iff _Type is a passthrough.
    // Parallels is_passthrough_v, in Capital-letter form per
    // the project's concept naming convention.
    template<typename _Type>
    concept Passthrough = is_passthrough_v<_Type>;

#endif


NS_END  // djinterp


#endif  // DJINTERP_META_PASSTHROUGH_
