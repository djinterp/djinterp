/******************************************************************************
* djinterp [restd]                                                view_base.hpp
*
* view_base tag header:
*   Provides the empty marker class historically used to opt a type
* into the C++20 view concept. A range that publicly inherits from
* view_base is treated as a view by the default specialisation of
* enable_view.
*
*   PORTABILITY:
*   Standalone empty class. Available unconditionally on C++98+.
*   The base-class mechanism is the original C++20 opt-in. C++20
* shipped a derived_from<view_base> + enable_view variable-template
* customisation point; restd mirrors both (variable template lives in
* enable_view.hpp).
*
*
* path:      /inc/djinterp/restd/ranges/view_base.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_VIEW_BASE_
#define DJINTERP_RESTD_RANGES_VIEW_BASE_ 1

#include "../../core/djinterp.hpp"


NS_RESTD


// ===========================================================================
// I.   VIEW_BASE
// ===========================================================================

// view_base
//   class: empty marker. A class type publicly deriving from
// view_base is considered a view by the default enable_view
// specialisation. Note that view_interface CRTP already provides
// this base via its own publicly-inherited view_base, so types
// inheriting from view_interface<D> do not need to inherit
// view_base directly.
class view_base
{};


NS_END  // restd


#endif  // DJINTERP_RESTD_RANGES_VIEW_BASE_
