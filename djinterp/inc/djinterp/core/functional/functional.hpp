/******************************************************************************
* djinterp [functional]                                        functional.hpp
*
* Root header and higher-order algorithms for the C++ functional module.
*   Includes all functional sub-modules and provides fully generic,
* SFINAE-constrained implementations of map, filter, fold (left/right),
* for_each, quantifiers (any, all, none), count_if, find_if, find_last,
* index_of, last_index_of, is_sorted, zip_with, take, skip, reduce, scan,
* flat_map, group_by, partition, distinct, reverse, slice, and range.
*
*   All algorithms work with:
*     - STL containers (std::vector, std::list, std::deque, etc.)
*     - Raw C-style arrays (pointer + count)
*     - Iterator pairs (begin/end)
*     - Initializer lists
*
*   SFINAE constraints use the traits from functional_traits.hpp to verify
* callable signatures at compile time.
*
*   Supersedes functional_all.hpp (root include header) and
* functional_algorithms.hpp (algorithm implementations), merging both into
* a single module.
*
* SUB-MODULES
* ===========
*   stl_functional.hpp            - backported STL utilities
*   functional_core.hpp           - type aliases, composition, combinators
*   functional_traits.hpp         - SFINAE type traits for callables
*   predicate_combinators.hpp     - predicate AND/OR/XOR/NOT combinators
*   compose.hpp                   - composition, partial application, memoize
*   pipeline.hpp                  - typed chainable pipeline
*   fn_builder.hpp                - fluent function chain builder
*   filter.hpp                    - collection filtering framework
*
*
* path:      /inc/functional/functional.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.02.19
******************************************************************************/

/*
TABLE OF CONTENTS (ALGORITHMS)
==============================
I.    INTERNAL HELPERS
II.   MAP
III.  FILTER
IV.   FOLD / REDUCE
V.    SCAN (PREFIX FOLD)
VI.   FOR-EACH
VII.  QUANTIFIERS
VIII. SEARCHING AND COUNTING
IX.   ORDERING
X.    TAKE AND SKIP
XI.   FLAT-MAP AND ZIP
XII.  GROUP-BY AND PARTITION
XIII. DISTINCT AND REVERSE
XIV.  SLICE AND RANGE
*/


#ifndef DJINTERP_FUNCTIONAL_
#define DJINTERP_FUNCTIONAL_ 1

// std
#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <map>
#include <numeric>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../djinterp.hpp"
#include "./functional_core.hpp"
#include "./functional_traits.hpp"
#include "./predicate.hpp"
#include "./compose.hpp"
#include "./pipeline.hpp"
#include "./fn_builder.hpp"
#include "./filter.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    INTERNAL HELPERS                                      ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL
    // has_begin_end
    //   helper: detects if a type has begin() and end() member
    // functions.
    template<typename _Container,
             typename = void>
    struct has_begin_end : std::false_type
    {};

    template<typename _Container>
    struct has_begin_end<_Container,
        void_t<decltype(std::begin(std::declval<_Container&>())),
               decltype(std::end(std::declval<_Container&>()))>>
        : std::true_type
    {};

    // has_push_back
    //   helper: detects if a type has push_back().
    template<typename _Container,
             typename = void>
    struct has_push_back : std::false_type
    {};

    template<typename _Container>
    struct has_push_back<_Container,
        void_t<decltype(std::declval<_Container&>().push_back(
            std::declval<typename _Container::value_type>()))>>
        : std::true_type
    {};

    // has_reserve
    //   helper: detects if a type has reserve().
    template<typename _Container,
             typename = void>
    struct has_reserve : std::false_type
    {};

    template<typename _Container>
    struct has_reserve<_Container,
        void_t<decltype(std::declval<_Container&>().reserve(
            std::declval<std::size_t>()))>>
        : std::true_type
    {};

    // container_value_type
    //   helper: extracts value_type from a container or iterator
    // range.
    template<typename _Container>
    using container_value_type = typename std::decay<
        decltype(*std::begin(std::declval<_Container&>()))>::type;

    // maybe_reserve
    //   helper: calls reserve if the container supports it.
    template<typename _Container>
    typename std::enable_if<has_reserve<_Container>::value>::type
    maybe_reserve(_Container& _c, std::size_t _n)
    {
        _c.reserve(_n);
    }

    template<typename _Container>
    typename std::enable_if<!has_reserve<_Container>::value>::type
    maybe_reserve(_Container&, std::size_t)
    {}

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             II.   MAP                                                   ///
///////////////////////////////////////////////////////////////////////////////

