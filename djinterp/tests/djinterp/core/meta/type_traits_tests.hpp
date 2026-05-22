/******************************************************************************
* djinterp [test]                                          type_traits_tests.hpp
*
*   Test-suite header for the djinterp type_traits.hpp module.
*
*   Public surface:
*     - per-section runner templates `type_traits_tests_<section>` which
*       each take a `test_handler&` and an optional variadic options pack;
*       any options supplied are forwarded to the handler before the
*       section's worker runs.  The runner returns `true` iff no new
*       failure or error counters were recorded during the worker call.
*     - a `type_traits_tests_all` aggregate driver in the same shape
*       which AND-folds every section's bool back to the caller
*     - a small library of helper types used across multiple sections so
*       each individual .cpp can stay focused on the trait under test
*
*   Per-section .cpp files implement the work in `` worker
* functions that take only a `test_handler&` and return `bool`; the
* inline wrappers below forward to those workers after applying any
* caller-supplied options via `apply_options_to_handler`.  The hook is
* in place; sections opt in to consuming options as the handler grows
* an option-application API.
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
// V.   AGGREGATE DRIVER
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


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_TYPE_TRAITS_TESTS_
