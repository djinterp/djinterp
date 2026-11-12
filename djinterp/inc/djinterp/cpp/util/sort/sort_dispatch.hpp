/*******************************************************************************
* djinterp [cpp]                                               sort_dispatch.hpp
*
*   The algorithm-agnostic sort.  Includes every algorithm header and layers
* one facility over them that lets a caller name the algorithm in either of
* two interchangeable ways:
*
*     - at compile time, with a tag object, so the choice costs nothing:
*           sort(DMergeSortTag(), v.begin(), v.end());
*     - at run time, with a DSortAlgorithm value, through one switch:
*           sort(DSortAlgorithm::merge, v.begin(), v.end());
*
* and to leave it out, which picks DDefaultSortTag (quicksort):
*           sort(v.begin(), v.end());
*
*   Both axes reach the same per-algorithm entry points, a tag names its
* run-time value as Tag::value, and algorithm_tag_t lifts a value back to its
* tag, so a choice made at run time and one made at compile time stay in
* step.  DSorter is the same facility as a callable object: DSorter<Tag> pins
* the algorithm in the type, DSorter<> holds a DSortAlgorithm.
*
*   `sort` IS AN OBJECT, NOT A FUNCTION.  A free function template named sort
* shares overload resolution with std::sort: an unqualified sort(first, last)
* on standard iterators finds djinterp::sort by ordinary lookup and std::sort
* by argument-dependent lookup, the two match equally well, and the call is
* ambiguous in the caller's code.  Lookup that finds an object performs no
* argument-dependent lookup at all, so djinterp::sort is a function object --
* the remedy std::ranges::sort uses for the same collision -- and the
* algorithm goes in the first argument, as an execution policy does in
* std::sort, rather than in a template argument an object cannot take.
*
*   Every comparator parameter accepts any std::sort-convention binary
* predicate, which every model of is_comparator is, so the whole comparator
* algebra of functional/comparator.hpp drops in unchanged.  The facility is a
* C++11 surface; before C++11 this header still includes the six algorithm
* headers, whose three-argument and _ordered entry points are C++98.
*
*
* path:      /inc/djinterp/cpp/util/sort/sort_dispatch.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.22
*                                                            revised: 2026.11.12
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SELECTOR                  DSortAlgorithm
II.   ALGORITHM TAGS            DSortTagBase, D*SortTag, DDynamicAlgorithm,
                                DDefaultSortTag, is_sort_tag
III.  TRAIT BRIDGE              sort_algorithm_traits, algorithm_tag_t,
                                sort_traits, properties_of, name_of
IV.   RUN-TIME DISPATCH         internal::visit_sort_algorithm,
                                internal::dispatch_runtime
V.    SORTER                    DSorter<Tag>, DSorter<DDynamicAlgorithm>
VI.   ENTRY POINT               sort
*/

#ifndef DJINTERP_UTIL_SORT_DISPATCH_HPP
#define DJINTERP_UTIL_SORT_DISPATCH_HPP 1

// std
#include <functional>                // std::less
#include <iterator>                  // std::iterator_traits
// djinterp
#include "../../djinterp.hpp"     // framework root
#include "./sort_common.hpp"         // sort_order,
                                     // internal::DOrderComparator
#include "./bubble_sort.hpp"         // bubble_sort, bubble_sort_traits
#include "./selection_sort.hpp"      // selection_sort, *_traits
#include "./insertion_sort.hpp"      // insertion_sort, *_traits
#include "./merge_sort.hpp"          // merge_sort, merge_sort_traits
#include "./quick_sort.hpp"          // quick_sort, quick_sort_traits
#include "./heap_sort.hpp"           // heap_sort, heap_sort_traits

//   the facility is C++11; the includes above are not, and before C++11 they
// are what this header contributes
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <type_traits>               // std::is_base_of, std::enable_if
// re_std
#include "../../../../re_std/cpp/cstdint/cstdint.hpp"  // re_std::uint8_t


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                    I.   SELECTOR                                        ///
///////////////////////////////////////////////////////////////////////////////

