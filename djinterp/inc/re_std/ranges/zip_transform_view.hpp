/*******************************************************************************
* djinterp [re_std]                                       zip_transform_view.hpp
*
* zip_transform_view view header:
*   zip_transform_view - walks N ranges in lockstep and yields f(a, b, ...)
* rather than the tuple.
*
*   IT IS NOT zip | transform, AND THE DIFFERENCE IS THE CALL SHAPE.
*   Layering transform over zip would hand the callable ONE tuple argument, so
*   every user would have to write `[](auto t){ return get<0>(t)+get<1>(t); }`
*   and unpack by hand.  std specifies the callable as taking the elements as
*   SEPARATE arguments - `[](int a, int b){ return a+b; }` - which is what
*   people actually want and what makes it composable with ordinary binary
*   functions like plus<>.  Implementing it directly rather than as a
*   composition is what preserves that.
*
*   The result is therefore invoke_result over the PACK of references, not
*   over a tuple - which is also why the reference type may legitimately be a
*   prvalue: f may return by value, and usually does.
*
*   SHORTEST-RANGE RULE, as zip - the sentinel test is "ANY component at its
*   end".  See zip_view.hpp.
*
*   STD IS C++23; re_std IS C++11.
*   INTERFACE ASSUMPTIONS: see ADAPTOR_ASSUMPTIONS.txt in this directory.
*
*
* path:      /inc/re_std/ranges/zip_transform_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_ZIP_TRANSFORM_VIEW_HPP
#define RE_STD_RANGES_ZIP_TRANSFORM_VIEW_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "../tuple/tuple.hpp"
#include "../functional/invoke.hpp"
#include "./view_interface.hpp"
#include "./zip_view.hpp"

namespace re_std
{
namespace ranges
{

// zip_transform_view
//   class: f applied to the lockstep elements of N ranges.
template<typename Func, typename... Views>
class zip_transform_view
    : public view_interface<zip_transform_view<Func, Views...> >
{
    typedef tuple<iterator_t<Views>...> _IterTuple;
    typedef tuple<sentinel_t<Views>...> _SentTuple;
    typedef internal::zip_ops<0, sizeof...(Views)> _Ops;
    typedef make_index_sequence<sizeof...(Views)>  _Indices;

    Func            m_func;
    tuple<Views...> m_views;

    template<size_t... I>
    _IterTuple make_begin(index_sequence<I...>)
    { return _IterTuple(ranges::begin(re_std::get<I>(m_views))...); }

    template<size_t... I>
    _SentTuple make_end(index_sequence<I...>)
    { return _SentTuple(ranges::end(re_std::get<I>(m_views))...); }

public:
    class sentinel
    {
        _SentTuple m_ends;
    public:
        sentinel() : m_ends() {}
        explicit sentinel(const _SentTuple& e) : m_ends(e) {}
        const _SentTuple& ends() const { return m_ends; }
    };

    class iterator
    {
        const Func* m_func;
        _IterTuple   m_its;

        template<size_t... I>
        auto call(index_sequence<I...>) const
            -> decltype(re_std::invoke(*m_func, *re_std::get<I>(m_its)...))
        { return re_std::invoke(*m_func, *re_std::get<I>(m_its)...); }

    public:
        //   Elements as SEPARATE arguments, not one tuple - see the header.
        typedef decltype(re_std::invoke(
            declval<const Func&>(),
            declval<range_reference_t<Views> >()...)) reference;
        typedef typename remove_cv<
            typename remove_reference<reference>::type>::type value_type;
        typedef ptrdiff_t          difference_type;
        typedef void               pointer;
        typedef input_iterator_tag iterator_category;

        iterator() : m_func(0), m_its() {}
        iterator(const Func& f, const _IterTuple& its)
            : m_func(&f), m_its(its) {}

        const _IterTuple& iters() const { return m_its; }

        reference operator*() const
        { return call(make_index_sequence<sizeof...(Views)>()); }

        iterator& operator++() { _Ops::advance(m_its); return *this; }
        iterator  operator++(int) { iterator t = *this; ++(*this); return t; }

        friend bool operator==(const iterator& a, const iterator& b)
        { return a.m_its == b.m_its; }
        friend bool operator!=(const iterator& a, const iterator& b)
        { return !(a.m_its == b.m_its); }
        friend bool operator==(const iterator& it, const sentinel& s)
        { return _Ops::any_at_end(it.m_its, s.ends()); }
        friend bool operator==(const sentinel& s, const iterator& it)
        { return _Ops::any_at_end(it.m_its, s.ends()); }
        friend bool operator!=(const iterator& it, const sentinel& s)
        { return !_Ops::any_at_end(it.m_its, s.ends()); }
        friend bool operator!=(const sentinel& s, const iterator& it)
        { return !_Ops::any_at_end(it.m_its, s.ends()); }
    };

    zip_transform_view() : m_func(), m_views() {}
    zip_transform_view(Func f, Views... views)
        : m_func(static_cast<Func&&>(f)),
          m_views(static_cast<Views&&>(views)...) {}

    iterator begin() { return iterator(m_func, make_begin(_Indices())); }
    sentinel end()   { return sentinel(make_end(_Indices())); }
};

}  // ranges
}

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_RANGES_ZIP_TRANSFORM_VIEW_HPP
