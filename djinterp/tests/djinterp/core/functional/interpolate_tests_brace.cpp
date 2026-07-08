/******************************************************************************
* djinterp [test]                                       interpolate_tests_brace.cpp
*
* Section II.i -- brace_scanner: the `{key}` syntax.  Exercises the full edge
* surface: plain literals, single / trimmed / interleaved / adjacent keys, the
* {{ and }} escapes, an unmatched '{' and a lone '}' kept literal, the empty
* and whitespace-only key, an empty source, and a compile-time scan.
******************************************************************************/

#include "interpolate_tests.hpp"


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP17_OR_HIGHER

using brace_t = ::djinterp::brace_scanner<char>;


// a format with no placeholders is a single literal run
bool
test_brace_plain_literal()
{
    const auto _v = scan_all(brace_t("hello world"));

    D_INTERP_CHECK(_v.size() == 1);
    D_INTERP_CHECK(!_v[0].is_key());
    D_INTERP_CHECK(_v[0].m_span == "hello world");

    return true;
}


// a lone placeholder yields one key with name and raw slice
bool
test_brace_single_key()
{
    const auto _v = scan_all(brace_t("{name}"));

    D_INTERP_CHECK(_v.size() == 1);
    D_INTERP_CHECK(_v[0].is_key());
    D_INTERP_CHECK(_v[0].m_key == "name");
    D_INTERP_CHECK(_v[0].m_raw == "{name}");

    return true;
}


// interior whitespace is trimmed off the key name (raw keeps it)
bool
test_brace_trimmed_key()
{
    const auto _v = scan_all(brace_t("{ \t name \n }"));

    D_INTERP_CHECK(_v.size() == 1);
    D_INTERP_CHECK(_v[0].is_key());
    D_INTERP_CHECK(_v[0].m_key == "name");
    D_INTERP_CHECK(_v[0].m_raw == "{ \t name \n }");

    return true;
}


// literal / key / literal alternation, in order
bool
test_brace_interleaved()
{
    const auto _v = scan_all(brace_t("a{k}b"));

    D_INTERP_CHECK(_v.size() == 3);
    D_INTERP_CHECK(!_v[0].is_key() && _v[0].m_span == "a");
    D_INTERP_CHECK(_v[1].is_key()  && _v[1].m_key == "k");
    D_INTERP_CHECK(!_v[2].is_key() && _v[2].m_span == "b");

    return true;
}


// adjacent placeholders produce two keys with no literal between
bool
test_brace_adjacent_keys()
{
    const auto _v = scan_all(brace_t("{a}{b}"));

    D_INTERP_CHECK(_v.size() == 2);
    D_INTERP_CHECK(_v[0].is_key() && _v[0].m_key == "a");
    D_INTERP_CHECK(_v[1].is_key() && _v[1].m_key == "b");

    return true;
}


// "{{" is an escaped literal '{' (a one-char literal), flushing around it
bool
test_brace_escaped_open()
{
    const auto _v = scan_all(brace_t("a{{b"));

    D_INTERP_CHECK(_v.size() == 3);
    D_INTERP_CHECK(!_v[0].is_key() && _v[0].m_span == "a");
    D_INTERP_CHECK(!_v[1].is_key() && _v[1].m_span == "{");
    D_INTERP_CHECK(!_v[2].is_key() && _v[2].m_span == "b");

    return true;
}


// "}}" is an escaped literal '}'
bool
test_brace_escaped_close()
{
    const auto _v = scan_all(brace_t("a}}b"));

    D_INTERP_CHECK(_v.size() == 3);
    D_INTERP_CHECK(!_v[0].is_key() && _v[0].m_span == "a");
    D_INTERP_CHECK(!_v[1].is_key() && _v[1].m_span == "}");
    D_INTERP_CHECK(!_v[2].is_key() && _v[2].m_span == "b");

    return true;
}


// an unmatched '{' (no closing brace) is kept literally
bool
test_brace_unmatched_open()
{
    const auto _v = scan_all(brace_t("{unclosed"));

    D_INTERP_CHECK(_v.size() == 1);
    D_INTERP_CHECK(!_v[0].is_key());
    D_INTERP_CHECK(_v[0].m_span == "{unclosed");

    return true;
}


// a lone '}' (not doubled) stays literal
bool
test_brace_lone_close()
{
    const auto _v = scan_all(brace_t("a}b"));

    D_INTERP_CHECK(_v.size() == 1);
    D_INTERP_CHECK(!_v[0].is_key());
    D_INTERP_CHECK(_v[0].m_span == "a}b");

    return true;
}


