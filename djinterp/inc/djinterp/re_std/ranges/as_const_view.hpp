/******************************************************************************
* djinterp [restd]                                           as_const_view.hpp
*
* as_const_view header:
*   Provides the C++23 const-projection adaptor. as_const_view<V>
* presents an underlying view V with each element exposed as a
* const reference.
*
*   This R22 rewrite delegates to restd::basic_const_iterator
* (shipped as part of the R22 internal-utilities batch); the R19
* original used a hand-rolled iterator with direct static_cast and
* duplicated the full iterator surface. The new implementation is
* a thin wrapper — begin returns basic_const_iterator<iterator_t<V>>;
* end returns the underlying sentinel directly (the
* basic_const_iterator template has cross-type == / != with
* arbitrary sentinel types, so this works for both common and
* non-common ranges without partial specialisation).
*
*   PORTABILITY:
*   - C++11+; CRTP + view_interface + delegation to
*     restd::basic_const_iterator.
*   - enable_borrowed_range inherits from the underlying view.
*
*   COLOCATED:
*   restd::views::as_const(r).
*
*
* path:      /inc/djinterp/restd/ranges/as_const_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_AS_CONST_VIEW_
#define DJINTERP_RESTD_RANGES_AS_CONST_VIEW_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../iterator/basic_const_iterator.hpp"
#include "./view_interface.hpp"
#include "./iterator_t.hpp"
#include "./sentinel_t.hpp"
#include "./enable_borrowed_range.hpp"
#include "./all.hpp"
#include "./range_adaptor_closure.hpp"


NS_RESTD


// ===========================================================================
// I.   AS_CONST_VIEW
// ===========================================================================

// as_const_view<_View>
//   class: lazy const-projection of _View. begin yields a
// basic_const_iterator wrapping the underlying iterator; end yields
// the underlying sentinel unchanged (it cross-compares correctly).
template<typename _View>
class as_const_view : public view_interface<as_const_view<_View> >
{
public:
    typedef _View                                           base_view;
    typedef basic_const_iterator<iterator_t<_View> >        iterator;
    typedef sentinel_t<_View>                               sentinel;


private:
    _View   m_base;


public:
    // -------- ctors --------
    D_CONSTEXPR
    as_const_view()
        : m_base()
    {}

    D_CONSTEXPR
    as_const_view(
        _View  _base
    )
        : m_base(static_cast<_View&&>(_base))
    {}


    // -------- base accessor --------
    D_CONSTEXPR _View
    base() const
    {
        return m_base;
    }


    // -------- begin / end --------
    //   function: begin() wraps the underlying begin in a
    // basic_const_iterator. end() returns the underlying sentinel
    // directly. The cross-type == / != on basic_const_iterator
    // makes this work whether or not the underlying view is a
    // common_range.
    D_CONSTEXPR iterator
    begin()
    {
        return iterator(restd::begin(m_base));
    }

    D_CONSTEXPR iterator
    begin() const
    {
        return iterator(restd::begin(m_base));
    }

    D_CONSTEXPR sentinel
    end()
    {
        return restd::end(m_base);
    }

    D_CONSTEXPR sentinel
    end() const
    {
        return restd::end(m_base);
    }


    // -------- size --------
    D_CONSTEXPR
    auto
    size()
        -> decltype(restd::size(m_base))
    {
        return restd::size(m_base);
    }

    D_CONSTEXPR
    auto
    size() const
        -> decltype(restd::size(m_base))
    {
        return restd::size(m_base);
    }
};


// ===========================================================================
// II.  ENABLE_BORROWED_RANGE OPT-IN
// ===========================================================================

template<typename _View>
struct enable_borrowed_range<as_const_view<_View> >
    : enable_borrowed_range<_View>
{};


// ===========================================================================
// III. VIEWS::AS_CONST (pipe-enabled closure-fn)
// ===========================================================================

namespace views
{
    struct as_const_fn : range_adaptor_closure<as_const_fn>
    {
        template<typename _R>
        D_CONSTEXPR_INLINE
        as_const_view<typename internal::all_dispatch<_R>::type>
        operator()(
            _R&&  _r
        ) const
        {
            typedef typename internal::all_dispatch<_R>::type view_type;
            return as_const_view<view_type>(
                internal::all_dispatch<_R>::call(static_cast<_R&&>(_r))
            );
        }
    };

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    inline D_CONSTEXPR as_const_fn as_const = as_const_fn();
#else
    static D_CONSTEXPR as_const_fn as_const = as_const_fn();
#endif
}  // namespace views


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_RANGES_AS_CONST_VIEW_