// map (container -> vector)
//   function: applies a transformer to each element, returning a new
// vector.  The transformer must be callable with the container's
// value_type.
template<typename _Container,
         typename _Fn,
         typename _ValueType  = internal::container_value_type<const _Container>,
         typename _ResultType = callable_result_t<_Fn, const _ValueType&>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value &&
             is_callable<_Fn, const _ValueType&>::value>::type>
D_NODISCARD
std::vector<_ResultType>
map(
    const _Container& _input,
    _Fn&& _fn
)
{
    std::vector<_ResultType> result;

    internal::maybe_reserve(result, std::distance(
        std::begin(_input), std::end(_input)));

    for (const auto& element : _input)
    {
        result.push_back(std::forward<_Fn>(_fn)(element));
    }

    return result;
}

// map (iterator range -> vector)
//   function: applies a transformer to each element in
// [first, last).
template<typename _InputIt,
         typename _Fn,
         typename _ValueType  = typename std::iterator_traits<_InputIt>::value_type,
         typename _ResultType = callable_result_t<_Fn, const _ValueType&>,
         typename = typename std::enable_if<
             is_callable<_Fn, const _ValueType&>::value>::type>
D_NODISCARD
std::vector<_ResultType>
map(
    _InputIt _first,
    _InputIt _last,
    _Fn&&    _fn
)
{
    std::vector<_ResultType> result;

    internal::maybe_reserve(result, std::distance(_first, _last));

    for (auto it = _first; it != _last; ++it)
    {
        result.push_back(std::forward<_Fn>(_fn)(*it));
    }

    return result;
}

// map (raw array -> vector)
//   function: applies a transformer to each element of a C-style
// array.
template<typename _Type,
         typename _Fn,
         typename _ResultType = callable_result_t<_Fn, const _Type&>,
         typename = typename std::enable_if<
             is_callable<_Fn, const _Type&>::value
         >::type>
D_NODISCARD
std::vector<_ResultType>
map(
    const _Type* _data,
    std::size_t _count, 
    _Fn&&       _fn
)
{
    std::vector<_ResultType> result;

    result.reserve(_count);

    for (std::size_t i = 0; i < _count; ++i)
    {
        result.push_back(std::forward<_Fn>(_fn)(_data[i]));
    }

    return result;
}

// map_in_place (container)
//   function: applies a transformer to each element in place.
template<typename _Container,
         typename _Fn,
         typename _ValueType = internal::container_value_type<_Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<_Container>::value &&
             is_callable<_Fn, _ValueType&>::value
         >::type>
void
map_in_place(
    _Container& _input,
    _Fn&& _fn
)
{
    for (auto& element : _input)
    {
        element = std::forward<_Fn>(_fn)(element);
    }

    return;
}


///////////////////////////////////////////////////////////////////////////////
///             III.  FILTER                                                ///
///////////////////////////////////////////////////////////////////////////////

// filter (container -> vector)
//   function: returns a new vector containing only elements for
// which the predicate returns true.
template<typename _Container,
         typename _Predicate,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value &&
             is_predicate<_Predicate, const _ValueType&>::value>::type>
D_NODISCARD
std::vector<_ValueType>
filter(
    const _Container& _input,
    _Predicate&&      _predicate
)
{
    std::vector<_ValueType> result;

    for (const auto& element : _input)
    {
        if (std::forward<_Predicate>(_predicate)(element))
        {
            result.push_back(element);
        }
    }

    return result;
}

