/******************************************************************************
* djinterp [container]                                           read_write.hpp
*
*   The READ_WRITE access-capability tag: a handle that grants BOTH observation
* and modification.  It is the top of the small access lattice
*
*               read_write          (read + write)
*               /        \
*       read_only       write_only  (read only)  (write only)
*
*   whose three named corners parameterise the container access wrappers.  An
* access capability is an OVERLAY: it constrains what a HOLDER may do through a
* given handle, and is orthogonal to a container's intrinsic mutability grade
* (mutable_container_traits.hpp) - the grade says what the underlying type CAN
* do, the capability says what THIS handle is permitted to do.
*
*   Each tag is self-describing through two constexpr bits, `can_read` and
* `can_write`; consumers (the wrappers) read those directly rather than through
* a predicate, so the tag carries no dependency beyond the core.
*
*   PORTABILITY:
*   C++11 baseline.
*
*
* path:      /inc/djinterp/core/container/access/read_write.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_READ_WRITE_
#define DJINTERP_READ_WRITE_ 1

// djinterp
#include "../../djinterp.hpp"   // NS_*


NS_DJINTERP


// read_write
//   tag: the access capability granting observation AND modification.
struct read_write
{
    static constexpr bool can_read  = true;
    static constexpr bool can_write = true;

    // name
    //   function: the capability's stable spelling (a function, not a data
    // member, so taking the name never requires an out-of-line definition).
    static constexpr const char* name() noexcept { return "read_write"; }
};


NS_END  // djinterp


#endif  // DJINTERP_READ_WRITE_
