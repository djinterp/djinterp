/*******************************************************************************
* djinterp [core]                     container_copy_merge_traits_tests_copy.cpp
*
*
* path:      /tests/djinterp/core/container/traits/container_copy_merge_traits_tests_copy.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "container_copy_merge_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// ===========================================================================
// II.  Copy
// ===========================================================================
//   Two traits of very different character.  is_copyable_container is a
// detected capability - a conjunction of container-hood and copy-
// constructibility.  copy_preserves_all_axes is a NOTE: an unconditional
// true_type recording a guarantee the spec supplies, detected from nothing.
// The tests keep the two apart, since reading the second as if it were the
// first is the mistake its shape invites.


/*
tests_is_copyable_container
  Verifies the copy capability on carriers of each kind.
  Tests the following:
  - an ordinary copyable container is copyable
  - the standard containers, across the disciplines
  - a container with a deleted copy constructor is not
  - a copyable non-container is not
  - a type that is neither is not
*/
bool
tests_is_copyable_container()
{
    bool ok = true;

    // true
    D_CM_CHECK(is_copyable_container<cm_seq>::value       == true);
    D_CM_CHECK(is_copyable_container<cm_vector>::value    == true);
    D_CM_CHECK(is_copyable_container<cm_array>::value     == true);
    D_CM_CHECK(is_copyable_container<cm_deque>::value     == true);
    D_CM_CHECK(is_copyable_container<cm_list>::value      == true);
    D_CM_CHECK(is_copyable_container<cm_string>::value    == true);
    D_CM_CHECK(is_copyable_container<cm_set>::value       == true);
    D_CM_CHECK(is_copyable_container<cm_multiset>::value  == true);
    D_CM_CHECK(is_copyable_container<cm_map>::value       == true);
    D_CM_CHECK(is_copyable_container<cm_multimap>::value  == true);

    // a deleted copy constructor
    D_CM_CHECK(is_copyable_container<cm_move_only>::value == false);

    // not a container
    D_CM_CHECK(is_copyable_container<cm_copyable_not_container>::value
                   == false);
    D_CM_CHECK(is_copyable_container<cm_pair>::value == false);
    D_CM_CHECK(is_copyable_container<int>::value     == false);
    D_CM_CHECK(is_copyable_container<void>::value    == false);

    return ok;
}

/*
tests_copy_needs_both_conjuncts
  Verifies that neither half of the conjunction is redundant.
  Tests the following:
  - a container that cannot be copied fails on the capability half, with the
    container half holding
  - a copyable type that is not a container fails on the container half, with
    the capability half holding
  - the conjunction re-derived from its two parts on every carrier
  - the two halves are independent, all four combinations occurring
*/
bool
tests_copy_needs_both_conjuncts()
{
    bool ok = true;

    // container, not copyable
    D_CM_CHECK(   ( is_iterable_container<cm_move_only>::value == true )
               && ( std::is_copy_constructible<cm_move_only>::value
                        == false )
               && ( is_copyable_container<cm_move_only>::value == false ));

    // copyable, not a container
    D_CM_CHECK(   ( is_iterable_container<
                        cm_copyable_not_container>::value == false )
               && ( std::is_copy_constructible<
                        cm_copyable_not_container>::value == true )
               && ( is_copyable_container<
                        cm_copyable_not_container>::value == false ));

    // both
    D_CM_CHECK(   ( is_iterable_container<cm_vector>::value == true )
               && ( std::is_copy_constructible<cm_vector>::value == true )
               && ( is_copyable_container<cm_vector>::value == true ));

    // neither
    D_CM_CHECK(   ( is_iterable_container<void>::value == false )
               && ( is_copyable_container<void>::value == false ));

    // the conjunction, re-derived
    D_CM_CHECK(is_copyable_container<cm_vector>::value
                   == (    is_iterable_container<cm_vector>::value
                        && std::is_copy_constructible<cm_vector>::value ));
    D_CM_CHECK(is_copyable_container<cm_move_only>::value
                   == (    is_iterable_container<cm_move_only>::value
                        && std::is_copy_constructible<cm_move_only>::value ));
    D_CM_CHECK(is_copyable_container<cm_pair>::value
                   == (    is_iterable_container<cm_pair>::value
                        && std::is_copy_constructible<cm_pair>::value ));

    return ok;
}

