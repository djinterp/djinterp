/*******************************************************************************
* djinterp [core]                                                       flat.hpp
*
*   The FLAT structural-kind tag: an opt-in marker for the structure axis,
* declaring that a container holds only leaves - depth 1, no node summand.
*
*   THE MODEL:
*   A container type factors as T = tau + F[T]: a LEAF summand (an element of the
* base type tau) and an optional NODE summand F[T] (a sub-container), the latter
* present only where nesting occurs.  The depth d counts nodes: a container is
* FLAT when d = 1 (every component a leaf) and HIERARCHICAL when d >= 2 (some
* component a node).  This tag names the flat case.
*
*   USE - IN CONJUNCTION WITH STRUCTURAL DETECTION:
*   The structure axis is decided primarily by structural SFINAE traits (see
* flat_container_traits.hpp / hierarchical_container_traits.hpp): a node_type
* member is the strong tell for hierarchy.  This tag is the OPT-IN supplement a
* type carries through `using structure_category = flat;`.  Because the strong
* structural signal is authoritative, a flat tag never overrides a node_type;
* what it does is assert leaf-ness where only the WEAK heuristic (a container-
* shaped value_type, e.g. a string element) would otherwise read as nested -
* there the author's flat tag suppresses that heuristic.  Read it through the
* `nests` bit (false here), which the traits consult.
*
*   PORTABILITY:
*   C++11 baseline.
*
*
* path:      /inc/djinterp/core/meta/flat.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.29
*                                                            revised: 2026.10.03
*******************************************************************************/

#ifndef DJINTERP_META_FLAT_HPP
#define DJINTERP_META_FLAT_HPP 1

// std
#include <cstddef>
// djinterp
#include "../../djinterp.hpp"   // NS_*


NS_DJINTERP


// flat
//   tag: the structural kind of a container that holds only leaves (depth 1).
struct flat
{
    static D_CONSTEXPR_VAR bool        nests     = false;   // no node summand
    static D_CONSTEXPR_VAR std::size_t min_depth = 1;       // every component a
                                                            // leaf

    // name
    //   function: the kind's stable spelling (a function, not a data member, so
    // taking the name never requires an out-of-line definition).
    static D_CONSTEXPR const char* name() D_NOEXCEPT { return "flat"; }
};


NS_END  // djinterp


#endif  // DJINTERP_META_FLAT_HPP