// DSortAlgorithm
//   enum: names each algorithm of the subsystem at run time.  Every
// enumerator has a tag (section II), and sort_algorithm_traits (section III)
// maps one to the other.  The values are pinned: they are what a stored or
// transmitted choice holds.
enum class DSortAlgorithm : re_std::uint8_t
{
    bubble    = 0,
    selection = 1,
    insertion = 2,
    merge     = 3,
    quick     = 4,
    heap      = 5
};


///////////////////////////////////////////////////////////////////////////////
///                    II.  ALGORITHM TAGS                                  ///
///////////////////////////////////////////////////////////////////////////////
//   A tag is an empty type naming one algorithm at compile time.  It inherits
// its algorithm's traits struct, so is_stable, is_in_place and is_adaptive
// have one source; adds the run-time enumerator as `value` and the name; and
// forwards apply() to the algorithm's three-argument entry point.

// DSortTagBase
//   struct: the empty base every algorithm tag inherits, so is_sort_tag can
// recognise one.
struct DSortTagBase
{
};

// D_INTERNAL_SORT_TAG
//   macro: defines the tag TAG for the algorithm whose entry point is ENTRY,
// whose traits struct is ENTRY##_traits and whose enumerator is ENUMERATOR.
// The six tags differ in nothing else, so one definition keeps them from
// drifting apart.  The entry point is called qualified, so a function of the
// same name in an iterator's namespace cannot join the overload set.
#define D_INTERNAL_SORT_TAG(TAG, ENTRY, ENUMERATOR)                           \
    struct TAG : DSortTagBase,                                                \
                 ENTRY##_traits                                               \
    {                                                                         \
        static constexpr DSortAlgorithm value = DSortAlgorithm::ENUMERATOR;   \
                                                                              \
        static const char* name()                                             \
        {                                                                     \
            return #ENTRY;                                                    \
        }                                                                     \
                                                                              \
        template<typename RandomIterator,                                     \
                 typename Comparator>                                         \
        static void apply(                                                    \
            RandomIterator _first,                                            \
            RandomIterator _last,                                             \
            Comparator     _comparator                                        \
        )                                                                     \
        {                                                                     \
            ::djinterp::ENTRY(_first,                                         \
                              _last,                                          \
                              _comparator);                                   \
                                                                              \
            return;                                                           \
        }                                                                     \
    }

// DBubbleSortTag .. DHeapSortTag
//   struct: the compile-time selectors, one per algorithm.
D_INTERNAL_SORT_TAG(DBubbleSortTag,    bubble_sort,    bubble);
D_INTERNAL_SORT_TAG(DSelectionSortTag, selection_sort, selection);
D_INTERNAL_SORT_TAG(DInsertionSortTag, insertion_sort, insertion);
D_INTERNAL_SORT_TAG(DMergeSortTag,     merge_sort,     merge);
D_INTERNAL_SORT_TAG(DQuickSortTag,     quick_sort,     quick);
D_INTERNAL_SORT_TAG(DHeapSortTag,      heap_sort,      heap);

#undef D_INTERNAL_SORT_TAG

// DDynamicAlgorithm
//   struct: the sentinel that selects run-time dispatch.  It is not a sort
// tag and has no apply(); DSorter<DDynamicAlgorithm> holds a DSortAlgorithm
// and resolves it through the switch of section IV.
struct DDynamicAlgorithm
{
};

// DDefaultSortTag
//   type: the algorithm a caller gets by not naming one.  Quicksort: median-
// of-three pivots and an insertion-sort finish make it the strongest general
// choice of the six.
typedef DQuickSortTag DDefaultSortTag;

// is_sort_tag
//   trait: true when Type is one of the algorithm tags.
template<typename Type>
struct is_sort_tag
    : std::is_base_of<DSortTagBase, Type>
{
};


///////////////////////////////////////////////////////////////////////////////
///                    III. TRAIT BRIDGE                                    ///
///////////////////////////////////////////////////////////////////////////////

// sort_algorithm_traits
//   trait: maps a DSortAlgorithm value known at compile time to its tag and
// its algorithm's properties.  The primary template is declared and never
// defined, so a value outside the six is a compile error.
template<DSortAlgorithm Algorithm>
struct sort_algorithm_traits;

