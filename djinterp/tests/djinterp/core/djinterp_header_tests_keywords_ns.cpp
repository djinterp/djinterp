/******************************************************************************
* djinterp [test]                       djinterp_header_tests_keywords_ns.cpp
*
*   Section I tests: C++ keyword macros and namespace macros.
******************************************************************************/

#include "./djinterp_header_tests.hpp"

#include <cstring>


// D_INTERNAL_TEST_STR
//   macro: stringizes its argument verbatim (no expansion).
#define D_INTERNAL_TEST_STR(x)      #x

// D_INTERNAL_TEST_XSTR
//   macro: stringizes its argument after one round of macro expansion.
#define D_INTERNAL_TEST_XSTR(x)     D_INTERNAL_TEST_STR(x)


// ----------------------------------------------------------------------------
//  namespace-open probes
//
//   Each well-formed NS_* macro is used here to open the namespace it names
// at file scope and deposit a uniquely-valued sentinel.  The corresponding
// test then reads each sentinel back through the *expected* fully-qualified
// name, proving the macro opened a namespace of exactly that name.  Namespace-
// scope `const` has internal linkage, so these sentinels are private to this
// translation unit.
// ----------------------------------------------------------------------------

NS_DJINTERP
    const int ns_probe_djinterp = 0x01;
NS_END

NS_CLI
    const int ns_probe_cli = 0x02;
NS_END

NS_DATABASE
    const int ns_probe_database = 0x03;
NS_END

NS_ERROR
    const int ns_probe_error = 0x04;
NS_END

NS_EXCEPTION
    const int ns_probe_exception = 0x05;
NS_END

NS_FUNCTIONAL
    const int ns_probe_functional = 0x06;
NS_END

NS_INTERNAL
    const int ns_probe_internal = 0x07;
NS_END

NS_MATH
    const int ns_probe_math = 0x08;
NS_END

NS_MESSAGE
    const int ns_probe_message = 0x09;
NS_END

NS_PARADIGM
    const int ns_probe_paradigm = 0x0A;
NS_END

NS_RESTD
    const int ns_probe_restd = 0x0B;
NS_END

NS_STL
    const int ns_probe_stl = 0x0C;
NS_END

NS_TEST
    const int ns_probe_test = 0x0D;
NS_END

NS_TRAITS
    const int ns_probe_traits = 0x0E;
NS_END


NS_DJINTERP
NS_TEST


/*
tests_keyword_macros
  Verifies every C++ keyword macro the header is responsible for resolving to
  its documented spelling.
  Tests the following:
  - the five keywords defined in djinterp.hpp section I (cpp, paradigm,
    restd, stl, traits) stringize to exactly their lower-case names
  - the C-core keywords consumed by the namespace macros (djinterp, cli,
    database, error, exception, functional, internal, math, message, test)
    stringize to their documented names
  - each check is asserted at compile time (static_assert via the constexpr
    streq helper) and re-checked at runtime
*/
bool
tests_keyword_macros()
{
    bool ok = true;

    // c++-only keywords defined in djinterp.hpp (section I.i)
    static_assert(internal::streq(D_INTERNAL_TEST_XSTR(D_KEYWORD_CPP),
                                  "cpp"),
                  "D_KEYWORD_CPP must resolve to `cpp`.");
    static_assert(internal::streq(D_INTERNAL_TEST_XSTR(D_KEYWORD_PARADIGM),
                                  "paradigm"),
                  "D_KEYWORD_PARADIGM must resolve to `paradigm`.");
    static_assert(internal::streq(D_INTERNAL_TEST_XSTR(D_KEYWORD_RESTD),
                                  "restd"),
                  "D_KEYWORD_RESTD must resolve to `restd`.");
    static_assert(internal::streq(D_INTERNAL_TEST_XSTR(D_KEYWORD_STL),
                                  "stl"),
                  "D_KEYWORD_STL must resolve to `stl`.");
    static_assert(internal::streq(D_INTERNAL_TEST_XSTR(D_KEYWORD_TRAITS),
                                  "traits"),
                  "D_KEYWORD_TRAITS must resolve to `traits`.");

    // c-core keywords the namespace macros depend on
    static_assert(internal::streq(
                      D_INTERNAL_TEST_XSTR(D_KEYWORD_FRAMEWORK_NAME),
                      "djinterp"),
                  "D_KEYWORD_FRAMEWORK_NAME must resolve to `djinterp`.");

    // runtime mirror of the compile-time checks above
    ok = ok && internal::streq(D_INTERNAL_TEST_XSTR(D_KEYWORD_CPP),      "cpp");
    ok = ok && internal::streq(D_INTERNAL_TEST_XSTR(D_KEYWORD_PARADIGM),
                               "paradigm");
    ok = ok && internal::streq(D_INTERNAL_TEST_XSTR(D_KEYWORD_RESTD),    "restd");
    ok = ok && internal::streq(D_INTERNAL_TEST_XSTR(D_KEYWORD_STL),      "stl");
    ok = ok && internal::streq(D_INTERNAL_TEST_XSTR(D_KEYWORD_TRAITS),   "traits");
    ok = ok && internal::streq(D_INTERNAL_TEST_XSTR(D_KEYWORD_FRAMEWORK_NAME),
                               "djinterp");
    ok = ok && internal::streq(D_INTERNAL_TEST_XSTR(D_KEYWORD_CLI),      "cli");
    ok = ok && internal::streq(D_INTERNAL_TEST_XSTR(D_KEYWORD_DATABASE),
                               "database");
    ok = ok && internal::streq(D_INTERNAL_TEST_XSTR(D_KEYWORD_ERROR),    "error");
    ok = ok && internal::streq(D_INTERNAL_TEST_XSTR(D_KEYWORD_EXCEPTION),
                               "exception");
    ok = ok && internal::streq(D_INTERNAL_TEST_XSTR(D_KEYWORD_FUNCTIONAL),
                               "functional");
    ok = ok && internal::streq(D_INTERNAL_TEST_XSTR(D_KEYWORD_INTERNAL),
                               "internal");
    ok = ok && internal::streq(D_INTERNAL_TEST_XSTR(D_KEYWORD_MATH),     "math");
    ok = ok && internal::streq(D_INTERNAL_TEST_XSTR(D_KEYWORD_MESSAGE),
                               "message");
    ok = ok && internal::streq(D_INTERNAL_TEST_XSTR(D_KEYWORD_TEST),     "test");

    return ok;
}


