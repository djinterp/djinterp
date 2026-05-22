/******************************************************************************
* djinterp [test]                                    type_traits_tests_cpp20.cpp
*
*   Unit tests for the C++20-features section (I.3) of type_traits.hpp:
*     - is_bounded_array, is_bounded_array_v
*     - is_unbounded_array, is_unbounded_array_v
*     - remove_cvref, remove_cvref_t
*     - type_identity, type_identity_t
*
*   On C++20+ these are imported from std::; on C++11..C++17 they are
* djinterp's own implementations. The tests cover the corner cases of
* both:
*
*   - is_bounded_array: true for T[N], false for T[] and any non-array
*   - is_unbounded_array: true for T[], false for T[N] and any non-array
*   - is_bounded_array and is_unbounded_array are mutually exclusive AND
*     mutually exhaustive across all array types
*   - remove_cvref: strips cv AND reference (composition order matters)
*     across the eight combinations of {const, !const} x {volatile,
*     !volatile} x {ref, !ref}
*   - type_identity: passes the type through unchanged for primitives,
*     references, arrays, cv-qualified types, function pointers
*
*
* path:      /inc/djinterp/test/type_traits_tests_cpp20.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/
#include "./type_traits_tests.hpp"


NS_DJINTERP
NS_TEST


// =========================================================================
// I.   is_bounded_array  (compile-time)
// =========================================================================

// positive cases
static_assert(is_bounded_array<int[5]>::value == true,
              "is_bounded_array<int[5]> -> true");
static_assert(is_bounded_array<int[1]>::value == true,
              "is_bounded_array<int[1]> -> true (size 1)");
static_assert(is_bounded_array<char[100]>::value == true,
              "is_bounded_array<char[100]> -> true");
static_assert(is_bounded_array<int[3][4]>::value == true,
              "is_bounded_array<int[3][4]> -> true (multi-dim outermost is bounded)");

// negative cases -- unbounded array
static_assert(is_bounded_array<int[]>::value == false,
              "is_bounded_array<int[]> -> false (unbounded)");

// negative cases -- non-array types
static_assert(is_bounded_array<int>::value == false,
              "is_bounded_array<int> -> false (scalar)");
static_assert(is_bounded_array<int*>::value == false,
              "is_bounded_array<int*> -> false (pointer)");
static_assert(is_bounded_array<int&>::value == false,
              "is_bounded_array<int&> -> false (reference)");
static_assert(is_bounded_array<std::vector<int>>::value == false,
              "is_bounded_array<vector<int>> -> false (class type)");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_bounded_array_v<int[5]> == true,
                  "is_bounded_array_v<int[5]> -> true");
    static_assert(is_bounded_array_v<int[]>  == false,
                  "is_bounded_array_v<int[]> -> false");
    static_assert(is_bounded_array_v<int>    == false,
                  "is_bounded_array_v<int>   -> false");
#endif


// =========================================================================
// II.  is_unbounded_array  (compile-time)
// =========================================================================

// positive cases
static_assert(is_unbounded_array<int[]>::value == true,
              "is_unbounded_array<int[]> -> true");
static_assert(is_unbounded_array<char[]>::value == true,
              "is_unbounded_array<char[]> -> true");

// negative cases -- bounded array
static_assert(is_unbounded_array<int[5]>::value == false,
              "is_unbounded_array<int[5]> -> false (bounded)");
static_assert(is_unbounded_array<int[1]>::value == false,
              "is_unbounded_array<int[1]> -> false (size 1)");

// negative cases -- non-array
static_assert(is_unbounded_array<int>::value == false,
              "is_unbounded_array<int> -> false");
static_assert(is_unbounded_array<int*>::value == false,
              "is_unbounded_array<int*> -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_unbounded_array_v<int[]>   == true,
                  "is_unbounded_array_v<int[]>  -> true");
    static_assert(is_unbounded_array_v<int[5]>  == false,
                  "is_unbounded_array_v<int[5]> -> false");
#endif


// =========================================================================
// III. is_bounded_array / is_unbounded_array  mutual exclusion
// =========================================================================
//   For any T, at most one of these traits is true.  For array types,
// exactly one is true (matching is bounded vs unbounded).

static_assert(is_bounded_array<int[5]>::value !=
              is_unbounded_array<int[5]>::value,
              "Mutual exclusion: int[5]");
static_assert(is_bounded_array<int[]>::value !=
              is_unbounded_array<int[]>::value,
              "Mutual exclusion: int[]");
static_assert(is_bounded_array<int>::value ==
              is_unbounded_array<int>::value,
              "Both false for non-array (int)");


// =========================================================================
// IV.  remove_cvref / remove_cvref_t  (compile-time)
// =========================================================================
//   Strips cv qualifiers AND reference. Order matters: remove_reference
// is applied first (a "reference to const T" loses the reference, then
// the const is stripped from the underlying T).

// pass-through for already-clean types
static_assert(std::is_same<remove_cvref_t<int>, int>::value,
              "remove_cvref_t<int> -> int (no-op)");
static_assert(std::is_same<typename remove_cvref<int>::type, int>::value,
              "remove_cvref<int>::type -> int (struct form)");

// lone qualifiers
static_assert(std::is_same<remove_cvref_t<const int>, int>::value,
              "remove_cvref_t<const int> -> int");
static_assert(std::is_same<remove_cvref_t<volatile int>, int>::value,
              "remove_cvref_t<volatile int> -> int");
static_assert(std::is_same<remove_cvref_t<const volatile int>, int>::value,
              "remove_cvref_t<const volatile int> -> int");

// lone references
static_assert(std::is_same<remove_cvref_t<int&>, int>::value,
              "remove_cvref_t<int&> -> int");
static_assert(std::is_same<remove_cvref_t<int&&>, int>::value,
              "remove_cvref_t<int&&> -> int");

// combinations
static_assert(std::is_same<remove_cvref_t<const int&>, int>::value,
              "remove_cvref_t<const int&> -> int");
static_assert(std::is_same<remove_cvref_t<volatile int&>, int>::value,
              "remove_cvref_t<volatile int&> -> int");
static_assert(std::is_same<remove_cvref_t<const int&&>, int>::value,
              "remove_cvref_t<const int&&> -> int");
static_assert(std::is_same<remove_cvref_t<const volatile int&>, int>::value,
              "remove_cvref_t<const volatile int&> -> int");
static_assert(std::is_same<remove_cvref_t<const volatile int&&>, int>::value,
              "remove_cvref_t<const volatile int&&> -> int");

// pointers are not modified by remove_cvref (pointer is its own type)
static_assert(std::is_same<remove_cvref_t<int*>, int*>::value,
              "remove_cvref_t<int*> -> int* (pointer survives)");
static_assert(std::is_same<remove_cvref_t<const int*>, const int*>::value,
              "remove_cvref_t<const int*> -> const int* (pointee const survives)");
static_assert(std::is_same<remove_cvref_t<int* const>, int*>::value,
              "remove_cvref_t<int* const> -> int* (top-level const stripped)");

// class types
struct test_class { int v; };
static_assert(std::is_same<remove_cvref_t<const test_class&>, test_class>::value,
              "remove_cvref_t<const test_class&> -> test_class");


// =========================================================================
// V.   type_identity / type_identity_t  (compile-time)
// =========================================================================

static_assert(std::is_same<type_identity_t<int>, int>::value,
              "type_identity_t<int> -> int");
static_assert(std::is_same<typename type_identity<int>::type, int>::value,
              "type_identity<int>::type -> int (struct form)");

// cv-qualifications and references pass through
static_assert(std::is_same<type_identity_t<const int>, const int>::value,
              "type_identity_t<const int> -> const int");
static_assert(std::is_same<type_identity_t<volatile int>, volatile int>::value,
              "type_identity_t<volatile int> -> volatile int");
static_assert(std::is_same<type_identity_t<int&>, int&>::value,
              "type_identity_t<int&> -> int&");
static_assert(std::is_same<type_identity_t<int&&>, int&&>::value,
              "type_identity_t<int&&> -> int&&");
static_assert(std::is_same<type_identity_t<const int&>, const int&>::value,
              "type_identity_t<const int&> -> const int&");

// compound types pass through
static_assert(std::is_same<type_identity_t<int[5]>, int[5]>::value,
              "type_identity_t<int[5]> -> int[5]");
static_assert(std::is_same<type_identity_t<int*>, int*>::value,
              "type_identity_t<int*> -> int*");
static_assert(std::is_same<type_identity_t<std::vector<int>>,
                           std::vector<int>>::value,
              "type_identity_t<vector<int>> -> vector<int>");

// function type
static_assert(std::is_same<type_identity_t<int(int)>, int(int)>::value,
              "type_identity_t<int(int)> -> int(int)");

// void
static_assert(std::is_same<type_identity_t<void>, void>::value,
              "type_identity_t<void> -> void");


// =========================================================================
// VI.  RUNTIME DRIVER
// =========================================================================

void
type_traits_tests_cpp20(
    test_handler& _test_handler
)
{
    // ---- is_bounded_array ----
    record_assertion(_test_handler, 
        is_bounded_array<int[5]>::value == true,
        "is_bounded_array<int[5]>");
    record_assertion(_test_handler, 
        is_bounded_array<int[1]>::value == true,
        "is_bounded_array<int[1]>");
    record_assertion(_test_handler, 
        is_bounded_array<int[3][4]>::value == true,
        "is_bounded_array<int[3][4]>");
    record_assertion(_test_handler, 
        is_bounded_array<int[]>::value == false,
        "is_bounded_array<int[]> -> false");
    record_assertion(_test_handler, 
        ( is_bounded_array<int>::value == false &&
          is_bounded_array<int*>::value == false &&
          is_bounded_array<int&>::value == false &&
          is_bounded_array<std::vector<int>>::value == false ),
        "is_bounded_array: non-array types -> false");

    // ---- is_unbounded_array ----
    record_assertion(_test_handler, 
        is_unbounded_array<int[]>::value == true,
        "is_unbounded_array<int[]>");
    record_assertion(_test_handler, 
        is_unbounded_array<int[5]>::value == false,
        "is_unbounded_array<int[5]> -> false");
    record_assertion(_test_handler, 
        ( is_unbounded_array<int>::value == false &&
          is_unbounded_array<int*>::value == false ),
        "is_unbounded_array: non-array types -> false");

    // mutual exclusion
    record_assertion(_test_handler, 
        is_bounded_array<int[5]>::value != is_unbounded_array<int[5]>::value,
        "Mutual exclusion: int[5]");
    record_assertion(_test_handler, 
        is_bounded_array<int[]>::value != is_unbounded_array<int[]>::value,
        "Mutual exclusion: int[]");

    // ---- remove_cvref ----
    record_assertion(_test_handler, 
        std::is_same<remove_cvref_t<int>, int>::value,
        "remove_cvref_t<int> -> int (no-op)");
    record_assertion(_test_handler, 
        std::is_same<remove_cvref_t<const int>, int>::value,
        "remove_cvref_t<const int> -> int");
    record_assertion(_test_handler, 
        std::is_same<remove_cvref_t<int&>, int>::value,
        "remove_cvref_t<int&> -> int");
    record_assertion(_test_handler, 
        std::is_same<remove_cvref_t<const int&>, int>::value,
        "remove_cvref_t<const int&> -> int");
    record_assertion(_test_handler, 
        std::is_same<remove_cvref_t<const volatile int&&>, int>::value,
        "remove_cvref_t<const volatile int&&> -> int");
    record_assertion(_test_handler, 
        std::is_same<remove_cvref_t<int*>, int*>::value,
        "remove_cvref_t<int*> -> int* (pointee unaffected)");
    record_assertion(_test_handler, 
        std::is_same<remove_cvref_t<const int*>, const int*>::value,
        "remove_cvref_t<const int*> -> const int* (pointee const survives)");
    record_assertion(_test_handler, 
        std::is_same<remove_cvref_t<int* const>, int*>::value,
        "remove_cvref_t<int* const> -> int* (top-level const stripped)");

    // ---- type_identity ----
    record_assertion(_test_handler, 
        std::is_same<type_identity_t<int>, int>::value,
        "type_identity_t<int>");
    record_assertion(_test_handler, 
        std::is_same<type_identity_t<const int&>, const int&>::value,
        "type_identity_t<const int&> -- preserves cv-ref");
    record_assertion(_test_handler, 
        std::is_same<type_identity_t<int[5]>, int[5]>::value,
        "type_identity_t<int[5]>");
    record_assertion(_test_handler, 
        std::is_same<type_identity_t<void>, void>::value,
        "type_identity_t<void>");

    return;
}


NS_END  // test
NS_END  // djinterp
