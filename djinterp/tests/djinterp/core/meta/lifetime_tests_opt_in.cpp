/*******************************************************************************
* djinterp [core]                                      lifetime_tests_opt_in.cpp
*
*
* path:      /tests/djinterp/core/meta/lifetime_tests_opt_in.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "lifetime_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_opt_in_detects_category_member
  has_lifetime_category detects the opt-in member.
  Tests the following:
  - it is true for a type declaring lifetime_category
  - it is true for each of the four opt-in fixtures
  - the result is a bool trait value
  - it derives from std::true_type in the positive case
*/
bool
tests_opt_in_detects_category_member()
{
    D_LT_CHECK(has_lifetime_category<opt_ct>::value);
    D_LT_CHECK(has_lifetime_category<opt_rt>::value);
    D_LT_CHECK(has_lifetime_category<opt_both>::value);
    D_LT_CHECK(has_lifetime_category<opt_none>::value);

    D_LT_CHECK(std::is_base_of<std::true_type, has_lifetime_category<opt_ct> >::value);

    return true;
}

/*
tests_opt_in_absent_is_false
  has_lifetime_category is false when the member is absent.
  Tests the following:
  - it is false for a scalar
  - it is false for a literal type without the member
  - it is false for a non-literal type without the member
  - it derives from std::false_type in the negative case
*/
bool
tests_opt_in_absent_is_false()
{
    D_LT_CHECK(!has_lifetime_category<int>::value);
    D_LT_CHECK(!has_lifetime_category<literal_noopt>::value);
    D_LT_CHECK(!has_lifetime_category<nonliteral>::value);
    D_LT_CHECK(!has_lifetime_category<std::string>::value);

    D_LT_CHECK(std::is_base_of<std::false_type, has_lifetime_category<int> >::value);

    return true;
}

/*
tests_opt_in_strips_cv_ref
  has_lifetime_category strips cv-ref before probing.
  Tests the following:
  - the qualified forms of an opt-in type still detect the member
  - a reference to an opt-in type detects it
  - a non-opt-in type stays undetected under cv-ref spellings
  - const and rvalue-reference spellings agree with the unqualified form
*/
bool
tests_opt_in_strips_cv_ref()
{
    D_LT_CHECK(has_lifetime_category<const opt_ct&>::value);
    D_LT_CHECK(has_lifetime_category<opt_ct&&>::value);
    D_LT_CHECK(has_lifetime_category<volatile opt_both>::value);

    D_LT_CHECK(has_lifetime_category<const opt_ct&>::value ==
               has_lifetime_category<opt_ct>::value);

    // non-opt-in stays false
    D_LT_CHECK(!has_lifetime_category<const literal_noopt&>::value);

    return true;
}

/*
tests_opt_in_member_reads_declared_value
  The internal category-member reader yields the declared value.
  Tests the following:
  - opt_ct reads compile_time
  - opt_rt reads runtime
  - opt_both reads both
  - opt_none reads none
*/
bool
tests_opt_in_member_reads_declared_value()
{
    D_LT_CHECK(internal::lifetime_category_member<opt_ct>::value ==
               lifetime::compile_time);
    D_LT_CHECK(internal::lifetime_category_member<opt_rt>::value ==
               lifetime::runtime);
    D_LT_CHECK(internal::lifetime_category_member<opt_both>::value ==
               lifetime::both);
    D_LT_CHECK(internal::lifetime_category_member<opt_none>::value ==
               lifetime::none);

    return true;
}

NS_END  // testing
NS_END  // djinterp
