/*******************************************************************************
* djinterp [testing]                container_filter_traits_tests_capability.cpp
*
*   Section I - capability detection: the READ capability (is_filter_source: an
* iterable, typed container), the READ+BUILD capability (is_container_filterable:
* a source that can also gather survivors, by push_back or a single-argument
* insert), the source-without-build case (is_filter_input_only), and the
* container-native filter() primitive (has_native_filter).
*
*   The conjunction in is_filter_source is pinned down from BOTH sides: a type
* with a value_type but no traversal (value_only) and a type with a traversal
* but no value_type (iterable_no_value) are each NOT a source.  Two library
* facts drive the build split: std::forward_list has neither push_back nor a
* one-argument insert (it grows through push_front / insert_after), and
* std::array is a fixed aggregate with no grow operation - so both are sources
* but NOT filterable.  has_native_filter is exercised with a matching primitive
* and a wrong-signature one, confirming the probe is shape-precise.
*
*
* path:      /tests/djinterp/core/container/traits/container_filter_traits_tests_capability.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_filter_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_is_filter_source
//   iterable AND typed (a value_type); the conjunction fails if either half does.
bool
tests_is_filter_source()
{
    bool _cf_ok = true;

    // every standard container is an iterable, typed source
    D_CF_CHECK( (is_filter_source<std::vector<int>>::value) );
    D_CF_CHECK( (is_filter_source<std::deque<int>>::value) );
    D_CF_CHECK( (is_filter_source<std::list<int>>::value) );
    D_CF_CHECK( (is_filter_source<std::forward_list<int>>::value) );
    D_CF_CHECK( (is_filter_source<std::set<int>>::value) );
    D_CF_CHECK( (is_filter_source<std::multiset<int>>::value) );
    D_CF_CHECK( (is_filter_source<std::unordered_set<int>>::value) );
    D_CF_CHECK( (is_filter_source<std::map<int, int>>::value) );
    D_CF_CHECK( (is_filter_source<std::array<int, 4>>::value) );
    D_CF_CHECK( (is_filter_source<std::string>::value) );

    // custom sources
    D_CF_CHECK( (is_filter_source<native_filterable>::value) );
    D_CF_CHECK( (is_filter_source<sorted_seq>::value) );
    D_CF_CHECK( (is_filter_source<native_wrong_sig>::value) );

    // NOT sources: each fails exactly one half of the conjunction
    D_CF_CHECK( (!is_filter_source<int>::value) );                  // neither
    D_CF_CHECK( (!is_filter_source<value_only>::value) );          // typed, but not iterable
    D_CF_CHECK( (!is_filter_source<iterable_no_value>::value) );   // iterable, but untyped

    return _cf_ok;
}


// tests_is_container_filterable
//   a source that can ALSO receive survivors (push_back or single-arg insert).
bool
tests_is_container_filterable()
{
    bool _cf_ok = true;

    // push_back-capable sequences
    D_CF_CHECK( (is_container_filterable<std::vector<int>>::value) );
    D_CF_CHECK( (is_container_filterable<std::deque<int>>::value) );
    D_CF_CHECK( (is_container_filterable<std::list<int>>::value) );
    D_CF_CHECK( (is_container_filterable<std::string>::value) );
    // single-argument-insert associatives
    D_CF_CHECK( (is_container_filterable<std::set<int>>::value) );
    D_CF_CHECK( (is_container_filterable<std::multiset<int>>::value) );
    D_CF_CHECK( (is_container_filterable<std::unordered_set<int>>::value) );
    D_CF_CHECK( (is_container_filterable<std::map<int, int>>::value) );
    // custom filterables
    D_CF_CHECK( (is_container_filterable<native_filterable>::value) );
    D_CF_CHECK( (is_container_filterable<sorted_seq>::value) );
    D_CF_CHECK( (is_container_filterable<native_wrong_sig>::value) );

    // sources with NO grow operation -> not filterable
    D_CF_CHECK( (!is_container_filterable<std::forward_list<int>>::value) );  // push_front / insert_after only
    D_CF_CHECK( (!is_container_filterable<std::array<int, 4>>::value) );      // fixed aggregate

    // non-sources cannot be filterable
    D_CF_CHECK( (!is_container_filterable<int>::value) );
    D_CF_CHECK( (!is_container_filterable<value_only>::value) );
    D_CF_CHECK( (!is_container_filterable<iterable_no_value>::value) );

    return _cf_ok;
}


// tests_is_filter_input_only
//   a source that CANNOT build its own result - exactly forward_list and array
//   among the containers here.
bool
tests_is_filter_input_only()
{
    bool _cf_ok = true;

    // source, but not build-capable
    D_CF_CHECK( (is_filter_input_only<std::forward_list<int>>::value) );
    D_CF_CHECK( (is_filter_input_only<std::array<int, 4>>::value) );

    // filterable containers are NOT input-only (they can build)
    D_CF_CHECK( (!is_filter_input_only<std::vector<int>>::value) );
    D_CF_CHECK( (!is_filter_input_only<std::set<int>>::value) );
    D_CF_CHECK( (!is_filter_input_only<native_filterable>::value) );

    // non-sources are NOT input-only (input-only still requires being a source)
    D_CF_CHECK( (!is_filter_input_only<int>::value) );
    D_CF_CHECK( (!is_filter_input_only<value_only>::value) );
    D_CF_CHECK( (!is_filter_input_only<iterable_no_value>::value) );

    return _cf_ok;
}


// tests_has_native_filter
//   a container's own filter(bool(*)(const value_type&)) member - detected only
//   for the exact shape.
bool
tests_has_native_filter()
{
    bool _cf_ok = true;

    // the matching primitive is detected
    D_CF_CHECK( (has_native_filter<native_filterable>::value) );

    // a filter() of the WRONG signature is NOT the native primitive
    D_CF_CHECK( (!has_native_filter<native_wrong_sig>::value) );

    // no standard container exposes a filter() member
    D_CF_CHECK( (!has_native_filter<std::vector<int>>::value) );
    D_CF_CHECK( (!has_native_filter<std::set<int>>::value) );
    D_CF_CHECK( (!has_native_filter<std::array<int, 4>>::value) );
    D_CF_CHECK( (!has_native_filter<std::forward_list<int>>::value) );
    D_CF_CHECK( (!has_native_filter<sorted_seq>::value) );

    // non-containers, too
    D_CF_CHECK( (!has_native_filter<int>::value) );
    D_CF_CHECK( (!has_native_filter<value_only>::value) );

    return _cf_ok;
}


NS_END  // testing
NS_END  // djinterp