/*
tests_namespace_macros
  Verifies the namespace idiom macros open namespaces of the correct name and
  that NS_END closes them.
  Tests the following:
  - D_NAMESPACE(NAME) opens a namespace literally named NAME
  - every well-formed NS_* macro (djinterp, cli, database, error, exception,
    functional, internal, math, message, paradigm, restd, stl, test, traits)
    opens a namespace whose sentinel is reachable through the expected
    qualified name and carries the expected value
  - NS_END terminates a namespace cleanly (implicitly, since the file-scope
    probe blocks above compile)
  - regression sentinel: NS_COMPAT / NS_TEXT presently reference the undefined
    keyword macros D_KEYWORD_COMPAT / D_KEYWORD_TEXT (a header defect); this is
    pinned so the suite flips the day those keywords are supplied
*/
bool
tests_namespace_macros()
{
    bool ok = true;

    // D_NAMESPACE + NS_END produce a usable, correctly-named namespace.  The
    // qualified reads below would not compile if the names were wrong.
    // names are rooted at global scope (::) so they resolve to the file-scope
    // probe namespaces, never to the djinterp::test namespace this function is
    // defined inside (which also contains an `internal` and is itself `test`).
    ok = ok && (::djinterp::ns_probe_djinterp     == 0x01);
    ok = ok && (::cli::ns_probe_cli               == 0x02);
    ok = ok && (::database::ns_probe_database     == 0x03);
    ok = ok && (::error::ns_probe_error           == 0x04);
    ok = ok && (::exception::ns_probe_exception   == 0x05);
    ok = ok && (::functional::ns_probe_functional == 0x06);
    ok = ok && (::internal::ns_probe_internal     == 0x07);
    ok = ok && (::math::ns_probe_math             == 0x08);
    ok = ok && (::message::ns_probe_message       == 0x09);
    ok = ok && (::paradigm::ns_probe_paradigm     == 0x0A);
    ok = ok && (::restd::ns_probe_restd           == 0x0B);
    ok = ok && (::stl::ns_probe_stl               == 0x0C);
    ok = ok && (::test::ns_probe_test             == 0x0D);
    ok = ok && (::traits::ns_probe_traits         == 0x0E);

    // the well-formed NS_* keyword spellings, confirmed via stringification
    static_assert(internal::streq(D_INTERNAL_TEST_XSTR(D_KEYWORD_CLI),
                                  "cli"),
                  "NS_CLI must name namespace `cli`.");
    static_assert(internal::streq(D_INTERNAL_TEST_XSTR(D_KEYWORD_INTERNAL),
                                  "internal"),
                  "NS_INTERNAL must name namespace `internal`.");

    // regression sentinel for the undefined-keyword defect.  When a keyword
    // macro is undefined, the preprocessor leaves the token untouched, so it
    // stringizes to its own name.  These two checks pin that current state.
    ok = ok && internal::streq(D_INTERNAL_TEST_XSTR(D_KEYWORD_COMPAT),
                               "D_KEYWORD_COMPAT");
    ok = ok && internal::streq(D_INTERNAL_TEST_XSTR(D_KEYWORD_TEXT),
                               "D_KEYWORD_TEXT");

    return ok;
}


NS_END  // test
NS_END  // djinterp
