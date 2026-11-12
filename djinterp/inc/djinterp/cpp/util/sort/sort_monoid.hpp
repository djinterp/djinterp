/*******************************************************************************
* djinterp [cpp]                                                 sort_monoid.hpp
*
*   The merge monoid: where the sort subsystem joins the functional algebra.
* Sorting does not fit the Functor / Monad / Comonad cluster -- it fixes the
* element type, rearranges rather than maps, and must see the whole sequence
* before it yields anything.  It does fit Semigroup and Monoid: two sorted runs
* combine, associatively, by merging, and the empty run is the identity.
* "Sorted runs under merge" is a monoid, and mconcat over a Foldable of
* singleton runs is a merge sort -- bottom-up, written entirely in the
* functional vocabulary.
*
*   DSortedRun<Type, Compare> is the newtype that carries the algebra: a
* value-semantic sequence holding its elements in Compare order, together with
* the Compare that ordered them.  It specializes both protocols against the
* SFINAE-hooked primaries of semigroup.hpp and monoid.hpp,
*
*     - semigroup_traits : combine is a stable merge of the two runs
*     - monoid_traits    : empty is the empty run, the identity for merge
*
* so the rest of the monoid surface works on it unchanged:
*
*     std::vector<DSortedRun<int> > runs =
*         to_singleton_runs(std::vector<int>{ 5, 2, 8, 1, 9, 3 });
*     std::vector<int> ordered = mconcat(runs).data();    // 1 2 3 5 8 9
*     DSortedRun<int>  both    = mappend(run_a, run_b);   // a merge
*
*   The algebra assumes one ordering: every run combined with another should
* share the same Compare semantics, as monoid.hpp's numeric and container
* monoids assume one operation per type.  combine is identity-exact even for a
* stateful Compare -- an empty operand is returned as it is, so its
* default-constructed comparator is never consulted.
*
*   mempty, and so mconcat, needs a Compare that default-constructs: the
* identity is the empty run, and it carries a default-constructed comparator.
* std::less, std::greater and natural<T>() do; the algebra's adapters --
* reversed, by_key, then -- do not, so a fold over runs ordered by one of
* those starts from the first run with mappend rather than from mempty.
*
*   The header requires C++11, as the functional protocols it specializes do;
* before C++11 it is empty.
*
*
* path:      /inc/djinterp/cpp/util/sort/sort_monoid.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.25
*                                                            revised: 2026.11.12
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SORTED RUN                DSortedRun<Type, Compare>
II.   MERGE PRIMITIVE           internal::merge_sorted_helper
III.  ALGEBRA REGISTRATION      semigroup_traits<DSortedRun<...> >,
                                monoid_traits<DSortedRun<...> >
IV.   FACTORIES                 make_sorted_run, adopt_sorted_run,
                                singleton_run, to_singleton_runs
*/

#ifndef DJINTERP_UTIL_SORT_MONOID_HPP
#define DJINTERP_UTIL_SORT_MONOID_HPP 1

// djinterp
#include "../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>                         // std::size_t
#include <functional>                      // std::less
#include <type_traits>                     // std::true_type
#include <utility>                         // std::move
#include <vector>                          // std::vector
// djinterp
#include "../../functional/semigroup.hpp"  // semigroup_traits, mappend
#include "../../functional/monoid.hpp"     // monoid_traits, mempty, mconcat
#include "../../functional/foldable.hpp"   // the Foldable mconcat folds
#include "./merge_sort.hpp"                // merge_sort, which establishes
                                           // a run's order


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                    I.   SORTED RUN                                      ///
///////////////////////////////////////////////////////////////////////////////

// DSortedRun
//   class: a value-semantic sequence of Type held in Compare order -- the
// newtype carrying the merge monoid.  Its invariant is that data() is sorted
// under compare(); every constructor and factory below establishes it.
// Compare defaults to std::less<Type>, ascending, and may be any model of
// is_comparator<Compare, Type>, a composed comparator from the algebra
// (by_key, reversed, then) included.
template<typename Type,
         typename Compare = std::less<Type> >
