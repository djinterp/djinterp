/******************************************************************************
* djinterp [test]                                     interpolate_tests_builder.cpp
*
* Section VII -- the lazy `interpolation` functor: a bound template plus a
* resolver chain carried in the type, forced by a terminal.  Verifies the
* interpolate() seed (identity passthrough), a single frame, the inline-bindings
* convenience overload, chained frames collapsing into one pass, frame priority,
* partial render, interpolate_if (open / closed gate), .recursive(), .into(),
* the string conversion, template_view() / resolver() accessors,
* make_interpolation, a non-default scanner, and constexpr construction.
******************************************************************************/

#include "interpolate_tests.hpp"


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// a seeded interpolation with no bindings reproduces the template
bool
test_builder_identity()
{
    const auto _i = ::djinterp::interpolate<char>(sv("{a}"));

    D_INTERP_CHECK(_i.str() == "{a}");
    D_INTERP_CHECK(_i.template_view() == "{a}");

    return true;
}


// a single appended resolver frame resolves its key
bool
test_builder_single_frame()
{
    const auto _r = ::djinterp::interpolate<char>(sv("{a}"))
                        .interpolate(::djinterp::map_resolver<char>{ {"a", "1"} })
                        .str();

    D_INTERP_CHECK(_r == "1");

    return true;
}


// the inline {key,value} overload is a convenience over the resolver form
bool
test_builder_bindings_convenience()
{
    const auto _r = ::djinterp::interpolate<char>(sv("{a}"))
                        .interpolate({ {"a", "1"} })
                        .str();

    D_INTERP_CHECK(_r == "1");

    return true;
}


// consecutive frames collapse into a single pass
bool
test_builder_chain_frames()
{
    const auto _r = ::djinterp::interpolate<char>(sv("{a}{b}"))
                        .interpolate({ {"a", "1"} })
                        .interpolate({ {"b", "2"} })
                        .str();

    D_INTERP_CHECK(_r == "12");

    return true;
}


// an earlier frame wins over a later one for the same key
bool
test_builder_frame_order()
{
    const auto _r = ::djinterp::interpolate<char>(sv("{a}"))
                        .interpolate({ {"a", "first"} })
                        .interpolate({ {"a", "second"} })
                        .str();

    D_INTERP_CHECK(_r == "first");

    return true;
}


// unknown keys pass through the whole chain
bool
test_builder_partial()
{
    const auto _r = ::djinterp::interpolate<char>(sv("{a}{b}"))
                        .interpolate({ {"a", "1"} })
                        .str();

    D_INTERP_CHECK(_r == "1{b}");

    return true;
}


// interpolate_if with an open gate resolves the key
bool
test_builder_if_open()
{
    const auto _r = ::djinterp::interpolate<char>(sv("{a}"))
                        .interpolate_if(pred_is_a{},
                            ::djinterp::map_resolver<char>{ {"a", "1"} })
                        .str();

    D_INTERP_CHECK(_r == "1");

    return true;
}


// interpolate_if with a closed gate leaves the placeholder
bool
test_builder_if_closed()
{
    const auto _r = ::djinterp::interpolate<char>(sv("{z}"))
                        .interpolate_if(pred_is_a{},
                            ::djinterp::map_resolver<char>{ {"z", "1"} })
                        .str();

    D_INTERP_CHECK(_r == "{z}");

    return true;
}


// .recursive() wraps the chain so resolved values are themselves interpolated
bool
test_builder_recursive()
{
    const auto _r = ::djinterp::interpolate<char>(sv("{a}"))
                        .interpolate({ {"a", "{b}"}, {"b", "B"} })
                        .recursive()
                        .str();

    D_INTERP_CHECK(_r == "B");

    return true;
}


// .into() renders into a caller-supplied sink
bool
test_builder_into()
{
    std::string _out;
    ::djinterp::interp_string_sink<char> _sink(_out);

    ::djinterp::interpolate<char>(sv("{a}"))
        .interpolate({ {"a", "1"} })
        .into(_sink);

    D_INTERP_CHECK(_out == "1");

    return true;
}


// conversion to a string forces a render (the value face)
bool
test_builder_conversion()
{
    const std::string _s =
        ::djinterp::interpolate<char>(sv("{a}")).interpolate({ {"a", "1"} });

    D_INTERP_CHECK(_s == "1");

    return true;
}


// template_view() and resolver() expose the bound state
bool
test_builder_accessors()
{
    const auto _i =
        ::djinterp::interpolate<char>(sv("{a}"))
            .interpolate(::djinterp::map_resolver<char>{ {"a", "1"} });

    D_INTERP_CHECK(_i.template_view() == "{a}");
    // the bound resolver chain is callable and resolves as expected
    D_INTERP_CHECK(_i.resolver()(sv("a")).found());
    D_INTERP_CHECK(_i.resolver()(sv("a")).value() == "1");

    return true;
}


// make_interpolation seeds with a resolver already bound
bool
test_builder_make_interpolation()
{
    const auto _r = ::djinterp::make_interpolation<char>(
                        sv("{a}"),
                        ::djinterp::map_resolver<char>{ {"a", "1"} })
                        .str();

    D_INTERP_CHECK(_r == "1");

    return true;
}


// the placeholder syntax is a scanner knob on the seed
bool
test_builder_custom_scanner()
{
    const auto _r =
        ::djinterp::interpolate<char, ::djinterp::sigil_scanner<char>>(sv("$a"))
            .interpolate({ {"a", "1"} })
            .str();

    D_INTERP_CHECK(_r == "1");

    return true;
}


// construction and template_view() are constant expressions
bool
test_builder_constexpr()
{
    constexpr auto _i = ::djinterp::interpolate<char>(sv("{a}"));
    static_assert(_i.template_view() == "{a}",
        "interpolate() + template_view() at compile time");

    return true;
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


dt::block_spec
builder_block()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    return dt::block_spec{
        "builder",
        "the lazy interpolation functor and its terminals",
        {
            { "identity",         "a seeded interpolation reproduces the template", &test_builder_identity },
            { "single_frame",     "a single frame resolves its key",              &test_builder_single_frame },
            { "bindings",         "the inline {k,v} overload is a convenience",    &test_builder_bindings_convenience },
            { "chain_frames",     "consecutive frames collapse into one pass",     &test_builder_chain_frames },
            { "frame_order",      "an earlier frame wins for the same key",        &test_builder_frame_order },
            { "partial",          "unknown keys pass through the chain",          &test_builder_partial },
            { "if_open",          "interpolate_if with an open gate resolves",     &test_builder_if_open },
            { "if_closed",        "interpolate_if with a closed gate passes",      &test_builder_if_closed },
            { "recursive",        ".recursive() interpolates resolved values",     &test_builder_recursive },
            { "into",             ".into() renders into a caller sink",           &test_builder_into },
            { "conversion",       "string conversion forces a render",            &test_builder_conversion },
            { "accessors",        "template_view() / resolver() expose state",     &test_builder_accessors },
            { "make",             "make_interpolation seeds with a resolver",      &test_builder_make_interpolation },
            { "custom_scanner",   "the placeholder syntax is a scanner knob",      &test_builder_custom_scanner },
            { "constexpr",        "construction + template_view() at compile time", &test_builder_constexpr }
        }
    };
#else
    return dt::block_spec{ "builder", "the lazy functor (requires C++17)", {} };
#endif
}


NS_END  // testing
NS_END  // djinterp
