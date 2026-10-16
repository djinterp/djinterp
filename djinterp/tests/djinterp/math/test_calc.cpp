/*******************************************************************************
* djinterp [math]                                                  test_calc.cpp
*
*
* path:      /tests/djinterp/math/test_calc.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <array>
#include <cmath>
// test_calc.cpp -- exercises the math/calculus subframework.
#include <cstdio>
// djinterp
#include "djinterp/math/calculus/calculus.hpp"

using namespace djinterp::math;

// ---- tiny harness ----------------------------------------------------------
static int g_pass = 0;
static int g_fail = 0;

static bool approx(double a, double b, double tol = 1e-6)
{
    double d = a - b;
    if (d < 0) d = -d;
    return d <= tol;
}
static void check(bool ok, const char* name)
{
    if (ok) { ++g_pass; }
    else    { ++g_fail; std::printf("  FAIL: %s\n", name); }
}
static void check_close(double got, double want, double tol, const char* name)
{
    bool ok = approx(got, want, tol);
    if (ok) { ++g_pass; }
    else    { ++g_fail; std::printf("  FAIL: %s (got %.12g want %.12g)\n",
                                    name, got, want); }
}

// constexpr functors for the constexpr integration / summation proofs
struct sqr_fn   { D_CONSTEXPR double operator()(double x) const { return x * x; } };
struct ident_fn { D_CONSTEXPR double operator()(long k)   const { return static_cast<double>(k); } };
struct nsq_fn   { D_CONSTEXPR double operator()(std::size_t n) const
                  { double d = static_cast<double>(n); return d * d; } };

// ============================================================================
int main()
{
    constexpr double PI = constants::pi<double>;
    constexpr double E  = constants::e<double>;

    // ====================================================================
    // 1. SYMBOLIC DIFFERENTIATION
    // ====================================================================
    {
        // d/dx sin(x) = cos(x)
        auto d_sin = derivative<0>(sin(x));
        check_close(d_sin(0.7), std::cos(0.7), 1e-9, "d/dx sin = cos");

        // d/dx cos(x) = -sin(x)
        auto d_cos = derivative<0>(cos(x));
        check_close(d_cos(0.7), -std::sin(0.7), 1e-9, "d/dx cos = -sin");

        // product rule: d/dx (x sin x) = sin x + x cos x
        auto d_prod = derivative<0>(x * sin(x));
        check_close(d_prod(1.3), std::sin(1.3) + 1.3 * std::cos(1.3),
                    1e-9, "product rule");

        // quotient rule: d/dx (sin x / x) = (x cos x - sin x)/x^2
        auto d_quot = derivative<0>(sin(x) / x);
        check_close(d_quot(1.3),
                    (1.3 * std::cos(1.3) - std::sin(1.3)) / (1.3 * 1.3),
                    1e-9, "quotient rule");

        // chain rule: d/dx sin(x^2) = 2x cos(x^2)
        auto d_chain = derivative<0>(sin(squared(x)));
        check_close(d_chain(1.1), 2.0 * 1.1 * std::cos(1.1 * 1.1),
                    1e-9, "chain rule");

        // d/dx exp(x) = exp(x)
        auto d_exp = derivative<0>(exp(x));
        check_close(d_exp(0.9), std::exp(0.9), 1e-9, "d/dx exp = exp");

        // d/dx ln(x) = 1/x
        auto d_log = derivative<0>(log(x));
        check_close(d_log(2.5), 1.0 / 2.5, 1e-9, "d/dx ln = 1/x");

        // d/dx atan(x) = 1/(1+x^2)
        auto d_atan = derivative<0>(atan(x));
        check_close(d_atan(0.8), 1.0 / (1.0 + 0.64), 1e-9, "d/dx atan");

        // d/dx sqrt(x) = 1/(2 sqrt x)
        auto d_sqrt = derivative<0>(sqrt(x));
        check_close(d_sqrt(4.0), 1.0 / (2.0 * 2.0), 1e-9, "d/dx sqrt");

        // d/dx |x| = sign(x)
        auto d_abs = derivative<0>(abs(x));
        check_close(d_abs(3.0),  1.0, 1e-9, "d/dx |x| (x>0)");
        check_close(d_abs(-3.0), -1.0, 1e-9, "d/dx |x| (x<0)");

        // composed: d/dx sinh(x) = cosh(x)
        auto d_sinh = derivative<0>(sinh(x));
        check_close(d_sinh(0.6), std::cosh(0.6), 1e-9, "d/dx sinh = cosh");

        // composed: d/dx x^3 = 3 x^2  (pow via exp/log, x>0)
        auto d_pow = derivative<0>(pow(x, constant(3.0)));
        check_close(d_pow(2.0), 3.0 * 4.0, 1e-7, "d/dx x^3 = 3x^2");

        // constexpr proof: the derivative evaluates at compile time
        constexpr auto dce = derivative<0>(sin(x));
        constexpr double v = dce(0.0);      // cos(0) = 1
        static_assert(v > 0.999 && v < 1.001, "constexpr derivative");
    }

    // ====================================================================
    // 2. HIGHER-ORDER + GRADIENT
    // ====================================================================
    {
        // d^2/dx^2 sin(x) = -sin(x)
        auto d2 = nth_derivative<0, 2>(sin(x));
        check_close(d2(1.0), -std::sin(1.0), 1e-9, "2nd derivative of sin");

        auto d2b = second_derivative<0>(cos(x));
        check_close(d2b(1.0), -std::cos(1.0), 1e-9, "second_derivative of cos");

        // gradient of f(x,y) = x^2 + y^2  is (2x, 2y)
        auto f  = squared(x) + squared(y);
        auto gr = gradient<2>(f);
        auto gv = gr(3.0, 4.0);
        check_close(gv[0], 6.0, 1e-9, "gradient df/dx");
        check_close(gv[1], 8.0, 1e-9, "gradient df/dy");
    }

    // ====================================================================
    // 3. NUMERICAL DIFFERENTIATION
    // ====================================================================
    {
        sqr_fn s;
        check_close(numeric_derivative(s, 3.0), 6.0, 1e-4, "numeric d/dx x^2");

        // numeric partials of x^2 + y^2 at (3,4)
        auto f = squared(x) + squared(y);
        check_close(numeric_partial<0>(f, std::array<double,2>{{3.0, 4.0}}),
                    6.0, 1e-4, "numeric partial x");
        check_close(numeric_partial<1>(f, std::array<double,2>{{3.0, 4.0}}),
                    8.0, 1e-4, "numeric partial y");

        // symbolic vs numeric cross-check on sin(x) exp(x)
        auto g  = sin(x) * exp(x);
        auto dg = derivative<0>(g);
        check_close(dg(0.5), numeric_derivative(
                        [](double t){ return std::sin(t) * std::exp(t); }, 0.5),
                    1e-4, "symbolic == numeric");
    }

    // ====================================================================
    // 4. INTEGRATION
    // ====================================================================
    {
        sqr_fn s;
        // integral of x^2 over [0,1] = 1/3
        check_close(trapezoid(s, 0.0, 1.0, 2000), 1.0/3.0, 1e-5, "trapezoid x^2");
        check_close(simpson  (s, 0.0, 1.0, 200),  1.0/3.0, 1e-9, "simpson x^2");
        check_close(romberg<7>(s, 0.0, 1.0),      1.0/3.0, 1e-9, "romberg x^2");

        // Gauss-Legendre is exact for x^2 -> 1/3 (constexpr)
        constexpr double gl = gauss_legendre5(sqr_fn{}, 0.0, 1.0);
        static_assert(gl > 0.33333332 && gl < 0.33333334, "constexpr gauss x^2");
        check_close(gl, 1.0/3.0, 1e-12, "gauss x^2");

        // romberg is constexpr too
        constexpr double rb = romberg<6>(sqr_fn{}, 0.0, 1.0);
        static_assert(rb > 0.33332 && rb < 0.33334, "constexpr romberg x^2");

        // integral of sin over [0, pi] = 2  (expression overload, constexpr)
        constexpr double Isin = integrate(sin(x), 0.0, PI, 256);
        static_assert(Isin > 1.999 && Isin < 2.001, "constexpr integral of sin");
        check_close(Isin, 2.0, 1e-3, "integrate(sin) over [0,pi]");
    }

    // ====================================================================
    // 5. SEQUENCES
    // ====================================================================
    {
        constexpr arithmetic_sequence<double> a(2.0, 3.0);
        static_assert(a.term(4) == 14.0, "arithmetic term");
        static_assert(a.partial_sum(5) == 40.0, "arithmetic partial sum");

        constexpr geometric_sequence<double> g(1.0, 2.0);
        static_assert(g.term(10) == 1024.0, "geometric term");
        static_assert(g.partial_sum(10) == 1023.0, "geometric partial sum");

        constexpr fibonacci_sequence fib;
        static_assert(fib.term(10) == 55ull, "fibonacci term");

        constexpr power_sequence<double> p(2);
        static_assert(p.term(5) == 25.0, "power sequence term");

        // general functor-backed sequence
        constexpr auto sq = make_sequence(nsq_fn{});
        static_assert(sq.term(6) == 36.0, "general sequence term");
        check(true, "sequences");
    }

    // ====================================================================
    // 6. SERIES
    // ====================================================================
    {
        // sum_{k=1}^{100} k = 5050  (constexpr)
        constexpr double s = sum(ident_fn{}, 1, 100);
        static_assert(s == 5050.0, "sum 1..100");

        // free partial_sum over a sequence
        constexpr arithmetic_sequence<double> a(2.0, 3.0);
        static_assert(partial_sum(a, 5) == 40.0, "free partial_sum");

        // closed forms
        static_assert(geometric_sum(1.0, 2.0, 10) == 1023.0, "geometric_sum");
        static_assert(arithmetic_sum(2.0, 3.0, 5) == 40.0, "arithmetic_sum");
        check_close(geometric_sum_infinite(1.0, 0.5), 2.0, 1e-12, "geom infinite");

        // Taylor: e^1 via Maclaurin of exp, exact symbolic derivatives
        constexpr double te = maclaurin_eval<8>(exp(x), 1.0);
        static_assert(te > 2.71 && te < 2.72, "constexpr maclaurin exp");
        check_close(te, E, 1e-3, "maclaurin exp(1)");

        // Taylor of sin about 0 at 0.5
        check_close(maclaurin_eval<7>(sin(x), 0.5), std::sin(0.5),
                    1e-6, "maclaurin sin(0.5)");
    }

    // ====================================================================
    // 7. ELEMENTARY FUNCTIONS / IDENTITIES (fn::)
    // ====================================================================
    {
        // combinatorial / number-theoretic (constexpr)
        static_assert(fn::factorial(5) == 120ull, "factorial");
        static_assert(fn::binomial(6, 2) == 15ull, "binomial");
        static_assert(fn::gcd(12, 18) == 6, "gcd");
        static_assert(fn::lcm(4, 6) == 12, "lcm");

        // algebraic / rounding
        check_close(fn::abs(-2.5), 2.5, 0.0, "fn::abs");
        check(fn::sign(-3.0) == -1, "fn::sign");
        check_close(fn::floor(2.7), 2.0, 0.0, "fn::floor");
        check_close(fn::ceil(2.1),  3.0, 0.0, "fn::ceil");
        check_close(fn::round(2.5), 3.0, 0.0, "fn::round");
        check_close(fn::clamp(5.0, 0.0, 1.0), 1.0, 0.0, "fn::clamp");
        check_close(fn::radians(180.0), PI, 1e-12, "fn::radians");
        check_close(fn::degrees(PI), 180.0, 1e-9, "fn::degrees");

        // horner: 3x^2 + 2x + 1 at x=2 -> 17
        std::array<double,3> coeffs{{3.0, 2.0, 1.0}};
        check_close(fn::horner(coeffs, 2.0), 17.0, 0.0, "fn::horner");

        // scalar transcendentals (vs cmath)
        check_close(fn::cos(0.0), 1.0, 1e-12, "fn::cos(0)");
        check_close(fn::sec(0.0), 1.0, 1e-12, "fn::sec(0)");
        check_close(fn::tanh(0.7), std::tanh(0.7), 1e-6, "fn::tanh");
        check_close(fn::log(E), 1.0, 1e-9, "fn::log(e)");
        check_close(fn::atan2(1.0, 1.0), PI / 4.0, 1e-6, "fn::atan2");
        check_close(fn::pow(2.0, 10.0), 1024.0, 1e-6, "fn::pow");

        // constexpr proof for a scalar transcendental
        constexpr double c0 = fn::cos(0.0);
        static_assert(c0 > 0.999 && c0 < 1.001, "constexpr fn::cos");
    }

    // ====================================================================
    std::printf("\ncalculus: %d passed, %d failed\n", g_pass, g_fail);
    if (g_fail == 0) { std::printf("ALL CALCULUS CHECKS PASSED\n"); }
    return g_fail == 0 ? 0 : 1;
}