template<>
struct sort_algorithm_traits<DSortAlgorithm::bubble>
    : bubble_sort_traits
{
    typedef DBubbleSortTag tag;
};

template<>
struct sort_algorithm_traits<DSortAlgorithm::selection>
    : selection_sort_traits
{
    typedef DSelectionSortTag tag;
};

template<>
struct sort_algorithm_traits<DSortAlgorithm::insertion>
    : insertion_sort_traits
{
    typedef DInsertionSortTag tag;
};

template<>
struct sort_algorithm_traits<DSortAlgorithm::merge>
    : merge_sort_traits
{
    typedef DMergeSortTag tag;
};

template<>
struct sort_algorithm_traits<DSortAlgorithm::quick>
    : quick_sort_traits
{
    typedef DQuickSortTag tag;
};

template<>
struct sort_algorithm_traits<DSortAlgorithm::heap>
    : heap_sort_traits
{
    typedef DHeapSortTag tag;
};

// algorithm_tag_t
//   type: the tag of a DSortAlgorithm value known at compile time, e.g.
// algorithm_tag_t<DSortAlgorithm::quick> is DQuickSortTag.
template<DSortAlgorithm Algorithm>
using algorithm_tag_t = typename sort_algorithm_traits<Algorithm>::tag;

// sort_traits
//   struct: an algorithm's properties as run-time values, for a
// DSortAlgorithm known only at run time.  properties_of fills one.
struct sort_traits
{
    bool is_stable;
    bool is_in_place;
    bool is_adaptive;
};


///////////////////////////////////////////////////////////////////////////////
///                    IV.  RUN-TIME DISPATCH                               ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // visit_sort_algorithm
    //   function: the one switch from a DSortAlgorithm value to its tag.
    // Calls _visitor.visit<Tag>() for the tag _algorithm names and returns
    // true, or returns false for a value that names no algorithm.
    //
    //   properties_of, name_of and dispatch_runtime all resolve through it,
    // so the run-time surface cannot disagree with itself, and a new
    // algorithm is one case here rather than one in each of three switches.
    template<typename Visitor>
    bool visit_sort_algorithm(
        DSortAlgorithm _algorithm,
        Visitor&       _visitor
    )
    {
        switch (_algorithm)
        {
            case DSortAlgorithm::bubble:
                _visitor.template visit<DBubbleSortTag>();

                return true;

            case DSortAlgorithm::selection:
                _visitor.template visit<DSelectionSortTag>();

                return true;

            case DSortAlgorithm::insertion:
                _visitor.template visit<DInsertionSortTag>();

                return true;

            case DSortAlgorithm::merge:
                _visitor.template visit<DMergeSortTag>();

                return true;

            case DSortAlgorithm::quick:
                _visitor.template visit<DQuickSortTag>();

                return true;

            case DSortAlgorithm::heap:
                _visitor.template visit<DHeapSortTag>();

                return true;
        }

        return false;
    }

    // DSortTraitsVisitor
    //   class: copies a tag's properties into a sort_traits.
    class DSortTraitsVisitor
    {
    public:
        explicit DSortTraitsVisitor(
            sort_traits& _traits
        )
            : m_traits(_traits)
        {}

        template<typename SortTag>
        void visit()
        {
            m_traits.is_stable   = SortTag::is_stable;
            m_traits.is_in_place = SortTag::is_in_place;
            m_traits.is_adaptive = SortTag::is_adaptive;

            return;
        }

    private:
        sort_traits& m_traits;
    };

    // DSortNameVisitor
    //   class: reads a tag's name.
    class DSortNameVisitor
    {
    public:
        DSortNameVisitor()
            : m_name("unknown")
        {}

        template<typename SortTag>
        void visit()
        {
            m_name = SortTag::name();

            return;
        }

        const char* name() const
        {
            return m_name;
        }

    private:
        const char* m_name;
    };

    // DSortApplyVisitor
    //   class: sorts one range with whichever tag it is handed.
    template<typename RandomIterator,
             typename Comparator>
    class DSortApplyVisitor
    {
    public:
        DSortApplyVisitor(
            RandomIterator _first,
            RandomIterator _last,
            Comparator&    _comparator
        )
            : m_first(_first),
              m_last(_last),
              m_comparator(_comparator)
        {}

        template<typename SortTag>
        void visit()
        {
            SortTag::apply(m_first,
                           m_last,
                           m_comparator);

            return;
        }

    private:
        RandomIterator m_first;
        RandomIterator m_last;
        Comparator&    m_comparator;
    };

    // dispatch_runtime
    //   function: sorts [_first, _last) under _comparator with the algorithm
    // _algorithm names.  A value that names no algorithm leaves the range as
    // it is.  DSorter<DDynamicAlgorithm> and the run-time forms of sort both
    // resolve through this, so the two cannot differ.
    template<typename RandomIterator,
             typename Comparator>
    void dispatch_runtime(
        DSortAlgorithm _algorithm,
        RandomIterator _first,
        RandomIterator _last,
        Comparator     _comparator
    )
    {
        DSortApplyVisitor<RandomIterator, Comparator> visitor(_first,
                                                              _last,
                                                              _comparator);

        visit_sort_algorithm(_algorithm,
                             visitor);

        return;
    }

