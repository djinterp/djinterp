/*******************************************************************************
* djinterp [core]                              lifetime_tests_classification.cpp
*
*
* path:      /tests/djinterp/core/meta/lifetime_tests_classification.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "lifetime_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_classification_opt_in_takes_priority
  lifetime_of returns the opt-in value for opt-in types.
  Tests the following:
  - opt_ct classifies as compile_time
  - opt_rt classifies as runtime
  - opt_both classifies as both
  - opt_none classifies as none
*/
bool
tests_classification_opt_in_takes_priority()
{
    D_LT_CHECK(lifetime_of<opt_ct>::value == lifetime::compile_time);
    D_LT_CHECK(lifetime_of<opt_rt>::value == lifetime::runtime);
    D_LT_CHECK(lifetime_of<opt_both>::value == lifetime::both);
    D_LT_CHECK(lifetime_of<opt_none>::value == lifetime::none);

    return true;
}

/*
tests_classification_opt_in_overrides_literal
  The decisive priority case: opt-in beats the literal-type default.
  Tests the following:
  - opt_rt IS structurally a literal type (confirmed via is_literal_type)
  - yet lifetime_of<opt_rt> is runtime, NOT both -- the opt-in overrides
  - opt_none is likewise structurally literal but classifies as none
  - a literal type WITHOUT opt-in, by contrast, does classify as both
*/
bool
tests_classification_opt_in_overrides_literal()
{
    // opt_rt is structurally literal ...
    D_LT_CHECK(is_literal_type<opt_rt>::value);
    // ... but the opt-in wins: runtime, not both
    D_LT_CHECK(lifetime_of<opt_rt>::value == lifetime::runtime);
    D_LT_CHECK(lifetime_of<opt_rt>::value != lifetime::both);

    // opt_none is structurally literal but classifies as none
    D_LT_CHECK(is_literal_type<opt_none>::value);
    D_LT_CHECK(lifetime_of<opt_none>::value == lifetime::none);

    // contrast: a literal type with no opt-in is both
    D_LT_CHECK(lifetime_of<literal_noopt>::value == lifetime::both);

    static_assert(lifetime_of<opt_rt>::value == lifetime::runtime,
                  "opt-in overrides literal at compile time");

    return true;
}

/*
tests_classification_literal_without_opt_in_is_both
  A literal type with no opt-in classifies as both.
  Tests the following:
  - the literal_noopt fixture is both
  - a scalar (int) is both
  - a trivial local struct is both
  - std::array-like triviality classifies as both (a plain aggregate here)
*/
bool
tests_classification_literal_without_opt_in_is_both()
{
    D_LT_CHECK(lifetime_of<literal_noopt>::value == lifetime::both);
    D_LT_CHECK(lifetime_of<int>::value == lifetime::both);
    D_LT_CHECK(lifetime_of<double>::value == lifetime::both);

    struct trivial_local { int a; char b; };
    D_LT_CHECK(lifetime_of<trivial_local>::value == lifetime::both);

    return true;
}

/*
tests_classification_nonliteral_falls_back_to_runtime
  A non-literal type with no opt-in classifies as runtime.
  Tests the following:
  - the nonliteral fixture is runtime
  - std::string is runtime
  - a class holding a std::string is runtime
  - void (not a literal, no opt-in) is runtime
*/
bool
tests_classification_nonliteral_falls_back_to_runtime()
{
    D_LT_CHECK(lifetime_of<nonliteral>::value == lifetime::runtime);
    D_LT_CHECK(lifetime_of<std::string>::value == lifetime::runtime);

    struct holds_string { std::string s; };
    D_LT_CHECK(lifetime_of<holds_string>::value == lifetime::runtime);

    D_LT_CHECK(lifetime_of<void>::value == lifetime::runtime);

    return true;
}

/*
tests_classification_carrier_type
  lifetime_of<T>::type and lifetime_of_t<T> are the carrier for the value.
  Tests the following:
  - lifetime_of<int>::type is both_lifetime
  - lifetime_of<opt_rt>::type is runtime_lifetime
  - lifetime_of_t agrees with ::type
  - the carrier's ::value equals the classified lifetime
*/
bool
tests_classification_carrier_type()
{
    D_LT_CHECK((std::is_same<lifetime_of<int>::type, both_lifetime>::value));
    D_LT_CHECK((std::is_same<lifetime_of<opt_rt>::type, runtime_lifetime>::value));
    D_LT_CHECK((std::is_same<lifetime_of<opt_ct>::type, compile_time_lifetime>::value));
    D_LT_CHECK((std::is_same<lifetime_of<opt_none>::type, none_lifetime>::value));

    // lifetime_of_t agrees with ::type
    D_LT_CHECK((std::is_same<lifetime_of_t<int>, lifetime_of<int>::type>::value));

    // the carrier's value equals the classified lifetime
    D_LT_CHECK(lifetime_of<int>::type::value == lifetime_of<int>::value);
    D_LT_CHECK(lifetime_of<opt_rt>::type::value == lifetime::runtime);

    return true;
}

/*
tests_classification_strips_cv_ref
  lifetime_of strips cv-ref.
  Tests the following:
  - lifetime_of<const opt_ct&> agrees with lifetime_of<opt_ct>
  - lifetime_of<const int&> is both
  - lifetime_of<const std::string&> is runtime
  - rvalue-reference and volatile spellings agree with the unqualified form
*/
bool
tests_classification_strips_cv_ref()
{
    D_LT_CHECK(lifetime_of<const opt_ct&>::value == lifetime_of<opt_ct>::value);
    D_LT_CHECK(lifetime_of<const opt_ct&>::value == lifetime::compile_time);

    D_LT_CHECK(lifetime_of<const int&>::value == lifetime::both);
    D_LT_CHECK(lifetime_of<const std::string&>::value == lifetime::runtime);

    D_LT_CHECK(lifetime_of<opt_rt&&>::value == lifetime::runtime);
    D_LT_CHECK(lifetime_of<volatile int>::value == lifetime::both);

    return true;
}

/*
tests_classification_value_companion
  lifetime_of_v tracks the classifier (C++14+).
  Tests the following:
  - the companion equals ::value for an opt-in type
  - and for a literal type
  - and for a non-literal type
  - it is usable in a constant expression
*/
bool
tests_classification_value_companion()
{
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    D_LT_CHECK(lifetime_of_v<opt_rt> == lifetime_of<opt_rt>::value);
    D_LT_CHECK(lifetime_of_v<int> == lifetime_of<int>::value);
    D_LT_CHECK(lifetime_of_v<std::string> == lifetime_of<std::string>::value);

    static_assert(lifetime_of_v<int> == lifetime::both, "companion is constexpr");
    static_assert(lifetime_of_v<opt_rt> == lifetime::runtime, "override is constexpr");
#else
    D_LT_CHECK(lifetime_of<int>::value == lifetime::both);
#endif

    return true;
}

NS_END  // testing
NS_END  // djinterp
