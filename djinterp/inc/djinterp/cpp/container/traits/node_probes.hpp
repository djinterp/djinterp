/*******************************************************************************
* djinterp [cpp][container]                                      node_probes.hpp
*
* Node Probes:
*   The eight detection traits that ask a node, or a container of nodes, for
* the tree methods and the node type: parent(), add_child(...), name(...),
* first_child(), children(), child_count(), version(), and a nested
* node_type. Each has one owner, this header, and it needs only decltype and
* std::declval, so its floor is C++11.
*
*   A header with a lower floor than the container traits can then ask the
* same questions they do: text/xml/xml.hpp builds from C++11 and uses these.
* node_container_traits.hpp (C++14), container_traits.hpp and
* nary_tree_traits.hpp (C++17) include it in place of the definitions they
* held.
*
* HOW TO USE
* ----------
*   Include it, or any of the three headers above, and ask a probe:
* has_children_method<T>::value, or has_children_method_v<T> where the
* language has variable templates. has_add_child_method and has_name_method
* take the call's argument types after T, as in
* has_name_method<const Tree, file_node_id>.
*
* OPTIONS
* -------
*   None: nothing in this header is chosen at run time.
*
* CONFIGURATION
* -------------
*   None: no D_CFG_ knob changes it. Below C++11 the header is empty.
*
*
* path:      /inc/djinterp/cpp/container/traits/node_probes.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.17
*                                                            revised: 2026.11.17
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    The Tree Methods
      ----------------
II.   The Node Type
      -------------
*/

#ifndef DJINTERP_CONTAINER_TRAITS_NODE_PROBES_HPP
#define DJINTERP_CONTAINER_TRAITS_NODE_PROBES_HPP 1

// FLOOR: below C++11 this file is empty, rather than an error; its probes
// need decltype and std::declval.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <type_traits>  // std::true_type, std::false_type
#include <utility>      // std::declval
// djinterp
#include "../../meta/trait_detect.hpp"  // D_TYPE_TRAIT_TRUE, _HAS_METHOD_WITH
#include "../../meta/type_utility.hpp"  // clean_t


NS_DJINTERP

// ===========================================================================
// I.   The Tree Methods
// ===========================================================================
//   Each probe is a struct, its value true when the expression is well
// formed, with a _v companion where the language has variable templates.
// Two take the call's argument types at the use, as std::is_invocable takes
// a callable's: add_child and name are called with different arguments by
// different trees (an id and a payload on an arena tree; a parent, name and
// type on a file tree; nothing at all on an xml node).

// has_parent_method
//   trait: true if Type has a callable parent() method, asked of a Type
// rvalue; has_parent_method<const T&> asks it of a const object, which is
// container_traits' has_parent_accessor<T>.
D_TYPE_TRAIT_TRUE(has_parent_method,
    decltype(std::declval<Type>().parent()))

// has_add_child_method
//   trait: has_add_child_method<Type, Args...> is true if
// `add_child(Args...)` can be called on an lvalue Type.
D_TYPE_TRAIT_HAS_METHOD_WITH(has_add_child_method, add_child)

// has_name_method
//   trait: has_name_method<Type, Args...> is true if `name(Args...)` can be
// called on an lvalue Type -- has_name_method<const Tree, file_node_id> for a
// file tree, whose name takes a node id.
D_TYPE_TRAIT_HAS_METHOD_WITH(has_name_method, name)

// has_first_child_method
//   trait: true if Type has a first_child() method, asked of a const object
// (the LCRS model: first_child and next_sibling).
D_TYPE_TRAIT_TRUE(has_first_child_method,
    decltype(std::declval<const Type&>().first_child()))

// has_children_method
//   trait: true if Type has a children() method, asked of a const object (the
// container model: children() returning an iterable).
D_TYPE_TRAIT_TRUE(has_children_method,
    decltype(std::declval<const Type&>().children()))

// has_child_count_method
//   trait: true if Type has a child_count() method, asked of a const object,
// on the container or on a node.
D_TYPE_TRAIT_TRUE(has_child_count_method,
    decltype(std::declval<const Type&>().child_count()))

// has_version_method
//   trait: true if Type has a version() const method returning a version
// stamp: the thread-safe containers' ABA stamp, and the n-ary trees' change
// tracking.
D_TYPE_TRAIT_TRUE(has_version_method,
    decltype(std::declval<const Type&>().version()))


// ===========================================================================
// II.  The Node Type
// ===========================================================================

// has_node_type
//   trait: true if Type, references and cv-qualifiers removed, has a nested
// `node_type` alias. (nary_tree_traits' has_node_type_alias asks it of the
// type as given.)
D_TYPE_TRAIT_TRUE(has_node_type,
    typename clean_t<Type>::node_type)


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER
#endif  // DJINTERP_CONTAINER_TRAITS_NODE_PROBES_HPP
