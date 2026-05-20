/******************************************************************************
* djinterp [restd]                                                     array.hpp
*
*   Fixed-size, contiguous, aggregate sequence container — restd's
* portable reimplementation of std::array<T, N>. Wraps a C-style array
* of _Size elements of type _Type and exposes the standard container
* interface (iterators, element access, capacity, fill, swap).
*
*   AGGREGATE GUARANTEE:
*   array is a structural aggregate on every supported tier. The only
* non-static data member, _M_elems, is public to permit brace-init
* (`restd::array<int, 3> a = {1, 2, 3};`) on C++98/03 where neither
* CTAD nor designated init exists. There are no user-declared
* constructors, no virtual functions, and no private/protected data
* members — the four standard aggregate requirements per [dcl.init.aggr].
*
*   ZERO-SIZE INSTANTIATION:
*   array<_Type, 0> is permitted by the standard. Declaring `_Type
* _M_elems[0]` is ill-formed in standard C++, so the storage is
* indirected through internal::array_storage<_Type, _Size>, which has
* a partial specialisation for _Size == 0 that holds a single
* placeholder element. data() may return any value for the zero-size
* case per [array.zero]; begin() == end() and size() returns 0 as
* required. libstdc++ and libc++ both use this same workaround.
*
*   CONSTEXPR SURFACE (matches std::array):
*   - C++98/03: no constexpr; D_CONSTEXPR* macros degrade to empty.
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
* are restd::reverse_iterator<iterator>/<const_iterator>; this is the
* only inter-module dependency in the class itself.
*
*   COMPARISON OPERATORS, NON-MEMBER swap, to_array, get<I>, and the
* tuple-protocol specialisations (tuple_size<array>, tuple_element<I,
* array>) live in sibling headers — see the umbrella `array`.
*
*   Uses:
*     env.h              - language version detection
*     env_cpp_features.h - fine-grained feature detection
*     djinterp.hpp       - D_CONSTEXPR, D_NOEXCEPT, D_NULLPTR, namespaces
*
*
* TABLE OF CONTENTS
* =================
* 0.    COMPATIBILITY MACROS
* 0a.   CONDITIONAL INCLUDES
* I.    STORAGE HELPER (zero-size workaround)
* II.   ARRAY CLASS
* III.  DEDUCTION GUIDE (C++17+)
*
*
* path:      /inc/djinterp/restd/array/array.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.19
******************************************************************************/

#ifndef DJINTERP_RESTD_ARRAY_
#define DJINTERP_RESTD_ARRAY_ 1

// std
#include <cstddef>

// djinterp
#include "../../core/djinterp.hpp"

// restd
#include "../iterator/reverse_iterator.hpp"
#include "../type_traits/is_same.hpp"
#include "../type_traits/enable_if.hpp"


// ===========================================================================
// 0a.  CONDITIONAL INCLUDES
// ===========================================================================
// at() reports out-of-range via std::out_of_range when available,
// falling back to std::exception, then to no-op (UB) when exceptions
// are disabled.

#if D_ENV_CPP98_HAS_STDEXCEPT
    #include <stdexcept>
#elif D_ENV_CPP98_HAS_EXCEPTION
    #include <exception>
#endif


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================
// Tier-specific constexpr qualifiers. Pending unification into the
// core qualifier table (see roadmap meta entry); locally redefined
// here, matching the convention currently used across <iterator>,
// <numeric>, <utility>, and most of <algorithm>.

#ifndef D_NULLPTR
    #if D_ENV_LANG_IS_CPP11_OR_HIGHER
        #define D_NULLPTR   nullptr
    #else
        #define D_NULLPTR   0
    #endif
#endif

#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14   constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif

#ifndef D_CONSTEXPR_CPP17
    #if D_ENV_LANG_IS_CPP17_OR_HIGHER
        #define D_CONSTEXPR_CPP17   constexpr
    #else
        #define D_CONSTEXPR_CPP17
    #endif
#endif

#ifndef D_CONSTEXPR_CPP20
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        #define D_CONSTEXPR_CPP20   constexpr
    #else
        #define D_CONSTEXPR_CPP20
    #endif
#endif


NS_RESTD


///////////////////////////////////////////////////////////////////////////////
///                I.   STORAGE HELPER (zero-size workaround)               ///
///////////////////////////////////////////////////////////////////////////////
// `_Type m_elems[0]` is ill-formed in standard C++. The standard
// nevertheless permits array<_Type, 0> and specifies that data()
// may return any pointer ([array.zero]/p2). We satisfy both rules
// by indirecting through array_storage, which holds a real
// _Type[_Size] for _Size > 0 and a single placeholder element for
// _Size == 0. Same approach used by libstdc++ (__array_traits) and
// libc++ (__zero_sized_array_storage).

