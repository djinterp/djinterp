/******************************************************************************
* djinterp [container]                                         hierarchical.hpp
*
*   The HIERARCHICAL structural-kind tag: an opt-in marker for the structure
* axis, declaring that a container nests - depth >= 2, a node summand present.
*
*   THE MODEL:
*   A container type factors as T = tau + F[T]: a LEAF summand (an element of the
* base type tau) and an optional NODE summand F[T] (a sub-container).  The node
* summand is present exactly where nesting occurs, so its presence IS hierarchy;
* the depth d counts nodes, and a container is HIERARCHICAL (nested) when d >= 2.
* This tag names that case.  Where the nesting is regular -- every leaf at one
* depth -- the type collapses to an iterated container of fixed depth; in the
* general composite case two values may nest differently and depth is a property
* of values, but the TYPE is hierarchical all the same.
*
*   USE - IN CONJUNCTION WITH STRUCTURAL DETECTION:
*   The structure axis is decided primarily by structural SFINAE traits (see
* hierarchical_container_traits.hpp): a node_type member -- the F[T] summand --
* is the strong, authoritative tell, and a container-shaped value_type is a
* weaker structural heuristic.  This tag is the OPT-IN supplement a type carries
* through `using structure_category = hierarchical;`, asserting hierarchy that
* structural detection cannot see (a composite type whose recursion the value_type
* chain does not expose).  Read it through the `nests` bit (true here), which the
* traits consult alongside the structural signals.
*
*   PORTABILITY:
*   C++11 baseline.
*
*
* path:      /inc/djinterp/core/container/structure/hierarchical.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_HIERARCHICAL_
#define DJINTERP_HIERARCHICAL_ 1

// std
#include <cstddef>
// djinterp
#include "../../djinterp.hpp"   // NS_*


NS_DJINTERP


// hierarchical
//   tag: the structural kind of a container that nests (depth >= 2).
struct hierarchical
{
    static constexpr bool        nests     = true;    // node summand present
    static constexpr std::size_t min_depth = 2;       // some component a node

    // name
    //   function: the kind's stable spelling.
    static constexpr const char* name() noexcept { return "hierarchical"; }
};


NS_END  // djinterp


#endif  // DJINTERP_HIERARCHICAL_
