/******************************************************************************
* djinterp [restd]                                                subrange.hpp
*
* subrange class template header:
*   Provides the C++20 iterator/sentinel pair view. subrange<I, S, K>
* models a contiguous slice of a larger range without owning the
* underlying storage. Selecting K = subrange_kind::sized adds a
* cached size field; K = subrange_kind::unsized omits it (saves a
* word when the size can always be computed by sentinel arithmetic).
*
*   PORTABILITY:
*   - Requires alias templates, decltype, rvalue refs, default
*     template arguments on functions. Available C++11+ only.
*   - Concepts (range, sized_sentinel_for, etc.) are emulated via
*     SFINAE where present in the C++20 contract — many spots in the
*     standard's contract are just left to lazy template instantiation
*     here, exactly as with view_interface.
*   - Specialises enable_borrowed_range<subrange<...>> to true,
*     mirroring the std::ranges::enable_borrowed_range opt-in for
*     std::ranges::subrange.
*
*
* path:      /inc/djinterp/restd/ranges/subrange.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_SUBRANGE_
#define DJINTERP_RESTD_RANGES_SUBRANGE_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../iterator/iterator_traits.hpp"
#include "./subrange_kind.hpp"
#include "./view_interface.hpp"
#include "./enable_borrowed_range.hpp"


NS_RESTD


// ===========================================================================
// I.   SUBRANGE (primary template)
// ===========================================================================

// subrange
//   class: iterator/sentinel pair. CRTP-derives from view_interface
// to inherit empty / front / back / operator[] / operator bool /
// data. size() is provided here only when _Kind is sized OR when
// the sentinel supports operator- with the iterator.
template<typename _Iter,
         typename _Sent = _Iter,
         subrange_kind _Kind = subrange_kind::unsized>
class subrange
    : public view_interface<subrange<_Iter, _Sent, _Kind> >
{
private:
    // size type derived from iterator_traits. Used only when sized.
    typedef typename iterator_traits<_Iter>::difference_type difference_type;


public:
    // iterator
    //   alias: re-export of the iterator template parameter.
    typedef _Iter   iterator;

    // sentinel
    //   alias: re-export of the sentinel template parameter.
    typedef _Sent   sentinel;


private:
    _Iter m_begin;
    _Sent m_end;


public:
    // default ctor
    //   function: value-initialised iterator and sentinel.
    D_CONSTEXPR
    subrange()
        : m_begin(),
          m_end()
    {}

    // value ctor (iter, sent)
    //   function: pair-of-endpoints construction. The kind is
    // unsized here — clients constructing a sized subrange via the
    // primary template must use the (iter, sent, size) overload.
    D_CONSTEXPR
    subrange(
        _Iter _b,
        _Sent _e
    )
        : m_begin(static_cast<_Iter&&>(_b)),
          m_end(static_cast<_Sent&&>(_e))
    {}

    // value ctor (iter, sent, size)
    //   function: sized-form construction. The size argument is
    // accepted on the primary template for API uniformity but is
    // not cached here; lookups go through end - begin. Overrides
    // are provided in the sized specialisation below.
    D_CONSTEXPR
    subrange(
        _Iter                _b,
        _Sent                _e,
        difference_type      // _n  -- intentionally unnamed, unused
    )
        : m_begin(static_cast<_Iter&&>(_b)),
          m_end(static_cast<_Sent&&>(_e))
    {}


    // begin
    //   function: returns the start iterator.
    D_CONSTEXPR _Iter
    begin() const
    {
        return m_begin;
    }

    // end
    //   function: returns the sentinel.
    D_CONSTEXPR _Sent
    end() const
    {
        return m_end;
    }

    // empty
    //   function: shadows view_interface::empty for the common
    // iter == sent case. (view_interface's version still works.)
    D_CONSTEXPR bool
    empty() const
    {
        return (m_begin == m_end);
    }

    // size (sized-only, via lazy SFINAE on sentinel - iterator)
    //   function: end() - begin(). Instantiates only when the
    // expression is well-formed. The signature uses a deduced
    // trailing return type so SFINAE applies on instantiation.
    D_CONSTEXPR
    auto
    size() const
        -> decltype(m_end - m_begin)
    {
        return (m_end - m_begin);
    }

    // advance
    //   function: advances begin() by _n positions. Returns
    // a reference to *this for chaining.
    D_CONSTEXPR_INLINE
    subrange&
    advance(
        difference_type _n
    )
    {
        // for input/forward iterators advance() in <iterator> handles
        // positive n; for bidirectional/random it handles negative as
        // well. The cost is iterator_category-dependent.
        for (; _n > 0; --_n)
        {
            ++m_begin;
        }
        for (; _n < 0; ++_n)
        {
            --m_begin;
        }

        return *this;
    }

    // next
    //   function: returns a copy of *this with begin() advanced by
    // _n positions.
    D_CONSTEXPR_INLINE
    subrange
    next(
        difference_type _n = 1
    ) const
    {
        subrange result(*this);
        result.advance(_n);
        return result;
    }

    // prev
    //   function: returns a copy of *this with begin() retreated by
    // _n positions.
    D_CONSTEXPR_INLINE
    subrange
    prev(
        difference_type _n = 1
    ) const
    {
        subrange result(*this);
        result.advance(-_n);
        return result;
    }
};


// ===========================================================================
// II.  SUBRANGE (sized specialisation)
// ===========================================================================

// subrange<_Iter, _Sent, sized>
//   class: stores an explicit cached size alongside the
// iterator/sentinel pair. Used when the underlying sentinel is not
// sized_sentinel_for the iterator (so end - begin is not O(1)) but
// the size is known up front.
template<typename _Iter,
         typename _Sent>
class subrange<_Iter, _Sent, subrange_kind::sized>
    : public view_interface<subrange<_Iter, _Sent, subrange_kind::sized> >
{
public:
    typedef _Iter   iterator;
    typedef _Sent   sentinel;

private:
    typedef typename iterator_traits<_Iter>::difference_type difference_type;

    _Iter           m_begin;
    _Sent           m_end;
    difference_type m_size;


public:
    // default ctor
    D_CONSTEXPR
    subrange()
        : m_begin(),
          m_end(),
          m_size(0)
    {}

    // value ctor (iter, sent, size)
    //   function: required form for the sized specialisation —
    // size must be supplied because we cannot compute it
    // automatically on a non-sized-sentinel pair.
    D_CONSTEXPR
    subrange(
        _Iter           _b,
        _Sent           _e,
        difference_type _n
    )
        : m_begin(static_cast<_Iter&&>(_b)),
          m_end(static_cast<_Sent&&>(_e)),
          m_size(_n)
    {}

    // value ctor (iter, sent) - sized-sentinel path
    //   function: kept for source compatibility with the unsized
    // primary; the size is computed once via sentinel arithmetic
    // and cached. Instantiates only when _Sent - _Iter is
    // well-formed.
    D_CONSTEXPR
    subrange(
        _Iter _b,
        _Sent _e
    )
        : m_begin(static_cast<_Iter&&>(_b)),
          m_end(static_cast<_Sent&&>(_e)),
          m_size(_e - _b)
    {}


    D_CONSTEXPR _Iter
    begin() const
    {
        return m_begin;
    }

    D_CONSTEXPR _Sent
    end() const
    {
        return m_end;
    }

    D_CONSTEXPR bool
    empty() const
    {
        return (m_size == 0);
    }

    // size (cached)
    //   function: returns the cached size. O(1) regardless of
    // iterator category.
    D_CONSTEXPR difference_type
    size() const
    {
        return m_size;
    }

    D_CONSTEXPR_INLINE
    subrange&
    advance(
        difference_type _n
    )
    {
        for (; _n > 0 && m_size > 0; --_n, --m_size)
        {
            ++m_begin;
        }
        for (; _n < 0; ++_n, ++m_size)
        {
            --m_begin;
        }

        return *this;
    }

    D_CONSTEXPR_INLINE
    subrange
    next(
        difference_type _n = 1
    ) const
    {
        subrange result(*this);
        result.advance(_n);
        return result;
    }

    D_CONSTEXPR_INLINE
    subrange
    prev(
        difference_type _n = 1
    ) const
    {
        subrange result(*this);
        result.advance(-_n);
        return result;
    }
};


// ===========================================================================
// III. BORROWED_RANGE OPT-IN
// ===========================================================================

// enable_borrowed_range<subrange<I, S, K>>
//   trait: subrange is a borrowed_range — its iterators remain
// valid after the subrange itself is destroyed because the subrange
// does not own the underlying storage.
template<typename _Iter,
         typename _Sent,
         subrange_kind _Kind>
struct enable_borrowed_range<subrange<_Iter, _Sent, _Kind> >
    : true_type
{};


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_RANGES_SUBRANGE_
