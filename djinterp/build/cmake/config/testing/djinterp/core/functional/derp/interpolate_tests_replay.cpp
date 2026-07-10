/******************************************************************************
* djinterp [test]                                      interpolate_tests_replay.cpp
*
* Section II.iii -- replay_scanner: replays a pre-scanned piece cache instead of
* re-deriving it from the text.  Verifies it reproduces a scan exactly, handles
* an empty cache, and exposes the scanner typedefs.
******************************************************************************/

#include "interpolate_tests.hpp"


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP17_OR_HIGHER

using replay_t = ::djinterp::replay_scanner<char>;


// replaying a cache reproduces the exact scanned sequence
bool
test_replay_from_scan()
{
    const auto _cache =
        scan_all(::djinterp::brace_scanner<char>("a{b}c"));
    const auto _v = scan_all(replay_t(_cache));

    D_INTERP_CHECK(_v.size() == _cache.size());
    D_INTERP_CHECK(_v.size() == 3);
    D_INTERP_CHECK(!_v[0].is_key() && _v[0].m_span == "a");
    D_INTERP_CHECK(_v[1].is_key()  && _v[1].m_key == "b");
    D_INTERP_CHECK(!_v[2].is_key() && _v[2].m_span == "c");

    return true;
}


// a single-key cache round-trips name and raw
bool
test_replay_key_fidelity()
{
    const auto _cache =
        scan_all(::djinterp::brace_scanner<char>("{ name }"));
    const auto _v = scan_all(replay_t(_cache));

    D_INTERP_CHECK(_v.size() == 1);
    D_INTERP_CHECK(_v[0].is_key());
    D_INTERP_CHECK(_v[0].m_key == "name");
    D_INTERP_CHECK(_v[0].m_raw == "{ name }");

    return true;
}


// an empty cache yields no pieces
bool
test_replay_empty()
{
    const std::vector<::djinterp::piece<char>> _empty;
    const auto _v = scan_all(replay_t(_empty));

    D_INTERP_CHECK(_v.empty());

    return true;
}


// the scanner exposes the framework scanner typedefs
bool
test_replay_typedefs()
{
    static_assert(std::is_same<replay_t::input_type, char>::value, "input_type");
    static_assert(std::is_same<replay_t::result_type,
                               ::djinterp::piece<char>>::value, "result_type");

    return true;
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


dt::block_spec
replay_block()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    return dt::block_spec{
        "replay_scanner",
        "replaying a pre-scanned piece cache",
        {
            { "from_scan",     "replaying a cache reproduces the scan exactly",  &test_replay_from_scan },
            { "key_fidelity",  "a cached key round-trips its name and raw slice", &test_replay_key_fidelity },
            { "empty",         "an empty cache yields no pieces",                &test_replay_empty },
            { "typedefs",      "the scanner exposes the scanner typedefs",       &test_replay_typedefs }
        }
    };
#else
    return dt::block_spec{ "replay_scanner", "replay scanner (requires C++17)", {} };
#endif
}


NS_END  // testing
NS_END  // djinterp
