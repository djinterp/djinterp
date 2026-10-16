/*******************************************************************************
* djinterp [testing]          container_multiplicity_traits_tests_predicates.cpp
*
*   Section III - the boolean projections of the kind: is_sequence_container
* (the comparator-less m = inf kind), is_unique_container (m = 1 set semantics),
* and is_multiset_container (either genuine-equivalence multiset, bounded OR
* unbounded).  Each is exercised on a representative of every kind so it is seen
* both true and false, and the unknown kind (forward_list, int) reads false on
* all three.
*
*
* path:      /tests/djinterp/core/container/traits/container_multiplicity_traits_tests_predicates.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_multiplicity_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_is_sequence_container
//   true only for the comparator-less (sequence) kind.
bool
tests_is_sequence_container()
{
    bool _cmp_ok = true;

    // sequences
    D_CMP_CHECK( (is_sequence_container<std::vector<int>>::value) );
    D_CMP_CHECK( (is_sequence_container<std::deque<int>>::value) );
    D_CMP_CHECK( (is_sequence_container<std::list<int>>::value) );
    D_CMP_CHECK( (is_sequence_container<std::array<int, 4>>::value) );
    D_CMP_CHECK( (is_sequence_container<std::string>::value) );

    // not sequences
    D_CMP_CHECK( (!is_sequence_container<std::set<int>>::value) );        // unique
    D_CMP_CHECK( (!is_sequence_container<std::multiset<int>>::value) );   // unbounded_multiset
    D_CMP_CHECK( (!is_sequence_container<interval_seq>::value) );         // unique (interval)
    D_CMP_CHECK( (!is_sequence_container<optin_bounded>::value) );        // bounded_multiset
    D_CMP_CHECK( (!is_sequence_container<std::forward_list<int>>::value) ); // unknown
    D_CMP_CHECK( (!is_sequence_container<int>::value) );                  // unknown

    return _cmp_ok;
}


// tests_is_unique_container
//   true only for the set-semantics (unique, m = 1) kind.
bool
tests_is_unique_container()
{
    bool _cmp_ok = true;

    // unique, reached by the keyed, interval, and opt-in routes
    D_CMP_CHECK( (is_unique_container<std::set<int>>::value) );
    D_CMP_CHECK( (is_unique_container<std::unordered_set<int>>::value) );
    D_CMP_CHECK( (is_unique_container<std::map<int, int>>::value) );
    D_CMP_CHECK( (is_unique_container<interval_seq>::value) );
    D_CMP_CHECK( (is_unique_container<interval_keyed>::value) );
    D_CMP_CHECK( (is_unique_container<optin_unique>::value) );

    // not unique
    D_CMP_CHECK( (!is_unique_container<std::vector<int>>::value) );       // sequence
    D_CMP_CHECK( (!is_unique_container<std::multiset<int>>::value) );     // unbounded_multiset
    D_CMP_CHECK( (!is_unique_container<optin_bounded>::value) );          // bounded_multiset
    D_CMP_CHECK( (!is_unique_container<optin_unbounded>::value) );        // unbounded_multiset
    D_CMP_CHECK( (!is_unique_container<std::forward_list<int>>::value) ); // unknown
    D_CMP_CHECK( (!is_unique_container<int>::value) );                    // unknown

    return _cmp_ok;
}


// tests_is_multiset_container
//   true for EITHER multiset kind (bounded or unbounded).
bool
tests_is_multiset_container()
{
    bool _cmp_ok = true;

    // unbounded and bounded multisets
    D_CMP_CHECK( (is_multiset_container<std::multiset<int>>::value) );
    D_CMP_CHECK( (is_multiset_container<std::multimap<int, int>>::value) );
    D_CMP_CHECK( (is_multiset_container<std::unordered_multiset<int>>::value) );
    D_CMP_CHECK( (is_multiset_container<optin_bounded>::value) );        // bounded_multiset
    D_CMP_CHECK( (is_multiset_container<optin_unbounded>::value) );      // unbounded_multiset

    // not multisets
    D_CMP_CHECK( (!is_multiset_container<std::vector<int>>::value) );    // sequence
    D_CMP_CHECK( (!is_multiset_container<std::set<int>>::value) );       // unique
    D_CMP_CHECK( (!is_multiset_container<interval_seq>::value) );        // unique
    D_CMP_CHECK( (!is_multiset_container<optin_unique>::value) );        // unique
    D_CMP_CHECK( (!is_multiset_container<std::forward_list<int>>::value) ); // unknown
    D_CMP_CHECK( (!is_multiset_container<int>::value) );                 // unknown

    return _cmp_ok;
}


NS_END  // testing
NS_END  // djinterp
