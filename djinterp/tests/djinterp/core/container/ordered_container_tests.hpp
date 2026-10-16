/*******************************************************************************
* djinterp [testing]                                 ordered_container_tests.hpp
*
*   DTest suite header for ordered_container.hpp - the ORDER axis.  Presents
*   two
* faces gated by DTEST_SPEC_MODE: in normal mode it pulls in the header under
* test and exposes the CRTP fixtures, the SFINAE-probe traits, and the check
* macro; in spec mode it pulls in test_defaults.hpp and exposes the
* ordered_container_spec() provider.  The bool tests_*() declarations are
* visible
* in both faces.
*
*   The suite exercises the three things this file actually defines: the
* order-axis VERDICT it re-exports (is_ordered_container /
* is_unordered_container
* - its headline correction is that positional identity excludes ALL
* associatives), the ordered_base CRTP order operations, and the free order
* algorithms (starts_with / ends_with / contains_subsequence).
*
*
* path:      /tests/djinterp/core/container/ordered_container_tests.hpp
* link(s):   TBA
* author(s): DTest suite                                     created: 2026.07.24
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_ORDERED_CONTAINER_TESTS_
#define DJINTERP_TESTS_ORDERED_CONTAINER_TESTS_ 1

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
#include "../../../../inc/djinterp/core/container/ordered_container.hpp"                    // the header under test
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"          // module_spec + run_module
#endif


NS_DJINTERP
NS_TESTING

// dt names the entities under test (djinterp::test); the spec provider needs
// dt::module_spec even in spec mode, so the alias is unconditional.
namespace dt = ::djinterp::test;


// D_OR_CHECK
//   macro: evaluate a predicate once; on failure print the failing expression
// and its location and return false from the enclosing tests_*() body.
// Variadic
// so a top-level comma inside a trait expression passes through whole.
// Depends
// on nothing from the header under test, so it stays outside the fixture
// guard.
#define D_OR_CHECK(...)                                                  \
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
//   fixture: a std::vector-backed derived container that mixes in the ordered
// order operations via CRTP.  Exposes begin()/end()/size(); the base supplies
// front/back/nth/is_sorted/is_palindrome/reverse/rotate/shift on top.
template<typename _Value>
class vec_seq
    : public ordered_base<vec_seq<_Value>>
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
//   fixture: a std::array-backed derived container.  Fixed storage still
//   admits
// every in-place order operation (reverse/rotate/shift keep the size), so it
// is
// used to show the CRTP base is agnostic to the backing store.
template<typename    _Value,
         std::size_t _N>
class arr_seq
    : public ordered_base<arr_seq<_Value, _N>>
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
//   element-for-element
// (same length, same order).  Reads the fixture through std::begin/std::end,
// so
// it works for any derived container the base is mixed into.
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
// container types.  They probe the enable_if guard (which turns on the
// overload
// only for ordered containers), so an associative or a non-container reads
// false without a hard error.
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

// I.   classifiers   (ordered_container_tests_classifiers.cpp)
bool tests_ordered_sequences_are_ordered();
bool tests_ordered_associatives_are_unordered();
bool tests_ordered_non_containers_are_unordered();
bool tests_ordered_unordered_predicate();

// II.  base   (ordered_container_tests_base.cpp)
bool tests_ordered_front_back();
bool tests_ordered_nth();
bool tests_ordered_is_sorted_default();
bool tests_ordered_is_sorted_custom();
bool tests_ordered_is_palindrome();
bool tests_ordered_reverse();
bool tests_ordered_rotate_left();
bool tests_ordered_rotate_right();
bool tests_ordered_shift_left();
bool tests_ordered_shift_right();
bool tests_ordered_storage_agnostic();

// III. algorithms   (ordered_container_tests_algorithms.cpp)
bool tests_ordered_starts_with();
bool tests_ordered_ends_with();
bool tests_ordered_contains_subsequence();
bool tests_ordered_algorithms_on_string();
bool tests_ordered_algorithms_gating();


// -- spec provider - spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
inline dt::module_spec
ordered_container_spec()
{
    return dt::module_spec{
        "ordered_container",
        "The ORDER axis (positional identity): the re-exported order verdict, "
        "the ordered_base CRTP order operations, and the free order "
        "algorithms.",
        {
            dt::block_spec{
                "classifiers",
                "the re-exported order verdict - the headline correction that "
                "positional identity excludes every associative container.",
                {
                    { "tests_ordered_sequences_are_ordered",
                      "positional sequences (vector/array/string/list/"
                      "forward_list/deque) are is_ordered_container.",
                      &tests_ordered_sequences_are_ordered },
                    { "tests_ordered_associatives_are_unordered",
                      "every associative (set/multiset/map/multimap and the "
                      "unordered_ ones) is NOT is_ordered_container.",
                      &tests_ordered_associatives_are_unordered },
                    { "tests_ordered_non_containers_are_unordered",
                      "scalars and pointers are not is_ordered_container.",
                      &tests_ordered_non_containers_are_unordered },
                    { "tests_ordered_unordered_predicate",
                      "is_unordered_container is the associative complement: "
                      "true for keyed containers, false for sequences.",
                      &tests_ordered_unordered_predicate },
                }
            },
            dt::block_spec{
                "base",
                "the ordered_base CRTP order operations, read and mutating, "
                "over a vector- and an array-backed derived container.",
                {
                    { "tests_ordered_front_back",
                      "front()/back() return the first/last element; they "
                      "coincide for a single-element sequence.",
                      &tests_ordered_front_back },
                    { "tests_ordered_nth",
                      "nth(n) yields an iterator to the n-th element for the "
                      "first, an interior, and the last position.",
                      &tests_ordered_nth },
                    { "tests_ordered_is_sorted_default",
                      "is_sorted() is true for a non-descending sequence, "
                      "empty, "
                      "and single; false for an out-of-order one.",
                      &tests_ordered_is_sorted_default },
                    { "tests_ordered_is_sorted_custom",
                      "is_sorted(cmp) honours a supplied comparator "
                      "(descending "
                      "under std::greater).",
                      &tests_ordered_is_sorted_custom },
                    { "tests_ordered_is_palindrome",
                      "is_palindrome() covers empty, single, even/odd "
                      "palindromes, and even/odd non-palindromes.",
                      &tests_ordered_is_palindrome },
                    { "tests_ordered_reverse",
                      "reverse() flips element order; empty and single are "
                      "no-ops; applying it twice is the identity.",
                      &tests_ordered_reverse },
                    { "tests_ordered_rotate_left",
                      "rotate_left(n) rotates left, with n==0 a no-op and "
                      "n>size reduced modulo size.",
                      &tests_ordered_rotate_left },
                    { "tests_ordered_rotate_right",
                      "rotate_right(n) rotates right; n==0 is a no-op, an "
                      "empty "
                      "sequence is guarded, and n>size wraps.",
                      &tests_ordered_rotate_right },
                    { "tests_ordered_shift_left",
                      "shift_left(n,fill) shifts left filling the tail; "
                      "n>=size "
                      "clears the whole sequence to fill.",
                      &tests_ordered_shift_left },
                    { "tests_ordered_shift_right",
                      "shift_right(n,fill) shifts right filling the head; "
                      "n>=size clears the whole sequence to fill.",
                      &tests_ordered_shift_right },
                    { "tests_ordered_storage_agnostic",
                      "the same operations run on an array-backed container, "
                      "showing the base ignores the backing store.",
                      &tests_ordered_storage_agnostic },
                }
            },
            dt::block_spec{
                "algorithms",
                "the free order algorithms and the enable_if guard that "
                "admits "
                "only ordered containers.",
                {
                    { "tests_ordered_starts_with",
                      "starts_with matches an empty/partial/full prefix and "
                      "rejects a mismatch or an over-long prefix.",
                      &tests_ordered_starts_with },
                    { "tests_ordered_ends_with",
                      "ends_with matches an empty/partial/full suffix and "
                      "rejects a mismatch or an over-long suffix.",
                      &tests_ordered_ends_with },
                    { "tests_ordered_contains_subsequence",
                      "contains_subsequence finds a sub-run at the head, "
                      "interior, and tail, and rejects an absent one.",
                      &tests_ordered_contains_subsequence },
                    { "tests_ordered_algorithms_on_string",
                      "the three algorithms behave the same on std::string.",
                      &tests_ordered_algorithms_on_string },
                    { "tests_ordered_algorithms_gating",
                      "the guard admits ordered containers and rejects "
                      "associatives and non-containers at compile time.",
                      &tests_ordered_algorithms_gating },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp

#endif  // DJINTERP_TESTS_ORDERED_CONTAINER_TESTS_
