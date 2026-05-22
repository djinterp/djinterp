/******************************************************************************
* djinterp [test]                                    type_traits_tests_tuple.cpp
*
*   Unit tests for the tuple-meta traits in Section III of type_traits.hpp.
* These four traits were moved out of dtuple.hpp because type_traits.hpp's
* `evaluate_types_for_trait` (and its dependents) consume them, and the
* original layering put consumers below their producer.  Tests live here,
* with the rest of the type_traits suite, for the same reason.
*
*     - first_arg,            first_arg_t
*     - is_tuple
*     - is_single_tuple_arg,  is_single_tuple_arg_v
*     - to_tuple,             to_tuple_t
*
*   Semantics:
*
*   first_arg<_Type, _Types...>
*     ::type names the first element of the pack.  Defined only for
*     non-empty packs; `first_arg<>` is intentionally undefined (the
*     empty-pack case is an ill-formed reference, not a SFINAE false).
*
*   is_tuple<_Type>
*     true_type iff _Type is a specialization of std::tuple, regardless
*     of element count.  Implemented via D_TRAIT_IS_SPECIALIZATION_OF,
*     so std::tuple<> matches as well as std::tuple<int, char, ...>.
*
*   is_single_tuple_arg<_Types...>
*     true_type iff the pack has exactly one element AND that element
*     is itself a std::tuple.  Empty pack -> false; non-tuple single
*     arg -> false; a tuple plus other args -> false.
*
*   to_tuple<_Types...>
*     The normalizer: produces a std::tuple from the pack.
*     - to_tuple<>             -> std::tuple<>          (explicit specialization)
*     - to_tuple<std::tuple<...>> -> std::tuple<...>    (pass-through)
*     - to_tuple<_T>           -> std::tuple<_T>        (wrap-once)
*     - to_tuple<_T1, _T2, ...> -> std::tuple<_T1, _T2, ...>
*     The empty-pack explicit specialization exists specifically to
*     avoid the ill-formed `first_arg<>::type` reference that the
*     primary template's std::conditional would otherwise evaluate.
*
*
* path:      /inc/djinterp/test/type_traits_tests_tuple.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.22
******************************************************************************/
#include "./type_traits_tests.hpp"


NS_DJINTERP
NS_TEST


// =========================================================================
// I.   first_arg  (compile-time)
// =========================================================================

// single arg -- ::type names the single type
static_assert(std::is_same<typename first_arg<int>::type, int>::value,
              "first_arg<int>::type == int");
static_assert(std::is_same<typename first_arg<double>::type, double>::value,
              "first_arg<double>::type == double");
static_assert(std::is_same<typename first_arg<std::tuple<int>>::type,
                           std::tuple<int>>::value,
              "first_arg<tuple<int>>::type == tuple<int>");

// multi arg -- ::type names the head
static_assert(std::is_same<typename first_arg<int, char>::type, int>::value,
              "first_arg<int, char>::type == int");
static_assert(std::is_same<typename first_arg<int, char, float>::type,
                           int>::value,
              "first_arg<int, char, float>::type == int");
static_assert(std::is_same<typename first_arg<std::tuple<>, int>::type,
                           std::tuple<>>::value,
              "first_arg<tuple<>, int>::type == tuple<>");

// first_arg_t alias agrees with the struct form
static_assert(std::is_same<first_arg_t<int>, int>::value,
              "first_arg_t<int> == int");
static_assert(std::is_same<first_arg_t<int, char, float>, int>::value,
              "first_arg_t<int, char, float> == int");

// NOTE: first_arg<> (empty pack) is intentionally undefined.  Naming it
// in a static_assert context would be ill-formed, so the empty-pack
// case is NOT exercised here.  It is exercised indirectly via the
// to_tuple<> specialization (Section IV), which exists precisely to
// avoid the empty-pack first_arg path.


// =========================================================================
// II.  is_tuple  (compile-time)
// =========================================================================

// positive cases -- specializations of std::tuple of various arities
static_assert(is_tuple<std::tuple<>>::value == true,
              "is_tuple<std::tuple<>> -> true (empty tuple)");
static_assert(is_tuple<std::tuple<int>>::value == true,
              "is_tuple<std::tuple<int>> -> true");
static_assert(is_tuple<std::tuple<int, char>>::value == true,
              "is_tuple<std::tuple<int, char>> -> true");
static_assert(is_tuple<std::tuple<int, char, float, double>>::value == true,
              "is_tuple<std::tuple<4-element>> -> true");

// nested tuple is still a tuple
static_assert(is_tuple<std::tuple<std::tuple<int>>>::value == true,
              "is_tuple<tuple<tuple<int>>> -> true");