class DSortedRun
{
public:
    typedef Type              value_type;
    typedef Compare           compare_type;
    typedef std::vector<Type> container_type;

    // DSortedRun
    //   constructor: the empty run, the monoid identity, with a
    // default-constructed comparator.
    DSortedRun()
        : m_data(),
          m_compare()
    {}

    // DSortedRun
    //   constructor: an empty run carrying _compare.
    explicit DSortedRun(
        const Compare& _compare
    )
        : m_data(),
          m_compare(_compare)
    {}

    // DSortedRun
    //   constructor: adopts _sorted_data, which must ALREADY be sorted under
    // _compare.  The trailing bool only tells this zero-cost constructor
    // apart from a sorting one; passing unsorted data breaks the invariant.
    // make_sorted_run sorts and then wraps.
    DSortedRun(
        container_type _sorted_data,
        const Compare& _compare,
        bool           /* _already_sorted */
    )
        : m_data(std::move(_sorted_data)),
          m_compare(_compare)
    {}

    // data
    //   function: the elements, in order.
    D_NODISCARD
    const container_type& data() const
    {
        return m_data;
    }

    // compare
    //   function: the comparator that orders them.
    D_NODISCARD
    const Compare& compare() const
    {
        return m_compare;
    }

    // size
    //   function: the number of elements.
    D_NODISCARD
    std::size_t size() const
    {
        return m_data.size();
    }

    // empty
    //   function: whether this is the identity, the empty run.
    D_NODISCARD
    bool empty() const
    {
        return m_data.empty();
    }

private:
    container_type m_data;
    Compare        m_compare;
};


