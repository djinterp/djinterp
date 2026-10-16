/*******************************************************************************
* djinterp [testing]          iterable_container_traits_tests_classification.cpp
*
*   Section II - the base split.  is_iterable_container requires BOTH halves of a
* traversal (begin AND end); is_non_iterable_container is the positional-access
* case - a value_type (the container-hood marker) with no traversal.
*
*   The two are NOT complements: they are both FALSE for a type that is neither
* container-shaped nor traversable (int, a built-in array, the alias-only fixture),
* which is exactly what the value_type guard buys.  The half-traversal fixtures are
* the sharp edge: begin_only and end_only each fail is_iterable_container (a
* traversal needs both ends) and, carrying a value_type, therefore classify as
* NON-iterable containers.
*
*
* path:      /tests/djinterp/core/container/traits/iterable_container_traits_tests_classification.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "iterable_container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_is_iterable_container
//   both begin() and end() are required.
bool
tests_is_iterable_container()
{
    bool _ic_ok = true;

    // every standard container traverses
    D_IC_CHECK( (is_iterable_container<std::vector<int>>::value) );
    D_IC_CHECK( (is_iterable_container<std::deque<int>>::value) );
    D_IC_CHECK( (is_iterable_container<std::list<int>>::value) );
    D_IC_CHECK( (is_iterable_container<std::forward_list<int>>::value) );
    D_IC_CHECK( (is_iterable_container<std::set<int>>::value) );
    D_IC_CHECK( (is_iterable_container<std::unordered_set<int>>::value) );
    D_IC_CHECK( (is_iterable_container<std::map<int, int>>::value) );
    D_IC_CHECK( (is_iterable_container<std::string>::value) );
    D_IC_CHECK( (is_iterable_container<std::array<int, 4>>::value) );

    // fixtures: both qualifications count, and a value_type is NOT required
    D_IC_CHECK( (is_iterable_container<const_only_iter>::value) );
    D_IC_CHECK( (is_iterable_container<mutable_only_iter>::value) );
    D_IC_CHECK( (is_iterable_container<no_value_type_iter>::value) );  // no value_type, still iterable

    // half a traversal is not a traversal
    D_IC_CHECK( (!is_iterable_container<begin_only>::value) );
    D_IC_CHECK( (!is_iterable_container<end_only>::value) );
    // no traversal at all
    D_IC_CHECK( (!is_iterable_container<positional_only>::value) );
    D_IC_CHECK( (!is_iterable_container<alias_only>::value) );
    D_IC_CHECK( (!is_iterable_container<int>::value) );
    D_IC_CHECK( (!is_iterable_container<int[4]>::value) );             // no member begin/end

    // cv-ref is stripped
    D_IC_CHECK( (is_iterable_container<const std::vector<int>&>::value) );

    return _ic_ok;
}


// tests_is_non_iterable_container
//   container-shaped (a value_type) but with no traversal.
bool
tests_is_non_iterable_container()
{
    bool _ic_ok = true;

    // the canonical case: positional access only
    D_IC_CHECK( (is_non_iterable_container<positional_only>::value) );
    // half-traversals carry a value_type and do not traverse -> non-iterable
    D_IC_CHECK( (is_non_iterable_container<begin_only>::value) );
    D_IC_CHECK( (is_non_iterable_container<end_only>::value) );

    // iterable containers are not non-iterable
    D_IC_CHECK( (!is_non_iterable_container<std::vector<int>>::value) );
    D_IC_CHECK( (!is_non_iterable_container<std::set<int>>::value) );
    D_IC_CHECK( (!is_non_iterable_container<std::array<int, 4>>::value) );
    D_IC_CHECK( (!is_non_iterable_container<const_only_iter>::value) );

    // the value_type guard: neither iterable NOR non-iterable
    D_IC_CHECK( (!is_non_iterable_container<int>::value) );
    D_IC_CHECK( (!is_non_iterable_container<int[4]>::value) );
    D_IC_CHECK( (!is_non_iterable_container<alias_only>::value) );      // aliases, but no value_type
    // iterable AND lacking a value_type -> excluded by both clauses
    D_IC_CHECK( (!is_non_iterable_container<no_value_type_iter>::value) );

    // the two classifications are disjoint, but do NOT partition all types
    D_IC_CHECK( (!(is_iterable_container<positional_only>::value
                   && is_non_iterable_container<positional_only>::value)) );
    D_IC_CHECK( (!is_iterable_container<int>::value
                 && !is_non_iterable_container<int>::value) );          // neither

    return _ic_ok;
}


NS_END  // testing
NS_END  // djinterp
