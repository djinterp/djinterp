/***********************************************************************
* re_std                                                           numeric.hpp
*
* umbrella header for re_std's <numeric> implementation.
*
* current contents:
*   serial folds:
*     accumulate (C++98), inner_product (C++98).
*   sequence-writing folds:
*     partial_sum (C++98), adjacent_difference (C++98).
*   sequence generation:
*     iota (C++11).
*   number theory:
*     gcd (C++17), lcm (C++17), midpoint (C++20, integer + pointer).
*   parallel-friendly folds (serial implementations):
*     reduce (C++17), transform_reduce (C++17),
*     inclusive_scan (C++17), exclusive_scan (C++17),
*     transform_inclusive_scan (C++17), transform_exclusive_scan (C++17).
*
* not yet implemented:
*   midpoint(float/double/long double)              -- focused phase pending
*   parallel-execution-policy overloads             -- await <execution>
*   C++26 saturation arithmetic: add_sat, sub_sat,
*     mul_sat, div_sat, saturate_cast               -- focused phase pending
*
* design notes:
*   - reduce() and the scan family REQUIRE associative ops; the
*     standard permits parallel reordering. re_std's implementations
*     are currently serial. Calling code that respects the
*     associativity contract will not need to change when re_std
*     grows parallel infrastructure.
*   - accumulate() is a strict left-fold; its op need NOT be
*     associative. Use accumulate when iteration order matters.
*
*
* path:      /inc/djinterp/re_std/numeric/numeric.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.09
***********************************************************************/

#ifndef DJINTERP_RE_STD_NUMERIC_
#define DJINTERP_RE_STD_NUMERIC_ 1

#include "djinterp.hpp"

#include "re_std/numeric/accumulate.hpp"
#include "re_std/numeric/inner_product.hpp"
#include "re_std/numeric/partial_sum.hpp"
#include "re_std/numeric/adjacent_difference.hpp"
#include "re_std/numeric/iota.hpp"
#include "re_std/numeric/gcd.hpp"
#include "re_std/numeric/lcm.hpp"
#include "re_std/numeric/midpoint.hpp"
#include "re_std/numeric/reduce.hpp"
#include "re_std/numeric/transform_reduce.hpp"
#include "re_std/numeric/inclusive_scan.hpp"
#include "re_std/numeric/exclusive_scan.hpp"
#include "re_std/numeric/transform_inclusive_scan.hpp"
#include "re_std/numeric/transform_exclusive_scan.hpp"

#endif  // DJINTERP_RE_STD_NUMERIC_
