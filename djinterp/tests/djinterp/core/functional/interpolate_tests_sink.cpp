/******************************************************************************
* djinterp [test]                                        interpolate_tests_sink.cpp
*
* Section IV -- interp_string_sink: appends literal runs and resolved values
* into a caller-owned buffer.  Verifies literal() and value() (over both a
* string and a view), ordered accumulation, that the caller's buffer is
* appended to (never cleared), and that an empty run is a no-op.
******************************************************************************/

#include "interpolate_tests.hpp"


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// literal() appends the run
bool
test_sink_literal()
{
    std::string _out;
    ::djinterp::interp_string_sink<char> _s(_out);

    _s.literal(sv("abc"));

    D_INTERP_CHECK(_out == "abc");

    return true;
}


// value() appends, accepting both an owning string and a view
bool
test_sink_value()
{
    std::string _out;
    ::djinterp::interp_string_sink<char> _s(_out);

    _s.value(std::string("xy"));
    _s.value(sv("z"));

    D_INTERP_CHECK(_out == "xyz");

    return true;
}


// mixed literal / value calls accumulate in order
bool
test_sink_multiple()
{
    std::string _out;
    ::djinterp::interp_string_sink<char> _s(_out);

    _s.literal(sv("a"));
    _s.value(sv("B"));
    _s.literal(sv("c"));

    D_INTERP_CHECK(_out == "aBc");

    return true;
}


// the caller's buffer is appended to, not overwritten
bool
test_sink_reuse()
{
    std::string _out = "X";
    ::djinterp::interp_string_sink<char> _s(_out);

    _s.literal(sv("Y"));

    D_INTERP_CHECK(_out == "XY");

    return true;
}


// an empty run appends nothing
bool
test_sink_empty()
{
    std::string _out;
    ::djinterp::interp_string_sink<char> _s(_out);

    _s.literal(sv(""));
    _s.value(sv(""));

    D_INTERP_CHECK(_out.empty());

    return true;
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


dt::block_spec
sink_block()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    return dt::block_spec{
        "sink",
        "interp_string_sink literal / value assembly",
        {
            { "literal",  "literal() appends the run",                        &test_sink_literal },
            { "value",    "value() appends a string or a view",               &test_sink_value },
            { "multiple", "mixed literal / value calls accumulate in order",   &test_sink_multiple },
            { "reuse",    "the caller buffer is appended to, not cleared",     &test_sink_reuse },
            { "empty",    "an empty run appends nothing",                     &test_sink_empty }
        }
    };
#else
    return dt::block_spec{ "sink", "string sink (requires C++17)", {} };
#endif
}


NS_END  // testing
NS_END  // djinterp