///////////////////////////////////////////////////////////////////////////////
///                    II.  MERGE PRIMITIVE                                 ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // merge_sorted_helper
    //   function: the stable two-pointer merge of two vectors already ordered
    // by _compare, into one ordered vector.
    //
    //   An element of _b is taken first only when it STRICTLY precedes the
    // element of _a it is compared with; on equivalence _a's goes first.  That
    // keeps equivalent elements in a-then-b order, which is what makes the
    // merge -- and every mconcat built on it -- stable.  It is the rule
    // merge_runs follows in merge_sort_common.hpp, for the same reason.
    template<typename Type,
             typename Compare>
    std::vector<Type> merge_sorted_helper(
        const std::vector<Type>& _a,
        const std::vector<Type>& _b,
        const Compare&           _compare
    )
    {
        typedef typename std::vector<Type>::size_type size_type;

        std::vector<Type> result;
        size_type         i;
        size_type         j;

        result.reserve(_a.size() + _b.size());

        i = 0;
        j = 0;

        // interleave while both have elements left
        while ( (i < _a.size()) &&
                (j < _b.size()) )
        {
            if (_compare(_b[j], _a[i]))
            {
                result.push_back(_b[j]);

                ++j;
            }
            else
            {
                result.push_back(_a[i]);

                ++i;
            }
        }

        // at most one of the two tails is non-empty
        result.insert(result.end(), _a.begin() + static_cast<
                          typename std::vector<Type>::difference_type>(i),
                      _a.end());
        result.insert(result.end(), _b.begin() + static_cast<
                          typename std::vector<Type>::difference_type>(j),
                      _b.end());

        return result;
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                    III. ALGEBRA REGISTRATION                            ///
///////////////////////////////////////////////////////////////////////////////
//   Both protocols are specialized in the explicit two-argument <T, void>
// form against the SFINAE-hooked primaries of semigroup.hpp and monoid.hpp,
// as every instance in monoid.hpp is written.  Neither operation is
// D_CONSTEXPR: a run holds a std::vector, so -- like monoid.hpp's std::string
// and std::vector instances -- they run at run time.

// semigroup_traits<DSortedRun<Type, Compare> >
//   instance: the associative combine is a stable merge.  An empty operand is
// returned as it is, so the identity laws hold exactly and a stateful
// comparator on the empty side is never consulted; otherwise the runs are
// merged under the left operand's comparator and the result adopted without
// re-sorting.
template<typename Type,
         typename Compare>
struct semigroup_traits<DSortedRun<Type, Compare>, void>
{
    using is_specialized = std::true_type;

    static
    DSortedRun<Type, Compare> combine(
        const DSortedRun<Type, Compare>& _a,
        const DSortedRun<Type, Compare>& _b
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

        return DSortedRun<Type, Compare>(
            internal::merge_sorted_helper(_a.data(),
                                          _b.data(),
                                          _a.compare()),
            _a.compare(),
            true);
    }
};

// monoid_traits<DSortedRun<Type, Compare> >
//   instance: the empty run is the identity for merge.
template<typename Type,
         typename Compare>
struct monoid_traits<DSortedRun<Type, Compare>, void>
{
    using is_specialized = std::true_type;

    static
    DSortedRun<Type, Compare> empty()
    {
        return DSortedRun<Type, Compare>();
    }
};


///////////////////////////////////////////////////////////////////////////////
///                    IV.  FACTORIES                                       ///
///////////////////////////////////////////////////////////////////////////////

// make_sorted_run
//   function: sorts _data under _compare with merge_sort, which is stable,
// and wraps the result.  The sorting factory; adopt_sorted_run is the
// zero-cost one for data already in order.
template<typename Type,
         typename Compare>
D_NODISCARD
DSortedRun<Type, Compare> make_sorted_run(
    std::vector<Type> _data,
    Compare           _compare
)
{
    ::djinterp::merge_sort(_data.begin(),
                           _data.end(),
                           _compare);

    return DSortedRun<Type, Compare>(std::move(_data),
                                     _compare,
                                     true);
}

// make_sorted_run
//   function: make_sorted_run under std::less<Type>, ascending.
template<typename Type>
D_NODISCARD
DSortedRun<Type> make_sorted_run(
    std::vector<Type> _data
)
{
    return make_sorted_run(std::move(_data),
                           std::less<Type>());
}

// adopt_sorted_run
//   function: wraps _data, which must ALREADY be ordered by _compare, without
// sorting it.
template<typename Type,
         typename Compare>
D_NODISCARD
DSortedRun<Type, Compare> adopt_sorted_run(
    std::vector<Type> _data,
    Compare           _compare
)
{
    return DSortedRun<Type, Compare>(std::move(_data),
                                     _compare,
                                     true);
}

// singleton_run
//   function: a run of one element, sorted without a comparison -- the atom of
// the bottom-up merge sort that to_singleton_runs and mconcat spell out.
template<typename Type,
         typename Compare>
D_NODISCARD
DSortedRun<Type, Compare> singleton_run(
    const Type&    _value,
    const Compare& _compare
)
{
    return DSortedRun<Type, Compare>(std::vector<Type>(1, _value),
                                     _compare,
                                     true);
}

// singleton_run
//   function: singleton_run under std::less<Type>, ascending.
template<typename Type>
D_NODISCARD
DSortedRun<Type> singleton_run(
    const Type& _value
)
{
    return singleton_run(_value,
                         std::less<Type>());
}

// to_singleton_runs
//   function: lifts every element of _values into its own singleton run,
// giving a Foldable of runs for mconcat -- which merges them back into one
// ordered run:
//
//     std::vector<int> ordered = mconcat(to_singleton_runs(values)).data();
template<typename Type,
         typename Compare>
D_NODISCARD
std::vector<DSortedRun<Type, Compare> > to_singleton_runs(
    const std::vector<Type>& _values,
    const Compare&           _compare
)
{
    std::vector<DSortedRun<Type, Compare> > runs;
    std::size_t                             index;

    runs.reserve(_values.size());

    for (index = 0; index < _values.size(); ++index)
    {
        runs.push_back(singleton_run(_values[index],
                                     _compare));
    }

    return runs;
}

// to_singleton_runs
//   function: to_singleton_runs under std::less<Type>, ascending.
template<typename Type>
D_NODISCARD
std::vector<DSortedRun<Type> > to_singleton_runs(
    const std::vector<Type>& _values
)
{
    return to_singleton_runs(_values,
                             std::less<Type>());
}


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_UTIL_SORT_MONOID_HPP
