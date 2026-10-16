/*******************************************************************************
* djinterp [core]                                      djinterp_tests_macros.cpp
*
*
* path:      /tests/djinterp/core/djinterp_tests_macros.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "djinterp_tests.hpp"


// D_DT_STR_ / D_DT_STR
//   macro: two-step stringizer so a keyword macro is fully expanded before it
// is turned into a string literal.
#define D_DT_STR_(_x) #_x
#define D_DT_STR(_x)  D_DT_STR_(_x)


// ---------------------------------------------------------------------------
//   namespace-macro probes
//     Each NS_* macro is exercised by opening the namespace it claims to open
//   and planting a uniquely-valued constant; the test then resolves that
//   constant at the fully-qualified path the macro promises.  A wrong
//   namespace name would fail to compile, so resolution IS the assertion.
// ---------------------------------------------------------------------------

D_NAMESPACE(dt_ns_generic_probe)
    constexpr int value = 101;
NS_END

NS_DATABASE
    constexpr int dt_ns_probe_database = 111;
NS_END

NS_DJINTERP
    constexpr int dt_ns_probe_djinterp = 112;

    NS_INTERNAL
        constexpr int dt_ns_probe_internal = 113;
    NS_END

    NS_TESTING
        constexpr int dt_ns_probe_testing = 114;
    NS_END
NS_END

NS_ERROR
    constexpr int dt_ns_probe_error = 115;
NS_END

NS_EXCEPTION
    constexpr int dt_ns_probe_exception = 116;
NS_END

NS_MATH
    constexpr int dt_ns_probe_math = 117;
NS_END

NS_MESSAGE
    constexpr int dt_ns_probe_message = 118;
NS_END

NS_PARADIGM
    constexpr int dt_ns_probe_paradigm = 119;
NS_END

NS_RESTD
    constexpr int dt_ns_probe_restd = 120;
NS_END

NS_TEST
    constexpr int dt_ns_probe_test = 121;
NS_END


NS_DJINTERP
NS_TESTING

// ---------------------------------------------------------------------------
//   qualifier-macro probes (compiled out when constexpr is intentionally
//   stripped via D_TESTING_CONSTEXPR == 1)
// ---------------------------------------------------------------------------
#if !( defined(D_TESTING_CONSTEXPR) && (D_TESTING_CONSTEXPR == 1) )

    D_CONSTEXPR        int dt_qual_plain  = 5;
    D_STATIC_CONSTEXPR int dt_qual_static  = 7;

    D_CONSTEXPR int
    dt_qual_triple(
        int _x
    )
    {
        return _x * 3;
    }

    D_CONSTEXPR_INLINE int
    dt_qual_inline_fn(
        int _x
    )
    {
        return _x + 1;
    }

    D_STATIC_CONSTEXPR_INLINE int
    dt_qual_static_inline_fn(
        int _x
    )
    {
        return _x + 2;
    }

    static_assert(dt_qual_plain == 5,
                  "D_CONSTEXPR variable must be a constant expression.");
    static_assert(dt_qual_static == 7,
                  "D_STATIC_CONSTEXPR variable must be a constant expression.");
    static_assert(dt_qual_triple(4) == 12,
                  "D_CONSTEXPR function must be a constant expression.");
    static_assert(dt_qual_inline_fn(4) == 5,
                  "D_CONSTEXPR_INLINE function must be a constant expression.");
    static_assert(dt_qual_static_inline_fn(4) == 6,
                  "D_STATIC_CONSTEXPR_INLINE function must be a constant "
                  "expression.");

#endif  // !D_TESTING_CONSTEXPR

// ---------------------------------------------------------------------------
//   noexcept-macro probes (only the C++11+ resolution is reachable here)
// ---------------------------------------------------------------------------
#if defined(__cplusplus) && D_ENV_LANG_IS_CPP11_OR_HIGHER

    void dt_noexcept_yes()        D_NOEXCEPT {}
    void dt_noexcept_unspecified()           {}
    void dt_noexcept_if_true()    D_NOEXCEPT_IF(true) {}
    void dt_noexcept_if_false()   D_NOEXCEPT_IF(false) {}
    void dt_noexcept_if_comma()   D_NOEXCEPT_IF(std::is_same<int, int>::value) {}

    static_assert(noexcept(dt_noexcept_yes()),
                  "D_NOEXCEPT must produce a non-throwing function.");
    static_assert(!noexcept(dt_noexcept_unspecified()),
                  "a function with no specifier must be potentially throwing.");
    static_assert(noexcept(dt_noexcept_if_true()),
                  "D_NOEXCEPT_IF(true) must be non-throwing.");
    static_assert(!noexcept(dt_noexcept_if_false()),
                  "D_NOEXCEPT_IF(false) must be potentially throwing.");
    static_assert(noexcept(dt_noexcept_if_comma()),
                  "D_NOEXCEPT_IF must forward a comma-bearing condition.");

#endif  // C++11+


/*
tests_macros_keywords
  Tests the following:
  - the framework keyword macros expand to the documented identifier tokens
  - covers keywords contributed by the C core (framework name, internal,
    testing, error, exception, math, message, test, database) and by the C++
    header (cpp, paradigm, re_std, stl, traits)
*/
bool
tests_macros_keywords()
{
    bool _ok = true;

    // C-core keywords
    D_DT_CHECK(_ok, std::strcmp(D_DT_STR(D_FRAMEWORK_NAME), "djinterp")  == 0);
    D_DT_CHECK(_ok, std::strcmp(D_DT_STR(D_KEYWORD_INTERNAL),       "internal")  == 0);
    D_DT_CHECK(_ok, std::strcmp(D_DT_STR(D_KEYWORD_TESTING),        "testing")   == 0);
    D_DT_CHECK(_ok, std::strcmp(D_DT_STR(D_KEYWORD_ERROR),          "error")     == 0);
    D_DT_CHECK(_ok, std::strcmp(D_DT_STR(D_KEYWORD_EXCEPTION),      "exception") == 0);
    D_DT_CHECK(_ok, std::strcmp(D_DT_STR(D_KEYWORD_MATH),           "math")      == 0);
    D_DT_CHECK(_ok, std::strcmp(D_DT_STR(D_KEYWORD_MESSAGE),        "message")   == 0);
    D_DT_CHECK(_ok, std::strcmp(D_DT_STR(D_KEYWORD_TEST),           "test")      == 0);
    D_DT_CHECK(_ok, std::strcmp(D_DT_STR(D_KEYWORD_DATABASE),       "database")  == 0);

    // C++-header keywords
    D_DT_CHECK(_ok, std::strcmp(D_DT_STR(D_KEYWORD_CPP),            "cpp")       == 0);
    D_DT_CHECK(_ok, std::strcmp(D_DT_STR(D_KEYWORD_PARADIGM),       "paradigm")  == 0);
    D_DT_CHECK(_ok, std::strcmp(D_DT_STR(D_KEYWORD_RE_STD),          "re_std")     == 0);
    D_DT_CHECK(_ok, std::strcmp(D_DT_STR(D_KEYWORD_STL),            "stl")       == 0);
    D_DT_CHECK(_ok, std::strcmp(D_DT_STR(D_KEYWORD_TRAITS),         "traits")    == 0);

    return _ok;
}