// negative cases -- non-tuple class templates
static_assert(is_tuple<std::vector<int>>::value == false,
              "is_tuple<std::vector<int>> -> false");
static_assert(is_tuple<std::pair<int, char>>::value == false,
              "is_tuple<std::pair<int, char>> -> false");

// pair-of-tuples is itself a pair, not a tuple
static_assert(is_tuple<std::pair<std::tuple<int>, std::tuple<char>>>::value == false,
              "is_tuple<pair<tuple, tuple>> -> false (the outer type is pair)");

// negative cases -- non-class types
static_assert(is_tuple<int>::value == false,
              "is_tuple<int> -> false");
static_assert(is_tuple<void>::value == false,
              "is_tuple<void> -> false");
static_assert(is_tuple<int*>::value == false,
              "is_tuple<int*> -> false");
static_assert(is_tuple<int[5]>::value == false,
              "is_tuple<int[5]> -> false");


// =========================================================================
// III. is_single_tuple_arg  (compile-time)
// =========================================================================

// positive cases -- exactly one std::tuple in the pack
static_assert(is_single_tuple_arg<std::tuple<int>>::value == true,
              "is_single_tuple_arg<tuple<int>> -> true");
static_assert(is_single_tuple_arg<std::tuple<int, char>>::value == true,
              "is_single_tuple_arg<tuple<int, char>> -> true");
static_assert(is_single_tuple_arg<std::tuple<>>::value == true,
              "is_single_tuple_arg<tuple<>> -> true (empty tuple is still a tuple)");

// negative case -- empty pack
static_assert(is_single_tuple_arg<>::value == false,
              "is_single_tuple_arg<> -> false (empty pack)");

// negative case -- single non-tuple
static_assert(is_single_tuple_arg<int>::value == false,
              "is_single_tuple_arg<int> -> false (not a tuple)");
static_assert(is_single_tuple_arg<std::vector<int>>::value == false,
              "is_single_tuple_arg<vector<int>> -> false");
static_assert(is_single_tuple_arg<std::pair<int, char>>::value == false,
              "is_single_tuple_arg<pair<int, char>> -> false");

// negative case -- multi-arg, even if the head is a tuple
static_assert(is_single_tuple_arg<std::tuple<int>, char>::value == false,
              "is_single_tuple_arg<tuple<int>, char> -> false (arity > 1)");
static_assert(is_single_tuple_arg<int, char>::value == false,
              "is_single_tuple_arg<int, char> -> false (arity > 1)");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_single_tuple_arg_v<std::tuple<int>> == true,
                  "is_single_tuple_arg_v: true case");
    static_assert(is_single_tuple_arg_v<int> == false,
                  "is_single_tuple_arg_v: false case (non-tuple)");
    static_assert(is_single_tuple_arg_v<> == false,
                  "is_single_tuple_arg_v: false case (empty pack)");
#endif


// =========================================================================
// IV.  to_tuple  (compile-time)
// =========================================================================

// empty pack -- explicit specialization avoids the ill-formed
// first_arg<> path that the primary template would otherwise hit.
static_assert(std::is_same<typename to_tuple<>::type, std::tuple<>>::value,
              "to_tuple<>::type == std::tuple<>");
static_assert(std::is_same<to_tuple_t<>, std::tuple<>>::value,
              "to_tuple_t<> alias agrees");

// single non-tuple arg -- wrapped exactly once
static_assert(std::is_same<to_tuple_t<int>, std::tuple<int>>::value,
              "to_tuple_t<int> == std::tuple<int>");
static_assert(std::is_same<to_tuple_t<char>, std::tuple<char>>::value,
              "to_tuple_t<char> == std::tuple<char>");
static_assert(std::is_same<to_tuple_t<std::vector<int>>,
                           std::tuple<std::vector<int>>>::value,
              "to_tuple_t<vector<int>> == tuple<vector<int>> (vector is not a tuple)");

// single tuple arg -- pass-through (the whole point of the trait)
static_assert(std::is_same<to_tuple_t<std::tuple<int>>,
                           std::tuple<int>>::value,
              "to_tuple_t<tuple<int>> == tuple<int> (pass-through, not double-wrapped)");
static_assert(std::is_same<to_tuple_t<std::tuple<int, char>>,
                           std::tuple<int, char>>::value,
              "to_tuple_t<tuple<int, char>> == tuple<int, char>");
static_assert(std::is_same<to_tuple_t<std::tuple<>>,
                           std::tuple<>>::value,
              "to_tuple_t<tuple<>> == tuple<> (empty tuple passes through)");

