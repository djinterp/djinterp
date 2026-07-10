/******************************************************************************
* djinterp [test]                                    interpolate_tests_prepared.cpp
*
* Section VIII -- prepared_interpolation: scan ONCE into a shared piece cache
* and replay it every render.  Verifies the mirrored builder surface (identity,
* frame, interpolate_if, recursive, conversion, accessors), the hot-path
* resolver-argument terminals (str(resolver) / into(sink, resolver)) reused
* across renders, pieces() inspection, the shared cache (a chained prepared
* refers to the SAME cache), make_prepared, and prepare_into filling a
* caller-owned container for a replay_scanner.
******************************************************************************/

#include "interpolate_tests.hpp"


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// a prepared template with no bindings reproduces the template
bool
test_prepared_identity()
{
    D_INTERP_CHECK(::djinterp::prepare<char>(sv("{a}")).str() == "{a}");
    D_INTERP_CHECK(::djinterp::prepare<char>(sv("{a}")).template_view() == "{a}");

    return true;
}


// an appended frame resolves against the replayed cache
bool
test_prepared_frame()
{
    const auto _r = ::djinterp::prepare<char>(sv("{a}"))
                        .interpolate({ {"a", "1"} })
                        .str();

    D_INTERP_CHECK(_r == "1");

    return true;
}


// the hot path: supply the resolver per render, reusing one prepared object
bool
test_prepared_hot_path_str()
{
    const auto _p = ::djinterp::prepare<char>(sv("{a}"));

    D_INTERP_CHECK(_p.str(::djinterp::map_resolver<char>{ {"a", "1"} }) == "1");
    // same cache, a different resolver -- nothing re-scanned
    D_INTERP_CHECK(_p.str(::djinterp::map_resolver<char>{ {"a", "2"} }) == "2");

    return true;
}


// the hot path into a caller sink
bool
test_prepared_hot_path_into()
{
    const auto _p = ::djinterp::prepare<char>(sv("{a}"));

    std::string _out;
    ::djinterp::interp_string_sink<char> _sink(_out);
    _p.into(_sink, ::djinterp::map_resolver<char>{ {"a", "Q"} });

    D_INTERP_CHECK(_out == "Q");

    return true;
}


// pieces() exposes the cached scan: one entry per literal run / placeholder
bool
test_prepared_pieces()
{
    const auto _p = ::djinterp::prepare<char>(sv("a{b}c"));

    D_INTERP_CHECK(_p.pieces().size() == 3);
    D_INTERP_CHECK(!_p.pieces()[0].is_key() && _p.pieces()[0].m_span == "a");
    D_INTERP_CHECK(_p.pieces()[1].is_key()  && _p.pieces()[1].m_key == "b");
    D_INTERP_CHECK(!_p.pieces()[2].is_key() && _p.pieces()[2].m_span == "c");

    return true;
}


// interpolate_if is mirrored on the prepared surface
bool
test_prepared_if()
{
    const auto _r = ::djinterp::prepare<char>(sv("{a}"))
                        .interpolate_if(pred_true{},
                            ::djinterp::map_resolver<char>{ {"a", "1"} })
                        .str();

    D_INTERP_CHECK(_r == "1");

    return true;
}


// .recursive() is mirrored (the outer scan is cached; nested scans run live)
bool
test_prepared_recursive()
{
    const auto _r = ::djinterp::prepare<char>(sv("{a}"))
                        .interpolate({ {"a", "{b}"}, {"b", "B"} })
                        .recursive()
                        .str();

    D_INTERP_CHECK(_r == "B");

    return true;
}


// the string conversion forces a render
bool
test_prepared_conversion()
{
    const std::string _s =
        ::djinterp::prepare<char>(sv("{a}")).interpolate({ {"a", "1"} });

    D_INTERP_CHECK(_s == "1");

    return true;
}


// a chained prepared shares the SAME underlying cache (a refcount bump)
bool
test_prepared_shared_cache()
{
    const auto _p = ::djinterp::prepare<char>(sv("{a}"));

    // .interpolate returns a new prepared over the same cache object
    D_INTERP_CHECK(
        &_p.pieces() ==
        &_p.interpolate(::djinterp::map_resolver<char>{ {"a", "1"} }).pieces());

    return true;
}


// make_prepared scans once with a resolver already bound
bool
test_prepared_make()
{
    const auto _r = ::djinterp::make_prepared<char>(
                        sv("{a}"),
                        ::djinterp::map_resolver<char>{ {"a", "1"} })
                        .str();

    D_INTERP_CHECK(_r == "1");

    return true;
}


// prepare_into fills a caller-owned cache for a replay_scanner render
bool
test_prepared_prepare_into()
{
    std::vector<::djinterp::piece<char>> _cache;
    ::djinterp::prepare_into(_cache, sv("{a}b"));

    D_INTERP_CHECK(_cache.size() == 2);

    std::string _out;
    ::djinterp::interp_string_sink<char> _sink(_out);
    ::djinterp::interpolate_into(
        _sink,
        ::djinterp::replay_scanner<char>(_cache),
        ::djinterp::map_resolver<char>{ {"a", "1"} });

    D_INTERP_CHECK(_out == "1b");

    return true;
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


dt::block_spec
prepared_block()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    return dt::block_spec{
        "prepared",
        "prepared_interpolation: scan once, replay many",
        {
            { "identity",      "a prepared template with no bindings passes through", &test_prepared_identity },
            { "frame",         "an appended frame resolves against the cache",   &test_prepared_frame },
            { "hot_path_str",  "str(resolver) reuses one prepared across renders", &test_prepared_hot_path_str },
            { "hot_path_into", "into(sink, resolver) on the hot path",           &test_prepared_hot_path_into },
            { "pieces",        "pieces() exposes the cached scan",               &test_prepared_pieces },
            { "if",            "interpolate_if is mirrored",                    &test_prepared_if },
            { "recursive",     ".recursive() is mirrored (nested scans live)",    &test_prepared_recursive },
            { "conversion",    "the string conversion forces a render",          &test_prepared_conversion },
            { "shared_cache",  "a chained prepared shares the same cache",       &test_prepared_shared_cache },
            { "make",          "make_prepared binds a resolver up front",        &test_prepared_make },
            { "prepare_into",  "prepare_into fills a caller cache for replay",    &test_prepared_prepare_into }
        }
    };
#else
    return dt::block_spec{ "prepared", "prepared templates (requires C++17)", {} };
#endif
}


NS_END  // testing
NS_END  // djinterp
