/******************************************************************************
* djinterp [functional]                              predicate_tests_traits.cpp
*
*   Tests for the structural detection traits: is_predicate_and / _or / _xor /
* _not / _nand / _nor, the aggregate is_predicate_combinator, and their _v
* variable-template aliases.
*
*   The detection is purely compile-time, so the bulk of these checks are
* static_assert (which fire at build time). A representative subset is also
* re-checked at runtime so the harness registry reflects the section.
*
*   C++11+ only; under C++98 this entry point compiles to a no-op pass.
*
* path:      /src/functional/predicate_tests_traits.cpp
******************************************************************************/

#include "./predicate_tests.hpp"


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// Concrete combinator types reused across the static checks below.
typedef internal::predicate_and_combinator<is_positive, is_even>  and_t;
typedef internal::predicate_or_combinator<is_positive, is_even>   or_t;
typedef internal::predicate_xor_combinator<is_positive, is_even>  xor_t;
typedef internal::predicate_not_combinator<is_positive>           not_t;
typedef internal::predicate_nand_combinator<is_positive, is_even> nand_t;
typedef internal::predicate_nor_combinator<is_positive, is_even>  nor_t;


// ---- positive structural identification (each trait matches its own) ----
static_assert(is_predicate_and<and_t>::value,   "and_t is a predicate_and");
static_assert(is_predicate_or<or_t>::value,     "or_t is a predicate_or");
static_assert(is_predicate_xor<xor_t>::value,   "xor_t is a predicate_xor");
static_assert(is_predicate_not<not_t>::value,   "not_t is a predicate_not");
static_assert(is_predicate_nand<nand_t>::value, "nand_t is a predicate_nand");
static_assert(is_predicate_nor<nor_t>::value,   "nor_t is a predicate_nor");

// ---- cross-negatives (a trait rejects every other combinator) ----
static_assert(!is_predicate_or<and_t>::value,   "and_t is not a predicate_or");
static_assert(!is_predicate_and<or_t>::value,   "or_t is not a predicate_and");
static_assert(!is_predicate_and<xor_t>::value,  "xor_t is not a predicate_and");
static_assert(!is_predicate_nand<nor_t>::value, "nor_t is not a predicate_nand");
static_assert(!is_predicate_nor<nand_t>::value, "nand_t is not a predicate_nor");
static_assert(!is_predicate_not<and_t>::value,  "and_t is not a predicate_not");

// ---- non-combinator types are rejected by every structural trait ----
static_assert(!is_predicate_and<is_positive>::value, "functor is not and");
static_assert(!is_predicate_or<int>::value,          "int is not or");
static_assert(!is_predicate_not<double>::value,      "double is not not");
static_assert(!is_predicate_combinator<is_positive>::value,
              "bare functor is not a combinator");
static_assert(!is_predicate_combinator<int>::value, "int is not a combinator");
static_assert(!is_predicate_combinator<void>::value,
              "void is not a combinator");

// ---- aggregate trait accepts every combinator kind ----
static_assert(is_predicate_combinator<and_t>::value,  "and is a combinator");
static_assert(is_predicate_combinator<or_t>::value,   "or is a combinator");
static_assert(is_predicate_combinator<xor_t>::value,  "xor is a combinator");
static_assert(is_predicate_combinator<not_t>::value,  "not is a combinator");
static_assert(is_predicate_combinator<nand_t>::value, "nand is a combinator");
static_assert(is_predicate_combinator<nor_t>::value,  "nor is a combinator");

// ---- decay-insensitivity (cv / ref forms answer identically) ----
static_assert(is_predicate_and<const and_t>::value,  "const and_t");
static_assert(is_predicate_and<and_t&>::value,       "and_t& ref");
static_assert(is_predicate_and<const and_t&>::value, "const and_t& ref");
static_assert(is_predicate_combinator<const nor_t&>::value,
              "const nor_t& is a combinator");

// ---- nested combinator types are still detected at the outer layer ----
typedef internal::predicate_not_combinator<and_t> not_of_and_t;
static_assert(is_predicate_not<not_of_and_t>::value,
              "not-of-and is a predicate_not");
