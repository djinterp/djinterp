/******************************************************************************
* re_std [ranges]                                                   zip_view.hpp
*
*   zip_view - walks N ranges in lockstep, yielding a tuple of references.
*
*   IT ENDS AT THE SHORTEST RANGE, and that single rule is most of the design.
* The sentinel comparison is therefore "is ANY component at its end", not
* "are ALL components at their ends" - the second would run off the end of
* every range shorter than the longest one, which is undefined behaviour
* rather than a wrong answer.  There is a test for exactly this with ranges of
* differing length.
*
*   DEREFERENCING YIELDS A TUPLE OF REFERENCES, not of values, so writing
* through a zipped element mutates the underlying range.  That is what makes
* `zip(a, b)` usable as an output range and what makes zip + sort work.  It
* also means value_type and reference are DIFFERENT types - tuple<T...> versus
* tuple<T&...> - which is why the iterator declares both rather than deriving
* one from the other.
*
*   COMPONENT ITERATORS ARE ADVANCED IN LOCKSTEP by a recursive helper rather
* than an index_sequence fold, because a fold expression is C++17 and this
* header reaches C++11.
*
*   STD IS C++23; re_std IS C++11 - a twelve-year back-port. The adaptor needs
* variadic templates and a tuple, both C++11; std was late because <ranges>
* itself was.
*
*   INTERFACE ASSUMPTIONS: see ADAPTOR_ASSUMPTIONS.txt in this directory.
*
* path:      /inc/djinterp/re_std/ranges/zip_view.hpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_RANGES_ZIP_VIEW_
#define DJINTERP_RE_STD_RANGES_ZIP_VIEW_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "../tuple/tuple.hpp"
#include "../iterator/iterator_tags.hpp"
#include "./range_traits.hpp"
#include "./range_access.hpp"
#include "./view_interface.hpp"

NS_RESTD
D_NAMESPACE(ranges)

NS_INTERNAL

    // zip_ops
    //   struct: lockstep operations over a tuple of component iterators.
    // Recursive rather than a fold expression, which would need C++17.
    template<size_t _Index, size_t _Count>
    struct zip_ops
    {
        template<typename _Its, typename _Sents>
        static bool any_at_end(const _Its& its, const _Sents& sents)
        {
            //   ANY, not all - see the header note.
            return (re_std::get<_Index>(its) == re_std::get<_Index>(sents))
                || zip_ops<_Index + 1, _Count>::any_at_end(its, sents);
        }

        template<typename _Its>
        static void advance(_Its& its)
        {
            ++re_std::get<_Index>(its);
            zip_ops<_Index + 1, _Count>::advance(its);
        }
    };

    template<size_t _Count>
    struct zip_ops<_Count, _Count>
    {
        template<typename _Its, typename _Sents>
        static bool any_at_end(const _Its&, const _Sents&) { return false; }

        template<typename _Its>
        static void advance(_Its&) { return; }
    };

NS_END  // internal


// zip_view
//   class: N ranges walked in lockstep.
template<typename... _Views>
class zip_view : public view_interface<zip_view<_Views...> >
{
    typedef tuple<iterator_t<_Views>...> _IterTuple;
    typedef tuple<sentinel_t<_Views>...> _SentTuple;
    typedef internal::zip_ops<0, sizeof...(_Views)> _Ops;
    typedef make_index_sequence<sizeof...(_Views)>  _Indices;

    tuple<_Views...> m_views;

    template<size_t... _I>
    _IterTuple make_begin(index_sequence<_I...>)
    { return _IterTuple(ranges::begin(re_std::get<_I>(m_views))...); }

    template<size_t... _I>
    _SentTuple make_end(index_sequence<_I...>)
    { return _SentTuple(ranges::end(re_std::get<_I>(m_views))...); }

public:
    class sentinel
    {
        _SentTuple m_ends;
    public:
        sentinel() : m_ends() {}
        explicit sentinel(const _SentTuple& ends) : m_ends(ends) {}
        const _SentTuple& ends() const { return m_ends; }
    };

    class iterator
    {
        _IterTuple m_its;

        template<size_t... _I>
        tuple<range_reference_t<_Views>...> deref(index_sequence<_I...>) const
        { return tuple<range_reference_t<_Views>...>(*re_std::get<_I>(m_its)...); }

    public:
        //   value_type and reference DIFFER - tuple of values versus tuple of
        // references. Declaring both is what lets algorithms copy an element
        // out while still writing through the iterator.
        typedef tuple<typename remove_reference<
                    range_reference_t<_Views> >::type...> value_type;
        typedef tuple<range_reference_t<_Views>...>       reference;
        typedef ptrdiff_t                                 difference_type;
        typedef void                                      pointer;
        typedef input_iterator_tag                        iterator_category;

        iterator() : m_its() {}
        explicit iterator(const _IterTuple& its) : m_its(its) {}

        const _IterTuple& iters() const { return m_its; }

        reference operator*() const
        { return deref(make_index_sequence<sizeof...(_Views)>()); }

        iterator& operator++() { _Ops::advance(m_its); return *this; }
        iterator  operator++(int) { iterator t = *this; ++(*this); return t; }

        //   HIDDEN FRIENDS, not namespace-scope templates. A non-member
        // template taking `typename zip_view<_Views...>::iterator` puts
        // _Views in a NON-DEDUCED context, so it can never be called - the
        // first draft did exactly that and silently had no comparison
        // operators at all. Defining them inside the class sidesteps
        // deduction entirely and keeps them findable by ADL.
        friend bool operator==(const iterator& a, const iterator& b)
        { return a.m_its == b.m_its; }
        friend bool operator!=(const iterator& a, const iterator& b)
        { return !(a.m_its == b.m_its); }

        //   The shortest-range rule.
        friend bool operator==(const iterator& it, const sentinel& s)
        { return _Ops::any_at_end(it.m_its, s.ends()); }
        friend bool operator==(const sentinel& s, const iterator& it)
        { return _Ops::any_at_end(it.m_its, s.ends()); }
        friend bool operator!=(const iterator& it, const sentinel& s)
        { return !_Ops::any_at_end(it.m_its, s.ends()); }
        friend bool operator!=(const sentinel& s, const iterator& it)
        { return !_Ops::any_at_end(it.m_its, s.ends()); }
    };

    zip_view() : m_views() {}
    explicit zip_view(_Views... views)
        : m_views(static_cast<_Views&&>(views)...) {}

    iterator begin() { return iterator(make_begin(_Indices())); }
    sentinel end()   { return sentinel(make_end(_Indices())); }
};

NS_END  // ranges
NS_END

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_RANGES_ZIP_VIEW_
