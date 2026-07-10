/******************************************************************************
* djinterp [test]                              type_traits_tests_macros_spec.cpp
*
*   Unit tests for the specialization-detection macros, now in
* trait_detect.hpp (re-exported through Section 0.3 of type_traits.hpp):
*     - D_TYPE_TRAIT_IS_SPECIALIZATION_OF_AS   (was D_TRAIT_IS_SPECIALIZATION_OF_AS)
*     - D_TYPE_TRAIT_IS_SPECIALIZATION_OF      (was D_TRAIT_IS_SPECIALIZATION_OF)
*
*   These macros are a distinct family from the SFINAE-detection ones:
* they emit a primary template (inherits std::false_type) plus a partial
* specialization on `TEMPLATE_NAME<_Types...>` (inherits std::true_type
* or a caller-supplied base).  They detect "is this an instance of X<...>?"
* by partial-specialization matching, not by SFINAE on an expression.
*
*   Limitations of the macros (documented in the type_traits.hpp comments,
* and exercised below):
*   - TEMPLATE_NAME must be a class template whose parameters are ALL
*     `typename`-kind.  Templates with non-type or template-template
*     parameters (e.g. std::array<T, N>) cannot be matched.  We confirm
*     that probing a non-matching type kind returns false rather than
*     hard-erroring.
*   - The macro accepts variadic instantiations including the empty pack,
*     so `is_specialization_of<X<>, X>` is true.
*
*   Test matrix:
*   - positive: a non-empty std::tuple<int, char> matches is_tuple
*   - positive: an empty std::tuple<> matches is_tuple (variadic empty)
*   - negative: a non-tuple class (std::vector<int>) does NOT match
*   - negative: a builtin (int) does NOT match
*   - the _AS variant inherits from the supplied INHERIT_EXPR
*
*
* path:      /inc/djinterp/test/type_traits_tests_macros_spec.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/
#include "./type_traits_tests.hpp"


NS_DJINTERP
NS_TEST


// =========================================================================
// I.   D_TYPE_TRAIT_IS_SPECIALIZATION_OF  (compile-time)
// =========================================================================
//   Sugar over _AS that inherits from std::true_type and emits a _v alias.

D_TYPE_TRAIT_IS_SPECIALIZATION_OF(macro_is_tuple, std::tuple)

// positive cases
static_assert(macro_is_tuple<std::tuple<int, char>>::value == true,
              "D_TYPE_TRAIT_IS_SPECIALIZATION_OF: tuple<int, char> -> true");
static_assert(macro_is_tuple<std::tuple<>>::value == true,
              "D_TYPE_TRAIT_IS_SPECIALIZATION_OF: empty tuple -> true (variadic accepts empty)");
static_assert(macro_is_tuple<std::tuple<int>>::value == true,
              "D_TYPE_TRAIT_IS_SPECIALIZATION_OF: single-element tuple -> true");
static_assert(macro_is_tuple<std::tuple<int, char, float, double>>::value == true,
              "D_TYPE_TRAIT_IS_SPECIALIZATION_OF: 4-element tuple -> true");

// negative cases
static_assert(macro_is_tuple<int>::value == false,
              "D_TYPE_TRAIT_IS_SPECIALIZATION_OF: builtin int -> false");
static_assert(macro_is_tuple<std::vector<int>>::value == false,
              "D_TYPE_TRAIT_IS_SPECIALIZATION_OF: vector<int> is not tuple<...> -> false");

// nested-pair edge case: pair<tuple, tuple> is NOT itself a tuple
static_assert(macro_is_tuple<std::pair<std::tuple<int>, std::tuple<char>>>::value == false,
              "D_TYPE_TRAIT_IS_SPECIALIZATION_OF: pair-of-tuples is not tuple -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(macro_is_tuple_v<std::tuple<int>>  == true,
                  "D_TYPE_TRAIT_IS_SPECIALIZATION_OF _v alias (true case)");
    static_assert(macro_is_tuple_v<int>              == false,
                  "D_TYPE_TRAIT_IS_SPECIALIZATION_OF _v alias (false case)");
#endif


// =========================================================================
// II.  D_TYPE_TRAIT_IS_SPECIALIZATION_OF_AS  (compile-time)
// =========================================================================
//   The base macro -- success-case inherits a caller-supplied base.
// Distinguishable base = integral_constant<int, 13>.  As with
// D_TYPE_TRAIT_TRUE_AS, the comma inside the base's template-arg list
// would confuse the preprocessor, so we typedef-alias it first.

using macro_spec_count_13_base = std::integral_constant<int, 13>;

D_TYPE_TRAIT_IS_SPECIALIZATION_OF_AS(macro_is_vector_count_13,
                                std::vector,
                                macro_spec_count_13_base)

// match cases inherit from the supplied base; verify ::value is the
// base's value, not std::true_type's 1.
static_assert(macro_is_vector_count_13<std::vector<int>>::value == 13,
              "D_TYPE_TRAIT_IS_SPECIALIZATION_OF_AS: match inherits supplied base (value=13)");
static_assert(std::is_base_of<macro_spec_count_13_base,
                              macro_is_vector_count_13<std::vector<int>>>::value,
              "D_TYPE_TRAIT_IS_SPECIALIZATION_OF_AS: base type matches INHERIT_EXPR");

// no-match: primary template -> std::false_type
static_assert(macro_is_vector_count_13<int>::value == false,
              "D_TYPE_TRAIT_IS_SPECIALIZATION_OF_AS: non-match ::value is false");
static_assert(std::is_base_of<std::false_type,
                              macro_is_vector_count_13<int>>::value,
              "D_TYPE_TRAIT_IS_SPECIALIZATION_OF_AS: non-match base is std::false_type");

// vector of a different element type still matches -- the macro detects
// "any specialization of vector"
static_assert(macro_is_vector_count_13<std::vector<double>>::value == 13,
              "D_TYPE_TRAIT_IS_SPECIALIZATION_OF_AS: vector<double> also matches");


// =========================================================================
// III. Cross-template hygiene  (compile-time)
// =========================================================================
//   A specialization-of trait for X must NOT fire for Y -- even if Y has
// the same arity or shape.

D_TYPE_TRAIT_IS_SPECIALIZATION_OF(macro_is_pair, std::pair)
D_TYPE_TRAIT_IS_SPECIALIZATION_OF(macro_is_vector, std::vector)

static_assert(macro_is_pair<std::pair<int, char>>::value == true,
              "macro_is_pair: pair<int, char> -> true");
static_assert(macro_is_pair<std::tuple<int, char>>::value == false,
              "macro_is_pair: tuple<int, char> -> false (not pair)");

static_assert(macro_is_vector<std::vector<int>>::value == true,
              "macro_is_vector: vector<int> -> true");
static_assert(macro_is_vector<std::tuple<int>>::value == false,
              "macro_is_vector: tuple<int> -> false (not vector)");


// =========================================================================
// IV.  RUNTIME DRIVER
// =========================================================================

void
type_traits_tests_macros_spec(
    test_handler& _test_handler
)
{
    // ---- D_TYPE_TRAIT_IS_SPECIALIZATION_OF (sugar) ----
    record_assertion(_test_handler, 
        macro_is_tuple<std::tuple<int, char>>::value == true,
        "D_TYPE_TRAIT_IS_SPECIALIZATION_OF: tuple<int,char>");
    record_assertion(_test_handler, 
        macro_is_tuple<std::tuple<>>::value == true,
        "D_TYPE_TRAIT_IS_SPECIALIZATION_OF: empty tuple");
    record_assertion(_test_handler, 
        macro_is_tuple<std::tuple<int>>::value == true,
        "D_TYPE_TRAIT_IS_SPECIALIZATION_OF: single-element tuple");
    record_assertion(_test_handler, 
        macro_is_tuple<int>::value == false,
        "D_TYPE_TRAIT_IS_SPECIALIZATION_OF: builtin int -> false");
    record_assertion(_test_handler, 
        macro_is_tuple<std::vector<int>>::value == false,
        "D_TYPE_TRAIT_IS_SPECIALIZATION_OF: vector -> false");

    // ---- D_TYPE_TRAIT_IS_SPECIALIZATION_OF_AS (base macro) ----
    record_assertion(_test_handler, 
        macro_is_vector_count_13<std::vector<int>>::value == 13,
        "D_TYPE_TRAIT_IS_SPECIALIZATION_OF_AS: match inherits supplied base");
    record_assertion(_test_handler, 
        std::is_base_of<macro_spec_count_13_base,
                        macro_is_vector_count_13<std::vector<int>>>::value,
        "D_TYPE_TRAIT_IS_SPECIALIZATION_OF_AS: base type matches INHERIT_EXPR");
    record_assertion(_test_handler, 
        macro_is_vector_count_13<int>::value == false,
        "D_TYPE_TRAIT_IS_SPECIALIZATION_OF_AS: non-match ::value false");
    record_assertion(_test_handler, 
        std::is_base_of<std::false_type,
                        macro_is_vector_count_13<int>>::value,
        "D_TYPE_TRAIT_IS_SPECIALIZATION_OF_AS: non-match base is false_type");
    record_assertion(_test_handler, 
        macro_is_vector_count_13<std::vector<double>>::value == 13,
        "D_TYPE_TRAIT_IS_SPECIALIZATION_OF_AS: vector<double> also matches");

    // ---- Cross-template hygiene ----
    record_assertion(_test_handler, 
        ( macro_is_pair<std::pair<int, char>>::value     == true &&
          macro_is_pair<std::tuple<int, char>>::value    == false ),
        "Cross hygiene: macro_is_pair fires for pair, not tuple");
    record_assertion(_test_handler, 
        ( macro_is_vector<std::vector<int>>::value       == true &&
          macro_is_vector<std::tuple<int>>::value        == false ),
        "Cross hygiene: macro_is_vector fires for vector, not tuple");

    return;
}


NS_END  // test
NS_END  // djinterp
