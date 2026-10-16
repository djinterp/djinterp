/*******************************************************************************
* djinterp [core/testing]                                     djinterp_tests.hpp
*
*   Unit-test declarations and shared helpers for inc/cpp/djinterp.hpp.
* Every test entry point is a flat `bool tests_*()` in `djinterp::testing`;
* the section translation units define them and the runner drives them.
* A test returns `true` on success and `false` on the first failed check.
*
*
* path:      /tests/djinterp/core/djinterp_tests.hpp
* link(s):   TBA
* author(s): Claude                                          created: 2026.06.24
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_
#define DJINTERP_TESTS_ 1

// std
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../../inc/djinterp/djinterp.hpp"


// D_DT_CHECK
//   macro: evaluates a boolean expression as one check inside a tests_*
// function.  On failure it prints the file, line, and stringized expression
// and clears the caller's accumulator `_ok_var`; on success it does nothing.
// The accumulator (never the macro) is what the test ultimately returns, so a
// single function reports every failing check rather than stopping at the
// first.  It is variadic so an expression containing a top-level comma (e.g.
// a multi-argument template such as `is_same<A, B>`) passes through intact.
#define D_DT_CHECK(_ok_var, ...)                                              \
    do                                                                        \
    {                                                                         \
        if ( !(__VA_ARGS__) )                                                 \
        {                                                                     \
            std::printf("    [CHECK FAILED] %s:%d: %s\n",                     \
                        __FILE__,                                             \
                        __LINE__,                                             \
                        #__VA_ARGS__);                                        \
            (_ok_var) = false;                                                \
        }                                                                     \
    }                                                                         \
    while (false)


NS_DJINTERP
NS_TESTING

// ===========================================================================
//   shared helper types
// ===========================================================================

// probe_target
//   type: concrete stand-in that `self` markers are resolved to in the
// resolve_self tests.
struct probe_target
{};

// user_template
//   type: user-defined variadic class template used to verify that the
// resolve_self typename-only catch-all rewrites arbitrary user templates,
// not just std:: ones.
template<typename...>
struct user_template
{};

// throwing_move
//   type: copyable value type whose move operations are declared
// `noexcept(false)` (yet never actually throw), used to exercise the
// noexcept propagation of constexpr_swap and to swap a non-trivial type at
// runtime.
struct throwing_move
{
    int value;

    throwing_move() noexcept
        : value(0)
    {}

    explicit throwing_move(
        int _v
    ) noexcept
        : value(_v)
    {}

    throwing_move(const throwing_move&)            = default;
    throwing_move& operator=(const throwing_move&) = default;

    throwing_move(
        throwing_move&& _other
    ) noexcept(false)
        : value(_other.value)
    {}

    throwing_move&
    operator=(
        throwing_move&& _other
    ) noexcept(false)
    {
        value = _other.value;

        return *this;
    }
};


// ===========================================================================
//   test declarations (grouped by the section of djinterp.hpp they cover)
// ===========================================================================

// I & II -- keyword, namespace, and qualifier macros
bool tests_macros_keywords();
bool tests_macros_namespaces();
bool tests_macros_constexpr_qualifiers();
bool tests_macros_noexcept();

// III.i -- void_t
bool tests_void_t_aliases_to_void();
bool tests_void_t_detection_idiom();

// III.ii -- abs_value / abs_value_v / abs_value_to_size_t
bool tests_abs_value_struct();
bool tests_abs_value_v();
bool tests_abs_value_to_size_t();

// III.iii -- clean / clean_t
bool tests_clean_basic();
bool tests_clean_edges();
bool tests_clean_t_alias();

// III.iv -- constexpr_swap
bool tests_constexpr_swap_runtime();
bool tests_constexpr_swap_constexpr();
bool tests_constexpr_swap_noexcept();

// III.v -- repeat / repeat_t (+ internal repeat_type_helper)
bool tests_repeat_zero();
bool tests_repeat_counts();
bool tests_repeat_t_alias_and_types();

// III.vi.a-d -- self / is_self / is_self_v
bool tests_self_is_self_positive();
bool tests_self_is_self_negative();

// III.vi.e-f -- resolve_self / resolve_self_t (+ specializations)
bool tests_resolve_self_passthrough();
bool tests_resolve_self_marker();
bool tests_resolve_self_smart_pointers();
bool tests_resolve_self_raw_pointer();
bool tests_resolve_self_variadic_templates();
bool tests_resolve_self_repeat();
bool tests_resolve_self_limitations();

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_
