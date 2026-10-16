/*******************************************************************************
* djinterp [testing]                              sequential_container_tests.hpp
*
*   DTest suite header for sequential_container.hpp - the (legacy) order-
* preserving base that ordered_container.hpp supersedes.  Two faces gated by
* DTEST_SPEC_MODE, exactly as the sibling suites: normal mode exposes the CRTP
* fixtures, the SFINAE-probe traits, and the check macro; spec mode exposes
* sequential_container_spec().
*
*   The suite covers what this file defines: the is_sequential_container
*   verdict
* and the DSequentialKind / sequential_kind storage classifier, the
* sequential_base CRTP operations, and the free order algorithms.
*
*   SUITE NOTES (pinned observations, not defects introduced by the suite):
*     1. is_sequential_container reports TRUE for std::set / std::multiset /
*        std::map / std::multimap - they carry no `hasher`, and the trait
*        excludes only hash-ordered associatives.  This contradicts this
*        module's own header note ("does NOT include ... multisets"), and is
*        exactly the verdict ordered_container.hpp was written to correct.  The
*        classifiers block pins the true behaviour; the descriptor says so.
*     2. sequential_kind classifies std::deque as list_like, not deque_like:
*        std::deque has no data() accessor, so it misses the array_like arm,
*        and
*        being bidirectional it is caught by the list_like arm before the
*        deque_like arm is ever reached.  Under the standard iterator hierarchy
*        (random-access implies bidirectional) the deque_like arm is therefore
*        unreachable with any conforming container, so no test can cover that
*        specific ternary arm without a pathological iterator.  The kind block
*        pins deque == list_like and names the dead arm.
*
*
* path:      /tests/djinterp/core/container/sequential_container_tests.hpp
* link(s):   TBA
* author(s): DTest suite                                     created: 2026.07.24
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_SEQUENTIAL_CONTAINER_TESTS_
#define DJINTERP_TESTS_SEQUENTIAL_CONTAINER_TESTS_ 1

// std
#include <array>
#include <cstddef>
#include <cstdio>
#include <deque>
#include <forward_list>
#include <functional>
#include <initializer_list>
#include <list>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
// djinterp
// djinterp core is ALWAYS first and unconditional.
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/core/container/sequential_container.hpp"                 // the header under test
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"          // module_spec + run_module
#endif


NS_DJINTERP
NS_TESTING

// dt names the entities under test (djinterp::test); unconditional so the spec
// provider can name dt::module_spec in spec mode.
namespace dt = ::djinterp::test;


// D_SQ_CHECK
//   macro: evaluate a predicate once; on failure print the failing expression
// and its location and return false from the enclosing body.  Variadic so a
// top-level comma in a trait expression passes through whole.
#define D_SQ_CHECK(...)                                                  \
    do                                                                   \
    {                                                                    \
        if (!(__VA_ARGS__))                                              \
        {                                                                \
            std::printf("    check failed: %s\n    at: %s:%d\n",         \
                        #__VA_ARGS__, __FILE__, __LINE__);               \
            return false;                                                \
        }                                                                \
    }                                                                    \
    while (0)


#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only

// vec_seq
//   fixture: a std::vector-backed derived container mixing in the sequential
// order operations via CRTP.  Exposes begin()/end()/size().
template<typename _Value>
class vec_seq
    : public sequential_base<vec_seq<_Value>>
{
public:
    vec_seq() = default;

    vec_seq(std::initializer_list<_Value> _init)
        : _data(_init)
    {
    }

    typename std::vector<_Value>::iterator
    begin()
    {
        return _data.begin();
    }

    typename std::vector<_Value>::const_iterator
    begin() const
    {
        return _data.begin();
    }

    typename std::vector<_Value>::iterator
    end()
    {
        return _data.end();
    }

    typename std::vector<_Value>::const_iterator
    end() const
    {
        return _data.end();
    }

    std::size_t
    size() const
    {
        return _data.size();
    }

private:
    std::vector<_Value> _data;
};

// arr_seq
//   fixture: a std::array-backed derived container, used to show the base is
// agnostic to the backing store.
template<typename    _Value,
         std::size_t _N>
class arr_seq
    : public sequential_base<arr_seq<_Value, _N>>
{
public:
    arr_seq() = default;

    arr_seq(std::initializer_list<_Value> _init)
    {
        std::size_t _i = 0;

        for (const _Value& _v : _init)
        {
            if (_i >= _N)
            {
                break;
            }

            _data[_i] = _v;
            ++_i;
        }
    }

    typename std::array<_Value, _N>::iterator
    begin()
    {
        return _data.begin();
    }

    typename std::array<_Value, _N>::const_iterator
    begin() const
    {
        return _data.begin();
    }

    typename std::array<_Value, _N>::iterator
    end()
    {
        return _data.end();
    }

    typename std::array<_Value, _N>::const_iterator
    end() const
    {
        return _data.end();
    }

    std::size_t
    size() const
    {
        return _N;
    }

private:
    std::array<_Value, _N> _data{};
};

// contents_equal
//   helper: true iff the range of _fixture equals _expected
//   element-for-element.
template<typename _Fixture,
         typename _Value>
bool
contents_equal(
    const _Fixture&               _fixture,
    std::initializer_list<_Value> _expected
)
{
    return _fixture.size() == _expected.size()
        && std::equal(std::begin(_fixture),
                      std::end(_fixture),
                      _expected.begin());
}

// can_starts_with / can_ends_with / can_contains_subsequence
//   traits: true iff the like-named free algorithm is callable with the given
// container types - a probe of the enable_if guard.
template<typename _C,
         typename _P,
         typename = void>
struct can_starts_with
    : std::false_type
{
};

template<typename _C,
         typename _P>
struct can_starts_with<_C, _P,
    std::void_t<decltype(starts_with(std::declval<const _C&>(),
                                     std::declval<const _P&>()))>>
    : std::true_type
{
};

template<typename _C,
         typename _S,
         typename = void>
struct can_ends_with
    : std::false_type
{
};

template<typename _C,
         typename _S>
struct can_ends_with<_C, _S,
    std::void_t<decltype(ends_with(std::declval<const _C&>(),
                                   std::declval<const _S&>()))>>
    : std::true_type
{
};

template<typename _C,
         typename _S,
         typename = void>
struct can_contains_subsequence
    : std::false_type
{
};

template<typename _C,
         typename _S>
struct can_contains_subsequence<_C, _S,
    std::void_t<decltype(contains_subsequence(std::declval<const _C&>(),
                                              std::declval<const _S&>()))>>
    : std::true_type
{
};

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- declarations - visible in BOTH modes ----------------------------------

// I.   traits   (sequential_container_tests_traits.cpp)
bool tests_sequential_sequences_are_sequential();
bool tests_sequential_ordered_associatives_pinned();
bool tests_sequential_hashed_associatives_excluded();
bool tests_sequential_non_containers_excluded();
bool tests_sequential_kind_classification();
bool tests_sequential_kind_deque_pinned();
bool tests_sequential_kind_v_matches();

// II.  base   (sequential_container_tests_base.cpp)
bool tests_sequential_front_back();
bool tests_sequential_nth();
bool tests_sequential_is_sorted_default();
bool tests_sequential_is_sorted_custom();
bool tests_sequential_is_palindrome();
bool tests_sequential_reverse();
bool tests_sequential_rotate_left();
bool tests_sequential_rotate_right();
bool tests_sequential_shift_left();
bool tests_sequential_shift_right();
bool tests_sequential_storage_agnostic();

// III. algorithms   (sequential_container_tests_algorithms.cpp)
bool tests_sequential_starts_with();
bool tests_sequential_ends_with();
bool tests_sequential_contains_subsequence();
bool tests_sequential_algorithms_on_string();
bool tests_sequential_algorithms_gating();


// -- spec provider - spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
inline dt::module_spec
sequential_container_spec()
{
    return dt::module_spec{
        "sequential_container",
        "The legacy order-preserving base superseded by "
        "ordered_container.hpp: "
        "the is_sequential_container verdict and DSequentialKind classifier, "
        "the sequential_base CRTP operations, and the free order algorithms.",
        {
            dt::block_spec{
                "traits",
                "the is_sequential_container verdict (iterable and no hasher) "
                "and the DSequentialKind storage classifier.",
                {
                    { "tests_sequential_sequences_are_sequential",
                      "positional sequences (vector/array/string/list/"
                      "forward_list/deque) are is_sequential_container.",
                      &tests_sequential_sequences_are_sequential },
                    { "tests_sequential_ordered_associatives_pinned",
                      "PINNED: set/multiset/map/multimap read TRUE (no "
                      "hasher), "
                      "contradicting the header note; ordered_container fixes "
                      "it.",
                      &tests_sequential_ordered_associatives_pinned },
                    { "tests_sequential_hashed_associatives_excluded",
                      "the unordered_ associatives are excluded (they carry a "
                      "hasher).",
                      &tests_sequential_hashed_associatives_excluded },
                    { "tests_sequential_non_containers_excluded",
                      "scalars and pointers are not is_sequential_container.",
                      &tests_sequential_non_containers_excluded },
                    { "tests_sequential_kind_classification",
                      "sequential_kind maps string/vector/array/list/"
                      "forward_list to their kinds and a scalar to none.",
                      &tests_sequential_kind_classification },
                    { "tests_sequential_kind_deque_pinned",
                      "PINNED: std::deque classifies as list_like (no data(), "
                      "bidirectional); the deque_like arm is unreachable.",
                      &tests_sequential_kind_deque_pinned },
                    { "tests_sequential_kind_v_matches",
                      "the sequential_kind_v alias equals sequential_kind<>::"
                      "value for each probed type.",
                      &tests_sequential_kind_v_matches },
                }
            },
            dt::block_spec{
                "base",
                "the sequential_base CRTP order operations over a vector- and "
                "an array-backed derived container.",
                {
                    { "tests_sequential_front_back",
                      "front()/back() return the first/last element and "
                      "coincide "
                      "for a single-element sequence.",
                      &tests_sequential_front_back },
                    { "tests_sequential_nth",
                      "nth(n) yields an iterator to the n-th element for the "
                      "first, an interior, and the last position.",
                      &tests_sequential_nth },
                    { "tests_sequential_is_sorted_default",
                      "is_sorted() is true for non-descending, empty, and "
                      "single; "
                      "false for an out-of-order sequence.",
                      &tests_sequential_is_sorted_default },
                    { "tests_sequential_is_sorted_custom",
                      "is_sorted(cmp) honours a supplied comparator.",
                      &tests_sequential_is_sorted_custom },
                    { "tests_sequential_is_palindrome",
                      "is_palindrome() covers empty, single, even/odd "
                      "palindromes, and even/odd non-palindromes.",
                      &tests_sequential_is_palindrome },
                    { "tests_sequential_reverse",
                      "reverse() flips order; empty and single are no-ops; "
                      "twice "
                      "is the identity.",
                      &tests_sequential_reverse },
                    { "tests_sequential_rotate_left",
                      "rotate_left(n): n==0 no-op, an ordinary rotate, n>size "
                      "reduced modulo size.",
                      &tests_sequential_rotate_left },
                    { "tests_sequential_rotate_right",
                      "rotate_right(n): n==0 no-op, an ordinary rotate, the "
                      "empty "
                      "guard, and n>size wrapping.",
                      &tests_sequential_rotate_right },
                    { "tests_sequential_shift_left",
                      "shift_left(n,fill) shifts left filling the tail; "
                      "n>=size "
                      "clears to fill.",
                      &tests_sequential_shift_left },
                    { "tests_sequential_shift_right",
                      "shift_right(n,fill) shifts right filling the head; "
                      "n>=size "
                      "clears to fill.",
                      &tests_sequential_shift_right },
                    { "tests_sequential_storage_agnostic",
                      "the same operations run on an array-backed container.",
                      &tests_sequential_storage_agnostic },
                }
            },
            dt::block_spec{
                "algorithms",
                "the free order algorithms and the enable_if guard admitting "
                "sequential containers.",
                {
                    { "tests_sequential_starts_with",
                      "starts_with matches an empty/partial/full prefix and "
                      "rejects a mismatch or an over-long prefix.",
                      &tests_sequential_starts_with },
                    { "tests_sequential_ends_with",
                      "ends_with matches an empty/partial/full suffix and "
                      "rejects "
                      "a mismatch or an over-long suffix.",
                      &tests_sequential_ends_with },
                    { "tests_sequential_contains_subsequence",
                      "contains_subsequence finds a sub-run at the head, "
                      "interior, "
                      "and tail, and rejects an absent one.",
                      &tests_sequential_contains_subsequence },
                    { "tests_sequential_algorithms_on_string",
                      "the three algorithms behave the same on std::string.",
                      &tests_sequential_algorithms_on_string },
                    { "tests_sequential_algorithms_gating",
                      "the guard admits sequences AND ordered associatives, "
                      "but "
                      "rejects hashed associatives and non-containers.",
                      &tests_sequential_algorithms_gating },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp

#endif  // DJINTERP_TESTS_SEQUENTIAL_CONTAINER_TESTS_