// filter (iterator range -> vector)
//   function: filters elements in [first, last) by predicate.
template<typename _InputIt,
         typename _Predicate,
         typename _ValueType = typename std::iterator_traits<_InputIt>::value_type,
         typename = typename std::enable_if<
             is_predicate<_Predicate, const _ValueType&>::value
         >::type>
D_NODISCARD
std::vector<_ValueType>
filter(
    _InputIt _first,
    _InputIt _last,
    _Predicate&& _predicate
)
{
    std::vector<_ValueType> result;

    for (auto it = _first; it != _last; ++it)
    {
        if (std::forward<_Predicate>(_predicate)(*it))
        {
            result.push_back(*it);
        }
    }

    return result;
}

// filter (raw array -> vector)
//   function: filters elements of a C-style array by predicate.
template<typename _Type,
         typename _Predicate,
         typename = typename std::enable_if<
             is_predicate<_Predicate, const _Type&>::value
         >::type>
D_NODISCARD
std::vector<_Type>
filter(
    const _Type* _data,
    std::size_t _count,
    _Predicate&& _predicate
)
{
    std::vector<_Type> result;

    for (std::size_t i = 0; i < _count; ++i)
    {
        if (std::forward<_Predicate>(_predicate)(_data[i]))
        {
            result.push_back(_data[i]);
        }
    }

    return result;
}

// filter_not (container -> vector)
//   function: returns elements for which the predicate returns
// false.
template<typename _Container,
         typename _Predicate,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value &&
             is_predicate<_Predicate, const _ValueType&>::value
         >::type>
D_NODISCARD
std::vector<_ValueType>
filter_not(
    const _Container& _input,
    _Predicate&&      _predicate
)
{
    return filter(_input,
                  [&_predicate](const _ValueType& _e)
                  {
                      return !_predicate(_e);
                  });
}


///////////////////////////////////////////////////////////////////////////////
///             IV.   FOLD / REDUCE                                        ///
///////////////////////////////////////////////////////////////////////////////

// fold_left (container)
//   function: left-associative fold over a container.
// fold_left({a,b,c}, init, f) = f(f(f(init, a), b), c)
template<typename _Container,
         typename _Acc,
         typename _Fn,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value &&
             is_callable<_Fn, const _Acc&, const _ValueType&>::value
         >::type>
D_NODISCARD
_Acc
fold_left(
    const _Container& _input,
    _Acc              _init,
    _Fn&&             _fn
)
{
    for (const auto& element : _input)
    {
        _init = std::forward<_Fn>(_fn)(
            static_cast<const _Acc&>(_init), element);
    }

    return _init;
}

// fold_left (iterator range)
//   function: left fold over an iterator range.
template<typename _InputIt,
         typename _Acc,
         typename _Fn,
         typename _ValueType = typename std::iterator_traits<_InputIt>::value_type,
         typename = typename std::enable_if<
             is_callable<_Fn, const _Acc&, const _ValueType&>::value
         >::type>
D_NODISCARD
_Acc
fold_left(
    _InputIt _first,
    _InputIt _last,
    _Acc     _init,
    _Fn&&    _fn)
{
    for (auto it = _first; it != _last; ++it)
    {
        _init = std::forward<_Fn>(_fn)(
            static_cast<const _Acc&>(_init), *it);
    }

    return _init;
}

// fold_right (container)
//   function: right-associative fold over a container.
// fold_right({a,b,c}, init, f) = f(a, f(b, f(c, init)))
// Requires bidirectional iteration.
template<typename _Container,
         typename _Acc,
         typename _Fn,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value &&
             is_callable<_Fn, const _ValueType&, const _Acc&>::value
         >::type>
