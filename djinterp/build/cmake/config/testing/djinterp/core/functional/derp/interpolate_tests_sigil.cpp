/******************************************************************************
* djinterp [test]                                       interpolate_tests_sigil.cpp
*
* Section II.ii -- sigil_scanner: the `$name` syntax.  Exercises plain literals,
* a single / mid-string / adjacent key, the doubled-sigil escape, a bare sigil
* (no name and at end-of-source) kept literal, a configurable sigil, the
* name-character set (alnum + underscore) and its boundary, and a compile-time
* scan.
******************************************************************************/

#include "interpolate_tests.hpp"


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP17_OR_HIGHER

using sigil_t = ::djinterp::sigil_scanner<char>;


// a format with no sigils is a single literal run
bool
test_sigil_plain()
{
    const auto _v = scan_all(sigil_t("hello"));

    D_INTERP_CHECK(_v.size() == 1);
    D_INTERP_CHECK(!_v[0].is_key() && _v[0].m_span == "hello");

    return true;
}


// a lone $name yields one key; raw includes the sigil
bool
test_sigil_single()
{
    const auto _v = scan_all(sigil_t("$name"));

    D_INTERP_CHECK(_v.size() == 1);
    D_INTERP_CHECK(_v[0].is_key());
    D_INTERP_CHECK(_v[0].m_key == "name");
    D_INTERP_CHECK(_v[0].m_raw == "$name");

    return true;
}


// a mid-string placeholder splits literal / key / literal
bool
test_sigil_mid()
{
    const auto _v = scan_all(sigil_t("a$b c"));

    D_INTERP_CHECK(_v.size() == 3);
    D_INTERP_CHECK(!_v[0].is_key() && _v[0].m_span == "a");
    D_INTERP_CHECK(_v[1].is_key()  && _v[1].m_key == "b");
    D_INTERP_CHECK(!_v[2].is_key() && _v[2].m_span == " c");

    return true;
}


// "$$" is an escaped literal sigil
bool
test_sigil_escaped()
{
    const auto _v = scan_all(sigil_t("a$$b"));

    D_INTERP_CHECK(_v.size() == 3);
    D_INTERP_CHECK(!_v[0].is_key() && _v[0].m_span == "a");
    D_INTERP_CHECK(!_v[1].is_key() && _v[1].m_span == "$");
    D_INTERP_CHECK(!_v[2].is_key() && _v[2].m_span == "b");

    return true;
}


// a bare sigil (no name follows) is kept literally
bool
test_sigil_bare()
{
    const auto _v = scan_all(sigil_t("$ x"));

    D_INTERP_CHECK(_v.size() == 1);
    D_INTERP_CHECK(!_v[0].is_key());
    D_INTERP_CHECK(_v[0].m_span == "$ x");

    return true;
}


// a sigil at end-of-source (no name) is kept literally
bool
test_sigil_at_end()
{
    const auto _v = scan_all(sigil_t("x$"));

    D_INTERP_CHECK(_v.size() == 1);
    D_INTERP_CHECK(!_v[0].is_key());
    D_INTERP_CHECK(_v[0].m_span == "x$");

    return true;
}


// the sigil character is configurable
bool
test_sigil_custom()
{
    const auto _v = scan_all(sigil_t("%name", '%'));

    D_INTERP_CHECK(_v.size() == 1);
    D_INTERP_CHECK(_v[0].is_key() && _v[0].m_key == "name");

    return true;
}


// adjacent placeholders yield two keys, no literal between
bool
test_sigil_adjacent()
{
    const auto _v = scan_all(sigil_t("$a$b"));

    D_INTERP_CHECK(_v.size() == 2);
    D_INTERP_CHECK(_v[0].is_key() && _v[0].m_key == "a");
    D_INTERP_CHECK(_v[1].is_key() && _v[1].m_key == "b");

    return true;
}


// a name is a maximal run of alnum + underscore
bool
test_sigil_name_chars()
{
    const auto _v = scan_all(sigil_t("$a_1B"));

    D_INTERP_CHECK(_v.size() == 1);
    D_INTERP_CHECK(_v[0].is_key() && _v[0].m_key == "a_1B");

    return true;
}


// the name stops at the first non-name character; the rest is literal
bool
test_sigil_name_boundary()
{
    const auto _v = scan_all(sigil_t("$name!"));

    D_INTERP_CHECK(_v.size() == 2);
    D_INTERP_CHECK(_v[0].is_key()  && _v[0].m_key == "name");
    D_INTERP_CHECK(!_v[1].is_key() && _v[1].m_span == "!");

    return true;
}


// the scanner exposes the framework scanner typedefs
bool
test_sigil_typedefs()
{
    static_assert(std::is_same<sigil_t::input_type, char>::value, "input_type");
    static_assert(std::is_same<sigil_t::result_type,
                               ::djinterp::piece<char>>::value, "result_type");

    return true;
}


// the scan is a constant expression
bool
test_sigil_constexpr()
{
    constexpr auto _scan = []() -> bool
    {
        sigil_t                 _s("$name");
        ::djinterp::piece<char> _p;

        return _s.next(_p) && _p.is_key() && (_p.m_key == "name") &&
               !_s.next(_p);
    };

    static_assert(_scan(), "sigil_scanner scans at compile time");
    D_INTERP_CHECK(_scan());

    return true;
}

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


dt::block_spec
sigil_block()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    return dt::block_spec{
        "sigil_scanner",
        "the $name placeholder syntax, escape, and configurable sigil",
        {
            { "plain",         "a no-sigil format is one literal run",           &test_sigil_plain },
            { "single",        "a lone $name yields one key incl. the sigil raw", &test_sigil_single },
            { "mid",           "a mid-string sigil splits literal / key / literal", &test_sigil_mid },
            { "escaped",       "$$ is an escaped literal sigil",                 &test_sigil_escaped },
            { "bare",          "a bare sigil with no name stays literal",        &test_sigil_bare },
            { "at_end",        "a sigil at end-of-source stays literal",         &test_sigil_at_end },
            { "custom",        "the sigil character is configurable",            &test_sigil_custom },
            { "adjacent",      "adjacent $a$b yield two keys",                   &test_sigil_adjacent },
            { "name_chars",    "a name spans alnum and underscore",              &test_sigil_name_chars },
            { "name_boundary", "the name stops at the first non-name char",       &test_sigil_name_boundary },
            { "typedefs",      "the scanner exposes the scanner typedefs",        &test_sigil_typedefs },
            { "constexpr",     "the scan is a constant expression",              &test_sigil_constexpr }
        }
    };
#else
    return dt::block_spec{ "sigil_scanner", "sigil scanner (requires C++17)", {} };
#endif
}


NS_END  // testing
NS_END  // djinterp
