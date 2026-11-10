/*******************************************************************************
* djinterp [testing]                      container_metadata_tests_detection.cpp
*
*   The detection section of container_metadata's suite: the
* is_container_metadata trait, and equality as a comparison of named entries
* rather than of sequences.
*
*
* path:      /tests/djinterp/cpp/container/container_metadata_tests_detection.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.11.09
*******************************************************************************/
#include "container_metadata_tests.hpp"  // corresponding header


NS_DJINTERP
NS_TESTING

/*
tests_container_metadata_detection
  is_container_metadata.
  Tests the following:
  - true for the default specialisation, for other key and value types and
    for another store
  - true through const, volatile and references (the trait cleans its
    argument)
  - false for a vector of pairs (the default store itself), a std::map, a
    scalar, and a type that carries metadata
  - the _v companion agrees, where the language has inline variables
*/
bool tests_container_metadata_detection()
{
    D_CM_CHECK(is_container_metadata<DCmStrings>::value);
    D_CM_CHECK(is_container_metadata<DCmNumbers>::value);
    D_CM_CHECK(is_container_metadata<DCmListed>::value);

    D_CM_CHECK(is_container_metadata<const DCmStrings>::value);
    D_CM_CHECK(is_container_metadata<volatile DCmStrings>::value);
    D_CM_CHECK(is_container_metadata<DCmStrings&>::value);
    D_CM_CHECK(is_container_metadata<const DCmStrings&>::value);
    D_CM_CHECK(is_container_metadata<DCmStrings&&>::value);

    D_CM_CHECK(!is_container_metadata<DCmStrings::store_type>::value);
    D_CM_CHECK(!is_container_metadata<std::map<std::string,
                                               std::string>>::value);
    D_CM_CHECK(!is_container_metadata<int>::value);
    D_CM_CHECK(!is_container_metadata<DCmCarried>::value);

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    D_CM_CHECK(is_container_metadata_v<DCmStrings>);
    D_CM_CHECK(!is_container_metadata_v<DCmCarried>);
#endif

    return true;
}

/*
tests_container_metadata_equality
  operator== and operator!=.
  Tests the following:
  - two empty stores are equal
  - the same entries in another order are equal: metadata is a set of
    named entries, not a sequence
  - a different size, a different value, or a different key at the same
    size makes them unequal
  - != is the negation of == in every case
*/
bool tests_container_metadata_equality()
{
    const DCmStrings empty_a;
    const DCmStrings empty_b;
    const DCmStrings ab = { { "a", "1" }, { "b", "2" } };
    const DCmStrings ba = { { "b", "2" }, { "a", "1" } };
    const DCmStrings a_only = { { "a", "1" } };
    const DCmStrings ab_other = { { "a", "1" }, { "b", "9" } };
    const DCmStrings ac = { { "a", "1" }, { "c", "2" } };

    D_CM_CHECK(empty_a == empty_b);
    D_CM_CHECK(!(empty_a != empty_b));

    D_CM_CHECK(ab == ba);
    D_CM_CHECK(!(ab != ba));

    D_CM_CHECK(!(ab == a_only));
    D_CM_CHECK(ab != a_only);
    D_CM_CHECK(!(a_only == ab));

    D_CM_CHECK(!(ab == ab_other));
    D_CM_CHECK(ab != ab_other);

    D_CM_CHECK(!(ab == ac));
    D_CM_CHECK(ab != ac);

    D_CM_CHECK(!(ab == empty_a));

    return true;
}

NS_END  // testing
NS_END  // djinterp
