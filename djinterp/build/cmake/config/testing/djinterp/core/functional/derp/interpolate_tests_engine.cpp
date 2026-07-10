/******************************************************************************
* djinterp [test]                                      interpolate_tests_engine.cpp
*
* Section V -- interpolate_into: the fold that binds scanner, resolver, and
* sink.  Verifies literal passthrough, a hit's value emission, a miss leaving
* the raw placeholder, mixed and partial interpolation, escape collapsing, the
* exact literal-vs-value DISPATCH (via a recording sink), and that the engine is
* scanner-agnostic (sigil and replay scanners drive it identically).
******************************************************************************/

#include "interpolate_tests.hpp"


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// render a brace template through a resolver into a fresh string
template<typename _Resolver>
static std::string
render_brace(
    sv         _tmpl,
    _Resolver  _resolver
)
{
    std::string _out;
    ::djinterp::interp_string_sink<char> _sink(_out);
    ::djinterp::interpolate_into(
        _sink, ::djinterp::brace_scanner<char>(_tmpl), _resolver);

    return _out;
}


// a literal-only template is emitted verbatim
bool
test_engine_literal_only()
{
    D_INTERP_CHECK(render_brace("hello",
                                ::djinterp::empty_resolver<char>{}) == "hello");

    return true;
}


// a hit's value replaces the placeholder
bool
test_engine_hit()
{
    D_INTERP_CHECK(render_brace("{a}",
                     ::djinterp::map_resolver<char>{ {"a", "1"} }) == "1");

    return true;
}


// a miss leaves the raw placeholder untouched (what makes partial well-defined)
bool
test_engine_miss_passthrough()
{
    D_INTERP_CHECK(render_brace("{a}",
                     ::djinterp::empty_resolver<char>{}) == "{a}");

    return true;
}


// literals and hits interleave correctly
bool
test_engine_mixed()
{
    D_INTERP_CHECK(render_brace("x{a}y{b}z",
                     ::djinterp::map_resolver<char>{ {"a", "1"}, {"b", "2"} })
                   == "x1y2z");

    return true;
}


// a partial render fills known keys and passes the rest through
bool
test_engine_partial()
{
    D_INTERP_CHECK(render_brace("{a}{b}",
                     ::djinterp::map_resolver<char>{ {"a", "1"} }) == "1{b}");

    return true;
}


// escapes collapse to their literal brace on the way through
bool
test_engine_escapes()
{
    D_INTERP_CHECK(render_brace("{{literal}}",
                     ::djinterp::empty_resolver<char>{}) == "{literal}");

    return true;
}


// the engine dispatches literal() for literals AND misses, value() for hits
bool
test_engine_dispatch()
{
    std::string _literals;
    std::string _values;
    recording_sink _sink{ &_literals, &_values };

    ::djinterp::interpolate_into(
        _sink,
        ::djinterp::brace_scanner<char>("a{hit}{miss}"),
        ::djinterp::map_resolver<char>{ {"hit", "H"} });

    // literal run "a" and the miss's raw "{miss}" both go to literal()
    D_INTERP_CHECK(_literals == "a|{miss}|");
    // only the hit's value goes to value()
    D_INTERP_CHECK(_values == "H|");

    return true;
}


// the engine is scanner-agnostic: a sigil scanner drives it identically
bool
test_engine_sigil()
{
    std::string _out;
    ::djinterp::interp_string_sink<char> _sink(_out);
    ::djinterp::interpolate_into(
        _sink,
        ::djinterp::sigil_scanner<char>("$a!"),
        ::djinterp::map_resolver<char>{ {"a", "1"} });

    D_INTERP_CHECK(_out == "1!");

    return true;
}


// a replay scanner over a scanned cache renders identically
bool
test_engine_replay()
{
    const auto _cache =
        scan_all(::djinterp::brace_scanner<char>("{a}!"));

    std::string _out;
    ::djinterp::interp_string_sink<char> _sink(_out);
    ::djinterp::interpolate_into(
        _sink,
        ::djinterp::replay_scanner<char>(_cache),
        ::djinterp::map_resolver<char>{ {"a", "Z"} });

    D_INTERP_CHECK(_out == "Z!");

    return true;
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


dt::block_spec
engine_block()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    return dt::block_spec{
        "engine",
        "interpolate_into: the scanner -> resolver -> sink fold",
        {
            { "literal_only", "a literal template is emitted verbatim",         &test_engine_literal_only },
            { "hit",          "a hit's value replaces the placeholder",         &test_engine_hit },
            { "miss",         "a miss leaves the raw placeholder",              &test_engine_miss_passthrough },
            { "mixed",        "literals and hits interleave correctly",         &test_engine_mixed },
            { "partial",      "a partial render passes unknown keys through",    &test_engine_partial },
            { "escapes",      "escapes collapse to their literal brace",        &test_engine_escapes },
            { "dispatch",     "literal() for literals+misses, value() for hits", &test_engine_dispatch },
            { "sigil",        "the engine is scanner-agnostic (sigil)",         &test_engine_sigil },
            { "replay",       "a replay scanner renders identically",           &test_engine_replay }
        }
    };
#else
    return dt::block_spec{ "engine", "the fold engine (requires C++17)", {} };
#endif
}


NS_END  // testing
NS_END  // djinterp