D_NODISCARD
_Acc
fold_right(
    const _Container& _input,
    _Acc              _init,
    _Fn&&             _fn)
{
    auto it = std::end(_input);
    auto bg = std::begin(_input);

    while (it != bg)
    {
        --it;
        _init = std::forward<_Fn>(_fn)(
            *it, static_cast<const _Acc&>(_init));
    }

    return _init;
}

// reduce (container, no initial value)
//   function: reduces elements using a binary operation with the
// first element as the initial value.
template<typename _Container,
         typename _Fn,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value &&
             is_callable<_Fn, const _ValueType&, const _ValueType&>::value
         >::type>
D_NODISCARD
_ValueType
reduce(
    const _Container& _input,
    _Fn&& _fn
)
{
    auto it  = std::begin(_input);
    auto end = std::end(_input);

    _ValueType acc = *it;
    ++it;

    for (; it != end; ++it)
    {
        acc = std::forward<_Fn>(_fn)(
            static_cast<const _ValueType&>(acc), *it);
    }

    return acc;
}

// reduce (container, with initial value)
//   function: alias for fold_left with same accumulator type.
template<typename _Container,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename _Fn,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value &&
             is_callable<_Fn, const _ValueType&, const _ValueType&>::value
         >::type>
D_NODISCARD
_ValueType
reduce(
    const _Container& _input,
    _ValueType        _init,
    _Fn&&             _fn
)
{
    return fold_left(_input, std::move(_init), std::forward<_Fn>(_fn));
}


///////////////////////////////////////////////////////////////////////////////
///             V.    SCAN (PREFIX FOLD)                                    ///
///////////////////////////////////////////////////////////////////////////////

// scan (container)
//   function: like fold_left, but returns a vector of all
// intermediate accumulator values (inclusive prefix
// sums/products/etc.).
template<typename _Container,
         typename _Acc,
         typename _Fn,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value &&
             is_callable<_Fn, const _Acc&, const _ValueType&>::value
         >::type>
D_NODISCARD
std::vector<_Acc>
scan(
    const _Container& _input,
    _Acc              _init,
    _Fn&&             _fn
)
{
    std::vector<_Acc> result;

    internal::maybe_reserve(result,
        static_cast<std::size_t>(
            std::distance(std::begin(_input), std::end(_input))) + 1);

    result.push_back(_init);

    for (const auto& element : _input)
    {
        _init = std::forward<_Fn>(_fn)(
            static_cast<const _Acc&>(_init), element);
        result.push_back(_init);
    }

    return result;
}


///////////////////////////////////////////////////////////////////////////////
///             VI.   FOR-EACH                                             ///
///////////////////////////////////////////////////////////////////////////////

// for_each (container, mutable)
//   function: applies a consumer to each element of a mutable
// container.
template<typename _Container,
         typename _Fn,
         typename _ValueType = internal::container_value_type<_Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<_Container>::value &&
             is_callable<_Fn, _ValueType&>::value
         >::type>
void
for_each(
    _Container& _input,
    _Fn&& _fn
)
{
    for (auto& element : _input)
    {
        std::forward<_Fn>(_fn)(element);
    }

    return;
}

// for_each_const (container, immutable)
//   function: applies a const consumer to each element.
template<typename _Container,
         typename _Fn,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value &&
             is_callable<_Fn, const _ValueType&>::value
         >::type>
void
for_each_const(
    const _Container& _input,
    _Fn&& _fn
)
{
    for (const auto& element : _input)
    {
        std::forward<_Fn>(_fn)(element);
    }

    return;
}

// for_each_indexed (container)
//   function: applies a function that receives both index and
// element.
template<typename _Container,
         typename _Fn,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value &&
             is_callable<_Fn, std::size_t, const _ValueType&>::value
         >::type>
void
for_each_indexed(
    const _Container& _input,
    _Fn&& _fn
)
{
    std::size_t index = 0;

    for (const auto& element : _input)
    {
        std::forward<_Fn>(_fn)(index, element);
        ++index;
    }

    return;
}


///////////////////////////////////////////////////////////////////////////////
///             VII.  QUANTIFIERS                                           ///
///////////////////////////////////////////////////////////////////////////////

