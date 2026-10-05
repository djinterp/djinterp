/*******************************************************************************
* djinterp [re_std]                                           viewable_range.hpp
*
* viewable_range header:
*   Provides the C++20 viewable_range concept as a SFINAE trait.
* The C++20 definition is:
*
*       viewable_range<T> =
*           range<T> AND
*           ((view<remove_cvref<T>> AND constructible_from<remove_cvref<T>, T>)
*            OR (NOT view<remove_cvref<T>>
*                AND (is_lvalue_reference<T>
*                     OR (movable<remove_reference<T>>
*                         AND NOT is_initializer_list<remove_cvref<T>>))))
*
*   The disjunction captures "types that views::all knows how to
* turn into a view": already-a-view (pass through), lvalue range
* (ref_view), or movable non-initializer-list rvalue (owning_view).
*
*   SCOPE LIMITATION:
*   - movable is approximated with is_move_constructible (consistent
*     with the simplification already used in re_std::ranges::view).
*   - constructible_from is approximated with is_constructible.
*
*   PORTABILITY:
*   - C++11+; depends on range<R> (Phase R2), view<R> (Phase R2),
*     std::initializer_list (always available C++11+).
*   - viewable_range_v variable template on C++14+.
*
*
* path:      /inc/re_std/ranges/viewable_range.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_VIEWABLE_RANGE_HPP
#define RE_STD_RANGES_VIEWABLE_RANGE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <initializer_list>

#include "../type_traits/type_traits.hpp"
#include "./range.hpp"
#include "./view.hpp"


namespace re_std
{
// ===========================================================================
// I.   IS_INITIALIZER_LIST (internal)
// ===========================================================================

namespace internal
{

template<typename T>
struct is_initializer_list_raw : false_type
{};

template<typename U>
struct is_initializer_list_raw<std::initializer_list<U> > : true_type
{};

// is_initializer_list
//   trait: true iff T (after cv-stripping) is a specialisation of
// std::initializer_list.
template<typename T>
struct is_initializer_list
    : is_initializer_list_raw<
          typename remove_cv<T>::type
      >
{};

}  // internal


// ===========================================================================
// II.  VIEWABLE_RANGE
// ===========================================================================

namespace internal
{

    // viewable_range_compute
    //   trait: encodes the disjunction in a single value.
    template<typename T>
    struct viewable_range_compute
    {
    private:
        typedef typename remove_cv<
                              typename remove_reference<T>::type
                          >::type                                cvref_stripped;
        typedef typename remove_reference<T>::type              ref_stripped;

        static const bool _is_range   = range<T>::value;
        static const bool _is_view    = view<cvref_stripped>::value;
        static const bool _ctor_ok    = is_constructible<cvref_stripped, T>::value;
        static const bool _is_lref    = is_lvalue_reference<T>::value;
        static const bool _is_movable = is_move_constructible<ref_stripped>::value;
        static const bool _is_init_list = is_initializer_list<cvref_stripped>::value;

    public:
        static const bool value =
            _is_range
            && (
                ( _is_view && _ctor_ok)
                ||
                (!_is_view && (
                    _is_lref
                    || (_is_movable && !_is_init_list)
                ))
            );
    };
}  // internal
// viewable_range<T>
//   trait: true iff views::all(T) is well-formed.
template<typename T>
struct viewable_range
    : public integral_constant<
                 bool,
                 internal::viewable_range_compute<T>::value
             >
{};


#if RE_STD_LANG_IS_CPP14_OR_HIGHER
template<typename T>
RE_STD_CONSTEXPR bool viewable_range_v = viewable_range<T>::value;
#endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RANGES_VIEWABLE_RANGE_HPP
