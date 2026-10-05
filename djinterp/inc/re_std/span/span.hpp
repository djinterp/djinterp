/*******************************************************************************
* djinterp [re_std]                                                     span.hpp
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
* path:      /inc/re_std/span/span.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.06.04
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_SPAN_SPAN_HPP
#define RE_STD_SPAN_SPAN_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>  // size_t, ptrdiff_t

#include "re_std/type_traits/type_traits.hpp"            // enable_if, is_convertible, remove_cv
#include "re_std/array/array.hpp"                        // array (for the array constructors)
#include "re_std/iterator/reverse_iterator.hpp"          // reverse_iterator (for rbegin/rend)

#include "re_std/span/dynamic_extent.hpp"

// reverse-iterator factories are constexpr only where the underlying
// reverse_iterator is constexpr-constructible (C++17 in std).
#if RE_STD_LANG_IS_CPP17_OR_HIGHER
#  define RE_STD_INTERNAL_SPAN_REV_CONSTEXPR RE_STD_CONSTEXPR
#else
#  define RE_STD_INTERNAL_SPAN_REV_CONSTEXPR
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
    //   size is the compile-time Extent. This keeps a fixed-extent span
    //   the size of a single pointer.
    template<typename Type, std::size_t Extent>
    struct span_storage
    {
        Type* m_ptr;

        RE_STD_CONSTEXPR
        span_storage() : m_ptr(0)
        {}

        RE_STD_CONSTEXPR
        span_storage(Type* _ptr, std::size_t /*size*/) : m_ptr(_ptr)
        {}

        RE_STD_CONSTEXPR std::size_t
        size() const
        { return Extent; }
    };

    // span_storage<Type, dynamic_extent>
    //   struct: specialization that additionally tracks a run-time size.
    template<typename Type>
    struct span_storage<Type, dynamic_extent>
    {
        Type*      m_ptr;
        std::size_t m_size;

        RE_STD_CONSTEXPR
        span_storage() : m_ptr(0), m_size(0)
        {}

        RE_STD_CONSTEXPR
        span_storage(Type* _ptr, std::size_t _size)
            : m_ptr(_ptr), m_size(_size)
        {}

        RE_STD_CONSTEXPR std::size_t
        size() const
        { return m_size; }
    };

}  // internal
// =============================================================================
// span<Type, Extent>
// =============================================================================

// span
//   class: non-owning view over Extent contiguous Type objects (or a
//   run-time count when Extent == dynamic_extent).
template<typename Type, std::size_t Extent = dynamic_extent>
class span
{
public:

    // member types
    typedef Type                                  element_type;
    typedef typename remove_cv<Type>::type        value_type;
    typedef std::size_t                            size_type;
    typedef std::ptrdiff_t                         difference_type;
    typedef Type*                                 pointer;
    typedef const Type*                           const_pointer;
    typedef Type&                                 reference;
    typedef const Type&                           const_reference;
    typedef Type*                                 iterator;
    typedef re_std::reverse_iterator<iterator>      reverse_iterator;

    // extent
    //   constant: the compile-time extent (dynamic_extent when run-time).
    static RE_STD_CONSTEXPR size_type extent = Extent;

    // -------------------------------------------------------------------------
    // constructors
    // -------------------------------------------------------------------------

    // span()
    //   function: default ctor. Participates only when a zero-length span
    //   is representable (Extent == 0 or dynamic_extent), matching std.
    template<std::size_t E = Extent,
             typename re_std::enable_if<(E == dynamic_extent || E == 0),
                                       int>::type = 0>
    RE_STD_CONSTEXPR
    span() noexcept : m_store()
    {}

    // span(pointer, size_type)
    //   function: view of _count elements beginning at _ptr.
    RE_STD_CONSTEXPR
    span(pointer _ptr, size_type _count) : m_store(_ptr, _count)
    {}

    // span(pointer, pointer)
    //   function: view of the half-open range [_first, _last).
    RE_STD_CONSTEXPR
    span(pointer _first, pointer _last)
        : m_store(_first, static_cast<size_type>(_last - _first))
    {}

    // span(Type (&)[N])
    //   function: view of a C array. Extent must match when fixed.
    template<std::size_t N,
             typename re_std::enable_if<(Extent == dynamic_extent
                                        || Extent == N),
                                       int>::type = 0>
    RE_STD_CONSTEXPR
    span(element_type (&_arr)[N]) noexcept : m_store(_arr, N)
    {}

    // span(array<U, N>&)
    //   function: view of a re_std::array. U(*)[] must be convertible to
    //   element_type(*)[] (the qualification-conversion rule std uses).
    template<typename U, std::size_t N,
             typename re_std::enable_if<
                 (Extent == dynamic_extent || Extent == N)
                 && re_std::is_convertible<U (*)[],
                                          element_type (*)[]>::value,
                 int>::type = 0>
    RE_STD_CONSTEXPR
    span(array<U, N>& _arr) noexcept : m_store(_arr.data(), N)
    {}

    // span(const array<U, N>&)
    //   function: const overload of the array ctor.
    template<typename U, std::size_t N,
             typename re_std::enable_if<
                 (Extent == dynamic_extent || Extent == N)
                 && re_std::is_convertible<const U (*)[],
                                          element_type (*)[]>::value,
                 int>::type = 0>
    RE_STD_CONSTEXPR
    span(const array<U, N>& _arr) noexcept : m_store(_arr.data(), N)
    {}

