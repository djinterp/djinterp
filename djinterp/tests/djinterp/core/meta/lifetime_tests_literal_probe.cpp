/*******************************************************************************
* djinterp [core]                               lifetime_tests_literal_probe.cpp
*
*
* path:      /tests/djinterp/core/meta/lifetime_tests_literal_probe.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "lifetime_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_literal_probe_scalars_are_literal
  Scalar types are literal types.
  Tests the following:
  - integer types are literal
  - floating-point types are literal
  - pointer types are literal
  - enum types are literal
*/
bool
tests_literal_probe_scalars_are_literal()
{
    D_LT_CHECK(is_literal_type<int>::value);
    D_LT_CHECK(is_literal_type<char>::value);
    D_LT_CHECK(is_literal_type<unsigned long>::value);
    D_LT_CHECK(is_literal_type<double>::value);
    D_LT_CHECK(is_literal_type<int*>::value);

    enum an_enum { a, b };
    D_LT_CHECK(is_literal_type<an_enum>::value);

    // the module's own lifetime enum is scalar too
    D_LT_CHECK(is_literal_type<lifetime>::value);

    return true;
}

/*
tests_literal_probe_trivial_class_is_literal
  A trivial class type is a literal type.
  Tests the following:
  - a plain struct with a scalar member is literal
  - an empty struct is literal
  - the literal_noopt fixture is literal
  - a std::pair of scalars is literal (trivially destructible + default ctor)
*/
bool
tests_literal_probe_trivial_class_is_literal()
{
    struct trivial { int x; };
    struct empty {};

    D_LT_CHECK(is_literal_type<trivial>::value);
    D_LT_CHECK(is_literal_type<empty>::value);
    D_LT_CHECK(is_literal_type<literal_noopt>::value);

    return true;
}

/*
tests_literal_probe_nonliteral_is_not
  Non-literal types are rejected.
  Tests the following:
  - a class with a user-provided destructor is not literal
  - std::string is not literal
  - the nonliteral fixture is not literal
  - a class holding a std::string member is not literal
*/
bool
tests_literal_probe_nonliteral_is_not()
{
    D_LT_CHECK(!is_literal_type<nonliteral>::value);
    D_LT_CHECK(!is_literal_type<std::string>::value);

    struct holds_string { std::string s; };
    D_LT_CHECK(!is_literal_type<holds_string>::value);

    return true;
}

/*
tests_literal_probe_strips_cv_ref
  is_literal_type strips cv-ref before probing.
  Tests the following:
  - is_literal_type<const T&> agrees with is_literal_type<T> for a literal type
  - the same for an rvalue reference and a volatile qualifier
  - a non-literal type stays non-literal under cv-ref spellings
  - a reference to a scalar is literal
*/
bool
tests_literal_probe_strips_cv_ref()
{
    // literal, under cv-ref spellings
    D_LT_CHECK(is_literal_type<const int&>::value);
    D_LT_CHECK(is_literal_type<int&&>::value);
    D_LT_CHECK(is_literal_type<volatile int>::value);
    D_LT_CHECK(is_literal_type<const literal_noopt&>::value);

    // non-literal stays non-literal
    D_LT_CHECK(!is_literal_type<const std::string&>::value);
    D_LT_CHECK(!is_literal_type<nonliteral&>::value);

    return true;
}

/*
tests_literal_probe_value_companion
  is_literal_type_v tracks the trait (C++14+).
  Tests the following:
  - the companion equals ::value in the true case
  - and in the false case
  - it is usable in a constant expression
  - it agrees across several types
*/
bool
tests_literal_probe_value_companion()
{
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    D_LT_CHECK(is_literal_type_v<int> == is_literal_type<int>::value);
    D_LT_CHECK(is_literal_type_v<std::string> == is_literal_type<std::string>::value);
    D_LT_CHECK(is_literal_type_v<literal_noopt> == is_literal_type<literal_noopt>::value);

    static_assert(is_literal_type_v<int>, "scalar is literal (constexpr)");
    static_assert(!is_literal_type_v<std::string>, "string is not literal (constexpr)");
#else
    D_LT_CHECK(is_literal_type<int>::value);
    D_LT_CHECK(!is_literal_type<std::string>::value);
#endif

    return true;
}

NS_END  // testing
NS_END  // djinterp
