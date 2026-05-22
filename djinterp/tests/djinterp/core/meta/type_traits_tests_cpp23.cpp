/******************************************************************************
* djinterp [test]                                    type_traits_tests_cpp23.cpp
*
*   Unit tests for the C++23-features section (I.4) of type_traits.hpp:
*     - is_scoped_enum
*     - is_scoped_enum_v
*
*   On C++23+ this is imported from std::; on C++11..C++20 it's djinterp's
* own implementation based on:
*     "scoped enum" == is_enum AND NOT implicitly convertible to int
*
*   Coverage:
*   - positive: enum class / enum struct (both syntactic forms; tested
*     via a single representative because they are equivalent)
*   - negative: unscoped enum (the historical `enum X { ... }` form)
*   - negative: non-enum types (int, class, void, pointer)
*   - the underlying-type variant: `enum class : char` still counts as
*     scoped (the qualifier, not the storage, is what matters)
*
*
* path:      /inc/djinterp/test/type_traits_tests_cpp23.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/
#include "./type_traits_tests.hpp"


NS_DJINTERP
NS_TEST


namespace
{

// scoped (enum class)
enum class scoped_default_t
{
    a,
    b
};

// scoped (enum struct, same semantics as enum class)
enum struct scoped_struct_t
{
    a,
    b
};

// scoped with explicit underlying type
enum class scoped_char_t : char
{
    a,
    b
};

// unscoped (legacy)
enum unscoped_default_t
{
    legacy_a,
    legacy_b
};

// unscoped with explicit underlying type
enum unscoped_int_t : int
{
    explicit_underlying_a,
    explicit_underlying_b
};

// not an enum at all -- defined for the negative case
struct enum_negative_class_t
{
    int x;
};

}  // namespace


// =========================================================================
// I.   is_scoped_enum  positive cases
// =========================================================================

static_assert(is_scoped_enum<scoped_default_t>::value == true,
              "is_scoped_enum<enum class> -> true");
static_assert(is_scoped_enum<scoped_struct_t>::value == true,
              "is_scoped_enum<enum struct> -> true");
static_assert(is_scoped_enum<scoped_char_t>::value == true,
              "is_scoped_enum<enum class : char> -> true (underlying type irrelevant)");

// the shared helper from the test header
static_assert(is_scoped_enum<type_traits_test_types::scoped_enum_t>::value == true,
              "is_scoped_enum<shared scoped helper>");


// =========================================================================
// II.  is_scoped_enum  negative cases
// =========================================================================

// unscoped enums
static_assert(is_scoped_enum<unscoped_default_t>::value == false,
              "is_scoped_enum<unscoped enum> -> false");
static_assert(is_scoped_enum<unscoped_int_t>::value == false,
              "is_scoped_enum<unscoped enum : int> -> false (explicit underlying != scoped)");
static_assert(is_scoped_enum<type_traits_test_types::unscoped_enum_t>::value == false,
              "is_scoped_enum<shared unscoped helper>");

// non-enum types
static_assert(is_scoped_enum<int>::value == false,
              "is_scoped_enum<int> -> false");
static_assert(is_scoped_enum<enum_negative_class_t>::value == false,
              "is_scoped_enum<class type> -> false");
static_assert(is_scoped_enum<void>::value == false,
              "is_scoped_enum<void> -> false");
static_assert(is_scoped_enum<int*>::value == false,
              "is_scoped_enum<int*> -> false");
static_assert(is_scoped_enum<int[5]>::value == false,
              "is_scoped_enum<int[5]> -> false");
static_assert(is_scoped_enum<std::vector<int>>::value == false,
              "is_scoped_enum<vector<int>> -> false");


// =========================================================================
// III. is_scoped_enum_v  (compile-time)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_scoped_enum_v<scoped_default_t>   == true,
                  "is_scoped_enum_v: scoped -> true");
    static_assert(is_scoped_enum_v<unscoped_default_t> == false,
                  "is_scoped_enum_v: unscoped -> false");
    static_assert(is_scoped_enum_v<int>                == false,
                  "is_scoped_enum_v: int -> false");
#endif


// =========================================================================
// IV.  RUNTIME DRIVER
// =========================================================================

void
type_traits_tests_cpp23(
    test_handler& _test_handler
)
{
    // positive
    record_assertion(_test_handler, 
        is_scoped_enum<scoped_default_t>::value == true,
        "is_scoped_enum<enum class>");
    record_assertion(_test_handler, 
        is_scoped_enum<scoped_struct_t>::value == true,
        "is_scoped_enum<enum struct>");
    record_assertion(_test_handler, 
        is_scoped_enum<scoped_char_t>::value == true,
        "is_scoped_enum<enum class : char>");
    record_assertion(_test_handler, 
        is_scoped_enum<type_traits_test_types::scoped_enum_t>::value == true,
        "is_scoped_enum<shared scoped helper>");

    // negative -- enum-but-unscoped
    record_assertion(_test_handler, 
        is_scoped_enum<unscoped_default_t>::value == false,
        "is_scoped_enum<unscoped enum>");
    record_assertion(_test_handler, 
        is_scoped_enum<unscoped_int_t>::value == false,
        "is_scoped_enum<unscoped : int>");
    record_assertion(_test_handler, 
        is_scoped_enum<type_traits_test_types::unscoped_enum_t>::value == false,
        "is_scoped_enum<shared unscoped helper>");

    // negative -- non-enum
    record_assertion(_test_handler, 
        ( is_scoped_enum<int>::value                  == false &&
          is_scoped_enum<enum_negative_class_t>::value == false &&
          is_scoped_enum<void>::value                  == false &&
          is_scoped_enum<int*>::value                  == false &&
          is_scoped_enum<int[5]>::value                == false &&
          is_scoped_enum<std::vector<int>>::value      == false ),
        "is_scoped_enum: non-enum types all -> false");

    return;
}


NS_END  // test
NS_END  // djinterp
