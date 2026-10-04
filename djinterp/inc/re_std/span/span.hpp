/***********************************************************************
* re_std                                                        span.hpp
*
* class template span:
*   Non-owning view over a contiguous sequence of objects. Mirrors
* std::span<ElementType, Extent> (C++20), back-ported to C++11. Carries a
* compile-time Extent when known (zero storage overhead beyond the
* pointer) and falls back to a run-time size when Extent is
* dynamic_extent.
*
*   Tiered surface: the full accessor / subview API is available from
* C++11. constexpr coverage widens with the language tier (C++11 single-
* return members are constexpr; reverse-iterator factories become
* constexpr from C++17, matching std::reverse_iterator's constexpr-ness).
* The contiguous-range constructor and the ranges borrowed/view opt-ins
* are deferred pending re_std::ranges (see span umbrella notes).
*
*
* path:      /inc/djinterp/re_std/span/span.hpp
* link(s):   TBA
* author(s): re_std contributors                       date: 2026.06.04
***********************************************************************/

#ifndef DJINTERP_RE_STD_SPAN_SPAN_
#define DJINTERP_RE_STD_SPAN_SPAN_ 1

#include "djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include <cstddef>  // size_t, ptrdiff_t

#include "re_std/type_traits/type_traits.hpp"   // enable_if, is_convertible, remove_cv
#include "re_std/array/array.hpp"               // array (for the array constructors)
#include "re_std/iterator/reverse_iterator.hpp" // reverse_iterator (for rbegin/rend)

#include "re_std/span/dynamic_extent.hpp"

// reverse-iterator factories are constexpr only where the underlying
// reverse_iterator is constexpr-constructible (C++17 in std).
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
#  define D_SPAN_REV_CONSTEXPR D_CONSTEXPR
#else
#  define D_SPAN_REV_CONSTEXPR
#endif

namespace re_std
{

// =============================================================================
// INTERNAL STORAGE
// =============================================================================

namespace internal
{

    // span_storage
    //   struct: holds the pointer and, for the dynamic case, the size.
    //   The fixed-extent primary template stores only the pointer; the
    //   size is the compile-time _Extent. This keeps a fixed-extent span
    //   the size of a single pointer.
    template<typename _Type, std::size_t _Extent>
    struct span_storage
    {
        _Type* m_ptr;

        D_CONSTEXPR
        span_storage() : m_ptr(0)
        {}

        D_CONSTEXPR
        span_storage(_Type* _ptr, std::size_t /*size*/) : m_ptr(_ptr)
        {}

        D_CONSTEXPR std::size_t
        size() const
        { return _Extent; }
    };

    // span_storage<_Type, dynamic_extent>
    //   struct: specialization that additionally tracks a run-time size.
    template<typename _Type>
    struct span_storage<_Type, dynamic_extent>
    {
        _Type*      m_ptr;
        std::size_t m_size;

        D_CONSTEXPR
        span_storage() : m_ptr(0), m_size(0)
        {}

        D_CONSTEXPR
        span_storage(_Type* _ptr, std::size_t _size)
            : m_ptr(_ptr), m_size(_size)
        {}

        D_CONSTEXPR std::size_t
        size() const
        { return m_size; }
    };

}  // namespace internal

// =============================================================================
// span<_Type, _Extent>
// =============================================================================

// span
//   class: non-owning view over _Extent contiguous _Type objects (or a
//   run-time count when _Extent == dynamic_extent).
template<typename _Type, std::size_t _Extent = dynamic_extent>
class span
{
public:

    // member types
    typedef _Type                                  element_type;
    typedef typename remove_cv<_Type>::type        value_type;
    typedef std::size_t                            size_type;
    typedef std::ptrdiff_t                         difference_type;
    typedef _Type*                                 pointer;
    typedef const _Type*                           const_pointer;
    typedef _Type&                                 reference;
    typedef const _Type&                           const_reference;
    typedef _Type*                                 iterator;
    typedef re_std::reverse_iterator<iterator>      reverse_iterator;

