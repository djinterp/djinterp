/******************************************************************************
* djinterp [test]                                 interpolate_tests_composition.cpp
*
* Section III (composition) -- chain_resolver (try A, else B; first hit wins,
* second consulted only on a miss) and when_resolver (gate a resolver on a key
* predicate; non-matching keys fall through as a miss), plus the chain / when
* factories.  Pins the two documented properties: chain's lazy fall-through
* (observed with a call-counting frame) and its associativity.
******************************************************************************/

#include "interpolate_tests.hpp"


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// chain: the first frame's hit wins
bool
test_chain_first_hit()
{
    const auto _c = ::djinterp::chain(
        ::djinterp::map_resolver<char>{ {"a", "1"} },
        ::djinterp::map_resolver<char>{ {"b", "2"} });

    D_INTERP_CHECK(_c(sv("a")).found() && _c(sv("a")).value() == "1");

    return true;
}


// chain: a first-frame miss falls through to the second
bool
test_chain_second_hit()
{
    const auto _c = ::djinterp::chain(
        ::djinterp::map_resolver<char>{ {"a", "1"} },
        ::djinterp::map_resolver<char>{ {"b", "2"} });

    D_INTERP_CHECK(_c(sv("b")).found() && _c(sv("b")).value() == "2");

    return true;
}


// chain: both frames missing yields a miss
bool
test_chain_both_miss()
{
    const auto _c = ::djinterp::chain(
        ::djinterp::map_resolver<char>{ {"a", "1"} },
        ::djinterp::map_resolver<char>{ {"b", "2"} });

    D_INTERP_CHECK(!_c(sv("c")).found());

    return true;
}


// chain: when both frames bind a key, the first frame's value is used
bool
test_chain_first_frame_priority()
{
    const auto _c = ::djinterp::chain(
        ::djinterp::map_resolver<char>{ {"a", "first"} },
        ::djinterp::map_resolver<char>{ {"a", "second"} });

    D_INTERP_CHECK(_c(sv("a")).value() == "first");

    return true;
}


// chain is LAZY: the second frame is consulted only when the first misses
bool
test_chain_lazy()
{
    int _calls = 0;

    // first frame hits 'a'; the counting frame trails it
    const auto _c = ::djinterp::chain(
        ::djinterp::map_resolver<char>{ {"a", "1"} },
        counting_resolver{ &_calls, false, sv() });

    // 'a' hits in the first frame -> the counting frame must NOT run
    (void)_c(sv("a"));
    D_INTERP_CHECK(_calls == 0);

    // 'z' misses the first frame -> the counting frame runs exactly once
    (void)_c(sv("z"));
    D_INTERP_CHECK(_calls == 1);

    return true;
}


// chain composition is associative
bool
test_chain_associative()
{
    const auto _a = ::djinterp::map_resolver<char>{ {"x", "A"} };
    const auto _b = ::djinterp::map_resolver<char>{ {"y", "B"} };
    const auto _d = ::djinterp::map_resolver<char>{ {"z", "C"} };

    const auto _left  = ::djinterp::chain(::djinterp::chain(_a, _b), _d);
    const auto _right = ::djinterp::chain(_a, ::djinterp::chain(_b, _d));

    const sv _keys[] = { sv("x"), sv("y"), sv("z"), sv("w") };
    for (const sv _k : _keys)
    {
        D_INTERP_CHECK(_left(_k).found() == _right(_k).found());
        if (_left(_k).found())
        {
            D_INTERP_CHECK(_left(_k).value() == _right(_k).value());
        }
    }

    return true;
}


// the chain() factory composes two resolvers
bool
test_chain_factory()
{
    const auto _c = ::djinterp::chain(
        ::djinterp::map_resolver<char>{ {"a", "1"} },
        ::djinterp::empty_resolver<char>{});

    D_INTERP_CHECK(_c(sv("a")).value() == "1");
    D_INTERP_CHECK(!_c(sv("b")).found());

    return true;
}


// when: a key passing the predicate is resolved by the inner resolver
bool
test_when_gate_open()
{
    const auto _w = ::djinterp::when(
        pred_is_a{},
        ::djinterp::map_resolver<char>{ {"a", "1"}, {"apple", "2"} });

    D_INTERP_CHECK(_w(sv("a")).found()     && _w(sv("a")).value() == "1");
    D_INTERP_CHECK(_w(sv("apple")).found() && _w(sv("apple")).value() == "2");

    return true;
}


// when: a key failing the predicate is a miss (gate closed)
bool
test_when_gate_closed()
{
    const auto _w = ::djinterp::when(
        pred_is_a{},
        ::djinterp::map_resolver<char>{ {"b", "1"} });

    D_INTERP_CHECK(!_w(sv("b")).found());

    return true;
}


// when: an open gate over an inner miss is still a miss
bool
test_when_open_inner_miss()
{
    const auto _w = ::djinterp::when(
        pred_true{},
        ::djinterp::map_resolver<char>{ {"a", "1"} });

    D_INTERP_CHECK(!_w(sv("z")).found());   // gate open, inner misses

    return true;
}


// the when() factory gates a resolver on a predicate
bool
test_when_factory()
{
    const auto _w = ::djinterp::when(
        pred_is_a{},
        ::djinterp::map_resolver<char>{ {"a", "1"} });

    D_INTERP_CHECK(_w(sv("a")).value() == "1");
    D_INTERP_CHECK(!_w(sv("nope")).found());

    return true;
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


dt::block_spec
composition_block()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    return dt::block_spec{
        "composition",
        "chain and when resolver composition",
        {
            { "chain_first_hit",   "chain: the first frame's hit wins",           &test_chain_first_hit },
            { "chain_second_hit",  "chain: a first miss falls through to second",  &test_chain_second_hit },
            { "chain_both_miss",   "chain: both missing yields a miss",           &test_chain_both_miss },
            { "chain_priority",    "chain: the earlier frame's value is used",     &test_chain_first_frame_priority },
            { "chain_lazy",        "chain: the second frame runs only on a miss",  &test_chain_lazy },
            { "chain_associative", "chain composition is associative",            &test_chain_associative },
            { "chain_factory",     "the chain() factory composes two resolvers",   &test_chain_factory },
            { "when_open",         "when: a passing key is resolved by the inner",  &test_when_gate_open },
            { "when_closed",       "when: a failing key is a miss",               &test_when_gate_closed },
            { "when_inner_miss",   "when: an open gate over an inner miss misses",  &test_when_open_inner_miss },
            { "when_factory",      "the when() factory gates a resolver",         &test_when_factory }
        }
    };
#else
    return dt::block_spec{ "composition", "resolver composition (requires C++17)", {} };
#endif
}


NS_END  // testing
NS_END  // djinterp
