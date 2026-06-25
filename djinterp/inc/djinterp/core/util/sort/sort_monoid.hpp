/******************************************************************************
* djinterp [utility]                                           sort_monoid.hpp
*
* The merge monoid: where the sort subsystem joins the functional algebra.
*   Sorting resists the Functor -> Monad -> Comonad cluster (it fixes the
* element type and rearranges rather than maps, and it is a strict whole-
* sequence barrier).  Where it does belong is Semigroup / Monoid: two already-
* sorted runs combine, associatively, by merging, and the empty run is the
* identity.  "Sorted runs under merge" is exactly a monoid, and mconcat over a
* Foldable of singleton runs IS a merge sort -- bottom-up, expressed entirely
* through the functional vocabulary.
*
*   sorted_run<T, Compare> is the newtype that carries the algebra: a value-
* semantic sequence holding its elements in Compare order together with the
* Compare used to order them.  It specializes BOTH protocols against the
* SFINAE-hooked primaries from semigroup.hpp / monoid.hpp:
*
*     - semigroup_traits : combine == a stable merge of the two runs
*     - monoid_traits    : empty   == the empty run (identity for merge)
*
* so the whole monoid surface lights up for free:
*
*     using namespace djinterp;
*     std::vector<sorted_run<int> > runs = to_singleton_runs(
*         std::vector<int>{ 5, 2, 8, 1, 9, 3 });
*     std::vector<int> ordered = mconcat(runs).data();   // {1,2,3,5,8,9}
*
*     // two prepared runs merge with the plain semigroup operation:
*     sorted_run<int> ab = mappend(run_a, run_b);
*
*   The algebra assumes a consistent ordering: every run combined together
* should share the same Compare semantics (as the numeric / container monoids
* in monoid.hpp assume a single operation per type).  combine is identity-
* exact even for a stateful Compare -- an empty operand is returned as-is, so
* its (default-constructed) comparator is never consulted.
*
*
* path:      /inc/djinterp/core/util/sort/sort_monoid.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.06.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SORTED RUN NEWTYPE
      1.  sorted_run<T, Compare>                  (value-semantic sorted seq)
II.   MERGE PRIMITIVE
      1.  merge_sorted                            (stable two-run merge)
III.  ALGEBRA REGISTRATION
      1.  semigroup_traits<sorted_run<...>>       (combine == merge)
      2.  monoid_traits<sorted_run<...>>          (empty   == empty run)
IV.   FACTORIES
      1.  make_sorted_run / adopt_sorted_run
      2.  singleton_run / to_singleton_runs
*/


#ifndef DJINTERP_UTILITY_SORT_MONOID_
#define DJINTERP_UTILITY_SORT_MONOID_ 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"
#include "./sort_common.hpp"               // default_comparator_t, comparator algebra
#include "./merge_sort.hpp"               // merge_sort, to establish the run invariant
#include "../../functional/semigroup.hpp"  // semigroup_traits, mappend
#include "../../functional/monoid.hpp"     // monoid_traits, mempty, mconcat
#include "../../functional/foldable.hpp"   // (mconcat folds a Foldable of runs)


//   The whole header is a no-op before C++11: it depends on the functional
// protocols (alias templates, decltype, scoped traits) which themselves
// require C++11 or later.
#if D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    SORTED RUN NEWTYPE                                    ///
///////////////////////////////////////////////////////////////////////////////

// sorted_run
//   struct: a value-semantic sequence of _Type held in _Compare order, the
// newtype carrying the merge monoid.  The class invariant is that data() is
// sorted with respect to compare(); every constructor and factory below
// establishes it.  _Compare defaults to the subsystem's default_comparator_t
// (ascending), but may be any model of is_comparator<_Compare, _Type>,
// including a composed comparator from the algebra (by_key, reversed, then).
template<typename _Type,
         typename _Compare = default_comparator_t<_Type> >
class sorted_run
{
public:
    typedef _Type                 value_type;
    typedef _Compare              compare_type;
    typedef std::vector<_Type>    container_type;

    // sorted_run (empty)
    //   ctor: the empty run -- the monoid identity.  Default-constructs the
    // comparator.
    sorted_run()
        : m_data(),
          m_compare()
    {
    }