    // span(const span<U, N>&)
    //   function: converting / extent-erasing copy from another span.
    template<typename U, std::size_t N,
             typename re_std::enable_if<
                 (Extent == dynamic_extent || N == dynamic_extent
                  || Extent == N)
                 && re_std::is_convertible<U (*)[],
                                          element_type (*)[]>::value,
                 int>::type = 0>
    RE_STD_CONSTEXPR
    span(const span<U, N>& _other) noexcept
        : m_store(_other.data(), _other.size())
    {}

    // copy ctor / copy assignment are the implicit, defaulted versions
    // (trivial: pointer, plus size for the dynamic specialization).

    // -------------------------------------------------------------------------
    // iterators
    // -------------------------------------------------------------------------

    RE_STD_CONSTEXPR iterator
    begin() const noexcept
    { return data(); }

    RE_STD_CONSTEXPR iterator
    end() const noexcept
    { return data() + size(); }

    RE_STD_INTERNAL_SPAN_REV_CONSTEXPR reverse_iterator
    rbegin() const noexcept
    { return reverse_iterator(end()); }

    RE_STD_INTERNAL_SPAN_REV_CONSTEXPR reverse_iterator
    rend() const noexcept
    { return reverse_iterator(begin()); }

    // -------------------------------------------------------------------------
    // element access
    // -------------------------------------------------------------------------

    RE_STD_CONSTEXPR reference
    front() const
    { return *data(); }

    RE_STD_CONSTEXPR reference
    back() const
    { return *(data() + (size() - 1)); }

    RE_STD_CONSTEXPR reference
    operator[](size_type _idx) const
    { return data()[_idx]; }

    RE_STD_CONSTEXPR pointer
    data() const noexcept
    { return m_store.m_ptr; }

    // -------------------------------------------------------------------------
    // observers
    // -------------------------------------------------------------------------

    RE_STD_CONSTEXPR size_type
    size() const noexcept
    { return m_store.size(); }

    RE_STD_CONSTEXPR size_type
    size_bytes() const noexcept
    { return m_store.size() * sizeof(element_type); }

    RE_STD_CONSTEXPR bool
    empty() const noexcept
    { return m_store.size() == 0; }

    // -------------------------------------------------------------------------
    // subviews
    // -------------------------------------------------------------------------

    // first<Count>()
    //   function: fixed-extent view of the first Count elements.
    template<std::size_t Count>
    RE_STD_CONSTEXPR span<element_type, Count>
    first() const
    { return span<element_type, Count>(data(), Count); }

    // first(count)
    //   function: dynamic-extent view of the first _count elements.
    RE_STD_CONSTEXPR span<element_type, dynamic_extent>
    first(size_type _count) const
    { return span<element_type, dynamic_extent>(data(), _count); }

    // last<Count>()
    //   function: fixed-extent view of the last Count elements.
    template<std::size_t Count>
    RE_STD_CONSTEXPR span<element_type, Count>
    last() const
    { return span<element_type, Count>(data() + (size() - Count), Count); }

    // last(count)
    //   function: dynamic-extent view of the last _count elements.
    RE_STD_CONSTEXPR span<element_type, dynamic_extent>
    last(size_type _count) const
    { return span<element_type, dynamic_extent>(data() + (size() - _count),
                                                _count); }

    // subspan<Offset, Count>()
    //   function: fixed-offset subview. When Count is dynamic_extent the
    //   resulting extent is (Extent - Offset) for a fixed parent, else
    //   dynamic_extent.
    template<std::size_t Offset, std::size_t Count = dynamic_extent>
    RE_STD_CONSTEXPR
    span<element_type,
         (Count != dynamic_extent
              ? Count
              : (Extent != dynamic_extent ? Extent - Offset
                                           : dynamic_extent))>
    subspan() const
    {
        return span<element_type,
                    (Count != dynamic_extent
                         ? Count
                         : (Extent != dynamic_extent ? Extent - Offset
                                                      : dynamic_extent))>(
            data() + Offset,
            (Count != dynamic_extent ? Count : size() - Offset));
    }

    // subspan(offset, count)
    //   function: dynamic subview from _offset. _count == dynamic_extent
    //   means "to the end".
    RE_STD_CONSTEXPR span<element_type, dynamic_extent>
    subspan(size_type _offset,
            size_type _count = dynamic_extent) const
    {
        return span<element_type, dynamic_extent>(
            data() + _offset,
            (_count == dynamic_extent ? size() - _offset : _count));
    }

private:

    internal::span_storage<Type, Extent> m_store;
};

// out-of-class definition of the static extent constant. Needed only on
// pre-C++17 tiers, where the in-class initializer is a declaration and a
// separate definition is required if extent is ODR-used (e.g. bound to a
// reference). On C++17+ the member is implicitly inline and a separate
// definition would be a deprecated, redundant redeclaration.
#if !RE_STD_LANG_IS_CPP17_OR_HIGHER
template<typename Type, std::size_t Extent>
RE_STD_CONSTEXPR typename span<Type, Extent>::size_type
    span<Type, Extent>::extent;
#endif

// =============================================================================
// DEDUCTION GUIDES (C++17+)
// =============================================================================

#if RE_STD_LANG_IS_CPP17_OR_HIGHER

template<typename Type, std::size_t N>
span(Type (&)[N]) -> span<Type, N>;

template<typename Type, std::size_t N>
span(array<Type, N>&) -> span<Type, N>;

template<typename Type, std::size_t N>
span(const array<Type, N>&) -> span<const Type, N>;

template<typename Type>
span(Type*, std::size_t) -> span<Type>;

template<typename Type>
span(Type*, Type*) -> span<Type>;

#endif  // RE_STD_LANG_IS_CPP17_OR_HIGHER

}  // re_std
#undef RE_STD_INTERNAL_SPAN_REV_CONSTEXPR

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_SPAN_SPAN_HPP
