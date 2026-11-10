/*******************************************************************************
* djinterp [test]                                          type_traits_tests.hpp
*
*   Test-suite header for the djinterp type_traits.hpp module.
*
*   Public surface:
*     - `type_traits_spec()` -- the whole suite as one module_spec (plain
*       data: a block of `{ name, descriptor, bool() }` unit tests, one per
*       section), in spec mode only. The runner hands it to run_module
*       (test_defaults.hpp), which runs it and reports it, on the console
*       and in a PDF.
*     - per-section `bool()` WORKERS, declared here and defined in the .cpp
*       files. Each records many checks on a local verdict and returns it:
*       one aggregate unit test.
*     - a small library of helper types used across multiple sections so
*       each individual .cpp can stay focused on the trait under test
*       (normal mode only).
*
*   DTEST_SPEC_MODE:
*   The runner defines DTEST_SPEC_MODE before including this header
* (mirroring the other DTest suites); type_traits_spec() is its entry
* point. The section .cpp files are compiled WITHOUT the macro and supply
* the worker definitions the spec's function pointers resolve to.
*
*   Section coverage map (see the individual .cpp files for detail):
*
*     0.2  detection idiom         -> type_traits_tests_detection.cpp
*     0.3  core detection macros   -> type_traits_tests_macros_core.cpp
*     0.3  member detection macros -> type_traits_tests_macros_member.cpp
*     0.3  operator detection      -> type_traits_tests_macros_op.cpp
*     0.3  specialization macros   -> type_traits_tests_macros_spec.cpp
*     I.1 + II  logical metafuncs  -> type_traits_tests_logical.cpp
*     I.2  callable traits         -> type_traits_tests_callable.cpp
*     I.3  C++20 features          -> type_traits_tests_cpp20.cpp
*     I.4  C++23 features          -> type_traits_tests_cpp23.cpp
*     III  evaluate/disjunction    -> type_traits_tests_evaluate.cpp
*     III  rules-of-N              -> type_traits_tests_rules.cpp
*     III  has_/is_ "container"    -> type_traits_tests_traits.cpp
*     III  is_zero, is_single_arg  -> type_traits_tests_arity.cpp
*     III  is_template family      -> type_traits_tests_template.cpp
*     III  tuple-meta (first_arg,  -> type_traits_tests_tuple.cpp
*           is_tuple, is_single_tuple_arg, to_tuple)
*
*
* path:      /tests/djinterp/cpp/meta/type_traits_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.20
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_TEST_TYPE_TRAITS_TESTS_
#define DJINTERP_TEST_TYPE_TRAITS_TESTS_ 1


// std
#include <cstdio>
#include <array>
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/meta/type_traits.hpp"
#include "../../../../inc/djinterp/cpp/meta/dtuple.hpp"
#endif
#ifdef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/modules/test/test_defaults.hpp"  // module_spec, run_module
#endif


// the name ::djinterp::test exists in both faces for the dt alias below
// (test_defaults.hpp declares it in the spec face; this reopens it)
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

// dt names DTest; unconditional, the spec provider needs it
namespace dt = ::djinterp::test;

#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only

// record_assertion
//   function: one check of a converted body: on failure it prints what was
// expected and clears the body's verdict.  (The bodies used DTest's retired
// handler API, record_assertion(handler, condition, what).)
inline
void record_assertion(
    bool&       _ok,
    bool        _condition,
    const char* _what
)
{
    if (!_condition)
    {
        std::printf("    ! %s\n", _what);
        _ok = false;
    }

    return;
}


// record_assertion (with detail)
//   function: the five-argument form the bodies also used: the detail and
// the extra text are printed with a failure.
inline
void record_assertion(
    bool&       _ok,
    bool        _condition,
    const char* _what,
    const char* _detail,
    const char* _extra = nullptr
)
{
    if (!_condition)
    {
        std::printf("    ! %s (%s%s%s)\n",
                    _what,
                    _detail ? _detail : "",
                    _extra ? "; " : "",
                    _extra ? _extra : "");
        _ok = false;
    }

    return;
}

// DUnitTestTally
//   struct: kept for the converted bodies' run_unit_test calls; a body's
// verdict is its bool now, so the tally holds nothing.
struct DUnitTestTally
{
};

// run_unit_test
//   function: runs one named unit inside a converted body, and prints its
// name when it was the unit that first failed.
template<typename Fn>
void run_unit_test(
    bool&           _ok,
    DUnitTestTally& _tally,
    const char*     _name,
    Fn              _unit
)
{
    const bool before = _ok;

    (void)_tally;
    _unit();

    if ( (before) &&
         (!_ok) )
    {
        std::printf("    [FAIL] %s\n", _name);
    }

    return;
}

// D_TYPE_TRAITS_CHECK
//   macro: one check of a converted body, its text the message.
#define D_TYPE_TRAITS_CHECK(...) \
    record_assertion(verdict, (__VA_ARGS__), #__VA_ARGS__)


// =========================================================================
// I.   SHARED HELPER TYPES
// =========================================================================
//   These types are referenced from more than one .cpp file in the suite.
// Narrow per-section helpers live in the .cpp files themselves, inside
// anonymous namespaces, to keep this header from becoming a bag of
// loosely-related fixtures.

namespace type_traits_test_types
{

// trivial_t
//   class: all special member functions trivially-implemented (Rule of
// Zero). Useful as the "boring well-behaved" baseline for many traits.
struct trivial_t
{
    int a;
    char b;
};

// full_special_member_t
//   class: all five special member functions explicitly user-defined and
// well-formed (Rule of Five). Used as the positive case for
// follows_rule_of_five and the negative case for follows_rule_of_zero.
struct full_special_member_t
{
    int data;

    full_special_member_t()
        : data(0)
    {
        return;
    }

    full_special_member_t(const full_special_member_t&  _other)
        : data(_other.data)
    {
        return;
    }

    full_special_member_t(full_special_member_t&& _other) noexcept
        : data(_other.data)
    {
        return;
    }

    full_special_member_t&
    operator=(const full_special_member_t& _other)
    {
        data = _other.data;
        return *this;
    }

    full_special_member_t&
    operator=(full_special_member_t&& _other) noexcept
    {
        data = _other.data;
        return *this;
    }

    ~full_special_member_t()
    {
        return;
    }
};

// copy_only_t
//   class: copy constructor + copy-assignment + destructor user-defined
// (no move). Positive case for follows_rule_of_three, negative case for
// follows_rule_of_five when the move operations are implicitly deleted.
struct copy_only_t
{
    int data;

    copy_only_t()
        : data(0)
    {
        return;
    }

    copy_only_t(const copy_only_t& _other)
        : data(_other.data)
    {
        return;
    }

    copy_only_t&
    operator=(const copy_only_t& _other)
    {
        data = _other.data;
        return *this;
    }

    ~copy_only_t()
    {
        return;
    }
};

// noncopyable_t
//   class: copy operations deleted. follows_rule_of_* should all be false
// because the copy operations are removed from the candidate set entirely.
struct noncopyable_t
{
    noncopyable_t() = default;
    noncopyable_t(
        const noncopyable_t&
    ) = delete;
    noncopyable_t& operator=(
        const noncopyable_t&
    ) = delete;
    noncopyable_t(
        noncopyable_t&&
    ) noexcept = default;
    noncopyable_t& operator=(
        noncopyable_t&&
    ) noexcept = default;
    ~noncopyable_t() = default;
};

// with_value_type
//   class: exposes a `value_type` nested type-alias (= int). Used by
// allocator-shaped detection traits and by HAS_TYPE testers.
struct with_value_type
{
    using value_type = int;
};

// with_size_and_size_type
//   class: exposes `size_type = std::size_t` AND a `size() -> std::size_t`
// method. Positive case for is_sized.
struct with_size_and_size_type
{
    using size_type = std::size_t;

    std::size_t size() const
    {
        return 0;
    }
};

// scoped_enum_t / unscoped_enum_t
//   enums: one of each kind. Used by is_scoped_enum tests and as
// reference points elsewhere.
enum class scoped_enum_t
{
    a = 0,
    b = 1
};

enum unscoped_enum_t
{
    unscoped_a = 0,
    unscoped_b = 1
};

}  // namespace type_traits_test_types


#endif  // !DTEST_SPEC_MODE  (fixtures)


// =========================================================================
// III. PER-SECTION WORKER DECLARATIONS  (defined in the .cpp files)
// =========================================================================

// Each worker is a `bool()` unit test: it records its section's checks on
// a local verdict and returns it. The spec provider (section IV) lists them
// for run_module.

bool type_traits_tests_detection();
bool type_traits_tests_macros_core();
bool type_traits_tests_macros_member();
bool type_traits_tests_macros_op();
bool type_traits_tests_macros_spec();
bool type_traits_tests_logical();
bool type_traits_tests_callable();
bool type_traits_tests_cpp20();
bool type_traits_tests_cpp23();
bool type_traits_tests_evaluate();
bool type_traits_tests_rules();
bool type_traits_tests_traits();
bool type_traits_tests_arity();
bool type_traits_tests_template();
bool type_traits_tests_tuple();


// =========================================================================
// IV.  MODULE SPEC  (the data view the spec-based runner consumes)
// =========================================================================
//   The framework's one-call runner (run_module in test_defaults.hpp)
// consumes a module_spec: plain data describing a module as blocks of unit
// tests, each a `{ name, descriptor, bool() }` triple. Each per-section
// worker is one aggregate unit test. The provider exists in spec mode only;
// the section .cpp files, built without DTEST_SPEC_MODE, define the
// workers.
#ifdef DTEST_SPEC_MODE


// type_traits_spec
//   provider: the whole type_traits suite as one module_spec -- a single
// block whose fifteen unit tests are the per-section workers, in the same
// declared order as the aggregate driver below.  Hand this to run_module
// (see the runner) to drive both the six-kind tree and the report / PDF.
//   Kept inline in the header so the runner needs only this file: the
// section .cpp translation units supply the worker definitions the
// function-pointer template arguments resolve to at link time.
inline
dt::module_spec type_traits_spec()
{
    dt::module_spec _module;

    _module.name       = "type_traits";
    _module.descriptor =
        "Compile-time metafunction module: the detection idiom (0.2), the "
        "trait-defining macro families (0.3, now in trait_detect.hpp), the "
        "portable standard-library traits (section I), and the djinterp "
        "custom trait surface (section III).";

    dt::block_spec _block;

    _block.name       = "type_traits";
    _block.descriptor =
        "One unit test per semantic section; each aggregates its section's "
        "assertions into a single pass/fail verdict.";

    _block.tests = std::vector<dt::test_spec>{
        { "detection",
          "0.2  detection idiom: nonesuch, detected_or/_t, is_detected[_v], "
          "is_detected_convertible, is_detected_exact",
          &type_traits_tests_detection },
        { "macros_core",
          "0.3  core macros: D_VOID_T, D_TYPE_TRAIT_TRUE[_AS/_FROM], "
          "D_TYPE_TRAIT_VALUE_BOOL, D_TYPE_TRAIT_TYPE_ALIAS",
          &type_traits_tests_macros_core },
        { "macros_member",
          "0.3  member macros: HAS_METHOD[_ARGS], HAS_TYPE, "
          "HAS_STATIC_MEMBER, MEMBER_TYPE_OR",
          &type_traits_tests_macros_member },
        { "macros_op",
          "0.3  operator macros: HAS_BINARY_OP, HAS_UNARY_OP, and the "
          "legacy HAS_METHOD_OF_TYPE family",
          &type_traits_tests_macros_op },
        { "macros_spec",
          "0.3  specialization macros: IS_SPECIALIZATION_OF[_AS]",
          &type_traits_tests_macros_spec },
        { "logical",
          "I.1 + II  bool_constant, conjunction, disjunction, negation, "
          "D_CONJUNCTION / D_DISJUNCTION / D_NEGATION",
          &type_traits_tests_logical },
        { "callable",
          "I.2  invoke_result, is_invocable[_r], is_nothrow_invocable[_r]",
          &type_traits_tests_callable },
        { "cpp20",
          "I.3  is_bounded_array, is_unbounded_array, remove_cvref, "
          "type_identity",
          &type_traits_tests_cpp20 },
        { "cpp23",
          "I.4  is_scoped_enum",
          &type_traits_tests_cpp23 },
        { "evaluate",
          "III  evaluate_types_for_trait, are_all_nonvoid, "
          "exclusive_disjunction",
          &type_traits_tests_evaluate },
        { "rules",
          "III  follows_rule_of_five / _three / _zero",
          &type_traits_tests_rules },
        { "traits",
          "III  has_max_size, has_nested_template_type, "
          "has_variadic_constructor, is_allocator, is_bounded, is_nonvoid, "
          "is_sized, is_valid_size_type",
          &type_traits_tests_traits },
        { "arity",
          "III  is_zero, is_nonzero, is_single_arg, is_single_type_arg",
          &type_traits_tests_arity },
        { "template",
          "III  is_template, is_template_with_args, "
          "is_template_parameter_base_of",
          &type_traits_tests_template },
        { "tuple",
          "III  first_arg, is_tuple, is_single_tuple_arg, to_tuple",
          &type_traits_tests_tuple }
    };

    _module.blocks.push_back(_block);

    return _module;
}


#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_TYPE_TRAITS_TESTS_
