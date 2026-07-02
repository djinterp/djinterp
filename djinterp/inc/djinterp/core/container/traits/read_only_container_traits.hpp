/******************************************************************************
* djinterp [container]                           read_only_container_traits.hpp
*
*   The read_only corner of the access axis: is_read_only_container, true iff a
* handle to the type grants observation but NOT mutation.  It is one of the two
* restrictions below the read_write baseline (read_write_container_traits.hpp),
* and it defers ENTIRELY to that baseline's classifier - it adds no detection of
* its own, only the predicate that picks out the read_only verdict.  The shared
* machinery (the structural read / write probes, the `capability` opt-in, the
* enum and its tag re-emission) lives once in the baseline; this header names its
* read_only face.
*
*   A read_only handle is detected either by its stamped `capability = read_only`
* tag (read back for posterity) or, absent a tag, structurally: a value observer
* is present (const operator[] / front / data / begin) while every mutator is
* absent.  This mirrors the read_only_container wrapper, which forwards only the
* const surface.
*
*   ORTHOGONALITY:
*   read_only here is the ACCESS capability of a handle, not the intrinsic
* mutability grade (mutable_container_traits.hpp): a read_only handle to a fully
* mutable container reads as read_only regardless of what the underlying type
* could do.
*
*   PORTABILITY:
*   C++11 baseline; the `_v` companion degrades with the language as the rest do.
*
*
* path:      /inc/djinterp/core/container/traits/read_only_container_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.01
******************************************************************************/

#ifndef DJINTERP_READ_ONLY_CONTAINER_TRAITS_
#define DJINTERP_READ_ONLY_CONTAINER_TRAITS_ 1

// std
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"                    // clean_t, NS_*
#include "../../meta/trait_detect.hpp"           // D_TYPE_TRAIT_VALUE_BOOL
#include "./read_write_container_traits.hpp"     // access_capability, access_capability_of


NS_DJINTERP


// is_read_only_container
//   trait: true iff a handle to the type grants observation only - the read_only
// corner of the access lattice.  Delegates to the baseline access classifier.
template<typename _Type>
struct is_read_only_container
    : std::integral_constant<bool,
          access_capability_of<clean_t<_Type>>::value
              == access_capability::read_only>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_read_only_container)


NS_END  // djinterp


#endif  // DJINTERP_READ_ONLY_CONTAINER_TRAITS_