// multi-arg pack -- always wrapped, even when the head is a tuple
static_assert(std::is_same<to_tuple_t<int, char>,
                           std::tuple<int, char>>::value,
              "to_tuple_t<int, char> == tuple<int, char>");
static_assert(std::is_same<to_tuple_t<int, char, float>,
                           std::tuple<int, char, float>>::value,
              "to_tuple_t<int, char, float> == tuple<int, char, float>");
static_assert(std::is_same<to_tuple_t<std::tuple<int>, char>,
                           std::tuple<std::tuple<int>, char>>::value,
              "to_tuple_t<tuple<int>, char> == tuple<tuple<int>, char> (NOT unwrapped)");

// idempotence -- applying to_tuple twice is the same as once
static_assert(std::is_same<to_tuple_t<to_tuple_t<int>>,
                           to_tuple_t<int>>::value,
              "to_tuple is idempotent on a wrapped scalar");
static_assert(std::is_same<to_tuple_t<to_tuple_t<int, char>>,
                           to_tuple_t<int, char>>::value,
              "to_tuple is idempotent on a wrapped pack");


// =========================================================================
// V.   Cross-trait coherence  (compile-time)
// =========================================================================
//   The four traits are interlocked: when is_single_tuple_arg fires,
// to_tuple takes the pass-through branch; otherwise it wraps.  And the
// result of to_tuple is always a std::tuple specialization (i.e.,
// is_tuple fires on it).

// to_tuple's output always satisfies is_tuple
static_assert(is_tuple<to_tuple_t<int>>::value == true,
              "is_tuple<to_tuple_t<int>> -> true (output is always a tuple)");
static_assert(is_tuple<to_tuple_t<int, char>>::value == true,
              "is_tuple<to_tuple_t<int, char>> -> true");
static_assert(is_tuple<to_tuple_t<std::tuple<int>>>::value == true,
              "is_tuple<to_tuple_t<tuple<int>>> -> true");
static_assert(is_tuple<to_tuple_t<>>::value == true,
              "is_tuple<to_tuple_t<>> -> true (empty tuple)");

// is_single_tuple_arg true -> to_tuple is the identity on the tuple
static_assert(is_single_tuple_arg<std::tuple<int, char>>::value == true &&
              std::is_same<to_tuple_t<std::tuple<int, char>>,
                           std::tuple<int, char>>::value,
              "single_tuple_arg true => to_tuple is pass-through");

// is_single_tuple_arg false (non-tuple single arg) -> to_tuple wraps
static_assert(is_single_tuple_arg<int>::value == false &&
              std::is_same<to_tuple_t<int>, std::tuple<int>>::value,
              "single_tuple_arg false (non-tuple) => to_tuple wraps");


// =========================================================================
// VI.  RUNTIME DRIVER
// =========================================================================

