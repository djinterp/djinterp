/*******************************************************************************
* djinterp [re_std]                                           join_with_view.hpp
*
* join_with_view header:
*   Provides the C++23 separator-join adaptor. join_with_view<V, S>
* flattens a range-of-ranges V, inserting a separator value S
* between consecutive inner ranges (but not before the first or
* after the last). For V = {{1,2},{3},{4,5}} and S = 0 the result
* is {1, 2, 0, 3, 0, 4, 5}.
*
*   SCOPE LIMITATION RELATIVE TO C++23:
*   The C++23 std::ranges::join_with_view accepts a separator that
* is either a single value OR a range of values. Re_std ships only
* the single-value separator (most common case: comma, space,
* newline, etc.). The range-separator form requires interleaving
* iteration through TWO range types and roughly doubles iterator
* state.
*
*   PORTABILITY:
*   - C++11+; CRTP + view_interface + custom iterator + sentinel.
*   - Same lvalue-reference inner constraint as join_view (no
*     non-propagating-cache for prvalue inners).
*   - Forward-iterator-strength only.
*   - The separator is stored by value; the iterator alternates
*     between "iterating an inner range" and "yielding the separator"
*     via an internal state flag.
*
*   COLOCATED:
*   re_std::views::join_with(r, sep) — direct form.
*   re_std::views::join_with(sep)    — bound form for pipe syntax.
*
*
* path:      /inc/re_std/ranges/join_with_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_RANGES_JOIN_WITH_VIEW_HPP
#define RE_STD_RANGES_JOIN_WITH_VIEW_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../type_traits/common_type.hpp"   // common_type
#include "../iterator/iterator_traits.hpp"
#include "./view_interface.hpp"
#include "./iterator_t.hpp"
#include "./sentinel_t.hpp"
#include "./range_reference_t.hpp"
#include "./all.hpp"
#include "./range_adaptor_closure.hpp"


namespace re_std
{


// ===========================================================================
// I.   JOIN_WITH_VIEW
// ===========================================================================

// join_with_view<View, Sep>
//   class: flattens a range-of-ranges, inserting Sep between each
// pair of consecutive inner ranges.
template<typename View,
         typename Sep>
class join_with_view : public view_interface<join_with_view<View, Sep> >
{
public:
    typedef View   base_view;
    typedef Sep    separator_type;


private:
    typedef typename remove_reference<
                          range_reference_t<View>
                      >::type                              inner_range;

    typedef typename iterator_traits<
                          iterator_t<View>
                      >::reference                         outer_reference;

    static_assert(
        is_lvalue_reference<outer_reference>::value
        || is_rvalue_reference<outer_reference>::value,
        "re_std::join_with_view requires the outer range's reference "
        "type to be a reference; prvalue inner ranges need a "
        "non-propagating cache, not yet shipped in re_std."
    );


    View   m_base;
    Sep    m_sep;


public:
    // =======================================================
    // I.A   NESTED ITERATOR
    // =======================================================

    // iterator
    //   class: alternates between yielding inner-range elements and
    // yielding the separator. State 'in_sep' toggles. Outer
    // advancement on inner-exhaustion plus the toggle implements
    // the C++23 interleave semantics.
    class iterator
    {
    private:
        // _bidi_clamp — clamps RA to bidi (R28 pattern).
        template<typename Cat>
        struct _bidi_clamp
        {
            typedef Cat type;
        };

    public:
        typedef typename _bidi_clamp<
                              typename iterator_traits<
                                            iterator_t<View>
                                        >::iterator_category
                          >::type                              iterator_category;
        typedef typename common_type<
                              typename iterator_traits<
                                            iterator_t<inner_range>
                                        >::value_type,
                              Sep
                          >::type                              value_type;
        typedef typename iterator_traits<
                              iterator_t<inner_range>
                          >::difference_type                   difference_type;
        typedef value_type                                     reference;
        typedef void                                           pointer;


    private:
        iterator_t<View>           m_outer;
        iterator_t<inner_range>     m_inner;
        bool                        m_in_sep;
        join_with_view const*       m_parent;


        // satisfy_outer
        //   function: after advancing m_outer, skip empty inner
        // ranges; sets m_inner to the start of the next non-empty
        // inner.
        void
        satisfy_outer()
        {
            sentinel_t<View> outer_end = re_std::end(m_parent->m_base);
            while (m_outer != outer_end)
            {
                inner_range& inner = *m_outer;
                m_inner = re_std::begin(inner);
                if (m_inner != re_std::end(inner))
                {
                    return;
                }
                ++m_outer;
            }
        }


        // _back_up_to_inner_last
        //   function: steps m_outer backward, skipping empty inners;
        // on the first non-empty inner sets m_inner to the position
        // just before its end (i.e. its last element). Used by
        // operator-- in the past-end / in_sep / inner-at-begin
        // transitions. Precondition: there exists at least one
        // non-empty inner before m_outer.
        void
        _back_up_to_inner_last()
        {
            while (true)
            {
                --m_outer;
                inner_range& inner = *m_outer;
                iterator_t<inner_range> inner_begin = re_std::begin(inner);
                iterator_t<inner_range> inner_end   = re_std::end(inner);
                if (inner_begin != inner_end)
                {
                    m_inner = inner_end;
                    --m_inner;
                    return;
                }
            }
        }


    public:
        RE_STD_CONSTEXPR
        iterator()
            : m_outer(),
              m_inner(),
              m_in_sep(false),
              m_parent(RE_STD_NULLPTR)
        {}

        iterator(
            join_with_view const*  _parent
        )
            : m_outer(re_std::begin(_parent->m_base)),
              m_inner(),
              m_in_sep(false),
              m_parent(_parent)
        {
            satisfy_outer();
        }


        RE_STD_CONSTEXPR iterator_t<View>
        outer_base() const
        {
            return m_outer;
        }


        // operator*
        //   function: yields the separator (by copy) when in
        // separator state; yields *m_inner otherwise.
        reference
        operator*() const
        {
            if (m_in_sep)
            {
                return reference(m_parent->m_sep);
            }
            return reference(*m_inner);
        }


        // operator++
        //   function: R28 — restructured so the canonical state
        // for in_sep has m_outer already at the next non-empty
        // inner (rather than at a possibly-empty outer that will
        // be skipped on the following ++). This makes forward and
        // backward iteration converge on the same iterator state
        // at every separator yield, enabling proper bidi equality.
        //
        //   - in_sep state: clear flag. State is already at the
        //     beginning of the next non-empty inner (set by the
        //     previous ++ via satisfy_outer).
        //   - non-sep state, inner not exhausted: ++m_inner.
        //   - non-sep state, inner exhausted: ++m_outer +
        //     satisfy_outer to skip empty inners; if any non-empty
        //     remains, enter in_sep state.
        iterator&
        operator++()
        {
            sentinel_t<View> outer_end = re_std::end(m_parent->m_base);
            if (m_in_sep)
            {
                m_in_sep = false;
                // State (m_outer / m_inner) already at the next
                // inner's begin — set by the previous ++ pass.
            }
            else
            {
                inner_range& inner = *m_outer;
                ++m_inner;
                if (m_inner == re_std::end(inner))
                {
                    ++m_outer;
                    satisfy_outer();  // skip empties; positions m_inner if found
                    if (m_outer != outer_end)
                    {
                        m_in_sep = true;
                    }
                    // else: done; m_outer == outer_end.
                }
            }
            return *this;
        }

        iterator
        operator++(int)
        {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }


        // operator-- (pre, bidirectional path)
        //   function: three cases.
        //   (a) past-end: back up through outers (skipping empties)
        //       to last non-empty inner's last element.
        //   (b) in_sep: previous element is the last element of the
        //       previous non-empty inner. Back up + clear flag.
        //   (c) non-sep, m_inner at begin: previous element is
        //       separator. Just flip in_sep flag (m_outer stays).
        //   (d) non-sep, m_inner past begin: --m_inner.
        //
        //   Compiles only when both outer and inner iterators
        // support operator-- AND V is a common_range.
        iterator&
        operator--()
        {
            sentinel_t<View> outer_end = re_std::end(m_parent->m_base);

            if (m_outer == outer_end)
            {
                // (a) past-end
                _back_up_to_inner_last();
                m_in_sep = false;
                return *this;
            }

            if (m_in_sep)
            {
                // (b) was at separator
                m_in_sep = false;
                _back_up_to_inner_last();
                return *this;
            }

            inner_range& inner = *m_outer;
            if (m_inner == re_std::begin(inner))
            {
                // (c) previous element is separator
                m_in_sep = true;
                return *this;
            }

            // (d) general retreat
            --m_inner;
            return *this;
        }

        iterator
        operator--(int)
        {
            iterator tmp = *this;
            --(*this);
            return tmp;
        }


        RE_STD_CONSTEXPR bool
        operator==(
            iterator const& _rhs
        ) const
        {
            return (m_outer == _rhs.m_outer)
                && ( (m_parent == RE_STD_NULLPTR)
                  || (m_outer == re_std::end(m_parent->m_base))
                  || (m_in_sep == _rhs.m_in_sep
                      && m_inner == _rhs.m_inner) );
        }

        RE_STD_CONSTEXPR bool
        operator!=(
            iterator const& _rhs
        ) const
        {
            return !(*this == _rhs);
        }
    };


    // =======================================================
    // I.B   NESTED SENTINEL
    // =======================================================

    class sentinel
    {
    private:
        sentinel_t<View>  m_outer_end;


    public:
        RE_STD_CONSTEXPR
        sentinel()
            : m_outer_end()
        {}

        RE_STD_CONSTEXPR explicit
        sentinel(
            sentinel_t<View>  _e
        )
            : m_outer_end(_e)
        {}


        friend RE_STD_CONSTEXPR bool
        operator==(
            iterator const&  _it,
            sentinel const&  _s
        )
        {
            return (_it.outer_base() == _s.m_outer_end);
        }

        friend RE_STD_CONSTEXPR bool
        operator!=(
            iterator const&  _it,
            sentinel const&  _s
        )
        {
            return !(_it == _s);
        }

        friend RE_STD_CONSTEXPR bool
        operator==(
            sentinel const&  _s,
            iterator const&  _it
        )
        {
            return (_it == _s);
        }

        friend RE_STD_CONSTEXPR bool
        operator!=(
            sentinel const&  _s,
            iterator const&  _it
        )
        {
            return !(_it == _s);
        }
    };


public:
    RE_STD_CONSTEXPR
    join_with_view()
        : m_base(),
          m_sep()
    {}

    RE_STD_CONSTEXPR
    join_with_view(
        View  _base,
        Sep   _sep
    )
        : m_base(static_cast<View&&>(_base)),
          m_sep(static_cast<Sep&&>(_sep))
    {}


    RE_STD_CONSTEXPR View
    base() const
    {
        return m_base;
    }

    RE_STD_CONSTEXPR Sep const&
    sep() const
    RE_STD_NOEXCEPT
    {
        return m_sep;
    }


    iterator
    begin() const
    {
        return iterator(this);
    }

    RE_STD_CONSTEXPR sentinel
    end() const
    {
        return sentinel(re_std::end(m_base));
    }
};


// ===========================================================================
// II.  JOIN_WITH_CLOSURE (bound form for pipe syntax)
// ===========================================================================

namespace internal
{

template<typename Sep>
struct join_with_closure : range_adaptor_closure<join_with_closure<Sep> >
{
    Sep sep;

    RE_STD_CONSTEXPR
    join_with_closure()
        : sep()
    {}

    RE_STD_CONSTEXPR explicit
    join_with_closure(
        Sep _s
    )
        : sep(static_cast<Sep&&>(_s))
    {}

    template<typename R>
    RE_STD_CONSTEXPR_INLINE
    join_with_view<typename internal::all_dispatch<R>::type, Sep>
    operator()(
        R&&  _r
    ) const
    {
        typedef typename internal::all_dispatch<R>::type view_type;
        return join_with_view<view_type, Sep>(
            internal::all_dispatch<R>::call(static_cast<R&&>(_r)),
            sep
        );
    }
};

}  // internal


// ===========================================================================
// III. VIEWS::JOIN_WITH
// ===========================================================================

namespace views
{
    // views::join_with(_r, _sep)  [direct form]
    template<typename R,
             typename Sep>
    RE_STD_CONSTEXPR_INLINE
    join_with_view<typename internal::all_dispatch<R>::type,
                   typename decay<Sep>::type>
    join_with(
        R&&    _r,
        Sep&&  _sep
    )
    {
        typedef typename internal::all_dispatch<R>::type  view_type;
        typedef typename decay<Sep>::type                 sep_type;
        return join_with_view<view_type, sep_type>(
            internal::all_dispatch<R>::call(static_cast<R&&>(_r)),
            static_cast<Sep&&>(_sep)
        );
    }

    // views::join_with(_sep)  [bound form]
    template<typename Sep>
    RE_STD_CONSTEXPR_INLINE
    internal::join_with_closure<typename decay<Sep>::type>
    join_with(
        Sep&&  _sep
    )
    {
        return internal::join_with_closure<typename decay<Sep>::type>(
            static_cast<Sep&&>(_sep)
        );
    }
}  // namespace views


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RANGES_JOIN_WITH_VIEW_HPP
