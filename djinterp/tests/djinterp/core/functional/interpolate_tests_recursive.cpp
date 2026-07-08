/******************************************************************************
* djinterp [test]                                   interpolate_tests_recursive.cpp
*
* Section VI -- recursive_resolver / recursive: expand a resolved VALUE that is
* itself a template.  Verifies flat (no-nesting) values, one/multi level
* nesting, the depth bound (budget 0 and 1, and a self-referential binding that
* terminates with the placeholder intact), partial nested expansion, a miss
* staying a miss, a non-default (sigil) scanner for nested values, use through
* the engine, the factory, and the owning value type.
******************************************************************************/

#include "interpolate_tests.hpp"


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// a value with no placeholders passes straight through
bool
test_recursive_flat()
{
    const auto _rr = ::djinterp::recursive<char>(
        ::djinterp::map_resolver<char>{ {"a", "1"} });

    D_INTERP_CHECK(_rr(sv("a")).found() && _rr(sv("a")).value() == "1");

    return true;
}


// a one-level nested value is expanded
bool
test_recursive_nested()
{
    const auto _rr = ::djinterp::recursive<char>(
        ::djinterp::map_resolver<char>{ {"a", "{b}"}, {"b", "B"} });

    D_INTERP_CHECK(_rr(sv("a")).value() == "B");

    return true;
}


// several levels expand fully
bool
test_recursive_deep()
{
    const auto _rr = ::djinterp::recursive<char>(
        ::djinterp::map_resolver<char>{
            {"a", "{b}"}, {"b", "{c}"}, {"c", "C"} });

    D_INTERP_CHECK(_rr(sv("a")).value() == "C");

    return true;
}


// a budget of 0 emits the value as-is (no expansion)
bool
test_recursive_depth_zero()
{
    const auto _rr = ::djinterp::recursive<char>(
        ::djinterp::map_resolver<char>{ {"a", "{b}"}, {"b", "B"} }, 0);

    D_INTERP_CHECK(_rr(sv("a")).value() == "{b}");

    return true;
}


// a budget of 1 expands exactly one level
bool
test_recursive_depth_one()
{
    const auto _rr = ::djinterp::recursive<char>(
        ::djinterp::map_resolver<char>{
            {"a", "{b}"}, {"b", "{c}"}, {"c", "C"} }, 1);

    D_INTERP_CHECK(_rr(sv("a")).value() == "{c}");

    return true;
}


// a self-referential binding terminates at the budget, leaving the placeholder
bool
test_recursive_self_ref()
{
    const auto _rr = ::djinterp::recursive<char>(
        ::djinterp::map_resolver<char>{ {"a", "{a}"} }, 2);

    // never loops: the deepest still-unresolved placeholder is left intact
    D_INTERP_CHECK(_rr(sv("a")).value() == "{a}");

    return true;
}


// nested keys resolve independently; an unbound nested key passes through
bool
test_recursive_partial()
{
    const auto _rr = ::djinterp::recursive<char>(
        ::djinterp::map_resolver<char>{ {"a", "{b}{c}"}, {"b", "B"} });

    D_INTERP_CHECK(_rr(sv("a")).value() == "B{c}");

    return true;
}


// a miss of the inner resolver stays a miss
bool
test_recursive_miss()
{
    const auto _rr = ::djinterp::recursive<char>(
        ::djinterp::map_resolver<char>{ {"a", "1"} });

    D_INTERP_CHECK(!_rr(sv("z")).found());

    return true;
}


// a nested value is re-scanned with the chosen scanner (here, sigil)
bool
test_recursive_custom_scanner()
{
    const auto _rr =
        ::djinterp::recursive<char, ::djinterp::sigil_scanner<char>>(
            ::djinterp::map_resolver<char>{ {"a", "$b"}, {"b", "B"} });

    D_INTERP_CHECK(_rr(sv("a")).value() == "B");

    return true;
}


// a recursive resolver drives the engine like any other
bool
test_recursive_via_engine()
{
    std::string _out;
    ::djinterp::interp_string_sink<char> _sink(_out);
    ::djinterp::interpolate_into(
        _sink,
        ::djinterp::brace_scanner<char>("{a}"),
        ::djinterp::recursive<char>(
            ::djinterp::map_resolver<char>{ {"a", "{b}"}, {"b", "B"} }));

    D_INTERP_CHECK(_out == "B");

    return true;
}


// the recursive frame owns a string value
bool
test_recursive_value_type()
{
    static_assert(
        std::is_same<::djinterp::recursive_resolver<
                         char, ::djinterp::map_resolver<char>>::value_type,
                     std::string>::value,
        "recursive_resolver owns a string value");

    return true;
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


dt::block_spec
recursive_block()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    return dt::block_spec{
        "recursive",
        "recursive_resolver: expand resolved values as nested templates",
        {
            { "flat",           "a value with no placeholders passes through",   &test_recursive_flat },
            { "nested",         "a one-level nested value is expanded",         &test_recursive_nested },
            { "deep",           "several nesting levels expand fully",          &test_recursive_deep },
            { "depth_zero",     "a budget of 0 emits the value as-is",          &test_recursive_depth_zero },
            { "depth_one",      "a budget of 1 expands exactly one level",       &test_recursive_depth_one },
            { "self_ref",       "a self-reference terminates at the budget",     &test_recursive_self_ref },
            { "partial",        "nested keys resolve independently",            &test_recursive_partial },
            { "miss",           "an inner miss stays a miss",                   &test_recursive_miss },
            { "custom_scanner", "nested values re-scan with the chosen scanner", &test_recursive_custom_scanner },
            { "via_engine",     "a recursive resolver drives the engine",       &test_recursive_via_engine },
            { "value_type",     "the recursive frame owns a string value",       &test_recursive_value_type }
        }
    };
#else
    return dt::block_spec{ "recursive", "recursive expansion (requires C++17)", {} };
#endif
}


NS_END  // testing
NS_END  // djinterp
