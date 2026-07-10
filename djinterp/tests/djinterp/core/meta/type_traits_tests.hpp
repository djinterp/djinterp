/******************************************************************************
* djinterp [test]                                          type_traits_tests.hpp
*
*   Test-suite header for the djinterp type_traits.hpp module.
*
*   Public surface:
*     - `type_traits_spec()` -- the whole suite as one module_spec (plain
*       data: a block of `{ name, descriptor, bool() }` unit tests, one per
*       section).  This is what the spec-based runner hands to run_module
*       (test_defaults.hpp), which lowers it into the six-kind test tree AND
*       projects it onto the console report and an optional PDF.  Each unit
*       test's `bool()` is a thin adapter (type_traits_detail::run_section)
*       over the corresponding per-section worker.
*     - per-section `void(test_handler&)` WORKERS, declared here and defined
*       in the .cpp files.  Each records many assertions but represents one
*       aggregate unit test; the spec adapter surfaces its pass/fail.
*     - a `type_traits_tests_all` aggregate driver (the legacy whole-suite
*       flow: one handler, sequential worker calls, AND-folded verdict),
*       compiled out under DTEST_SPEC_MODE since the spec-based runner does
*       not need it.
*     - a small library of helper types used across multiple sections so
*       each individual .cpp can stay focused on the trait under test.
*
*   DTEST_SPEC_MODE:
*   A runner that consumes the suite as data defines DTEST_SPEC_MODE before
* including this header (mirroring the other DTest suites).  In that mode
* the legacy aggregate driver is dropped and type_traits_spec() is the
* entry point.  The section .cpp files are compiled WITHOUT the macro and
* supply the worker definitions the spec's function pointers resolve to.
*
*   Per-section .cpp files implement the work in `void(test_handler&)`
* worker functions; they record their assertions directly via
* record_assertion().  `apply_options_to_handler` remains as the hook for
* the day the handler grows an option-application API.
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
* path:      /inc/djinterp/test/type_traits_tests.hpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/

#ifndef DJINTERP_TEST_TYPE_TRAITS_TESTS_
#define DJINTERP_TEST_TYPE_TRAITS_TESTS_ 1


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
#include <djinterp/core/djinterp.hpp>
#include <djinterp/test/test_handler.hpp>
#include <djinterp/test/test_options.hpp>
#include <djinterp/test/test_defaults.hpp>
#include <djinterp/core/meta/type_traits.hpp>
#include <djinterp/core/meta/dtuple.hpp>


NS_DJINTERP
NS_TEST

using namespace djinterp::test;

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
    noncopyable_t()                                    = default;
    noncopyable_t(const noncopyable_t&)                = delete;
    noncopyable_t& operator=(const noncopyable_t&)     = delete;
    noncopyable_t(noncopyable_t&&) noexcept            = default;
    noncopyable_t& operator=(noncopyable_t&&) noexcept = default;
    ~noncopyable_t()                                   = default;
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

    std::size_t
    size() const
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


// =========================================================================
// II.  OPTION FORWARDING HOOK
// =========================================================================

// apply_options_to_handler
//   helper: forwards an arbitrary pack of caller-supplied options
// into `_handler` before a section runs.  Each option is expected
// to be a recognized DTest option tag (see is_valid_test_option
// in test_concepts.hpp); the handler is expected to consume them
// in pack order.
//
//   Current state: the body is a sink expression that consumes
// every element of the pack with zero side effects.  This keeps
// the public per-section signature stable today while leaving a
// single, well-named extension point for the day test_handler
// grows an `apply_option(_Opt&&)` (or analogous) hook.  When that
// hook lands, replace the sink with the per-element forward and
// every per-section wrapper picks it up automatically.
//
//   The function template is well-formed on an empty pack
// (`sizeof...(_Options) == 0`) and on any combination of
// option-tag types, so callers may pass zero or many options
// without compile-time penalty.
template<typename... _Options>
inline void
apply_options_to_handler(
    test_handler&    _handler,
    _Options&&...    _options
)
{
    // sink the references so unused-parameter diagnostics stay
    // quiet on every supported standard.  On an empty pack the
    // initializer_list is empty and folds to a no-op; on a
    // non-empty pack each element is comma-discarded.
    (void)_handler;
    (void)std::initializer_list<int>{
        ((void)_options, 0)...
    };

    return;
}


// =========================================================================
// III. PER-SECTION WORKER DECLARATIONS  (defined in the .cpp files)
// =========================================================================

// Each worker has the uniform signature `void(test_handler&)` so a
// single function-pointer type drives them all from the runner.  The
// worker bodies record assertions directly via record_assertion(),
// which updates session_result counters on the handler.  Pass/fail is
// derived from the counter delta around each call (see the runner and
// type_traits_tests_all below) -- the workers do not return a verdict
// and they do not take options.  Options apply suite-wide and are
// consumed once by apply_options_to_handler() above.

void type_traits_tests_detection(test_handler& _test_handler);
void type_traits_tests_macros_core(test_handler& _test_handler);
void type_traits_tests_macros_member(test_handler& _test_handler);
void type_traits_tests_macros_op(test_handler& _test_handler);
void type_traits_tests_macros_spec(test_handler& _test_handler);
void type_traits_tests_logical(test_handler& _test_handler);
void type_traits_tests_callable(test_handler& _test_handler);
void type_traits_tests_cpp20(test_handler& _test_handler);
void type_traits_tests_cpp23(test_handler& _test_handler);
void type_traits_tests_evaluate(test_handler& _test_handler);
void type_traits_tests_rules(test_handler& _test_handler);
void type_traits_tests_traits(test_handler& _test_handler);
void type_traits_tests_arity(test_handler& _test_handler);
void type_traits_tests_template(test_handler& _test_handler);
void type_traits_tests_tuple(test_handler& _test_handler);


// =========================================================================
// IV.  MODULE SPEC  (the data view the spec-based runner consumes)
// =========================================================================
//   The framework's one-call runner (run_module in test_defaults.hpp) does
// not drive `void(test_handler&)` workers directly -- it consumes a
// module_spec: plain data describing a module as blocks of unit tests,
// each unit test a `{ name, descriptor, bool() }` triple.  run_module
// lowers that spec into the six-kind test tree AND projects it onto the
// report / PDF, so authoring the suite once as data feeds both views.
//
//   Each per-section worker here is one aggregate unit test (it records
// many assertions but reports a single pass/fail), which is exactly the
// shape a test_spec wants.  The only impedance is the signature: the spec
// needs `bool()`, the workers are `void(test_handler&)`.  run_section
// bridges the two -- it runs one worker against a private handler and
// returns whether that run added no failures or errors.  A function-pointer
// non-type template parameter gives one distinct thunk per worker without
// hand-writing fifteen wrappers.

namespace type_traits_detail
{

// run_section
//   adapter: presents one `void(test_handler&)` worker as a nullary
// `bool()` predicate.  Runs the worker against a fresh default_test_handler
// and returns true iff it recorded no new failures and no new errors.  The
// worker still pushes its per-assertion leaves into that private handler,
// so nothing about the section's own recording changes -- only the verdict
// is surfaced, which is all a test_spec consumes.
template<void (*_Worker)(test_handler&)>
inline bool
run_section()
{
    default_test_handler _handler;

    _Worker(_handler);

    return ( (_handler.failed() == 0) &&
             (_handler.errors() == 0) );
}

}  // namespace type_traits_detail


// type_traits_spec
//   provider: the whole type_traits suite as one module_spec -- a single
// block whose fifteen unit tests are the per-section workers, in the same
// declared order as the aggregate driver below.  Hand this to run_module
// (see the runner) to drive both the six-kind tree and the report / PDF.
//   Kept inline in the header so the runner needs only this file: the
// section .cpp translation units supply the worker definitions the
// function-pointer template arguments resolve to at link time.
inline module_spec
type_traits_spec()
{
    module_spec _module;

    _module.name       = "type_traits";
    _module.descriptor =
        "Compile-time metafunction module: the detection idiom (0.2), the "
        "trait-defining macro families (0.3, now in trait_detect.hpp), the "
        "portable standard-library traits (section I), and the djinterp "
        "custom trait surface (section III).";

    block_spec _block;

    _block.name       = "type_traits";
    _block.descriptor =
        "One unit test per semantic section; each aggregates its section's "
        "assertions into a single pass/fail verdict.";

    _block.tests = std::vector<test_spec>{
        { "detection",
          "0.2  detection idiom: nonesuch, detected_or/_t, is_detected[_v], "
          "is_detected_convertible, is_detected_exact",
          &type_traits_detail::run_section<&type_traits_tests_detection> },
        { "macros_core",
          "0.3  core macros: D_VOID_T, D_TYPE_TRAIT_TRUE[_AS/_FROM], "
          "D_TYPE_TRAIT_VALUE_BOOL, D_TYPE_TRAIT_TYPE_ALIAS",
          &type_traits_detail::run_section<&type_traits_tests_macros_core> },
        { "macros_member",
          "0.3  member macros: HAS_METHOD[_ARGS], HAS_TYPE, "
          "HAS_STATIC_MEMBER, MEMBER_TYPE_OR",
          &type_traits_detail::run_section<&type_traits_tests_macros_member> },
        { "macros_op",
          "0.3  operator macros: HAS_BINARY_OP, HAS_UNARY_OP, and the "
          "legacy HAS_METHOD_OF_TYPE family",
          &type_traits_detail::run_section<&type_traits_tests_macros_op> },
        { "macros_spec",
          "0.3  specialization macros: IS_SPECIALIZATION_OF[_AS]",
          &type_traits_detail::run_section<&type_traits_tests_macros_spec> },
        { "logical",
          "I.1 + II  bool_constant, conjunction, disjunction, negation, "
          "D_CONJUNCTION / D_DISJUNCTION / D_NEGATION",
          &type_traits_detail::run_section<&type_traits_tests_logical> },
        { "callable",
          "I.2  invoke_result, is_invocable[_r], is_nothrow_invocable[_r]",
          &type_traits_detail::run_section<&type_traits_tests_callable> },
        { "cpp20",
          "I.3  is_bounded_array, is_unbounded_array, remove_cvref, "
          "type_identity",
          &type_traits_detail::run_section<&type_traits_tests_cpp20> },
        { "cpp23",
          "I.4  is_scoped_enum",
          &type_traits_detail::run_section<&type_traits_tests_cpp23> },
        { "evaluate",
          "III  evaluate_types_for_trait, are_all_nonvoid, "
          "exclusive_disjunction",
          &type_traits_detail::run_section<&type_traits_tests_evaluate> },
        { "rules",
          "III  follows_rule_of_five / _three / _zero",
          &type_traits_detail::run_section<&type_traits_tests_rules> },
        { "traits",
          "III  has_max_size, has_nested_template_type, "
          "has_variadic_constructor, is_allocator, is_bounded, is_nonvoid, "
          "is_sized, is_valid_size_type",
          &type_traits_detail::run_section<&type_traits_tests_traits> },
        { "arity",
          "III  is_zero, is_nonzero, is_single_arg, is_single_type_arg",
          &type_traits_detail::run_section<&type_traits_tests_arity> },
        { "template",
          "III  is_template, is_template_with_args, "
          "is_template_parameter_base_of",
          &type_traits_detail::run_section<&type_traits_tests_template> },
        { "tuple",
          "III  first_arg, is_tuple, is_single_tuple_arg, to_tuple",
          &type_traits_detail::run_section<&type_traits_tests_tuple> }
    };

    _module.blocks.push_back(_block);

    return _module;
}


// =========================================================================
// V.   AGGREGATE DRIVER  (legacy whole-suite void() flow)
// =========================================================================
//   Runs every per-section worker in declared order against the
// supplied handler, applying any caller-supplied options to the
// handler once up front (the options apply to the suite as a
// whole, not to a single section).  Returns `true` iff no worker
// recorded any new failures or errors on the handler -- the
// workers themselves return `void`, so the verdict comes from
// the counter delta around the run.  Every section is run even
// after an earlier failure -- short-circuit logic would mask
// later-section regressions in the same run.
//
//   Retained for callers that still drive the suite the old way (one
// handler, sequential worker calls).  It is the "fixtures" the spec-mode
// runner does not need, so it is compiled out when DTEST_SPEC_MODE is
// defined -- the runner consumes type_traits_spec() instead.
#ifndef DTEST_SPEC_MODE

template<typename... _Options>
inline bool
type_traits_tests_all(
    test_handler&    _handler,
    _Options&&...    _options
)
{
    std::size_t _failed_before;
    std::size_t _errors_before;

    apply_options_to_handler(_handler,
                             std::forward<_Options>(_options)...);

    _failed_before = _handler.failed();
    _errors_before = _handler.errors();

    type_traits_tests_detection     (_handler);
    type_traits_tests_macros_core   (_handler);
    type_traits_tests_macros_member (_handler);
    type_traits_tests_macros_op     (_handler);
    type_traits_tests_macros_spec   (_handler);
    type_traits_tests_logical       (_handler);
    type_traits_tests_callable      (_handler);
    type_traits_tests_cpp20         (_handler);
    type_traits_tests_cpp23         (_handler);
    type_traits_tests_evaluate      (_handler);
    type_traits_tests_rules         (_handler);
    type_traits_tests_traits        (_handler);
    type_traits_tests_arity         (_handler);
    type_traits_tests_template      (_handler);
    type_traits_tests_tuple         (_handler);

    return ( (_handler.failed() == _failed_before) &&
             (_handler.errors() == _errors_before) );
}

#endif  // !DTEST_SPEC_MODE


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_TYPE_TRAITS_TESTS_