/*
tests_copy_capability_false_positive
  Records a limitation the trait INHERITS from std::is_copy_constructible.

  For a standard container of move-only elements the copy constructor is
  DECLARED unconditionally and becomes ill-formed only when instantiated -
  which happens outside the immediate context, where SFINAE cannot see it.  So
  std::is_copy_constructible reports true, is_copyable_container reports true,
  and an actual copy fails to compile.

  The header's stated contract for this trait is that "a copy of a value is
  producible", and for these types it is not.  The limitation is the standard
  library's rather than this header's, and no type-level probe can do better -
  a hand-written declval-based probe reports true as well - but a caller
  branching on this trait to decide whether to copy will still be wrong.

  Tests the following:
  - the trait is true for a std::vector of move-only elements
  - the underlying standard trait is what makes it so
  - the container half is genuinely satisfied, so the false positive comes
    entirely from the capability half
  - a directly move-only container is still correctly rejected, so the trait
    is not simply blind to move-only-ness - it is blind to it only when hidden
    one level down, inside the element type
*/
bool
tests_copy_capability_false_positive()
{
    bool ok = true;

    // the trait reports copyable
    D_CM_CHECK(is_copyable_container<cm_unique_vector>::value == true);

    // ... because the standard trait does
    D_CM_CHECK(std::is_copy_constructible<cm_unique_vector>::value == true);

    // ... while the element type is plainly not copyable
    D_CM_CHECK(std::is_copy_constructible<std::unique_ptr<int> >::value
                   == false);

    // the container half is genuinely satisfied, so the fault is in the other
    D_CM_CHECK(is_iterable_container<cm_unique_vector>::value == true);

    // a directly move-only container IS caught
    D_CM_CHECK(is_copyable_container<cm_move_only>::value == false);

    // so the two carriers disagree though neither can really be copied
    D_CM_CHECK(is_copyable_container<cm_unique_vector>::value
                   != is_copyable_container<cm_move_only>::value);

    return ok;
}

/*
tests_copy_preserves_all_axes
  Verifies the NOTE trait, and the boundary its shape invites getting wrong.

  copy_preserves_all_axes is std::true_type for every type whatever.  It
  records a guarantee from the spec - that a copy differs from its original in
  identity alone - and detects nothing.  So it is true for types that are not
  containers, and true for containers that cannot be copied at all: it says
  what a copy WOULD preserve, not that one can be made.  Only alongside
  is_copyable_container does it say anything about a given type.

  Tests the following:
  - true for containers of every discipline
  - true for a container that cannot be copied, where is_copyable_container is
    false - the two disagree, which is the point
  - true for types that are not containers at all, including void
  - it derives from std::true_type exactly, so the constant is structural and
    not a conjunction that happens to hold on the carriers tried
*/
bool
tests_copy_preserves_all_axes()
{
    bool ok = true;

    // true across the disciplines
    D_CM_CHECK(copy_preserves_all_axes<cm_vector>::value   == true);
    D_CM_CHECK(copy_preserves_all_axes<cm_set>::value      == true);
    D_CM_CHECK(copy_preserves_all_axes<cm_map>::value      == true);
    D_CM_CHECK(copy_preserves_all_axes<cm_multimap>::value == true);

    // true where the copy capability is FALSE
    D_CM_CHECK(   ( is_copyable_container<cm_move_only>::value == false )
               && ( copy_preserves_all_axes<cm_move_only>::value == true ));

    // true for non-containers
    D_CM_CHECK(copy_preserves_all_axes<int>::value     == true);
    D_CM_CHECK(copy_preserves_all_axes<cm_pair>::value == true);
    D_CM_CHECK(copy_preserves_all_axes<void>::value    == true);
    D_CM_CHECK(copy_preserves_all_axes<
                   cm_copyable_not_container>::value == true);

    // the two traits are not interchangeable
    D_CM_CHECK(copy_preserves_all_axes<int>::value
                   != is_copyable_container<int>::value);

    // structurally the constant, not a coincidence of the carriers
    D_CM_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        copy_preserves_all_axes<int> >::value);
    D_CM_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        copy_preserves_all_axes<void> >::value);
    D_CM_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        copy_preserves_all_axes<cm_move_only> >::value);

    return ok;
}

