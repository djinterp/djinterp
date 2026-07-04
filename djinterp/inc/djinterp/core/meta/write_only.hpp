/******************************************************************************
* djinterp [container]                                           write_only.hpp
*
*   The WRITE_ONLY access-capability tag: a handle that grants modification but
* NOT observation of element values - a sink.  It is the corner of the access
* lattice (see read_write.hpp) opposite read_only.
*
*   The intended write_only contract is APPEND-ONLY: a holder may push data in
* (push_back / insert-at-end / emplace / clear) but may not read element values
* back out, and may not perform the position-dependent edits (random insert,
* erase) that would require observing the contents first.  Container metadata
* that reveals no element value - size(), empty() - remains observable; reading
* an element does not.  This contract is realised by the write_only_container
* wrapper, which holds its container privately and forwards only the append
* surface plus that value-free metadata.
*
*   An access capability is an OVERLAY, orthogonal to the intrinsic mutability
* grade (mutable_container_traits.hpp): write_only is NOT a grade - the
* underlying container is whatever it is - it is a restriction on the handle.
*
*   The tag is self-describing through `can_read` / `can_write`.
*
*   PORTABILITY:
*   C++11 baseline.
*
*
* path:      /inc/djinterp/core/container/access/write_only.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_WRITE_ONLY_
#define DJINTERP_WRITE_ONLY_ 1

// djinterp
#include "../../djinterp.hpp"   // NS_*


NS_DJINTERP


// write_only
//   tag: the access capability granting modification only (an append-only sink).
struct write_only
{
    static constexpr bool can_read  = false;
    static constexpr bool can_write = true;

    // name
    //   function: the capability's stable spelling.
    static constexpr const char* name() noexcept { return "write_only"; }
};


NS_END  // djinterp


#endif  // DJINTERP_WRITE_ONLY_