NS_END  // internal

// properties_of
//   function: the properties of the algorithm a run-time value names, as the
// tag publishes them; all false for a value that names no algorithm.
inline
sort_traits properties_of(
    DSortAlgorithm _algorithm
)
{
    sort_traits result;

    result.is_stable   = false;
    result.is_in_place = false;
    result.is_adaptive = false;

    internal::DSortTraitsVisitor visitor(result);

    internal::visit_sort_algorithm(_algorithm,
                                   visitor);

    return result;
}

// name_of
//   function: the name of the algorithm a run-time value names -- the name of
// its entry point, e.g. "merge_sort" -- or "unknown".
inline
const char* name_of(
    DSortAlgorithm _algorithm
)
{
    internal::DSortNameVisitor visitor;

    internal::visit_sort_algorithm(_algorithm,
                                   visitor);

    return visitor.name();
}


///////////////////////////////////////////////////////////////////////////////
///                    V.   SORTER                                          ///
///////////////////////////////////////////////////////////////////////////////
//   The facility as a callable object.  DSorter<Tag> fixes the algorithm in
// its type and is empty; DSorter<> -- DSorter<DDynamicAlgorithm> -- holds a
// DSortAlgorithm and resolves it on every call.  The two have the same three
// call operators.

// DSorter
//   class: a sort with its algorithm fixed at compile time by Algorithm, one
// of the tags.
template<typename Algorithm = DDynamicAlgorithm>
class DSorter
{
    static_assert(is_sort_tag<Algorithm>::value,
                  "DSorter takes a sort tag (e.g. DQuickSortTag), or no "
                  "argument for run-time selection.");

public:
    // algorithm, is_stable, is_in_place, is_adaptive
    //   value: the fixed algorithm's enumerator and properties.
    static constexpr DSortAlgorithm algorithm   = Algorithm::value;
    static constexpr bool           is_stable   = Algorithm::is_stable;
    static constexpr bool           is_in_place = Algorithm::is_in_place;
    static constexpr bool           is_adaptive = Algorithm::is_adaptive;

    // name
    //   function: the fixed algorithm's name.
    static const char* name()
    {
        return Algorithm::name();
    }

    // operator()(first, last, comp)
    //   function: sorts [_first, _last) under _comparator.
    template<typename RandomIterator,
             typename Comparator>
    void operator()(
        RandomIterator _first,
        RandomIterator _last,
        Comparator     _comparator
    ) const
    {
        Algorithm::apply(_first,
                         _last,
                         _comparator);

        return;
    }

    // operator()(first, last)
    //   function: sorts [_first, _last) ascending, under std::less.
    template<typename RandomIterator>
    void operator()(
        RandomIterator _first,
        RandomIterator _last
    ) const
    {
        typedef typename std::iterator_traits<RandomIterator>::value_type
            value_type;

        Algorithm::apply(_first,
                         _last,
                         std::less<value_type>());

        return;
    }

