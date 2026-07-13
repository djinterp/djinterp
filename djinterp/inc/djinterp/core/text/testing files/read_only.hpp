/******************************************************************************
* djinterp [container]                                            read_only.hpp
*
*   The READ_ONLY access-capability tag: a handle that grants observation but
* NOT modification.  It is a corner of the access lattice headed by read_write
* (see read_write.hpp): read_only and write_only are the two incomparable
* single-capability handles below the full one.
*
*   An access capability is an OVERLAY, orthogonal to a container's intrinsic
* mutability grade (mutable_container_traits.hpp): a read_only handle to a fully
* mutable container still forbids mutation through THAT handle, regardless of
* what the underlying type could do.  The read_only_container wrapper enforces
* this by holding its container privately and forwarding only the const surface.
*
*   The tag is self-describing through `can_read` / `can_write`.
*
*   PORTABILITY:
*   C++11 baseline.
*
*
* path:      /inc/djinterp/core/container/access/read_only.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_READ_ONLY_
#define DJINTERP_READ_ONLY_ 1

// djinterp
#include "../../djinterp.hpp"   // NS_*


NS_DJINTERP


// read_only
//   tag: the access capability granting observation only.
struct read_only
{
    static constexpr bool can_read  = true;
    static constexpr bool can_write = false;

    // name
    //   function: the capability's stable spelling.
    static constexpr const char* name() noexcept { return "read_only"; }
};


NS_END  // djinterp


#endif  // DJINTERP_READ_ONLY_