    // sorted_run (empty, with comparator)
    //   ctor: an empty run carrying an explicit comparator.
    explicit sorted_run(
        const _Compare& _compare
    )
        : m_data(),
          m_compare(_compare)
    {
    }

    // sorted_run (adopt)
    //   ctor: wraps a vector that is ALREADY sorted with respect to _compare.
    // The boolean tag disambiguates this zero-cost adopting constructor from a
    // sorting factory; passing unsorted data here breaks the invariant.  Use
    // make_sorted_run to sort-then-wrap.
    sorted_run(
        container_type  _sorted_data,
        const _Compare& _compare,
        bool            /*_already_sorted*/
    )
        : m_data(static_cast<container_type&&>(_sorted_data)),
          m_compare(_compare)
    {
    }

    // data
    //   accessor: the ordered elements.
    D_NODISCARD
    const container_type& data() const
    {
        return m_data;
    }

    // compare
    //   accessor: the comparator establishing the order.
    D_NODISCARD
    const _Compare& compare() const
    {
        return m_compare;
    }

    // size
    //   accessor: element count.
    D_NODISCARD
    std::size_t size() const
    {
        return m_data.size();
    }

    // empty
    //   accessor: whether this is the identity (empty) run.
    D_NODISCARD
    bool empty() const
    {
        return m_data.empty();
    }

private:
    container_type m_data;
    _Compare       m_compare;
};


