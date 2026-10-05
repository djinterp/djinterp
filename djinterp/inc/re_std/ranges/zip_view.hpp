/*******************************************************************************
* djinterp [re_std]                                                 zip_view.hpp
*
* zip_view view header:
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
*
* path:      /inc/re_std/ranges/zip_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_RANGES_ZIP_VIEW_HPP
#define RE_STD_RANGES_ZIP_VIEW_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "../utility/make_integer_sequence.hpp"
#include "../tuple/tuple.hpp"
#include "../tuple/tuple_get.hpp"
#include "./view_interface.hpp"
#include "../iterator/input_iterator_tag.hpp"
#include "./ranges_access.hpp"
#include "./iterator_t.hpp"
#include "./range_reference_t.hpp"
#include "./sentinel_t.hpp"

namespace re_std
{
namespace ranges
{

namespace internal
{

    // zip_ops
    //   struct: lockstep operations over a tuple of component iterators.
    // Recursive rather than a fold expression, which would need C++17.
    template<size_t Index, size_t Count>
    struct zip_ops
    {
        template<typename Its, typename Sents>
        static bool any_at_end(const Its& its, const Sents& sents)
        {
            //   ANY, not all - see the header note.
            return (re_std::get<Index>(its) == re_std::get<Index>(sents))
                || zip_ops<Index + 1, Count>::any_at_end(its, sents);
        }

        template<typename Its>
        static void advance(Its& its)
        {
            ++re_std::get<Index>(its);
            zip_ops<Index + 1, Count>::advance(its);
        }
    };

    template<size_t Count>
    struct zip_ops<Count, Count>
    {
        template<typename Its, typename Sents>
        static bool any_at_end(const Its&, const Sents&) { return false; }

        template<typename Its>
        static void advance(Its&) { return; }
    };

}  // internal


// zip_view
//   class: N ranges walked in lockstep.
template<typename... Views>
class zip_view : public view_interface<zip_view<Views...> >
{
    typedef tuple<iterator_t<Views>...> _IterTuple;
    typedef tuple<sentinel_t<Views>...> _SentTuple;
    typedef internal::zip_ops<0, sizeof...(Views)> _Ops;
    typedef make_index_sequence<sizeof...(Views)>  _Indices;

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
        explicit sentinel(const _SentTuple& ends) : m_ends(ends) {}
        const _SentTuple& ends() const { return m_ends; }
    };

    class iterator
    {
        _IterTuple m_its;

        template<size_t... I>
        tuple<range_reference_t<Views>...> deref(index_sequence<I...>) const
        { return tuple<range_reference_t<Views>...>(*re_std::get<I>(m_its)...); }

    public:
        //   value_type and reference DIFFER - tuple of values versus tuple of
        // references. Declaring both is what lets algorithms copy an element
        // out while still writing through the iterator.
        typedef tuple<typename remove_reference<
                    range_reference_t<Views> >::type...> value_type;
        typedef tuple<range_reference_t<Views>...>       reference;
        typedef ptrdiff_t                                 difference_type;
        typedef void                                      pointer;
        typedef input_iterator_tag                        iterator_category;

        iterator() : m_its() {}
        explicit iterator(const _IterTuple& its) : m_its(its) {}

        const _IterTuple& iters() const { return m_its; }

        reference operator*() const
        { return deref(make_index_sequence<sizeof...(Views)>()); }

        iterator& operator++() { _Ops::advance(m_its); return *this; }
        iterator  operator++(int) { iterator t = *this; ++(*this); return t; }

        //   HIDDEN FRIENDS, not namespace-scope templates. A non-member
        // template taking `typename zip_view<Views...>::iterator` puts
        // Views in a NON-DEDUCED context, so it can never be called - the
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
    explicit zip_view(Views... views)
        : m_views(static_cast<Views&&>(views)...) {}

    iterator begin() { return iterator(make_begin(_Indices())); }
    sentinel end()   { return sentinel(make_end(_Indices())); }
};

}  // ranges
}

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_RANGES_ZIP_VIEW_HPP