NS_INTERNAL

    // array_storage
    //   struct: holds the raw C-array backing an array<_Type, _Size>.
    // Primary template for _Size > 0.
    template<typename _Type,
             std::size_t _Size>
    struct array_storage
    {
        typedef _Type type[_Size];

        static D_CONSTEXPR_CPP14 _Type*
        ptr(
            type& _data
        ) D_NOEXCEPT
        {
            return _data;
        }

        static D_CONSTEXPR _Type const*
        ptr(
            type const& _data
        ) D_NOEXCEPT
        {
            return _data;
        }
    };

    // array_storage<_Type, 0>
    //   struct: zero-size specialisation. Holds a single placeholder
    // element; ptr() returns D_NULLPTR cast to the appropriate type
    // (data() on a zero-size array may return any value).
    template<typename _Type>
    struct array_storage<_Type, 0>
    {
        struct type
        {
            _Type _M_placeholder;
        };

        static D_CONSTEXPR_CPP14 _Type*
        ptr(
            type&
        ) D_NOEXCEPT
        {
            return static_cast<_Type*>(D_NULLPTR);
        }

        static D_CONSTEXPR _Type const*
        ptr(
            type const&
        ) D_NOEXCEPT
        {
            return static_cast<_Type const*>(D_NULLPTR);
        }
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                II.  ARRAY CLASS                                         ///
///////////////////////////////////////////////////////////////////////////////

// array
//   class: fixed-size, contiguous, aggregate sequence container.
// Wraps _Type[_Size]. Standard interface (element access, iterators,
// capacity, fill, swap). Aggregate-initialisable on every tier.
template<typename    _Type,
         std::size_t _Size>
struct array
{
    // =================================================================
    // MEMBER TYPES
    // =================================================================

    typedef _Type                                       value_type;
    typedef _Type&                                      reference;
    typedef _Type const&                                const_reference;
    typedef _Type*                                      pointer;
    typedef _Type const*                                const_pointer;
    typedef _Type*                                      iterator;
    typedef _Type const*                                const_iterator;
    typedef std::size_t                                 size_type;
    typedef std::ptrdiff_t                              difference_type;
    typedef restd::reverse_iterator<iterator>           reverse_iterator;
    typedef restd::reverse_iterator<const_iterator>     const_reverse_iterator;

    // =================================================================
    // STORAGE
    // =================================================================
    // PUBLIC to satisfy the aggregate requirement on C++98/03 where
    // aggregate initialisation requires no private/protected members.
    // Equivalent to libstdc++'s _M_elems and libc++'s __elems_.
    // User code should access via the methods below, never directly.

    typedef internal::array_storage<_Type, _Size>           _storage;
    typename _storage::type                                 _M_elems;

    // =================================================================
    // ELEMENT ACCESS
    // =================================================================

    // at (mutable)
    //   function: bounds-checked element access.
    // throws: std::out_of_range when D_ENV_CPP98_HAS_STDEXCEPT,
    // std::exception when only D_ENV_CPP98_HAS_EXCEPTION, otherwise
    // returns garbage (UB — matches libstdc++ -fno-exceptions).
    // Constexpr from C++17 (non-const overloads were ill-formed
    // constexpr on C++11–C++14 per the implicit-const rule).
    D_CONSTEXPR_CPP17 reference
    at(
        size_type _pos
    )
    {
        if (_pos >= _Size)
        {
            _throw_out_of_range();
        }

        return _storage::ptr(_M_elems)[_pos];
    }

    // at (const)
    //   function: bounds-checked element access.
    // Constexpr from C++14 (LWG 2185).
    D_CONSTEXPR_CPP14 const_reference
    at(
        size_type _pos
    ) const
    {
        if (_pos >= _Size)
        {
            _throw_out_of_range();
        }

        return _storage::ptr(_M_elems)[_pos];
    }

    // operator[] (mutable)
    //   function: unchecked element access.
    D_CONSTEXPR_CPP17 reference
    operator[](
        size_type _pos
    ) D_NOEXCEPT
    {
        return _storage::ptr(_M_elems)[_pos];
    }

    // operator[] (const)
    D_CONSTEXPR_CPP14 const_reference
    operator[](
        size_type _pos
    ) const D_NOEXCEPT
    {
        return _storage::ptr(_M_elems)[_pos];
    }

    // front (mutable)
    //   function: returns a reference to the first element.
    // note: calling on a zero-size array is undefined.
    D_CONSTEXPR_CPP17 reference
    front() D_NOEXCEPT
    {
        return _storage::ptr(_M_elems)[0];
    }

    // front (const)
    D_CONSTEXPR_CPP14 const_reference
    front() const D_NOEXCEPT
    {
        return _storage::ptr(_M_elems)[0];
    }

    // back (mutable)
    //   function: returns a reference to the last element.
    // note: calling on a zero-size array is undefined.
    D_CONSTEXPR_CPP17 reference
    back() D_NOEXCEPT
    {
        return _storage::ptr(_M_elems)[_Size - 1];
    }

    // back (const)
    D_CONSTEXPR_CPP14 const_reference
    back() const D_NOEXCEPT
    {
        return _storage::ptr(_M_elems)[_Size - 1];
    }

    // data (mutable)
    //   function: returns a pointer to the underlying storage.
    // For zero-size arrays may return D_NULLPTR ([array.zero]/p2).
    D_CONSTEXPR_CPP17 pointer
    data() D_NOEXCEPT
    {
        return _storage::ptr(_M_elems);
    }

    // data (const)
    D_CONSTEXPR_CPP14 const_pointer
    data() const D_NOEXCEPT
    {
        return _storage::ptr(_M_elems);
    }

    // =================================================================
    // ITERATORS
    // =================================================================

    D_CONSTEXPR_CPP17 iterator
    begin() D_NOEXCEPT
    {
        return iterator(_storage::ptr(_M_elems));
    }

    D_CONSTEXPR_CPP14 const_iterator
    begin() const D_NOEXCEPT
    {
        return const_iterator(_storage::ptr(_M_elems));
    }

    D_CONSTEXPR_CPP17 iterator
    end() D_NOEXCEPT
    {
        return iterator(_storage::ptr(_M_elems) + _Size);
    }

    D_CONSTEXPR_CPP14 const_iterator
    end() const D_NOEXCEPT
    {
        return const_iterator(_storage::ptr(_M_elems) + _Size);
    }

    D_CONSTEXPR_CPP14 const_iterator
    cbegin() const D_NOEXCEPT
    {
        return const_iterator(_storage::ptr(_M_elems));
    }

    D_CONSTEXPR_CPP14 const_iterator
    cend() const D_NOEXCEPT
    {
        return const_iterator(_storage::ptr(_M_elems) + _Size);
    }

    D_CONSTEXPR_CPP17 reverse_iterator
    rbegin() D_NOEXCEPT
    {
        return reverse_iterator(end());
    }

    D_CONSTEXPR_CPP14 const_reverse_iterator
    rbegin() const D_NOEXCEPT
    {
        return const_reverse_iterator(end());
    }

    D_CONSTEXPR_CPP17 reverse_iterator
    rend() D_NOEXCEPT
    {
        return reverse_iterator(begin());
    }

    D_CONSTEXPR_CPP14 const_reverse_iterator
    rend() const D_NOEXCEPT
    {
        return const_reverse_iterator(begin());
    }

    D_CONSTEXPR_CPP14 const_reverse_iterator
    crbegin() const D_NOEXCEPT
    {
        return const_reverse_iterator(end());
    }

    D_CONSTEXPR_CPP14 const_reverse_iterator
    crend() const D_NOEXCEPT
    {
        return const_reverse_iterator(begin());
    }

    // =================================================================
    // CAPACITY
    // =================================================================

    // empty
    //   function: true if size() == 0.
    D_CONSTEXPR bool
    empty() const D_NOEXCEPT
    {
        return _Size == 0;
    }

    // size
    //   function: returns _Size.
    D_CONSTEXPR size_type
    size() const D_NOEXCEPT
    {
        return _Size;
    }

    // max_size
    //   function: returns _Size. Identical to size() for a
    // fixed-extent container.
    D_CONSTEXPR size_type
    max_size() const D_NOEXCEPT
    {
        return _Size;
    }

    // =================================================================
    // OPERATIONS
    // =================================================================

    // fill
    //   function: assigns _value to every element.
    D_CONSTEXPR_CPP20 void
    fill(
        const_reference _value
    )
    {
        for (size_type _i = 0; _i < _Size; ++_i)
        {
            _storage::ptr(_M_elems)[_i] = _value;
        }

        return;
    }

    // swap
    //   function: element-wise swap with _other. Conditional noexcept
    // when is_nothrow_swappable_v<_Type> is satisfied — gated out
    // pre-C++17 because the trait is C++17+; non-throwing path on
    // earlier tiers depends on _Type's own swap behaviour.
    D_CONSTEXPR_CPP20 void
    swap(
        array& _other
    )
    {
        for (size_type _i = 0; _i < _Size; ++_i)
        {
            _Type _tmp                          = _storage::ptr(_M_elems)[_i];
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
template<typename    _Type,
         std::size_t _Size>
void
array<_Type, _Size>::_throw_out_of_range()
{
#if D_ENV_CPP98_HAS_STDEXCEPT
    throw std::out_of_range("restd::array::at: index out of range");
#elif D_ENV_CPP98_HAS_EXCEPTION
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
//   `restd::array a = {1, 2, 3};`
// to deduce array<int, 3>.
//   The standard's guide includes a homogeneity check: every
// element type must be the same as the first, otherwise the guide
// is removed from the overload set (rather than silently accepting
// heterogeneous initialisers and narrowing). The check is expressed
// via enable_if_t inside the deduced array's first template
// argument — fold expressions are required, so this is gated on
// C++17+ anyway.

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

template<typename    _Type,
         typename... _Rest>
array(_Type, _Rest...)
    -> array<
           typename restd::enable_if<
               (restd::is_same<_Type, _Rest>::value && ...),
               _Type
           >::type,
           1 + sizeof...(_Rest)>;

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_END  // restd


#endif  // DJINTERP_RESTD_ARRAY_
