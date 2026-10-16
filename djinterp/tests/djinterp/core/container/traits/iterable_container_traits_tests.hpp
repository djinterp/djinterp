/*******************************************************************************
* djinterp [testing]                         iterable_container_traits_tests.hpp
*
*   Declarations, shared helpers, and helper TYPES for the
* iterable_container_traits.hpp unit-test suite.  The ITERABILITY axis asks in what
* mode a container's contents may be traversed: a type is ITERABLE when it exposes
* a begin()/end() pair, NON-ITERABLE when it looks like a container (it has a
* value_type) but offers positional access only, and its MODE - the access each
* visited position grants - ranks none < const_only < non_const as a capability
* order.
*
*   MECHANISM.  The traits are compile-time facts; each check compares a constant
* reading against its expected result.  A failing check prints the offending
* expression and drives the function's bool result to false, which the runner turns
* into a non-zero exit code.  The value-level mode algebra (names, predicates, rank,
* subsumption) is plain constexpr and is exercised directly.
*
*   SCOPE.  These tests cover the traits this header actually DEFINES - sections I,
* II, III and V.  The file's prose header also describes a "section IV" operational
* iterator-CATEGORY layer (forward / bidirectional / random-access + contiguity),
* but no such section is present in the code: those traits live in
* iterator_category_traits.hpp (is_forward_iterable, is_bidirectional_iterable,
* is_random_access_iterable, has_data_accessor, is_contiguous_iterable) and are
* tested with that header.  The prose appears to be stale after the move.
*
*   HELPER TYPES.  Custom fixtures isolate the branches the standard containers
* cannot reach on their own:
*     - const_only_iter     only CONST begin()/end()      -> iterable, const_only
*                           (a const member is still callable on a non-const lvalue,
*                           so has_begin_method fires)
*     - mutable_only_iter   only NON-CONST begin()/end()  -> provides_const_iteration
*                           is FALSE while the mode is non_const (and so
*                           is_const_iterable_container is TRUE - see below)
*     - no_value_type_iter  begin()/end() to int*, NO value_type -> iterable, yet
*                           provides_mutable_iteration is FALSE because that probe
*                           names value_type; the mode reads const_only even though
*                           the traversal is genuinely settable
*     - positional_only     value_type + data()/size()/operator[], no traversal
*                           -> the canonical NON-iterable container
*     - begin_only          value_type + begin() but NO end() -> not iterable (so the
*                           mode is none) even though provides_mutable_iteration is
*                           TRUE, since that probe never touches end()
*     - end_only            value_type + end() but no begin() -> the mirror case
*     - alias_only          iterator / const_iterator aliases, no traversal and no
*                           value_type -> alias detection is independent of both
*
*   NAMING TRAP (deliberately pinned).  is_const_iterable_container is defined as
* mode_grants_observation(mode), i.e. mode != none - it means "may be OBSERVED by
* traversal", not "const-ONLY".  It is therefore TRUE for a mutably-iterable
* container such as std::vector<int>, and is distinct from
* provides_const_iteration (which asks whether begin()/end() are callable on a
* CONST lvalue).  mutable_only_iter separates the two.
*
*
* path:      /tests/djinterp/core/container/traits/iterable_container_traits_tests.hpp
* link(s):   TBA
* author(s): unit-test suite                                 created: 2026.07.18
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_ITERABLE_CONTAINER_TRAITS_TESTS_
#define DJINTERP_ITERABLE_CONTAINER_TRAITS_TESTS_ 1

// std
#include <array>
#include <cstddef>
#include <cstdio>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>
// djinterp
// djinterp (bare include; the leaf CMakeLists adds inc/djinterp/core/container/traits)
#include "../../../../../inc/djinterp/core/container/traits/iterable_container_traits.hpp"


#ifndef D_KEYWORD_TESTING
    #define D_KEYWORD_TESTING   testing
#endif  // D_KEYWORD_TESTING

#ifndef NS_TESTING
    #define NS_TESTING          D_NAMESPACE(D_KEYWORD_TESTING)
#endif  // NS_TESTING


// D_IC_CHECK
//   macro: the per-suite runtime check (unique letters: IC = Iterable Container).
// Records a failure into the enclosing function's `_ic_ok` accumulator and prints
// the offending expression.  Always pass the condition already wrapped in
// parentheses so interior template commas (map<int,int>, array<int,N>) survive
// macro argument splitting.
#define D_IC_CHECK(_cond)                                                     \
    do {                                                                      \
        if (!(_cond)) {                                                       \
            std::printf("    - check failed: %s\n", #_cond);                  \
            _ic_ok = false;                                                   \
        }                                                                     \
    } while (false)


NS_DJINTERP
NS_TESTING


// ===========================================================================
// Shared helpers
// ===========================================================================

// ic_cstr_eq
//   helper: constexpr C-string equality, so the mode name spellings can be checked
// directly (std::strcmp is not constexpr before C++23).
constexpr bool
ic_cstr_eq(const char* _a, const char* _b) noexcept
{
    return (*_a == *_b) && (*_a == '\0' ? true : ic_cstr_eq(_a + 1, _b + 1));
}


// ===========================================================================
// Helper types
// ===========================================================================
//   Members are declared, not defined: every trait here is a decltype/SFINAE
// probe in an unevaluated context, so declarations suffice.

// const_only_iter: only CONST begin()/end().  has_begin_method still fires (a const
// member is callable on the non-const lvalue the probe uses), and the dereference
// is a const lvalue, so the mode is const_only.
struct const_only_iter
{
    using value_type = int;
    const int* begin() const;
    const int* end()   const;
};

// mutable_only_iter: only NON-CONST begin()/end() - no const overload at all.  The
// mode is non_const, yet provides_const_iteration is FALSE (nothing is callable on
// a const lvalue).  This separates provides_const_iteration from the
// observation-shaped is_const_iterable_container.
struct mutable_only_iter
{
    using value_type = int;
    int* begin();
    int* end();
};

// no_value_type_iter: a genuinely settable traversal, but NO value_type alias.  The
// provides_mutable_iteration probe names value_type, so it fails SFINAE and the
// mode reads const_only despite the traversal being writable.
struct no_value_type_iter
{
    int* begin();
    int* end();
};

// positional_only: the canonical NON-iterable container - value_type plus positional
// access, no traversal.
struct positional_only
{
    using value_type = int;
    const int*  data() const;
    std::size_t size() const;
    int&        operator[](std::size_t);
};

// begin_only: half a traversal.  is_iterable_container is FALSE (end() is missing),
// so the mode is none - even though provides_mutable_iteration is TRUE, that probe
// never touching end().
struct begin_only
{
    using value_type = int;
    int* begin();
};

// end_only: the mirror half.
struct end_only
{
    using value_type = int;
    int* end();
};

// alias_only: iterator / const_iterator aliases with no traversal and no value_type -
// alias detection is independent of both traversal and container-hood.
struct alias_only
{
    using iterator       = int*;
    using const_iterator = const int*;
};


// ===========================================================================
// Test-function declarations (defined in the section .cpp files)
// ===========================================================================

// -- I.   SFINAE method / alias detection --
bool tests_has_begin_end_methods();
bool tests_alias_detection();

// -- II.  Base classification --
bool tests_is_iterable_container();
bool tests_is_non_iterable_container();

// -- III. Mode sub-axis, value-level algebra --
bool tests_iteration_mode_name();
bool tests_mode_value_predicates();
bool tests_mode_grants();
bool tests_iteration_mode_rank();
bool tests_iteration_mode_subsumes();

// -- III. Mode sub-axis, type-level traits --
bool tests_provides_const_iteration();
bool tests_provides_mutable_iteration();
bool tests_iteration_mode_of();
bool tests_derived_mode_predicates();

// -- V.   Aggregate snapshot + variable-template companions --
bool tests_iterable_container_class();
bool tests_variable_templates();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_ITERABLE_CONTAINER_TRAITS_TESTS_
