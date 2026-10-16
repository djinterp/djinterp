/*******************************************************************************
* djinterp [testing]     container_conversion_traits_tests_construction_path.cpp
*
*   Section II - construction-path detection: is_range_constructible (To can be
* built from From's iterator range), is_range_insertable (To accepts the range
* through a two-argument insert), and their union has_constructive_path.
*
*   Two structural facts the standard library pins down and that these tests
* lock in:
*     - the two-argument insert(first, last) is an ASSOCIATIVE-container idiom
*       (set / multiset / map ...); the sequence containers spell insert with a
*       position, so they are range-CONSTRUCTIBLE but not range-INSERTABLE.
*     - std::array is an aggregate: neither range-constructible nor
*       range-insertable.  It is the canonical `has_constructive_path == false`
*       target.
*
*   NUANCE.  These probes test only that the constructor / insert EXPRESSION is
* well-formed at overload resolution - they do not instantiate its body - so a
* range constructor of a container whose element is not actually convertible
* (vector<int> -> vector<vector<int>>, vector<int> -> vector<opaque>) still
* reports a path.  Element compatibility is a SEPARATE concern (Section III);
* the tier logic consults it first.
*
*
* path:      /tests/djinterp/core/container/traits/container_conversion_traits_tests_construction_path.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_conversion_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_range_constructible
//   To(begin, end) - present for every std container with a range constructor;
//   absent for std::array (aggregate) and for the from-not-iterable gate.
bool
tests_range_constructible()
{
    bool _ccv_ok = true;

    // sequence -> sequence / associative: all have range constructors
    D_CCV_CHECK( (is_range_constructible<std::vector<int>, std::vector<int>>::value) );
    D_CCV_CHECK( (is_range_constructible<std::vector<int>, std::list<int>>::value) );
    D_CCV_CHECK( (is_range_constructible<std::vector<int>, std::deque<int>>::value) );
    D_CCV_CHECK( (is_range_constructible<std::vector<int>, std::forward_list<int>>::value) );
    D_CCV_CHECK( (is_range_constructible<std::vector<int>, std::set<int>>::value) );
    D_CCV_CHECK( (is_range_constructible<std::vector<int>, std::multiset<int>>::value) );
    D_CCV_CHECK( (is_range_constructible<std::set<int>,    std::vector<int>>::value) );
    D_CCV_CHECK( (is_range_constructible<std::array<int, 4>, std::vector<int>>::value) );

    // convertible-but-not-same element: the range ctor is still viable
    D_CCV_CHECK( (is_range_constructible<std::vector<int>, std::list<double>>::value) );

    // std::array is an aggregate - NO range constructor (the canonical false)
    D_CCV_CHECK( (!is_range_constructible<std::vector<int>, std::array<int, 4>>::value) );

    // sorted_vec exposes no range constructor
    D_CCV_CHECK( (!is_range_constructible<std::vector<int>, sorted_vec>::value) );

    // from-not-iterable gate: an int has no begin()/end(), so no path off it
    D_CCV_CHECK( (!is_range_constructible<int, std::vector<int>>::value) );

    return _ccv_ok;
}


// tests_range_insertable
//   To.insert(begin, end) - the two-argument insert is an associative idiom;
//   sequence containers (position-argument insert) do NOT match it.
bool
tests_range_insertable()
{
    bool _ccv_ok = true;

    // associative targets accept a bare (first, last) insert
    D_CCV_CHECK( (is_range_insertable<std::vector<int>, std::set<int>>::value) );
    D_CCV_CHECK( (is_range_insertable<std::vector<int>, std::multiset<int>>::value) );
    D_CCV_CHECK( (is_range_insertable<std::list<int>,   std::set<int>>::value) );

    // sequence targets spell insert with a POSITION - no two-argument overload
    D_CCV_CHECK( (!is_range_insertable<std::vector<int>, std::vector<int>>::value) );
    D_CCV_CHECK( (!is_range_insertable<std::vector<int>, std::list<int>>::value) );
    D_CCV_CHECK( (!is_range_insertable<std::vector<int>, std::deque<int>>::value) );

    // std::array has no insert at all
    D_CCV_CHECK( (!is_range_insertable<std::vector<int>, std::array<int, 4>>::value) );

    // from-not-iterable gate
    D_CCV_CHECK( (!is_range_insertable<int, std::set<int>>::value) );

    return _ccv_ok;
}


// tests_has_constructive_path
//   the disjunction: some data-preserving construction of To from From exists.
bool
tests_has_constructive_path()
{
    bool _ccv_ok = true;

    // range-constructible carries the path
    D_CCV_CHECK( (has_constructive_path<std::vector<int>, std::vector<int>>::value) );
    D_CCV_CHECK( (has_constructive_path<std::vector<int>, std::list<int>>::value) );
    D_CCV_CHECK( (has_constructive_path<std::vector<int>, std::deque<int>>::value) );
    D_CCV_CHECK( (has_constructive_path<std::array<int, 4>, std::vector<int>>::value) );

    // range-insertable ALSO carries it (associative target)
    D_CCV_CHECK( (has_constructive_path<std::vector<int>, std::set<int>>::value) );

    // signature-viability, not deep instantiation: a path is reported even when
    // the element type would not actually convert (see the file header)
    D_CCV_CHECK( (has_constructive_path<std::vector<int>, std::vector<opaque>>::value) );
    D_CCV_CHECK( (has_constructive_path<std::vector<int>, std::vector<std::vector<int>>>::value) );

    // neither constructible nor insertable -> no path
    D_CCV_CHECK( (!has_constructive_path<std::vector<int>, std::array<int, 4>>::value) );
    D_CCV_CHECK( (!has_constructive_path<std::vector<int>, sorted_vec>::value) );

    // from-not-iterable gate
    D_CCV_CHECK( (!has_constructive_path<int, std::vector<int>>::value) );

    return _ccv_ok;
}


NS_END  // testing
NS_END  // djinterp
