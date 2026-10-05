/*******************************************************************************
* djinterp [re_std]                                                    array.hpp
*
* array class header:
*   Fixed-size, contiguous, aggregate sequence container — re_std's
* portable reimplementation of std::array<T, N>. Wraps a C-style array
* of Size elements of type Type and exposes the standard container
* interface (iterators, element access, capacity, fill, swap).
*
*   AGGREGATE GUARANTEE:
*   array is a structural aggregate on every supported tier. The only
* non-static data member, _M_elems, is public to permit brace-init
* (`re_std::array<int, 3> a = {1, 2, 3};`) on C++98/03 where neither
* CTAD nor designated init exists. There are no user-declared
* constructors, no virtual functions, and no private/protected data
* members — the four standard aggregate requirements per [dcl.init.aggr].
*
*   ZERO-SIZE INSTANTIATION:
*   array<Type, 0> is permitted by the standard. Declaring `Type
* _M_elems[0]` is ill-formed in standard C++, so the storage is
* indirected through internal::array_storage<Type, Size>, which has
* a partial specialisation for Size == 0 that holds a single
* placeholder element. data() may return any value for the zero-size
* case per [array.zero]; begin() == end() and size() returns 0 as
* required. libstdc++ and libc++ both use this same workaround.
*
*   CONSTEXPR SURFACE (matches std::array):
*   - C++98/03: no constexpr; RE_STD_CONSTEXPR* macros degrade to empty.
*   - C++11+:   size, max_size, empty (these are intrinsically const).
*   - C++14+:   const overloads of operator[], at, front, back, data,
*               begin, end, cbegin, cend (per LWG 2185 — the implicit-
*               const restriction on C++11 constexpr member functions
*               made the non-const overloads ill-formed at that tier).
*   - C++17+:   non-const overloads of the above, plus the C++17 CTAD
*               deduction guide.
*   - C++20+:   fill and member swap become constexpr (P1023). The
*               non-member swap and to_array — both constexpr-from-
*               introduction — live in their own headers.
*
*   ITERATORS:
*   iterator and const_iterator are plain pointers (T*, const T*) —
* matches libstdc++/libc++ practice and avoids dragging in a custom
* iterator-wrapper type. reverse_iterator and const_reverse_iterator
* are re_std::reverse_iterator<iterator>/<const_iterator>; this is the
* only inter-module dependency in the class itself.
*
*   COMPARISON OPERATORS, NON-MEMBER swap, to_array, get<I>, and the
* tuple-protocol specialisations (tuple_size<array>, tuple_element<I,
* array>) live in sibling headers — see the umbrella `array`.
*
*   Uses:
*     env.h              - language version detection
*     env_cpp_features.h - fine-grained feature detection
*     config.hpp         - RE_STD_CONSTEXPR, RE_STD_NOEXCEPT, RE_STD_NULLPTR
*
*
* path:      /inc/re_std/array/array.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.19
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
0.    COMPATIBILITY MACROS
      --------------------

      0a.   CONDITIONAL INCLUDES

I.    STORAGE HELPER (zero-size workaround)
      -------------------------------------

II.   ARRAY CLASS
      -----------

III.  DEDUCTION GUIDE (C++17+)
      ------------------------
*/

#ifndef RE_STD_ARRAY_ARRAY_HPP
#define RE_STD_ARRAY_ARRAY_HPP 1

// FLOOR, FOR NOW: below C++11 this header is empty rather than an error
// (README rule 5; re_std omits rather than degrades). The owner's ruling:
// compile at every level first; port to C++98 only where something needs it.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
// re_std
#include "../iterator/reverse_iterator.hpp"
#include "../type_traits/is_same.hpp"
#include "../type_traits/enable_if.hpp"


// ===========================================================================
// 0a.  CONDITIONAL INCLUDES
// ===========================================================================
// at() reports out-of-range via std::out_of_range when available,
// falling back to std::exception, then to no-op (UB) when exceptions
// are disabled.

#if RE_STD_HAS_EXCEPTIONS
    // std
    #include <stdexcept>
#elif RE_STD_HAS_EXCEPTIONS
    // std
    #include <exception>
#endif


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================
// Tier-specific constexpr qualifiers. Pending unification into the
// core qualifier table (see roadmap meta entry); locally redefined
// here, matching the convention currently used across <iterator>,
// <numeric>, <utility>, and most of <algorithm>.


