/*******************************************************************************
* djinterp [test]                                   concepts_tests_container.cpp
*
*   Tests for concepts.hpp section II.6: container and allocator
* concepts.  Covers: has_value_type, has_size_type, has_iterator,
* has_const_iterator, sizeable, has_max_size_c, bounded_c, allocator_c.
*
*   Edge cases of particular interest:
*     - has_max_size_c requires both a `size_type` nested alias AND a
*       `max_size` static member; either one missing -> not satisfied.
*     - sizeable requires `size_type`, a `size()` method, AND that the
*       method's return type be convertible to std::size_t.
*     - bounded_c is a two-arg concept (type plus a unary trait
*       *template*); the second argument is a `template<typename>
*       typename`, not a concept, because C++20 doesn't permit passing
*       concepts as template-template parameters.  We use
*       std::is_class_v's wrapping trait as the predicate side.
*     - allocator_c is checked against both std::allocator<int> and
*       the suite's minimal_allocator fixture.
*
*
* path:      /tests/djinterp/cpp/meta/concepts_tests_container.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.11.09
*******************************************************************************/

// std
#include <vector>
// djinterp
#include "concepts_tests.hpp"


NS_DJINTERP
NS_TESTING
bool concepts_tests_container()
{
    bool verdict = true;

    using fixtures::sized_container;
    using fixtures::bounded_container;
    using fixtures::no_size_member;
    using fixtures::no_size_type;
    using fixtures::trivial_class;
    using fixtures::minimal_allocator;

    // -------------------------------------------------------------------
    // has_value_type / has_size_type / has_iterator / has_const_iterator
    // -------------------------------------------------------------------

    DT_CONCEPT    (has_value_type_c<sized_container>);
    DT_CONCEPT    (has_value_type_c<std::vector<int>>);
    DT_NOT_CONCEPT(has_value_type_c<trivial_class>);
    DT_NOT_CONCEPT(has_value_type_c<int>);

    DT_CONCEPT    (has_size_type_c<sized_container>);
    DT_CONCEPT    (has_size_type_c<no_size_member>);          // has only size_type
    DT_NOT_CONCEPT(has_size_type_c<no_size_type>);            // has only size()
    DT_NOT_CONCEPT(has_size_type_c<trivial_class>);

    DT_CONCEPT    (has_iterator<sized_container>);
    DT_CONCEPT    (has_iterator<std::vector<int>>);
    DT_NOT_CONCEPT(has_iterator<trivial_class>);

    DT_CONCEPT    (has_const_iterator<sized_container>);
    DT_CONCEPT    (has_const_iterator<std::vector<int>>);
    DT_NOT_CONCEPT(has_const_iterator<trivial_class>);

    // -------------------------------------------------------------------
    // sizeable -- size_type + size() returning a size_type-convertible
    // -------------------------------------------------------------------

    DT_CONCEPT    (sizeable<sized_container>);
    DT_CONCEPT    (sizeable<bounded_container>);
    DT_CONCEPT    (sizeable<std::vector<int>>);

    //   asymmetric negatives - missing either half breaks sizeable
    DT_NOT_CONCEPT(sizeable<no_size_member>);              // no size()
    DT_NOT_CONCEPT(sizeable<no_size_type>);                // no size_type
    DT_NOT_CONCEPT(sizeable<trivial_class>);
    DT_NOT_CONCEPT(sizeable<int>);

    // -------------------------------------------------------------------
    // has_max_size_c -- size_type + static max_size convertible to size_type
    // -------------------------------------------------------------------

    DT_CONCEPT    (has_max_size_c<bounded_container>);
    DT_NOT_CONCEPT(has_max_size_c<sized_container>);       // no max_size
    DT_NOT_CONCEPT(has_max_size_c<no_size_type>);          // no size_type
    DT_NOT_CONCEPT(has_max_size_c<int>);

    // -------------------------------------------------------------------
    // bounded_c -- the predicate-template version
    // -------------------------------------------------------------------
    //   bounded_c<T, UnaryTrait> = UnaryTrait<T>::value && has_max_size_c<T>
    // We pass std::is_class as the predicate.

    DT_CONCEPT    ((bounded_c<bounded_container, std::is_class>));
    DT_NOT_CONCEPT((bounded_c<sized_container,   std::is_class>));   // no max_size
    DT_NOT_CONCEPT((bounded_c<int,               std::is_class>));   // !is_class
    DT_NOT_CONCEPT((bounded_c<bounded_container, std::is_union>));   // !is_union

    // -------------------------------------------------------------------
    // allocator_c
    // -------------------------------------------------------------------

    DT_CONCEPT    (allocator_c<std::allocator<int>>);
    DT_CONCEPT    (allocator_c<std::allocator<double>>);
    DT_CONCEPT    (allocator_c<minimal_allocator<int>>);
    DT_NOT_CONCEPT(allocator_c<int>);
    DT_NOT_CONCEPT(allocator_c<trivial_class>);
    DT_NOT_CONCEPT(allocator_c<sized_container>);                    // no allocate

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
