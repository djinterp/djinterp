/******************************************************************************
* djinterp [re_std]                                             enable_view.hpp
*
* enable_view customization point header:
*   Provides the customization-point variable template that classifies a
* type as a view (C++20). The default specialisation returns true iff
* the type publicly and unambiguously derives from view_base. Users may
* specialise enable_view<T> for their own types to opt in or out
* independently of any base-class relationship.
*
*   PORTABILITY:
*   - C++14+: real variable template (D_RE_STD_HAS_ENABLE_VIEW_VAR == 1).
*   - C++98/03/11: trait-struct fallback. enable_view<T>::value is the
*     equivalent boolean. The trait works on any conforming compiler.
*
*
* path:      /inc/djinterp/re_std/ranges/enable_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_RANGES_ENABLE_VIEW_
#define DJINTERP_RE_STD_RANGES_ENABLE_VIEW_ 1

#include "../../core/djinterp.hpp"
#include "../type_traits/type_traits.hpp"
#include "./view_base.hpp"


NS_RESTD


// ===========================================================================
// 0.   DETECTION MACRO
// ===========================================================================

// D_RE_STD_HAS_ENABLE_VIEW_VAR
//   constant: 1 when enable_view is exposed as a constexpr bool
// variable template. 0 when only the trait-struct form is available.
#ifndef D_RE_STD_HAS_ENABLE_VIEW_VAR
    #if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
        #define D_RE_STD_HAS_ENABLE_VIEW_VAR  1
    #else
        #define D_RE_STD_HAS_ENABLE_VIEW_VAR  0
    #endif
#endif


NS_INTERNAL


// ===========================================================================
// I.   DETECTION HELPER (derived-from-view_base test)
// ===========================================================================

// enable_view_base
//   trait: SFINAE-friendly base-class test. value is true when
// _Type publicly and unambiguously derives from view_base.
// note: implemented via the classic conversion-overload idiom so it
// works on C++98/03 without compiler intrinsics. cv-qualifiers on
// _Type are stripped via remove_cv to match the C++20 semantics
// (the variable template specialisation drops cv).
template<typename _Type>
class enable_view_base
{
private:
    typedef char yes_type;
    struct       no_type { char pad[2]; };

    static yes_type test(view_base const volatile*);
    static no_type  test(...);

public:
    static const bool value =
        (sizeof(test(static_cast<_Type*>(0))) == sizeof(yes_type));
};


NS_END  // internal


// ===========================================================================
// II.  ENABLE_VIEW (primary trait)
// ===========================================================================

// enable_view (trait)
//   trait: true when _Type is a view. Defaults to inheritance from
// view_base. Users may fully specialise this trait to opt their own
// types in or out without touching the inheritance hierarchy.
// note: the trait form is always present and is the back-port path
// for C++98/03/11 where variable templates are unavailable.
template<typename _Type>
struct enable_view
    : integral_constant<bool,
                        internal::enable_view_base<
                            typename remove_cv<_Type>::type
                        >::value>
{};


// ===========================================================================
// III. ENABLE_VIEW_V (variable template, C++14+)
// ===========================================================================

#if D_RE_STD_HAS_ENABLE_VIEW_VAR

// enable_view_v
//   variable: convenience constexpr accessor. Matches the C++20
// std::ranges::enable_view variable-template form (which in C++20 is
// itself the customisation point; the trait struct is re_std-specific
// for back-portability).
template<typename _Type>
D_CONSTEXPR bool enable_view_v = enable_view<_Type>::value;

#endif  // D_RE_STD_HAS_ENABLE_VIEW_VAR


NS_END  // re_std


#endif  // DJINTERP_RE_STD_RANGES_ENABLE_VIEW_
