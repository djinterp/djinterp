/*******************************************************************************
* djinterp [testing]             container_multiplicity_traits_tests_verdict.cpp
*
*   Section II - multiplicity_kind_of and multiplicity_bound_of, exercised across
* all five kinds and the full precedence:
*     non-countable            -> unknown
*     opt-in multiplicity      -> the declared bound's kind (authoritative)
*     interval bounds          -> unique
*     comparator-less          -> sequence
*     keyed + unique insert    -> unique
*     keyed, no unique insert  -> unbounded_multiset
*   The precedence is pinned: the opt-in wins over interval bounds
* (optin_over_interval is bounded_multiset, not unique), and interval bounds win
* over the keyed branches (interval_keyed is unique).  Two edges only reachable
* through the opt-in are covered: bounded_multiset (1 < m < inf) and the m = 0
* case that maps back to unknown.  The numeric bound tracks the opt-in verbatim
* where present, else follows from the kind (unique -> 1, everything else -> inf).
*
*
* path:      /tests/djinterp/core/container/traits/container_multiplicity_traits_tests_verdict.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_multiplicity_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_multiplicity_kind_of
//   the resolved kind across every branch and every enum value.
bool
tests_multiplicity_kind_of()
{
    bool _cmp_ok = true;

    // -- sequence: countable, comparator-less --
    D_CMP_CHECK( (multiplicity_kind_of<std::vector<int>>::value == multiplicity_kind::sequence) );
    D_CMP_CHECK( (multiplicity_kind_of<std::deque<int>>::value  == multiplicity_kind::sequence) );
    D_CMP_CHECK( (multiplicity_kind_of<std::list<int>>::value   == multiplicity_kind::sequence) );
    D_CMP_CHECK( (multiplicity_kind_of<std::array<int, 4>>::value == multiplicity_kind::sequence) );
    D_CMP_CHECK( (multiplicity_kind_of<std::string>::value      == multiplicity_kind::sequence) );

    // -- unique: keyed + unique insert, an interval, or an opt-in of 1 --
    D_CMP_CHECK( (multiplicity_kind_of<std::set<int>>::value           == multiplicity_kind::unique) );
    D_CMP_CHECK( (multiplicity_kind_of<std::unordered_set<int>>::value == multiplicity_kind::unique) );
    D_CMP_CHECK( (multiplicity_kind_of<std::map<int, int>>::value      == multiplicity_kind::unique) );
    D_CMP_CHECK( (multiplicity_kind_of<interval_seq>::value            == multiplicity_kind::unique) );  // interval > sequence
    D_CMP_CHECK( (multiplicity_kind_of<interval_keyed>::value          == multiplicity_kind::unique) );  // interval > keyed
    D_CMP_CHECK( (multiplicity_kind_of<optin_unique>::value            == multiplicity_kind::unique) );  // opt-in 1

    // -- unbounded_multiset: keyed, insert returns a bare iterator, or opt-in inf --
    D_CMP_CHECK( (multiplicity_kind_of<std::multiset<int>>::value           == multiplicity_kind::unbounded_multiset) );
    D_CMP_CHECK( (multiplicity_kind_of<std::multimap<int, int>>::value      == multiplicity_kind::unbounded_multiset) );
    D_CMP_CHECK( (multiplicity_kind_of<std::unordered_multiset<int>>::value == multiplicity_kind::unbounded_multiset) );
    D_CMP_CHECK( (multiplicity_kind_of<optin_unbounded>::value              == multiplicity_kind::unbounded_multiset) );

    // -- bounded_multiset: reachable ONLY through the opt-in (1 < m < inf) --
    D_CMP_CHECK( (multiplicity_kind_of<optin_bounded>::value       == multiplicity_kind::bounded_multiset) );
    D_CMP_CHECK( (multiplicity_kind_of<optin_over_interval>::value == multiplicity_kind::bounded_multiset) );  // opt-in > interval

    // -- unknown: not countable, or an opt-in of 0 (below the floor) --
    D_CMP_CHECK( (multiplicity_kind_of<std::forward_list<int>>::value == multiplicity_kind::unknown) );  // no size()
    D_CMP_CHECK( (multiplicity_kind_of<keyed_no_size>::value          == multiplicity_kind::unknown) );  // guard precedes key logic
    D_CMP_CHECK( (multiplicity_kind_of<int>::value                    == multiplicity_kind::unknown) );
    D_CMP_CHECK( (multiplicity_kind_of<optin_zero>::value             == multiplicity_kind::unknown) );  // m = 0 -> unknown

    return _cmp_ok;
}


// tests_multiplicity_kind_companions
//   the ::type carrier and the `_v` value agree with ::value.
bool
tests_multiplicity_kind_companions()
{
    bool _cmp_ok = true;

    // ::type carrier
    D_CMP_CHECK( (multiplicity_kind_of_t<std::vector<int>>::value == multiplicity_kind::sequence) );
    D_CMP_CHECK( (multiplicity_kind_of_t<std::set<int>>::value    == multiplicity_kind::unique) );
    D_CMP_CHECK( (multiplicity_kind_of_t<std::multiset<int>>::value == multiplicity_kind::unbounded_multiset) );
    D_CMP_CHECK( (multiplicity_kind_of_t<int>::value             == multiplicity_kind::unknown) );

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // _v companion
    D_CMP_CHECK( (multiplicity_kind_of_v<std::vector<int>>
                     == multiplicity_kind_of<std::vector<int>>::value) );
    D_CMP_CHECK( (multiplicity_kind_of_v<std::set<int>>       == multiplicity_kind::unique) );
    D_CMP_CHECK( (multiplicity_kind_of_v<optin_bounded>       == multiplicity_kind::bounded_multiset) );
    D_CMP_CHECK( (multiplicity_kind_of_v<optin_unbounded>     == multiplicity_kind::unbounded_multiset) );
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return _cmp_ok;
}


// tests_multiplicity_bound_of
//   the numeric bound m - opt-in verbatim where present, else implied by the kind.
bool
tests_multiplicity_bound_of()
{
    bool _cmp_ok = true;

    // implied by kind: unique -> 1
    D_CMP_CHECK( (multiplicity_bound_of<std::set<int>>::value == unique_multiplicity) );
    D_CMP_CHECK( (multiplicity_bound_of<std::map<int, int>>::value == unique_multiplicity) );
    D_CMP_CHECK( (multiplicity_bound_of<interval_seq>::value  == unique_multiplicity) );   // unique via interval, no opt-in
    D_CMP_CHECK( (multiplicity_bound_of<std::set<int>>::value == 1u) );

    // implied by kind: sequence / unbounded_multiset / unknown -> inf
    D_CMP_CHECK( (multiplicity_bound_of<std::vector<int>>::value   == unbounded_multiplicity) );  // sequence
    D_CMP_CHECK( (multiplicity_bound_of<std::multiset<int>>::value == unbounded_multiplicity) );  // unbounded_multiset
    D_CMP_CHECK( (multiplicity_bound_of<std::forward_list<int>>::value == unbounded_multiplicity) );  // unknown
    D_CMP_CHECK( (multiplicity_bound_of<int>::value               == unbounded_multiplicity) );  // unknown

    // opt-in verbatim: the declared value, whatever it is
    D_CMP_CHECK( (multiplicity_bound_of<optin_unique>::value      == 1u) );
    D_CMP_CHECK( (multiplicity_bound_of<optin_bounded>::value     == 5u) );
    D_CMP_CHECK( (multiplicity_bound_of<optin_over_interval>::value == 5u) );
    D_CMP_CHECK( (multiplicity_bound_of<optin_unbounded>::value   == unbounded_multiplicity) );
    // the opt-in is reported verbatim even when the KIND is unknown (m = 0)
    D_CMP_CHECK( (multiplicity_bound_of<optin_zero>::value        == 0u) );

    return _cmp_ok;
}


NS_END  // testing
NS_END  // djinterp