// any (container)
//   function: returns true if any element satisfies the predicate.
// Short-circuits on first match.
template<typename _Container,
         typename _Predicate,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value &&
             is_predicate<_Predicate, const _ValueType&>::value
         >::type>
D_NODISCARD
bool
any(
    const _Container& _input,
    _Predicate&&      _predicate
)
{
    for (const auto& element : _input)
    {
        if (std::forward<_Predicate>(_predicate)(element))
        {
            return true;
        }
    }

    return false;
}

// all (container)
//   function: returns true if all elements satisfy the predicate.
// Short-circuits on first failure.
template<typename _Container,
         typename _Predicate,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value &&
             is_predicate<_Predicate, const _ValueType&>::value
         >::type>
D_NODISCARD
bool
all(
    const _Container& _input,
    _Predicate&& _predicate
)
{
    for (const auto& element : _input)
    {
        if (!std::forward<_Predicate>(_predicate)(element))
        {
            return false;
        }
    }

    return true;
}

// none (container)
//   function: returns true if no element satisfies the predicate.
// Short-circuits on first match.
template<typename _Container,
         typename _Predicate,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value &&
             is_predicate<_Predicate, const _ValueType&>::value
         >::type>
D_NODISCARD
bool
none(
    const _Container& _input,
    _Predicate&&      _predicate
)
{
    return !any(_input, std::forward<_Predicate>(_predicate));
}


///////////////////////////////////////////////////////////////////////////////
///             VIII. SEARCHING AND COUNTING                                ///
///////////////////////////////////////////////////////////////////////////////

// count_if (container)
//   function: counts elements satisfying the predicate.
template<typename _Container,
         typename _Predicate,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value &&
             is_predicate<_Predicate, const _ValueType&>::value
         >::type>
D_NODISCARD
std::size_t
count_if(
    const _Container& _input,
    _Predicate&&      _predicate
)
{
    std::size_t result = 0;

    for (const auto& element : _input)
    {
        if (std::forward<_Predicate>(_predicate)(element))
        {
            ++result;
        }
    }

    return result;
}

// find_if (container)
//   function: returns a pointer to the first element satisfying
// the predicate, or nullptr if none found.
template<typename _Container,
         typename _Predicate,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value &&
             is_predicate<_Predicate, const _ValueType&>::value
         >::type>
D_NODISCARD
const _ValueType*
find_if(
    const _Container& _input,
    _Predicate&&      _predicate
)
{
    for (const auto& element : _input)
    {
        if (std::forward<_Predicate>(_predicate)(element))
        {
            return &element;
        }
    }

    return nullptr;
}

// find_if (mutable container)
//   function: returns a mutable pointer to the first matching
// element.
template<typename _Container,
         typename _Predicate,
         typename _ValueType = internal::container_value_type<_Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<_Container>::value &&
             is_predicate<_Predicate, const _ValueType&>::value &&
             !std::is_const<_Container>::value
         >::type>
D_NODISCARD
_ValueType*
find_if(
    _Container&  _input,
    _Predicate&& _predicate
)
{
    for (auto& element : _input)
    {
        if (std::forward<_Predicate>(_predicate)(element))
        {
            return &element;
        }
    }

    return nullptr;
}

// find_last (container)
//   function: returns a pointer to the last element satisfying
// the predicate, or nullptr if none found.
template<typename _Container,
         typename _Predicate,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value &&
             is_predicate<_Predicate, const _ValueType&>::value
         >::type>
D_NODISCARD
const _ValueType*
find_last(
    const _Container& _input,
    _Predicate&&      _predicate
)
{
    const _ValueType* found = nullptr;

    for (const auto& element : _input)
    {
        if (std::forward<_Predicate>(_predicate)(element))
        {
            found = &element;
        }
    }

    return found;
}

