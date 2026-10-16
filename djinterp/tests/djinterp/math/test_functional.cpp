/*******************************************************************************
* djinterp [math]                                            test_functional.cpp
*
*
* path:      /tests/djinterp/math/test_functional.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// test_functional.cpp -- verifies the math <-> functional wiring.
//
// The real djinterp functional subframework can't be compiled here (it assumes
// a nested include tree and the full env). So we take ONLY the std-only adapter
// layer of the bridge (DJINTERP_MATH_FUNCTIONAL_NO_DEPS) and verify the
// integration contract against the documented semantics:
//   * is_callable<F, Args...>  ==  std::is_invocable<const F&, Args...>
//   * is_predicate<P, Arg>     ==  invocable with Arg, result convertible to bool
// The compose / pipe / curry stand-ins below mirror functional.md exactly.

#define DJINTERP_MATH_FUNCTIONAL_NO_DEPS 1
// std
#include <cstdio>
#include <type_traits>
// djinterp
#include "djinterp/math/functional.hpp"

using namespace djinterp::math;
namespace mf = djinterp::math::functional;

// ---------------------------------------------------------------------------
// Local stand-ins for djinterp::functional (sandbox only) -- documented behaviour
// ---------------------------------------------------------------------------
template<typename _F, typename _G>
struct composed                       // compose(f, g): math order f(g(x))
{
    _F f; _G g;
    template<typename _X>
    constexpr auto operator()(_X _x) const -> decltype(f(g(_x))) { return f(g(_x)); }
};
template<typename _F, typename _G>
constexpr composed<_F,_G> compose(_F _f, _G _g) { return { _f, _g }; }

template<typename _F, typename _G>
struct piped                          // pipe(f, g): left-to-right g(f(x))
{
    _F f; _G g;
    template<typename _X>
    constexpr auto operator()(_X _x) const -> decltype(g(f(_x))) { return g(f(_x)); }
};
template<typename _F, typename _G>
constexpr piped<_F,_G> pipe(_F _f, _G _g) { return { _f, _g }; }

template<typename _F>                 // curry a binary callable: f(a)(b)
struct curried2
{
    _F f;
    struct bound { _F f; double a;
        constexpr double operator()(double _b) const { return f(a, _b); } };
    constexpr bound operator()(double _a) const { return { f, _a }; }
};
template<typename _F>
constexpr curried2<_F> curry2(_F _f) { return { _f }; }

// ===========================================================================
int main()
{
    // ---- 1. raw expressions model is_callable (const-lvalue invocable) -----
    constexpr auto sq    = x * x;          // 1-var
    constexpr auto plane = x + y;          // 2-var
    static_assert(std::is_invocable_r<double, const decltype(sq)&,    double>::value,
                  "single-var expression is a unary callable (is_callable)");
    static_assert(std::is_invocable_r<double, const decltype(plane)&, double, double>::value,
                  "two-var expression is a binary callable (is_callable)");

    // ---- 2. a math relation models is_predicate (bool result) --------------
    constexpr auto positive = (x > constant(0.0));
    static_assert(std::is_invocable_r<bool, const decltype(positive)&, double>::value,
                  "relation is a unary predicate (is_predicate)");

    // ---- 3. adapters expose CONCRETE (introspectable) signatures -----------
    // Taking a member-function pointer of a fixed type only compiles when the
    // adapter's operator() is non-template -- which is the whole point, since
    // the raw expression's operator() is a template and is NOT introspectable.
    using au_t = mf::unary_adapter<decltype(sq)>;
    using ab_t = mf::binary_adapter<decltype(plane)>;
    using ap_t = mf::predicate_adapter<decltype(positive)>;
    double (au_t::*mpu)(double)         const = &au_t::operator(); (void)mpu;
    double (ab_t::*mpb)(double, double) const = &ab_t::operator(); (void)mpb;
    bool   (ap_t::*mpp)(double)         const = &ap_t::operator(); (void)mpp;

    // ---- 4. adapters compute correctly -------------------------------------
    constexpr auto au = mf::as_unary(sq);
    constexpr auto ab = mf::as_binary(plane);
    constexpr auto ap = mf::as_predicate(positive);
    static_assert(au(3.0) == 9.0,            "unary adapter");
    static_assert(ab(1.0, 2.0) == 3.0,       "binary adapter");
    static_assert(ap(1.0) && !ap(-1.0),      "predicate adapter");

    // ---- 5. flow through compose / pipe / curry ----------------------------
    // compose(sq, +1) at 2.0  ->  sq(2+1) = 9
    constexpr auto inc = mf::as_unary(x + constant(1.0));
    constexpr auto c   = compose(au, inc);
    static_assert(c(2.0) == 9.0,             "compose (math order)");
    // pipe(+1, sq) at 2.0  ->  sq(2+1) = 9
    constexpr auto p   = pipe(inc, au);
    static_assert(p(2.0) == 9.0,             "pipe (left-to-right)");
    // curry a 2-var expression: g(1)(2) = 3
    constexpr auto g   = curry2(ab);
    static_assert(g(1.0)(2.0) == 3.0,        "curry / partial application");

    printf("functional wiring OK: au(3)=%.0f  ab(1,2)=%.0f  ap(1)=%d ap(-1)=%d  "
           "compose=%.0f pipe=%.0f curry=%.0f\n",
           au(3.0), ab(1.0, 2.0), (int)ap(1.0), (int)ap(-1.0),
           c(2.0), p(2.0), g(1.0)(2.0));
    return 0;
}
