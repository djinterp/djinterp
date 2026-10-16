/*******************************************************************************
* djinterp [core]                                     lifetime_tests_derived.cpp
*
*
* path:      /tests/djinterp/core/meta/lifetime_tests_derived.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "lifetime_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_derived_is_constexpr_lifetime
  is_constexpr_lifetime is true for constexpr-capable types.
  Tests the following:
  - a literal type (both) is constexpr-capable
  - an opt-in compile_time type is
  - an opt-in both type is
  - a runtime-only type and an opt-in none type are NOT
*/
bool
tests_derived_is_constexpr_lifetime()
{
    D_LT_CHECK(is_constexpr_lifetime<int>::value);            // both -> yes
    D_LT_CHECK(is_constexpr_lifetime<literal_noopt>::value);  // both -> yes
    D_LT_CHECK(is_constexpr_lifetime<opt_ct>::value);         // compile_time -> yes
    D_LT_CHECK(is_constexpr_lifetime<opt_both>::value);       // both -> yes

    D_LT_CHECK(!is_constexpr_lifetime<nonliteral>::value);    // runtime -> no
    D_LT_CHECK(!is_constexpr_lifetime<opt_rt>::value);        // runtime -> no
    D_LT_CHECK(!is_constexpr_lifetime<opt_none>::value);      // none -> no

    return true;
}

/*
tests_derived_is_runtime_only_lifetime
  is_runtime_only_lifetime is true exactly for runtime-only types.
  Tests the following:
  - a non-literal type is runtime-only
  - an opt-in runtime type is runtime-only
  - a literal type (both) is NOT runtime-only
  - an opt-in compile_time and an opt-in none type are NOT runtime-only
*/
bool
tests_derived_is_runtime_only_lifetime()
{
    D_LT_CHECK(is_runtime_only_lifetime<nonliteral>::value);
    D_LT_CHECK(is_runtime_only_lifetime<std::string>::value);
    D_LT_CHECK(is_runtime_only_lifetime<opt_rt>::value);

    D_LT_CHECK(!is_runtime_only_lifetime<int>::value);        // both
    D_LT_CHECK(!is_runtime_only_lifetime<opt_ct>::value);     // compile_time
    D_LT_CHECK(!is_runtime_only_lifetime<opt_both>::value);   // both
    D_LT_CHECK(!is_runtime_only_lifetime<opt_none>::value);   // none

    return true;
}

/*
tests_derived_is_dual_lifetime
  is_dual_lifetime is true exactly for both-stage types.
  Tests the following:
  - a literal type is dual
  - an opt-in both type is dual
  - a compile-time-only and a runtime-only type are NOT dual
  - an opt-in none type is NOT dual
*/
bool
tests_derived_is_dual_lifetime()
{
    D_LT_CHECK(is_dual_lifetime<int>::value);
    D_LT_CHECK(is_dual_lifetime<literal_noopt>::value);
    D_LT_CHECK(is_dual_lifetime<opt_both>::value);

    D_LT_CHECK(!is_dual_lifetime<opt_ct>::value);     // compile_time only
    D_LT_CHECK(!is_dual_lifetime<opt_rt>::value);     // runtime only
    D_LT_CHECK(!is_dual_lifetime<nonliteral>::value); // runtime only
    D_LT_CHECK(!is_dual_lifetime<opt_none>::value);   // none

    return true;
}

/*
tests_derived_predicates_agree_with_classifier
  Each derived predicate equals the matching projection of lifetime_of.
  Tests the following:
  - is_constexpr_lifetime equals is_compile_time(lifetime_of::value)
  - is_runtime_only_lifetime equals is_runtime_only(lifetime_of::value)
  - is_dual_lifetime equals is_both(lifetime_of::value)
  - the agreements hold across the whole fixture battery
*/
bool
tests_derived_predicates_agree_with_classifier()
{
    // A compile-time-checkable helper across representative types.
    #define AGREE(T)                                                              \
        D_LT_CHECK(is_constexpr_lifetime<T>::value ==                             \
                   is_compile_time(lifetime_of<T>::value));                       \
        D_LT_CHECK(is_runtime_only_lifetime<T>::value ==                          \
                   is_runtime_only(lifetime_of<T>::value));                       \
        D_LT_CHECK(is_dual_lifetime<T>::value ==                                  \
                   is_both(lifetime_of<T>::value))

    AGREE(int);
    AGREE(literal_noopt);
    AGREE(nonliteral);
    AGREE(std::string);
    AGREE(opt_ct);
    AGREE(opt_rt);
    AGREE(opt_both);
    AGREE(opt_none);

    #undef AGREE

    return true;
}

/*
tests_derived_trait_shape_and_companions
  The derived traits are proper boolean traits with _v companions.
  Tests the following:
  - each derives from std::integral_constant<bool, ...>
  - value_type is bool
  - the _v companions equal ::value (C++14+)
  - the companions are constant expressions (C++14+)
*/
bool
tests_derived_trait_shape_and_companions()
{
    // integral_constant lineage / value_type
    D_LT_CHECK(std::is_base_of<std::true_type, is_constexpr_lifetime<int> >::value);
    D_LT_CHECK(std::is_base_of<std::true_type, is_dual_lifetime<int> >::value);
    D_LT_CHECK(std::is_base_of<std::true_type, is_runtime_only_lifetime<nonliteral> >::value);
    D_LT_CHECK((std::is_same<is_constexpr_lifetime<int>::value_type, bool>::value));

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    D_LT_CHECK(is_constexpr_lifetime_v<int> == is_constexpr_lifetime<int>::value);
    D_LT_CHECK(is_runtime_only_lifetime_v<nonliteral> ==
               is_runtime_only_lifetime<nonliteral>::value);
    D_LT_CHECK(is_dual_lifetime_v<int> == is_dual_lifetime<int>::value);

    static_assert(is_constexpr_lifetime_v<int>, "companion constexpr");
    static_assert(is_dual_lifetime_v<int>, "companion constexpr");
    static_assert(is_runtime_only_lifetime_v<nonliteral>, "companion constexpr");
#endif

    return true;
}

NS_END  // testing
NS_END  // djinterp