// index_of (container)
//   function: returns the index of the first element satisfying
// the predicate, or npos (size_t max) if none found.
template<typename _Container,
         typename _Predicate,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value &&
             is_predicate<_Predicate, const _ValueType&>::value
         >::type>
D_NODISCARD
std::size_t
index_of(
    const _Container& _input,
    _Predicate&& _predicate
)
{
    std::size_t idx = 0;

    for (const auto& element : _input)
    {
        if (std::forward<_Predicate>(_predicate)(element))
        {
            return idx;
        }

        ++idx;
    }

    return static_cast<std::size_t>(-1);
}

// last_index_of (container)
//   function: returns the index of the last element satisfying
// the predicate, or npos (size_t max) if none found.
template<typename _Container,
         typename _Predicate,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value &&
             is_predicate<_Predicate, const _ValueType&>::value
         >::type>
D_NODISCARD
std::size_t
last_index_of(
    const _Container& _input,
    _Predicate&& _predicate
)
{
    std::size_t found = static_cast<std::size_t>(-1);
    std::size_t idx   = 0;

    for (const auto& element : _input)
    {
        if (std::forward<_Predicate>(_predicate)(element))
        {
            found = idx;
        }

        ++idx;
    }

    return found;
}


///////////////////////////////////////////////////////////////////////////////
///             IX.   ORDERING                                              ///
///////////////////////////////////////////////////////////////////////////////

// is_sorted (container, with comparator)
//   function: returns true if the container is sorted according
// to the given comparator (which returns true if first < second).
template<typename _Container,
         typename _Compare,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value &&
             is_callable<_Compare, const _ValueType&, const _ValueType&>::value
         >::type>
D_NODISCARD
bool
is_sorted
(
    const _Container& _input,
    _Compare&&        _cmp)
{
    auto it  = std::begin(_input);
    auto end = std::end(_input);

    if (it == end)
    {
        return true;
    }

    auto prev = it;
    ++it;

    for (; it != end; ++it)
    {
        if (std::forward<_Compare>(_cmp)(*it, *prev))
        {
            return false;
        }

        prev = it;
    }

    return true;
}

// is_sorted (container, default ordering via operator<)
//   function: returns true if elements are in non-decreasing
// order.
template<typename _Container,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value
         >::type>
D_NODISCARD
bool
is_sorted(
    const _Container& _input
)
{
    return is_sorted(_input,
                     [](const _ValueType& _a, const _ValueType& _b)
                     {
                         return _a < _b;
                     });
}


///////////////////////////////////////////////////////////////////////////////
///             X.    TAKE AND SKIP                                         ///
///////////////////////////////////////////////////////////////////////////////

// take (container)
//   function: returns a vector of the first _n elements.
template<typename _Container,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value
         >::type>
D_NODISCARD
std::vector<_ValueType>
take(const _Container& _input, std::size_t _n)
{
    std::vector<_ValueType> result;
    std::size_t             count = 0;

    for (const auto& element : _input)
    {
        if (count >= _n)
        {
            break;
        }

        result.push_back(element);
        ++count;
    }

    return result;
}

// take_while (container)
//   function: takes elements while the predicate is true.
template<typename _Container,
         typename _Predicate,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value &&
             is_predicate<_Predicate, const _ValueType&>::value
         >::type>
D_NODISCARD
std::vector<_ValueType>
take_while(
    const _Container& _input,
    _Predicate&&      _predicate
)
{
    std::vector<_ValueType> result;

    for (const auto& element : _input)
    {
        if (!std::forward<_Predicate>(_predicate)(element))
        {
            break;
        }

        result.push_back(element);
    }

    return result;
}

// skip (container)
//   function: returns a vector with the first _n elements removed.
template<typename _Container,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value
         >::type>
D_NODISCARD
std::vector<_ValueType>
skip(
    const _Container& _input,
    std::size_t _n
)
{
    std::vector<_ValueType> result;
    std::size_t             count = 0;

    for (const auto& element : _input)
    {
        if (count >= _n)
        {
            result.push_back(element);
        }

        ++count;
    }

    return result;
}

