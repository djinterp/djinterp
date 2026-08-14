/******************************************************************************
* re_std [ranges]                                              elements_view.hpp
*
*   elements_view<V, N> - projects each tuple-like element onto its N'th
* member.  keys_view and values_view are the N=0 and N=1 spellings.
*
*   WHY IT IS NOT JUST transform_view WITH A LAMBDA.
*   `r | transform([](auto& p){ return get<0>(p); })` returns by VALUE, which
* copies every key and makes the result read-only.  elements_view yields a
* REFERENCE into the underlying element, so `for (auto& k : keys(m)) k = ...`
* modifies the map's elements in place and no copy is made.  Preserving the
* reference is the entire reason this adaptor exists as its own type rather
* than as a convenience alias over transform.
*
*   THE REFERENCE TYPE IS COMPUTED, NOT ASSUMED.  It is whatever
* `get<N>(*base_iterator)` yields - which for a pair<const K, V>& is
* `const K&` for N=0 and `V&` for N=1, asymmetrically.  Hard-coding either
* would break the other, so the iterator derives it with decltype rather than
* naming a type.
*
*   STD IS C++20 (keys/values C++20, elements C++20); re_std IS C++11.
*
*   INTERFACE ASSUMPTIONS: see ADAPTOR_ASSUMPTIONS.txt in this directory.
*
* path:      /inc/djinterp/re_std/ranges/elements_view.hpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_RANGES_ELEMENTS_VIEW_
#define RESTD_RANGES_ELEMENTS_VIEW_ 1

#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../tuple/tuple.hpp"
#include "../utility/utility.hpp"
#include "../iterator/iterator_tags.hpp"
#include "./range_traits.hpp"
#include "./range_access.hpp"
#include "./view_interface.hpp"

NS_DJINTERP
NS_RESTD
D_NAMESPACE(ranges)

// elements_view
//   class: projects each element onto its N'th tuple member.
template<typename _View, size_t _Index>
class elements_view : public view_interface<elements_view<_View, _Index> >
{
    typedef iterator_t<_View> _BaseIter;
    typedef sentinel_t<_View> _BaseSent;

    _View m_base;

public:
    class sentinel
    {
        _BaseSent m_end;
    public:
        sentinel() : m_end() {}
        explicit sentinel(const _BaseSent& e) : m_end(e) {}
        const _BaseSent& base() const { return m_end; }
    };

    class iterator
    {
        _BaseIter m_it;

    public:
        //   Derived, not named: get<0> of a pair<const K,V>& is const K&
        // while get<1> is V&, so no single spelling covers both.
        typedef decltype(re_std::get<_Index>(*declval<_BaseIter&>())) reference;
        typedef typename remove_cv<
            typename remove_reference<reference>::type>::type value_type;
        typedef ptrdiff_t          difference_type;
        typedef void               pointer;
        typedef input_iterator_tag iterator_category;

        iterator() : m_it() {}
        explicit iterator(const _BaseIter& it) : m_it(it) {}

        const _BaseIter& base() const { return m_it; }

        reference operator*() const { return re_std::get<_Index>(*m_it); }

        iterator& operator++() { ++m_it; return *this; }
        iterator  operator++(int) { iterator t = *this; ++(*this); return t; }

        friend bool operator==(const iterator& a, const iterator& b)
        { return a.m_it == b.m_it; }
        friend bool operator!=(const iterator& a, const iterator& b)
        { return !(a.m_it == b.m_it); }
        friend bool operator==(const iterator& a, const sentinel& s)
        { return a.m_it == s.base(); }
        friend bool operator==(const sentinel& s, const iterator& a)
        { return a.m_it == s.base(); }
        friend bool operator!=(const iterator& a, const sentinel& s)
        { return !(a.m_it == s.base()); }
        friend bool operator!=(const sentinel& s, const iterator& a)
        { return !(a.m_it == s.base()); }
    };

    elements_view() : m_base() {}
    explicit elements_view(_View base) : m_base(static_cast<_View&&>(base)) {}

    iterator begin() { return iterator(ranges::begin(m_base)); }
    sentinel end()   { return sentinel(ranges::end(m_base)); }
};

//   keys_view / values_view are spellings, not separate types.
template<typename _View> struct keys_view_of   { typedef elements_view<_View, 0> type; };
template<typename _View> struct values_view_of { typedef elements_view<_View, 1> type; };

NS_END  // ranges
NS_END
NS_END

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_RANGES_ELEMENTS_VIEW_
