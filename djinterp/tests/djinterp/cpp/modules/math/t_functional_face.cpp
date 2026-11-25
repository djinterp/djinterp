/*******************************************************************************
* djinterp [cpp][math]                                     t_functional_face.cpp
*
*   Tests for math/functional.hpp, the bridge to the functional subframework,
* at every level from C++98 (step 30, 5b). Below C++11 the header is empty,
* as the subframework is. From C++11: the fixed-signature adapters, each
* call a constant expression; with the subframework itself, not stand-ins,
* compose (a constant expression too), pipe, curry and flip over adapters,
* a pipeline's filter and map over adapters and raw expressions, and
* is_callable and is_predicate of both.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++11 -Wall -Wextra -Werror -Wshadow -pedantic-errors -Iinc    \
*         tests/djinterp/cpp/modules/math/t_functional_face.cpp                \
*         -o t_functional -lm
*
*
* path:      /tests/djinterp/cpp/modules/math/t_functional_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.22
*                                                            revised: 2026.11.22
*******************************************************************************/
#include "../../../../../inc/djinterp/cpp/modules/math/functional.hpp"  // the
                                                              // tested header
// std
#include <cstdio>  // std::printf


namespace
{
    long g_checks   = 0;
    long g_failures = 0;

    void
    d_tests_expect(
        bool        _ok,
        const char* _what,
        int         _line
    )
    {
        ++g_checks;

        // report the first few failures with what was checked, and where
        if (!_ok)
        {
            ++g_failures;

            if (g_failures <= 40)
            {
                std::printf("  FAIL line %d: %s\n", _line, _what);
            }
        }

        return;
    }

    #define D_TESTS_EXPECT(ok, what)                                          \
        d_tests_expect((ok), (what), __LINE__)

}  // namespace


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <type_traits>  // std::is_same
#include <utility>      // std::declval
#include <vector>       // std::vector

namespace
{
    namespace dm = djinterp::math;
    namespace dv = djinterp::math::variables;
    namespace mf = djinterp::math::functional;


    //==========================================================================
    // 1.  THE ADAPTERS, CONSTANT EXPRESSIONS FROM C++11
    //==========================================================================

    D_CONSTEXPR double g_unary   = mf::as_unary(dv::x * dv::x)(3.0);
    D_CONSTEXPR double g_binary  = mf::as_binary(dv::x - dv::y)(3.0, 1.0);
    D_CONSTEXPR double g_ternary =
        mf::as_ternary( (dv::x * dv::y) + dv::z )(2.0, 3.0, 4.0);
    D_CONSTEXPR bool   g_holds   = mf::as_predicate(dv::x > 0.0)(1.0);
    D_CONSTEXPR bool   g_fails   = mf::as_predicate(dv::x > 0.0)(-1.0);
    D_CONSTEXPR float  g_float   = mf::as_unary<float>(dv::x * 2.0)(1.5f);

    // d_tests_adapters
    //   each adapter's call, folded at compile time; its fixed signature.
    void
    d_tests_adapters()
    {
        typedef decltype(mf::as_binary(dv::x + dv::y)) binary_type;

        D_TESTS_EXPECT( (g_unary == 9.0) &&
                        (g_binary == 2.0) &&
                        (g_ternary == 10.0) &&
                        (g_holds) &&
                        (!g_fails),
                        "as_unary, as_binary, as_ternary and as_predicate, "
                        "each call a constant expression from C++11");
        D_TESTS_EXPECT( (g_float == 3.0f) &&
                        (std::is_same<decltype(g_float), const float>::value) &&
                        (std::is_same<decltype(std::declval<binary_type>()(
                                          1.0, 2.0)),
                                      double>::value),
                        "an adapter's value type double by default, or the "
                        "one named");

        return;
    }


    //==========================================================================
    // 2.  WITH THE FUNCTIONAL SUBFRAMEWORK
    //==========================================================================

    // compose in math order, a constant expression from C++11
    D_CONSTEXPR double g_composed =
        djinterp::compose(mf::as_unary(dv::x * dv::x),
                          mf::as_unary(dv::x + 1.0))(2.0);

    // d_tests_subframework
    //   the subframework's combinators over adapters and raw expressions;
    // its call traits of both.
    void
    d_tests_subframework()
    {
        std::vector<double> values;

        values.push_back(-1.0);
        values.push_back(2.0);
        values.push_back(3.0);

        const std::vector<double> squares =
            djinterp::pipeline_from(values)
                .filter(mf::as_predicate(dv::x > 0.0))
                .map(mf::as_unary(dv::x * dv::x))
                .to_vector();
        const std::vector<double> raw =
            djinterp::pipeline_from(values)
                .map(dv::x + 1.0)
                .to_vector();
        const auto curried = djinterp::curry(mf::as_binary(dv::x - dv::y));
        const auto flipped = djinterp::flip(mf::as_binary(dv::x - dv::y));
        const auto piped   = djinterp::pipe(mf::as_unary(dv::x * dv::x),
                                            mf::as_unary(dv::x + 1.0));

        typedef decltype(dv::x * dv::x)               square_type;
        typedef decltype(dv::x > 0.0)                 positive_type;
        typedef mf::unary_adapter<square_type>        unary_type;
        typedef mf::predicate_adapter<positive_type>  predicate_type;

        D_TESTS_EXPECT( (g_composed == 9.0) &&
                        (piped(2.0) == 5.0),
                        "compose(f, g) f(g(x)), a constant expression from "
                        "C++11; pipe(f, g) g(f(x))");
        D_TESTS_EXPECT( (curried(10.0)(3.0) == 7.0) &&
                        (flipped(3.0, 10.0) == 7.0),
                        "curry and flip of a binary adapter, its arity read "
                        "from its fixed signature");
        D_TESTS_EXPECT( (squares.size() == 2) &&
                        (squares[0] == 4.0) &&
                        (squares[1] == 9.0) &&
                        (raw.size() == 3) &&
                        (raw[0] == 0.0) &&
                        (raw[2] == 4.0),
                        "a pipeline filtered by a relation and mapped by an "
                        "expression, adapted or raw");
        D_TESTS_EXPECT( (djinterp::is_callable<unary_type, double>::value) &&
                        (djinterp::is_callable<square_type, double>::value) &&
                        (djinterp::is_callable<decltype(dv::x + dv::y),
                                               double, double>::value) &&
                        (djinterp::is_predicate<predicate_type,
                                                double>::value) &&
                        (djinterp::is_predicate<positive_type,
                                                double>::value),
                        "is_callable of adapters and raw expressions; "
                        "is_predicate of predicate adapters and relations");

        return;
    }

}  // namespace

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


int
main()
{
    D_TESTS_EXPECT(DJINTERP_MATH_FUNCTIONAL_HPP == 1,
                   "the header reads at every level, empty below C++11");

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    d_tests_adapters();
    d_tests_subframework();
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    std::printf("t_functional_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