    // operator()(first, last, comp, order)
    //   function: sorts [_first, _last) under _comparator, in the direction
    // _order asks for.
    template<typename RandomIterator,
             typename Comparator>
    void operator()(
        RandomIterator    _first,
        RandomIterator    _last,
        Comparator        _comparator,
        sort_order::value _order
    ) const
    {
        internal::DOrderComparator<Comparator> wrapped(_comparator, _order);

        Algorithm::apply(_first,
                         _last,
                         wrapped);

        return;
    }
};

#if !D_ENV_LANG_IS_CPP17_OR_HIGHER

//   Before C++17 a static constexpr member that is odr-used -- bound to a
// reference, say -- needs a definition outside the class.  A class template's
// may sit in a header; from C++17 the members are inline and these would be
// redundant.
template<typename Algorithm>
constexpr DSortAlgorithm DSorter<Algorithm>::algorithm;

template<typename Algorithm>
constexpr bool DSorter<Algorithm>::is_stable;

template<typename Algorithm>
constexpr bool DSorter<Algorithm>::is_in_place;

template<typename Algorithm>
constexpr bool DSorter<Algorithm>::is_adaptive;

#endif  // !C++17

// DSorter<DDynamicAlgorithm>
//   class: a sort whose algorithm is a DSortAlgorithm held at run time,
// resolved through internal::dispatch_runtime on every call.
template<>
class DSorter<DDynamicAlgorithm>
{
public:
    // DSorter
    //   constructor: selects DDefaultSortTag's algorithm.
    DSorter()
        : m_algorithm(DDefaultSortTag::value)
    {}

    // DSorter
    //   constructor: selects _algorithm.
    explicit DSorter(
        DSortAlgorithm _algorithm
    )
        : m_algorithm(_algorithm)
    {}

    // algorithm
    //   function: the selected algorithm.
    DSortAlgorithm algorithm() const
    {
        return m_algorithm;
    }

    // set_algorithm
    //   function: selects _algorithm for the calls that follow.
    void set_algorithm(
        DSortAlgorithm _algorithm
    )
    {
        m_algorithm = _algorithm;

        return;
    }

    // is_stable, is_in_place, is_adaptive
    //   function: the selected algorithm's properties.
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
    //   function: the selected algorithm's name.
    const char* name() const
    {
        return name_of(m_algorithm);
    }

    // operator()(first, last, comp)
    //   function: sorts [_first, _last) under _comparator.
    template<typename RandomIterator,
             typename Comparator>
    void operator()(
        RandomIterator _first,
        RandomIterator _last,
        Comparator     _comparator
    ) const
    {
        internal::dispatch_runtime(m_algorithm,
                                   _first,
                                   _last,
                                   _comparator);

        return;
    }

    // operator()(first, last)
    //   function: sorts [_first, _last) ascending, under std::less.
    template<typename RandomIterator>
    void operator()(
        RandomIterator _first,
        RandomIterator _last
    ) const
    {
        typedef typename std::iterator_traits<RandomIterator>::value_type
            value_type;

        internal::dispatch_runtime(m_algorithm,
                                   _first,
                                   _last,
                                   std::less<value_type>());

        return;
    }

    // operator()(first, last, comp, order)
    //   function: sorts [_first, _last) under _comparator, in the direction
    // _order asks for.
    template<typename RandomIterator,
             typename Comparator>
    void operator()(
        RandomIterator    _first,
        RandomIterator    _last,
        Comparator        _comparator,
        sort_order::value _order
    ) const
    {
        internal::DOrderComparator<Comparator> wrapped(_comparator, _order);

        internal::dispatch_runtime(m_algorithm,
                                   _first,
                                   _last,
                                   wrapped);

        return;
    }

private:
    DSortAlgorithm m_algorithm;
};


