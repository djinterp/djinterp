/*******************************************************************************
* djinterp [re_std]                                          iterator_traits.hpp
*
* iterator_traits class header:
* extracts the five (or six, on C++20+) type aliases that an iterator
* exposes, in a uniform way regardless of whether Iter is a class
* iterator with member typedefs, a raw pointer, or something
* iterator-traits-shaped that lacks the member typedefs.
*
* exposed members (when Iter is a valid iterator):
*   value_type           - element type the iterator points to
*   difference_type      - signed type of (it1 - it2)
*   pointer              - the iterator's pointer type
*   reference            - the iterator's reference type
*   iterator_category    - re_std tag (NOT std), see translation below
*   iterator_concept     - C++20+, optional
*
* primary template behaviour matches C++17+ std::iterator_traits: if
* Iter does not expose the required member typedefs, the primary
* template is EMPTY (no nested types). Earlier std behaviour
* (pre-C++17) defined the typedefs unconditionally, causing hard
* errors when Iter wasn't actually an iterator; re_std does not
* reproduce that footgun.
*
* TAG TRANSLATION:
*   iterator_traits ALWAYS yields a re_std tag for iterator_category,
*   even when the wrapped iterator's own category is a std tag (as is
*   the case for std::vector::iterator, std::list::iterator, etc.).
*   This means re_std algorithms can tag-dispatch consistently against
*   re_std tags regardless of where the iterator came from.
*
*   Translation is most-specific-first: a std::random_access_iterator_tag
*   becomes re_std::random_access_iterator_tag, not the merely-derivable
*   re_std::input_iterator_tag.
*
*   When the iterator's category is already a re_std tag, translation is
*   a no-op (the std-base checks all fail — re_std tags don't derive
*   from std tags).
*
*   Translation uses re_std's own is_base_of and enable_if, which work on
*   std's tag types like any others, so no C++11 <type_traits> is needed:
*   only <iterator>, which C++98 has, for the std tags themselves.
*
* specialisations:
*   iterator_traits<T*>           random-access (or contiguous on C++20+)
*   iterator_traits<const T*>     same, with const T as value_type
*
*
* path:      /inc/re_std/iterator/iterator_traits.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.08
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ITERATOR_ITERATOR_TRAITS_HPP
#define RE_STD_ITERATOR_ITERATOR_TRAITS_HPP 1

// std
#include <cstddef>
#include <iterator>      // for std::*_iterator_tag (translation source)
// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "re_std/iterator/input_iterator_tag.hpp"
#include "re_std/iterator/output_iterator_tag.hpp"
#include "re_std/iterator/forward_iterator_tag.hpp"
#include "re_std/iterator/bidirectional_iterator_tag.hpp"
#include "re_std/iterator/random_access_iterator_tag.hpp"
#include "../type_traits/enable_if.hpp"   // enable_if (translation)
#include "../type_traits/is_base_of.hpp"  // is_base_of (translation)

#if RE_STD_LANG_IS_CPP20_OR_HIGHER
    #include "re_std/iterator/contiguous_iterator_tag.hpp"
#endif


namespace re_std
{
namespace internal
{

    // ---- tag translation ----
    //
    // Map a (possibly-std, possibly-re_std) iterator-category tag to
    // its re_std equivalent. Cascade is most-specific-first: a
    // random_access_iterator_tag must NOT match the input arm even
    // though it derives from it.
    //
    // The default arm (no std-base match) returns the input tag
    // unchanged, which means re_std tags pass through and unknown
    // user-defined tags pass through too. Both behaviours are
    // intentional.

    template<typename Cat, typename = void>
    struct translate_tag
    {
        // Default: no std base matched. Pass through unchanged.
        typedef Cat type;
    };

    #if RE_STD_LANG_IS_CPP20_OR_HIGHER
        template<typename Cat>
        struct translate_tag
        <
            Cat,
            typename re_std::enable_if
            <
                re_std::is_base_of<std::contiguous_iterator_tag, Cat>::value
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
    template<typename Cat>
    struct translate_tag
    <
        Cat,
        typename re_std::enable_if
        <
            re_std::is_base_of<std::random_access_iterator_tag, Cat>::value
            #if RE_STD_LANG_IS_CPP20_OR_HIGHER
                && !re_std::is_base_of<std::contiguous_iterator_tag, Cat>::value
            #endif
        >::type
    >
    {
        typedef random_access_iterator_tag type;
    };

    template<typename Cat>
    struct translate_tag
    <
        Cat,
        typename re_std::enable_if
        <
            re_std::is_base_of<std::bidirectional_iterator_tag, Cat>::value
            && !re_std::is_base_of<std::random_access_iterator_tag, Cat>::value
        >::type
    >
    {
        typedef bidirectional_iterator_tag type;
    };

    template<typename Cat>
    struct translate_tag
    <
        Cat,
        typename re_std::enable_if
        <
            re_std::is_base_of<std::forward_iterator_tag, Cat>::value
            && !re_std::is_base_of<std::bidirectional_iterator_tag, Cat>::value
        >::type
    >
    {
        typedef forward_iterator_tag type;
    };

    template<typename Cat>
    struct translate_tag
    <
        Cat,
        typename re_std::enable_if
        <
            re_std::is_base_of<std::input_iterator_tag, Cat>::value
            && !re_std::is_base_of<std::forward_iterator_tag, Cat>::value
        >::type
    >
    {
        typedef input_iterator_tag type;
    };

    template<typename Cat>
    struct translate_tag
    <
        Cat,
        typename re_std::enable_if
        <
            re_std::is_base_of<std::output_iterator_tag, Cat>::value
        >::type
    >
    {
        typedef output_iterator_tag type;
    };


    // ---- detection: does Iter expose all five member typedefs? ----

    // iter_traits_sink
    //   trait: void for any five types -- the detection sink. Five fixed
    // parameters rather than void_t's pack, so detection needs no C++11
    // feature and iterator_traits exists from C++98, as std's does.
    template<typename A,
             typename B,
             typename C,
             typename D,
             typename E>
    struct iter_traits_sink
    {
        typedef void type;
    };

    template<typename Iter, typename = void>
    struct iter_traits_impl
    {
        // primary: empty (matches C++17+ std behaviour)
    };

    template<typename Iter>
    struct iter_traits_impl
    <
        Iter,
        typename iter_traits_sink
        <
            typename Iter::value_type,
            typename Iter::difference_type,
            typename Iter::pointer,
            typename Iter::reference,
            typename Iter::iterator_category
        >::type
    >
    {
        typedef typename Iter::value_type        value_type;
        typedef typename Iter::difference_type   difference_type;
        typedef typename Iter::pointer           pointer;
        typedef typename Iter::reference         reference;

        // Tag translation happens here.
        typedef typename translate_tag
        <
            typename Iter::iterator_category
        >::type                                    iterator_category;
    };

}  // internal
// ---- public iterator_traits ----

template<typename Iter>
struct iterator_traits : public internal::iter_traits_impl<Iter>
{
};


// ---- raw-pointer specialisations ----

template<typename T>
struct iterator_traits<T*>
{
    typedef T                          value_type;
    typedef std::ptrdiff_t              difference_type;
    typedef T*                         pointer;
    typedef T&                         reference;
    typedef random_access_iterator_tag  iterator_category;

    #if RE_STD_LANG_IS_CPP20_OR_HIGHER
        typedef contiguous_iterator_tag iterator_concept;
    #endif
};

template<typename T>
struct iterator_traits<const T*>
{
    // value_type stays T (not const T) — matches std behaviour. The
    // const-ness lives in pointer/reference, where it matters for
    // assignment.
    typedef T                          value_type;
    typedef std::ptrdiff_t              difference_type;
    typedef const T*                   pointer;
    typedef const T&                   reference;
    typedef random_access_iterator_tag  iterator_category;

    #if RE_STD_LANG_IS_CPP20_OR_HIGHER
        typedef contiguous_iterator_tag iterator_concept;
    #endif
};


}  // re_std
#endif  // RE_STD_ITERATOR_ITERATOR_TRAITS_HPP