    // extent
    //   constant: the compile-time extent (dynamic_extent when run-time).
    static D_CONSTEXPR size_type extent = _Extent;

    // -------------------------------------------------------------------------
    // constructors
    // -------------------------------------------------------------------------

    // span()
    //   function: default ctor. Participates only when a zero-length span
    //   is representable (_Extent == 0 or dynamic_extent), matching std.
    template<std::size_t _E = _Extent,
             typename re_std::enable_if<(_E == dynamic_extent || _E == 0),
                                       int>::type = 0>
    D_CONSTEXPR
    span() noexcept : m_store()
    {}

    // span(pointer, size_type)
    //   function: view of _count elements beginning at _ptr.
    D_CONSTEXPR
    span(pointer _ptr, size_type _count) : m_store(_ptr, _count)
    {}

    // span(pointer, pointer)
    //   function: view of the half-open range [_first, _last).
    D_CONSTEXPR
    span(pointer _first, pointer _last)
        : m_store(_first, static_cast<size_type>(_last - _first))
    {}

    // span(_Type (&)[N])
    //   function: view of a C array. Extent must match when fixed.
    template<std::size_t _N,
             typename re_std::enable_if<(_Extent == dynamic_extent
                                        || _Extent == _N),
                                       int>::type = 0>
    D_CONSTEXPR
    span(element_type (&_arr)[_N]) noexcept : m_store(_arr, _N)
    {}

    // span(array<_U, N>&)
    //   function: view of a re_std::array. _U(*)[] must be convertible to
    //   element_type(*)[] (the qualification-conversion rule std uses).
    template<typename _U, std::size_t _N,
             typename re_std::enable_if<
                 (_Extent == dynamic_extent || _Extent == _N)
                 && re_std::is_convertible<_U (*)[],
                                          element_type (*)[]>::value,
                 int>::type = 0>
    D_CONSTEXPR
    span(array<_U, _N>& _arr) noexcept : m_store(_arr.data(), _N)
    {}

    // span(const array<_U, N>&)
    //   function: const overload of the array ctor.
    template<typename _U, std::size_t _N,
             typename re_std::enable_if<
                 (_Extent == dynamic_extent || _Extent == _N)
                 && re_std::is_convertible<const _U (*)[],
                                          element_type (*)[]>::value,
                 int>::type = 0>
    D_CONSTEXPR
    span(const array<_U, _N>& _arr) noexcept : m_store(_arr.data(), _N)
    {}

    // span(const span<_U, N>&)
    //   function: converting / extent-erasing copy from another span.
    template<typename _U, std::size_t _N,
             typename re_std::enable_if<
                 (_Extent == dynamic_extent || _N == dynamic_extent
                  || _Extent == _N)
                 && re_std::is_convertible<_U (*)[],
                                          element_type (*)[]>::value,
                 int>::type = 0>
    D_CONSTEXPR
    span(const span<_U, _N>& _other) noexcept
        : m_store(_other.data(), _other.size())
    {}

    // copy ctor / copy assignment are the implicit, defaulted versions
    // (trivial: pointer, plus size for the dynamic specialization).

    // -------------------------------------------------------------------------
    // iterators
    // -------------------------------------------------------------------------

    D_CONSTEXPR iterator
    begin() const noexcept
    { return data(); }

    D_CONSTEXPR iterator
    end() const noexcept
    { return data() + size(); }

    D_SPAN_REV_CONSTEXPR reverse_iterator
    rbegin() const noexcept
    { return reverse_iterator(end()); }

    D_SPAN_REV_CONSTEXPR reverse_iterator
    rend() const noexcept
    { return reverse_iterator(begin()); }

    // -------------------------------------------------------------------------
    // element access
    // -------------------------------------------------------------------------

    D_CONSTEXPR reference
    front() const
    { return *data(); }

    D_CONSTEXPR reference
    back() const
    { return *(data() + (size() - 1)); }

    D_CONSTEXPR reference
    operator[](size_type _idx) const
    { return data()[_idx]; }

    D_CONSTEXPR pointer
    data() const noexcept
    { return m_store.m_ptr; }