// skip_while (container)
//   function: skips elements while the predicate is true, then
// takes the rest.
template<typename _Container,
         typename _Predicate,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value &&
             is_predicate<_Predicate, const _ValueType&>::value
         >::type>
D_NODISCARD
std::vector<_ValueType>
skip_while(
    const _Container& _input,
    _Predicate&&      _predicate
)
{
    std::vector<_ValueType> result;
    bool                    skipping = true;

    for (const auto& element : _input)
    {
        if (skipping && std::forward<_Predicate>(_predicate)(element))
        {
            continue;
        }

        skipping = false;
        result.push_back(element);
    }

    return result;
}


///////////////////////////////////////////////////////////////////////////////
///             XI.   FLAT-MAP AND ZIP                                      ///
///////////////////////////////////////////////////////////////////////////////

// flat_map (container)
//   function: applies a function that returns a container to each
// element, then flattens all results into a single vector.
template<typename _Container,
         typename _Fn,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename _InnerContainer = callable_result_t<_Fn, const _ValueType&>,
         typename _ResultType = internal::container_value_type<const _InnerContainer>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value &&
             is_callable<_Fn, const _ValueType&>::value &&
             internal::has_begin_end<const _InnerContainer>::value
         >::type>
D_NODISCARD
std::vector<_ResultType>
flat_map(
    const _Container& _input,
    _Fn&&             _fn
)
{
    std::vector<_ResultType> result;

    for (const auto& element : _input)
    {
        auto inner = std::forward<_Fn>(_fn)(element);

        for (const auto& inner_element : inner)
        {
            result.push_back(inner_element);
        }
    }

    return result;
}

// zip_with (two containers)
//   function: combines corresponding elements from two containers
// using a binary function. The result has the length of the
// shorter container.
template<typename _Container1,
         typename _Container2,
         typename _Fn,
         typename _Value1  = internal::container_value_type<const _Container1>,
         typename _Value2  = internal::container_value_type<const _Container2>,
         typename _ResultType = callable_result_t<_Fn, const _Value1&, const _Value2&>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container1>::value &&
             internal::has_begin_end<const _Container2>::value &&
             is_callable<_Fn, const _Value1&, const _Value2&>::value
         >::type>
D_NODISCARD
std::vector<_ResultType>
zip_with(
    const _Container1& _input1,
    const _Container2& _input2,
    _Fn&&              _fn
)
{
    std::vector<_ResultType> result;

    auto it1  = std::begin(_input1);
    auto it2  = std::begin(_input2);
    auto end1 = std::end(_input1);
    auto end2 = std::end(_input2);

    while (it1 != end1 && it2 != end2)
    {
        result.push_back(std::forward<_Fn>(_fn)(*it1, *it2));
        ++it1;
        ++it2;
    }

    return result;
}

// zip (two containers -> vector of pairs)
//   function: pairs corresponding elements from two containers.
template<typename _Container1,
         typename _Container2,
         typename _Value1 = internal::container_value_type<const _Container1>,
         typename _Value2 = internal::container_value_type<const _Container2>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container1>::value &&
             internal::has_begin_end<const _Container2>::value
         >::type>
D_NODISCARD
std::vector<std::pair<_Value1, _Value2>>
zip(
    const _Container1& _input1,
    const _Container2& _input2
)
{
    return zip_with(_input1, _input2,
                    [](const _Value1& _a, const _Value2& _b)
                    {
                        return std::make_pair(_a, _b);
                    });
}


///////////////////////////////////////////////////////////////////////////////
///             XII.  GROUP-BY AND PARTITION                                ///
///////////////////////////////////////////////////////////////////////////////

// group_by (container)
//   function: groups elements by a key function into a map of
// vectors.
template<typename _Container,
         typename _KeyFn,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename _KeyType   = callable_result_t<_KeyFn, const _ValueType&>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value &&
             is_callable<_KeyFn, const _ValueType&>::value
         >::type>
