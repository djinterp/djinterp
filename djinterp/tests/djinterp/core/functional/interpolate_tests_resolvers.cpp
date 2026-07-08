/******************************************************************************
* djinterp [test]                                   interpolate_tests_resolvers.cpp
*
* Section III (leaf resolvers) -- empty_resolver (identity: everything misses),
* map_resolver (linear {key,value} lookup; a miss is a real miss), and
* lookup_resolver (adapt an always-hit callable, owning its produced value),
* plus the bindings / lookup factories and the resolvers' value types.
******************************************************************************/

#include "interpolate_tests.hpp"


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// the empty resolver misses on every key (the identity source)
bool
test_empty_resolver()
{
    ::djinterp::empty_resolver<char> _e;

    D_INTERP_CHECK(!_e(sv("anything")).found());
    D_INTERP_CHECK(!_e(sv("")).found());

    return true;
}


// map_resolver hits a bound key with its value
bool
test_map_hit()
{
    ::djinterp::map_resolver<char> _m{ {"a", "1"}, {"b", "2"} };

    D_INTERP_CHECK(_m(sv("a")).found() && _m(sv("a")).value() == "1");
    D_INTERP_CHECK(_m(sv("b")).found() && _m(sv("b")).value() == "2");

    return true;
}


// an unbound key is a MISS (not an empty hit)
bool
test_map_miss()
{
    ::djinterp::map_resolver<char> _m{ {"a", "1"} };

    D_INTERP_CHECK(!_m(sv("c")).found());

    return true;
}


// on duplicate keys, the first binding wins (linear scan order)
bool
test_map_first_wins()
{
    ::djinterp::map_resolver<char> _m{ {"a", "1"}, {"a", "2"} };

    D_INTERP_CHECK(_m(sv("a")).value() == "1");

    return true;
}


// an empty binding list misses on everything
bool
test_map_empty()
{
    ::djinterp::map_resolver<char> _m({});

    D_INTERP_CHECK(!_m(sv("a")).found());

    return true;
}


// the bindings() factory builds an equivalent map_resolver
bool
test_bindings_factory()
{
    const auto _m = ::djinterp::bindings<char>({ {"k", "v"} });

    D_INTERP_CHECK(_m(sv("k")).found() && _m(sv("k")).value() == "v");
    D_INTERP_CHECK(!_m(sv("nope")).found());

    return true;
}


// lookup_resolver ALWAYS hits (a missing key still resolves), via its callable
bool
test_lookup_always_hit()
{
    ::djinterp::lookup_resolver<char, wrap_fn> _lr{ wrap_fn{} };

    D_INTERP_CHECK(_lr(sv("x")).found());
    D_INTERP_CHECK(_lr(sv("x")).value() == "[x]");
    // even a "missing" key hits -- the always-hit contract
    D_INTERP_CHECK(_lr(sv("missing")).found());
    D_INTERP_CHECK(_lr(sv("missing")).value() == "[missing]");

    return true;
}


// the lookup() factory builds an equivalent lookup_resolver
bool
test_lookup_factory()
{
    const auto _lr = ::djinterp::lookup<char>(wrap_fn{});

    D_INTERP_CHECK(_lr(sv("y")).found() && _lr(sv("y")).value() == "[y]");

    return true;
}


// the resolvers advertise their value types (view for map/empty, owning
// string for lookup)
bool
test_resolver_value_types()
{
    static_assert(std::is_same<::djinterp::map_resolver<char>::value_type,
                               sv>::value,
        "map_resolver yields views");
    static_assert(
        std::is_same<::djinterp::lookup_resolver<char, wrap_fn>::value_type,
                     std::string>::value,
        "lookup_resolver owns a string value");
    static_assert(std::is_same<::djinterp::empty_resolver<char>::view_type,
                               sv>::value,
        "empty_resolver view_type is the view");

    return true;
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


dt::block_spec
resolvers_block()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    return dt::block_spec{
        "resolvers",
        "empty / map / lookup leaf resolvers and their factories",
        {
            { "empty",         "empty_resolver misses on every key",              &test_empty_resolver },
            { "map_hit",       "map_resolver hits a bound key with its value",     &test_map_hit },
            { "map_miss",      "an unbound key is a real miss",                    &test_map_miss },
            { "map_first_wins", "on duplicate keys the first binding wins",         &test_map_first_wins },
            { "map_empty",     "an empty binding list misses on everything",       &test_map_empty },
            { "bindings",      "the bindings() factory builds a map_resolver",     &test_bindings_factory },
            { "lookup_hit",    "lookup_resolver always hits via its callable",     &test_lookup_always_hit },
            { "lookup",        "the lookup() factory builds a lookup_resolver",    &test_lookup_factory },
            { "value_types",   "map/empty yield views, lookup owns a string",      &test_resolver_value_types }
        }
    };
#else
    return dt::block_spec{ "resolvers", "leaf resolvers (requires C++17)", {} };
#endif
}


NS_END  // testing
NS_END  // djinterp
