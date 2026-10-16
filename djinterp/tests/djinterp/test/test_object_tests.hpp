/*******************************************************************************
* djinterp [test]                                          test_object_tests.hpp
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
* path:      /tests/djinterp/test/test_object_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.22
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTING_TEST_OBJECT_
#define DJINTERP_TESTING_TEST_OBJECT_ 1

// std
#include <cstdint>
#include <cstdio>
// djinterp
// djinterp  -- framework header first, then mode-gated.
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include <djinterp/test/test_object.hpp>
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"   // spec mode: module_spec + run_module
#endif


NS_DJINTERP
NS_TESTING

// dt names the entities under test (djinterp::test); the tests live here in
// djinterp::testing.  (Spec mode also uses it for dt::module_spec / run_module.)
namespace dt = ::djinterp::test;


#ifndef DTEST_SPEC_MODE  // fixtures + check helper: normal (section-file) mode only


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

#endif  // !DTEST_SPEC_MODE  (fixtures)


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


#ifdef DTEST_SPEC_MODE

// =========================================================================
//  suite spec provider (spec mode)
//    Descriptors are drawn from what each test actually asserts.
// =========================================================================

inline dt::module_spec
object_spec()
{
    return dt::module_spec{
        "test_object",
        "test_object.hpp - the unified test object: a node's type identity, "
        "evaluated result, terminal status, and metadata, plus the convenience "
        "aliases and factories that build them.",
        {
            dt::block_spec{
                "metadata",
                "test_metadata, the node's key-value store: its aliases and its "
                "set / get / contains surface.",
                {
                    { "tests_test_metadata_types",
                      "test_metadata exposes the expected value_type / key_type aliases",
                      &tests_test_metadata_types },
                    { "tests_test_metadata_set",
                      "set inserts a key-value pair and the size grows to match",
                      &tests_test_metadata_set },
                    { "tests_test_metadata_get",
                      "get returns the stored value, and the empty string for an absent key",
                      &tests_test_metadata_get },
                    { "tests_test_metadata_contains",
                      "contains reports whether a key is present",
                      &tests_test_metadata_contains },
                }
            },
            dt::block_spec{
                "object",
                "The object's type surface: nested aliases, the status constants "
                "and their values, the storage members, and the type traits.",
                {
                    { "tests_object_aliases",
                      "the nested aliases (status_type, metadata_type, ...) are the expected types",
                      &tests_object_aliases },
                    { "tests_object_status_constants",
                      "the status constants carry their documented integer values (passed = 0, ...)",
                      &tests_object_status_constants },
                    { "tests_object_storage",
                      "the storage members (result / status / id / metadata) have the expected types",
                      &tests_object_storage },
                    { "tests_object_traits",
                      "the type traits reflect the object's non-trivial, metadata-owning storage",
                      &tests_object_traits },
                }
            },
            dt::block_spec{
                "construction",
                "The five constructors: default, type-id, type + result, "
                "type + metadata, and their noexcept specifications.",
                {
                    { "tests_object_ctor_default",
                      "a default-constructed object is pending with a false result",
                      &tests_object_ctor_default },
                    { "tests_object_ctor_type_id",
                      "the type-id constructor stores the given kind id",
                      &tests_object_ctor_type_id },
                    { "tests_object_ctor_type_result",
                      "the type + result constructor stores the id and the evaluated result",
                      &tests_object_ctor_type_result },
                    { "tests_object_ctor_metadata",
                      "the metadata constructor stores the id and the supplied metadata",
                      &tests_object_ctor_metadata },
                    { "tests_object_ctor_noexcept",
                      "the constructors' noexcept specifications match the metadata container's",
                      &tests_object_ctor_noexcept },
                }
            },
            dt::block_spec{
                "protocol",
                "The read-only query surface: operator bool, status / result, "
                "passed / failed, type_id, and the callable query.",
                {
                    { "tests_object_bool_conversion",
                      "operator bool reflects whether the object passed",
                      &tests_object_bool_conversion },
                    { "tests_object_status_result",
                      "status() and result() report the outcome consistently",
                      &tests_object_status_result },
                    { "tests_object_passed_failed",
                      "passed() and failed() classify the terminal status",
                      &tests_object_passed_failed },
                    { "tests_object_type_id",
                      "type_id() returns the stored kind id",
                      &tests_object_type_id },
                    { "tests_object_callable_query",
                      "callable_id() / has_callable() report the bound callable, sentinel when none",
                      &tests_object_callable_query },
                }
            },
            dt::block_spec{
                "mutation",
                "The mutators: evaluate, skip, set_status, set_type_id, "
                "set_callable_id, and their composition.",
                {
                    { "tests_object_evaluate",
                      "evaluate(true) sets the result and drives the status to passed",
                      &tests_object_evaluate },
                    { "tests_object_skip",
                      "skip() marks the object skipped with a false result",
                      &tests_object_skip },
                    { "tests_object_set_status",
                      "set_status() forces the terminal status directly",
                      &tests_object_set_status },
                    { "tests_object_set_type_id",
                      "set_type_id() replaces the stored kind id",
                      &tests_object_set_type_id },
                    { "tests_object_set_callable_id",
                      "set_callable_id() binds a callable id to the node",
                      &tests_object_set_callable_id },
                    { "tests_object_mutation_sequence",
                      "a sequence of mutations composes, each observable in the final state",
                      &tests_object_mutation_sequence },
                }
            },
            dt::block_spec{
                "metadata_access",
                "metadata() and set_metadata (copy / move), with their noexcept "
                "specifications.",
                {
                    { "tests_object_metadata_accessor",
                      "metadata() exposes the object's metadata store for read and write",
                      &tests_object_metadata_accessor },
                    { "tests_object_set_metadata_copy",
                      "set_metadata copies a metadata container into the object",
                      &tests_object_set_metadata_copy },
                    { "tests_object_set_metadata_move",
                      "set_metadata moves a metadata container into the object",
                      &tests_object_set_metadata_move },
                    { "tests_object_metadata_noexcept",
                      "set_metadata's noexcept specification tracks the container's",
                      &tests_object_metadata_noexcept },
                }
            },
            dt::block_spec{
                "aliases",
                "The convenience instantiations.",
                {
                    { "tests_basic_test",
                      "basic_test is the default test_object<> instantiation",
                      &tests_basic_test },
                    { "tests_tagged_test",
                      "tagged_test is the metadata-carrying test_object alias",
                      &tests_tagged_test },
                }
            },
            dt::block_spec{
                "factory",
                "The object factories: make_test, make_interior, and the named "
                "interior form.",
                {
                    { "tests_make_test",
                      "make_test builds a leaf object with the given id and result",
                      &tests_make_test },
                    { "tests_make_interior",
                      "make_interior builds an interior (non-leaf) object",
                      &tests_make_interior },
                    { "tests_make_interior_named",
                      "make_interior with a name records the name in the object's metadata",
                      &tests_make_interior_named },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


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


#endif  // DJINTERP_TESTING_TEST_OBJECT_
