/*******************************************************************************
* djinterp [testing]                                      fixed_string_tests.hpp
*
*   Unit-test surface for the structural string-literal carrier (core/meta/
* fixed_string.hpp).  This header carries ONLY declarations and the fixtures
* the section TUs share; every `tests_fixed_string_*` function is defined in
* its own section .cpp and reported by fixed_string_tests_runner.cpp.
*
*   WHAT IS TESTED, AND HOW:
*   The module's reason for existing is a claim the type system cannot be
* asked about directly.  "fixed_string<N> is a structural type, so it may
* be a C++20 class-type NTTP" has no corresponding trait - there is no
* std::is_structural - so the only way to state it is to USE the type in
* every template-parameter position and let the build decide.  Section IV
* does exactly that, through four vehicles (fs_holder, fs_size_of,
* fs_nonempty, fs_guarded) covering class templates, variable templates,
* concepts and constrained functions.  Those vehicles compiling IS the
* claim; the checks inside them then confirm the value arrived intact.
*
*   The individual structural requirements are stated separately in
* section I as the traits that DO exist - trivial copy, trivial destroy,
* no virtuals, standard layout, a non-mutable public payload - so that a
* future edit which breaks structurality fails with a precise diagnosis
* rather than only an inscrutable "not a valid template argument".
*
*   THE TWO CONTRACTS THAT LOOK ALIKE:
*   size() and view() are byte-count accessors: both derive from _N, never
* from a search for a null.  c_str() hands out a C string, which stops at
* the first null.  For an ordinary literal the two agree, and a test built
* only from ordinary literals would never separate them.  fs_embedded is
* the fixture that does: for "a\0b", _N is 4, so size() is 3 and view()
* spans three bytes INCLUDING the interior null, while strlen(c_str()) is
* 1.  Several checks below turn on that divergence, because an
* implementation that quietly became strlen-based would pass everything
* else.
*
*   WHAT IS DELIBERATELY NOT TESTED:
*   fixed_string<0> is unreachable and untestable.  It cannot arise
* through the deduction guide - a string literal's array always includes
* its terminating null, so _N is at least 1 - and instantiating it
* directly declares `char data[0]`, which ISO C++ forbids (GCC accepts it
* only as an extension, and -Wpedantic rejects it).  Were it reachable,
* size() would evaluate 0 - 1 on an unsigned type and wrap to SIZE_MAX;
* that is worth knowing, but stating it would require a build the project
* does not permit, so it is recorded here instead.  fs_empty pins the
* smallest _N that IS reachable.
*
*   The adversarial type zoo from test_traits.hpp is not used here.  It
* quantifies over TYPE parameters, and this module's only parameter is a
* std::size_t; the axis that matters instead is payload length and
* payload content, which the fixtures below cover directly.
*
*
* path:      /tests/djinterp/core/meta/fixed_string_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    check plumbing            (D_FS_CHECK)
      --------------------------------------

II.   literal fixtures          (fs_abc, fs_empty, fs_embedded, fs_long)
      ------------------------------------------------------------------

III.  inspection helpers        (fs_bytes_equal, fs_extent, ...)
      ----------------------------------------------------------

IV.   NTTP vehicles             (fs_holder, fs_size_of, fs_nonempty, ...)
      -------------------------------------------------------------------

V.    test declarations
      -----------------
*/

#ifndef DJINTERP_TESTING_FIXED_STRING_TESTS_
#define DJINTERP_TESTING_FIXED_STRING_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string_view>
#include <type_traits>
// djinterp
#include "../../../../inc/djinterp/core/meta/fixed_string.hpp"                // the module under test


NS_DJINTERP
NS_TESTING


// ===========================================================================
// I.   check plumbing
// ===========================================================================

// fixed_string_check
//   function: records one assertion.  Clears _all on failure and emits a
// line naming the failing expression and its source line, so a red run
// says WHICH claim broke without a debugger.  Returns the outcome so a
// caller may also branch on it.
D_INLINE bool
fixed_string_check(
    bool&       _all,
    bool        _ok,
    const char* _expression,
    int         _line
)
{
    // report the failure; the section keeps running so one break does
    // not mask the state of every claim after it
    if (!_ok)
    {
        std::printf("      ! line %d: %s\n", _line, _expression);
        _all = false;
    }

    return _ok;
}

// D_FS_CHECK
//   macro: states one claim about fixed_string.hpp.  The condition is
// passed as __VA_ARGS__ so that a template argument list may contain
// commas (`std::is_same_v<a, b>` is one argument here, not two).
#define D_FS_CHECK(_all, ...)                                                 \
    ::djinterp::testing::fixed_string_check((_all),                           \
                                            (__VA_ARGS__),                    \
                                            #__VA_ARGS__,                     \
                                            __LINE__)


// ===========================================================================
// II.  literal fixtures
// ===========================================================================

// fs_abc
//   value: the ordinary case - three characters plus the terminating
// null, so _N is 4 and size() is 3.
inline constexpr fixed_string fs_abc{"abc"};

// fs_empty
//   value: the shortest payload a string literal can produce.  _N is 1 -
// the terminating null and nothing else - so size() is 0 and view() is
// empty.  This is the boundary the _N - 1 arithmetic in size() and view()
// has to get right, and the smallest _N the deduction guide can reach.
inline constexpr fixed_string fs_empty{""};

