/******************************************************************************
* djinterp [test]                                      djinterp_header_tests.hpp
*
*   Unit-test declarations for the djinterp C++ core header (djinterp.hpp).
* Declares one bool-returning test group per like-semantic section of the
* header, plus the small set of shared helper functions and helper types the
* per-section source files draw on.
*
*   COVERAGE INTENT:
*   The suite aims for full coverage of djinterp.hpp: every macro, every
* trait, every template specialization, and every value/branch is exercised.
* Compile-time guarantees are asserted with static_assert; the bool return of
* each group mirrors those guarantees at runtime so a harness can tally
* pass/fail.  Helper types live in the internal namespace per the style guide.
*
*   PORTABILITY:
*   C++11 minimum.  All standard-version-specific behaviour (variable
* templates, relaxed-constexpr swap) is gated on the env.h / env_cpp_features.h
* feature macros so the suite compiles and runs unchanged across C++11
* through C++23.
*
*
* TABLE OF CONTENTS
* =================
* I.    C++ KEYWORDS & NAMESPACE MACROS
* II.   CONSTEXPR / noexcept SUPPORT
* III.  CORE TYPE UTILITIES
*       i.   void_t
*       ii.  abs_value
*       iii. clean
*       iv.  constexpr_swap
*       v.   repeat
*       vi.  self / resolve_self
*
*
* path:      /inc/djinterp/test/djinterp_header_tests.hpp
* link(s):   TBA
* author(s): djinterp test suite                           created: 2026.05.21
******************************************************************************/

#ifndef DJINTERP_HEADER_TESTS_
#define DJINTERP_HEADER_TESTS_ 1

// std
#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../core/djinterp.hpp"


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                0.   SHARED HELPER FUNCTIONS & TYPES                      ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // streq
    //   helper: constexpr-capable C-string equality, used by the keyword
    // tests to compare stringized macro expansions both at compile time
    // (static_assert) and at runtime.  Written as a single-return recursion
    // so it is valid constexpr all the way back to C++11.
    constexpr bool streq(
        const char* _a,
        const char* _b
    )
    {
        return ( (*_a != *_b)
                 ? false
                 : ( (*_a == '\0')
                     ? true
                     : streq(_a + 1, _b + 1) ) );
    }

    // user_box
    //   type: a user-defined class template whose parameters are all types.
    // Used to prove resolve_self's variadic catch-all matches arbitrary
    // (non-std) class templates, not just standard-library ones.
    template<typename... _Args>
    struct user_box
    {};

    // nontype_box
    //   type: a class template carrying a non-type parameter.  resolve_self's
    // variadic catch-all only matches template<typename...> forms, so this
    // type must fall through to the passthrough primary template -- the
    // negative case the header documents.
    template<typename _Type,
             int      _N>
    struct nontype_box
    {};

    // move_only
    //   type: move-constructible / move-assignable but non-copyable, with a
    // payload that survives moves.  Exercises the move-based body of
    // constexpr_swap and confirms swap does not require copyability.
    struct move_only
    {
        int value;

        explicit move_only(
            int _v = 0
        ) noexcept
            : value(_v)
        {}

        move_only(const move_only&)            = delete;
        move_only& operator=(const move_only&) = delete;

        move_only(
            move_only&& _other
        ) noexcept
            : value(_other.value)
        {
            _other.value = -1;

            return;
        }

        move_only&
        operator=(
            move_only&& _other
        ) noexcept
        {
            value        = _other.value;
            _other.value = -1;

            return *this;
        }
    };

    // throwing_move
    //   type: a type whose move operations are not marked noexcept.  Used to
    // confirm constexpr_swap's computed noexcept specification is false for
    // such types and true for nothrow-movable ones.  The move operations are
    // user-provided (not defaulted) and carry no noexcept specifier, so they
    // are potentially-throwing -- defaulting them would let the compiler
    // recompute them as noexcept for this trivially-movable payload.
    struct throwing_move
    {
        int value;

        explicit throwing_move(
            int _v = 0
        )
            : value(_v)
        {}

        throwing_move(const throwing_move&)            = default;
        throwing_move& operator=(const throwing_move&) = default;

        throwing_move(
            throwing_move&& _other
        )
            : value(_other.value)
        {}

        throwing_move&
        operator=(
            throwing_move&& _other
        )
        {
            value = _other.value;

            return *this;
        }
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                I.   C++ KEYWORDS & NAMESPACE MACROS                      ///
///////////////////////////////////////////////////////////////////////////////

bool tests_keyword_macros();
bool tests_namespace_macros();


///////////////////////////////////////////////////////////////////////////////
///                II.  CONSTEXPR / noexcept SUPPORT                         ///
///////////////////////////////////////////////////////////////////////////////

bool tests_constexpr_macros();
bool tests_noexcept_macro();


///////////////////////////////////////////////////////////////////////////////
///                III. CORE TYPE UTILITIES                                  ///
///////////////////////////////////////////////////////////////////////////////

// i.   void_t
bool tests_void_t();

// ii.  abs_value
bool tests_abs_value();
bool tests_abs_value_v();
bool tests_abs_value_to_size_t();

// iii. clean
bool tests_clean();

// iv.  constexpr_swap
bool tests_constexpr_swap();

// v.   repeat
bool tests_repeat();

// vi.  self / resolve_self
bool tests_self_and_is_self();
bool tests_resolve_self();


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_HEADER_TESTS_
