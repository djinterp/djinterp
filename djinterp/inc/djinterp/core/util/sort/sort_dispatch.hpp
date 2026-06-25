/******************************************************************************
* djinterp [utility]                                         sort_dispatch.hpp
*
*   djinterp algorithm-agnostic sort dispatch facility.
* Pulls in sort_common.hpp and every algorithm header, then layers an
* algorithm-agnostic dispatch facility on top of them.  This is the dispatch
* component of the sort subsystem; the umbrella sort.hpp re-exports it
* alongside the functional-layer bridges (sort_monoid.hpp, and the opt-in
* sort_view.hpp).  The facility lets a caller name a sorting algorithm in
* either of two interchangeable ways:
*
*     - at compile time, by supplying an algorithm tag type as a template
*       argument (zero-overhead static dispatch); or
*     - at run time, by supplying a sort_algorithm enum value as a function
*       argument (switch dispatch).
*
*   Both axes resolve to the very same per-algorithm entry points, and a
* single sort_algorithm value may be lifted to its compile-time tag via
* algorithm_tag_t, so a choice made at run time and a choice made at compile
* time stay in lock-step.
*
*   The umbrella include is valid in every supported language mode, but the
* dispatch facility itself requires C++11 or later (it relies on the
* hint-free algorithm entry points, scoped enumerations, and tag dispatch).
*
*   Every entry point that accepts a comparator accepts any model of
* is_comparator<C, T> -- including the whole comparator algebra from
* functional/comparator.hpp (natural, by_key, by_member, reversed, then, ...)
* now reachable through sort_common.hpp -- since those are ordinary
* std::sort-convention binary callables.
*
*   overview:
*     I.    sort_algorithm            (runtime selector enum)
*     II.   algorithm tags            (compile-time selectors + traits)
*     III.  algorithm trait bridge    (enum <-> tag, runtime properties)
*     IV.   runtime dispatch          (internal switch)
*     V.    sorter                    (the algorithm-agnostic sort type)
*     VI.   entry points              (sort<tag>(...) and sort(algo, ...))
*
*
* path:      /inc/djinterp/core/util/sort/sort_dispatch.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.03.22
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_DISPATCH_
#define DJINTERP_UTILITY_SORT_DISPATCH_ 1

// std
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"
#include "./sort_common.hpp"
#include "./bubble_sort.hpp"
#include "./selection_sort.hpp"
#include "./insertion_sort.hpp"
#include "./merge_sort.hpp"
#include "./quick_sort.hpp"
#include "./heap_sort.hpp"


// the algorithm-agnostic facility is a C++11+ surface; the umbrella include
// above is still useful in C++98 (it exposes every algorithm entry point
// through a single header)
#if D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_DJINTERP

///////////////////////////////////////////////////////////////////////////////
///                    I.   SELECTOR                                        ///
///////////////////////////////////////////////////////////////////////////////

// sort_algorithm
//   enum: runtime selector naming each algorithm exposed by this subsystem.
// Every enumerator has a matching compile-time tag (section II) and the two
// are bridged by sort_algorithm_traits (section III).
enum class sort_algorithm : std::uint8_t
{
    bubble,
    selection,
    insertion,
    merge,
    quick,
    heap
};


///////////////////////////////////////////////////////////////////////////////
///                    II.  ALGORITHM TAGS                                  ///
///////////////////////////////////////////////////////////////////////////////
//   Each tag is an empty type that names one algorithm at compile time. A tag
// carries the algorithm's compile-time traits, its corresponding runtime
// enumerator (`value`), a human-readable `name`, and a uniform `apply` that
// forwards to the algorithm's hint-free entry point.

// sort_tag_base
//   struct: empty marker inherited by every algorithm tag so that tags can
// be detected with is_sort_tag.
struct sort_tag_base
{
};

// bubble_sort_tag
//   struct: compile-time selector and dispatcher for bubble sort.
struct bubble_sort_tag : sort_tag_base
{
    static constexpr sort_algorithm value       = sort_algorithm::bubble;
    static constexpr bool            is_stable   = true;
    static constexpr bool            is_in_place = true;
    static constexpr bool            is_adaptive = true;

    // name
    //   function: returns the algorithm's identifying name.
    static const char* name()
    {
        return "bubble_sort";
    }

    // apply
    //   function: sorts [_first, _last) with bubble sort under _comp.
    template<typename _RandomIterator,
             typename _Comparator>
    static void apply(_RandomIterator _first,
                      _RandomIterator _last,
                      _Comparator     _comp)
    {
        bubble_sort(_first,
                    _last,
                    _comp);

        return;
    }
};

// selection_sort_tag
//   struct: compile-time selector and dispatcher for selection sort.
struct selection_sort_tag : sort_tag_base
{
    static constexpr sort_algorithm value       = sort_algorithm::selection;
    static constexpr bool            is_stable   = false;
    static constexpr bool            is_in_place = true;
    static constexpr bool            is_adaptive = false;

    // name
    //   function: returns the algorithm's identifying name.
    static const char* name()
    {
        return "selection_sort";
    }

    // apply
    //   function: sorts [_first, _last) with selection sort under _comp.
    template<typename _RandomIterator,
             typename _Comparator>
    static void apply(_RandomIterator _first,
                      _RandomIterator _last,
                      _Comparator     _comp)
    {
        selection_sort(_first,
                       _last,
                       _comp);

        return;
    }
};

// insertion_sort_tag
//   struct: compile-time selector and dispatcher for insertion sort.
struct insertion_sort_tag : sort_tag_base
{
    static constexpr sort_algorithm value       = sort_algorithm::insertion;
    static constexpr bool            is_stable   = true;
    static constexpr bool            is_in_place = true;
    static constexpr bool            is_adaptive = true;

    // name
    //   function: returns the algorithm's identifying name.
    static const char* name()
    {
        return "insertion_sort";
    }

    // apply
    //   function: sorts [_first, _last) with insertion sort under _comp.
    template<typename _RandomIterator,
             typename _Comparator>
    static void apply(_RandomIterator _first,
                      _RandomIterator _last,
                      _Comparator     _comp)
    {
        insertion_sort(_first,
                       _last,
                       _comp);

        return;
    }
};

// merge_sort_tag
//   struct: compile-time selector and dispatcher for merge sort.
struct merge_sort_tag : sort_tag_base
{
    static constexpr sort_algorithm value       = sort_algorithm::merge;
    static constexpr bool            is_stable   = true;
    static constexpr bool            is_in_place = false;
    static constexpr bool            is_adaptive = false;

    // name
    //   function: returns the algorithm's identifying name.
    static const char* name()
    {
        return "merge_sort";
    }

    // apply
    //   function: sorts [_first, _last) with merge sort under _comp.
    template<typename _RandomIterator,
             typename _Comparator>
    static void apply(_RandomIterator _first,
                      _RandomIterator _last,
                      _Comparator     _comp)
    {
        merge_sort(_first,
                   _last,
                   _comp);

        return;
    }
};

// quick_sort_tag
//   struct: compile-time selector and dispatcher for quicksort.
struct quick_sort_tag : sort_tag_base
{
    static constexpr sort_algorithm value       = sort_algorithm::quick;
    static constexpr bool            is_stable   = false;
    static constexpr bool            is_in_place = true;
    static constexpr bool            is_adaptive = false;

    // name
    //   function: returns the algorithm's identifying name.
    static const char* name()
    {
        return "quick_sort";
    }

    // apply
    //   function: sorts [_first, _last) with quicksort under _comp.
    template<typename _RandomIterator,
             typename _Comparator>
    static void apply(_RandomIterator _first,
                      _RandomIterator _last,
                      _Comparator     _comp)
    {
        quick_sort(_first,
                   _last,
                   _comp);

        return;
    }
};

// heap_sort_tag
//   struct: compile-time selector and dispatcher for heap sort.
struct heap_sort_tag : sort_tag_base
{
    static constexpr sort_algorithm value       = sort_algorithm::heap;
    static constexpr bool            is_stable   = false;
    static constexpr bool            is_in_place = true;
    static constexpr bool            is_adaptive = false;

    // name
    //   function: returns the algorithm's identifying name.
    static const char* name()
    {
        return "heap_sort";
    }

    // apply
    //   function: sorts [_first, _last) with heap sort under _comp.
    template<typename _RandomIterator,
             typename _Comparator>
    static void apply(_RandomIterator _first,
                      _RandomIterator _last,
                      _Comparator     _comp)
    {
        heap_sort(_first,
                  _last,
                  _comp);

        return;
    }
};

// dynamic_algorithm
//   struct: sentinel tag selecting run-time dispatch. It carries no `apply`
// of its own; sorter<dynamic_algorithm> stores a sort_algorithm value and
// resolves the algorithm through the runtime switch (section IV).
struct dynamic_algorithm
{
};

// default_sort_tag
//   type: the algorithm used when a caller does not name one. Quicksort is a
// strong general-purpose default (median-of-three pivot, insertion-sort
// fallback for small ranges).
typedef quick_sort_tag default_sort_tag;

// is_sort_tag
//   trait: detects whether _Type is one of the algorithm tag types.
template<typename _Type>
struct is_sort_tag : std::is_base_of<sort_tag_base, _Type>
{
};


///////////////////////////////////////////////////////////////////////////////
///                    III. ALGORITHM TRAIT BRIDGE                          ///
///////////////////////////////////////////////////////////////////////////////
//   Bridges the runtime selector (section I) and the compile-time tags
// (section II). sort_algorithm_traits lifts an enumerator to its tag and
// compile-time properties; properties_of and name_of expose the same data
// for a value known only at run time.

// sort_algorithm_traits
//   trait: maps a compile-time sort_algorithm value to its tag and
// properties. The primary template is intentionally left undefined; only the
// six enumerator specializations below are valid.
template<sort_algorithm _Algorithm>
struct sort_algorithm_traits;

// sort_algorithm_traits<bubble>
//   trait: bridge specialization for bubble sort.
template<>
struct sort_algorithm_traits<sort_algorithm::bubble>
{
    typedef bubble_sort_tag tag;

    static constexpr bool is_stable   = bubble_sort_tag::is_stable;
    static constexpr bool is_in_place = bubble_sort_tag::is_in_place;
    static constexpr bool is_adaptive = bubble_sort_tag::is_adaptive;
};

// sort_algorithm_traits<selection>
//   trait: bridge specialization for selection sort.
template<>
struct sort_algorithm_traits<sort_algorithm::selection>
{
    typedef selection_sort_tag tag;

    static constexpr bool is_stable   = selection_sort_tag::is_stable;
    static constexpr bool is_in_place = selection_sort_tag::is_in_place;
    static constexpr bool is_adaptive = selection_sort_tag::is_adaptive;
};

// sort_algorithm_traits<insertion>
//   trait: bridge specialization for insertion sort.
template<>
struct sort_algorithm_traits<sort_algorithm::insertion>
{
    typedef insertion_sort_tag tag;

    static constexpr bool is_stable   = insertion_sort_tag::is_stable;
    static constexpr bool is_in_place = insertion_sort_tag::is_in_place;
    static constexpr bool is_adaptive = insertion_sort_tag::is_adaptive;
};

// sort_algorithm_traits<merge>
//   trait: bridge specialization for merge sort.
template<>
struct sort_algorithm_traits<sort_algorithm::merge>
{
    typedef merge_sort_tag tag;

    static constexpr bool is_stable   = merge_sort_tag::is_stable;
    static constexpr bool is_in_place = merge_sort_tag::is_in_place;
    static constexpr bool is_adaptive = merge_sort_tag::is_adaptive;
};

// sort_algorithm_traits<quick>
//   trait: bridge specialization for quicksort.
template<>
struct sort_algorithm_traits<sort_algorithm::quick>
{
    typedef quick_sort_tag tag;

    static constexpr bool is_stable   = quick_sort_tag::is_stable;
    static constexpr bool is_in_place = quick_sort_tag::is_in_place;
    static constexpr bool is_adaptive = quick_sort_tag::is_adaptive;
};

// sort_algorithm_traits<heap>
//   trait: bridge specialization for heap sort.
template<>
struct sort_algorithm_traits<sort_algorithm::heap>
{
    typedef heap_sort_tag tag;

    static constexpr bool is_stable   = heap_sort_tag::is_stable;
    static constexpr bool is_in_place = heap_sort_tag::is_in_place;
    static constexpr bool is_adaptive = heap_sort_tag::is_adaptive;
};

// algorithm_tag_t
//   type: lifts a runtime sort_algorithm value to its compile-time tag,
// e.g. algorithm_tag_t<sort_algorithm::quick> is quick_sort_tag.
template<sort_algorithm _Algorithm>
using algorithm_tag_t = typename sort_algorithm_traits<_Algorithm>::tag;

// sort_traits
//   struct: run-time-queryable snapshot of an algorithm's properties.
struct sort_traits
{
    bool is_stable;
    bool is_in_place;
    bool is_adaptive;
};

// properties_of
//   function: returns the sort_traits for a run-time sort_algorithm value.
// The values are sourced from the algorithm tags, keeping a single source of
// truth across the compile-time and run-time surfaces.
inline sort_traits properties_of(sort_algorithm _algorithm)
{
    sort_traits result;

    result.is_stable   = false;
    result.is_in_place = false;
    result.is_adaptive = false;

    // map the enumerator onto its tag's compile-time properties
    switch (_algorithm)
    {
        case sort_algorithm::bubble:
            result.is_stable   = bubble_sort_tag::is_stable;
            result.is_in_place = bubble_sort_tag::is_in_place;
            result.is_adaptive = bubble_sort_tag::is_adaptive;
            break;

        case sort_algorithm::selection:
            result.is_stable   = selection_sort_tag::is_stable;
            result.is_in_place = selection_sort_tag::is_in_place;
            result.is_adaptive = selection_sort_tag::is_adaptive;
            break;

        case sort_algorithm::insertion:
            result.is_stable   = insertion_sort_tag::is_stable;
            result.is_in_place = insertion_sort_tag::is_in_place;
            result.is_adaptive = insertion_sort_tag::is_adaptive;
            break;

        case sort_algorithm::merge:
            result.is_stable   = merge_sort_tag::is_stable;
            result.is_in_place = merge_sort_tag::is_in_place;
            result.is_adaptive = merge_sort_tag::is_adaptive;
            break;

        case sort_algorithm::quick:
            result.is_stable   = quick_sort_tag::is_stable;
            result.is_in_place = quick_sort_tag::is_in_place;
            result.is_adaptive = quick_sort_tag::is_adaptive;
            break;

        case sort_algorithm::heap:
            result.is_stable   = heap_sort_tag::is_stable;
            result.is_in_place = heap_sort_tag::is_in_place;
            result.is_adaptive = heap_sort_tag::is_adaptive;
            break;
    }

    return result;
}

// name_of
//   function: returns the identifying name for a run-time sort_algorithm
// value, or "unknown" for an unrecognised enumerator.
inline const char* name_of(sort_algorithm _algorithm)
{
    const char* result;

    result = "unknown";

    // map the enumerator onto its tag's name
    switch (_algorithm)
    {
        case sort_algorithm::bubble:
            result = bubble_sort_tag::name();
            break;

        case sort_algorithm::selection:
            result = selection_sort_tag::name();
            break;

        case sort_algorithm::insertion:
            result = insertion_sort_tag::name();
            break;

        case sort_algorithm::merge:
            result = merge_sort_tag::name();
            break;

        case sort_algorithm::quick:
            result = quick_sort_tag::name();
            break;

        case sort_algorithm::heap:
            result = heap_sort_tag::name();
            break;
    }

    return result;
}


///////////////////////////////////////////////////////////////////////////////
///                    IV.  RUNTIME DISPATCH                                ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // dispatch_runtime
    //   function: forwards [_first, _last) to the algorithm named by the
    // run-time value _algorithm, applying the comparator _comp. An
    // unrecognised enumerator is a no-op (the range is left untouched).
    //
    //   This switch is the single point of run-time selection; both the
    // free sort(algorithm, ...) entry points and sorter<dynamic_algorithm>
    // resolve through it, so the two share identical dispatch behaviour.
    template<typename _RandomIterator,
             typename _Comparator>
    void dispatch_runtime(sort_algorithm  _algorithm,
                          _RandomIterator _first,
                          _RandomIterator _last,
                          _Comparator     _comp)
    {
        switch (_algorithm)
        {
            case sort_algorithm::bubble:
                bubble_sort_tag::apply(_first, _last, _comp);
                break;

            case sort_algorithm::selection:
                selection_sort_tag::apply(_first, _last, _comp);
                break;

            case sort_algorithm::insertion:
                insertion_sort_tag::apply(_first, _last, _comp);
                break;

            case sort_algorithm::merge:
                merge_sort_tag::apply(_first, _last, _comp);
                break;

            case sort_algorithm::quick:
                quick_sort_tag::apply(_first, _last, _comp);
                break;

            case sort_algorithm::heap:
                heap_sort_tag::apply(_first, _last, _comp);
                break;
        }

        return;
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                    V.   SORTER                                          ///
///////////////////////////////////////////////////////////////////////////////
//   The algorithm-agnostic sort type. A sorter is a small callable that hides
// which algorithm performs the work behind a uniform operator(). The
// algorithm may be fixed at compile time by naming a tag (the primary
// template, stateless and zero-overhead) or chosen at run time by leaving the
// parameter defaulted and supplying a sort_algorithm value (the
// dynamic_algorithm specialization, which stores the selector).

// sorter
//   class: algorithm-agnostic sort functor. sorter<tag> pins the algorithm at
// compile time; sorter<> (i.e. sorter<dynamic_algorithm>) defers the choice
// to a run-time sort_algorithm value.
template<typename _Algorithm = dynamic_algorithm>
class sorter
{
    static_assert(is_sort_tag<_Algorithm>::value,
                  "Template parameter `_Algorithm` must be a sort tag type "
                  "(e.g. quick_sort_tag) or the dynamic_algorithm sentinel.");

public:
    // algorithm
    //   value: the enumerator naming the pinned algorithm.
    static constexpr sort_algorithm algorithm   = _Algorithm::value;

    // is_stable / is_in_place / is_adaptive
    //   value: the pinned algorithm's compile-time properties.
    static constexpr bool           is_stable   = _Algorithm::is_stable;
    static constexpr bool           is_in_place = _Algorithm::is_in_place;
    static constexpr bool           is_adaptive = _Algorithm::is_adaptive;

    // name
    //   function: returns the pinned algorithm's identifying name.
    static const char* name()
    {
        return _Algorithm::name();
    }

    // operator()(first, last, comp)
    //   function: sorts [_first, _last) under _comp using the pinned
    // algorithm.
    template<typename _RandomIterator,
             typename _Comparator>
    void operator()(_RandomIterator _first,
                    _RandomIterator _last,
                    _Comparator     _comp) const
    {
        _Algorithm::apply(_first,
                          _last,
                          _comp);

        return;
    }

    // operator()(first, last)
    //   function: sorts [_first, _last) ascending using the pinned algorithm
    // and the default comparator.
    template<typename _RandomIterator>
    void operator()(_RandomIterator _first,
                    _RandomIterator _last) const
    {
        typedef typename
            std::iterator_traits<_RandomIterator>::value_type value_type;

        _Algorithm::apply(_first,
                          _last,
                          less<value_type>());

        return;
    }

    // operator()(first, last, comp, order)
    //   function: sorts [_first, _last) using the pinned algorithm, adapting
    // _comp to the requested _order.
    template<typename _RandomIterator,
             typename _Comparator>
    void operator()(_RandomIterator _first,
                    _RandomIterator _last,
                    _Comparator     _comp,
                    sort_order      _order) const
    {
        internal::order_comparator<_Comparator> wrapped(_comp, _order);

        _Algorithm::apply(_first,
                          _last,
                          wrapped);

        return;
    }
};

// sorter<dynamic_algorithm>
//   class: run-time specialization. Stores a sort_algorithm selector and
// resolves the algorithm through the internal switch on each call.
template<>
class sorter<dynamic_algorithm>
{
public:
    // sorter
    //   function: constructs a sorter using the default algorithm.
    sorter()
        : m_algorithm(default_sort_tag::value)
    {
    }

    // sorter
    //   function: constructs a sorter bound to _algorithm.
    explicit sorter(
        sort_algorithm _algorithm
    )
        : m_algorithm(_algorithm)
    {
    }

    // algorithm
    //   function: returns the currently selected algorithm.
    sort_algorithm algorithm() const
    {
        return m_algorithm;
    }

    // set_algorithm
    //   function: rebinds the sorter to _algorithm.
    void set_algorithm(sort_algorithm _algorithm)
    {
        m_algorithm = _algorithm;

        return;
    }

    // is_stable / is_in_place / is_adaptive
    //   function: the selected algorithm's properties, resolved at run time.
    bool is_stable() const
    {
        return properties_of(m_algorithm).is_stable;
    }

    bool is_in_place() const
    {
        return properties_of(m_algorithm).is_in_place;
    }

    bool is_adaptive() const
    {
        return properties_of(m_algorithm).is_adaptive;
    }

    // name
    //   function: returns the selected algorithm's identifying name.
    const char* name() const
    {
        return name_of(m_algorithm);
    }

    // operator()(first, last, comp)
    //   function: sorts [_first, _last) under _comp using the selected
    // algorithm.
    template<typename _RandomIterator,
             typename _Comparator>
    void operator()(_RandomIterator _first,
                    _RandomIterator _last,
                    _Comparator     _comp) const
    {
        internal::dispatch_runtime(m_algorithm,
                                   _first,
                                   _last,
                                   _comp);

        return;
    }

    // operator()(first, last)
    //   function: sorts [_first, _last) ascending using the selected
    // algorithm and the default comparator.
    template<typename _RandomIterator>
    void operator()(_RandomIterator _first,
                    _RandomIterator _last) const
    {
        typedef typename
            std::iterator_traits<_RandomIterator>::value_type value_type;

        internal::dispatch_runtime(m_algorithm,
                                   _first,
                                   _last,
                                   less<value_type>());

        return;
    }

    // operator()(first, last, comp, order)
    //   function: sorts [_first, _last) using the selected algorithm,
    // adapting _comp to the requested _order.
    template<typename _RandomIterator,
             typename _Comparator>
    void operator()(_RandomIterator _first,
                    _RandomIterator _last,
                    _Comparator     _comp,
                    sort_order      _order) const
    {
        internal::order_comparator<_Comparator> wrapped(_comp, _order);

        internal::dispatch_runtime(m_algorithm,
                                   _first,
                                   _last,
                                   wrapped);

        return;
    }

private:
    sort_algorithm m_algorithm;
};


///////////////////////////////////////////////////////////////////////////////
///                    VI.  ENTRY POINTS                                    ///
///////////////////////////////////////////////////////////////////////////////
//   Free-function sugar over sorter. The compile-time overloads take the
// algorithm tag as a leading template parameter (defaulted to
// default_sort_tag); the run-time overloads take a sort_algorithm value as a
// leading function parameter. The two families never collide: a leading
// sort_algorithm argument selects the run-time form, its absence the
// compile-time form.

// ----------------------------------------------------------------------------
// A.  compile-time: sort<tag>(first, last, ...)
// ----------------------------------------------------------------------------

// sort
//   function: sorts [_first, _last) under _comp using the algorithm named by
// the tag _SortTag (default_sort_tag when omitted).
template<typename _SortTag = default_sort_tag,
         typename _RandomIterator,
         typename _Comparator>
void sort(_RandomIterator _first,
          _RandomIterator _last,
          _Comparator     _comp)
{
    sorter<_SortTag>()(_first,
                       _last,
                       _comp);

    return;
}

// sort
//   function: sorts [_first, _last) ascending using the algorithm named by
// _SortTag and the default comparator.
template<typename _SortTag = default_sort_tag,
         typename _RandomIterator>
void sort(_RandomIterator _first,
          _RandomIterator _last)
{
    sorter<_SortTag>()(_first,
                       _last);

    return;
}

// sort
//   function: sorts [_first, _last) using the algorithm named by _SortTag,
// adapting _comp to the requested _order.
template<typename _SortTag = default_sort_tag,
         typename _RandomIterator,
         typename _Comparator>
void sort(_RandomIterator _first,
          _RandomIterator _last,
          _Comparator     _comp,
          sort_order      _order)
{
    sorter<_SortTag>()(_first,
                       _last,
                       _comp,
                       _order);

    return;
}

// ----------------------------------------------------------------------------
// B.  run-time: sort(algorithm, first, last, ...)
// ----------------------------------------------------------------------------

// sort
//   function: sorts [_first, _last) under _comp using the algorithm named by
// the run-time value _algorithm.
template<typename _RandomIterator,
         typename _Comparator>
void sort(sort_algorithm  _algorithm,
          _RandomIterator _first,
          _RandomIterator _last,
          _Comparator     _comp)
{
    sorter<dynamic_algorithm> dispatcher(_algorithm);

    dispatcher(_first,
               _last,
               _comp);

    return;
}

// sort
//   function: sorts [_first, _last) ascending using the algorithm named by
// the run-time value _algorithm and the default comparator.
template<typename _RandomIterator>
void sort(sort_algorithm  _algorithm,
          _RandomIterator _first,
          _RandomIterator _last)
{
    sorter<dynamic_algorithm> dispatcher(_algorithm);

    dispatcher(_first,
               _last);

    return;
}

// sort
//   function: sorts [_first, _last) using the algorithm named by the run-time
// value _algorithm, adapting _comp to the requested _order.
template<typename _RandomIterator,
         typename _Comparator>
void sort(sort_algorithm  _algorithm,
          _RandomIterator _first,
          _RandomIterator _last,
          _Comparator     _comp,
          sort_order      _order)
{
    sorter<dynamic_algorithm> dispatcher(_algorithm);

    dispatcher(_first,
               _last,
               _comp,
               _order);

    return;
}


NS_END  // djinterp


#endif  // C++11


#endif  // DJINTERP_UTILITY_SORT_DISPATCH_
