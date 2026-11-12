/*******************************************************************************
* djinterp [cpp]                                              djinterp_tests.hpp
*
*   Unit-test declarations and shared helpers for inc/cpp/djinterp.hpp.
* Every test entry point is a flat `bool tests_*()` in `djinterp::testing`;
* the section translation units define them and the runner drives them.
* A test returns `true` on success and `false` on the first failed check.
*
*
* path:      /tests/djinterp/cpp/djinterp_tests.hpp
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
#include "../../../inc/djinterp/cpp/djinterp.hpp"
#ifdef DTEST_SPEC_MODE
    #include "../../../inc/djinterp/c/modules/test/test_suite.h"  // the spec
#endif  // DTEST_SPEC_MODE


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

    throwing_move(
        const throwing_move&
    ) = default;
    throwing_move& operator=(
        const throwing_move&
    ) = default;

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

// III.v -- repeat_type / repeat_type_t (+ internal repeat_type_helper)
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


// ===========================================================================
//   the spec (DTEST_SPEC_MODE only)
// ===========================================================================
// The runner's face: the suite as DTest's C face tables. A C spec row takes
// a bool (void) test, which each tests_* is, so the suite runs without the
// C++ framework, which needs C++17.
#ifdef DTEST_SPEC_MODE

NS_DJINTERP
NS_TESTING

// djinterp_tests_macros_rows
//   constant: the macros block's rows: djinterp.hpp sections I and II.
static const struct dj_test_case_spec djinterp_tests_macros_rows[] =
{
    { "tests_macros_keywords",
      "the keyword macros expand to their documented tokens",
      &tests_macros_keywords },
    { "tests_macros_namespaces",
      "every NS_* macro opens its namespace and NS_END closes it",
      &tests_macros_namespaces },
    { "tests_macros_constexpr_qualifiers",
      "the D_CONSTEXPR family qualifies as documented",
      &tests_macros_constexpr_qualifiers },
    { "tests_macros_noexcept",
      "D_NOEXCEPT and D_NOEXCEPT_IF track their condition",
      &tests_macros_noexcept }
};

// djinterp_tests_void_t_rows
//   constant: the void_t block's rows.
static const struct dj_test_case_spec djinterp_tests_void_t_rows[] =
{
    { "tests_void_t_aliases_to_void",
      "void_t of any types is void",
      &tests_void_t_aliases_to_void },
    { "tests_void_t_detection_idiom",
      "void_t drives the detection idiom",
      &tests_void_t_detection_idiom }
};

// djinterp_tests_abs_value_rows
//   constant: the abs_value block's rows.
static const struct dj_test_case_spec djinterp_tests_abs_value_rows[] =
{
    { "tests_abs_value_struct",
      "abs_value<T, N> is |N| and keeps the type T",
      &tests_abs_value_struct },
    { "tests_abs_value_v",
      "abs_value_v equals abs_value<T, N>::value",
      &tests_abs_value_v },
    { "tests_abs_value_to_size_t",
      "abs_value_to_size_t is |N| as a std::size_t",
      &tests_abs_value_to_size_t }
};

// djinterp_tests_clean_rows
//   constant: the clean block's rows.
static const struct dj_test_case_spec djinterp_tests_clean_rows[] =
{
    { "tests_clean_basic",
      "clean strips references and top-level cv",
      &tests_clean_basic },
    { "tests_clean_edges",
      "only top-level cv goes; a pointer to const keeps its const",
      &tests_clean_edges },
    { "tests_clean_t_alias",
      "clean_t<T> is clean<T>::type",
      &tests_clean_t_alias }
};

// djinterp_tests_constexpr_swap_rows
//   constant: the constexpr_swap block's rows.
static const struct dj_test_case_spec djinterp_tests_constexpr_swap_rows[] =
{
    { "tests_constexpr_swap_runtime",
      "constexpr_swap exchanges trivial, allocating and user values",
      &tests_constexpr_swap_runtime },
    { "tests_constexpr_swap_constexpr",
      "constexpr_swap evaluates in a constant expression",
      &tests_constexpr_swap_constexpr },
    { "tests_constexpr_swap_noexcept",
      "constexpr_swap is noexcept exactly when the moves are",
      &tests_constexpr_swap_noexcept }
};

// djinterp_tests_repeat_rows
//   constant: the repeat block's rows.
static const struct dj_test_case_spec djinterp_tests_repeat_rows[] =
{
    { "tests_repeat_zero",
      "repeat_type<T, 0> is the empty tuple",
      &tests_repeat_zero },
    { "tests_repeat_counts",
      "repeat_type<T, N> is a tuple of N T",
      &tests_repeat_counts },
    { "tests_repeat_t_alias_and_types",
      "repeat_type_t<T, N> is repeat_type<T, N>::type and keeps T exactly",
      &tests_repeat_t_alias_and_types }
};

// djinterp_tests_self_rows
//   constant: the self block's rows.
static const struct dj_test_case_spec djinterp_tests_self_rows[] =
{
    { "tests_self_is_self_positive",
      "is_self holds for the self marker",
      &tests_self_is_self_positive },
    { "tests_self_is_self_negative",
      "is_self matches the marker exactly and nothing else",
      &tests_self_is_self_negative }
};

// djinterp_tests_resolve_self_rows
//   constant: the resolve_self block's rows.
static const struct dj_test_case_spec djinterp_tests_resolve_self_rows[] =
{
    { "tests_resolve_self_passthrough",
      "a type with no self marker is unchanged",
      &tests_resolve_self_passthrough },
    { "tests_resolve_self_marker",
      "a bare self resolves to the target",
      &tests_resolve_self_marker },
    { "tests_resolve_self_smart_pointers",
      "unique_ptr, shared_ptr and weak_ptr of self resolve",
      &tests_resolve_self_smart_pointers },
    { "tests_resolve_self_raw_pointer",
      "self* resolves to a pointer to the target",
      &tests_resolve_self_raw_pointer },
    { "tests_resolve_self_variadic_templates",
      "self resolves in any argument of any class template, nested",
      &tests_resolve_self_variadic_templates },
    { "tests_resolve_self_repeat",
      "repeat_type<self, N> resolves before it repeats",
      &tests_resolve_self_repeat },
    { "tests_resolve_self_limitations",
      "the documented limits: non-type parameters, cv and reference self",
      &tests_resolve_self_limitations }
};

// djinterp_tests_blocks
//   constant: the suite's blocks, in djinterp.hpp's order.
static const struct dj_test_block_spec djinterp_tests_blocks[] =
{
    { "macros", "keyword, namespace and qualifier macros",
      djinterp_tests_macros_rows,
      sizeof(djinterp_tests_macros_rows) /
          sizeof(djinterp_tests_macros_rows[0]) },
    { "void_t", "the void_t SFINAE sink",
      djinterp_tests_void_t_rows,
      sizeof(djinterp_tests_void_t_rows) /
          sizeof(djinterp_tests_void_t_rows[0]) },
    { "abs_value", "abs_value, abs_value_v and abs_value_to_size_t",
      djinterp_tests_abs_value_rows,
      sizeof(djinterp_tests_abs_value_rows) /
          sizeof(djinterp_tests_abs_value_rows[0]) },
    { "clean", "clean and clean_t",
      djinterp_tests_clean_rows,
      sizeof(djinterp_tests_clean_rows) /
          sizeof(djinterp_tests_clean_rows[0]) },
    { "constexpr_swap", "the move-only-safe swap",
      djinterp_tests_constexpr_swap_rows,
      sizeof(djinterp_tests_constexpr_swap_rows) /
          sizeof(djinterp_tests_constexpr_swap_rows[0]) },
    { "repeat", "repeat_type and repeat_type_t",
      djinterp_tests_repeat_rows,
      sizeof(djinterp_tests_repeat_rows) /
          sizeof(djinterp_tests_repeat_rows[0]) },
    { "self", "the self marker and is_self",
      djinterp_tests_self_rows,
      sizeof(djinterp_tests_self_rows) /
          sizeof(djinterp_tests_self_rows[0]) },
    { "resolve_self", "resolve_self and resolve_self_t",
      djinterp_tests_resolve_self_rows,
      sizeof(djinterp_tests_resolve_self_rows) /
          sizeof(djinterp_tests_resolve_self_rows[0]) }
};

// djinterp_tests_module
//   constant: the suite's one module.
static const struct dj_test_module_spec djinterp_tests_module =
{
    "djinterp", "djinterp.hpp: the C++ framework root",
    djinterp_tests_blocks,
    sizeof(djinterp_tests_blocks) / sizeof(djinterp_tests_blocks[0])
};

// djinterp_tests_spec
//   function: the suite's provider, for the runner.
inline const struct dj_test_module_spec*
djinterp_tests_spec()
{
    return &djinterp_tests_module;
}

NS_END  // testing
NS_END  // djinterp

#endif  // DTEST_SPEC_MODE


#endif  // DJINTERP_TESTS_
