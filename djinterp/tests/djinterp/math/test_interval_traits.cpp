/*******************************************************************************
* djinterp [math]                                       test_interval_traits.cpp
*
*
* path:      /tests/djinterp/math/test_interval_traits.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// test_interval_traits.cpp -- verifies the discreteness traits discriminate
// correctly. All checks are static_asserts: if this compiles, it passes.
#include "djinterp/math/interval/closed_interval.hpp"
#include "djinterp/math/interval/open_interval.hpp"
#include "djinterp/math/interval/discrete_interval.hpp"
#include "djinterp/math/interval/interval.hpp"

using namespace djinterp::math;

// ---- standalone closed: continuous, NOT discrete (the original bug) ---------
static_assert( is_interval<closed_interval<int,0,5>>::value,            "closed is interval");
static_assert(!is_discrete_interval<closed_interval<int,0,5>>::value,   "closed not discrete");
static_assert( is_continuous_interval<closed_interval<int,0,5>>::value, "closed continuous");

// ---- standalone open: continuous, NOT discrete -----------------------------
static_assert( is_interval<open_interval<int,0,5>>::value,            "open is interval");
static_assert(!is_discrete_interval<open_interval<int,0,5>>::value,   "open not discrete");
static_assert( is_continuous_interval<open_interval<int,0,5>>::value, "open continuous");

// ---- standalone discrete: discrete -----------------------------------------
static_assert( is_interval<discrete_interval<int,0,10,2>>::value,             "discrete is interval");
static_assert( is_discrete_interval<discrete_interval<int,0,10,2>>::value,    "discrete IS discrete");
static_assert(!is_continuous_interval<discrete_interval<int,0,10,2>>::value,  "discrete not continuous");
// default step is 1 (> 0) -> still discrete
static_assert( is_discrete_interval<discrete_interval<int,0,10>>::value,      "discrete default step");

// ---- unified interval, continuous (step == 0): NOT discrete ----------------
// (this is the case mere presence-detection got wrong: the unified interval
//  always carries a step member, zero for continuous.)
static_assert( is_interval<interval<int,0,5>>::value,            "unified is interval");
static_assert(!is_discrete_interval<interval<int,0,5>>::value,   "unified continuous not discrete");
static_assert( is_continuous_interval<interval<int,0,5>>::value, "unified continuous");

// ---- unified interval, discrete (step > 0): discrete -----------------------
static_assert( is_discrete_interval<interval<int,0,10,false,false,2>>::value,   "unified discrete IS discrete");
static_assert(!is_continuous_interval<interval<int,0,10,false,false,2>>::value, "unified discrete not continuous");

// ---- open/half-open unified intervals remain continuous --------------------
static_assert(!is_discrete_interval<interval<int,0,5,true,true>>::value,  "unified open not discrete");
static_assert(!is_discrete_interval<interval<int,0,5,false,true>>::value, "unified half-open not discrete");

// ---- a non-interval type is neither ----------------------------------------
static_assert(!is_interval<double>::value,          "double not interval");
static_assert(!is_discrete_interval<double>::value, "double not discrete");

int main() { return 0; }
