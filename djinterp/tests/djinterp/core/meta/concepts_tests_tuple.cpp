/*******************************************************************************
* djinterp [test]                                       concepts_tests_tuple.cpp
*
*   Tests for concepts.hpp section II.4: tuple concepts.
* Covers: is_tuple_c, tuple_like, homogeneous_tuple, empty_tuple,
* nonempty_tuple, single_element_tuple.
*
*   Edge cases of particular interest:
*     - is_tuple_c is documented as accepting cv-qualified tuples - the
*       implementation strips remove_cv before checking the
*       specialization, so `const std::tuple<int>` should match.
*     - tuple_like accepts std::array and std::pair too (they have
*       std::tuple_size / std::get specializations), in addition to
*       std::tuple itself.  This is the broader concept.
*     - homogeneous_tuple<std::tuple<int>> is true vacuously (a one-
*       element pack has no pairs that could disagree).
*     - empty_tuple and nonempty_tuple partition is_tuple_c.
*
*
* path:      /tests/djinterp/core/meta/concepts_tests_tuple.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <array>
#include <utility>
// djinterp
#include "concepts_tests.hpp"


NS_DJINTERP_TEST


void
concepts_tests_tuple(
    test_handler& _test_handler,
    test_option   _options...)
{
    using fixtures::homogeneous_tuple_t;
    using fixtures::heterogeneous_tuple_t;
    using fixtures::empty_tuple_t;
    using fixtures::single_element_tuple_t;
    using fixtures::trivial_class;

    // -------------------------------------------------------------------
    // is_tuple_c -- std::tuple specializations only, cv-stripped
    // -------------------------------------------------------------------

    DT_CONCEPT    (is_tuple_c<empty_tuple_t>);
    DT_CONCEPT    (is_tuple_c<single_element_tuple_t>);
    DT_CONCEPT    (is_tuple_c<homogeneous_tuple_t>);
    DT_CONCEPT    (is_tuple_c<heterogeneous_tuple_t>);

    //   cv-qualifiers must not defeat the concept
    DT_CONCEPT    (is_tuple_c<const heterogeneous_tuple_t>);
    DT_CONCEPT    (is_tuple_c<volatile heterogeneous_tuple_t>);
    DT_CONCEPT    (is_tuple_c<const volatile heterogeneous_tuple_t>);

    //   not tuples: arrays, pairs, plain types
    DT_NOT_CONCEPT(is_tuple_c<int>);
    DT_NOT_CONCEPT(is_tuple_c<trivial_class>);
    //   the extra outer parens are required: the comma inside the
    // std::pair / std::array argument list would otherwise split the
    // expression into multiple macro arguments at the preprocessor
    // level.  Wrapping in `((expr))` makes the comma a sub-expression
    // and the preprocessor sees one argument.
    DT_NOT_CONCEPT((is_tuple_c<std::pair<int, int>>));
    DT_NOT_CONCEPT((is_tuple_c<std::array<int, 3>>));

    // -------------------------------------------------------------------
    // tuple_like -- broader: anything with tuple_size / tuple_element
    // -------------------------------------------------------------------

    DT_CONCEPT    (tuple_like<empty_tuple_t>);
    DT_CONCEPT    (tuple_like<heterogeneous_tuple_t>);
    DT_CONCEPT    ((tuple_like<std::pair<int, double>>));
    DT_CONCEPT    ((tuple_like<std::array<int, 3>>));

    //   cvref does not defeat tuple_like (remove_cvref is applied)
    DT_CONCEPT    (tuple_like<const heterogeneous_tuple_t&>);

    DT_NOT_CONCEPT(tuple_like<int>);
    DT_NOT_CONCEPT(tuple_like<trivial_class>);

    //   structural invariant: every tuple is tuple_like
    DT_CONCEPT(!is_tuple_c<heterogeneous_tuple_t> || tuple_like<heterogeneous_tuple_t>);
    DT_CONCEPT(!is_tuple_c<empty_tuple_t>         || tuple_like<empty_tuple_t>);

    // -------------------------------------------------------------------
    // homogeneous_tuple -- all elements same type
    // -------------------------------------------------------------------

    DT_CONCEPT    (homogeneous_tuple<homogeneous_tuple_t>);
    DT_CONCEPT    (homogeneous_tuple<single_element_tuple_t>);   // vacuously
    DT_CONCEPT    (homogeneous_tuple<const homogeneous_tuple_t>);
    DT_NOT_CONCEPT(homogeneous_tuple<heterogeneous_tuple_t>);
    //   empty tuple is intentionally NOT classified as homogeneous -
    // the trait's "all same" predicate requires at least one type to
    // sample from.  See the helper in concepts.hpp section II.4 -
    // is_tuple_homogeneous_impl only inherits true_type for the
    // single-element and N-element-same specializations, leaving
    // empty as the primary false_type.
    DT_NOT_CONCEPT(homogeneous_tuple<empty_tuple_t>);
    DT_NOT_CONCEPT(homogeneous_tuple<int>);                      // not a tuple

    // -------------------------------------------------------------------
    // empty_tuple -- size 0
    // -------------------------------------------------------------------

    DT_CONCEPT    (empty_tuple<empty_tuple_t>);
    DT_CONCEPT    (empty_tuple<const empty_tuple_t>);
    DT_NOT_CONCEPT(empty_tuple<single_element_tuple_t>);
    DT_NOT_CONCEPT(empty_tuple<heterogeneous_tuple_t>);
    DT_NOT_CONCEPT(empty_tuple<int>);

    // -------------------------------------------------------------------
    // nonempty_tuple -- size > 0
    // -------------------------------------------------------------------

    DT_CONCEPT    (nonempty_tuple<single_element_tuple_t>);
    DT_CONCEPT    (nonempty_tuple<heterogeneous_tuple_t>);
    DT_NOT_CONCEPT(nonempty_tuple<empty_tuple_t>);
    DT_NOT_CONCEPT(nonempty_tuple<int>);

    //   structural invariant: empty_tuple and nonempty_tuple partition
    // is_tuple_c
    DT_CONCEPT(!is_tuple_c<empty_tuple_t>
              || (empty_tuple<empty_tuple_t> != nonempty_tuple<empty_tuple_t>));
    DT_CONCEPT(!is_tuple_c<heterogeneous_tuple_t>
              || (empty_tuple<heterogeneous_tuple_t> != nonempty_tuple<heterogeneous_tuple_t>));

    // -------------------------------------------------------------------
    // single_element_tuple -- size 1
    // -------------------------------------------------------------------

    DT_CONCEPT    (single_element_tuple<single_element_tuple_t>);
    DT_NOT_CONCEPT(single_element_tuple<empty_tuple_t>);
    DT_NOT_CONCEPT(single_element_tuple<heterogeneous_tuple_t>);
    DT_NOT_CONCEPT(single_element_tuple<int>);

    return;
}


NS_END  // test
NS_END  // djinterp
