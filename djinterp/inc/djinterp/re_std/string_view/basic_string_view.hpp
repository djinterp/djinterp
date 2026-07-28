/******************************************************************************
* djinterp [restd]                                       basic_string_view.hpp
*
* non-owning character view header:
*   Provides restd::basic_string_view<_CharT, _Traits> — a read-only
* (pointer, length) view over a contiguous character sequence, owning
* nothing. Mirrors the std::basic_string_view interface: the full
* element-access / capacity / iterator surface, the modifiers
* (remove_prefix / remove_suffix / swap), copy / substr / compare, the
* six find-family operations, and the prefix/suffix/substring queries
* (starts_with / ends_with / contains).
*
*   RESTD AHEAD OF STD:
*   std::basic_string_view landed in C++17; restd back-ports the C++17
* interface to C++11. starts_with / ends_with (std C++20) and contains
* (std C++23) are likewise available from C++11 in restd. The whole
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
* path:      /inc/djinterp/restd/string_view/basic_string_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.04
******************************************************************************/

#ifndef DJINTERP_RESTD_STRING_VIEW_BASIC_STRING_VIEW_
#define DJINTERP_RESTD_STRING_VIEW_BASIC_STRING_VIEW_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


// djinterp
#include "./char_traits.hpp"
#include "../iterator/reverse_iterator.hpp"

// std (fundamental types only)
#include <cstddef>

#if defined(D_ENV_CPP98_HAS_STDEXCEPT) && D_ENV_CPP98_HAS_STDEXCEPT
    #include <stdexcept>
#else
    #include <cstdlib>
#endif


// D_CONSTEXPR_CPP14
//   macro: constexpr on C++14+ (relaxed constexpr — locals, loops),
// empty otherwise. Locally defined pending the global qualifier-macro-
// table entry (see roadmap meta note).
#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14 constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


NS_RESTD


// =============================================================================
// I.   INTERNAL: ERROR PATH
// =============================================================================

NS_INTERNAL

    // sv_throw_out_of_range
    //   function: raise the out-of-bounds error for at / substr / copy.
    // Throws std::out_of_range when <stdexcept> is available; otherwise
    // aborts (the no-exception fallback).
    inline void
    sv_throw_out_of_range(
        const char*  _msg
    )
    {
    #if defined(D_ENV_CPP98_HAS_STDEXCEPT) && D_ENV_CPP98_HAS_STDEXCEPT
        throw std::out_of_range(_msg);
    #else
        (void) _msg;
        std::abort();
    #endif
    }

NS_END  // internal


// =============================================================================
// II.  BASIC_STRING_VIEW
// =============================================================================

// basic_string_view
//   class: read-only view over a contiguous sequence of _CharT, with
// character operations supplied by _Traits. Holds only a pointer and a
// length; copying a view is cheap and never touches the underlying
// storage.
template<typename _CharT,
         typename _Traits = char_traits<_CharT> >
class basic_string_view
{
public:
    // member types
    typedef _Traits                                  traits_type;
    typedef _CharT                                   value_type;
    typedef _CharT*                                  pointer;
    typedef const _CharT*                            const_pointer;
    typedef _CharT&                                  reference;
    typedef const _CharT&                            const_reference;
    typedef const _CharT*                            const_iterator;
    typedef const_iterator                           iterator;
    typedef restd::reverse_iterator<const_iterator>  const_reverse_iterator;
    typedef const_reverse_iterator                   reverse_iterator;
    typedef std::size_t                              size_type;
    typedef std::ptrdiff_t                           difference_type;

    // npos
    //   constant: returned by the search operations to signal "not
    // found"; the largest representable size_type.
    static D_CONSTEXPR size_type npos = static_cast<size_type>(-1);

    // -------------------------------------------------------------------------
    // construction
    // -------------------------------------------------------------------------

    // basic_string_view()
    //   function: an empty view (null data, zero length).
    D_CONSTEXPR
    basic_string_view() D_NOEXCEPT
        : m_data(0)
        , m_size(0)
    {}

    // basic_string_view(const basic_string_view&)
    //   function: copy — defaulted, trivial.
    D_CONSTEXPR
    basic_string_view(
        const basic_string_view&  _other
    ) D_NOEXCEPT = default;