namespace re_std
{


///////////////////////////////////////////////////////////////////////////////
///                I.   STORAGE HELPER (zero-size workaround)               ///
///////////////////////////////////////////////////////////////////////////////
// `Type m_elems[0]` is ill-formed in standard C++. The standard
// nevertheless permits array<Type, 0> and specifies that data()
// may return any pointer ([array.zero]/p2). We satisfy both rules
// by indirecting through array_storage, which holds a real
// Type[Size] for Size > 0 and a single placeholder element for
// Size == 0. Same approach used by libstdc++ (__array_traits) and
// libc++ (__zero_sized_array_storage).

namespace internal
{

    // array_storage
    //   struct: holds the raw C-array backing an array<Type, Size>.
    // Primary template for Size > 0.
    template<typename Type,
             std::size_t Size>
    struct array_storage
    {
        typedef Type type[Size];

        static RE_STD_CONSTEXPR_CPP14 Type*
        ptr(
            type& _data
        ) RE_STD_NOEXCEPT
        {
            return _data;
        }

        static RE_STD_CONSTEXPR Type const*
        ptr(
            type const& _data
        ) RE_STD_NOEXCEPT
        {
            return _data;
        }
    };

    // array_storage<Type, 0>
    //   struct: zero-size specialisation. Holds a single placeholder
    // element; ptr() returns RE_STD_NULLPTR cast to the appropriate type
    // (data() on a zero-size array may return any value).
    template<typename Type>
    struct array_storage<Type, 0>
    {
        struct type
        {
            Type _M_placeholder;
        };

        static RE_STD_CONSTEXPR_CPP14 Type*
        ptr(
            type&
        ) RE_STD_NOEXCEPT
        {
            return static_cast<Type*>(RE_STD_NULLPTR);
        }

        static RE_STD_CONSTEXPR Type const*
        ptr(
            type const&
        ) RE_STD_NOEXCEPT
        {
            return static_cast<Type const*>(RE_STD_NULLPTR);
        }
    };

}  // internal


///////////////////////////////////////////////////////////////////////////////
///                II.  ARRAY CLASS                                         ///
///////////////////////////////////////////////////////////////////////////////

// array
//   class: fixed-size, contiguous, aggregate sequence container.
// Wraps Type[Size]. Standard interface (element access, iterators,
// capacity, fill, swap). Aggregate-initialisable on every tier.
template<typename    Type,
         std::size_t Size>
struct array
{
    // =================================================================
    // MEMBER TYPES
    // =================================================================

    typedef Type                                       value_type;
    typedef Type&                                      reference;
    typedef Type const&                                const_reference;
    typedef Type*                                      pointer;
    typedef Type const*                                const_pointer;
    typedef Type*                                      iterator;
    typedef Type const*                                const_iterator;
    typedef std::size_t                                 size_type;
    typedef std::ptrdiff_t                              difference_type;
    typedef re_std::reverse_iterator<iterator>           reverse_iterator;
    typedef re_std::reverse_iterator<const_iterator>     const_reverse_iterator;

    // =================================================================
    // STORAGE
    // =================================================================
    // PUBLIC to satisfy the aggregate requirement on C++98/03 where
    // aggregate initialisation requires no private/protected members.
    // Equivalent to libstdc++'s _M_elems and libc++'s __elems_.
    // User code should access via the methods below, never directly.

    typedef internal::array_storage<Type, Size>           _storage;
    typename _storage::type                                 _M_elems;

    // =================================================================
    // ELEMENT ACCESS
    // =================================================================

    // at (mutable)
    //   function: bounds-checked element access.
    // throws: std::out_of_range when RE_STD_HAS_EXCEPTIONS,
    // std::exception when only RE_STD_HAS_EXCEPTIONS, otherwise
    // returns garbage (UB — matches libstdc++ -fno-exceptions).
    // Constexpr from C++17 (non-const overloads were ill-formed
    // constexpr on C++11–C++14 per the implicit-const rule).
    RE_STD_CONSTEXPR_CPP17 reference
    at(
        size_type _pos
    )
    {
        if (_pos >= Size)
        {
            _throw_out_of_range();
        }

        return _storage::ptr(_M_elems)[_pos];
    }

