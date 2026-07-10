/******************************************************************************
* djinterp [test]                                    interpolate_tests_concepts.cpp
*
* Section IX (C++20) -- the concept parallels of the scanner / resolver / sink
* contracts: scanner_for, resolver_for, sink_for.  Verifies each accepts the
* shipped models and rejects an unrelated type (int).  Gated on C++20: below it
* the block is emitted empty.
******************************************************************************/

#include "interpolate_tests.hpp"


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP17_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

// scanner_for accepts every shipped scanner and rejects a non-scanner
bool
test_concepts_scanner_for()
{
    static_assert(::djinterp::scanner_for<::djinterp::brace_scanner<char>>,
        "brace_scanner models scanner_for");
    static_assert(::djinterp::scanner_for<::djinterp::sigil_scanner<char>>,
        "sigil_scanner models scanner_for");
    static_assert(::djinterp::scanner_for<::djinterp::replay_scanner<char>>,
        "replay_scanner models scanner_for");
    static_assert(!::djinterp::scanner_for<int>,
        "int is not a scanner");

    D_INTERP_CHECK(::djinterp::scanner_for<::djinterp::brace_scanner<char>>);
    D_INTERP_CHECK(::djinterp::scanner_for<::djinterp::sigil_scanner<char>>);
    D_INTERP_CHECK(::djinterp::scanner_for<::djinterp::replay_scanner<char>>);
    D_INTERP_CHECK(!::djinterp::scanner_for<int>);

    return true;
}


// resolver_for accepts the resolvers and rejects a non-resolver
bool
test_concepts_resolver_for()
{
    static_assert(::djinterp::resolver_for<::djinterp::map_resolver<char>>,
        "map_resolver models resolver_for");
    static_assert(::djinterp::resolver_for<::djinterp::empty_resolver<char>>,
        "empty_resolver models resolver_for");
    static_assert(
        ::djinterp::resolver_for<::djinterp::lookup_resolver<char, wrap_fn>>,
        "lookup_resolver models resolver_for");
    static_assert(!::djinterp::resolver_for<int>,
        "int is not a resolver");

    D_INTERP_CHECK(::djinterp::resolver_for<::djinterp::map_resolver<char>>);
    D_INTERP_CHECK(::djinterp::resolver_for<::djinterp::empty_resolver<char>>);
    D_INTERP_CHECK(!::djinterp::resolver_for<int>);

    return true;
}


// sink_for accepts the string sink and rejects a non-sink
bool
test_concepts_sink_for()
{
    static_assert(::djinterp::sink_for<::djinterp::interp_string_sink<char>>,
        "interp_string_sink models sink_for");
    static_assert(!::djinterp::sink_for<int>,
        "int is not a sink");

    D_INTERP_CHECK(::djinterp::sink_for<::djinterp::interp_string_sink<char>>);
    D_INTERP_CHECK(!::djinterp::sink_for<int>);

    return true;
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


dt::block_spec
concepts_block()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS
    return dt::block_spec{
        "concepts",
        "the scanner_for / resolver_for / sink_for contract concepts",
        {
            { "scanner_for",  "scanner_for accepts scanners, rejects int",   &test_concepts_scanner_for },
            { "resolver_for", "resolver_for accepts resolvers, rejects int",  &test_concepts_resolver_for },
            { "sink_for",     "sink_for accepts the sink, rejects int",      &test_concepts_sink_for }
        }
    };
#else
    return dt::block_spec{ "concepts", "contract concepts (requires C++20)", {} };
#endif
}


NS_END  // testing
NS_END  // djinterp
