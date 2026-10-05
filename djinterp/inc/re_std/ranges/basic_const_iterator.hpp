/*******************************************************************************
* djinterp [re_std]                                     basic_const_iterator.hpp
*
* forwarding header:
*   basic_const_iterator and iter_const_reference are defined once, in
* re_std/iterator/basic_const_iterator.hpp, beside the rest of <iterator>
* where std declares them; that is the copy re_std's ranges headers
* include. This file held a second, older copy, so a translation unit
* that included both redefined the class; it now forwards to the single
* definition, and its path keeps working.
*
*
* path:      /inc/re_std/ranges/basic_const_iterator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_RANGES_BASIC_CONST_ITERATOR_HPP
#define RE_STD_RANGES_BASIC_CONST_ITERATOR_HPP 1

// re_std
#include "../iterator/basic_const_iterator.hpp"  // basic_const_iterator

#endif  // RE_STD_RANGES_BASIC_CONST_ITERATOR_HPP
