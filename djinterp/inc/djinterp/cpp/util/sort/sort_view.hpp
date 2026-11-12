/*******************************************************************************
* djinterp [cpp]                                                   sort_view.hpp
*
*   The dataflow bridge: sort as a terminal of the lazy view pipeline.
* view.hpp builds lazy pipelines with operator| -- adapters (transform,
* filter, take, ...) compose without evaluating, and a terminal (to_vector,
* count, fold) drains the pipeline into a concrete value.  Sorting cannot be a
* lazy adapter: it must see every element before it can yield the first.  It
* belongs exactly where the laziness ends, as a TERMINAL.  This header adds
* sort terminals that drain a view -- or a container, which view.hpp lifts to
* a view -- into a std::vector ordered by the chosen comparator and algorithm:
*
*     std::vector<person> by_age =
*         people
*         | transform(promote)
*         | sorted_by(by_key(&person::age) | then(by_member(&person::name)));
*
*   The terminals join view.hpp's operator| by shape alone.  view.hpp
* recognises a terminal structurally -- a type with an apply(view) returning a
* value that is not a view -- so nothing in view.hpp changes: these types
* satisfy is_terminal, and the existing (view | terminal) and
* (container | terminal) overloads take them.  As for every view.hpp
* terminal, a pipe with a standard container on the left finds those
* overloads only where djinterp's operator| is in scope -- inside namespace
* djinterp, or after `using djinterp::operator|;` -- since neither operand's
* namespace holds them.
*
*   OPT-IN.  sort.hpp does not include this header, so a caller who only sorts
* ranges does not pay for the view subsystem; include it to pipe into the
* dataflow.  It requires C++11, as both view.hpp and the dispatch facility
* do; before C++11 it is empty.
*
*
* path:      /inc/djinterp/cpp/util/sort/sort_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.25
*                                                            revised: 2026.11.12
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SORT TERMINALS            internal::DSortDefaultTerminal,
                                internal::DSortWithTerminal<Compare>
II.   TERMINAL FACTORIES        sorted, sorted_by
*/

#ifndef DJINTERP_UTIL_SORT_VIEW_HPP
#define DJINTERP_UTIL_SORT_VIEW_HPP 1

// djinterp
#include "../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <functional>                  // std::less
#include <type_traits>                 // std::decay
#include <utility>                     // std::forward, std::move
#include <vector>                      // std::vector
// djinterp
#include "../../functional/view.hpp"   // the terminal protocol, operator|
#include "./sort_dispatch.hpp"         // DSorter<>, DSortAlgorithm


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                    I.   SORT TERMINALS                                  ///
///////////////////////////////////////////////////////////////////////////////
//   view.hpp recognises a terminal by shape: a template apply(const View&)
// whose result is not a view.  Both terminals below drain the upstream view
// into a vector, as to_vector does, and sort it in place with the run-time
// sorter.  The trailing return type keeps the body out of view.hpp's
// is_terminal probe, which instantiates only the signature.

NS_INTERNAL

    // DSortDefaultTerminal
    //   class: drains a view into a std::vector ascending under std::less of
    // the element type, sorted with the algorithm it holds.
    class DSortDefaultTerminal
    {
    public:
        explicit DSortDefaultTerminal(
            DSortAlgorithm _algorithm
        )
            : m_algorithm(_algorithm)
        {}

        template<typename View>
        auto apply(
            const View& _view
        ) const
        -> std::vector<typename View::value_type>
        {
            typedef typename View::value_type value_type;

            std::vector<value_type> result;

            // a loop rather than the range constructor: a view's end may be
            // a sentinel of another type, and its iterator need not publish
            // iterator_traits
            for (auto it = _view.begin(); it != _view.end(); ++it)
            {
                result.push_back(*it);
            }

            DSorter<DDynamicAlgorithm> sorter(m_algorithm);

            sorter(result.begin(),
                   result.end(),
                   std::less<value_type>());

            return result;
        }

    private:
        DSortAlgorithm m_algorithm;
    };

    // DSortWithTerminal
    //   class: drains a view into a std::vector ordered by the comparator it
    // holds -- any model of is_comparator, a composed one from the algebra
    // included -- sorted with the algorithm it holds.
    template<typename Compare>
    class DSortWithTerminal
    {
    public:
        DSortWithTerminal(
            Compare        _compare,
            DSortAlgorithm _algorithm
        )
            : m_compare(std::move(_compare)),
              m_algorithm(_algorithm)
        {}

        template<typename View>
        auto apply(
            const View& _view
        ) const
        -> std::vector<typename View::value_type>
        {
            typedef typename View::value_type value_type;

            std::vector<value_type> result;

            for (auto it = _view.begin(); it != _view.end(); ++it)
            {
                result.push_back(*it);
            }

            DSorter<DDynamicAlgorithm> sorter(m_algorithm);

            sorter(result.begin(),
                   result.end(),
                   m_compare);

            return result;
        }

    private:
        Compare        m_compare;
        DSortAlgorithm m_algorithm;
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                    II.  TERMINAL FACTORIES                              ///
///////////////////////////////////////////////////////////////////////////////
//   Each returns a terminal that view.hpp's (view | terminal) and
// (container | terminal) overloads drive, as to_vector() and count() do.
// Without an algorithm they use DDefaultSortTag's.

// sorted
//   function: a terminal that drains the pipeline into a std::vector,
// ascending under std::less of the element type.
//
//     auto v = some_view | sorted();
inline
internal::DSortDefaultTerminal sorted()
{
    return internal::DSortDefaultTerminal(DDefaultSortTag::value);
}

// sorted
//   function: sorted() with _algorithm.
inline
internal::DSortDefaultTerminal sorted(
    DSortAlgorithm _algorithm
)
{
    return internal::DSortDefaultTerminal(_algorithm);
}

// sorted_by
//   function: a terminal that drains the pipeline into a std::vector ordered
// by _compare -- typically a comparator built from the algebra.
//
//     auto v = some_view | sorted_by(by_member(&person::age));
template<typename Compare>
D_NODISCARD
internal::DSortWithTerminal<typename std::decay<Compare>::type> sorted_by(
    Compare&& _compare
)
{
    return internal::DSortWithTerminal<typename std::decay<Compare>::type>(
        std::forward<Compare>(_compare),
        DDefaultSortTag::value);
}

// sorted_by
//   function: sorted_by(comp) with _algorithm.  The algorithm comes first,
// so this form cannot be confused with the one-argument one.
template<typename Compare>
D_NODISCARD
internal::DSortWithTerminal<typename std::decay<Compare>::type> sorted_by(
    DSortAlgorithm _algorithm,
    Compare&&      _compare
)
{
    return internal::DSortWithTerminal<typename std::decay<Compare>::type>(
        std::forward<Compare>(_compare),
        _algorithm);
}


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_UTIL_SORT_VIEW_HPP
