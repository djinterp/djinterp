/******************************************************************************
* djinterp [test]                            type_traits_tests_macros_member.cpp
*
*   Unit tests for the member-detection macros in Section 0.3 of
* type_traits.hpp:
*     - D_TRAIT_DETECT_METHOD              (covered indirectly via HAS_METHOD)
*     - D_TRAIT_DETECT_METHOD_ARGS         (single-arg form only -- see below)
*     - D_TRAIT_HAS_METHOD
*     - D_TRAIT_HAS_METHOD_ARGS            (single-arg form only -- see below)
*     - D_TRAIT_HAS_TYPE
*     - D_TRAIT_HAS_STATIC_MEMBER
*
*   KNOWN BROKEN MACROS (not exercised here -- bugs filed):
*
*   1. D_TRAIT_DETECT_METHOD_ARGS for multi-arg cases:
*      The macro expands to `std::declval<__VA_ARGS__>()`, which packs
*      every argument into a single `std::declval` template-argument
*      list.  `std::declval` is unary, so any call shape with two or
*      more types fails to compile.  By extension, D_TRAIT_HAS_METHOD_ARGS
*      only works for the SINGLE-arg call form.
*
*   2. D_TRAIT_HAS_METHOD_TYPED and D_TRAIT_HAS_METHOD_CONVERTIBLE:
*      Both synthesise an INHERIT_EXPR of shape
*      `std::is_same<D_TRAIT_DETECT_METHOD_ARGS(...), RETURN_TYPE>`.
*      The synthesised expression contains a top-level comma INSIDE the
*      template-argument list (the `, RETURN_TYPE` part).  When the
*      surrounding D_TRAIT_IS_DETECTED_AS macro processes its arguments,
*      that comma is seen as a fourth argument separator, so the macro
*      always errors with "too many arguments".  Neither macro can be
*      successfully invoked as currently written.
*
*   For each working macro the tests cover:
*     - positive (subject has the feature) and negative (does not)
*     - the trait fires `false` on a builtin (int) as a sanity check
*     - the `_v` variable-template alias matches `::value`
*
*
* path:      /inc/djinterp/test/type_traits_tests_macros_member.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/
#include "./type_traits_tests.hpp"


NS_DJINTERP
NS_TEST

namespace
{

// container_like
//   subject: a "container shape" class with
//     - a `value_type` typedef
//     - a `size_type` typedef
//     - a `push_back(value_type)` method
//     - a `static const int magic` data member
//   The positive case for every macro tested here.
struct container_like
{
    using value_type = int;
    using size_type  = std::size_t;
    using element_t  = value_type;  // alternate-name typedef for HAS_TYPE

    int
    push_back(value_type _v)
    {
        (void)_v;
        return 0;
    }

    int
    erase(value_type _v)
    {
        (void)_v;
        return 0;
    }

    static const int magic = 42;
};

// bare_class
//   subject: no value_type, no methods, no static members. Negative
// case for every macro tested here.
struct bare_class
{};

// has_value_type_no_methods
//   subject: typedef exists but push_back / erase do not. Negative case
// for HAS_METHOD because the typedef is needed for the probe expression
// to be parseable; if value_type is missing, the failure could be
// blamed on the typedef rather than on the method.
struct has_value_type_no_methods
{
    using value_type = int;
};

// unrelated_t
//   non-converting class type: used to drive the "wrong arg type" branch
// of HAS_METHOD_ARGS without accidentally succeeding via an implicit
// numeric conversion.
struct unrelated_t {};

}  // namespace


// =========================================================================
// I.   D_TRAIT_HAS_METHOD  (compile-time)
// =========================================================================
//   D_TRAIT_HAS_METHOD probes `_Type::METHOD(_Type::value_type{})`,
// requiring both a `value_type` typedef AND a callable method.

D_TRAIT_HAS_METHOD(has_push_back, push_back)

static_assert(has_push_back<container_like>::value == true,
              "D_TRAIT_HAS_METHOD: positive case");
static_assert(has_push_back<has_value_type_no_methods>::value == false,
              "D_TRAIT_HAS_METHOD: value_type but no method -> false");
static_assert(has_push_back<bare_class>::value == false,
              "D_TRAIT_HAS_METHOD: nothing -> false");
static_assert(has_push_back<int>::value == false,
              "D_TRAIT_HAS_METHOD: builtin int -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(has_push_back_v<container_like>      == true,
                  "D_TRAIT_HAS_METHOD _v alias (true case)");
    static_assert(has_push_back_v<bare_class>          == false,
                  "D_TRAIT_HAS_METHOD _v alias (false case)");
#endif


// =========================================================================
// II.  D_TRAIT_HAS_METHOD_ARGS  (compile-time, single-arg form only)
// =========================================================================
//   Detects a method callable with the supplied argument type list,
// independent of a `value_type` typedef.  This macro is broken in the
// multi-arg form (see file header); only single-arg cases are exercised
// here.

D_TRAIT_HAS_METHOD_ARGS(has_erase_int, erase, int)

static_assert(has_erase_int<container_like>::value == true,
              "D_TRAIT_HAS_METHOD_ARGS: erase(int) found");
static_assert(has_erase_int<bare_class>::value == false,
              "D_TRAIT_HAS_METHOD_ARGS: not present -> false");
static_assert(has_erase_int<int>::value == false,
              "D_TRAIT_HAS_METHOD_ARGS: builtin int -> false");

// genuinely-incompatible arg type -- no implicit conversion path
D_TRAIT_HAS_METHOD_ARGS(has_erase_unrelated, erase, unrelated_t)

static_assert(has_erase_unrelated<container_like>::value == false,
              "D_TRAIT_HAS_METHOD_ARGS: incompatible arg type -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(has_erase_int_v<container_like>      == true,
                  "D_TRAIT_HAS_METHOD_ARGS _v alias (true)");
    static_assert(has_erase_int_v<bare_class>          == false,
                  "D_TRAIT_HAS_METHOD_ARGS _v alias (false)");
#endif


// =========================================================================
// III. D_TRAIT_HAS_TYPE  (compile-time)
// =========================================================================

D_TRAIT_HAS_TYPE(has_value_type_alias, value_type)

static_assert(has_value_type_alias<container_like>::value == true,
              "D_TRAIT_HAS_TYPE: value_type present -> true");
static_assert(has_value_type_alias<has_value_type_no_methods>::value == true,
              "D_TRAIT_HAS_TYPE: value_type alone -> true");
static_assert(has_value_type_alias<bare_class>::value == false,
              "D_TRAIT_HAS_TYPE: no nested type -> false");
static_assert(has_value_type_alias<int>::value == false,
              "D_TRAIT_HAS_TYPE: builtin -> false");

D_TRAIT_HAS_TYPE(has_size_type_alias, size_type)

static_assert(has_size_type_alias<container_like>::value == true,
              "D_TRAIT_HAS_TYPE: size_type present -> true");
static_assert(has_size_type_alias<has_value_type_no_methods>::value == false,
              "D_TRAIT_HAS_TYPE: size_type absent (other typedefs present) -> false");

// alternate-name probe -- catches a typedef whose name differs from value_type
D_TRAIT_HAS_TYPE(has_element_t_alias, element_t)

static_assert(has_element_t_alias<container_like>::value == true,
              "D_TRAIT_HAS_TYPE: element_t alias present");
static_assert(has_element_t_alias<has_value_type_no_methods>::value == false,
              "D_TRAIT_HAS_TYPE: element_t missing -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(has_value_type_alias_v<container_like>             == true,
                  "D_TRAIT_HAS_TYPE _v alias (true)");
    static_assert(has_value_type_alias_v<bare_class>                 == false,
                  "D_TRAIT_HAS_TYPE _v alias (false)");
#endif


// =========================================================================
// IV.  D_TRAIT_HAS_STATIC_MEMBER  (compile-time)
// =========================================================================
//   Detection via `decltype(_Type::MEMBER_NAME)`. Per the macro comments,
// only validates that the name resolves at class scope -- does not
// constrain the member type.

D_TRAIT_HAS_STATIC_MEMBER(has_magic_static, magic)

static_assert(has_magic_static<container_like>::value == true,
              "D_TRAIT_HAS_STATIC_MEMBER: static const present -> true");
static_assert(has_magic_static<bare_class>::value == false,
              "D_TRAIT_HAS_STATIC_MEMBER: no static -> false");
static_assert(has_magic_static<int>::value == false,
              "D_TRAIT_HAS_STATIC_MEMBER: builtin int -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(has_magic_static_v<container_like>  == true,
                  "D_TRAIT_HAS_STATIC_MEMBER _v alias (true)");
    static_assert(has_magic_static_v<bare_class>      == false,
                  "D_TRAIT_HAS_STATIC_MEMBER _v alias (false)");
#endif


// =========================================================================
// V.   RUNTIME DRIVER
// =========================================================================

void
type_traits_tests_macros_member(
    test_handler& _test_handler
)
{
    // ---- HAS_METHOD ----
    record_assertion(_test_handler, 
        has_push_back<container_like>::value == true,
        "D_TRAIT_HAS_METHOD: positive case");
    record_assertion(_test_handler, 
        has_push_back<has_value_type_no_methods>::value == false,
        "D_TRAIT_HAS_METHOD: value_type but no method -> false");
    record_assertion(_test_handler, 
        has_push_back<bare_class>::value == false,
        "D_TRAIT_HAS_METHOD: bare class -> false");

    // ---- HAS_METHOD_ARGS (single arg) ----
    record_assertion(_test_handler, 
        has_erase_int<container_like>::value == true,
        "D_TRAIT_HAS_METHOD_ARGS: erase(int) present");
    record_assertion(_test_handler, 
        has_erase_int<bare_class>::value == false,
        "D_TRAIT_HAS_METHOD_ARGS: bare class -> false");
    record_assertion(_test_handler, 
        has_erase_unrelated<container_like>::value == false,
        "D_TRAIT_HAS_METHOD_ARGS: incompatible arg type -> false");

    // ---- HAS_TYPE ----
    record_assertion(_test_handler, 
        has_value_type_alias<container_like>::value == true,
        "D_TRAIT_HAS_TYPE: value_type present");
    record_assertion(_test_handler, 
        has_value_type_alias<bare_class>::value == false,
        "D_TRAIT_HAS_TYPE: nothing -> false");
    record_assertion(_test_handler, 
        has_size_type_alias<container_like>::value == true,
        "D_TRAIT_HAS_TYPE: size_type present");
    record_assertion(_test_handler, 
        has_size_type_alias<has_value_type_no_methods>::value == false,
        "D_TRAIT_HAS_TYPE: size_type absent (only value_type) -> false");
    record_assertion(_test_handler, 
        has_element_t_alias<container_like>::value == true,
        "D_TRAIT_HAS_TYPE: element_t alternate name present");

    // ---- HAS_STATIC_MEMBER ----
    record_assertion(_test_handler, 
        has_magic_static<container_like>::value == true,
        "D_TRAIT_HAS_STATIC_MEMBER: present");
    record_assertion(_test_handler, 
        has_magic_static<bare_class>::value == false,
        "D_TRAIT_HAS_STATIC_MEMBER: absent");

    return;
}


NS_END  // test
NS_END  // djinterp
