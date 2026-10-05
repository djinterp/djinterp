/*******************************************************************************
* djinterp [re_std]                                            as_const_view.hpp
*
* as_const_view header:
*   Provides the C++23 const-projection adaptor. as_const_view<V>
* presents an underlying view V with each element exposed as a
* const reference.
*
*   This R22 rewrite delegates to re_std::basic_const_iterator
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
*     re_std::basic_const_iterator.
*   - enable_borrowed_range inherits from the underlying view.
*
*   COLOCATED:
*   re_std::views::as_const(r).
*
*
* path:      /inc/re_std/ranges/as_const_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_RANGES_AS_CONST_VIEW_HPP
#define RE_STD_RANGES_AS_CONST_VIEW_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../iterator/basic_const_iterator.hpp"
#include "./view_interface.hpp"
#include "./iterator_t.hpp"
#include "./sentinel_t.hpp"
#include "./enable_borrowed_range.hpp"
#include "./all.hpp"
#include "./range_adaptor_closure.hpp"


namespace re_std
{


// ===========================================================================
// I.   AS_CONST_VIEW
// ===========================================================================

// as_const_view<View>
//   class: lazy const-projection of View. begin yields a
// basic_const_iterator wrapping the underlying iterator; end yields
// the underlying sentinel unchanged (it cross-compares correctly).
template<typename View>
class as_const_view : public view_interface<as_const_view<View> >
{
public:
    typedef View                                           base_view;
    typedef basic_const_iterator<iterator_t<View> >        iterator;
    typedef sentinel_t<View>                               sentinel;


private:
    View   m_base;


public:
    // -------- ctors --------
    RE_STD_CONSTEXPR
    as_const_view()
        : m_base()
    {}

    RE_STD_CONSTEXPR
    as_const_view(
        View  _base
    )
        : m_base(static_cast<View&&>(_base))
    {}


    // -------- base accessor --------
    RE_STD_CONSTEXPR View
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
    RE_STD_CONSTEXPR_CPP14 iterator
    begin()
    {
        return iterator(re_std::begin(m_base));
    }

    RE_STD_CONSTEXPR iterator
    begin() const
    {
        return iterator(re_std::begin(m_base));
    }

    RE_STD_CONSTEXPR_CPP14 sentinel
    end()
    {
        return re_std::end(m_base);
    }

    RE_STD_CONSTEXPR sentinel
    end() const
    {
        return re_std::end(m_base);
    }


    // -------- size --------
    RE_STD_CONSTEXPR_CPP14
    auto
    size()
        -> decltype(re_std::size(m_base))
    {
        return re_std::size(m_base);
    }

    RE_STD_CONSTEXPR
    auto
    size() const
        -> decltype(re_std::size(m_base))
    {
        return re_std::size(m_base);
    }
};


// ===========================================================================
// II.  ENABLE_BORROWED_RANGE OPT-IN
// ===========================================================================

template<typename View>
struct enable_borrowed_range<as_const_view<View> >
    : enable_borrowed_range<View>
{};


// ===========================================================================
// III. VIEWS::AS_CONST (pipe-enabled closure-fn)
// ===========================================================================

namespace views
{
    struct as_const_fn : range_adaptor_closure<as_const_fn>
    {
        template<typename R>
        RE_STD_CONSTEXPR_INLINE
        as_const_view<typename internal::all_dispatch<R>::type>
        operator()(
            R&&  _r
        ) const
        {
            typedef typename internal::all_dispatch<R>::type view_type;
            return as_const_view<view_type>(
                internal::all_dispatch<R>::call(static_cast<R&&>(_r))
            );
        }
    };

#if RE_STD_LANG_IS_CPP17_OR_HIGHER
    inline RE_STD_CONSTEXPR as_const_fn as_const = as_const_fn();
#else
    static RE_STD_CONSTEXPR as_const_fn as_const = as_const_fn();
#endif
}  // namespace views


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RANGES_AS_CONST_VIEW_HPP