/*
tests_copy_cv_ref
  Verifies that both copy traits honour the clean_t contract.
  Tests the following:
  - is_copyable_container is identical across all seven cv- and reference-
    qualified spellings, true and false alike
  - a const-qualified spelling does NOT make a copyable container
    uncopyable - the trait strips the qualification before asking, which is
    what makes it a statement about the type rather than about a particular
    handle
  - the note trait is trivially uniform, checked so that a future conditional
    implementation would have to keep it so
*/
bool
tests_copy_cv_ref()
{
    bool ok = true;

    D_CM_CV_REF_AGREE(is_copyable_container, cm_vector,    true);
    D_CM_CV_REF_AGREE(is_copyable_container, cm_seq,       true);
    D_CM_CV_REF_AGREE(is_copyable_container, cm_map,       true);
    D_CM_CV_REF_AGREE(is_copyable_container, cm_move_only, false);
    D_CM_CV_REF_AGREE(is_copyable_container, cm_pair,      false);
    D_CM_CV_REF_AGREE(is_copyable_container,
                      cm_copyable_not_container, false);

    // qualification is stripped, not honoured
    D_CM_CHECK(is_copyable_container<const cm_vector>::value == true);
    D_CM_CHECK(is_copyable_container<const cm_vector&>::value == true);

    D_CM_CV_REF_AGREE(copy_preserves_all_axes, cm_vector,    true);
    D_CM_CV_REF_AGREE(copy_preserves_all_axes, cm_move_only, true);

    return ok;
}

/*
tests_copy_companions
  Verifies the `_v` companions, which the trait_detect macro emits for both of
  these single-parameter traits.
  Tests the following:
  - each companion agrees with its ::value
  - each is usable in a constant expression
  - both traits derive from integral_constant and carry the standard members
  - the block is gated on variable-template support, since below C++14 the
    engine emits nothing and ::value remains the portable spelling
*/
bool
tests_copy_companions()
{
    bool ok = true;

    // the standard metafunction shape
    D_CM_STATIC_CHECK(
        std::is_base_of<std::true_type,
                        is_copyable_container<cm_vector> >::value);
    D_CM_STATIC_CHECK(
        std::is_base_of<std::false_type,
                        is_copyable_container<int> >::value);
    D_CM_STATIC_CHECK(
        std::is_same<is_copyable_container<cm_vector>::value_type,
                     bool>::value);
    D_CM_STATIC_CHECK(
        std::is_same<copy_preserves_all_axes<int>::type,
                     std::true_type>::value);

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    D_CM_STATIC_CHECK(is_copyable_container_v<cm_vector>);
    D_CM_STATIC_CHECK(is_copyable_container_v<int> == false);
    D_CM_STATIC_CHECK(copy_preserves_all_axes_v<int>);

    D_CM_CHECK(is_copyable_container_v<cm_vector>
                   == is_copyable_container<cm_vector>::value);
    D_CM_CHECK(is_copyable_container_v<cm_move_only>
                   == is_copyable_container<cm_move_only>::value);
    D_CM_CHECK(is_copyable_container_v<cm_pair>
                   == is_copyable_container<cm_pair>::value);
    D_CM_CHECK(is_copyable_container_v<const cm_vector&>
                   == is_copyable_container<cm_vector>::value);
    D_CM_CHECK(copy_preserves_all_axes_v<cm_vector>
                   == copy_preserves_all_axes<cm_vector>::value);
    D_CM_CHECK(copy_preserves_all_axes_v<int>
                   == copy_preserves_all_axes<int>::value);

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return ok;
}


NS_END  // testing
NS_END  // djinterp
