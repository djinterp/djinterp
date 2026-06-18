/******************************************************************************
* djinterp [test]                                           test_container.hpp
*
*   The test_container contract.  test_container is not a concrete type:
* it is the trait + concept surface that detects whether an arbitrary
* container qualifies to HOLD and RUN test_objects - the test-domain
* analogue of set_traits / set_concepts, expressed the way every other
* container category in the framework is.
*
*   The contract is layered, leanest first:
*   - is_test_object_container    the MINIMUM: a value_type whose element
*                                 satisfies the test object protocol
*                                 (is_test_evaluable), sequential
*                                 traversal (begin/end), and capacity
*                                 (size/empty).  Flat or tree-shaped
*                                 containers alike may qualify.
*   - is_rooted_test_container    adds root(): the container is a forest
*                                 of roots, each the root of a tree of
*                                 test_objects reachable by navigation.
*   - is_buildable_test_container adds append_child(): the read/run
*                                 minimum plus the build surface that
*                                 rank-checked insertion and graft use.
*
*   Any user container meeting the minimum is an acceptable
* test_container; test_tree (test_tree.hpp) is the framework default,
* general-use implementation that satisfies the whole ladder.
*
*   The structural member probes are emitted through the framework
* detection-trait macro DSL (trait_detect.hpp); every `_v` companion is
* feature-gated.  Under C++20 the contract is restated as concepts that
* forward to the same traits, so the two agree by construction.  The
* element protocol check (is_test_evaluable) is reused from
* test_object_traits.hpp.
*
*   This module supersedes the detection role of the retired
* test_tree_traits.hpp / test_tree_concepts.hpp: the genuinely useful
* structural probes are reworked here, and the test_tree-overlay-
* specific surface (underlying_container_type, validate_rank, the
* nary-backing predicates) is dropped.
*
*   PORTABILITY:
*   C++11 minimum.  `_v` companions on C++14+.  Concepts on C++20.
*
*
* TABLE OF CONTENTS
* =================
* I.    STRUCTURAL MEMBER PROBES
* II.   ELEMENT PROTOCOL
* III.  TEST-CONTAINER CONTRACT
* IV.   CONCEPTS
*
*
* path:      /inc/djinterp/test/test_container.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.17
******************************************************************************/

#ifndef DJINTERP_TEST_CONTAINER_
#define DJINTERP_TEST_CONTAINER_ 1

#ifndef __cplusplus
    #error "test_container.hpp requires C++ compilation"
#endif

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../core/djinterp.hpp"
#include "../core/meta/type_traits.hpp"
#include "../core/meta/trait_detect.hpp"
#include "./test_common.hpp"
#include "./test_object_traits.hpp"


#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "test_container.hpp requires C++11 or higher"
#endif


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   STRUCTURAL MEMBER PROBES                             ///
///////////////////////////////////////////////////////////////////////////////

//   The minimal structural members the trait family probes for.  Each
// is emitted through trait_detect.hpp's SFINAE engine, so the `_v`
// companion is generated and feature-gated automatically.  The probes
// query _Type directly; the contract predicates below apply clean_t
// before invoking them, so a const / ref-qualified container agrees
// with its bare form.

// has_value_type
//   trait: true iff `_Type` exposes a nested value_type alias - the
// element type the container holds.
D_TYPE_TRAIT_HAS_TYPE(has_value_type, value_type)

// has_size_accessor
//   trait: true iff `_Type` exposes size() on a const lvalue.
D_TYPE_TRAIT_TRUE(has_size_accessor,
    decltype(std::declval<const _Type&>().size()))

// has_empty_method
//   trait: true iff `_Type` exposes empty() on a const lvalue.
D_TYPE_TRAIT_TRUE(has_empty_method,
    decltype(std::declval<const _Type&>().empty()))

// has_begin_end
//   trait: true iff `_Type` exposes both begin() and end() on a
// non-const lvalue - the sequential traversal a runner walks.
D_TYPE_TRAIT_TRUE(has_begin_end,
    decltype(std::declval<_Type&>().begin()),
    decltype(std::declval<_Type&>().end()))

// has_root_method
//   trait: true iff `_Type` exposes root() on a non-const lvalue - the
// forest entry point each tree of test_objects hangs from.
D_TYPE_TRAIT_TRUE(has_root_method,
    decltype(std::declval<_Type&>().root()))

// has_clear_method
//   trait: true iff `_Type` exposes clear() on a non-const lvalue.
D_TYPE_TRAIT_TRUE(has_clear_method,
    decltype(std::declval<_Type&>().clear()))