///////////////////////////////////////////////////////////////////////////////
///             II.   MERGE PRIMITIVE                                       ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // merge_sorted_helper
    //   helper: the classic stable two-pointer merge of two ranges already
    // ordered by _compare, into one ordered vector.  Stability: when neither
    // element strictly precedes the other (an equivalence), the element from
    // _a is taken first, so the merge never reorders equivalents and is a
    // strict-weak-ordering-stable combine.
    template<typename _Type,
             typename _Compare>
    std::vector<_Type> merge_sorted_helper(
        const std::vector<_Type>& _a,
        const std::vector<_Type>& _b,
        const _Compare&           _compare
    )
    {
        std::vector<_Type> _result;
        _result.reserve(_a.size() + _b.size());

        typename std::vector<_Type>::size_type _i = 0;
        typename std::vector<_Type>::size_type _j = 0;

        while (_i < _a.size() && _j < _b.size())
        {
            // take from _b only when _b[j] strictly precedes _a[i]; otherwise
            // take from _a -- keeps equivalents in a-before-b order (stable).
            if (_compare(_b[_j], _a[_i]))
            {
                _result.push_back(_b[_j]);
                ++_j;
            }
            else
            {
                _result.push_back(_a[_i]);
                ++_i;
            }
        }

        while (_i < _a.size())
        {
            _result.push_back(_a[_i]);
            ++_i;
        }

        while (_j < _b.size())
        {
            _result.push_back(_b[_j]);
            ++_j;
        }

        return _result;
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             III.  ALGEBRA REGISTRATION                                  ///
///////////////////////////////////////////////////////////////////////////////
//   Both protocols are specialized in the explicit two-argument `<T, void>`
// form against the SFINAE-hooked primaries in semigroup.hpp / monoid.hpp, the
// same way every instance in monoid.hpp is written.  combine and empty are not
// marked D_CONSTEXPR: a run holds a std::vector, so -- exactly like the
// std::string / std::vector instances in monoid.hpp -- the operations run at
// run time.

// semigroup_traits<sorted_run<_Type, _Compare>>
//   instance: the associative combine is a stable merge.  An empty operand is
// returned as-is (so the identity laws hold exactly, and a stateful comparator
// on the empty side is never consulted); otherwise the two runs are merged
// under the left operand's comparator, and the merged-and-still-sorted vector
// is adopted without re-sorting.
template<typename _Type,
         typename _Compare>
struct semigroup_traits<sorted_run<_Type, _Compare>, void>
{
    using is_specialized = std::true_type;

    static
    sorted_run<_Type, _Compare> combine(
        const sorted_run<_Type, _Compare>& _a,
        const sorted_run<_Type, _Compare>& _b
    )
    {
        if (_a.empty())
        {
            return _b;
        }

        if (_b.empty())
        {
            return _a;
        }

        return sorted_run<_Type, _Compare>(
            internal::merge_sorted_helper(_a.data(), _b.data(), _a.compare()),
            _a.compare(),
            true /* already sorted by the merge */);
    }
};

// monoid_traits<sorted_run<_Type, _Compare>>
//   instance: the empty run is the identity for merge.
template<typename _Type,
         typename _Compare>
struct monoid_traits<sorted_run<_Type, _Compare>, void>
{
    using is_specialized = std::true_type;

    static
    sorted_run<_Type, _Compare> empty()
    {
        return sorted_run<_Type, _Compare>();
    }
};


///////////////////////////////////////////////////////////////////////////////
///             IV.   FACTORIES                                             ///
///////////////////////////////////////////////////////////////////////////////

// make_sorted_run
//   function: builds a run from an arbitrary (unsorted) vector by ordering it
// with the sort subsystem (merge_sort) under _compare, then wrapping it.  This
// is the sorting entry point; adopt_sorted_run is the zero-cost path when the
// data is known to be ordered already.
template<typename _Type,
         typename _Compare>
D_NODISCARD
sorted_run<_Type, _Compare> make_sorted_run(
    std::vector<_Type> _data,
    _Compare           _compare
)
{
    ::djinterp::merge_sort(_data.begin(), _data.end(), _compare);

    return sorted_run<_Type, _Compare>(
        static_cast<std::vector<_Type>&&>(_data), _compare, true);
}

// make_sorted_run (default comparator)
//   function: make_sorted_run with the subsystem's default ascending order.
template<typename _Type>
D_NODISCARD
sorted_run<_Type, default_comparator_t<_Type> > make_sorted_run(
    std::vector<_Type> _data
)
{
    return make_sorted_run(
        static_cast<std::vector<_Type>&&>(_data),
        default_comparator_t<_Type>());
}

// adopt_sorted_run
//   function: wraps a vector that is ALREADY ordered by _compare, with no
// sort.  Precondition: _data is sorted with respect to _compare.
template<typename _Type,
         typename _Compare>
D_NODISCARD
sorted_run<_Type, _Compare> adopt_sorted_run(
    std::vector<_Type> _data,
    _Compare           _compare
)
{
    return sorted_run<_Type, _Compare>(
        static_cast<std::vector<_Type>&&>(_data), _compare, true);
}

// singleton_run
//   function: a run of one element -- trivially sorted, no comparison needed.
// The atom of the bottom-up merge sort: to_singleton_runs lifts a sequence into
// these, and mconcat merges them back into one ordered run.
template<typename _Type,
         typename _Compare>
D_NODISCARD
sorted_run<_Type, _Compare> singleton_run(
    const _Type&    _value,
    const _Compare& _compare
)
{
    std::vector<_Type> _one;
    _one.push_back(_value);

    return sorted_run<_Type, _Compare>(
        static_cast<std::vector<_Type>&&>(_one), _compare, true);
}

// singleton_run (default comparator)
//   function: singleton_run with the subsystem's default ascending order.
template<typename _Type>
D_NODISCARD
sorted_run<_Type, default_comparator_t<_Type> > singleton_run(
    const _Type& _value
)
{
    return singleton_run(_value, default_comparator_t<_Type>());
}

// to_singleton_runs
//   function: lifts every element of a vector into its own singleton run,
// yielding a Foldable (std::vector) of runs ready for mconcat.  mconcat over
// the result is a bottom-up merge sort expressed through the monoid:
//
//     std::vector<int> ordered = mconcat(to_singleton_runs(unsorted)).data();
template<typename _Type,
         typename _Compare>
D_NODISCARD
std::vector<sorted_run<_Type, _Compare> > to_singleton_runs(
    const std::vector<_Type>& _values,
    const _Compare&           _compare
)
{
    std::vector<sorted_run<_Type, _Compare> > _runs;
    _runs.reserve(_values.size());

    for (typename std::vector<_Type>::size_type _k = 0;
         _k < _values.size();
         ++_k)
    {
        _runs.push_back(singleton_run(_values[_k], _compare));
    }

    return _runs;
}

// to_singleton_runs (default comparator)
//   function: to_singleton_runs with the subsystem's default ascending order.
template<typename _Type>
D_NODISCARD
std::vector<sorted_run<_Type, default_comparator_t<_Type> > >
to_singleton_runs(
    const std::vector<_Type>& _values
)
{
    return to_singleton_runs(_values, default_comparator_t<_Type>());
}


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_UTILITY_SORT_MONOID_
