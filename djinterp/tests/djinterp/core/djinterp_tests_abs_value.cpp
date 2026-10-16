/*******************************************************************************
* djinterp [core]                                   djinterp_tests_abs_value.cpp
*
*
* path:      /tests/djinterp/core/djinterp_tests_abs_value.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "djinterp_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_abs_value_struct
  Tests the following:
  - abs_value<T,N>::value computes |N| for negative, positive, and zero N
  - the result's underlying type is exactly T (the trait preserves the type)
  - works across several integral types (int, long, long long, short,
    signed char) and passes an unsigned value through unchanged
*/
bool
tests_abs_value_struct()
{
    bool _ok = true;

    // negative inputs -> magnitude
    D_DT_CHECK(_ok, (::djinterp::abs_value<int, -5>::value) == 5);
    D_DT_CHECK(_ok, (::djinterp::abs_value<int, -1>::value) == 1);
    D_DT_CHECK(_ok, (::djinterp::abs_value<long, -123456L>::value) == 123456L);
    D_DT_CHECK(_ok, (::djinterp::abs_value<long long, -9000000000LL>::value) == 9000000000LL);
    D_DT_CHECK(_ok, (::djinterp::abs_value<short, -7>::value) == 7);
    D_DT_CHECK(_ok, (::djinterp::abs_value<signed char, -42>::value) == 42);

    // positive and zero inputs -> identity
    D_DT_CHECK(_ok, (::djinterp::abs_value<int, 5>::value) == 5);
    D_DT_CHECK(_ok, (::djinterp::abs_value<int, 0>::value) == 0);
    D_DT_CHECK(_ok, (::djinterp::abs_value<long long, 9000000000LL>::value) == 9000000000LL);

    // unsigned: condition is never taken, value passes through
    D_DT_CHECK(_ok, (::djinterp::abs_value<unsigned, 7u>::value) == 7u);
    D_DT_CHECK(_ok, (::djinterp::abs_value<unsigned, 0u>::value) == 0u);

    // the member's underlying type is exactly the trait's first parameter
    D_DT_CHECK(_ok, (std::is_same<std::remove_cv_t<decltype(::djinterp::abs_value<int, -5>::value)>, int>::value));
    D_DT_CHECK(_ok, (std::is_same<std::remove_cv_t<decltype(::djinterp::abs_value<long long, -5LL>::value)>, long long>::value));
    D_DT_CHECK(_ok, (std::is_same<std::remove_cv_t<decltype(::djinterp::abs_value<unsigned, 5u>::value)>, unsigned>::value));

    // compile-time contract
    static_assert(::djinterp::abs_value<int, -5>::value == 5, "");
    static_assert(::djinterp::abs_value<int, 0>::value == 0, "");

    return _ok;
}

/*
tests_abs_value_v
  Tests the following:
  - the abs_value_v variable template equals abs_value<T,N>::value
  - magnitude for negative, identity for positive/zero, unsigned pass-through
  - the variable template's underlying type is T
  (compiled out where variable templates are unavailable)
*/
bool
tests_abs_value_v()
{
    bool _ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    D_DT_CHECK(_ok, (::djinterp::abs_value_v<int, -5>) == 5);
    D_DT_CHECK(_ok, (::djinterp::abs_value_v<int, 5>) == 5);
    D_DT_CHECK(_ok, (::djinterp::abs_value_v<int, 0>) == 0);
    D_DT_CHECK(_ok, (::djinterp::abs_value_v<long long, -42LL>) == 42LL);
    D_DT_CHECK(_ok, (::djinterp::abs_value_v<unsigned, 9u>) == 9u);

    // the variable template result matches the struct member
    D_DT_CHECK(_ok, (::djinterp::abs_value_v<int, -123>) == (::djinterp::abs_value<int, -123>::value));

    // type of the variable template is T
    D_DT_CHECK(_ok, (std::is_same<std::remove_cv_t<decltype(::djinterp::abs_value_v<int, -5>)>, int>::value));
    D_DT_CHECK(_ok, (std::is_same<std::remove_cv_t<decltype(::djinterp::abs_value_v<long long, -5LL>)>, long long>::value));

    static_assert(::djinterp::abs_value_v<int, -7> == 7, "");
#else
    (void)_ok;
#endif

    return _ok;
}

/*
tests_abs_value_to_size_t
  Tests the following:
  - abs_value_to_size_t<T,N> yields |N| as a std::size_t
  - magnitude for negative inputs, identity for positive/zero
  - the result type is ALWAYS std::size_t regardless of the source type T
  (compiled out where variable templates are unavailable)
*/
bool
tests_abs_value_to_size_t()
{
    bool _ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    D_DT_CHECK(_ok, (::djinterp::abs_value_to_size_t<int, -5>) == static_cast<std::size_t>(5));
    D_DT_CHECK(_ok, (::djinterp::abs_value_to_size_t<int, 5>) == static_cast<std::size_t>(5));
    D_DT_CHECK(_ok, (::djinterp::abs_value_to_size_t<int, 0>) == static_cast<std::size_t>(0));
    D_DT_CHECK(_ok, (::djinterp::abs_value_to_size_t<long long, -123LL>) == static_cast<std::size_t>(123));
    D_DT_CHECK(_ok, (::djinterp::abs_value_to_size_t<short, -7>) == static_cast<std::size_t>(7));

    // result type is std::size_t for every source integral type
    D_DT_CHECK(_ok, (std::is_same<std::remove_cv_t<decltype(::djinterp::abs_value_to_size_t<int, -5>)>, std::size_t>::value));
    D_DT_CHECK(_ok, (std::is_same<std::remove_cv_t<decltype(::djinterp::abs_value_to_size_t<long long, -5LL>)>, std::size_t>::value));
    D_DT_CHECK(_ok, (std::is_same<std::remove_cv_t<decltype(::djinterp::abs_value_to_size_t<short, -5>)>, std::size_t>::value));

    static_assert(::djinterp::abs_value_to_size_t<int, -7> == static_cast<std::size_t>(7), "");
#else
    (void)_ok;
#endif

    return _ok;
}

NS_END  // testing
NS_END  // djinterp