D_NODISCARD
std::map<_KeyType, std::vector<_ValueType>>
group_by(
    const _Container& _input,
    _KeyFn&&          _key_fn
)
{
    std::map<_KeyType, std::vector<_ValueType>> result;

    for (const auto& element : _input)
    {
        result[std::forward<_KeyFn>(_key_fn)(element)].push_back(element);
    }

    return result;
}

// partition (container)
//   function: splits elements into two vectors: those that pass
// and those that fail the predicate.
template<typename _Container,
         typename _Predicate,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value &&
             is_predicate<_Predicate, const _ValueType&>::value
         >::type>
D_NODISCARD
std::pair<std::vector<_ValueType>, std::vector<_ValueType>>
partition(
    const _Container& _input,
    _Predicate&&      _predicate
)
{
    std::vector<_ValueType> pass;
    std::vector<_ValueType> fail;

    for (const auto& element : _input)
    {
        if (std::forward<_Predicate>(_predicate)(element))
        {
            pass.push_back(element);
        }
        else
        {
            fail.push_back(element);
        }
    }

    return std::make_pair(std::move(pass), std::move(fail));
}


///////////////////////////////////////////////////////////////////////////////
///             XIII. DISTINCT AND REVERSE                                  ///
///////////////////////////////////////////////////////////////////////////////

// distinct (container, with equality comparator)
//   function: removes consecutive duplicates as determined by _eq.
template<typename _Container,
         typename _Eq,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value &&
             is_callable<_Eq, const _ValueType&, const _ValueType&>::value
         >::type>
D_NODISCARD
std::vector<_ValueType>
distinct(
    const _Container& _input,
    _Eq&&             _eq
)
{
    std::vector<_ValueType> result;

    for (const auto& element : _input)
    {
        bool found = false;

        for (const auto& existing : result)
        {
            if (std::forward<_Eq>(_eq)(element, existing))
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            result.push_back(element);
        }
    }

    return result;
}

// distinct (container, default equality via operator==)
//   function: removes duplicates using operator==.
template<typename _Container,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value
         >::type>
D_NODISCARD
std::vector<_ValueType>
distinct(
    const _Container& _input
)
{
    return distinct(_input,
                    [](const _ValueType& _a, const _ValueType& _b)
                    {
                        return _a == _b;
                    });
}

// reverse (container)
//   function: returns a vector with elements in reverse order.
template<typename _Container,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value
         >::type>
D_NODISCARD
std::vector<_ValueType>
reverse(const _Container& _input)
{
    std::vector<_ValueType> result(std::begin(_input), std::end(_input));

    std::reverse(result.begin(), result.end());

    return result;
}


///////////////////////////////////////////////////////////////////////////////
///             XIV.  SLICE AND RANGE                                       ///
///////////////////////////////////////////////////////////////////////////////

// slice (container)
//   function: returns elements in the range [start, end) with
// given step.
template<typename _Container,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value
         >::type>
D_NODISCARD
std::vector<_ValueType>
slice(
    const _Container& _input,
    std::size_t       _start,
    std::size_t       _end,
    std::size_t       _step = 1
)
{
    std::vector<_ValueType> result;
    std::size_t             idx = 0;

    if (_step == 0)
    {
        return result;
    }

    for (const auto& element : _input)
    {
        if (idx >= _end)
        {
            break;
        }

        if (idx >= _start && ((idx - _start) % _step == 0))
        {
            result.push_back(element);
        }

        ++idx;
    }

    return result;
}

// range (container)
//   function: returns elements in the half-open range
// [start, end).
template<typename _Container,
         typename _ValueType = internal::container_value_type<const _Container>,
         typename = typename std::enable_if<
             internal::has_begin_end<const _Container>::value
         >::type>
D_NODISCARD
std::vector<_ValueType>
range(
    const _Container& _input,
    std::size_t       _start,
    std::size_t       _end
)
{
    return slice(_input, _start, _end, 1);
}


NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_