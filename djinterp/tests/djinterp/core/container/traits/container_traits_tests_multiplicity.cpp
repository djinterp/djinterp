/*******************************************************************************
* djinterp [testing]                     container_traits_tests_multiplicity.cpp
*
*   Section XII - uniqueness and multiplicity.  enforces_uniqueness is the
* structural set-shape test: a key_type WITHOUT a mapped_type.  allows_duplicates
* is its negation.  has_bounded_multiplicity is a valid multiplicity_interval OR
* a multiplicity_min()/multiplicity_max() accessor pair.
*
*   The heuristic's limit is documented here: because enforces_uniqueness looks
* only for key-without-mapped, std::multiset reads TRUE even though it permits
* duplicates (it is structurally indistinguishable from std::set), while a keyed
* value store (std::map) reads FALSE because it also carries a mapped_type.
*
*
* path:      /tests/djinterp/core/container/traits/container_traits_tests_multiplicity.cpp
* link(s):   TBA
* author(s): unit-test suite                                        created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "container_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_enforces_uniqueness
//   key_type present, mapped_type absent.
bool
tests_enforces_uniqueness()
{
    bool _ct_ok = true;

    // sets: key, no mapped
    D_CT_CHECK( (enforces_uniqueness<std::set<int>>::value) );
    D_CT_CHECK( (enforces_uniqueness<std::unordered_set<int>>::value) );
    // structural limit: multiset also reads TRUE (key, no mapped) despite dups
    D_CT_CHECK( (enforces_uniqueness<std::multiset<int>>::value) );

    // maps: key AND mapped -> not "unique" by this structural test
    D_CT_CHECK( (!enforces_uniqueness<std::map<int, int>>::value) );
    D_CT_CHECK( (!enforces_uniqueness<std::multimap<int, int>>::value) );
    // sequences: no key_type
    D_CT_CHECK( (!enforces_uniqueness<std::vector<int>>::value) );
    D_CT_CHECK( (!enforces_uniqueness<std::list<int>>::value) );
    D_CT_CHECK( (!enforces_uniqueness<int>::value) );

    return _ct_ok;
}


// tests_allows_duplicates
//   the exact negation of enforces_uniqueness.
bool
tests_allows_duplicates()
{
    bool _ct_ok = true;

    // maps and sequences allow duplicates
    D_CT_CHECK( (allows_duplicates<std::map<int, int>>::value) );
    D_CT_CHECK( (allows_duplicates<std::vector<int>>::value) );
    D_CT_CHECK( (allows_duplicates<std::list<int>>::value) );
    D_CT_CHECK( (allows_duplicates<int>::value) );

    // sets do not (and multiset shares the set's structural reading)
    D_CT_CHECK( (!allows_duplicates<std::set<int>>::value) );
    D_CT_CHECK( (!allows_duplicates<std::unordered_set<int>>::value) );
    D_CT_CHECK( (!allows_duplicates<std::multiset<int>>::value) );

    // spot-check the negation relation directly
    D_CT_CHECK( (allows_duplicates<std::set<int>>::value != enforces_uniqueness<std::set<int>>::value) );
    D_CT_CHECK( (allows_duplicates<std::map<int, int>>::value != enforces_uniqueness<std::map<int, int>>::value) );

    return _ct_ok;
}


// tests_has_bounded_multiplicity
//   a valid multiplicity_interval OR a multiplicity_min/max accessor pair.
bool
tests_has_bounded_multiplicity()
{
    bool _ct_ok = true;

    // the accessor-pair fixture
    D_CT_CHECK( (has_bounded_multiplicity<mult_accessors>::value) );

    // a bogus (non-interval) multiplicity_interval alias and no accessors -> false
    D_CT_CHECK( (!has_bounded_multiplicity<intervals_bogus>::value) );
    // std containers constrain multiplicity by neither route
    D_CT_CHECK( (!has_bounded_multiplicity<std::set<int>>::value) );
    D_CT_CHECK( (!has_bounded_multiplicity<std::vector<int>>::value) );
    D_CT_CHECK( (!has_bounded_multiplicity<int>::value) );

    return _ct_ok;
}


NS_END  // testing
NS_END  // djinterp