    // -------------------------------------------------------------------------
    // observers
    // -------------------------------------------------------------------------

    D_CONSTEXPR size_type
    size() const noexcept
    { return m_store.size(); }

    D_CONSTEXPR size_type
    size_bytes() const noexcept
    { return m_store.size() * sizeof(element_type); }

    D_CONSTEXPR bool
    empty() const noexcept
    { return m_store.size() == 0; }

    // -------------------------------------------------------------------------
    // subviews
    // -------------------------------------------------------------------------

    // first<_Count>()
    //   function: fixed-extent view of the first _Count elements.
    template<std::size_t _Count>
    D_CONSTEXPR span<element_type, _Count>
    first() const
    { return span<element_type, _Count>(data(), _Count); }

    // first(count)
    //   function: dynamic-extent view of the first _count elements.
    D_CONSTEXPR span<element_type, dynamic_extent>
    first(size_type _count) const
    { return span<element_type, dynamic_extent>(data(), _count); }

    // last<_Count>()
    //   function: fixed-extent view of the last _Count elements.
    template<std::size_t _Count>
    D_CONSTEXPR span<element_type, _Count>
    last() const
    { return span<element_type, _Count>(data() + (size() - _Count), _Count); }

    // last(count)
    //   function: dynamic-extent view of the last _count elements.
    D_CONSTEXPR span<element_type, dynamic_extent>
    last(size_type _count) const
    { return span<element_type, dynamic_extent>(data() + (size() - _count),
                                                _count); }

    // subspan<_Offset, _Count>()
    //   function: fixed-offset subview. When _Count is dynamic_extent the
    //   resulting extent is (Extent - Offset) for a fixed parent, else
    //   dynamic_extent.
    template<std::size_t _Offset, std::size_t _Count = dynamic_extent>
    D_CONSTEXPR
    span<element_type,
         (_Count != dynamic_extent
              ? _Count
              : (_Extent != dynamic_extent ? _Extent - _Offset
                                           : dynamic_extent))>
    subspan() const
    {
        return span<element_type,
                    (_Count != dynamic_extent
                         ? _Count
                         : (_Extent != dynamic_extent ? _Extent - _Offset
                                                      : dynamic_extent))>(
            data() + _Offset,
            (_Count != dynamic_extent ? _Count : size() - _Offset));
    }

    // subspan(offset, count)
    //   function: dynamic subview from _offset. _count == dynamic_extent
    //   means "to the end".
    D_CONSTEXPR span<element_type, dynamic_extent>
    subspan(size_type _offset,
            size_type _count = dynamic_extent) const
    {
        return span<element_type, dynamic_extent>(
            data() + _offset,
            (_count == dynamic_extent ? size() - _offset : _count));
    }

private:

    internal::span_storage<_Type, _Extent> m_store;
};

// out-of-class definition of the static extent constant. Needed only on
// pre-C++17 tiers, where the in-class initializer is a declaration and a
// separate definition is required if extent is ODR-used (e.g. bound to a
// reference). On C++17+ the member is implicitly inline and a separate
// definition would be a deprecated, redundant redeclaration.
#if !D_ENV_LANG_IS_CPP17_OR_HIGHER
template<typename _Type, std::size_t _Extent>
D_CONSTEXPR typename span<_Type, _Extent>::size_type
    span<_Type, _Extent>::extent;
#endif

// =============================================================================
// DEDUCTION GUIDES (C++17+)
// =============================================================================

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

template<typename _Type, std::size_t _N>
span(_Type (&)[_N]) -> span<_Type, _N>;

template<typename _Type, std::size_t _N>
span(array<_Type, _N>&) -> span<_Type, _N>;

template<typename _Type, std::size_t _N>
span(const array<_Type, _N>&) -> span<const _Type, _N>;

template<typename _Type>
span(_Type*, std::size_t) -> span<_Type>;

template<typename _Type>
span(_Type*, _Type*) -> span<_Type>;

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

}  // namespace re_std

#undef D_SPAN_REV_CONSTEXPR

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_SPAN_SPAN_