static_assert(!is_predicate_and<not_of_and_t>::value,
              "not-of-and is not a predicate_and at the outer layer");
static_assert(is_predicate_combinator<not_of_and_t>::value,
              "nested combinator is still a combinator");

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
// ---- _v variable-template aliases agree with the ::value form ----
static_assert(is_predicate_and_v<and_t>,           "and_v");
static_assert(is_predicate_or_v<or_t>,             "or_v");
static_assert(is_predicate_xor_v<xor_t>,           "xor_v");
static_assert(is_predicate_not_v<not_t>,           "not_v");
static_assert(is_predicate_nand_v<nand_t>,         "nand_v");
static_assert(is_predicate_nor_v<nor_t>,           "nor_v");
static_assert(is_predicate_combinator_v<and_t>,    "combinator_v and");
static_assert(!is_predicate_combinator_v<is_positive>, "combinator_v functor");
static_assert(!is_predicate_and_v<or_t>,           "and_v rejects or_t");
#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


/*
test_predicate_traits
  Tests the structural detection traits.
  Tests the following (a runtime mirror of the compile-time static_asserts
  above, so the harness registry reflects this section):
  - each is_predicate_<op> trait recognizes its own combinator
  - each trait rejects the other combinator kinds and plain functors
  - the aggregate is_predicate_combinator accepts every kind and rejects
    non-combinators
  - decay-insensitivity (cv / ref forms answer identically)
  - the _v aliases agree with the ::value form (C++14+)
  The static_asserts above guarantee these at build time; the checks here
  re-validate at run time for coverage accounting.
*/
std::size_t
test_predicate_traits(
    test_registry& _reg
)
{
    std::size_t before;

    before = _reg.failures();

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // positive identification
    D_TESTING_CHECK(_reg, is_predicate_and<and_t>::value);
    D_TESTING_CHECK(_reg, is_predicate_or<or_t>::value);
    D_TESTING_CHECK(_reg, is_predicate_xor<xor_t>::value);
    D_TESTING_CHECK(_reg, is_predicate_not<not_t>::value);
    D_TESTING_CHECK(_reg, is_predicate_nand<nand_t>::value);
    D_TESTING_CHECK(_reg, is_predicate_nor<nor_t>::value);

    // cross-negatives
    D_TESTING_CHECK(_reg, (is_predicate_or<and_t>::value  == false));
    D_TESTING_CHECK(_reg, (is_predicate_and<or_t>::value  == false));
    D_TESTING_CHECK(_reg, (is_predicate_nor<nand_t>::value == false));

    // non-combinators
    D_TESTING_CHECK(_reg, (is_predicate_and<is_positive>::value == false));
    D_TESTING_CHECK(_reg, (is_predicate_combinator<is_positive>::value
                           == false));
    D_TESTING_CHECK(_reg, (is_predicate_combinator<int>::value == false));

    // aggregate accepts every kind
    D_TESTING_CHECK(_reg, is_predicate_combinator<and_t>::value);
    D_TESTING_CHECK(_reg, is_predicate_combinator<or_t>::value);
    D_TESTING_CHECK(_reg, is_predicate_combinator<xor_t>::value);
    D_TESTING_CHECK(_reg, is_predicate_combinator<not_t>::value);
    D_TESTING_CHECK(_reg, is_predicate_combinator<nand_t>::value);
    D_TESTING_CHECK(_reg, is_predicate_combinator<nor_t>::value);

    // decay-insensitivity
    D_TESTING_CHECK(_reg, is_predicate_and<const and_t&>::value);
    D_TESTING_CHECK(_reg, is_predicate_combinator<const nor_t&>::value);

    // nested
    D_TESTING_CHECK(_reg, is_predicate_not<not_of_and_t>::value);
    D_TESTING_CHECK(_reg, (is_predicate_and<not_of_and_t>::value == false));
#else
    (void)_reg;  // C++98: traits unavailable
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    return (_reg.failures() - before);
}


NS_END  // testing
NS_END  // djinterp
