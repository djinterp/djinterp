/******************************************************************************
* djinterp [utility]                                             sort_view.hpp
*
* The dataflow bridge: sort as a terminal in the lazy view pipeline.
*   view.hpp builds lazy pipelines with operator| -- adapters (transform,
* filter, take, ...) compose lazily, and a terminal (to_vector, count, fold)
* drains the pipeline to a concrete value.  Sorting cannot be a lazy adapter:
* it is a strict whole-sequence barrier that must see every element before it
* can yield the first.  It belongs precisely where the laziness ends -- as a
* TERMINAL.  This header adds sort terminals that drain a view (or a container,
* which view.hpp lifts to a ref_view) into a std::vector ordered by the chosen
* comparator and algorithm:
*
*     using namespace djinterp;
*     std::vector<person> people = ...;
*     std::vector<person> by_age =
*         people
*         | views::transform(promote)
*         | sorted_by(by_key(&person::age) | then(by_member(&person::name)));
*
*   The terminals plug into view.hpp's existing operator| purely by shape:
* view.hpp recognizes a terminal structurally (a type with apply(view)
* returning a non-view value), so NOTHING in view.hpp changes -- these types
* simply satisfy is_terminal and the already-present (view | terminal) and
* (container | terminal) overloads pick them up.
*
*   OPT-IN.  This bridge is deliberately kept OUT of the sort.hpp umbrella so a
* caller who only sorts ranges does not pay for the view subsystem; include
* sort_view.hpp explicitly when piping into the dataflow.
*
*
* path:      /inc/djinterp/core/util/sort/sort_view.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.06.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SORT TERMINALS                               (namespace internal)
      1.  sort_default_terminal                    (default comparator)
      2.  sort_with_terminal<Compare>              (explicit comparator)
II.   TERMINAL FACTORIES
      1.  sorted / sorted(algorithm)
      2.  sorted_by(comp) / sorted_by(algorithm, comp)
*/


#ifndef DJINTERP_UTILITY_SORT_VIEW_
#define DJINTERP_UTILITY_SORT_VIEW_ 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"
#include "./sort_dispatch.hpp"            // sorter<dynamic_algorithm>, sort_algorithm
#include "../../functional/view.hpp"      // terminal protocol + operator| overloads


//   The bridge requires C++11 or later: both the view subsystem and the
// dispatch facility do.
#if D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///             I.    SORT TERMINALS                                        ///
///////////////////////////////////////////////////////////////////////////////
//   A terminal is recognized by view.hpp structurally: a type with a template
// apply(const View&) whose result is a non-view value.  Both terminals below
// mirror to_vector_terminal -- they iterate the upstream view into a vector,
// then sort it in place via the runtime sorter.  The trailing return type
// keeps the body out of the is_terminal probe (which instantiates only the
// signature, against single_view<int>).

NS_INTERNAL

    // sort_default_terminal
    //   helper: drains a view into a std::vector ordered by the element type's
    // default comparator (ascending), using the selected algorithm.
    class sort_default_terminal
    {
    public:
        explicit sort_default_terminal(
            sort_algorithm _algorithm
        )
            : m_algorithm(_algorithm)
        {
        }

        template<typename _View>
        auto apply(
            const _View& _view
        ) const
        -> std::vector<typename _View::value_type>
        {
            typedef typename _View::value_type value_type;

            std::vector<value_type> _result;

            for (auto _it = _view.begin(); _it != _view.end(); ++_it)
            {
                _result.push_back(*_it);
            }

            sorter<dynamic_algorithm> _dispatcher(m_algorithm);
            _dispatcher(_result.begin(),
                        _result.end(),
                        default_comparator_t<value_type>());

            return _result;
        }

    private:
        sort_algorithm m_algorithm;
    };


    // sort_with_terminal
    //   helper: drains a view into a std::vector ordered by an explicit
    // comparator (any model of is_comparator, including a composed comparator
    // from the algebra), using the selected algorithm.
    template<typename _Compare>
    class sort_with_terminal
    {
    public:
        sort_with_terminal(
            _Compare       _compare,
            sort_algorithm _algorithm
        )
            : m_compare(static_cast<_Compare&&>(_compare)),
              m_algorithm(_algorithm)
        {
        }

        template<typename _View>
        auto apply(
            const _View& _view
        ) const
        -> std::vector<typename _View::value_type>
        {
            typedef typename _View::value_type value_type;

            std::vector<value_type> _result;

            for (auto _it = _view.begin(); _it != _view.end(); ++_it)
            {
                _result.push_back(*_it);
            }

            sorter<dynamic_algorithm> _dispatcher(m_algorithm);
            _dispatcher(_result.begin(), _result.end(), m_compare);

            return _result;
        }

    private:
        _Compare       m_compare;
        sort_algorithm m_algorithm;
    };

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             II.   TERMINAL FACTORIES                                    ///
///////////////////////////////////////////////////////////////////////////////
//   These mirror to_vector() / count() in view.hpp: each returns a terminal
// that the existing (view | terminal) and (container | terminal) operator|
// overloads drive.  The default algorithm matches default_sort_tag
// (quick_sort).

// sorted
//   function: terminal that drains the pipeline into a std::vector ordered by
// the element type's default (ascending) comparator.
//
//     auto v = (some_view) | sorted();
inline
internal::sort_default_terminal
sorted()
{
    return internal::sort_default_terminal(sort_algorithm::quick);
}

// sorted (explicit algorithm)
//   function: sorted() with a caller-chosen algorithm.
inline
internal::sort_default_terminal
sorted(
    sort_algorithm _algorithm
)
{
    return internal::sort_default_terminal(_algorithm);
}

// sorted_by
//   function: terminal that drains the pipeline into a std::vector ordered by
// an explicit comparator -- typically one built from the comparator algebra.
//
//     auto v = (some_view) | sorted_by(by_member(&person::age));
template<typename _Compare>
D_NODISCARD
internal::sort_with_terminal<typename std::decay<_Compare>::type>
sorted_by(
    _Compare&& _compare
)
{
    return internal::sort_with_terminal<typename std::decay<_Compare>::type>(
        static_cast<_Compare&&>(_compare), sort_algorithm::quick);
}

// sorted_by (explicit algorithm)
//   function: sorted_by(comp) with a caller-chosen algorithm.  The algorithm
// leads so the two-argument form is unambiguous against the one-argument form.
template<typename _Compare>
D_NODISCARD
internal::sort_with_terminal<typename std::decay<_Compare>::type>
sorted_by(
    sort_algorithm _algorithm,
    _Compare&&     _compare
)
{
    return internal::sort_with_terminal<typename std::decay<_Compare>::type>(
        static_cast<_Compare&&>(_compare), _algorithm);
}


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_UTILITY_SORT_VIEW_
