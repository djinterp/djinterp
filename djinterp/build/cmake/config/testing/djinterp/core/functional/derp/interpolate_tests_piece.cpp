/******************************************************************************
* djinterp [test]                                       interpolate_tests_piece.cpp
*
* Section I -- scan-event vocabulary: piece_kind and the `piece` token every
* scanner emits (its default state, the literal vs key faces, is_key(), and the
* framework typedefs a piece exposes).
******************************************************************************/

#include "interpolate_tests.hpp"


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// a default-constructed piece is an empty literal
bool
test_piece_defaults()
{
    ::djinterp::piece<char> _p;

    D_INTERP_CHECK(!_p.is_key());
    D_INTERP_CHECK(_p.m_kind == ::djinterp::piece_kind::literal);
    D_INTERP_CHECK(_p.m_span.empty());
    D_INTERP_CHECK(_p.m_key.empty());
    D_INTERP_CHECK(_p.m_raw.empty());

    return true;
}


// a literal piece reports not-a-key and carries its span
bool
test_piece_literal()
{
    ::djinterp::piece<char> _p;
    _p.m_kind = ::djinterp::piece_kind::literal;
    _p.m_span = sv("hello");

    D_INTERP_CHECK(!_p.is_key());
    D_INTERP_CHECK(_p.m_span == "hello");

    return true;
}


// a key piece reports is_key and carries both the trimmed name and the raw slice
bool
test_piece_key()
{
    ::djinterp::piece<char> _p;
    _p.m_kind = ::djinterp::piece_kind::key;
    _p.m_key  = sv("name");
    _p.m_raw  = sv("{name}");

    D_INTERP_CHECK(_p.is_key());
    D_INTERP_CHECK(_p.m_key == "name");
    D_INTERP_CHECK(_p.m_raw == "{name}");

    return true;
}


// the piece exposes the framework token typedefs
bool
test_piece_typedefs()
{
    using piece_t = ::djinterp::piece<char>;

    static_assert(std::is_same<piece_t::char_type, char>::value,
        "piece char_type is the element type");
    static_assert(std::is_same<piece_t::value_type, sv>::value,
        "piece value_type is the view");
    static_assert(std::is_same<piece_t::kind_type,
                               ::djinterp::piece_kind>::value,
        "piece kind_type is piece_kind");
    static_assert(std::is_same<piece_t::view_type, sv>::value,
        "piece view_type is the view");

    // the two kinds are distinct
    D_INTERP_CHECK(::djinterp::piece_kind::literal !=
                   ::djinterp::piece_kind::key);

    return true;
}


// is_key() is a constant expression (the token is usable at compile time)
bool
test_piece_constexpr()
{
    constexpr ::djinterp::piece<char> _lit {};
    static_assert(!_lit.is_key(), "a default piece is a literal at compile time");

    return true;
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


dt::block_spec
piece_block()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    return dt::block_spec{
        "piece",
        "the piece scan-event token and piece_kind",
        {
            { "defaults",  "a default piece is an empty literal",                 &test_piece_defaults },
            { "literal",   "a literal piece reports not-a-key and carries a span", &test_piece_literal },
            { "key",       "a key piece reports is_key and carries name + raw",    &test_piece_key },
            { "typedefs",  "value_type is the view and kind_type is piece_kind",   &test_piece_typedefs },
            { "constexpr", "is_key() is usable in a constant expression",          &test_piece_constexpr }
        }
    };
#else
    return dt::block_spec{ "piece", "scan-event vocabulary (requires C++17)", {} };
#endif
}


NS_END  // testing
NS_END  // djinterp
