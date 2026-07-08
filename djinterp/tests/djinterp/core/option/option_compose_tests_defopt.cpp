/******************************************************************************
* djinterp [test]                              option_compose_tests_defopt.cpp
*
*   Section I of the option_compose.hpp suite: defopt.
*
*     defopt<_Key, _Args...>   - an intention-revealing alias for
*                                option<_Key, _Args...>; the "surface" being
*                                described.  Carries no semantics of its own -
*                                it must be exactly the option type.
*
*   There is nothing to compute here beyond alias identity, so the tests pin
* defopt against the corresponding option<> for a unary key, a key with one
* opaque arg, and a key with several - confirming the args are carried through
* verbatim and in order.
*
*   (Whole file gated on C++20 concepts - see the suite header.)
*
*
* path:      /tests/djinterp/core/option/option_compose_tests_defopt.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.07
******************************************************************************/

// std
#include <type_traits>
// djinterp
#include "option_compose_tests.hpp"


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

// defopt_unary
//   a key-only surface is exactly the unary option.
bool
defopt_unary()
{
    constexpr bool ok = std::is_same<defopt<oc_key::a>, option<oc_key::a>>::value;

    static_assert(ok, "defopt<K> == option<K>");
    return ok;
}

// defopt_with_args
//   a surface with one opaque arg is exactly option<K, arg>.
bool
defopt_with_args()
{
    constexpr bool ok =
        std::is_same<defopt<oc_key::a, oc_val<5>>,
                     option<oc_key::a, oc_val<5>>>::value;

    static_assert(ok, "defopt<K, A> == option<K, A>");
    return ok;
}

// defopt_multi_arg
//   several args are carried through verbatim and in order.
bool
defopt_multi_arg()
{
    constexpr bool ok =
        std::is_same<defopt<oc_key::a, oc_val<5>, oc_val<6>>,
                     option<oc_key::a, oc_val<5>, oc_val<6>>>::value;

    static_assert(ok, "defopt<K, A, B> == option<K, A, B> (args verbatim, in order)");
    return ok;
}

#endif  // C++20 concepts


// ---------------------------------------------------------------------------
// block provider
// ---------------------------------------------------------------------------
::djinterp::test::block_spec
option_compose_defopt_block()
{
    ::djinterp::test::block_spec b;
    b.name = "I. defopt";
#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS
    b.descriptor = "surface alias: defopt<K, Args...> == option<K, Args...>";
    b.tests = {
        { "defopt_unary",      "defopt<K> == option<K>",                 &defopt_unary },
        { "defopt_with_args",  "defopt<K, A> == option<K, A>",           &defopt_with_args },
        { "defopt_multi_arg",  "defopt<K, A, B> == option<K, A, B>",     &defopt_multi_arg },
    };
#else
    b.descriptor = "skipped: option_compose.hpp needs C++20 concepts";
#endif
    return b;
}


NS_END  // testing
NS_END  // djinterp