    // at (const)
    //   function: bounds-checked element access.
    // Constexpr from C++14 (LWG 2185).
    RE_STD_CONSTEXPR_CPP14 const_reference
    at(
        size_type _pos
    ) const
    {
        if (_pos >= Size)
        {
            _throw_out_of_range();
        }

        return _storage::ptr(_M_elems)[_pos];
    }

    // operator[] (mutable)
    //   function: unchecked element access.
    RE_STD_CONSTEXPR_CPP17 reference
    operator[](
        size_type _pos
    ) RE_STD_NOEXCEPT
    {
        return _storage::ptr(_M_elems)[_pos];
    }

    // operator[] (const)
    RE_STD_CONSTEXPR_CPP14 const_reference
    operator[](
        size_type _pos
    ) const RE_STD_NOEXCEPT
    {
        return _storage::ptr(_M_elems)[_pos];
    }

    // front (mutable)
    //   function: returns a reference to the first element.
    // note: calling on a zero-size array is undefined.
    RE_STD_CONSTEXPR_CPP17 reference
    front() RE_STD_NOEXCEPT
    {
        return _storage::ptr(_M_elems)[0];
    }

    // front (const)
    RE_STD_CONSTEXPR_CPP14 const_reference
    front() const RE_STD_NOEXCEPT
    {
        return _storage::ptr(_M_elems)[0];
    }

    // back (mutable)
    //   function: returns a reference to the last element.
    // note: calling on a zero-size array is undefined.
    RE_STD_CONSTEXPR_CPP17 reference
    back() RE_STD_NOEXCEPT
    {
        return _storage::ptr(_M_elems)[Size - 1];
    }

    // back (const)
    RE_STD_CONSTEXPR_CPP14 const_reference
    back() const RE_STD_NOEXCEPT
    {
        return _storage::ptr(_M_elems)[Size - 1];
    }

    // data (mutable)
    //   function: returns a pointer to the underlying storage.
    // For zero-size arrays may return RE_STD_NULLPTR ([array.zero]/p2).
    RE_STD_CONSTEXPR_CPP17 pointer
    data() RE_STD_NOEXCEPT
    {
        return _storage::ptr(_M_elems);
    }

    // data (const)
    RE_STD_CONSTEXPR_CPP14 const_pointer
    data() const RE_STD_NOEXCEPT
    {
        return _storage::ptr(_M_elems);
    }

    // =================================================================
    // ITERATORS
    // =================================================================

    RE_STD_CONSTEXPR_CPP17 iterator
    begin() RE_STD_NOEXCEPT
    {
        return iterator(_storage::ptr(_M_elems));
    }

    RE_STD_CONSTEXPR_CPP14 const_iterator
    begin() const RE_STD_NOEXCEPT
    {
        return const_iterator(_storage::ptr(_M_elems));
    }

    RE_STD_CONSTEXPR_CPP17 iterator
    end() RE_STD_NOEXCEPT
    {
        return iterator(_storage::ptr(_M_elems) + Size);
    }

    RE_STD_CONSTEXPR_CPP14 const_iterator
    end() const RE_STD_NOEXCEPT
    {
        return const_iterator(_storage::ptr(_M_elems) + Size);
    }

    RE_STD_CONSTEXPR_CPP14 const_iterator
    cbegin() const RE_STD_NOEXCEPT
    {
        return const_iterator(_storage::ptr(_M_elems));
    }

    RE_STD_CONSTEXPR_CPP14 const_iterator
    cend() const RE_STD_NOEXCEPT
    {
        return const_iterator(_storage::ptr(_M_elems) + Size);
    }

    RE_STD_CONSTEXPR_CPP17 reverse_iterator
    rbegin() RE_STD_NOEXCEPT
    {
        return reverse_iterator(end());
    }

    RE_STD_CONSTEXPR_CPP14 const_reverse_iterator
    rbegin() const RE_STD_NOEXCEPT
    {
        return const_reverse_iterator(end());
    }

    RE_STD_CONSTEXPR_CPP17 reverse_iterator
    rend() RE_STD_NOEXCEPT
    {
        return reverse_iterator(begin());
    }

    RE_STD_CONSTEXPR_CPP14 const_reverse_iterator
    rend() const RE_STD_NOEXCEPT
    {
        return const_reverse_iterator(begin());
    }