void
type_traits_tests_tuple(
    test_handler& _test_handler
)
{
    // ---- first_arg ----
    record_assertion(_test_handler,
        std::is_same<typename first_arg<int>::type, int>::value,
        "first_arg<int>::type == int");
    record_assertion(_test_handler,
        std::is_same<typename first_arg<int, char>::type, int>::value,
        "first_arg<int, char>::type == int (head)");
    record_assertion(_test_handler,
        std::is_same<typename first_arg<int, char, float>::type, int>::value,
        "first_arg<int, char, float>::type == int (head)");
    record_assertion(_test_handler,
        std::is_same<typename first_arg<std::tuple<>, int>::type,
                     std::tuple<>>::value,
        "first_arg<tuple<>, int>::type == tuple<>");
    record_assertion(_test_handler,
        std::is_same<first_arg_t<int, char>, int>::value,
        "first_arg_t<int, char> == int (alias)");

    // ---- is_tuple ----
    record_assertion(_test_handler,
        is_tuple<std::tuple<>>::value == true,
        "is_tuple<std::tuple<>>");
    record_assertion(_test_handler,
        is_tuple<std::tuple<int>>::value == true,
        "is_tuple<std::tuple<int>>");
    record_assertion(_test_handler,
        is_tuple<std::tuple<int, char, float>>::value == true,
        "is_tuple<std::tuple<int, char, float>>");
    record_assertion(_test_handler,
        is_tuple<std::tuple<std::tuple<int>>>::value == true,
        "is_tuple<nested tuple>");
    record_assertion(_test_handler,
        is_tuple<int>::value == false,
        "is_tuple<int> -> false");
    record_assertion(_test_handler,
        is_tuple<std::vector<int>>::value == false,
        "is_tuple<vector<int>> -> false");
    record_assertion(_test_handler,
        is_tuple<std::pair<int, char>>::value == false,
        "is_tuple<pair<int, char>> -> false");
    record_assertion(_test_handler,
        ( is_tuple<void>::value  == false &&
          is_tuple<int*>::value  == false &&
          is_tuple<int[5]>::value == false ),
        "is_tuple: non-class types -> false");

    // ---- is_single_tuple_arg ----
    record_assertion(_test_handler,
        is_single_tuple_arg<std::tuple<int>>::value == true,
        "is_single_tuple_arg<tuple<int>>");
    record_assertion(_test_handler,
        is_single_tuple_arg<std::tuple<>>::value == true,
        "is_single_tuple_arg<tuple<>> (empty tuple)");
    record_assertion(_test_handler,
        is_single_tuple_arg<std::tuple<int, char>>::value == true,
        "is_single_tuple_arg<tuple<int, char>>");
    record_assertion(_test_handler,
        is_single_tuple_arg<>::value == false,
        "is_single_tuple_arg<> -> false (empty pack)");
    record_assertion(_test_handler,
        is_single_tuple_arg<int>::value == false,
        "is_single_tuple_arg<int> -> false (not a tuple)");
    record_assertion(_test_handler,
        is_single_tuple_arg<std::tuple<int>, char>::value == false,
        "is_single_tuple_arg<tuple<int>, char> -> false (arity > 1)");
    record_assertion(_test_handler,
        is_single_tuple_arg<int, char>::value == false,
        "is_single_tuple_arg<int, char> -> false");

    // ---- to_tuple ----
    record_assertion(_test_handler,
        std::is_same<to_tuple_t<>, std::tuple<>>::value,
        "to_tuple_t<> == std::tuple<> (empty-pack specialization)");
    record_assertion(_test_handler,
        std::is_same<to_tuple_t<int>, std::tuple<int>>::value,
        "to_tuple_t<int> == tuple<int> (wrap)");
    record_assertion(_test_handler,
        std::is_same<to_tuple_t<std::tuple<int>>, std::tuple<int>>::value,
        "to_tuple_t<tuple<int>> == tuple<int> (pass-through)");
    record_assertion(_test_handler,
        std::is_same<to_tuple_t<std::tuple<>>, std::tuple<>>::value,
        "to_tuple_t<tuple<>> == tuple<> (empty pass-through)");
    record_assertion(_test_handler,
        std::is_same<to_tuple_t<std::tuple<int, char>>,
                     std::tuple<int, char>>::value,
        "to_tuple_t<tuple<int, char>> == tuple<int, char>");
    record_assertion(_test_handler,
        std::is_same<to_tuple_t<int, char>, std::tuple<int, char>>::value,
        "to_tuple_t<int, char> == tuple<int, char> (multi-arg wrap)");
    record_assertion(_test_handler,
        std::is_same<to_tuple_t<std::tuple<int>, char>,
                     std::tuple<std::tuple<int>, char>>::value,
        "to_tuple_t<tuple<int>, char> -- tuple+other wraps (NOT unwrapped)");
    record_assertion(_test_handler,
        std::is_same<to_tuple_t<to_tuple_t<int>>, to_tuple_t<int>>::value,
        "to_tuple is idempotent (scalar)");
    record_assertion(_test_handler,
        std::is_same<to_tuple_t<to_tuple_t<int, char>>,
                     to_tuple_t<int, char>>::value,
        "to_tuple is idempotent (pack)");

    // ---- cross-trait coherence ----
    record_assertion(_test_handler,
        is_tuple<to_tuple_t<int>>::value == true,
        "to_tuple's output is always a std::tuple (scalar input)");
    record_assertion(_test_handler,
        is_tuple<to_tuple_t<int, char, float>>::value == true,
        "to_tuple's output is always a std::tuple (pack input)");
    record_assertion(_test_handler,
        is_tuple<to_tuple_t<>>::value == true,
        "to_tuple's output is always a std::tuple (empty input)");
    record_assertion(_test_handler,
        ( is_single_tuple_arg<std::tuple<int, char>>::value == true &&
          std::is_same<to_tuple_t<std::tuple<int, char>>,
                       std::tuple<int, char>>::value ),
        "single_tuple_arg true => to_tuple is pass-through");
    record_assertion(_test_handler,
        ( is_single_tuple_arg<int>::value == false &&
          std::is_same<to_tuple_t<int>, std::tuple<int>>::value ),
        "single_tuple_arg false (non-tuple) => to_tuple wraps");

    return;
}


NS_END  // test
NS_END  // djinterp
