/******************************************************************************
* djinterp [test]                                type_traits_tests_detection.cpp
*
*   Unit tests for Section 0.2 of type_traits.hpp:
*     - nonesuch (sanity)
*     - detected_or, detected_or_t
*     - detected_t
*     - is_detected, is_detected_v
*     - is_detected_convertible, is_detected_convertible_v
*     - is_detected_exact, is_detected_exact_v
*
*   The detection idiom is the foundation everything else in section 0
* is built on, so coverage here is intentionally thorough:
*
*   - probe a well-formed expression (`typename T::value_type`) to verify
*     the success path returns the deduced type / true_type
*   - probe an ill-formed expression to verify the failure path returns
*     the default / false_type
*   - exercise both `detected_or_t` (custom default) and `detected_t`
*     (nonesuch default)
*   - validate is_detected_convertible's three-way outcome: success and
*     convertible (true), success and not convertible (false), failure
*     (false)
*   - validate is_detected_exact's two-way outcome: exact match (true),
*     not match (false)
*
*   nonesuch is checked structurally — it must be uninstantiable.  We
* check via SFINAE probes that std::is_default_constructible,
* std::is_copy_constructible, and std::is_destructible all return false
* for it.
*
*
* path:      /inc/djinterp/test/type_traits_tests_detection.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/
#include "./type_traits_tests.hpp"


NS_DJINTERP
NS_TEST



namespace
{
// ----- detector subjects -----

// has_value_type
//   probe: well-formed expression — `typename T::value_type`. Used to
// drive `is_detected`, `detected_t`, `detected_or` etc.
template<typename _T>
using has_value_type_expr = typename _T::value_type;

// has_size_member
//   probe: well-formed expression — `decltype(std::declval<T&>().size())`.
template<typename _T>
using has_size_member_expr = decltype(std::declval<_T&>().size());

// with_int_value_type
//   subject class: has `value_type = int`. Drives the SUCCESS path of
// detection idiom probes against has_value_type_expr.
struct with_int_value_type
{
    using value_type = int;
};

// with_long_value_type
//   subject class: has `value_type = long`. Used for
// is_detected_convertible (int -> long is convertible) and
// is_detected_exact (long != int).
struct with_long_value_type
{
    using value_type = long;
};

// without_value_type
//   subject class: deliberately lacks `value_type`. Drives the FAILURE
// path of the probes.
struct without_value_type
{};

// with_unrelated_value_type
//   subject class: has a `value_type` that is NOT convertible to the
// expected type. Drives the conversion-failure branch of
// is_detected_convertible.
struct with_unrelated_value_type
{
    struct opaque {};
    using value_type = opaque;
};

}  // namespace


// =========================================================================
// I.   nonesuch  (compile-time structural checks)
// =========================================================================
//   nonesuch must be uninstantiable. The class is constructed so that
// the compiler refuses to default-construct, copy-construct, destroy,
// or copy-assign it.

static_assert(std::is_default_constructible<nonesuch>::value == false,
              "nonesuch should NOT be default-constructible");
static_assert(std::is_copy_constructible<nonesuch>::value == false,
              "nonesuch should NOT be copy-constructible");
static_assert(std::is_destructible<nonesuch>::value == false,
              "nonesuch should NOT be destructible");
static_assert(std::is_copy_assignable<nonesuch>::value == false,
              "nonesuch should NOT be copy-assignable");


// =========================================================================
// II.  detected_or / detected_or_t  (compile-time)
// =========================================================================

//   detected_or<_Default, _Op, _Args...>
//     - if _Op<_Args...> is well-formed, ::type = _Op<_Args...>,
//                                        ::value_t = std::true_type
//     - else                              ::type = _Default,
//                                        ::value_t = std::false_type

// success path -- value_type is int
static_assert(std::is_same<typename detected_or<float,
                                                 has_value_type_expr,
                                                 with_int_value_type>::type,
                           int>::value,
              "detected_or success: yields int (value_type of subject)");
static_assert(std::is_same<typename detected_or<float,
                                                 has_value_type_expr,
                                                 with_int_value_type>::value_t,
                           std::true_type>::value,
              "detected_or success: value_t is std::true_type");

// failure path -- falls back to the supplied default
static_assert(std::is_same<typename detected_or<float,
                                                 has_value_type_expr,
                                                 without_value_type>::type,
                           float>::value,
              "detected_or failure: yields the supplied default (float)");
static_assert(std::is_same<typename detected_or<float,
                                                 has_value_type_expr,
                                                 without_value_type>::value_t,
                           std::false_type>::value,
              "detected_or failure: value_t is std::false_type");

// detected_or_t alias
static_assert(std::is_same<detected_or_t<float,
                                          has_value_type_expr,
                                          with_int_value_type>,
                           int>::value,
              "detected_or_t alias: success path yields int");
static_assert(std::is_same<detected_or_t<float,
                                          has_value_type_expr,
                                          without_value_type>,
                           float>::value,
              "detected_or_t alias: failure path yields default");


// =========================================================================
// III. detected_t  (compile-time)
// =========================================================================
//   detected_t uses `nonesuch` as its built-in default (i.e. always
// signals failure with a sentinel type).

static_assert(std::is_same<detected_t<has_value_type_expr, with_int_value_type>,
                           int>::value,
              "detected_t success: yields int");

static_assert(std::is_same<detected_t<has_value_type_expr, without_value_type>,
                           nonesuch>::value,
              "detected_t failure: yields nonesuch sentinel");


// =========================================================================
// IV.  is_detected / is_detected_v  (compile-time)
// =========================================================================

static_assert(is_detected<has_value_type_expr, with_int_value_type>::value == true,
              "is_detected: with value_type -> true");
static_assert(is_detected<has_value_type_expr, without_value_type>::value == false,
              "is_detected: without value_type -> false");

static_assert(is_detected<has_size_member_expr,
                          std::vector<int>>::value == true,
              "is_detected: std::vector<int> has .size() -> true");
static_assert(is_detected<has_size_member_expr, int>::value == false,
              "is_detected: int has no .size() -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_detected_v<has_value_type_expr, with_int_value_type> == true,
                  "is_detected_v: matches ::value (true case)");
    static_assert(is_detected_v<has_value_type_expr, without_value_type> == false,
                  "is_detected_v: matches ::value (false case)");
#endif


// =========================================================================
// V.   is_detected_convertible / is_detected_convertible_v  (compile-time)
// =========================================================================
//   Three-way logic: (1) probe success + convertible, (2) probe success
// + NOT convertible, (3) probe failure (counts as false because
// `nonesuch` cannot be converted to anything useful).

// success + convertible (int -> long)
static_assert(is_detected_convertible<long,
                                       has_value_type_expr,
                                       with_int_value_type>::value == true,
              "is_detected_convertible: int probed, convertible to long -> true");

// success + convertible (int -> int)
static_assert(is_detected_convertible<int,
                                       has_value_type_expr,
                                       with_int_value_type>::value == true,
              "is_detected_convertible: int probed, convertible to int -> true");

// success + NOT convertible (opaque -> long)
static_assert(is_detected_convertible<long,
                                       has_value_type_expr,
                                       with_unrelated_value_type>::value == false,
              "is_detected_convertible: opaque probed, not convertible to long -> false");

// failure (probe ill-formed) -- nonesuch is not convertible to anything
static_assert(is_detected_convertible<long,
                                       has_value_type_expr,
                                       without_value_type>::value == false,
              "is_detected_convertible: probe failure -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_detected_convertible_v<long,
                                             has_value_type_expr,
                                             with_int_value_type> == true,
                  "is_detected_convertible_v: matches ::value (true)");
    static_assert(is_detected_convertible_v<long,
                                             has_value_type_expr,
                                             without_value_type> == false,
                  "is_detected_convertible_v: matches ::value (false)");
#endif


// =========================================================================
// VI.  is_detected_exact / is_detected_exact_v  (compile-time)
// =========================================================================

// success + exact (int == int)
static_assert(is_detected_exact<int,
                                 has_value_type_expr,
                                 with_int_value_type>::value == true,
              "is_detected_exact: int matches int -> true");

// success + NOT exact (long != int)
static_assert(is_detected_exact<long,
                                 has_value_type_expr,
                                 with_int_value_type>::value == false,
              "is_detected_exact: long does not match int -> false");

// failure -- nonesuch != int
static_assert(is_detected_exact<int,
                                 has_value_type_expr,
                                 without_value_type>::value == false,
              "is_detected_exact: probe failure (nonesuch != int) -> false");

// exact with cv-qualified expected -- strict type-identity
static_assert(is_detected_exact<const int,
                                 has_value_type_expr,
                                 with_int_value_type>::value == false,
              "is_detected_exact: const int does not match int -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_detected_exact_v<int,
                                       has_value_type_expr,
                                       with_int_value_type> == true,
                  "is_detected_exact_v: matches ::value (true)");
    static_assert(is_detected_exact_v<long,
                                       has_value_type_expr,
                                       with_int_value_type> == false,
                  "is_detected_exact_v: matches ::value (false)");
#endif


// =========================================================================
// VII. RUNTIME DRIVER
// =========================================================================

void
type_traits_tests_detection(
    test_handler& _test_handler
)
{
    // ---- nonesuch ----
    record_assertion(_test_handler, 
        ( std::is_default_constructible<nonesuch>::value == false &&
          std::is_copy_constructible   <nonesuch>::value == false &&
          std::is_destructible         <nonesuch>::value == false &&
          std::is_copy_assignable      <nonesuch>::value == false ),
        "nonesuch: uninstantiable (all five special members rejected)");

    // ---- detected_or / detected_or_t ----
    record_assertion(_test_handler, 
        std::is_same<typename detected_or<float, has_value_type_expr,
                                          with_int_value_type>::type, int>::value,
        "detected_or: success -> int");
    record_assertion(_test_handler, 
        std::is_same<typename detected_or<float, has_value_type_expr,
                                          with_int_value_type>::value_t,
                     std::true_type>::value,
        "detected_or: success value_t is true_type");
    record_assertion(_test_handler, 
        std::is_same<typename detected_or<float, has_value_type_expr,
                                          without_value_type>::type, float>::value,
        "detected_or: failure -> supplied default");
    record_assertion(_test_handler, 
        std::is_same<typename detected_or<float, has_value_type_expr,
                                          without_value_type>::value_t,
                     std::false_type>::value,
        "detected_or: failure value_t is false_type");
    record_assertion(_test_handler, 
        std::is_same<detected_or_t<float, has_value_type_expr,
                                    with_int_value_type>, int>::value,
        "detected_or_t alias: success path");
    record_assertion(_test_handler, 
        std::is_same<detected_or_t<float, has_value_type_expr,
                                    without_value_type>, float>::value,
        "detected_or_t alias: failure path");

    // ---- detected_t ----
    record_assertion(_test_handler, 
        std::is_same<detected_t<has_value_type_expr, with_int_value_type>,
                     int>::value,
        "detected_t: success -> int");
    record_assertion(_test_handler, 
        std::is_same<detected_t<has_value_type_expr, without_value_type>,
                     nonesuch>::value,
        "detected_t: failure -> nonesuch sentinel");

    // ---- is_detected ----
    record_assertion(_test_handler, 
        is_detected<has_value_type_expr, with_int_value_type>::value == true,
        "is_detected: with value_type -> true");
    record_assertion(_test_handler, 
        is_detected<has_value_type_expr, without_value_type>::value == false,
        "is_detected: without value_type -> false");
    record_assertion(_test_handler, 
        is_detected<has_size_member_expr, std::vector<int>>::value == true,
        "is_detected: vector has .size() -> true");
    record_assertion(_test_handler, 
        is_detected<has_size_member_expr, int>::value == false,
        "is_detected: int has no .size() -> false");

    // ---- is_detected_convertible ----
    record_assertion(_test_handler, 
        is_detected_convertible<long, has_value_type_expr,
                                with_int_value_type>::value == true,
        "is_detected_convertible: int -> long convertible -> true");
    record_assertion(_test_handler, 
        is_detected_convertible<int, has_value_type_expr,
                                with_int_value_type>::value == true,
        "is_detected_convertible: int -> int convertible -> true");
    record_assertion(_test_handler, 
        is_detected_convertible<long, has_value_type_expr,
                                with_unrelated_value_type>::value == false,
        "is_detected_convertible: opaque not convertible to long -> false");
    record_assertion(_test_handler, 
        is_detected_convertible<long, has_value_type_expr,
                                without_value_type>::value == false,
        "is_detected_convertible: probe failure -> false");

    // ---- is_detected_exact ----
    record_assertion(_test_handler, 
        is_detected_exact<int, has_value_type_expr,
                          with_int_value_type>::value == true,
        "is_detected_exact: int matches int -> true");
    record_assertion(_test_handler, 
        is_detected_exact<long, has_value_type_expr,
                          with_int_value_type>::value == false,
        "is_detected_exact: long does not match int -> false");
    record_assertion(_test_handler, 
        is_detected_exact<int, has_value_type_expr,
                          without_value_type>::value == false,
        "is_detected_exact: probe failure (nonesuch != int) -> false");
    record_assertion(_test_handler, 
        is_detected_exact<const int, has_value_type_expr,
                          with_int_value_type>::value == false,
        "is_detected_exact: const int does not match int (strict identity)");

    return;
}


NS_END  // test
NS_END  // djinterp
