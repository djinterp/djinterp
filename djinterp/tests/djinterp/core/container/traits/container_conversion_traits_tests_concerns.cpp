/*******************************************************************************
* djinterp [testing]              container_conversion_traits_tests_concerns.cpp
*
*   Section III - the per-concern predicates the tier verdict is assembled
* from, each tested in isolation:
*     - elements_compatible        same-or-convertible value_type (the
*                                  prerequisite of any element-carrying move).
*     - conversion_forgets_content To's native content level is strictly
*                                  COARSER than From's (a quotient: forget order,
*                                  forget multiplicity).
*     - conversion_clamps_capacity From is unbounded, To is bounded (a guarded
*                                  capacity clamp).
*     - conversion_changes_topology From and To sit on opposite sides of the
*                                  flat / hierarchical divide.
*
*   The content hierarchy runs none < set < bag < seq < str.  Two library
* subtleties surface here and are deliberately pinned down:
*     - a container-shaped ELEMENT counts as a nesting level, so
*       std::vector<std::string> and std::array<std::string,4> are HIERARCHICAL.
*     - std::forward_list exposes no size(), so it is not "countable"; its
*       native content level is `none` (rank 0), the coarsest rung - which makes
*       a sequence -> forward_list move a content-forgetting one.
*
*
* path:      /tests/djinterp/core/container/traits/container_conversion_traits_tests_concerns.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_conversion_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_elements_compatible
//   same value_type OR a convertible one; keyed pair elements are neither of a
//   plain element.
bool
tests_elements_compatible()
{
    bool _ccv_ok = true;

    // identical element type
    D_CCV_CHECK( (elements_compatible<std::vector<int>, std::vector<int>>::value) );
    D_CCV_CHECK( (elements_compatible<std::vector<int>, std::list<int>>::value) );
    D_CCV_CHECK( (elements_compatible<std::vector<int>, std::deque<int>>::value) );
    D_CCV_CHECK( (elements_compatible<std::vector<int>, std::set<int>>::value) );

    // convertible element type (either direction)
    D_CCV_CHECK( (elements_compatible<std::vector<int>, std::list<double>>::value) );
    D_CCV_CHECK( (elements_compatible<std::vector<double>, std::vector<int>>::value) );

    // a plain-element sequence and our sorted_vec share int
    D_CCV_CHECK( (elements_compatible<std::vector<int>, sorted_vec>::value) );
    D_CCV_CHECK( (elements_compatible<sorted_vec, std::vector<int>>::value) );

    // no conversion in EITHER direction -> incompatible
    D_CCV_CHECK( (!elements_compatible<std::vector<int>, std::vector<opaque>>::value) );
    D_CCV_CHECK( (!elements_compatible<std::vector<opaque>, std::vector<int>>::value) );

    // a map's pair element is not the same as, nor convertible to, a plain int
    D_CCV_CHECK( (!elements_compatible<std::map<int, int>, std::set<int>>::value) );

    return _ccv_ok;
}


// tests_conversion_forgets_content
//   To native strictly coarser than From native (rank(To) < rank(From)).
bool
tests_conversion_forgets_content()
{
    bool _ccv_ok = true;

    // forget order (seq -> set) and order+multiplicity collapses
    D_CCV_CHECK( (conversion_forgets_content<std::vector<int>, std::set<int>>::value) );
    // forget order only into a multiset (seq -> bag)
    D_CCV_CHECK( (conversion_forgets_content<std::vector<int>, std::multiset<int>>::value) );
    // forget multiplicity (bag -> set): deduplication
    D_CCV_CHECK( (conversion_forgets_content<std::multiset<int>, std::set<int>>::value) );
    // keyed forget multiplicity (multimap -> map)
    D_CCV_CHECK( (conversion_forgets_content<std::multimap<int, int>, std::map<int, int>>::value) );

    // edge: forward_list has no size() -> native level `none` (rank 0), so a
    // sequence -> forward_list move forgets content
    D_CCV_CHECK( (conversion_forgets_content<std::vector<int>, std::forward_list<int>>::value) );

    // moving toward a FINER level does not forget
    D_CCV_CHECK( (!conversion_forgets_content<std::set<int>, std::vector<int>>::value) );
    D_CCV_CHECK( (!conversion_forgets_content<std::set<int>, std::multiset<int>>::value) );
    D_CCV_CHECK( (!conversion_forgets_content<std::map<int, int>, std::multimap<int, int>>::value) );

    // same level forgets nothing
    D_CCV_CHECK( (!conversion_forgets_content<std::vector<int>, std::vector<int>>::value) );
    D_CCV_CHECK( (!conversion_forgets_content<std::deque<int>, std::list<int>>::value) );

    return _ccv_ok;
}


// tests_conversion_clamps_capacity
//   unbounded From narrowed to a bounded To (the only container-side clamp).
bool
tests_conversion_clamps_capacity()
{
    bool _ccv_ok = true;

    // unbounded (vector / deque / list) -> bounded (array): a clamp
    D_CCV_CHECK( (conversion_clamps_capacity<std::vector<int>, std::array<int, 4>>::value) );
    D_CCV_CHECK( (conversion_clamps_capacity<std::deque<int>,  std::array<int, 4>>::value) );
    D_CCV_CHECK( (conversion_clamps_capacity<std::list<int>,   std::array<int, 4>>::value) );

    // bounded From (array) -> unbounded To: no clamp (the bound is being dropped)
    D_CCV_CHECK( (!conversion_clamps_capacity<std::array<int, 4>, std::vector<int>>::value) );

    // bounded -> bounded: From already bounded, so the guard's !bounded(From) fails
    D_CCV_CHECK( (!conversion_clamps_capacity<std::array<int, 4>, std::array<int, 8>>::value) );

    // unbounded -> unbounded: To is not bounded
    D_CCV_CHECK( (!conversion_clamps_capacity<std::vector<int>, std::vector<int>>::value) );
    D_CCV_CHECK( (!conversion_clamps_capacity<std::vector<int>, std::list<int>>::value) );
    D_CCV_CHECK( (!conversion_clamps_capacity<std::set<int>,    std::set<int>>::value) );

    return _ccv_ok;
}


// tests_conversion_changes_topology
//   flat <-> hierarchical crossings (a container-shaped element counts as depth).
bool
tests_conversion_changes_topology()
{
    bool _ccv_ok = true;

    // flat -> hierarchical and back
    D_CCV_CHECK( (conversion_changes_topology<std::vector<int>, std::vector<std::vector<int>>>::value) );
    D_CCV_CHECK( (conversion_changes_topology<std::vector<std::vector<int>>, std::vector<int>>::value) );

    // a string element makes the outer container nest (depth 2)
    D_CCV_CHECK( (conversion_changes_topology<std::vector<int>, std::vector<std::string>>::value) );
    D_CCV_CHECK( (conversion_changes_topology<std::vector<std::string>, std::vector<int>>::value) );

    // even a fixed array of strings is hierarchical
    D_CCV_CHECK( (conversion_changes_topology<std::array<std::string, 4>, std::vector<int>>::value) );

    // both flat -> no crossing
    D_CCV_CHECK( (!conversion_changes_topology<std::vector<int>, std::list<int>>::value) );
    D_CCV_CHECK( (!conversion_changes_topology<std::vector<int>, std::set<int>>::value) );
    D_CCV_CHECK( (!conversion_changes_topology<std::vector<int>, std::forward_list<int>>::value) );

    // both hierarchical -> no crossing
    D_CCV_CHECK( (!conversion_changes_topology<std::vector<std::vector<int>>, std::vector<std::vector<int>>>::value) );

    return _ccv_ok;
}


NS_END  // testing
NS_END  // djinterp
