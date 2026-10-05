/*******************************************************************************
* djinterp [re_std]                                        basic_string_view.hpp
*
* non-owning character view header:
*   Provides re_std::basic_string_view<CharT, Traits> — a read-only
* (pointer, length) view over a contiguous character sequence, owning
* nothing. Mirrors the std::basic_string_view interface: the full
* element-access / capacity / iterator surface, the modifiers
* (remove_prefix / remove_suffix / swap), copy / substr / compare, the
* six find-family operations, and the prefix/suffix/substring queries
* (starts_with / ends_with / contains).
*
*   RE_STD AHEAD OF STD:
*   std::basic_string_view landed in C++17; re_std back-ports the C++17
* interface to C++11. starts_with / ends_with (std C++20) and contains
* (std C++23) are likewise available from C++11 in re_std. The whole
* surface becomes constexpr at C++14 (relaxed constexpr — the find /
* compare loops and the mutating modifiers) versus the standard's
* C++17: trivial observers (size, data, operator[], begin/end, front,
* back) are constexpr from C++11.
*
*   DEFERRED (vs the latest standard):
*   The contiguous-iterator / sentinel constructor (std C++20) and the
* range constructor (std C++23) are NOT shipped — they require the
* contiguous_iterator concept and ranges::data / ranges::size, which
* live in <iterator> / <ranges> machinery beyond this module's scope.
* The C++23 deleted basic_string_view(nullptr_t) constructor IS
* provided (back-ported). operator<< (std C++17) is deferred pending
* the iostream subsystem. The C++20 Traits::comparison_category hook is
* not consulted; operator<=> yields strong_ordering directly.
*
*   DEPENDENCIES:
*   ./char_traits.hpp (default traits + the operations used here),
* ../iterator/reverse_iterator.hpp (rbegin / rend), <cstddef> for
* size_t / ptrdiff_t, and <stdexcept> (when available) for the
* out_of_range thrown by at / substr / copy.
*
*
* path:      /inc/re_std/string_view/basic_string_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.04
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_STRING_VIEW_BASIC_STRING_VIEW_HPP
#define RE_STD_STRING_VIEW_BASIC_STRING_VIEW_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER


// re_std
#include "./char_traits.hpp"
#include "../iterator/reverse_iterator.hpp"

// std (fundamental types only)
// std
#include <cstddef>

#if defined(RE_STD_HAS_EXCEPTIONS) && RE_STD_HAS_EXCEPTIONS
    // std
    #include <stdexcept>
#else
    // std
    #include <cstdlib>
#endif


namespace re_std
{


// =============================================================================
// I.   INTERNAL: ERROR PATH
// =============================================================================

namespace internal
{

    // sv_throw_out_of_range
    //   function: raise the out-of-bounds error for at / substr / copy.
    // Throws std::out_of_range when <stdexcept> is available; otherwise
    // aborts (the no-exception fallback).
    inline void
    sv_throw_out_of_range(
        const char*  _msg
    )
    {
    #if defined(RE_STD_HAS_EXCEPTIONS) && RE_STD_HAS_EXCEPTIONS
        throw std::out_of_range(_msg);
    #else
        (void) _msg;
        std::abort();
    #endif
    }

}  // internal


// =============================================================================
// II.  BASIC_STRING_VIEW
// =============================================================================

// basic_string_view
//   class: read-only view over a contiguous sequence of CharT, with
// character operations supplied by Traits. Holds only a pointer and a
// length; copying a view is cheap and never touches the underlying
// storage.
template<typename CharT,
         typename Traits = char_traits<CharT> >
class basic_string_view
{
public:
    // member types
    typedef Traits                                  traits_type;
    typedef CharT                                   value_type;
    typedef CharT*                                  pointer;
    typedef const CharT*                            const_pointer;
    typedef CharT&                                  reference;
    typedef const CharT&                            const_reference;
    typedef const CharT*                            const_iterator;
    typedef const_iterator                           iterator;
    typedef re_std::reverse_iterator<const_iterator>  const_reverse_iterator;
    typedef const_reverse_iterator                   reverse_iterator;
    typedef std::size_t                              size_type;
    typedef std::ptrdiff_t                           difference_type;

    // npos
    //   constant: returned by the search operations to signal "not
    // found"; the largest representable size_type.
    static RE_STD_CONSTEXPR size_type npos = static_cast<size_type>(-1);

    // -------------------------------------------------------------------------
    // construction
    // -------------------------------------------------------------------------

    // basic_string_view()
    //   function: an empty view (null data, zero length).
    RE_STD_CONSTEXPR
    basic_string_view() RE_STD_NOEXCEPT
        : m_data(0)
        , m_size(0)
    {}

    // basic_string_view(const basic_string_view&)
    //   function: copy — defaulted, trivial.
    RE_STD_CONSTEXPR
    basic_string_view(
        const basic_string_view&  _other
    ) RE_STD_NOEXCEPT = default;

    // basic_string_view(const CharT*, size_type)
    //   function: view the _count characters beginning at _s.
    RE_STD_CONSTEXPR
    basic_string_view(
        const CharT*  _s,
        size_type      _count
    )
        : m_data(_s)
        , m_size(_count)
    {}

    // basic_string_view(const CharT*)
    //   function: view a null-terminated string; length via
    // traits_type::length.
    RE_STD_CONSTEXPR_CPP14
    basic_string_view(
        const CharT*  _s
    )
        : m_data(_s)
        , m_size(traits_type::length(_s))
    {}

    // basic_string_view(nullptr_t) = delete
    //   function: back-port of the C++23 deletion — forbids
    // constructing a view from a null pointer literal.
    basic_string_view(
        decltype(nullptr)
    ) = delete;

    // operator=
    //   function: copy assignment — defaulted, trivial.
    RE_STD_CONSTEXPR_CPP14 basic_string_view&
    operator=(
        const basic_string_view&  _other
    ) RE_STD_NOEXCEPT = default;

    // -------------------------------------------------------------------------
    // iterators
    // -------------------------------------------------------------------------

    RE_STD_CONSTEXPR const_iterator begin()  const RE_STD_NOEXCEPT { return m_data; }
    RE_STD_CONSTEXPR const_iterator end()    const RE_STD_NOEXCEPT { return m_data + m_size; }
    RE_STD_CONSTEXPR const_iterator cbegin() const RE_STD_NOEXCEPT { return m_data; }
    RE_STD_CONSTEXPR const_iterator cend()   const RE_STD_NOEXCEPT { return m_data + m_size; }

    RE_STD_CONSTEXPR_CPP14 const_reverse_iterator
    rbegin() const RE_STD_NOEXCEPT
    {
        return const_reverse_iterator(end());
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

    // -------------------------------------------------------------------------
    // capacity
    // -------------------------------------------------------------------------

    RE_STD_CONSTEXPR size_type size()   const RE_STD_NOEXCEPT { return m_size; }
    RE_STD_CONSTEXPR size_type length() const RE_STD_NOEXCEPT { return m_size; }
    RE_STD_CONSTEXPR bool      empty()  const RE_STD_NOEXCEPT { return m_size == 0; }

    RE_STD_CONSTEXPR size_type
    max_size() const RE_STD_NOEXCEPT
    {
        return static_cast<size_type>(-1) / sizeof(CharT);
    }

    // -------------------------------------------------------------------------
    // element access
    // -------------------------------------------------------------------------

    RE_STD_CONSTEXPR const_reference
    operator[](
        size_type  _pos
    ) const
    {
        return m_data[_pos];
    }

    RE_STD_CONSTEXPR_CPP14 const_reference
    at(
        size_type  _pos
    ) const
    {
        if (_pos >= m_size)
        {
            internal::sv_throw_out_of_range("re_std::basic_string_view::at");
        }
        return m_data[_pos];
    }

    RE_STD_CONSTEXPR const_reference front() const { return m_data[0]; }
    RE_STD_CONSTEXPR const_reference back()  const { return m_data[m_size - 1]; }
    RE_STD_CONSTEXPR const_pointer   data()  const RE_STD_NOEXCEPT { return m_data; }

    // -------------------------------------------------------------------------
    // modifiers
    // -------------------------------------------------------------------------

    RE_STD_CONSTEXPR_CPP14 void
    remove_prefix(
        size_type  _n
    )
    {
        m_data += _n;
        m_size -= _n;
        return;
    }

    RE_STD_CONSTEXPR_CPP14 void
    remove_suffix(
        size_type  _n
    )
    {
        m_size -= _n;
        return;
    }

    RE_STD_CONSTEXPR_CPP14 void
    swap(
        basic_string_view&  _other
    ) RE_STD_NOEXCEPT
    {
        const_pointer  _td = m_data;
        size_type      _ts = m_size;
        m_data = _other.m_data;
        m_size = _other.m_size;
        _other.m_data = _td;
        _other.m_size = _ts;
        return;
    }

    // -------------------------------------------------------------------------
    // operations
    // -------------------------------------------------------------------------

    // copy
    //   function: copy at most _count characters starting at _pos into
    // the caller's buffer _dst. Returns the number copied.
    RE_STD_CONSTEXPR_CPP14 size_type
    copy(
        CharT*    _dst,
        size_type  _count,
        size_type  _pos = 0
    ) const
    {
        if (_pos > m_size)
        {
            internal::sv_throw_out_of_range("re_std::basic_string_view::copy");
        }
        size_type _rlen = m_size - _pos;
        if (_count < _rlen)
        {
            _rlen = _count;
        }
        traits_type::copy(_dst, m_data + _pos, _rlen);
        return _rlen;
    }

    // substr
    //   function: a view of at most _count characters starting at _pos.
    RE_STD_CONSTEXPR_CPP14 basic_string_view
    substr(
        size_type  _pos   = 0,
        size_type  _count = npos
    ) const
    {
        if (_pos > m_size)
        {
            internal::sv_throw_out_of_range("re_std::basic_string_view::substr");
        }
        size_type _rlen = m_size - _pos;
        if (_count < _rlen)
        {
            _rlen = _count;
        }
        return basic_string_view(m_data + _pos, _rlen);
    }

    // compare
    //   function: lexicographic three-way comparison against _other.
    RE_STD_CONSTEXPR_CPP14 int
    compare(
        basic_string_view  _other
    ) const RE_STD_NOEXCEPT
    {
        size_type _rlen = m_size < _other.m_size ? m_size : _other.m_size;
        int _r = traits_type::compare(m_data, _other.m_data, _rlen);
        if (_r != 0)
        {
            return _r;
        }
        if (m_size < _other.m_size) { return -1; }
        if (m_size > _other.m_size) { return  1; }
        return 0;
    }

    RE_STD_CONSTEXPR_CPP14 int
    compare(
        size_type          _pos1,
        size_type          _count1,
        basic_string_view  _other
    ) const
    {
        return substr(_pos1, _count1).compare(_other);
    }

    RE_STD_CONSTEXPR_CPP14 int
    compare(
        size_type          _pos1,
        size_type          _count1,
        basic_string_view  _other,
        size_type          _pos2,
        size_type          _count2
    ) const
    {
        return substr(_pos1, _count1).compare(_other.substr(_pos2, _count2));
    }

    RE_STD_CONSTEXPR_CPP14 int
    compare(
        const CharT*  _s
    ) const
    {
        return compare(basic_string_view(_s));
    }

    RE_STD_CONSTEXPR_CPP14 int
    compare(
        size_type      _pos1,
        size_type      _count1,
        const CharT*  _s
    ) const
    {
        return substr(_pos1, _count1).compare(basic_string_view(_s));
    }

    RE_STD_CONSTEXPR_CPP14 int
    compare(
        size_type      _pos1,
        size_type      _count1,
        const CharT*  _s,
        size_type      _count2
    ) const
    {
        return substr(_pos1, _count1).compare(basic_string_view(_s, _count2));
    }

    // starts_with  (back-port of std C++20)
    RE_STD_CONSTEXPR_CPP14 bool
    starts_with(
        basic_string_view  _x
    ) const RE_STD_NOEXCEPT
    {
        return m_size >= _x.m_size
               && traits_type::compare(m_data, _x.m_data, _x.m_size) == 0;
    }

    RE_STD_CONSTEXPR bool
    starts_with(
        CharT  _c
    ) const RE_STD_NOEXCEPT
    {
        return m_size != 0 && traits_type::eq(m_data[0], _c);
    }

    RE_STD_CONSTEXPR_CPP14 bool
    starts_with(
        const CharT*  _s
    ) const
    {
        return starts_with(basic_string_view(_s));
    }

    // ends_with  (back-port of std C++20)
    RE_STD_CONSTEXPR_CPP14 bool
    ends_with(
        basic_string_view  _x
    ) const RE_STD_NOEXCEPT
    {
        return m_size >= _x.m_size
               && traits_type::compare(
                      m_data + (m_size - _x.m_size), _x.m_data, _x.m_size) == 0;
    }

    RE_STD_CONSTEXPR bool
    ends_with(
        CharT  _c
    ) const RE_STD_NOEXCEPT
    {
        return m_size != 0 && traits_type::eq(m_data[m_size - 1], _c);
    }

    RE_STD_CONSTEXPR_CPP14 bool
    ends_with(
        const CharT*  _s
    ) const
    {
        return ends_with(basic_string_view(_s));
    }

    // contains  (back-port of std C++23)
    RE_STD_CONSTEXPR_CPP14 bool
    contains(
        basic_string_view  _x
    ) const RE_STD_NOEXCEPT
    {
        return find(_x) != npos;
    }

    RE_STD_CONSTEXPR_CPP14 bool
    contains(
        CharT  _c
    ) const RE_STD_NOEXCEPT
    {
        return find(_c) != npos;
    }

    RE_STD_CONSTEXPR_CPP14 bool
    contains(
        const CharT*  _s
    ) const
    {
        return find(_s) != npos;
    }

    // -------------------------------------------------------------------------
    // search — find
    // -------------------------------------------------------------------------

    RE_STD_CONSTEXPR_CPP14 size_type
    find(
        basic_string_view  _x,
        size_type          _pos = 0
    ) const RE_STD_NOEXCEPT
    {
        if (_x.m_size == 0)
        {
            return _pos <= m_size ? _pos : npos;
        }
        if (_pos >= m_size || _x.m_size > m_size - _pos)
        {
            return npos;
        }
        const size_type _last = m_size - _x.m_size;
        for (size_type _i = _pos; _i <= _last; ++_i)
        {
            if (traits_type::compare(m_data + _i, _x.m_data, _x.m_size) == 0)
            {
                return _i;
            }
        }
        return npos;
    }

    RE_STD_CONSTEXPR_CPP14 size_type
    find(
        CharT     _c,
        size_type  _pos = 0
    ) const RE_STD_NOEXCEPT
    {
        for (size_type _i = _pos; _i < m_size; ++_i)
        {
            if (traits_type::eq(m_data[_i], _c))
            {
                return _i;
            }
        }
        return npos;
    }

    RE_STD_CONSTEXPR_CPP14 size_type
    find(
        const CharT*  _s,
        size_type      _pos,
        size_type      _count
    ) const
    {
        return find(basic_string_view(_s, _count), _pos);
    }

    RE_STD_CONSTEXPR_CPP14 size_type
    find(
        const CharT*  _s,
        size_type      _pos = 0
    ) const
    {
        return find(basic_string_view(_s), _pos);
    }

    // -------------------------------------------------------------------------
    // search — rfind
    // -------------------------------------------------------------------------

    RE_STD_CONSTEXPR_CPP14 size_type
    rfind(
        basic_string_view  _x,
        size_type          _pos = npos
    ) const RE_STD_NOEXCEPT
    {
        if (_x.m_size > m_size)
        {
            return npos;
        }
        size_type _start = m_size - _x.m_size;
        if (_start > _pos)
        {
            _start = _pos;
        }
        for (size_type _i = _start + 1; _i != 0; --_i)
        {
            if (traits_type::compare(m_data + (_i - 1), _x.m_data, _x.m_size) == 0)
            {
                return _i - 1;
            }
        }
        return npos;
    }

    RE_STD_CONSTEXPR_CPP14 size_type
    rfind(
        CharT     _c,
        size_type  _pos = npos
    ) const RE_STD_NOEXCEPT
    {
        if (m_size == 0)
        {
            return npos;
        }
        size_type _start = m_size - 1;
        if (_start > _pos)
        {
            _start = _pos;
        }
        for (size_type _i = _start + 1; _i != 0; --_i)
        {
            if (traits_type::eq(m_data[_i - 1], _c))
            {
                return _i - 1;
            }
        }
        return npos;
    }

    RE_STD_CONSTEXPR_CPP14 size_type
    rfind(
        const CharT*  _s,
        size_type      _pos,
        size_type      _count
    ) const
    {
        return rfind(basic_string_view(_s, _count), _pos);
    }

    RE_STD_CONSTEXPR_CPP14 size_type
    rfind(
        const CharT*  _s,
        size_type      _pos = npos
    ) const
    {
        return rfind(basic_string_view(_s), _pos);
    }

    // -------------------------------------------------------------------------
    // search — find_first_of
    // -------------------------------------------------------------------------

    RE_STD_CONSTEXPR_CPP14 size_type
    find_first_of(
        basic_string_view  _x,
        size_type          _pos = 0
    ) const RE_STD_NOEXCEPT
    {
        for (size_type _i = _pos; _i < m_size; ++_i)
        {
            if (traits_type::find(_x.m_data, _x.m_size, m_data[_i]) != 0)
            {
                return _i;
            }
        }
        return npos;
    }

    RE_STD_CONSTEXPR_CPP14 size_type
    find_first_of(
        CharT     _c,
        size_type  _pos = 0
    ) const RE_STD_NOEXCEPT
    {
        return find(_c, _pos);
    }

    RE_STD_CONSTEXPR_CPP14 size_type
    find_first_of(
        const CharT*  _s,
        size_type      _pos,
        size_type      _count
    ) const
    {
        return find_first_of(basic_string_view(_s, _count), _pos);
    }

    RE_STD_CONSTEXPR_CPP14 size_type
    find_first_of(
        const CharT*  _s,
        size_type      _pos = 0
    ) const
    {
        return find_first_of(basic_string_view(_s), _pos);
    }

    // -------------------------------------------------------------------------
    // search — find_last_of
    // -------------------------------------------------------------------------

    RE_STD_CONSTEXPR_CPP14 size_type
    find_last_of(
        basic_string_view  _x,
        size_type          _pos = npos
    ) const RE_STD_NOEXCEPT
    {
        if (m_size == 0)
        {
            return npos;
        }
        size_type _start = m_size - 1;
        if (_start > _pos)
        {
            _start = _pos;
        }
        for (size_type _i = _start + 1; _i != 0; --_i)
        {
            if (traits_type::find(_x.m_data, _x.m_size, m_data[_i - 1]) != 0)
            {
                return _i - 1;
            }
        }
        return npos;
    }

    RE_STD_CONSTEXPR_CPP14 size_type
    find_last_of(
        CharT     _c,
        size_type  _pos = npos
    ) const RE_STD_NOEXCEPT
    {
        return rfind(_c, _pos);
    }

    RE_STD_CONSTEXPR_CPP14 size_type
    find_last_of(
        const CharT*  _s,
        size_type      _pos,
        size_type      _count
    ) const
    {
        return find_last_of(basic_string_view(_s, _count), _pos);
    }

    RE_STD_CONSTEXPR_CPP14 size_type
    find_last_of(
        const CharT*  _s,
        size_type      _pos = npos
    ) const
    {
        return find_last_of(basic_string_view(_s), _pos);
    }

    // -------------------------------------------------------------------------
    // search — find_first_not_of
    // -------------------------------------------------------------------------

    RE_STD_CONSTEXPR_CPP14 size_type
    find_first_not_of(
        basic_string_view  _x,
        size_type          _pos = 0
    ) const RE_STD_NOEXCEPT
    {
        for (size_type _i = _pos; _i < m_size; ++_i)
        {
            if (traits_type::find(_x.m_data, _x.m_size, m_data[_i]) == 0)
            {
                return _i;
            }
        }
        return npos;
    }

    RE_STD_CONSTEXPR_CPP14 size_type
    find_first_not_of(
        CharT     _c,
        size_type  _pos = 0
    ) const RE_STD_NOEXCEPT
    {
        for (size_type _i = _pos; _i < m_size; ++_i)
        {
            if (!traits_type::eq(m_data[_i], _c))
            {
                return _i;
            }
        }
        return npos;
    }

    RE_STD_CONSTEXPR_CPP14 size_type
    find_first_not_of(
        const CharT*  _s,
        size_type      _pos,
        size_type      _count
    ) const
    {
        return find_first_not_of(basic_string_view(_s, _count), _pos);
    }

    RE_STD_CONSTEXPR_CPP14 size_type
    find_first_not_of(
        const CharT*  _s,
        size_type      _pos = 0
    ) const
    {
        return find_first_not_of(basic_string_view(_s), _pos);
    }

    // -------------------------------------------------------------------------
    // search — find_last_not_of
    // -------------------------------------------------------------------------

    RE_STD_CONSTEXPR_CPP14 size_type
    find_last_not_of(
        basic_string_view  _x,
        size_type          _pos = npos
    ) const RE_STD_NOEXCEPT
    {
        if (m_size == 0)
        {
            return npos;
        }
        size_type _start = m_size - 1;
        if (_start > _pos)
        {
            _start = _pos;
        }
        for (size_type _i = _start + 1; _i != 0; --_i)
        {
            if (traits_type::find(_x.m_data, _x.m_size, m_data[_i - 1]) == 0)
            {
                return _i - 1;
            }
        }
        return npos;
    }

    RE_STD_CONSTEXPR_CPP14 size_type
    find_last_not_of(
        CharT     _c,
        size_type  _pos = npos
    ) const RE_STD_NOEXCEPT
    {
        if (m_size == 0)
        {
            return npos;
        }
        size_type _start = m_size - 1;
        if (_start > _pos)
        {
            _start = _pos;
        }
        for (size_type _i = _start + 1; _i != 0; --_i)
        {
            if (!traits_type::eq(m_data[_i - 1], _c))
            {
                return _i - 1;
            }
        }
        return npos;
    }

    RE_STD_CONSTEXPR_CPP14 size_type
    find_last_not_of(
        const CharT*  _s,
        size_type      _pos,
        size_type      _count
    ) const
    {
        return find_last_not_of(basic_string_view(_s, _count), _pos);
    }

    RE_STD_CONSTEXPR_CPP14 size_type
    find_last_not_of(
        const CharT*  _s,
        size_type      _pos = npos
    ) const
    {
        return find_last_not_of(basic_string_view(_s), _pos);
    }

private:
    const_pointer  m_data;
    size_type      m_size;
};


// Out-of-class definition of npos for pre-C++17 (where a constexpr
// static data member is not implicitly inline and may be odr-used).
#if !RE_STD_LANG_IS_CPP17_OR_HIGHER
template<typename CharT,
         typename Traits>
RE_STD_CONSTEXPR typename basic_string_view<CharT, Traits>::size_type
basic_string_view<CharT, Traits>::npos;
#endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_STRING_VIEW_BASIC_STRING_VIEW_HPP
