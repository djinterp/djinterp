/*******************************************************************************
* djinterp [testing]             container_multiplicity_traits_tests_signals.cpp
*
*   Section I - the structural signals the verdict is read from: the
* countability guard (a value_type AND a const size()), the closed-interval
* signal (static lower_bound / upper_bound), the unique-insert signal (a single-
* element insert returning a `.second`), and the opt-in `multiplicity` constant.
*
*   Two library facts drive the edges: std::forward_list has NO size(), so it
* fails the countability guard despite carrying a value_type; and a std::multiset
* insert returns a bare iterator (no `.second`), so it fails the unique-insert
* signal while a std::set (pair<iterator,bool>) passes it.
*
*
* path:      /tests/djinterp/core/container/traits/container_multiplicity_traits_tests_signals.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_multiplicity_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_is_countable_container
//   a value_type AND a const-callable size(); both halves required.
bool
tests_is_countable_container()
{
    bool _cmp_ok = true;

    // value_type + size(): countable
    D_CMP_CHECK( (is_countable_container<std::vector<int>>::value) );
    D_CMP_CHECK( (is_countable_container<std::deque<int>>::value) );
    D_CMP_CHECK( (is_countable_container<std::list<int>>::value) );
    D_CMP_CHECK( (is_countable_container<std::array<int, 4>>::value) );
    D_CMP_CHECK( (is_countable_container<std::string>::value) );
    D_CMP_CHECK( (is_countable_container<std::set<int>>::value) );
    D_CMP_CHECK( (is_countable_container<std::multiset<int>>::value) );
    D_CMP_CHECK( (is_countable_container<std::unordered_set<int>>::value) );
    D_CMP_CHECK( (is_countable_container<std::map<int, int>>::value) );
    D_CMP_CHECK( (is_countable_container<interval_seq>::value) );
    D_CMP_CHECK( (is_countable_container<optin_unique>::value) );

    // forward_list carries a value_type but has NO size() -> not countable
    D_CMP_CHECK( (!is_countable_container<std::forward_list<int>>::value) );
    // a keyed type without size() also fails the guard
    D_CMP_CHECK( (!is_countable_container<keyed_no_size>::value) );
    // no value_type at all
    D_CMP_CHECK( (!is_countable_container<int>::value) );

    return _cmp_ok;
}


// tests_has_interval_bounds_signal
//   static lower_bound AND upper_bound (a closed-interval carrier).
bool
tests_has_interval_bounds_signal()
{
    bool _cmp_ok = true;

    D_CMP_CHECK( (has_interval_bounds_signal<interval_seq>::value) );
    D_CMP_CHECK( (has_interval_bounds_signal<interval_keyed>::value) );
    D_CMP_CHECK( (has_interval_bounds_signal<optin_over_interval>::value) );

    D_CMP_CHECK( (!has_interval_bounds_signal<std::vector<int>>::value) );
    D_CMP_CHECK( (!has_interval_bounds_signal<std::set<int>>::value) );
    D_CMP_CHECK( (!has_interval_bounds_signal<optin_unique>::value) );   // opt-in, but no bounds
    D_CMP_CHECK( (!has_interval_bounds_signal<int>::value) );

    return _cmp_ok;
}


// tests_has_unique_insert
//   a single-element insert whose result has a `.second` (unique associative).
bool
tests_has_unique_insert()
{
    bool _cmp_ok = true;

    // pair<iterator,bool> inserts
    D_CMP_CHECK( (has_unique_insert<std::set<int>>::value) );
    D_CMP_CHECK( (has_unique_insert<std::unordered_set<int>>::value) );
    D_CMP_CHECK( (has_unique_insert<std::map<int, int>>::value) );

    // multiset inserts return a bare iterator (no .second)
    D_CMP_CHECK( (!has_unique_insert<std::multiset<int>>::value) );
    D_CMP_CHECK( (!has_unique_insert<std::unordered_multiset<int>>::value) );
    // sequences have no single-argument value insert at all
    D_CMP_CHECK( (!has_unique_insert<std::vector<int>>::value) );
    D_CMP_CHECK( (!has_unique_insert<std::array<int, 4>>::value) );
    D_CMP_CHECK( (!has_unique_insert<int>::value) );

    return _cmp_ok;
}


// tests_has_multiplicity_bound
//   presence of the opt-in static `multiplicity` constant.
bool
tests_has_multiplicity_bound()
{
    bool _cmp_ok = true;

    D_CMP_CHECK( (has_multiplicity_bound<optin_unique>::value) );
    D_CMP_CHECK( (has_multiplicity_bound<optin_bounded>::value) );
    D_CMP_CHECK( (has_multiplicity_bound<optin_unbounded>::value) );
    D_CMP_CHECK( (has_multiplicity_bound<optin_zero>::value) );     // present, even though it declares 0
    D_CMP_CHECK( (has_multiplicity_bound<optin_over_interval>::value) );

    D_CMP_CHECK( (!has_multiplicity_bound<std::vector<int>>::value) );
    D_CMP_CHECK( (!has_multiplicity_bound<std::set<int>>::value) );
    D_CMP_CHECK( (!has_multiplicity_bound<interval_seq>::value) );
    D_CMP_CHECK( (!has_multiplicity_bound<int>::value) );

    return _cmp_ok;
}


NS_END  // testing
NS_END  // djinterp
