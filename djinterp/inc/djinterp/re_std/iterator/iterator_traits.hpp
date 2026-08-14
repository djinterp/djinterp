/***********************************************************************
* restd                                                    iterator_traits.hpp
*
* extracts the five (or six, on C++20+) type aliases that an iterator
* exposes, in a uniform way regardless of whether _Iter is a class
* iterator with member typedefs, a raw pointer, or something
* iterator-traits-shaped that lacks the member typedefs.
*
* exposed members (when _Iter is a valid iterator):
*   value_type           - element type the iterator points to
*   difference_type      - signed type of (it1 - it2)
*   pointer              - the iterator's pointer type
*   reference            - the iterator's reference type
*   iterator_category    - restd tag (NOT std), see translation below
*   iterator_concept     - C++20+, optional
*
* primary template behaviour matches C++17+ std::iterator_traits: if
* _Iter does not expose the required member typedefs, the primary
* template is EMPTY (no nested types). Earlier std behaviour
* (pre-C++17) defined the typedefs unconditionally, causing hard
* errors when _Iter wasn't actually an iterator; restd does not
* reproduce that footgun.
*
* TAG TRANSLATION:
*   iterator_traits ALWAYS yields a restd tag for iterator_category,
*   even when the wrapped iterator's own category is a std tag (as is
*   the case for std::vector::iterator, std::list::iterator, etc.).
*   This means restd algorithms can tag-dispatch consistently against
*   restd tags regardless of where the iterator came from.
*
*   Translation is most-specific-first: a std::random_access_iterator_tag
*   becomes restd::random_access_iterator_tag, not the merely-derivable
*   restd::input_iterator_tag.
*
*   When the iterator's category is already a restd tag, translation is
*   a no-op (the std-base checks all fail — restd tags don't derive
*   from std tags).
*
*   Translation uses std::is_base_of, which is unavoidable here: the
*   only reason the std category is in scope is that <iterator> has
*   been included by whoever defined the iterator, so <type_traits>'s
*   is_base_of is also available. Localised exception to the no-std-
*   traits rule, justified by necessity.
*
* specialisations:
*   iterator_traits<_T*>           random-access (or contiguous on C++20+)
*   iterator_traits<const _T*>     same, with const _T as value_type
*
*
* path:      /inc/djinterp/re_std/iterator/iterator_traits.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.08
***********************************************************************/

#ifndef RESTD_ITERATOR_ITERATOR_TRAITS_
#define RESTD_ITERATOR_ITERATOR_TRAITS_ 1

#include "djinterp.hpp"

#include <cstddef>
#include <iterator>      // for std::*_iterator_tag (translation source)
#include <type_traits>   // for std::is_base_of (translation lookup)

#include "restd/iterator/input_iterator_tag.hpp"
#include "restd/iterator/output_iterator_tag.hpp"
#include "restd/iterator/forward_iterator_tag.hpp"
#include "restd/iterator/bidirectional_iterator_tag.hpp"
#include "restd/iterator/random_access_iterator_tag.hpp"
#include "restd/type_traits/void_t.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    #include "restd/iterator/contiguous_iterator_tag.hpp"
#endif


namespace restd
{
namespace internal
{

    // ---- tag translation ----
    //
    // Map a (possibly-std, possibly-restd) iterator-category tag to
    // its restd equivalent. Cascade is most-specific-first: a
    // random_access_iterator_tag must NOT match the input arm even
    // though it derives from it.
    //
    // The default arm (no std-base match) returns the input tag
    // unchanged, which means restd tags pass through and unknown
    // user-defined tags pass through too. Both behaviours are
    // intentional.

    template<typename _Cat, typename = void>
    struct translate_tag
    {
        // Default: no std base matched. Pass through unchanged.
        typedef _Cat type;
    };

    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        template<typename _Cat>
        struct translate_tag
        <
            _Cat,
            typename std::enable_if
            <
                std::is_base_of<std::contiguous_iterator_tag, _Cat>::value
            >::type
        >
        {
            typedef contiguous_iterator_tag type;
        };
    #endif

