/***********************************************************************
* restd                                                            numeric.hpp
*
* umbrella header for restd's <numeric> implementation.
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
*     standard permits parallel reordering. restd's implementations
*     are currently serial. Calling code that respects the
*     associativity contract will not need to change when restd
*     grows parallel infrastructure.
*   - accumulate() is a strict left-fold; its op need NOT be
*     associative. Use accumulate when iteration order matters.
*
*
* path:      /inc/restd/numeric.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.09
***********************************************************************/

#ifndef RESTD_NUMERIC_
#define RESTD_NUMERIC_ 1

#include "djinterp.hpp"

#include "restd/numeric/accumulate.hpp"
#include "restd/numeric/inner_product.hpp"
#include "restd/numeric/partial_sum.hpp"
#include "restd/numeric/adjacent_difference.hpp"
#include "restd/numeric/iota.hpp"
#include "restd/numeric/gcd.hpp"
#include "restd/numeric/lcm.hpp"
#include "restd/numeric/midpoint.hpp"
#include "restd/numeric/reduce.hpp"
#include "restd/numeric/transform_reduce.hpp"
#include "restd/numeric/inclusive_scan.hpp"
#include "restd/numeric/exclusive_scan.hpp"
#include "restd/numeric/transform_inclusive_scan.hpp"
#include "restd/numeric/transform_exclusive_scan.hpp"

#endif  // RESTD_NUMERIC_