///////////////////////////////////////////////////////////////////////////////
///                    VI.  ENTRY POINT                                     ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // DSortFunction
    //   class: the type of djinterp::sort.  Nine call operators in three rows
    // of three: the default algorithm, a tag, and a DSortAlgorithm, each with
    // (first, last), (first, last, comp) and (first, last, comp, order).  The
    // rows cannot collide: a tag or an enumerator is never an iterator of the
    // same type as the next argument, and the tag row is constrained to tags.
    class DSortFunction
    {
    public:
        // the default algorithm
        template<typename RandomIterator>
        void operator()(
            RandomIterator _first,
            RandomIterator _last
        ) const
        {
            DSorter<DDefaultSortTag>()(_first,
                                       _last);

            return;
        }

        template<typename RandomIterator,
                 typename Comparator>
        void operator()(
            RandomIterator _first,
            RandomIterator _last,
            Comparator     _comparator
        ) const
        {
            DSorter<DDefaultSortTag>()(_first,
                                       _last,
                                       _comparator);

            return;
        }

        template<typename RandomIterator,
                 typename Comparator>
        void operator()(
            RandomIterator    _first,
            RandomIterator    _last,
            Comparator        _comparator,
            sort_order::value _order
        ) const
        {
            DSorter<DDefaultSortTag>()(_first,
                                       _last,
                                       _comparator,
                                       _order);

            return;
        }

        // a tag: the algorithm fixed at compile time
        template<typename SortTag,
                 typename RandomIterator>
        typename std::enable_if<is_sort_tag<SortTag>::value>::type
        operator()(
            SortTag,
            RandomIterator _first,
            RandomIterator _last
        ) const
        {
            DSorter<SortTag>()(_first,
                               _last);

            return;
        }

        template<typename SortTag,
                 typename RandomIterator,
                 typename Comparator>
        typename std::enable_if<is_sort_tag<SortTag>::value>::type
        operator()(
            SortTag,
            RandomIterator _first,
            RandomIterator _last,
            Comparator     _comparator
        ) const
        {
            DSorter<SortTag>()(_first,
                               _last,
                               _comparator);

            return;
        }

        template<typename SortTag,
                 typename RandomIterator,
                 typename Comparator>
        typename std::enable_if<is_sort_tag<SortTag>::value>::type
        operator()(
            SortTag,
            RandomIterator    _first,
            RandomIterator    _last,
            Comparator        _comparator,
            sort_order::value _order
        ) const
        {
            DSorter<SortTag>()(_first,
                               _last,
                               _comparator,
                               _order);

            return;
        }

        // a DSortAlgorithm: the algorithm chosen at run time
        template<typename RandomIterator>
        void operator()(
            DSortAlgorithm _algorithm,
            RandomIterator _first,
            RandomIterator _last
        ) const
        {
            DSorter<DDynamicAlgorithm> sorter(_algorithm);

            sorter(_first,
                   _last);

            return;
        }

        template<typename RandomIterator,
                 typename Comparator>
        void operator()(
            DSortAlgorithm _algorithm,
            RandomIterator _first,
            RandomIterator _last,
            Comparator     _comparator
        ) const
        {
            DSorter<DDynamicAlgorithm> sorter(_algorithm);

            sorter(_first,
                   _last,
                   _comparator);

            return;
        }

        template<typename RandomIterator,
                 typename Comparator>
        void operator()(
            DSortAlgorithm    _algorithm,
            RandomIterator    _first,
            RandomIterator    _last,
            Comparator        _comparator,
            sort_order::value _order
        ) const
        {
            DSorter<DDynamicAlgorithm> sorter(_algorithm);

            sorter(_first,
                   _last,
                   _comparator,
                   _order);

            return;
        }
    };

NS_END  // internal

// sort
//   object: sorts a range, with the default algorithm, a tag's, or the one a
// DSortAlgorithm names:
//
//       sort(first, last[, comp[, order]])
//       sort(DMergeSortTag(), first, last[, comp[, order]])
//       sort(DSortAlgorithm::merge, first, last[, comp[, order]])
//
//   An object rather than a function template so that an unqualified call is
// never ambiguous with std::sort; see the banner.  One object per program
// from C++17, one per translation unit before -- it has no state, so only
// its address can tell them apart.
D_CONSTEXPR_INLINE_VAR internal::DSortFunction sort{};


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_UTIL_SORT_DISPATCH_HPP
