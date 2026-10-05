/*******************************************************************************
* djinterp [re_std]                                                      cmp.hpp
*
* forwarding header:
*   The sign-safe integer comparisons -- cmp_equal, cmp_not_equal,
* cmp_less, cmp_greater, cmp_less_equal, cmp_greater_equal and in_range --
* are defined once, in re_std/utility/intcmp.hpp, which utility.hpp
* includes. This file held a second implementation of the same seven
* functions, so a translation unit that included both redefined them; it
* now forwards to the single definition, and its path keeps working.
*
*
* path:      /inc/re_std/utility/cmp.hpp
* link(s):   TBA
* author(s): re_std team                                     created: 2026.05.02
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_UTILITY_CMP_HPP
#define RE_STD_UTILITY_CMP_HPP 1

// re_std
#include "./intcmp.hpp"  // cmp_equal ... in_range

#endif  // RE_STD_UTILITY_CMP_HPP