/*
tests_macros_namespaces
  Tests the following:
  - D_NAMESPACE opens an arbitrarily-named namespace and NS_END closes it
  - every NS_* macro opens a namespace of the documented name (database,
    djinterp, internal, testing, error, exception, math, message, paradigm,
    re_std, test), confirmed by resolving a probe at its fully-qualified path
  - nesting works: djinterp::internal and djinterp::testing built via macros
*/
bool
tests_macros_namespaces()
{
    bool _ok = true;

    D_DT_CHECK(_ok, ::dt_ns_generic_probe::value == 101);
    D_DT_CHECK(_ok, ::database::dt_ns_probe_database == 111);
    D_DT_CHECK(_ok, ::djinterp::dt_ns_probe_djinterp == 112);
    D_DT_CHECK(_ok, ::djinterp::internal::dt_ns_probe_internal == 113);
    D_DT_CHECK(_ok, ::djinterp::testing::dt_ns_probe_testing == 114);
    D_DT_CHECK(_ok, ::error::dt_ns_probe_error == 115);
    D_DT_CHECK(_ok, ::exception::dt_ns_probe_exception == 116);
    D_DT_CHECK(_ok, ::math::dt_ns_probe_math == 117);
    D_DT_CHECK(_ok, ::message::dt_ns_probe_message == 118);
    D_DT_CHECK(_ok, ::paradigm::dt_ns_probe_paradigm == 119);
    D_DT_CHECK(_ok, ::re_std::dt_ns_probe_restd == 120);
    D_DT_CHECK(_ok, ::test::dt_ns_probe_test == 121);

    return _ok;
}