// "{}" is a key with an empty name (the first '}' closes)
bool
test_brace_empty_key()
{
    const auto _v = scan_all(brace_t("{}"));

    D_INTERP_CHECK(_v.size() == 1);
    D_INTERP_CHECK(_v[0].is_key());
    D_INTERP_CHECK(_v[0].m_key.empty());
    D_INTERP_CHECK(_v[0].m_raw == "{}");

    return true;
}


// an all-whitespace key trims to the empty name
bool
test_brace_whitespace_key()
{
    const auto _v = scan_all(brace_t("{   }"));

    D_INTERP_CHECK(_v.size() == 1);
    D_INTERP_CHECK(_v[0].is_key());
    D_INTERP_CHECK(_v[0].m_key.empty());

    return true;
}


// an empty source yields no pieces at all
bool
test_brace_empty_source()
{
    const auto _v = scan_all(brace_t(""));

    D_INTERP_CHECK(_v.empty());

    return true;
}


// keys at the very start and end bracket an interior literal
bool
test_brace_start_and_end()
{
    const auto _v = scan_all(brace_t("{a}mid{b}"));

    D_INTERP_CHECK(_v.size() == 3);
    D_INTERP_CHECK(_v[0].is_key()  && _v[0].m_key == "a");
    D_INTERP_CHECK(!_v[1].is_key() && _v[1].m_span == "mid");
    D_INTERP_CHECK(_v[2].is_key()  && _v[2].m_key == "b");

    return true;
}


// the scanner exposes the framework scanner typedefs
bool
test_brace_typedefs()
{
    static_assert(std::is_same<brace_t::input_type, char>::value,
        "input_type is the element type");
    static_assert(std::is_same<brace_t::item_type,
                               ::djinterp::piece<char>>::value,
        "item_type is a piece");
    static_assert(std::is_same<brace_t::result_type,
                               ::djinterp::piece<char>>::value,
        "result_type is a piece");
    static_assert(std::is_same<brace_t::piece_type,
                               ::djinterp::piece<char>>::value,
        "piece_type is a piece");

    return true;
}


// the scan is a constant expression
bool
test_brace_constexpr()
{
    // evaluated at compile time; the runtime call re-confirms it
    constexpr auto _scan = []() -> bool
    {
        brace_t                 _s("a{k}");
        ::djinterp::piece<char> _p;

        bool _ok = _s.next(_p) && !_p.is_key() && (_p.m_span == "a");
        _ok = _ok && _s.next(_p) && _p.is_key() && (_p.m_key == "k") &&
              (_p.m_raw == "{k}");
        _ok = _ok && !_s.next(_p);

        return _ok;
    };

    static_assert(_scan(), "brace_scanner scans at compile time");
    D_INTERP_CHECK(_scan());

    return true;
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


dt::block_spec
brace_block()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    return dt::block_spec{
        "brace_scanner",
        "the {key} placeholder syntax, escapes, and edge cases",
        {
            { "plain_literal",  "a no-placeholder format is one literal run",       &test_brace_plain_literal },
            { "single_key",     "a lone {name} yields one key with name and raw",   &test_brace_single_key },
            { "trimmed_key",    "interior whitespace is trimmed from the name",     &test_brace_trimmed_key },
            { "interleaved",    "literal / key / literal alternate in order",       &test_brace_interleaved },
            { "adjacent_keys",  "{a}{b} yields two keys with no literal between",    &test_brace_adjacent_keys },
            { "escaped_open",   "{{ is an escaped literal open-brace",              &test_brace_escaped_open },
            { "escaped_close",  "}} is an escaped literal close-brace",            &test_brace_escaped_close },
            { "unmatched_open", "an unmatched { is kept literally",                &test_brace_unmatched_open },
            { "lone_close",     "a lone } stays literal",                          &test_brace_lone_close },
            { "empty_key",      "{} is a key with an empty name",                  &test_brace_empty_key },
            { "whitespace_key", "an all-whitespace key trims to empty",            &test_brace_whitespace_key },
            { "empty_source",   "an empty source yields no pieces",                &test_brace_empty_source },
            { "start_and_end",  "keys at the ends bracket an interior literal",     &test_brace_start_and_end },
            { "typedefs",       "the scanner exposes input/item/result typedefs",   &test_brace_typedefs },
            { "constexpr",      "the scan is a constant expression",               &test_brace_constexpr }
        }
    };
#else
    return dt::block_spec{ "brace_scanner", "brace scanner (requires C++17)", {} };
#endif
}


NS_END  // testing
NS_END  // djinterp