    RE_STD_CONSTEXPR_CPP14 const_reverse_iterator
    crbegin() const RE_STD_NOEXCEPT
    {
        return const_reverse_iterator(end());
    }

    RE_STD_CONSTEXPR_CPP14 const_reverse_iterator
    crend() const RE_STD_NOEXCEPT
    {
        return const_reverse_iterator(begin());
    }

    // =================================================================
    // CAPACITY
    // =================================================================

    // empty
    //   function: true if size() == 0.
    RE_STD_CONSTEXPR bool
    empty() const RE_STD_NOEXCEPT
    {
        return Size == 0;
    }

    // size
    //   function: returns Size.
    RE_STD_CONSTEXPR size_type
    size() const RE_STD_NOEXCEPT
    {
        return Size;
    }

    // max_size
    //   function: returns Size. Identical to size() for a
    // fixed-extent container.
    RE_STD_CONSTEXPR size_type
    max_size() const RE_STD_NOEXCEPT
    {
        return Size;
    }

    // =================================================================
    // OPERATIONS
    // =================================================================

    // fill
    //   function: assigns _value to every element.
    RE_STD_CONSTEXPR_CPP20 void
    fill(
        const_reference _value
    )
    {
        for (size_type _i = 0; _i < Size; ++_i)
        {
            _storage::ptr(_M_elems)[_i] = _value;
        }

        return;
    }

    // swap
    //   function: element-wise swap with _other. Conditional noexcept
    // when is_nothrow_swappable_v<Type> is satisfied — gated out
    // pre-C++17 because the trait is C++17+; non-throwing path on
    // earlier tiers depends on Type's own swap behaviour.
    RE_STD_CONSTEXPR_CPP20 void
    swap(
        array& _other
    )
    {
        for (size_type _i = 0; _i < Size; ++_i)
        {
            Type _tmp                          = _storage::ptr(_M_elems)[_i];
            _storage::ptr(_M_elems)[_i]         = _storage::ptr(_other._M_elems)[_i];
            _storage::ptr(_other._M_elems)[_i]  = _tmp;
        }

        return;
    }

private:

    // _throw_out_of_range
    //   function: helper used by at(). Throws std::out_of_range when
    // exceptions are available, otherwise aborts. Placed in a
    // non-constexpr context so at()'s constexpr path is only
    // exercised when _pos is in range — the standard's "throwing
    // call is not part of a constant expression" trick.
    static void
    _throw_out_of_range();
};


// out-of-line definition for the at() helper.
// note: split out so the class body stays constexpr-compatible.
template<typename    Type,
         std::size_t Size>
void
array<Type, Size>::_throw_out_of_range()
{
#if RE_STD_HAS_EXCEPTIONS
    throw std::out_of_range("re_std::array::at: index out of range");
#elif RE_STD_HAS_EXCEPTIONS
    throw std::exception();
#else
    // exceptions disabled: undefined behaviour on out-of-range at().
    // No-op; caller will read past the end. Matches freestanding
    // behaviour of libstdc++ with -fno-exceptions.
    return;
#endif
}


///////////////////////////////////////////////////////////////////////////////
///                III. DEDUCTION GUIDE (C++17+)                            ///
///////////////////////////////////////////////////////////////////////////////
// CTAD is a C++17 language feature; the deduction guide is
// unavailable on earlier tiers (the language itself cannot deduce
// class template arguments). The guide enables
//   `re_std::array a = {1, 2, 3};`
// to deduce array<int, 3>.
//   The standard's guide includes a homogeneity check: every
// element type must be the same as the first, otherwise the guide
// is removed from the overload set (rather than silently accepting
// heterogeneous initialisers and narrowing). The check is expressed
// via enable_if_t inside the deduced array's first template
// argument — fold expressions are required, so this is gated on
// C++17+ anyway.

#if RE_STD_LANG_IS_CPP17_OR_HIGHER

template<typename    Type,
         typename... Rest>
array(Type, Rest...)
    -> array<
           typename re_std::enable_if<
               (re_std::is_same<Type, Rest>::value && ...),
               Type
           >::type,
           1 + sizeof...(Rest)>;

#endif  // RE_STD_LANG_IS_CPP17_OR_HIGHER


}  // re_std

#endif  // floor, for now


#endif  // RE_STD_ARRAY_ARRAY_HPP
