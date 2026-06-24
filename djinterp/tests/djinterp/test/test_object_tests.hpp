/******************************************************************************
* djinterp [test]                                        test_object_tests.hpp
*
*   Declarations for the unit-test suite covering test_object.hpp.  Each free
* function exercises one facet of the header and returns true iff every check
* inside it passed.  Tests are grouped into translation units by the semantic
* section of test_object.hpp they cover:
*
*   - test_object_tests_metadata.cpp        -> test_metadata (helper class)
*   - test_object_tests_object.cpp          -> aliases, status constants,
*                                              storage, type-level traits
*   - test_object_tests_construction.cpp    -> the five constructors
*   - test_object_tests_protocol.cpp        -> the read-only query surface
*                                              (operator bool, status, result,
*                                              passed, failed, type_id,
*                                              callable_id, has_callable)
*   - test_object_tests_mutation.cpp        -> evaluate, skip, set_status,
*                                              set_type_id, set_callable_id
*   - test_object_tests_metadata_access.cpp -> metadata() / set_metadata()
*   - test_object_tests_aliases.cpp         -> basic_test, tagged_test
*   - test_object_tests_factory.cpp         -> make_test, make_interior
*
*   Helpers (all flat in djinterp::testing):
*   - test_object_check : reports a failing check, forwards the boolean.
*   - litmeta           : a *literal* metadata container, so test_object's
*                         constexpr ctors / accessors / (C++14+) mutators can
*                         be exercised in constant expressions -- which the
*                         default test_metadata (it owns a std::vector) cannot.
*   - throwmeta         : a metadata container whose special members are all
*                         potentially-throwing, used to confirm that
*                         test_object's D_NOEXCEPT_IF qualifiers propagate the
*                         container's exception specification.
*
*   NOTE: the entities under test live in djinterp::test; the tests themselves
* live, flat, in djinterp::testing.
*
*
* path:      /inc/djinterp/test/test_object_tests.hpp
* link(s):   TBA
* author(s): DTest contributors                            created: 2026.06.22
******************************************************************************/

#ifndef DJINTERP_TEST_OBJECT_TESTS_
#define DJINTERP_TEST_OBJECT_TESTS_ 1

// std
#include <cstdint>
#include <cstdio>
// djinterp
#include "test_object.hpp"


NS_DJINTERP
NS_TESTING


// test_object_check
//   function: reports the result of a single check.  When _condition is
// false, prints the stringized expression with its source location to stdout;
// always returns _condition so the caller can fold it into a running result.
inline bool
test_object_check(
    bool        _condition,
    const char* _expression,
    const char* _file,
    int         _line
)
{
    if (!_condition)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expression);
    }

    return _condition;
}


// litmeta
//   helper: a minimal *literal* metadata container.  Exposes the value_type
// alias test_object requires and is a literal type (constexpr ctors, trivial
// copy / destruction), so test_object<..., litmeta> is constexpr-friendly and
// trivially copyable.
struct litmeta
{
    using value_type = int;

    int tag;

    constexpr litmeta() D_NOEXCEPT
        : tag(0)
    {}

    constexpr explicit litmeta(int _tag) D_NOEXCEPT
        : tag(_tag)
    {}
};


// throwmeta
//   helper: a metadata container whose every special member is user-provided
// and therefore potentially-throwing (none are noexcept).  Used to verify
// that test_object's D_NOEXCEPT_IF qualifiers report noexcept(false) when the
// container's corresponding operation can throw.
struct throwmeta
{
    using value_type = int;

    throwmeta() {}
    throwmeta(const throwmeta&) {}
    throwmeta(throwmeta&&) {}
    throwmeta& operator=(const throwmeta&) { return *this; }
    throwmeta& operator=(throwmeta&&)      { return *this; }
};


// convenience instantiations used throughout the suite
using lit_object   = ::djinterp::test::test_object<std::uint8_t, litmeta>;
using throw_object = ::djinterp::test::test_object<std::uint8_t, throwmeta>;


// -- test_metadata (helper class) --------------------------------------------
bool tests_test_metadata_types();
bool tests_test_metadata_set();
bool tests_test_metadata_get();
bool tests_test_metadata_contains();

// -- aliases, status constants, storage, traits ------------------------------
bool tests_object_aliases();
bool tests_object_status_constants();
bool tests_object_storage();
bool tests_object_traits();

// -- construction ------------------------------------------------------------
bool tests_object_ctor_default();
bool tests_object_ctor_type_id();
bool tests_object_ctor_type_result();
bool tests_object_ctor_metadata();
bool tests_object_ctor_noexcept();

// -- read-only query surface -------------------------------------------------
bool tests_object_bool_conversion();
bool tests_object_status_result();
bool tests_object_passed_failed();
bool tests_object_type_id();
bool tests_object_callable_query();

// -- mutation ----------------------------------------------------------------
bool tests_object_evaluate();
bool tests_object_skip();
bool tests_object_set_status();
bool tests_object_set_type_id();
bool tests_object_set_callable_id();
bool tests_object_mutation_sequence();

// -- metadata access ---------------------------------------------------------
bool tests_object_metadata_accessor();
bool tests_object_set_metadata_copy();
bool tests_object_set_metadata_move();
bool tests_object_metadata_noexcept();

// -- convenience aliases -----------------------------------------------------
bool tests_basic_test();
bool tests_tagged_test();

// -- factory functions -------------------------------------------------------
bool tests_make_test();
bool tests_make_interior();
bool tests_make_interior_named();


NS_END  // testing
NS_END  // djinterp


// D_TO_CHECK
//   macro: evaluates its argument exactly once and routes it through
// test_object_check, capturing the expression text and source location.
// Yields the boolean result for accumulation at the call site.  Variadic so a
// condition containing a top-level comma (e.g. std::is_same<A, B>::value)
// passes through as a single argument.
#define D_TO_CHECK(...)                                                       \
    ::djinterp::testing::test_object_check(                                   \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)


#endif  // DJINTERP_TEST_OBJECT_TESTS_