    // The C++20 contiguous specialisation above is more specialised
    // than the random_access one below, so when both are viable
    // (i.e. cat derives from contiguous) the contiguous one wins.
    // Pre-C++20, the contiguous spec doesn't exist, so random_access
    // is the most-specialised viable option — exactly what we want.
    template<typename _Cat>
    struct translate_tag
    <
        _Cat,
        typename std::enable_if
        <
            std::is_base_of<std::random_access_iterator_tag, _Cat>::value
            #if D_ENV_LANG_IS_CPP20_OR_HIGHER
                && !std::is_base_of<std::contiguous_iterator_tag, _Cat>::value
            #endif
        >::type
    >
    {
        typedef random_access_iterator_tag type;
    };

    template<typename _Cat>
    struct translate_tag
    <
        _Cat,
        typename std::enable_if
        <
            std::is_base_of<std::bidirectional_iterator_tag, _Cat>::value
            && !std::is_base_of<std::random_access_iterator_tag, _Cat>::value
        >::type
    >
    {
        typedef bidirectional_iterator_tag type;
    };

    template<typename _Cat>
    struct translate_tag
    <
        _Cat,
        typename std::enable_if
        <
            std::is_base_of<std::forward_iterator_tag, _Cat>::value
            && !std::is_base_of<std::bidirectional_iterator_tag, _Cat>::value
        >::type
    >
    {
        typedef forward_iterator_tag type;
    };

    template<typename _Cat>
    struct translate_tag
    <
        _Cat,
        typename std::enable_if
        <
            std::is_base_of<std::input_iterator_tag, _Cat>::value
            && !std::is_base_of<std::forward_iterator_tag, _Cat>::value
        >::type
    >
    {
        typedef input_iterator_tag type;
    };

    template<typename _Cat>
    struct translate_tag
    <
        _Cat,
        typename std::enable_if
        <
            std::is_base_of<std::output_iterator_tag, _Cat>::value
        >::type
    >
    {
        typedef output_iterator_tag type;
    };


    // ---- detection: does _Iter expose all five member typedefs? ----

    template<typename _Iter, typename = void>
    struct iter_traits_impl
    {
        // primary: empty (matches C++17+ std behaviour)
    };

    template<typename _Iter>
    struct iter_traits_impl
    <
        _Iter,
        typename void_t
        <
            typename _Iter::value_type,
            typename _Iter::difference_type,
            typename _Iter::pointer,
            typename _Iter::reference,
            typename _Iter::iterator_category
        >::type
    >
    {
        typedef typename _Iter::value_type        value_type;
        typedef typename _Iter::difference_type   difference_type;
        typedef typename _Iter::pointer           pointer;
        typedef typename _Iter::reference         reference;

        // Tag translation happens here.
        typedef typename translate_tag
        <
            typename _Iter::iterator_category
        >::type                                    iterator_category;
    };

}  // namespace internal


// ---- public iterator_traits ----

template<typename _Iter>
struct iterator_traits : public internal::iter_traits_impl<_Iter>
{
};


// ---- raw-pointer specialisations ----

template<typename _T>
struct iterator_traits<_T*>
{
    typedef _T                          value_type;
    typedef std::ptrdiff_t              difference_type;
    typedef _T*                         pointer;
    typedef _T&                         reference;
    typedef random_access_iterator_tag  iterator_category;

    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        typedef contiguous_iterator_tag iterator_concept;
    #endif
};

template<typename _T>
struct iterator_traits<const _T*>
{
    // value_type stays _T (not const _T) — matches std behaviour. The
    // const-ness lives in pointer/reference, where it matters for
    // assignment.
    typedef _T                          value_type;
    typedef std::ptrdiff_t              difference_type;
    typedef const _T*                   pointer;
    typedef const _T&                   reference;
    typedef random_access_iterator_tag  iterator_category;

    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        typedef contiguous_iterator_tag iterator_concept;
    #endif
};


}  // namespace restd

#endif  // RESTD_ITERATOR_ITERATOR_TRAITS_
