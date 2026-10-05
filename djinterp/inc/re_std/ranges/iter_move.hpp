/*******************************************************************************
* djinterp [re_std]                                                iter_move.hpp
*
* forwarding header:
*   iter_move and iter_rvalue_reference are defined once, in
* re_std/iterator/iter_move.hpp, beside the rest of <iterator> where std
* declares them. This file held a second, older copy, so a translation
* unit that included both redefined every name in it; it now forwards to
* the single definition, and its path keeps working.
*
*
* path:      /inc/re_std/ranges/iter_move.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_RANGES_ITER_MOVE_HPP
#define RE_STD_RANGES_ITER_MOVE_HPP 1

// re_std
#include "../iterator/iter_move.hpp"  // iter_move, iter_rvalue_reference

#endif  // RE_STD_RANGES_ITER_MOVE_HPP
