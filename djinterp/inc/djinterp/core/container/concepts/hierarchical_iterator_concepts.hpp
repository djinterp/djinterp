/******************************************************************************
* djinterp [container]                        hierarchical_iterator_concepts.hpp
*
* C++20 concepts for the HIERARCHICAL / FLAT iterator shapes -- the
* `requires`-facing view of iterator/hierarchical_iterator_traits.hpp. THE
* CONCEPTS ADD NO POLICY. Each is
* exactly its trait, spelled so it can constrain a template instead of gating
* one through enable_if. The trait stays the single source of truth. NAMES.
* meta/concepts.hpp already owns the general type-level concepts (the `_c`
* family), and constexpr_iterator_concepts.hpp the constexpr-iteration ones;
* neither is duplicated here. Where an obvious name is otherwise taken, the
* concept takes a form that cannot collide -- a concept and a class of one name
* in one namespace is a hard redeclaration. PORTABILITY: Gated on C++20 +
* concepts. Below that the header is empty and callers use the `::value` / `_v`
* forms directly. path:
* /inc/djinterp/core/container/iterator/hierarchical_iterator_concepts.hpp
* link(s): TBA author(s): Samuel 'teer' Neal-Blim created: 2026.07.14
* *****************************************************************************/

#ifndef DJINTERP_HIERARCHICAL_ITERATOR_CONCEPTS_
#define DJINTERP_HIERARCHICAL_ITERATOR_CONCEPTS_ 1

// djinterp
#include "../../djinterp.hpp"
#include "../../meta/concepts.hpp"   // D_CONCEPT_FROM_TRAIT
#include "../iterator/hierarchical_iterator_traits.hpp"
#include "../iterator/flat_iterator_traits.hpp"
//   flat side: siblings + leaf test
                                       // (the sibling/leaf surface lives on the
                                       // flat side)


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP
NS_CONTAINER
NS_TRAITS

// ==========================================================================
//  TREE-WALKING SURFACE  (down into the node)
// ==========================================================================


// ChildNavigableIterator
// concept: reaches its children -- a first_child accessor. The vertical move a
// flat iterator does not have.
D_CONCEPT_FROM_TRAIT(ChildNavigableIterator, has_first_child_accessor_v)


// NodeKeyedIterator
// concept: exposes the node's KEY -- position in the tree carries an address,
// not just a value.
D_CONCEPT_FROM_TRAIT(NodeKeyedIterator, has_node_key_accessor_v)


// NodeValuedIterator
//   concept: exposes the node's VALUE -- the payload at this position.
D_CONCEPT_FROM_TRAIT(NodeValuedIterator, has_node_value_accessor_v)


// ==========================================================================
//  SIBLING / LEAF SURFACE  (across and at the fringe)
// ==========================================================================


// SiblingNavigableIterator
//   concept: reaches its siblings -- the horizontal move across one level.
D_CONCEPT_FROM_TRAIT(SiblingNavigableIterator, has_sibling_accessor_v)


// LeafTestableIterator
// concept: can say whether it sits at a leaf -- the base case a tree walk stops
// on.
D_CONCEPT_FROM_TRAIT(LeafTestableIterator, has_is_leaf_method_v)

NS_END  // traits
NS_END  // container
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


#endif  // DJINTERP_HIERARCHICAL_ITERATOR_CONCEPTS_
