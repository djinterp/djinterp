/*******************************************************************************
* djinterp [re_std]                                              enable_view.hpp
*
* enable_view customization point header:
*   Provides the customization-point variable template that classifies a
* type as a view (C++20). The default specialisation returns true iff
* the type publicly and unambiguously derives from view_base. Users may
* specialise enable_view<T> for their own types to opt in or out
* independently of any base-class relationship.
*
*   PORTABILITY:
*   - C++14+: real variable template (RE_STD_HAS_ENABLE_VIEW_VAR == 1).
*   - C++98/03/11: trait-struct fallback. enable_view<T>::value is the
*     equivalent boolean. The trait works on any conforming compiler.
*
*
* path:      /inc/re_std/ranges/enable_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_RANGES_ENABLE_VIEW_HPP
#define RE_STD_RANGES_ENABLE_VIEW_HPP 1

// FLOOR, FOR NOW: below C++11 this header is empty rather than an error
// (README rule 5; re_std omits rather than degrades). The owner's ruling:
// compile at every level first; port to C++98 only where something needs it.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"
#include "./view_base.hpp"


namespace re_std
{


// ===========================================================================
// 0.   DETECTION MACRO
// ===========================================================================

// RE_STD_HAS_ENABLE_VIEW_VAR
//   constant: 1 when enable_view is exposed as a constexpr bool
// variable template. 0 when only the trait-struct form is available.
#ifndef RE_STD_HAS_ENABLE_VIEW_VAR
    #if RE_STD_LANG_HAS_VARIABLE_TEMPLATES
        #define RE_STD_HAS_ENABLE_VIEW_VAR  1
    #else
        #define RE_STD_HAS_ENABLE_VIEW_VAR  0
    #endif
#endif


namespace internal
{


// ===========================================================================
// I.   DETECTION HELPER (derived-from-view_base test)
// ===========================================================================

// enable_view_base
//   trait: SFINAE-friendly base-class test. value is true when
// Type publicly and unambiguously derives from view_base.
// note: implemented via the classic conversion-overload idiom so it
// works on C++98/03 without compiler intrinsics. cv-qualifiers on
// Type are stripped via remove_cv to match the C++20 semantics
// (the variable template specialisation drops cv).
template<typename Type>
class enable_view_base
{
private:
    typedef char yes_type;
    struct       no_type { char pad[2]; };

    static yes_type test(view_base const volatile*);
    static no_type  test(...);

public:
    static const bool value =
        (sizeof(test(static_cast<Type*>(0))) == sizeof(yes_type));
};


}  // internal


// ===========================================================================
// II.  ENABLE_VIEW (primary trait)
// ===========================================================================

// enable_view (trait)
//   trait: true when Type is a view. Defaults to inheritance from
// view_base. Users may fully specialise this trait to opt their own
// types in or out without touching the inheritance hierarchy.
// note: the trait form is always present and is the back-port path
// for C++98/03/11 where variable templates are unavailable.
template<typename Type>
struct enable_view
    : integral_constant<bool,
                        internal::enable_view_base<
                            typename remove_cv<Type>::type
                        >::value>
{};


// ===========================================================================
// III. ENABLE_VIEW_V (variable template, C++14+)
// ===========================================================================

#if RE_STD_HAS_ENABLE_VIEW_VAR

// enable_view_v
//   variable: convenience constexpr accessor. Matches the C++20
// std::ranges::enable_view variable-template form (which in C++20 is
// itself the customisation point; the trait struct is re_std-specific
// for back-portability).
template<typename Type>
RE_STD_CONSTEXPR bool enable_view_v = enable_view<Type>::value;

#endif  // RE_STD_HAS_ENABLE_VIEW_VAR


}  // re_std

#endif  // floor, for now


#endif  // RE_STD_RANGES_ENABLE_VIEW_HPP
