/*******************************************************************************
* djinterp [cpp][meta]                                kv_pair_tests_contract.cpp
*
*   Section III of the kv_pair suite: DKvPair against the key/value
* structural contract meta/kv.hpp detects. The members are named key and
* value so that the contract's readers -- is_kv_pair, kv_key_t, kv_value_t,
* and option/'s entry readers -- see a DKvPair as a pair; from 2026.10.04 to
* 2026.11.11 they were m_key and m_value, and is_kv_pair<DKvPair<...>> was
* false.
*
*
* path:      /tests/djinterp/cpp/meta/kv_pair_tests_contract.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#include "./kv_pair_tests.hpp"  // corresponding header
// std
#include <string>
#include <type_traits>
// djinterp
#include "../../../../inc/djinterp/cpp/meta/kv.hpp"  // is_kv_pair, kv_key_t


NS_DJINTERP
NS_TESTING

/*
tests_contract_is_kv_pair
  DKvPair meets meta/kv.hpp's structural contract.
  Tests the following:
  - has_kv_key and has_kv_value see the key and value members
  - is_kv_pair holds, for scalar and string pairs alike
  - kv_key_t and kv_value_t name the pair's key and value types
*/
bool tests_contract_is_kv_pair()
{
    D_KV_CHECK((has_kv_key<DKvPair<int, long> >::value));
    D_KV_CHECK((has_kv_value<DKvPair<int, long> >::value));
    D_KV_CHECK((is_kv_pair<DKvPair<int, long> >::value));
    D_KV_CHECK((is_kv_pair<DKvPair<std::string, int> >::value));
    D_KV_CHECK((std::is_same<kv_key_t<DKvPair<int, long> >, int>::value));
    D_KV_CHECK((std::is_same<kv_value_t<DKvPair<int, long> >, long>::value));

    return true;
}

/*
tests_contract_members_are_the_contracts
  The contract is read through the members themselves.
  Tests the following:
  - the members are public data members named key and value, reachable by
    pointer to member, as a caller's own row type's are
  - a pair of trivially copyable types is standard layout and trivially
    copyable, the precondition of is_kv_addressable
*/
bool tests_contract_members_are_the_contracts()
{
    typedef DKvPair<int, long> pair_type;

    int pair_type::* const  key_member   = &pair_type::key;
    long pair_type::* const value_member = &pair_type::value;
    pair_type               p(3, 4L);

    D_KV_CHECK(p.*key_member == 3);
    D_KV_CHECK(p.*value_member == 4L);
    D_KV_CHECK((std::is_standard_layout<pair_type>::value));
    D_KV_CHECK((std::is_trivially_copyable<pair_type>::value));
    D_KV_CHECK((is_kv_addressable<pair_type>::value));

    return true;
}

NS_END  // testing
NS_END  // djinterp
