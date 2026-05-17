/******************************************************************************
* djinterp [restd]                                          viewable_range.hpp
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
*     with the simplification already used in restd::ranges::view).
*   - constructible_from is approximated with is_constructible.
*
*   PORTABILITY:
*   - C++11+; depends on range<R> (Phase R2), view<R> (Phase R2),
*     std::initializer_list (always available C++11+).
*   - viewable_range_v variable template on C++14+.
*
*
* path:      /inc/djinterp/restd/ranges/viewable_range.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_VIEWABLE_RANGE_
#define DJINTERP_RESTD_RANGES_VIEWABLE_RANGE_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include <initializer_list>

#include "../type_traits/type_traits.hpp"
#include "./range.hpp"
#include "./view.hpp"


NS_RESTD


// ===========================================================================
// I.   IS_INITIALIZER_LIST (internal)
// ===========================================================================

NS_INTERNAL

template<typename _T>
struct is_initializer_list_raw : false_type
{};

template<typename _U>
struct is_initializer_list_raw<std::initializer_list<_U> > : true_type
{};

// is_initializer_list
//   trait: true iff _T (after cv-stripping) is a specialisation of
// std::initializer_list.
template<typename _T>
struct is_initializer_list
    : is_initializer_list_raw<
          typename remove_cv<_T>::type
      >
{};

NS_END  // internal


// ===========================================================================
// II.  VIEWABLE_RANGE
// ===========================================================================

namespace internal
{
    // viewable_range_compute
    //   helper: encodes the disjunction in a single value.
    template<typename _T>
    struct viewable_range_compute
    {
    private:
        typedef typename remove_cv<
                              typename remove_reference<_T>::type
                          >::type                                cvref_stripped;
        typedef typename remove_reference<_T>::type              ref_stripped;

        static const bool _is_range   = range<_T>::value;
        static const bool _is_view    = view<cvref_stripped>::value;
        static const bool _ctor_ok    = is_constructible<cvref_stripped, _T>::value;
        static const bool _is_lref    = is_lvalue_reference<_T>::value;
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
}  // namespace internal


// viewable_range<_T>
//   trait: true iff views::all(_T) is well-formed.
template<typename _T>
struct viewable_range
    : public integral_constant<
                 bool,
                 internal::viewable_range_compute<_T>::value
             >
{};


#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _T>
D_CONSTEXPR bool viewable_range_v = viewable_range<_T>::value;
#endif


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_RANGES_VIEWABLE_RANGE_
