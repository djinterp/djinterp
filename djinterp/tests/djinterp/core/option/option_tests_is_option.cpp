/******************************************************************************
* djinterp [test]                                   option_tests_is_option.cpp
*
*   Section III of the option.hpp suite: the is_option detection trait.
*
*     is_option<_Type>     - std::false_type, except the partial
*                            specialization is_option<option<_Key,_Args...>>
*                            which is std::true_type.  Catches BOTH the unary
*                            and the args form via one (possibly empty) pack.
*     is_option_v<_Type>   - is_option<clean_t<_Type>>::value: the variable
*                            template first STRIPS cv-qualifiers and references.
*
*   The load-bearing distinctions verified here:
*     - the raw STRUCT does not clean (is_option<option<K>&>::value is false),
*       while the _v shorthand does (is_option_v<option<K>&> is true);
*     - detection is by TEMPLATE IDENTITY, so a type that duck-types the whole
*       option<> surface is still rejected.
*
*
* path:      /tests/djinterp/core/option/option_tests_is_option.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp
#include "option_tests.hpp"


NS_DJINTERP
NS_TESTING


// isopt_true_forms
//   is_option is true (both the ::value and the _v spellings) for the unary
// form and for args forms of every arity - the specialization's empty-or-not
// pack matches them all.
bool
isopt_true_forms()
{
    constexpr bool ok =
        is_option<option<opt_key::alpha>>::value                       &&
        is_option<option<opt_key::alpha, arg_a>>::value                &&
        is_option<option<opt_key::alpha, arg_a, arg_b, arg_c>>::value  &&
        is_option_v<option<opt_key::alpha>>                            &&
        is_option_v<option<opt_key::alpha, arg_a>>                     &&
        is_option_v<option<42, int, char>>;

    static_assert(ok, "is_option true for unary and all args forms");
    return ok;
}

// isopt_false_nonoptions
//   is_option is false for types that are not option<> specializations,
// including the framework's own arg_not_found sentinel and a bare std::tuple
// (the args container, but not an option itself).
bool
isopt_false_nonoptions()
{
    constexpr bool ok =
        !is_option<int>::value                          &&
        !is_option<void>::value                         &&
        !is_option<opt_key>::value                      &&
        !is_option<std::tuple<int, char>>::value        &&
        !is_option<arg_not_found>::value                &&
        !is_option<option<opt_key::alpha>*>::value      &&   // pointer-to-option is not an option
        !is_option_v<int>                               &&
        !is_option_v<void>                              &&
        !is_option_v<std::tuple<arg_a>>;

    static_assert(ok, "is_option false for non-option types");
    return ok;
}

// isopt_struct_does_not_clean
//   the raw is_option<> STRUCT matches option<...> exactly and does NOT strip
// cv / references: a reference or const-qualified option is not the option
// template itself, so the struct reports false.
bool
isopt_struct_does_not_clean()
{
    constexpr bool ok =
        !is_option<option<opt_key::alpha>&>::value               &&
        !is_option<option<opt_key::alpha>&&>::value              &&
        !is_option<const option<opt_key::alpha>>::value          &&
        !is_option<const option<opt_key::alpha>&>::value         &&
        !is_option<volatile option<opt_key::alpha>>::value;

    static_assert(ok, "is_option<> struct does not strip cv/ref");
    return ok;
}

// isopt_v_cleans_cvref
//   is_option_v goes through clean_t, so it sees past every cv / reference
// combination: true for a decorated option, false for a decorated non-option.
bool
isopt_v_cleans_cvref()
{
    constexpr bool ok =
        is_option_v<option<opt_key::alpha>&>                    &&
        is_option_v<option<opt_key::alpha>&&>                   &&
        is_option_v<const option<opt_key::alpha>>              &&
        is_option_v<const option<opt_key::alpha>&>             &&
        is_option_v<volatile option<opt_key::alpha>>           &&
        is_option_v<const volatile option<opt_key::alpha>&>    &&
        is_option_v<const option<opt_key::alpha, arg_a>&>      &&
        !is_option_v<const int&>                               &&
        !is_option_v<volatile std::tuple<int>&>;

    static_assert(ok, "is_option_v strips cv/ref before matching");
    return ok;
}

// isopt_decoy_rejected
//   a type that reproduces the ENTIRE option<> member surface (key_type,
// args_type, key, has_args, arg_count) is still rejected - detection is by
// template identity, not structural shape.
bool
isopt_decoy_rejected()
{
    constexpr bool ok =
        !is_option<option_decoy>::value        &&
        !is_option_v<option_decoy>             &&
        !is_option_v<const option_decoy&>;

    static_assert(ok, "duck-typed decoy is not an option");
    return ok;
}

// isopt_trait_shape
//   is_option is a genuine bool integral_constant: it derives from
// std::true_type / std::false_type and carries value_type bool, so it drops
// into any trait-composition site.
bool
isopt_trait_shape()
{
    constexpr bool ok =
        std::is_base_of<std::true_type,  is_option<option<opt_key::alpha>>>::value  &&
        std::is_base_of<std::false_type, is_option<int>>::value                     &&
        std::is_same<is_option<option<opt_key::alpha>>::value_type, bool>::value    &&
        std::is_same<is_option<int>::value_type, bool>::value;

    static_assert(ok, "is_option is a bool integral_constant");
    return ok;
}


// ---------------------------------------------------------------------------
// block provider
// ---------------------------------------------------------------------------
::djinterp::test::block_spec
option_is_option_block()
{
    ::djinterp::test::block_spec b;
    b.name       = "is_option";
    b.descriptor = "is_option / is_option_v detection of option<> specializations";
    b.tests      = {
        { "isopt_true_forms",
          "is_option true for the unary form and args forms of any arity",
          &isopt_true_forms },
        { "isopt_false_nonoptions",
          "is_option false for int/void/enum/tuple/arg_not_found/option*",
          &isopt_false_nonoptions },
        { "isopt_struct_does_not_clean",
          "the is_option<> struct does not strip cv/ref (matches option<> exactly)",
          &isopt_struct_does_not_clean },
        { "isopt_v_cleans_cvref",
          "is_option_v strips cv/ref before matching (clean_t)",
          &isopt_v_cleans_cvref },
        { "isopt_decoy_rejected",
          "a duck-typed look-alike is rejected: detection is by template identity",
          &isopt_decoy_rejected },
        { "isopt_trait_shape",
          "is_option derives from true_type/false_type with value_type bool",
          &isopt_trait_shape },
    };
    return b;
}


NS_END  // testing
NS_END  // djinterp