// fs_embedded
//   value: a literal carrying an INTERIOR null.  _N is 4 - 'a', '\0',
// 'b', '\0' - so size() reports 3 while strlen(c_str()) reports 1.  That
// divergence is what separates this type's byte-count contract from a
// C-string contract, and it is the only fixture that can tell a correct
// size() apart from one that searched for a null.
inline constexpr fixed_string fs_embedded{"a\0b"};

// fs_long
//   value: a payload long enough that the constructor's copy loop runs
// many iterations, so a bug that only appears past the first few bytes
// has somewhere to hide.
inline constexpr fixed_string fs_long{"the quick brown fox jumped over"};


// ===========================================================================
// III. inspection helpers
// ===========================================================================

// fs_bytes_equal
//   function: whether _Value holds the same _N bytes as _Literal, the
// TERMINATING NULL INCLUDED.  The module promises a byte-for-byte copy of
// the array behind a string literal, so comparing all _N bytes states that
// promise exactly; a comparison that stopped at the first null would agree
// with a strncpy-shaped bug instead of catching it.
//
//   Both parameters carry the same _N, so a length mismatch is a
// deduction failure rather than a silent short read.
template<std::size_t _N>
constexpr bool
fs_bytes_equal(
    const fixed_string<_N>& _value,
    const char            (&_literal)[_N]
)
{
    for (std::size_t i = 0; i < _N; ++i)
    {
        // one differing byte is enough to fail the claim
        if (_value.data[i] != _literal[i])
        {
            return false;
        }
    }

    return true;
}

// fs_extent
//   function: recovers _N from a VALUE rather than from a spelled-out
// specialization.  Deduction happens on the parameter, so this reports
// what the deduction guide actually produced instead of what the caller
// assumed it would.
template<std::size_t _N>
constexpr std::size_t
fs_extent(
    const fixed_string<_N>&
)
{
    return _N;
}

// fs_null_terminated
//   function: whether the payload's last byte is a null.  Kept separate
// from fs_bytes_equal because this is the property c_str() depends on,
// whereas fs_bytes_equal is the property the constructor promises; a
// literal satisfies both, but they are not the same claim.
template<std::size_t _N>
constexpr bool
fs_null_terminated(
    const fixed_string<_N>& _value
)
{
    return _value.data[_N - 1] == '\0';
}

// fs_is_equality_comparable
//   trait: whether two values of these types may be compared with ==.
// The module ships no operator==, so this reads false for fixed_string
// itself and true for the std::string_view that view() hands back - which
// together state that value comparison is meant to go through view().
template<typename _Left,
         typename _Right>
struct fs_is_equality_comparable
    : std::bool_constant<requires (const _Left&  _l,
                                   const _Right& _r) { _l == _r; }>
{};


// ===========================================================================
// IV.  NTTP vehicles
// ===========================================================================
//   fixed_string exists so a string literal can be carried as a C++20
// class-type non-type template parameter, and there is no trait that can
// be asked whether a type is structural.  The claim is therefore stated by
// USE: the four vehicles below put a fixed_string in every position an
// NTTP can occupy, and their merely compiling is the proof.  The checks in
// section IV then confirm the value survived the round trip.

// fs_holder
//   type: a class template parameterised on a fixed_string.  Hands the
// argument back out so a test can compare what emerged against what went
// in.  The parameter is a placeholder for a deduced class type, so both
// fs_holder<fixed_string{"abc"}> and fs_holder<"abc"> name it.
template<fixed_string _Text>
struct fs_holder
{
    static constexpr auto value = _Text;
};

// fs_size_of
//   value: a variable template parameterised on a fixed_string, so the
// argument is consumed outside a class scope too.
template<fixed_string _Text>
inline constexpr std::size_t fs_size_of = _Text.size();

// fs_nonempty
//   concept: a constraint evaluated on a fixed_string NTTP, which
// requires the argument to be usable in a constant expression during
// constraint checking rather than only inside a template body.
template<fixed_string _Text>
concept fs_nonempty = (_Text.size() > 0);

// fs_guarded
//   function: a function template constrained on a fixed_string NTTP.
// Distinct from fs_size_of because the argument is used in a
// requires-clause, which is checked before the body is ever instantiated.
template<fixed_string _Text>
    requires fs_nonempty<_Text>
constexpr std::size_t
fs_guarded()
{
    return _Text.size();
}

// fs_binds_nttp
//   trait: true when _Op has the signature "template parameterised on one
// fixed_string".  Binding a template as a template template argument
// checks its signature without instantiating it.
template<template<fixed_string> class _Op>
struct fs_binds_nttp : std::true_type
{};


// ===========================================================================
// V.   test declarations
// ===========================================================================

// section I - the type's own shape (fixed_string_tests_structure.cpp)
bool tests_fixed_string_layout();
bool tests_fixed_string_structural_requirements();
bool tests_fixed_string_distinctness();

// section II - construction and CTAD (fixed_string_tests_construction.cpp)
bool tests_fixed_string_construction();
bool tests_fixed_string_deduction_guide();
bool tests_fixed_string_embedded_nulls();

// section III - the accessors (fixed_string_tests_accessors.cpp)
bool tests_fixed_string_c_str();
bool tests_fixed_string_size();
bool tests_fixed_string_view();

// section IV - use as an NTTP (fixed_string_tests_nttp.cpp)
bool tests_fixed_string_as_nttp();
bool tests_fixed_string_argument_equivalence();
bool tests_fixed_string_nttp_round_trip();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_FIXED_STRING_TESTS_
