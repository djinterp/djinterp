/******************************************************************************
* djinterp [test]                        djinterp_header_tests_abs_value.cpp
*
*   Section III.ii tests: abs_value trait, abs_value_v, abs_value_to_size_t.
******************************************************************************/

#include "./djinterp_header_tests.hpp"


NS_DJINTERP
NS_TEST


/*
tests_abs_value
  Verifies the compile-time absolute-value trait.
  Tests the following:
  - the false branch (_N >= 0): positive and zero inputs pass through unchanged
  - the true branch (_N < 0): negative inputs are negated to their magnitude
  - a representative spread of integral types (signed char, short, int, long,
    long long, char, unsigned int, std::size_t)
  - unsigned types take the false branch unconditionally
  - a large-magnitude negative value within range (INT_MIN is intentionally
    NOT tested: negating the most-negative value is undefined behaviour)
  - ::value carries the trait's _Type parameter
*/
bool
tests_abs_value()
{
    bool ok = true;

    // false branch: non-negative inputs pass through.
    static_assert((abs_value<int, 5>::value == 5),
                  "abs_value of a positive must be itself.");
    static_assert((abs_value<int, 0>::value == 0),
                  "abs_value of zero must be zero.");

    // true branch: negative inputs are negated.
    static_assert((abs_value<int, -5>::value == 5),
                  "abs_value of a negative must be its magnitude.");

    // spread of signed integral types, both branches.
    static_assert((abs_value<signed char, -7>::value == 7),
                  "abs_value<signed char> failed.");
    static_assert((abs_value<short, -123>::value == 123),
                  "abs_value<short> failed.");
    static_assert((abs_value<long, -100000L>::value == 100000L),
                  "abs_value<long> failed.");
    static_assert((abs_value<long long, -9000000000LL>::value
                       == 9000000000LL),
                  "abs_value<long long> failed.");

    // unsigned types: always the false branch.
    static_assert((abs_value<unsigned int, 5u>::value == 5u),
                  "abs_value<unsigned> must pass through.");
    static_assert((abs_value<std::size_t,
                             static_cast<std::size_t>(42)>::value == 42),
                  "abs_value<size_t> must pass through.");

    // large in-range negative (one above INT_MIN).
    static_assert((abs_value<int, -2147483647>::value == 2147483647),
                  "abs_value of a large negative failed.");

    // ::value preserves the trait's _Type.
    static_assert(std::is_same<
                      std::remove_const<
                          decltype(abs_value<short, -3>::value)>::type,
                      short>::value,
                  "abs_value<short>::value must be a short.");

    // runtime mirror across both branches.
    ok = ok && (abs_value<int, 5>::value   == 5);
    ok = ok && (abs_value<int, 0>::value   == 0);
    ok = ok && (abs_value<int, -5>::value  == 5);
    ok = ok && (abs_value<long long, -9000000000LL>::value == 9000000000LL);
    ok = ok && (abs_value<unsigned int, 5u>::value == 5u);

    return ok;
}


/*
tests_abs_value_v
  Verifies the abs_value_v variable template (C++14+).
  Tests the following:
  - abs_value_v agrees with abs_value<>::value across both branches and a
    range of integral types
  - the result type is the trait's _Type
  When variable templates are unavailable (pre-C++14), the group is skipped
  and reports success vacuously.
*/
bool
tests_abs_value_v()
{
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    bool ok = true;

    static_assert((abs_value_v<int, -5> == 5),
                  "abs_value_v negative branch failed.");
    static_assert((abs_value_v<int, 8> == 8),
                  "abs_value_v positive branch failed.");
    static_assert((abs_value_v<int, 0> == 0),
                  "abs_value_v zero failed.");
    static_assert((abs_value_v<long, -77L> == 77L),
                  "abs_value_v<long> failed.");
    static_assert((abs_value_v<unsigned int, 9u> == 9u),
                  "abs_value_v<unsigned> failed.");

    // agreement with the trait form.
    static_assert((abs_value_v<short, -3> == abs_value<short, -3>::value),
                  "abs_value_v must equal abs_value<>::value.");

    // the variable's type is the trait's _Type.
    static_assert(std::is_same<
                      std::remove_const<decltype(abs_value_v<int, -5>)>::type,
                      int>::value,
                  "abs_value_v<int, ...> must be an int.");

    ok = ok && (abs_value_v<int, -5> == 5);
    ok = ok && (abs_value_v<int, 8>  == 8);
    ok = ok && (abs_value_v<long, -77L> == 77L);

    return ok;
#else
    // variable templates unavailable on this standard; nothing to exercise.
    return true;
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
}


/*
tests_abs_value_to_size_t
  Verifies the abs_value_to_size_t variable template (C++14+).
  Tests the following:
  - the magnitude of _N is yielded as a std::size_t for both branches
  - the result type is exactly std::size_t regardless of the source _Type
  - a signed source magnitude survives the widening to size_t
  When variable templates are unavailable (pre-C++14), the group is skipped
  and reports success vacuously.
*/
bool
tests_abs_value_to_size_t()
{
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    bool ok = true;

    static_assert((abs_value_to_size_t<int, -5>
                       == static_cast<std::size_t>(5)),
                  "abs_value_to_size_t negative branch failed.");
    static_assert((abs_value_to_size_t<int, 12>
                       == static_cast<std::size_t>(12)),
                  "abs_value_to_size_t positive branch failed.");
    static_assert((abs_value_to_size_t<long long, -9000000000LL>
                       == static_cast<std::size_t>(9000000000LL)),
                  "abs_value_to_size_t<long long> failed.");

    // result is exactly std::size_t.
    static_assert(std::is_same<
                      std::remove_const<
                          decltype(abs_value_to_size_t<int, -5>)>::type,
                      std::size_t>::value,
                  "abs_value_to_size_t must yield a std::size_t.");

    ok = ok && (abs_value_to_size_t<int, -5>
                    == static_cast<std::size_t>(5));
    ok = ok && (abs_value_to_size_t<int, 12>
                    == static_cast<std::size_t>(12));

    return ok;
#else
    return true;
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
}


NS_END  // test
NS_END  // djinterp
