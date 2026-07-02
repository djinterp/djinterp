/******************************************************************************
* djinterp [container]                          write_only_container_traits.hpp
*
*   The write_only corner of the access axis: is_write_only_container, true iff a
* handle to the type grants mutation but NOT observation of element values - a
* sink.  It is the restriction below the read_write baseline
* (read_write_container_traits.hpp) opposite read_only, and it defers ENTIRELY to
* that baseline's classifier - it adds no detection of its own, only the
* predicate that picks out the write_only verdict.  The shared machinery (the
* structural read / write probes, the `capability` opt-in, the enum and its tag
* re-emission) lives once in the baseline; this header names its write_only face.
*
*   A write_only handle is detected either by its stamped `capability = write_only`
* tag (read back for posterity) or, absent a tag, structurally: a mutator is
* present (push_back / emplace_back / push_front / clear / assignable
* operator[]) while every VALUE observer is absent.  Crucially, the value-free
* metadata size() / empty() is not an observer, so a sink that exposes only
* those plus the append surface still reads write-only - which is exactly the
* write_only_container wrapper's contract.
*
*   ORTHOGONALITY:
*   write_only here is the ACCESS capability of a handle, not the intrinsic
* mutability grade (mutable_container_traits.hpp): it is a restriction on the
* handle, orthogonal to what the underlying container could do.
*
*   PORTABILITY:
*   C++11 baseline; the `_v` companion degrades with the language as the rest do.
*
*
* path:      /inc/djinterp/core/container/traits/write_only_container_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.01
******************************************************************************/

#ifndef DJINTERP_WRITE_ONLY_CONTAINER_TRAITS_
#define DJINTERP_WRITE_ONLY_CONTAINER_TRAITS_ 1

// std
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"                    // clean_t, NS_*
#include "../../meta/trait_detect.hpp"           // D_TYPE_TRAIT_VALUE_BOOL
#include "./read_write_container_traits.hpp"     // access_capability, access_capability_of


NS_DJINTERP


// is_write_only_container
//   trait: true iff a handle to the type grants mutation only (an append-only
// sink) - the write_only corner of the access lattice.  Delegates to the
// baseline access classifier.
template<typename _Type>
struct is_write_only_container
    : std::integral_constant<bool,
          access_capability_of<clean_t<_Type>>::value
              == access_capability::write_only>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_write_only_container)


NS_END  // djinterp


#endif  // DJINTERP_WRITE_ONLY_CONTAINER_TRAITS_
