/******************************************************************************
* djinterp [test]                                 type_traits_tests_template.cpp
*
*   Unit tests for the template-detection traits in Section III of
* type_traits.hpp:
*     - is_template,                  _v
*     - is_template_with_args,        _v
*     - is_template_parameter_base_of, _v
*
*   Semantics:
*
*   is_template<T>
*     True iff T is a specialization of some template-of-typenames with
*     ZERO arguments (i.e., T == X<>).  The partial specialization that
*     fires is `is_template<_Type<>>`, so only the empty form matches.
*     Note: `is_template<std::tuple<int>>::value` is FALSE -- the partial
*     spec doesn't fire because the template-instantiation has args.
*
*   is_template_with_args<T>
*     True iff T is a specialization of some template-of-typenames with
*     ANY argument count (including zero -- variadic accepts empty).
*     So both `std::tuple<>` and `std::tuple<int, char>` match.
*
*   is_template_parameter_base_of<T>
*     True iff T has a nested `value_type` AND is_base_of<value_type, T>.
*     The probe is `typename _Type::value_type, enable_if<is_base_of<vt, T>>`.
*     A class deriving from itself via its own value_type (CRTP-style)
*     fires the trait.
*
*
* path:      /inc/djinterp/test/type_traits_tests_template.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/
#include "./type_traits_tests.hpp"


NS_DJINTERP
NS_TEST

namespace
{

// base_class_t / derived_with_base_value_type
//   Subjects for is_template_parameter_base_of: derived_with_base_value_type
// has a `value_type = base_class_t` typedef and derives from
// base_class_t.  The base IS a base of the derived, and the derived's
// value_type IS the base.
struct base_class_t
{
    int b;
};

struct derived_with_base_value_type : base_class_t
{
    using value_type = base_class_t;
};

// class_with_unrelated_value_type
//   Has a `value_type` typedef but the type is NOT a base of the class.
// Negative case: probe expression is well-formed (value_type exists)
// but enable_if<is_base_of<...>> fails.
struct class_with_unrelated_value_type
{
    using value_type = int;  // int is not a base of any class type
};

}  // namespace


// =========================================================================
// I.   is_template  (compile-time)
// =========================================================================
//   Only the EMPTY template instantiation matches.

// positive case: empty tuple
static_assert(is_template<std::tuple<>>::value == true,
              "is_template<std::tuple<>> -> true (empty template instantiation)");

// negative case: non-empty tuple
static_assert(is_template<std::tuple<int>>::value == false,
              "is_template<std::tuple<int>> -> false (has args)");
static_assert(is_template<std::tuple<int, char>>::value == false,
              "is_template<std::tuple<int, char>> -> false (has args)");

// negative case: non-template type
static_assert(is_template<int>::value == false,
              "is_template<int> -> false (not a template)");
static_assert(is_template<void>::value == false,
              "is_template<void> -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_template_v<std::tuple<>>   == true,
                  "is_template_v: empty tuple -> true");
    static_assert(is_template_v<std::tuple<int>> == false,
                  "is_template_v: non-empty tuple -> false");
    static_assert(is_template_v<int>             == false,
                  "is_template_v: int -> false");
#endif


// =========================================================================
// II.  is_template_with_args  (compile-time)
// =========================================================================
//   Matches ANY specialization of a template-of-typenames, INCLUDING
// the empty form (because the variadic `_Args...` accepts zero args).

// positive cases
static_assert(is_template_with_args<std::tuple<int>>::value == true,
              "is_template_with_args<std::tuple<int>> -> true");
static_assert(is_template_with_args<std::tuple<int, char, float>>::value == true,
              "is_template_with_args<std::tuple<3 args>> -> true");
static_assert(is_template_with_args<std::tuple<>>::value == true,
              "is_template_with_args<std::tuple<>> -> true (empty pack OK for variadic)");
static_assert(is_template_with_args<std::vector<int>>::value == true,
              "is_template_with_args<std::vector<int>> -> true");
static_assert(is_template_with_args<std::pair<int, double>>::value == true,
              "is_template_with_args<std::pair<int, double>> -> true");

// negative cases -- not class templates of typenames
static_assert(is_template_with_args<int>::value == false,
              "is_template_with_args<int> -> false");
static_assert(is_template_with_args<void>::value == false,
              "is_template_with_args<void> -> false");
static_assert(is_template_with_args<int*>::value == false,
              "is_template_with_args<int*> -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_template_with_args_v<std::tuple<int>> == true,
                  "is_template_with_args_v (true)");
    static_assert(is_template_with_args_v<int>             == false,
                  "is_template_with_args_v (false)");
#endif


// =========================================================================
// III. is_template / is_template_with_args  cross-check
// =========================================================================
//   is_template_with_args is a superset of is_template (it accepts both
// empty and non-empty), so for any T, is_template<T> -> is_template_with_args<T>.

// std::tuple<>: matches both
static_assert(is_template<std::tuple<>>::value == true &&
              is_template_with_args<std::tuple<>>::value == true,
              "Both fire on std::tuple<>");

// std::tuple<int>: only is_template_with_args
static_assert(is_template<std::tuple<int>>::value == false &&
              is_template_with_args<std::tuple<int>>::value == true,
              "Only is_template_with_args fires on std::tuple<int>");

// int: neither fires
static_assert(is_template<int>::value == false &&
              is_template_with_args<int>::value == false,
              "Neither fires on int");


// =========================================================================
// IV.  is_template_parameter_base_of  (compile-time)
// =========================================================================
//   The trait fires iff `_Type::value_type` exists AND is_base_of<vt, _Type>.

// positive case: derived has value_type = base, and is derived from base
static_assert(is_template_parameter_base_of<derived_with_base_value_type>::value == true,
              "is_template_parameter_base_of<derived_with_base_value_type> -> true");

// negative case: value_type exists but is NOT a base
static_assert(is_template_parameter_base_of<class_with_unrelated_value_type>::value == false,
              "is_template_parameter_base_of<vt unrelated> -> false");

// negative case: no value_type at all
static_assert(is_template_parameter_base_of<base_class_t>::value == false,
              "is_template_parameter_base_of<base_class_t> -> false (no value_type)");

// negative case: builtin
static_assert(is_template_parameter_base_of<int>::value == false,
              "is_template_parameter_base_of<int> -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_template_parameter_base_of_v<derived_with_base_value_type> == true,
                  "is_template_parameter_base_of_v (true)");
    static_assert(is_template_parameter_base_of_v<int> == false,
                  "is_template_parameter_base_of_v (false)");
#endif


// =========================================================================
// V.   RUNTIME DRIVER
// =========================================================================

void
type_traits_tests_template(
    test_handler& _test_handler
)
{
    // ---- is_template ----
    record_assertion(_test_handler, 
        is_template<std::tuple<>>::value == true,
        "is_template<std::tuple<>> -> true");
    record_assertion(_test_handler, 
        is_template<std::tuple<int>>::value == false,
        "is_template<std::tuple<int>> -> false (has args)");
    record_assertion(_test_handler, 
        is_template<std::tuple<int, char>>::value == false,
        "is_template<std::tuple<int, char>> -> false");
    record_assertion(_test_handler, 
        is_template<int>::value == false,
        "is_template<int>");
    record_assertion(_test_handler, 
        is_template<void>::value == false,
        "is_template<void>");

    // ---- is_template_with_args ----
    record_assertion(_test_handler, 
        is_template_with_args<std::tuple<int>>::value == true,
        "is_template_with_args<std::tuple<int>>");
    record_assertion(_test_handler, 
        is_template_with_args<std::tuple<int, char, float>>::value == true,
        "is_template_with_args<std::tuple<3 args>>");
    record_assertion(_test_handler, 
        is_template_with_args<std::tuple<>>::value == true,
        "is_template_with_args<std::tuple<>> (variadic empty OK)");
    record_assertion(_test_handler, 
        is_template_with_args<std::vector<int>>::value == true,
        "is_template_with_args<std::vector<int>>");
    record_assertion(_test_handler, 
        is_template_with_args<int>::value == false,
        "is_template_with_args<int>");
    record_assertion(_test_handler, 
        is_template_with_args<void>::value == false,
        "is_template_with_args<void>");
    record_assertion(_test_handler, 
        is_template_with_args<int*>::value == false,
        "is_template_with_args<int*>");

    // ---- cross-check ----
    record_assertion(_test_handler, 
        ( is_template<std::tuple<>>::value == true &&
          is_template_with_args<std::tuple<>>::value == true ),
        "Both traits fire on std::tuple<>");
    record_assertion(_test_handler, 
        ( is_template<std::tuple<int>>::value == false &&
          is_template_with_args<std::tuple<int>>::value == true ),
        "Only with_args fires on std::tuple<int>");
    record_assertion(_test_handler, 
        ( is_template<int>::value == false &&
          is_template_with_args<int>::value == false ),
        "Neither fires on int");

    // ---- is_template_parameter_base_of ----
    record_assertion(_test_handler, 
        is_template_parameter_base_of<derived_with_base_value_type>::value == true,
        "is_template_parameter_base_of: CRTP-style positive case");
    record_assertion(_test_handler, 
        is_template_parameter_base_of<class_with_unrelated_value_type>::value == false,
        "is_template_parameter_base_of: vt unrelated -> false");
    record_assertion(_test_handler, 
        is_template_parameter_base_of<base_class_t>::value == false,
        "is_template_parameter_base_of: no value_type -> false");
    record_assertion(_test_handler, 
        is_template_parameter_base_of<int>::value == false,
        "is_template_parameter_base_of<int>");

    return;
}


NS_END  // test
NS_END  // djinterp
