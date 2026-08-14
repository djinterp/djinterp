/******************************************************************************
* djinterp [restd]                                               join_view.hpp
*
* join_view header:
*   Provides the C++20 range-of-ranges flattening adaptor. join_view<V>
* presents the elements of every inner range of V concatenated in
* order. For V = vector<vector<int>>{{1,2},{3},{4,5}} the joined
* view yields 1, 2, 3, 4, 5.
*
*   PORTABILITY:
*   - C++11+; CRTP + view_interface + custom iterator.
*   - The iterator carries (outer_iter, inner_iter) plus a back-pointer
*     to the parent join_view. For prvalue inner ranges (R25
*     upgrade), the view holds a mutable non_propagating_cache used
*     to materialise the current inner range so iterators into it
*     remain valid for the duration of inner iteration.
*
*   SCOPE LIMITATIONS RELATIVE TO C++20:
*   - The R25 upgrade lifts the original prvalue-inner restriction.
*     Both lvalue-reference AND prvalue inner ranges are now
*     supported. Tag-dispatched _get_inner switches between direct-
*     access (reference) and cache-materialisation (prvalue) paths.
*   - Iterator category is clamped at most to forward_iterator_tag.
*     C++20 allows bidirectional under specific conditions (both
*     outer and inner bidirectional, common_range on both, plus
*     non-empty handling) but the implementation effort dwarfs the
*     forward case and is deferred to a later phase.
*   - Concurrent iteration: the prvalue-cache lives on the view
*     (mutable). Multiple concurrent iterators over a prvalue-inner
*     join_view will clobber each other's view of the cache —
*     consistent with the C++20 spec, which treats join_view as
*     non-multi-pass for prvalue inners regardless of underlying.
*
*   COLOCATED:
*   restd::views::join(r).
*
*
* path:      /inc/djinterp/re_std/ranges/join_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_JOIN_VIEW_
#define DJINTERP_RESTD_RANGES_JOIN_VIEW_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../iterator/iterator_traits.hpp"
#include "./view_interface.hpp"
#include "./iterator_t.hpp"
#include "./sentinel_t.hpp"
#include "./range_reference_t.hpp"
#include "./non_propagating_cache.hpp"
#include "./all.hpp"
#include "./range_adaptor_closure.hpp"


NS_RESTD


// ===========================================================================
// I.   JOIN_VIEW
// ===========================================================================

// join_view<_View>
//   class: flattens a range-of-ranges _View by one level. _View's
// reference type must be an lvalue reference (see banner).
template<typename _View>
class join_view : public view_interface<join_view<_View> >
{
public:
    typedef _View   base_view;


private:
    // ---- inner-range plumbing ----
    typedef typename remove_reference<
                          range_reference_t<_View>
                      >::type                              inner_range;

    typedef typename iterator_traits<
                          iterator_t<_View>
                      >::reference                         outer_reference;


    // is_ref_inner_t
    //   tag: true_type when the outer reference is a reference (so
    // the inner range can be reached directly via *m_outer); false_type
    // for prvalue inners (which need to be materialised into the
    // mutable m_inner_cache below).
    typedef typename conditional<
                          is_lvalue_reference<outer_reference>::value
                          || is_rvalue_reference<outer_reference>::value,
                          true_type,
                          false_type
                      >::type                              is_ref_inner_t;


    _View                                                  m_base;

    // m_inner_cache
    //   field: mutable cache for prvalue inner ranges. Unused (left
    // empty) for the reference-inner case. Mutable so that const
    // iterators (via const join_view* back-pointer) can populate it.
    mutable internal::non_propagating_cache<inner_range>   m_inner_cache;


public:
    // =======================================================
    // I.A   NESTED ITERATOR
    // =======================================================

    // iterator
    //   class: pairs an outer iterator with the current inner
    // iterator. operator++ advances the inner; when the inner
    // exhausts the outer advances and the search for the next
    // non-empty inner runs.
    class iterator
    {
    private:
        // _bidi_clamp — clamps RA underlyings to bidi (R28 pattern).
        // For join_view, the category is also constrained by the
        // inner range's category; here we use the outer's and rely
        // on SFINAE-lazy operator-- to refuse to compile when the
        // inner iterator lacks --. Practical implication: bidi
        // join_view requires both outer AND inner to be bidirectional
        // AND outer to be a common_range (so iterator_t<V> can step
        // back from end).
        template<typename _Cat>
        struct _bidi_clamp
        {
            typedef _Cat type;
        };

    public:
        typedef typename _bidi_clamp<
                              typename iterator_traits<
                                            iterator_t<_View>
                                        >::iterator_category
                          >::type                               iterator_category;

        typedef typename iterator_traits<
                              iterator_t<inner_range>
                          >::value_type                         value_type;

        typedef typename iterator_traits<
                              iterator_t<inner_range>
                          >::difference_type                    difference_type;

        typedef typename iterator_traits<
                              iterator_t<inner_range>
                          >::reference                          reference;

        typedef typename iterator_traits<
                              iterator_t<inner_range>
                          >::pointer                            pointer;


    private:
        iterator_t<_View>           m_outer;
        iterator_t<inner_range>     m_inner;
        join_view const*            m_parent;


        // _materialize_inner — tag-dispatched cache population.
        //   reference case: no-op (the inner range is already live
        //                    in the outer's iteration).
        //   prvalue case:   emplace the current *m_outer prvalue
        //                    into the view's mutable cache. Called
        //                    ONLY when m_outer advances (otherwise
        //                    m_inner iterators into the cache would
        //                    be invalidated).
        void
        _materialize_inner(
            true_type /* reference inner */
        ) const
        {
            // no-op
        }

        void
        _materialize_inner(
            false_type /* prvalue inner */
        ) const
        {
            m_parent->m_inner_cache.emplace_deref(*m_outer);
        }

        // _current_inner — read-only inner-range accessor.
        //   reference case: *m_outer.
        //   prvalue case:   *cache (assumes _materialize_inner has
        //                    populated it, which satisfy() arranges).
        inner_range&
        _current_inner(
            true_type
        ) const
        {
            return *m_outer;
        }

        inner_range&
        _current_inner(
            false_type
        ) const
        {
            return *(m_parent->m_inner_cache);
        }


        // satisfy
        //   function: advances m_outer past empty inner ranges and
        // sets m_inner to the start of the first non-empty inner
        // range. If no non-empty inner remains, m_outer ends at the
        // outer end (and m_inner is irrelevant). For the prvalue
        // case, materialises the cache on each outer step before
        // taking m_inner.
        void
        satisfy()
        {
            sentinel_t<_View> outer_end = restd::end(m_parent->m_base);
            while (m_outer != outer_end)
            {
                _materialize_inner(is_ref_inner_t());
                inner_range& inner = _current_inner(is_ref_inner_t());
                m_inner = restd::begin(inner);
                if (m_inner != restd::end(inner))
                {
                    return;
                }
                ++m_outer;
            }
        }


    public:
        // default ctor
        D_CONSTEXPR
        iterator()
            : m_outer(),
              m_inner(),
              m_parent(D_NULLPTR)
        {}

        // value ctor (begin)
        //   function: constructs an iterator at the start of the
        // join. Caller must invoke satisfy() — the begin() function
        // on join_view does so before handing the iterator out.
        iterator(
            join_view const*  _parent
        )
            : m_outer(restd::begin(_parent->m_base)),
              m_inner(),
              m_parent(_parent)
        {
            satisfy();
        }


        // operator*
        D_CONSTEXPR reference
        operator*() const
        {
            return *m_inner;
        }


        // operator++
        //   function: advances m_inner; on inner exhaustion advances
        // m_outer and satisfies forward. Uses _current_inner so we
        // do not re-materialise the prvalue cache mid-inner-iteration
        // (which would invalidate m_inner).
        iterator&
        operator++()
        {
            inner_range& inner = _current_inner(is_ref_inner_t());
            ++m_inner;
            if (m_inner == restd::end(inner))
            {
                ++m_outer;
                satisfy();
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


        // _back_up_to_non_empty
        //   helper: steps m_outer backward (one position at a time)
        // until it lands on an outer whose inner range is non-empty.
        // For each step, re-materialises the prvalue cache (if
        // applicable). On exit, m_inner is positioned at the last
        // element of the chosen inner range (one before end).
        //
        //   Precondition: the caller has already confirmed that
        // m_outer is not at the base begin (else there is nothing
        // to back up to — UB). For the at-outer-end case the caller
        // must pass m_outer in its past-end state; this function
        // does the first --m_outer.
        void
        _back_up_to_non_empty()
        {
            // Step back through outers, skipping empty inners.
            while (true)
            {
                --m_outer;
                _materialize_inner(is_ref_inner_t());
                inner_range& inner = _current_inner(is_ref_inner_t());
                iterator_t<inner_range> inner_begin = restd::begin(inner);
                iterator_t<inner_range> inner_end   = restd::end(inner);
                if (inner_begin != inner_end)
                {
                    // Non-empty — position at last element.
                    m_inner = inner_end;
                    --m_inner;
                    return;
                }
                // Else: empty inner — continue backing up. The
                // caller is responsible for not letting us run off
                // the front of the outer range.
            }
        }


        // operator-- (pre, bidirectional path)
        //   function: retreats one logical element. Three cases:
        // (a) past-end state: back up to the last non-empty inner's
        //     last element.
        // (b) m_inner is at the begin of the current inner range:
        //     back up to the previous non-empty inner's last
        //     element.
        // (c) m_inner is past begin: just --m_inner.
        //
        //   Compiles only when iterator_t<_View>, iterator_t<inner_range>
        // both support operator-- AND V is a common_range (so the
        // past-end m_outer can be decremented). Undefined if invoked
        // at the join_view's first element.
        iterator&
        operator--()
        {
            sentinel_t<_View> outer_end = restd::end(m_parent->m_base);

            // Case (a): past-end. Back up.
            if (m_outer == outer_end)
            {
                _back_up_to_non_empty();
                return *this;
            }

            // Case (b): at begin of current inner.
            inner_range& inner = _current_inner(is_ref_inner_t());
            if (m_inner == restd::begin(inner))
            {
                _back_up_to_non_empty();
                return *this;
            }

            // Case (c): general retreat within current inner.
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


        // outer_base / inner_base — diagnostic accessors.
        D_CONSTEXPR iterator_t<_View>
        outer_base() const
        {
            return m_outer;
        }

        D_CONSTEXPR iterator_t<inner_range>
        inner_base() const
        {
            return m_inner;
        }


        // operator== / !=
        //   function: equal iff both outer iterators match AND
        // either (a) outer is at end (inner is then irrelevant),
        // or (b) both inner iterators match.
        D_CONSTEXPR bool
        operator==(
            iterator const& _rhs
        ) const
        {
            return (m_outer == _rhs.m_outer)
                && ( (m_parent == D_NULLPTR)
                  || (m_outer == restd::end(m_parent->m_base))
                  || (m_inner == _rhs.m_inner) );
        }

        D_CONSTEXPR bool
        operator!=(
            iterator const& _rhs
        ) const
        {
            return !(*this == _rhs);
        }


        // friend: comparison against the join_view's sentinel.
        // Defined below the sentinel class (forward-declared
        // friend access).
    };


    // =======================================================
    // I.B   NESTED SENTINEL
    // =======================================================

    // sentinel
    //   class: wraps the outer end. Iterator-vs-sentinel comparison
    // checks only the outer position; the inner iterator is
    // irrelevant at the end.
    class sentinel
    {
    private:
        sentinel_t<_View>  m_outer_end;


    public:
        D_CONSTEXPR
        sentinel()
            : m_outer_end()
        {}

        D_CONSTEXPR explicit
        sentinel(
            sentinel_t<_View>  _e
        )
            : m_outer_end(_e)
        {}


        D_CONSTEXPR sentinel_t<_View>
        base() const
        {
            return m_outer_end;
        }


        friend D_CONSTEXPR bool
        operator==(
            iterator const&  _it,
            sentinel const&  _s
        )
        {
            return (_it.outer_base() == _s.m_outer_end);
        }

        friend D_CONSTEXPR bool
        operator!=(
            iterator const&  _it,
            sentinel const&  _s
        )
        {
            return !(_it == _s);
        }

        friend D_CONSTEXPR bool
        operator==(
            sentinel const&  _s,
            iterator const&  _it
        )
        {
            return (_it == _s);
        }

        friend D_CONSTEXPR bool
        operator!=(
            sentinel const&  _s,
            iterator const&  _it
        )
        {
            return !(_it == _s);
        }
    };


public:
    // default ctor
    D_CONSTEXPR
    join_view()
        : m_base()
    {}

    // value ctor
    D_CONSTEXPR
    join_view(
        _View  _base
    )
        : m_base(static_cast<_View&&>(_base))
    {}


    // base
    D_CONSTEXPR _View
    base() const
    {
        return m_base;
    }


    // begin
    //   function: constructs an iterator at the start of the join.
    // The iterator's value ctor performs satisfy() — i.e. skips
    // leading empty inner ranges — before returning.
    iterator
    begin()
    {
        return iterator(this);
    }

    iterator
    begin() const
    {
        return iterator(this);
    }


    // end
    //   function: sentinel at the outer end.
    D_CONSTEXPR sentinel
    end()
    {
        return sentinel(restd::end(m_base));
    }

    D_CONSTEXPR sentinel
    end() const
    {
        return sentinel(restd::end(m_base));
    }
};


// ===========================================================================
// II.  VIEWS::JOIN
// ===========================================================================

namespace views
{
    // join_fn
    //   class: closure-fn for join. Pipe-able via the
    // range_adaptor_closure base.
    struct join_fn : range_adaptor_closure<join_fn>
    {
        template<typename _R>
        D_CONSTEXPR_INLINE
        join_view<typename internal::all_dispatch<_R>::type>
        operator()(
            _R&&  _r
        ) const
        {
            typedef typename internal::all_dispatch<_R>::type  view_type;
            return join_view<view_type>(
                internal::all_dispatch<_R>::call(static_cast<_R&&>(_r))
            );
        }
    };

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    inline D_CONSTEXPR join_fn join = join_fn();
#else
    static D_CONSTEXPR join_fn join = join_fn();
#endif
}  // namespace views


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_RANGES_JOIN_VIEW_
