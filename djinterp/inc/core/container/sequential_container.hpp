/******************************************************************************
* djinterp [container]                               sequential_container.hpp
*
* CRTP base for sequential (order-preserving) containers.
*   A sequential container is any container for which element order is
* defined and stable.  This includes:
*   - arrays (C arrays, std::array, std::vector, std::deque)
*   - lists (std::list, std::forward_list)
*   - strings (std::string, std::string_view)
*   - custom ordered containers (stacks, queues, ring buffers)
*
*   It does NOT include:
*   - unordered associative containers (std::unordered_map/set)
*   - cyclic graphs (no stable "first" element)
*   - multisets (iteration order is defined by comparator, not
*     insertion — use is_sorted_container instead)
*
*   The CRTP base provides order-dependent operations that work on
* any sequential container without knowing its storage layout:
*   - positional access (at, front, back)
*   - slicing (sub-range extraction)
*   - rotation and reversal
*   - shift operations (logical shift left/right by N positions)
*   - order predicates (is_sorted, is_palindrome, etc.)
*
*   Derived classes implement the storage and expose begin()/end().
* This base never allocates or owns data.
*
* TABLE OF CONTENTS
* =================
* I.      Sequential Container Traits
* II.     sequential_base (CRTP)
* III.    Free-Function Order Algorithms
*
*
* path:      \inc\container\sequential_container.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2026.03.24
******************************************************************************/

#ifndef DJINTERP_SEQUENTIAL_CONTAINER_
#define DJINTERP_SEQUENTIAL_CONTAINER_ 1

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include "..\djinterp.hpp"
#include "meta\container_traits.hpp"
#include "meta\iterator_traits.hpp"


NS_DJINTERP
NS_CONTAINER

// =============================================================================
// I.   Sequential Container Traits
// =============================================================================

NS_TRAITS

// is_sequential_container
//   type trait: true if the container preserves insertion
// order.  Structurally: iterable + NOT unordered
// associative (no hasher/key_equal) + NOT flagged as
// unordered.
//
// This is broader than is_sequence_container (which
// requires front/insert/erase per the STL named
// requirement).  A forward_list is sequential but not
// always a full SequenceContainer.
NS_INTERNAL

    template<typename _C, typename = void>
    struct has_hasher_check : std::false_type
    {};

    template<typename _C>
    struct has_hasher_check<_C,
        std::void_t<typename _C::hasher>>
        : std::true_type
    {};

NS_END  // internal

template<typename _Type>
struct is_sequential_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( is_iterable_container_v<clean_type>     &&
          !internal::has_hasher_check<
               clean_type>::value );
};

template<typename _Type>
inline constexpr bool is_sequential_container_v =
    is_sequential_container<_Type>::value;

// DSequentialKind
//   enum: classifies the sequential storage model.
enum class DSequentialKind
{
    // unknown or non-sequential
    none,

    // contiguous random-access (array, vector)
    array_like,

    // node-based bidirectional (list)
    list_like,

    // node-based forward-only (forward_list, slist)
    forward_list_like,

    // double-ended contiguous segments (deque)
    deque_like,

    // string / text buffer
    string_like
};

NS_INTERNAL

    template<typename _Type, typename = void>
    struct has_c_str_check : std::false_type
    {};

    template<typename _Type>
    struct has_c_str_check<_Type,
        std::void_t<decltype(
            std::declval<const _Type&>().c_str())>>
        : std::true_type
    {};

    template<typename _Type>
    struct sequential_kind_impl
    {
        using C = clean_t<_Type>;

        static constexpr DSequentialKind value =
            // string-like (has c_str())
            has_c_str_check<C>::value
                ? DSequentialKind::string_like

            // contiguous + random-access
            : ( has_data_accessor_v<C> &&
                is_random_access_iterable_v<C> )
                ? DSequentialKind::array_like

            // bidirectional but not contiguous
            : ( is_bidirectional_iterable_v<C> &&
                !has_data_accessor_v<C> )
                ? DSequentialKind::list_like

            // forward-only
            : ( is_forward_iterable_v<C> &&
                !is_bidirectional_iterable_v<C> )
                ? DSequentialKind::forward_list_like

            // random-access but not contiguous
            // (deque pattern)
            : ( is_random_access_iterable_v<C> &&
                !has_data_accessor_v<C> )
                ? DSequentialKind::deque_like

            : DSequentialKind::none;
    };

NS_END  // internal

template<typename _Type>
struct sequential_kind
{
    static constexpr DSequentialKind value =
        internal::sequential_kind_impl<_Type>::value;
};

template<typename _Type>
inline constexpr DSequentialKind
    sequential_kind_v =
        sequential_kind<_Type>::value;

NS_END  // traits


// =============================================================================
// II.  sequential_base (CRTP)
// =============================================================================
// Provides order-dependent operations for any sequential
// container.  The derived class must expose:
//   - begin() / end()
//   - size()
// Optional for full functionality:
//   - operator[] (for positional access)
//   - push_back / insert (for mutable operations)

template<typename _Derived>
class sequential_base
{
protected:
    sequential_base()  = default;
    ~sequential_base() = default;

private:
    _Derived& self()
    {
        return static_cast<_Derived&>(*this);
    }

    const _Derived& self() const
    {
        return static_cast<const _Derived&>(*this);
    }

public:
    // --- positional access ---

    auto front() const
        -> decltype(*std::begin(
               std::declval<const _Derived&>()))
    {
        return *std::begin(self());
    }