    // basic_string_view(const _CharT*, size_type)
    //   function: view the _count characters beginning at _s.
    D_CONSTEXPR
    basic_string_view(
        const _CharT*  _s,
        size_type      _count
    )
        : m_data(_s)
        , m_size(_count)
    {}

    // basic_string_view(const _CharT*)
    //   function: view a null-terminated string; length via
    // traits_type::length.
    D_CONSTEXPR_CPP14
    basic_string_view(
        const _CharT*  _s
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
    D_CONSTEXPR_CPP14 basic_string_view&
    operator=(
        const basic_string_view&  _other
    ) D_NOEXCEPT = default;

    // -------------------------------------------------------------------------
    // iterators
    // -------------------------------------------------------------------------

    D_CONSTEXPR const_iterator begin()  const D_NOEXCEPT { return m_data; }
    D_CONSTEXPR const_iterator end()    const D_NOEXCEPT { return m_data + m_size; }
    D_CONSTEXPR const_iterator cbegin() const D_NOEXCEPT { return m_data; }
    D_CONSTEXPR const_iterator cend()   const D_NOEXCEPT { return m_data + m_size; }

    D_CONSTEXPR_CPP14 const_reverse_iterator
    rbegin() const D_NOEXCEPT
    {
        return const_reverse_iterator(end());
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

    // -------------------------------------------------------------------------
    // capacity
    // -------------------------------------------------------------------------

    D_CONSTEXPR size_type size()   const D_NOEXCEPT { return m_size; }
    D_CONSTEXPR size_type length() const D_NOEXCEPT { return m_size; }
    D_CONSTEXPR bool      empty()  const D_NOEXCEPT { return m_size == 0; }

    D_CONSTEXPR size_type
    max_size() const D_NOEXCEPT
    {
        return static_cast<size_type>(-1) / sizeof(_CharT);
    }

    // -------------------------------------------------------------------------
    // element access
    // -------------------------------------------------------------------------

    D_CONSTEXPR const_reference
    operator[](
        size_type  _pos
    ) const
    {
        return m_data[_pos];
    }

    D_CONSTEXPR_CPP14 const_reference
    at(
        size_type  _pos
    ) const
    {
        if (_pos >= m_size)
        {
            internal::sv_throw_out_of_range("restd::basic_string_view::at");
        }
        return m_data[_pos];
    }

    D_CONSTEXPR const_reference front() const { return m_data[0]; }
    D_CONSTEXPR const_reference back()  const { return m_data[m_size - 1]; }
    D_CONSTEXPR const_pointer   data()  const D_NOEXCEPT { return m_data; }

    // -------------------------------------------------------------------------
    // modifiers
    // -------------------------------------------------------------------------

    D_CONSTEXPR_CPP14 void
    remove_prefix(
        size_type  _n
    )
    {
        m_data += _n;
        m_size -= _n;
        return;
    }

    D_CONSTEXPR_CPP14 void
    remove_suffix(
        size_type  _n
    )
    {
        m_size -= _n;
        return;
    }

    D_CONSTEXPR_CPP14 void
    swap(
        basic_string_view&  _other
    ) D_NOEXCEPT
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
    D_CONSTEXPR_CPP14 size_type
    copy(
        _CharT*    _dst,
        size_type  _count,
        size_type  _pos = 0
    ) const
    {
        if (_pos > m_size)
        {
            internal::sv_throw_out_of_range("restd::basic_string_view::copy");
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
    D_CONSTEXPR_CPP14 basic_string_view
    substr(
        size_type  _pos   = 0,
        size_type  _count = npos
    ) const
    {
        if (_pos > m_size)
        {
            internal::sv_throw_out_of_range("restd::basic_string_view::substr");
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
    D_CONSTEXPR_CPP14 int
    compare(
        basic_string_view  _other
    ) const D_NOEXCEPT
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

    D_CONSTEXPR_CPP14 int
    compare(
        size_type          _pos1,
        size_type          _count1,
        basic_string_view  _other
    ) const
    {
        return substr(_pos1, _count1).compare(_other);
    }

    D_CONSTEXPR_CPP14 int
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

    D_CONSTEXPR_CPP14 int
    compare(
        const _CharT*  _s
    ) const
    {
        return compare(basic_string_view(_s));
    }

    D_CONSTEXPR_CPP14 int
    compare(
        size_type      _pos1,
        size_type      _count1,
        const _CharT*  _s
    ) const
    {
        return substr(_pos1, _count1).compare(basic_string_view(_s));
    }

    D_CONSTEXPR_CPP14 int
    compare(
        size_type      _pos1,
        size_type      _count1,
        const _CharT*  _s,
        size_type      _count2
    ) const
    {
        return substr(_pos1, _count1).compare(basic_string_view(_s, _count2));
    }

    // starts_with  (back-port of std C++20)
    D_CONSTEXPR_CPP14 bool
    starts_with(
        basic_string_view  _x
    ) const D_NOEXCEPT
    {
        return m_size >= _x.m_size
               && traits_type::compare(m_data, _x.m_data, _x.m_size) == 0;
    }

    D_CONSTEXPR bool
    starts_with(
        _CharT  _c
    ) const D_NOEXCEPT
    {
        return m_size != 0 && traits_type::eq(m_data[0], _c);
    }

    D_CONSTEXPR_CPP14 bool
    starts_with(
        const _CharT*  _s
    ) const
    {
        return starts_with(basic_string_view(_s));
    }

    // ends_with  (back-port of std C++20)
    D_CONSTEXPR_CPP14 bool
    ends_with(
        basic_string_view  _x
    ) const D_NOEXCEPT
    {
        return m_size >= _x.m_size
               && traits_type::compare(
                      m_data + (m_size - _x.m_size), _x.m_data, _x.m_size) == 0;
    }

    D_CONSTEXPR bool
    ends_with(
        _CharT  _c
    ) const D_NOEXCEPT
    {
        return m_size != 0 && traits_type::eq(m_data[m_size - 1], _c);
    }

    D_CONSTEXPR_CPP14 bool
    ends_with(
        const _CharT*  _s
    ) const
    {
        return ends_with(basic_string_view(_s));
    }

    // contains  (back-port of std C++23)
    D_CONSTEXPR_CPP14 bool
    contains(
        basic_string_view  _x
    ) const D_NOEXCEPT
    {
        return find(_x) != npos;
    }

    D_CONSTEXPR_CPP14 bool
    contains(
        _CharT  _c
    ) const D_NOEXCEPT
    {
        return find(_c) != npos;
    }

    D_CONSTEXPR_CPP14 bool
    contains(
        const _CharT*  _s
    ) const
    {
        return find(_s) != npos;
    }

    // -------------------------------------------------------------------------
    // search — find
    // -------------------------------------------------------------------------

    D_CONSTEXPR_CPP14 size_type
    find(
        basic_string_view  _x,
        size_type          _pos = 0
    ) const D_NOEXCEPT
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

    D_CONSTEXPR_CPP14 size_type
    find(
        _CharT     _c,
        size_type  _pos = 0
    ) const D_NOEXCEPT
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

    D_CONSTEXPR_CPP14 size_type
    find(
        const _CharT*  _s,
        size_type      _pos,
        size_type      _count
    ) const
    {
        return find(basic_string_view(_s, _count), _pos);
    }

    D_CONSTEXPR_CPP14 size_type
    find(
        const _CharT*  _s,
        size_type      _pos = 0
    ) const
    {
        return find(basic_string_view(_s), _pos);
    }

    // -------------------------------------------------------------------------
    // search — rfind
    // -------------------------------------------------------------------------

    D_CONSTEXPR_CPP14 size_type
    rfind(
        basic_string_view  _x,
        size_type          _pos = npos
    ) const D_NOEXCEPT
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

    D_CONSTEXPR_CPP14 size_type
    rfind(
        _CharT     _c,
        size_type  _pos = npos
    ) const D_NOEXCEPT
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

    D_CONSTEXPR_CPP14 size_type
    rfind(
        const _CharT*  _s,
        size_type      _pos,
        size_type      _count
    ) const
    {
        return rfind(basic_string_view(_s, _count), _pos);
    }

    D_CONSTEXPR_CPP14 size_type
    rfind(
        const _CharT*  _s,
        size_type      _pos = npos
    ) const
    {
        return rfind(basic_string_view(_s), _pos);
    }

    // -------------------------------------------------------------------------
    // search — find_first_of
    // -------------------------------------------------------------------------

    D_CONSTEXPR_CPP14 size_type
    find_first_of(
        basic_string_view  _x,
        size_type          _pos = 0
    ) const D_NOEXCEPT
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

    D_CONSTEXPR_CPP14 size_type
    find_first_of(
        _CharT     _c,
        size_type  _pos = 0
    ) const D_NOEXCEPT
    {
        return find(_c, _pos);
    }

    D_CONSTEXPR_CPP14 size_type
    find_first_of(
        const _CharT*  _s,
        size_type      _pos,
        size_type      _count
    ) const
    {
        return find_first_of(basic_string_view(_s, _count), _pos);
    }

    D_CONSTEXPR_CPP14 size_type
    find_first_of(
        const _CharT*  _s,
        size_type      _pos = 0
    ) const
    {
        return find_first_of(basic_string_view(_s), _pos);
    }

    // -------------------------------------------------------------------------
    // search — find_last_of
    // -------------------------------------------------------------------------

    D_CONSTEXPR_CPP14 size_type
    find_last_of(
        basic_string_view  _x,
        size_type          _pos = npos
    ) const D_NOEXCEPT
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

    D_CONSTEXPR_CPP14 size_type
    find_last_of(
        _CharT     _c,
        size_type  _pos = npos
    ) const D_NOEXCEPT
    {
        return rfind(_c, _pos);
    }

    D_CONSTEXPR_CPP14 size_type
    find_last_of(
        const _CharT*  _s,
        size_type      _pos,
        size_type      _count
    ) const
    {
        return find_last_of(basic_string_view(_s, _count), _pos);
    }

    D_CONSTEXPR_CPP14 size_type
    find_last_of(
        const _CharT*  _s,
        size_type      _pos = npos
    ) const
    {
        return find_last_of(basic_string_view(_s), _pos);
    }

    // -------------------------------------------------------------------------
    // search — find_first_not_of
    // -------------------------------------------------------------------------

    D_CONSTEXPR_CPP14 size_type
    find_first_not_of(
        basic_string_view  _x,
        size_type          _pos = 0
    ) const D_NOEXCEPT
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

    D_CONSTEXPR_CPP14 size_type
    find_first_not_of(
        _CharT     _c,
        size_type  _pos = 0
    ) const D_NOEXCEPT
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

    D_CONSTEXPR_CPP14 size_type
    find_first_not_of(
        const _CharT*  _s,
        size_type      _pos,
        size_type      _count
    ) const
    {
        return find_first_not_of(basic_string_view(_s, _count), _pos);
    }

    D_CONSTEXPR_CPP14 size_type
    find_first_not_of(
        const _CharT*  _s,
        size_type      _pos = 0
    ) const
    {
        return find_first_not_of(basic_string_view(_s), _pos);
    }

    // -------------------------------------------------------------------------
    // search — find_last_not_of
    // -------------------------------------------------------------------------

    D_CONSTEXPR_CPP14 size_type
    find_last_not_of(
        basic_string_view  _x,
        size_type          _pos = npos
    ) const D_NOEXCEPT
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

    D_CONSTEXPR_CPP14 size_type
    find_last_not_of(
        _CharT     _c,
        size_type  _pos = npos
    ) const D_NOEXCEPT
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

    D_CONSTEXPR_CPP14 size_type
    find_last_not_of(
        const _CharT*  _s,
        size_type      _pos,
        size_type      _count
    ) const
    {
        return find_last_not_of(basic_string_view(_s, _count), _pos);
    }

    D_CONSTEXPR_CPP14 size_type
    find_last_not_of(
        const _CharT*  _s,
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
#if !D_ENV_LANG_IS_CPP17_OR_HIGHER
template<typename _CharT,
         typename _Traits>
D_CONSTEXPR typename basic_string_view<_CharT, _Traits>::size_type
basic_string_view<_CharT, _Traits>::npos;
#endif


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_STRING_VIEW_BASIC_STRING_VIEW_
