/******************************************************************************
* djinterp [test]                                  interpolate_tests_resolution.cpp
*
* Section III.i -- resolution<value>: the minimal maybe a resolver answers with.
* Verifies found()/value(), the resolved()/unresolved() factories, and the LAZY
* or_else combinator that chaining routes through -- a hit returns unchanged
* without invoking the alternative; a miss invokes it; and or_else chains.
******************************************************************************/

#include "interpolate_tests.hpp"


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// resolved() builds a hit carrying its value
bool
test_resolution_hit()
{
    const auto _r = ::djinterp::resolved(sv("v"));

    D_INTERP_CHECK(_r.found());
    D_INTERP_CHECK(_r.value() == "v");

    return true;
}


// unresolved() builds a miss
bool
test_resolution_miss()
{
    const auto _r = ::djinterp::unresolved<sv>();

    D_INTERP_CHECK(!_r.found());

    return true;
}


// or_else on a hit returns the hit and does NOT invoke the alternative (lazy)
bool
test_resolution_or_else_hit()
{
    bool _called = false;
    const auto _r = ::djinterp::resolved(sv("v")).or_else(
        [&_called]()
        {
            _called = true;
            return ::djinterp::unresolved<sv>();
        });

    D_INTERP_CHECK(_r.found() && _r.value() == "v");
    D_INTERP_CHECK(!_called);   // laziness: the fallback never ran

    return true;
}


// or_else on a miss invokes the alternative and returns its result
bool
test_resolution_or_else_miss()
{
    bool _called = false;
    const auto _r = ::djinterp::unresolved<sv>().or_else(
        [&_called]()
        {
            _called = true;
            return ::djinterp::resolved(sv("alt"));
        });

    D_INTERP_CHECK(_r.found() && _r.value() == "alt");
    D_INTERP_CHECK(_called);

    return true;
}


// or_else chains: the first hit down the chain wins
bool
test_resolution_or_else_chain()
{
    const auto _r =
        ::djinterp::unresolved<sv>()
            .or_else([]() { return ::djinterp::unresolved<sv>(); })
            .or_else([]() { return ::djinterp::resolved(sv("z")); });

    D_INTERP_CHECK(_r.found() && _r.value() == "z");

    return true;
}


// value_type is the carried value; resolved() decays its argument
bool
test_resolution_value_type()
{
    static_assert(std::is_same<::djinterp::resolution<sv>::value_type,
                               sv>::value,
        "resolution value_type is the value");

    // resolved(const view&) decays to resolution<view>
    const sv   _k = sv("x");
    const auto _r = ::djinterp::resolved(_k);
    static_assert(std::is_same<decltype(_r)::value_type, sv>::value,
        "resolved decays its argument type");

    D_INTERP_CHECK(_r.found() && _r.value() == "x");

    return true;
}


// resolution and or_else are constant expressions
bool
test_resolution_constexpr()
{
    constexpr auto _eval = []() -> bool
    {
        const auto _hit  = ::djinterp::resolved(sv("v"));
        const auto _miss = ::djinterp::unresolved<sv>();

        return _hit.found() && (_hit.value() == "v") && !_miss.found() &&
               (_miss.or_else([]() { return ::djinterp::resolved(sv("x")); })
                    .value() == "x");
    };

    static_assert(_eval(), "resolution + or_else at compile time");
    D_INTERP_CHECK(_eval());

    return true;
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


dt::block_spec
resolution_block()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    return dt::block_spec{
        "resolution",
        "the found/value answer and its lazy or_else combinator",
        {
            { "hit",           "resolved() builds a hit carrying its value",      &test_resolution_hit },
            { "miss",          "unresolved() builds a miss",                      &test_resolution_miss },
            { "or_else_hit",   "or_else on a hit skips the alternative (lazy)",    &test_resolution_or_else_hit },
            { "or_else_miss",  "or_else on a miss runs the alternative",          &test_resolution_or_else_miss },
            { "or_else_chain", "or_else chains: the first hit wins",              &test_resolution_or_else_chain },
            { "value_type",    "value_type is the value; resolved() decays",       &test_resolution_value_type },
            { "constexpr",     "resolution + or_else are constant expressions",    &test_resolution_constexpr }
        }
    };
#else
    return dt::block_spec{ "resolution", "resolution (requires C++17)", {} };
#endif
}


NS_END  // testing
NS_END  // djinterp