/*
tests_macros_constexpr_qualifiers
  Tests the following:
  - D_CONSTEXPR, D_STATIC_CONSTEXPR, D_CONSTEXPR_INLINE, and
    D_STATIC_CONSTEXPR_INLINE yield entities usable in constant expressions
    (enforced at compile time by the static_asserts above and exercised at
    runtime here, including a genuine array-bound constant-expression context)
  Note: when D_TESTING_CONSTEXPR == 1 the constexpr qualifier is intentionally
  stripped, so these checks are compiled out in that configuration.
*/
bool
tests_macros_constexpr_qualifiers()
{
    bool _ok = true;

#if !( defined(D_TESTING_CONSTEXPR) && (D_TESTING_CONSTEXPR == 1) )
    D_DT_CHECK(_ok, dt_qual_plain == 5);
    D_DT_CHECK(_ok, dt_qual_static == 7);
    D_DT_CHECK(_ok, dt_qual_triple(4) == 12);
    D_DT_CHECK(_ok, dt_qual_inline_fn(4) == 5);
    D_DT_CHECK(_ok, dt_qual_static_inline_fn(4) == 6);

    // dt_qual_triple(2) must be a constant expression to size this array
    int _probe[dt_qual_triple(2)] = { 0 };
    D_DT_CHECK(_ok, (sizeof(_probe) / sizeof(_probe[0])) == 6);
    D_DT_CHECK(_ok, _probe[0] == 0);
#else
    (void)_ok;
#endif

    return _ok;
}

/*
tests_macros_noexcept
  Tests the following:
  - D_NOEXCEPT marks a function non-throwing, versus a bare function which is
    potentially throwing
  - D_NOEXCEPT_IF(true) / D_NOEXCEPT_IF(false) track their condition
  - D_NOEXCEPT_IF forwards a condition containing a top-level comma intact
  Note: only the C++11+ resolution (noexcept) is reachable in this build; the
  pre-C++11 (throw()) and C (no-op) branches are alternate compilations.
*/
bool
tests_macros_noexcept()
{
    bool _ok = true;

#if defined(__cplusplus) && D_ENV_LANG_IS_CPP11_OR_HIGHER
    D_DT_CHECK(_ok, noexcept(dt_noexcept_yes()) == true);
    D_DT_CHECK(_ok, noexcept(dt_noexcept_unspecified()) == false);
    D_DT_CHECK(_ok, noexcept(dt_noexcept_if_true()) == true);
    D_DT_CHECK(_ok, noexcept(dt_noexcept_if_false()) == false);
    D_DT_CHECK(_ok, noexcept(dt_noexcept_if_comma()) == true);
#else
    (void)_ok;
#endif

    return _ok;
}

NS_END  // testing
NS_END  // djinterp

#undef D_DT_STR
#undef D_DT_STR_