    auto back() const
        -> decltype(*std::begin(
               std::declval<const _Derived&>()))
    {
        auto it = std::end(self());
        --it;

        return *it;
    }

    // nth
    //   returns iterator to the Nth element.
    auto nth(std::size_t _n) const
    {
        auto it = std::begin(self());

        std::advance(it, _n);

        return it;
    }

    // --- order predicates ---

    // is_sorted (default ordering)
    bool is_sorted() const
    {
        return std::is_sorted(
            std::begin(self()),
            std::end(self()));
    }

    // is_sorted (custom comparator)
    template<typename _Compare>
    bool is_sorted(_Compare _cmp) const
    {
        return std::is_sorted(
            std::begin(self()),
            std::end(self()),
            _cmp);
    }

    // is_palindrome
    //   true if the sequence reads the same forwards and
    // backwards.
    bool is_palindrome() const
    {
        auto fwd = std::begin(self());
        auto rev = std::end(self());

        if (fwd == rev)
        {
            return true;
        }

        --rev;

        while (fwd != rev)
        {
            if (!(*fwd == *rev))
            {
                return false;
            }

            ++fwd;

            if (fwd == rev)
            {
                break;
            }

            --rev;
        }

        return true;
    }

    // --- mutating order operations ---
    // These modify the derived container in-place.

    // reverse
    void reverse()
    {
        std::reverse(
            std::begin(self()),
            std::end(self()));
    }

    // rotate_left
    //   rotates elements left by _n positions.
    void rotate_left(std::size_t _n)
    {
        auto bg = std::begin(self());

        std::advance(bg,
            _n % self().size());

        std::rotate(
            std::begin(self()),
            bg,
            std::end(self()));
    }

    // rotate_right
    //   rotates elements right by _n positions.
    void rotate_right(std::size_t _n)
    {
        std::size_t sz = self().size();

        if (sz == 0)
        {
            return;
        }

        rotate_left(sz - (_n % sz));
    }

    // shift_left
    //   shifts elements left by _n, filling vacated
    // positions with _fill.
    template<typename _Value>
    void shift_left(std::size_t _n,
                    const _Value& _fill)
    {
        std::size_t sz = self().size();

        if (_n >= sz)
        {
            std::fill(
                std::begin(self()),
                std::end(self()),
                _fill);

            return;
        }

        auto src = std::begin(self());
        std::advance(src, _n);

        std::move(src,
                  std::end(self()),
                  std::begin(self()));

        auto tail = std::end(self());
        std::advance(tail,
            -static_cast<std::ptrdiff_t>(_n));

        std::fill(tail,
                  std::end(self()),
                  _fill);
    }

    // shift_right
    //   shifts elements right by _n, filling vacated
    // positions with _fill.
    template<typename _Value>
    void shift_right(std::size_t _n,
                     const _Value& _fill)
    {
        std::size_t sz = self().size();

        if (_n >= sz)
        {
            std::fill(
                std::begin(self()),
                std::end(self()),
                _fill);

            return;
        }

        auto dst = std::end(self());
        auto src = std::end(self());
        std::advance(src,
            -static_cast<std::ptrdiff_t>(_n));

        std::move_backward(
            std::begin(self()),
            src,
            dst);

        auto head = std::begin(self());
        std::advance(head, _n);

        std::fill(std::begin(self()),
                  head,
                  _fill);
    }
};


// =============================================================================
// III. Free-Function Order Algorithms
// =============================================================================
// Non-member algorithms that work on any sequential
// container.

// starts_with
//   function: true if _container begins with _prefix.
template<typename _Container,
         typename _Prefix>
inline typename std::enable_if<
    traits::is_sequential_container_v<_Container> &&
    traits::is_sequential_container_v<_Prefix>,
    bool
>::type
starts_with(const _Container& _container,
            const _Prefix&    _prefix)
{
    auto c_it  = std::begin(_container);
    auto c_end = std::end(_container);
    auto p_it  = std::begin(_prefix);
    auto p_end = std::end(_prefix);

    for (; p_it != p_end; ++c_it, ++p_it)
    {
        if (c_it == c_end || !(*c_it == *p_it))
        {
            return false;
        }
    }

    return true;
}

// ends_with
//   function: true if _container ends with _suffix.
// Requires bidirectional iteration.
template<typename _Container,
         typename _Suffix>
inline typename std::enable_if<
    traits::is_sequential_container_v<_Container> &&
    traits::is_sequential_container_v<_Suffix>,
    bool
>::type
ends_with(const _Container& _container,
          const _Suffix&    _suffix)
{
    auto c_sz = _container.size();
    auto s_sz = _suffix.size();

    if (s_sz > c_sz)
    {
        return false;
    }

    auto c_it = std::begin(_container);
    std::advance(c_it, c_sz - s_sz);

    auto s_it = std::begin(_suffix);

    for (; s_it != std::end(_suffix);
         ++c_it, ++s_it)
    {
        if (!(*c_it == *s_it))
        {
            return false;
        }
    }

    return true;
}

// contains_subsequence
//   function: true if _sub appears as a contiguous
// subsequence within _container.
template<typename _Container,
         typename _Sub>
inline typename std::enable_if<
    traits::is_sequential_container_v<_Container> &&
    traits::is_sequential_container_v<_Sub>,
    bool
>::type
contains_subsequence(const _Container& _container,
                     const _Sub&       _sub)
{
    return (std::search(
        std::begin(_container),
        std::end(_container),
        std::begin(_sub),
        std::end(_sub)) != std::end(_container));
}


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_SEQUENTIAL_CONTAINER_