// has_append_child_method
//   trait: true iff `_Type` exposes append_child(node_type*,
// value_type) - the build-surface entry point for growing a tree of
// test_objects.
D_TYPE_TRAIT_TRUE(has_append_child_method,
    decltype(std::declval<_Type&>().append_child(
        std::declval<typename _Type::node_type*>(),
        std::declval<typename _Type::value_type>())))


///////////////////////////////////////////////////////////////////////////////
///                II.  ELEMENT PROTOCOL                                     ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // container_element_evaluable
    //   trait: guarded element check.  The primary template (no
    // value_type) is false; the guard keeps the contract predicates
    // well-formed for types that do not expose value_type, where
    // naming _Type::value_type would otherwise be a hard error.
    template<typename _Type,
             bool     _HasValueType>
    struct container_element_evaluable
    {
        static constexpr bool value = false;
    };

    // container_element_evaluable (value_type present)
    //   trait: defers to is_test_evaluable on the element type.
    template<typename _Type>
    struct container_element_evaluable<_Type, true>
    {
        static constexpr bool value =
            is_test_evaluable<typename _Type::value_type>::value;
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                III. TEST-CONTAINER CONTRACT                              ///
///////////////////////////////////////////////////////////////////////////////

// is_test_object_container
//   trait: the MINIMUM contract - true iff `_Type` can hold and run
// test_objects.  Requires a value_type whose element satisfies the
// test object protocol (is_test_evaluable), sequential traversal
// (begin/end), and capacity reporting (size/empty).  This is the lean
// "container of test_objects" bar - tree-shaped or flat.
template<typename _Type>
struct is_test_object_container
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        ( has_value_type<clean_type>::value     &&
          has_begin_end<clean_type>::value      &&
          has_size_accessor<clean_type>::value  &&
          has_empty_method<clean_type>::value   &&
          internal::container_element_evaluable<
              clean_type,
              has_value_type<clean_type>::value>::value );
};

D_TYPE_TRAIT_VALUE_BOOL(is_test_object_container)


// is_rooted_test_container
//   trait: a forest / tree-shaped test_object container - the minimum
// contract PLUS a root() entry point, so each held element may be the
// root of a tree of test_objects walked by child navigation.
template<typename _Type>
struct is_rooted_test_container
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        ( is_test_object_container<clean_type>::value &&
          has_root_method<clean_type>::value );
};

D_TYPE_TRAIT_VALUE_BOOL(is_rooted_test_container)


// is_buildable_test_container
//   trait: a rooted test_object container that can also be GROWN - adds
// the append_child(node_type*, value_type) build surface used by
// rank-checked insertion and graft.  Separates the build capability
// from the read / run minimum above.
template<typename _Type>
struct is_buildable_test_container
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr bool value =
        ( is_rooted_test_container<clean_type>::value &&
          has_append_child_method<clean_type>::value );
};

D_TYPE_TRAIT_VALUE_BOOL(is_buildable_test_container)


///////////////////////////////////////////////////////////////////////////////
///                IV.  CONCEPTS                                             ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

//   The contract restated as C++20 concepts.  Each forwards to the
// trait of the same shape, so a type satisfies the concept iff it
// satisfies the trait - one source of truth.

// value_typed_test_container
//   concept: exposes a value_type alias.
template<typename _Type>
concept value_typed_test_container =
    has_value_type<clean_t<_Type>>::value;

// iterable_test_container
//   concept: exposes begin()/end() for sequential traversal.
template<typename _Type>
concept iterable_test_container =
    has_begin_end<clean_t<_Type>>::value;

// sized_test_container
//   concept: exposes size() and empty().
template<typename _Type>
concept sized_test_container =
    ( has_size_accessor<clean_t<_Type>>::value &&
      has_empty_method<clean_t<_Type>>::value );

// rootable_test_container
//   concept: exposes root().
template<typename _Type>
concept rootable_test_container =
    has_root_method<clean_t<_Type>>::value;

// growable_test_container
//   concept: exposes append_child(node_type*, value_type).
template<typename _Type>
concept growable_test_container =
    has_append_child_method<clean_t<_Type>>::value;


// test_object_container
//   concept: the MINIMUM contract; mirrors is_test_object_container.
template<typename _Type>
concept test_object_container =
    is_test_object_container<clean_t<_Type>>::value;

// rooted_test_container
//   concept: forest / tree-shaped container; mirrors
// is_rooted_test_container.
template<typename _Type>
concept rooted_test_container =
    is_rooted_test_container<clean_t<_Type>>::value;

// buildable_test_container
//   concept: growable container; mirrors is_buildable_test_container.
template<typename _Type>
concept buildable_test_container =
    is_buildable_test_container<clean_t<_Type>>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_CONTAINER_
