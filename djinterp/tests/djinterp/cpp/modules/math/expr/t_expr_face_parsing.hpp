/*******************************************************************************
* djinterp [cpp][math]                                   t_expr_face_parsing.hpp
*
* One part of t_expr_face.cpp, the expression language's face test (S438), its
* sections 8 and 9: both folds, and parsing and rendering. The test includes
* the parts in order, one program; each part includes the one before it.
*
*
* path:      /tests/djinterp/cpp/modules/math/expr/t_expr_face_parsing.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.21
*                                                            revised: 2026.11.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_CPP_MATH_EXPR_T_EXPR_FACE_PARSING_HPP
#define DJINTERP_TESTS_CPP_MATH_EXPR_T_EXPR_FACE_PARSING_HPP 1

#include "./t_expr_face_types.hpp"      // the part before


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


namespace
{

    //==========================================================================
    // 8.  FOLDS
    //==========================================================================

    // d_tests_bindings
    //   expr_bindings, at, and expr_context.
    void
    d_tests_bindings()
    {
        const m::expr_bindings none;

        D_TESTS_EXPECT( (none.empty()) &&
                        (none.size() == 0) &&
                        (none.find("x") == D_NULLPTR),
                        "bindings made by default: none");

        m::expr_bindings two = m::at("x", 2.0).at("n", 3);

        D_TESTS_EXPECT( (two.size() == 2) &&
                        (two.find("x") != D_NULLPTR) &&
                        (*two.find("x") == m::value(2.0)) &&
                        (two.find("x")->kind() == D_MATH_VALUE_DOUBLE) &&
                        (two.find("n") != D_NULLPTR) &&
                        (*two.find("n") == m::value(3)) &&
                        (two.find("y") == D_NULLPTR),
                        "at binds, and chains");

        two.at("x", m::value::ratio(1, 3));

        D_TESTS_EXPECT( (two.size() == 2) &&
                        (two.find("x") != D_NULLPTR) &&
                        (*two.find("x") == m::value::ratio(1, 3)),
                        "at binds a bound name anew");

        const m::expr_context context;
        static_cast<void>(context);

        D_TESTS_EXPECT( (std::is_default_constructible<
                             m::expr_context>::value),
                        "a context made by default");

        return;
    }

    // d_tests_each_type
    //   whether each operation, and each way a leaf has its value, gives in
    // Type what it gives in double: - and the unary +, mod with and without
    // its corrections, a NaN for a value with no nearest, a literal no
    // decimal, and no value for a term made by default.
    template<typename Type>
    bool
    d_tests_each_type()
    {
        const Type nan = std::numeric_limits<Type>::quiet_NaN();
        const Type one = Type(1);
        const Type six = Type(6);

        return ( (d_tests_typed<Type>("7 - 2", Type(5)))                     &&
                 (d_tests_typed<Type>("+x", Type(3), m::at("x", 3)))         &&
                 (d_tests_typed<Type>("-7 mod 3", Type(2)))                  &&
                 (d_tests_typed<Type>("6 mod -3", -Type(0)))                 &&
                 (d_tests_typed<Type>("x", one, m::at("x", true)))           &&
                 (d_tests_typed<Type>("x", nan, m::at("x", m::value())))     &&
                 (m::expr_evaluate<Type>(m::expr_number(
                      m::value::ratio(1, 6), "1/6")).is_ok())                &&
                 (d_tests_same_real(
                      m::expr_evaluate<Type>(m::expr_number(
                          m::value::ratio(1, 6), "1/6")).value(),
                      one / six))                                            &&
                 (d_tests_is_error(m::expr_evaluate<Type>(m::expr_term()),
                                   m::expr_status::missing_operand,
                                   m::expr_no_offset,
                                   "a term made by default has no value")) );
    }

    // d_tests_fold_typed
    //   expr_evaluate<Type>: each literal rounded once; + - * / the type's;
    // ^ the C core's pow; mod floored; a variable its value's nearest.
    void
    d_tests_fold_typed()
    {
        const m::expr_bindings x3 = m::at("x", 3);
        volatile float         f1 = 1.0f;
        volatile float         f3 = 3.0f;
        volatile double        d1 = 1.0;
        volatile double        d3 = 3.0;
        volatile long double   l1 = 1.0L;
        volatile long double   l3 = 3.0L;
        volatile double        a  = 0.1;
        volatile double        b  = 0.2;

        D_TESTS_EXPECT( d_tests_typed<float>("2 * x ^ 2 + 1", 19.0f, x3) &&
                        d_tests_typed<double>("2 * x ^ 2 + 1", 19.0, x3) &&
                        d_tests_typed<long double>("2 * x ^ 2 + 1",
                                                   19.0L,
                                                   x3),
                        "2 * x ^ 2 + 1 at x = 3: 19 in each type");
        D_TESTS_EXPECT( d_tests_typed<float>("1 / 3", f1 / f3) &&
                        d_tests_typed<double>("1 / 3", d1 / d3) &&
                        d_tests_typed<long double>("1 / 3", l1 / l3),
                        "1 / 3: the type's own quotient");
        D_TESTS_EXPECT( d_tests_typed<double>("0.1 + 0.2", a + b),
                        "0.1 + 0.2 in double: each literal rounded, then "
                        "the sum");
        D_TESTS_EXPECT( d_tests_typed<float>(
                            "1.00000005960464477539062500000001",
                            1.0f + FLT_EPSILON),
                        "a literal rounded once to float, not through "
                        "double");
        D_TESTS_EXPECT( d_tests_typed<double>("7 mod 3", 1.0) &&
                        d_tests_typed<double>("-7 mod 3", 2.0) &&
                        d_tests_typed<double>("7 mod -3", -2.0) &&
                        d_tests_typed<double>("-7 mod -3", -1.0) &&
                        d_tests_typed<float>("7.5 mod 2", 1.5f) &&
                        d_tests_typed<long double>("-0.5 mod 1", 0.5L),
                        "mod floored: the divisor's sign");
        D_TESTS_EXPECT( d_tests_typed<double>("6 mod 3", 0.0) &&
                        d_tests_typed<double>("6 mod -3", -0.0) &&
                        d_tests_typed<double>(
                            "5 mod 0",
                            std::numeric_limits<double>::quiet_NaN()),
                        "mod: no remainder 0 with the divisor's sign; by 0 a "
                        "NaN");
        D_TESTS_EXPECT( d_tests_typed<double>(
                            "x mod y",
                            std::numeric_limits<double>::infinity(),
                            m::at("x", -1).at(
                                "y",
                                std::numeric_limits<double>::infinity())),
                        "mod: fmod plus the divisor, as Python's: -1 mod inf "
                        "is inf");

        volatile double two  = 2.0;
        volatile double half = 0.5;

        D_TESTS_EXPECT( d_tests_typed<double>("2 ^ 10", 1024.0) &&
                        d_tests_typed<double>("2 ^ -2", 0.25) &&
                        d_tests_typed<double>("0 ^ 0", 1.0) &&
                        d_tests_typed<double>("2 ^ 0.5",
                                              d_math_pow(two, half)),
                        "^ the C core's pow");
        D_TESTS_EXPECT( d_tests_typed<double>(
                            "(-8) ^ (1 / 3)",
                            std::numeric_limits<double>::quiet_NaN()) &&
                        d_tests_typed<double>(
                            "0 ^ -1",
                            std::numeric_limits<double>::infinity()) &&
                        d_tests_typed<double>(
                            "1 / 0",
                            std::numeric_limits<double>::infinity()) &&
                        d_tests_typed<double>(
                            "0 / 0",
                            std::numeric_limits<double>::quiet_NaN()),
                        "the type's IEEE answers where arithmetic fails");
        D_TESTS_EXPECT( d_tests_typed<double>("-x", -0.0, m::at("x", 0)) &&
                        d_tests_typed<double>("+x", 3.0, x3) &&
                        d_tests_typed<double>("--x", 3.0, x3),
                        "the unary - and +");
        D_TESTS_EXPECT( d_tests_typed<double>("x",
                                              d1 / d3,
                                              m::at("x",
                                                    m::value::ratio(1, 3))) &&
                        d_tests_typed<float>("x", 0.1f, m::at("x", 0.1)),
                        "a variable: its value's nearest in the type");
        D_TESTS_EXPECT( d_tests_typed<double>(
                            "x + 1",
                            std::numeric_limits<double>::quiet_NaN(),
                            m::at("x", D_MATH_VALUE_ERROR_POLE)) &&
                        d_tests_typed<double>(
                            "x + 1",
                            std::numeric_limits<double>::quiet_NaN(),
                            m::at("x", m::value())),
                        "a value with no nearest, an error or none: a NaN");
        D_TESTS_EXPECT( d_tests_typed<double>("x + 1", 2.0, m::at("x", true)) &&
                        d_tests_typed<float>("x + 1",
                                             1.0f,
                                             m::at("x", false)) &&
                        d_tests_typed<double>("x",
                                              0.0,
                                              m::at("x", false)) &&
                        d_tests_typed<long double>(
                            "x",
                            1.0L,
                            m::at("x", m::value(true))),
                        "a boolean: 1 or 0, as true and false are (2d)");

        // a number a fold made, and one whose literal is no decimal
        const m::expr_term third = m::expr_number(m::value::ratio(1, 3));
        const m::expr_term odd   = m::expr_number(m::value::ratio(1, 3),
                                                  "1/3");
        const m::expr_result<double> got_third =
            m::expr_evaluate<double>(third);
        const m::expr_result<double> got_odd = m::expr_evaluate<double>(odd);

        D_TESTS_EXPECT( (got_third.is_ok()) &&
                        (d_tests_same_real(got_third.value(), d1 / d3)) &&
                        (got_odd.is_ok()) &&
                        (d_tests_same_real(got_odd.value(), d1 / d3)),
                        "a number with no decimal literal: its value's "
                        "nearest");
        D_TESTS_EXPECT( (d_tests_each_type<float>()) &&
                        (d_tests_each_type<double>()) &&
                        (d_tests_each_type<long double>()),
                        "each operation, and each leaf's way, in each "
                        "type");

        return;
    }

    // d_tests_fold_value
    //   expr_evaluate to a value: exact where the value layer is; ^ exact
    // for whole exponents and for rational ones of whole roots; mod exact;
    // the tower's kind otherwise.
    void
    d_tests_fold_value()
    {
        const m::value third = m::value::ratio(1, 3);

        D_TESTS_EXPECT( d_tests_valued("1/3 + 1/6", m::value::ratio(1, 2)) &&
                        d_tests_valued("0.1 + 0.2", m::value::ratio(3, 10)),
                        "1/3 + 1/6 exactly 1/2; 0.1 + 0.2 exactly 3/10");
        D_TESTS_EXPECT( d_tests_valued("2 * x ^ 2 + 1",
                                       m::value(19),
                                       m::at("x", 3)),
                        "2 * x ^ 2 + 1 at x = 3: the integer 19");

        const m::value overflow = d_tests_value_of("9223372036854775807 + 1");

        D_TESTS_EXPECT( (overflow.kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                        ((overflow.flags() & D_MATH_VALUE_FLAG_INEXACT) != 0),
                        "an exact sum past the family: G2's long double, "
                        "flagged");

        // ^, exact
        D_TESTS_EXPECT( d_tests_valued("2 ^ 10", m::value(1024)) &&
                        d_tests_valued("2 ^ -2", m::value::ratio(1, 4)) &&
                        d_tests_valued("(2 / 3) ^ 3", m::value::ratio(8, 27)) &&
                        d_tests_valued("(-2) ^ 3", m::value(-8)) &&
                        d_tests_valued("0 ^ 0", m::value(1)),
                        "^ to a whole power: exact");
        D_TESTS_EXPECT( d_tests_valued("4 ^ 0.5", m::value(2)) &&
                        d_tests_valued("8 ^ (1 / 3)", m::value(2)) &&
                        d_tests_valued("(1 / 4) ^ 0.5",
                                       m::value::ratio(1, 2)) &&
                        d_tests_valued("4 ^ 1.5", m::value(8)) &&
                        d_tests_valued("(4 / 9) ^ -0.5",
                                       m::value::ratio(3, 2)) &&
                        d_tests_valued("0 ^ 0.5", m::value(0)),
                        "^ to a rational power of whole roots: exact");
        D_TESTS_EXPECT( d_tests_valued("0 ^ -1",
                                       m::value(D_MATH_VALUE_ERROR_POLE)) &&
                        d_tests_valued("0 ^ -0.5",
                                       m::value(D_MATH_VALUE_ERROR_POLE)) &&
                        m::expr_is_complex(
                            d_tests_value_of("(-8) ^ (1 / 3)")) &&
                        d_tests_valued("(-4) ^ (1 / 2)",
                                       m::expr_complex(m::value(0),
                                                       m::value(2))),
                        "^: exact 0 to a negative power a pole; a negative "
                        "base to a power not whole its principal complex "
                        "value, exact where it is a Gaussian rational (S272, "
                        "S337)");

        volatile long double two  = 2.0L;
        volatile long double half = 0.5L;
        volatile long double some = 0.333L;
        const m::value       big  = d_tests_value_of("3 ^ 40");

        D_TESTS_EXPECT( d_tests_valued("2 ^ 0.5",
                                       m::value(d_math_pow(two, half))) &&
                        d_tests_valued("2 ^ 0.333",
                                       m::value(d_math_pow(two, some))),
                        "^ of exact operands not exact: the C core's pow in "
                        "long double");
        D_TESTS_EXPECT( (big.kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                        ((big.flags() & D_MATH_VALUE_FLAG_INEXACT) != 0),
                        "3 ^ 40, past the family: G2's long double, flagged");
        D_TESTS_EXPECT( d_tests_valued("x ^ 2",
                                       m::value(2.25),
                                       m::at("x", 1.5)) &&
                        d_tests_valued("2 ^ x",
                                       m::value(d_math_pow(2.0f, 0.5f)),
                                       m::at("x", 0.5f)) &&
                        d_tests_valued("x ^ y",
                                       m::value(d_math_pow(2.0, 0.5)),
                                       m::at("x", 2).at("y", 0.5)),
                        "^ with an inexact operand: in the wider inexact "
                        "kind");

        // mod
        D_TESTS_EXPECT( d_tests_valued("7 mod 3", m::value(1)) &&
                        d_tests_valued("-7 mod 3", m::value(2)) &&
                        d_tests_valued("7 mod -3", m::value(-2)) &&
                        d_tests_valued("7.5 mod 2", m::value::ratio(3, 2)) &&
                        d_tests_valued("-7.5 mod 2", m::value::ratio(1, 2)),
                        "mod of exact operands: exact, floored");
        D_TESTS_EXPECT( d_tests_valued("5 mod 0",
                                       m::value(D_MATH_VALUE_ERROR_DOMAIN)),
                        "mod by exact 0: a domain error");
        D_TESTS_EXPECT( d_tests_valued("x mod 2",
                                       m::value(1.5f),
                                       m::at("x", 7.5f)) &&
                        d_tests_valued("x mod 2",
                                       m::value(0.5f),
                                       m::at("x", -7.5f)) &&
                        d_tests_valued("x mod y",
                                       m::value(0.5),
                                       m::at("x", third * m::value(3) +
                                                      m::value::ratio(1, 2))
                                           .at("y", 1.0)),
                        "mod with an inexact operand: in its kind, "
                        "floored");

        const m::value far_mod = d_tests_value_of(
            "9223372036854775807 mod (1 / 2)");

        D_TESTS_EXPECT( (far_mod.kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                        (far_mod == m::value(0.0L)),
                        "mod whose exact quotient is past the family: in "
                        "long double");

        return;
    }

    // d_tests_fold_passing
    //   errors and what is no number, through + ^ and mod; flags carried;
    // and a boxed operand.
    void
    d_tests_fold_passing()
    {
        const m::value pole   = m::value(D_MATH_VALUE_ERROR_POLE);
        const m::value domain = m::value(D_MATH_VALUE_ERROR_DOMAIN);

        D_TESTS_EXPECT( d_tests_valued("x + 1", pole, m::at("x", pole)) &&
                        d_tests_valued("x ^ 2", pole, m::at("x", pole)) &&
                        d_tests_valued("2 ^ x", pole, m::at("x", pole)) &&
                        d_tests_valued("x mod 2", pole, m::at("x", pole)) &&
                        d_tests_valued("2 mod x", pole, m::at("x", pole)) &&
                        d_tests_valued("+x", pole, m::at("x", pole)),
                        "an error passes through each operation");
        D_TESTS_EXPECT( d_tests_valued("x ^ y",
                                       pole,
                                       m::at("x", pole).at("y", domain)) &&
                        d_tests_valued("x mod y",
                                       domain,
                                       m::at("x", domain).at("y", pole)),
                        "the first error, of two");
        D_TESTS_EXPECT( d_tests_valued("x + 1", domain, m::at("x", true)) &&
                        d_tests_valued("x ^ 2", domain, m::at("x", true)) &&
                        d_tests_valued("2 ^ x",
                                       domain,
                                       m::at("x", m::value())) &&
                        d_tests_valued("x mod 2",
                                       domain,
                                       m::at("x", m::value())) &&
                        d_tests_valued("2 mod x", domain, m::at("x", false)) &&
                        d_tests_valued("+x", domain, m::at("x", true)),
                        "a boolean or none in arithmetic: a domain error");
        D_TESTS_EXPECT( d_tests_valued("+x", m::value(2), m::at("x", 2)) &&
                        d_tests_valued("-x", m::value(-2), m::at("x", 2)),
                        "the unary + a number itself, and -");

        const m::value flagged = d_tests_value_of("9223372036854775807 + 1");
        const m::expr_bindings at_flagged = m::at("x", flagged);

        D_TESTS_EXPECT( ((d_tests_value_of("x ^ 0.5", at_flagged).flags() &
                          D_MATH_VALUE_FLAG_INEXACT) != 0) &&
                        ((d_tests_value_of("x mod 3", at_flagged).flags() &
                          D_MATH_VALUE_FLAG_INEXACT) != 0) &&
                        ((d_tests_value_of("2 ^ x", at_flagged).flags() &
                          D_MATH_VALUE_FLAG_INEXACT) != 0),
                        "^ and mod in the tower's kind carry their "
                        "operands' flags");

        // a boxed operand, a wide integer
        const m::value wide = d_tests_wide_of("100000000000000000000");
        const m::value square = d_tests_value_of("x ^ 2", m::at("x", wide));
        const m::value rest   = d_tests_value_of("x mod 7", m::at("x", wide));

        D_TESTS_EXPECT( (square == d_tests_wide_of(
                             "10000000000000000000000000000000000000000")) &&
                        (square.is_exact()),
                        "a wide integer to a whole power: exact, by its "
                        "kind's products");
        D_TESTS_EXPECT( (rest.kind() == D_MATH_VALUE_LONG_DOUBLE),
                        "a wide integer mod an integer: in long double");

        return;
    }

    // d_tests_functions_in
    //   whether each standard function in Type is the C core's plain form
    // in Type, at arguments each type holds exactly; abs |x|; factorial n!.
    template<typename Type>
    bool
    d_tests_functions_in()
    {
        volatile Type half    = Type(0.5);
        volatile Type three_2 = Type(1.5);
        volatile Type two     = Type(2);
        volatile Type minus   = Type(-2.5);

        return ( (d_tests_typed<Type>("sqrt(2)", d_math_sqrt(two)))         &&
                 (d_tests_typed<Type>("cbrt(-2.5)", d_math_cbrt(minus)))    &&
                 (d_tests_typed<Type>("sin(0.5)", d_math_sin(half)))        &&
                 (d_tests_typed<Type>("cos(0.5)", d_math_cos(half)))        &&
                 (d_tests_typed<Type>("tan(0.5)", d_math_tan(half)))        &&
                 (d_tests_typed<Type>("asin(0.5)", d_math_asin(half)))      &&
                 (d_tests_typed<Type>("acos(0.5)", d_math_acos(half)))      &&
                 (d_tests_typed<Type>("atan(0.5)", d_math_atan(half)))      &&
                 (d_tests_typed<Type>("sinh(0.5)", d_math_sinh(half)))      &&
                 (d_tests_typed<Type>("cosh(0.5)", d_math_cosh(half)))      &&
                 (d_tests_typed<Type>("tanh(0.5)", d_math_tanh(half)))      &&
                 (d_tests_typed<Type>("exp(0.5)", d_math_exp(half)))        &&
                 (d_tests_typed<Type>("exp2(0.5)", d_math_exp2(half)))      &&
                 (d_tests_typed<Type>("log(1.5)", d_math_log(three_2)))     &&
                 (d_tests_typed<Type>("log2(1.5)", d_math_log2(three_2)))   &&
                 (d_tests_typed<Type>("log10(1.5)", d_math_log10(three_2))) &&
                 (d_tests_typed<Type>("floor(-2.5)", Type(-3)))             &&
                 (d_tests_typed<Type>("ceil(-2.5)", Type(-2)))              &&
                 (d_tests_typed<Type>("round(-2.5)", Type(-3)))             &&
                 (d_tests_typed<Type>("trunc(-2.5)", Type(-2)))             &&
                 (d_tests_typed<Type>("abs(-2.5)", Type(2.5)))              &&
                 (d_tests_typed<Type>("factorial(5)", Type(120)))           &&
                 (d_tests_typed<Type>("atan2(1.5, -2.5)",
                                      d_math_atan2(three_2, minus)))        &&
                 (d_tests_typed<Type>("hypot(1.5, 2)",
                                      d_math_hypot(three_2, two)))          &&
                 (d_tests_typed<Type>("pow(2, 0.5)", d_math_pow(two, half))) &&
                 (d_tests_typed<Type>("fmod(-8, 3)", Type(-2)))             &&
                 (d_tests_typed<Type>("remainder(7, 2)", Type(-1))) );
    }

    // d_tests_fold_functions
    //   the fold in a type: each standard function the C core's; the
    // factorial; the constants; calls in the arithmetic.
    void
    d_tests_fold_functions()
    {
        const double inf = std::numeric_limits<double>::infinity();
        const double nan = std::numeric_limits<double>::quiet_NaN();

        D_TESTS_EXPECT( d_tests_functions_in<float>(),
                        "each standard function in float: the C core's");
        D_TESTS_EXPECT( d_tests_functions_in<double>(),
                        "each standard function in double: the C core's");
        D_TESTS_EXPECT( d_tests_functions_in<long double>(),
                        "each standard function in long double: the C "
                        "core's");
        D_TESTS_EXPECT( d_tests_typed<double>("0!", 1.0) &&
                        d_tests_typed<double>("3!", 6.0) &&
                        d_tests_typed<double>("171!", inf) &&
                        d_tests_typed<float>("35!",
                                             std::numeric_limits<float>::
                                                 infinity()) &&
                        d_tests_typed<long double>(
                            "3000!",
                            std::numeric_limits<long double>::infinity()) &&
                        d_tests_typed<double>("x!", inf, m::at("x", inf)) &&
                        d_tests_typed<double>("2.5!", nan) &&
                        d_tests_typed<float>(
                            "(-1)!",
                            std::numeric_limits<float>::quiet_NaN()) &&
                        d_tests_typed<long double>(
                            "x!",
                            std::numeric_limits<long double>::quiet_NaN(),
                            m::at("x", nan)),
                        "n!: the product, an infinity past the type; a NaN "
                        "for a fraction, a negative number, a NaN");

        volatile double     big       = 1.0;
        volatile long double big_long = 1.0L;

        for (int factor = 2; factor <= 170; ++factor)
        {
            big      = big * factor;
            big_long = big_long * factor;
        }

        D_TESTS_EXPECT( d_tests_typed<double>("170!", big) &&
                        d_tests_typed<long double>("170!", big_long),
                        "170!: the product rounded as it is made");
        D_TESTS_EXPECT( d_tests_typed<double>("abs(x)", 0.0,
                                              m::at("x", -0.0)) &&
                        d_tests_typed<float>("pi",
                                             m::constants::of<float>::pi()) &&
                        d_tests_typed<double>("e",
                                              m::constants::of<double>::e()) &&
                        d_tests_typed<long double>(
                            "pi",
                            m::constants::of<long double>::pi()),
                        "abs(-0) is 0; pi and e, each type's");
        D_TESTS_EXPECT( d_tests_typed<double>("2 ^ 3! - sqrt(16) * e ^ 0",
                                              60.0) &&
                        d_tests_typed<double>("sin(x) ^ 2", 0.0,
                                              m::at("x", 0)),
                        "calls and the factorial in the arithmetic");

        return;
    }

    // d_tests_fold_functions_value
    //   the fold to a value: each function exact wherever its value is
    // rational, its domain's errors, and the tower's kind elsewhere.
    void
    d_tests_fold_functions_value()
    {
        const m::value domain = m::value(D_MATH_VALUE_ERROR_DOMAIN);
        const m::value pole   = m::value(D_MATH_VALUE_ERROR_POLE);
        const m::value zero   = m::value(0);
        const m::value one    = m::value(1);

        // the arguments of the C core's answers the folds are held to:
        // volatile, so that each answer is computed as the test runs, by the
        // C library as the fold's is, never by the compiler, which rounds
        // correctly where the library may be an ulp away (G6)
        volatile long double two_l       = 2.0L;
        volatile long double minus_two_l = -2.0L;
        volatile long double one_l       = 1.0L;
        volatile long double minus_one_l = -1.0L;
        volatile long double half_l      = 0.5L;
        volatile long double zero_l      = 0.0L;

        D_TESTS_EXPECT( d_tests_valued("sqrt(4/9)", m::value::ratio(2, 3)) &&
                        d_tests_valued("sqrt(0)", zero) &&
                        d_tests_valued("cbrt(-27/8)",
                                       m::value::ratio(-3, 2)) &&
                        d_tests_valued("hypot(3, 4)", m::value(5)) &&
                        d_tests_valued("hypot(5/2, 6)",
                                       m::value::ratio(13, 2)) &&
                        d_tests_valued("pow(4, 1/2)", m::value(2)),
                        "roots and powers: exact where the root is");
        D_TESTS_EXPECT( d_tests_valued("sqrt(2)",
                                       m::value(d_math_sqrt(two_l))) &&
                        d_tests_valued("cbrt(2)",
                                       m::value(d_math_cbrt(two_l))) &&
                        d_tests_valued("cbrt(-2)",
                                       m::value(d_math_cbrt(minus_two_l))) &&
                        d_tests_valued("hypot(1, 1)",
                                       m::value(d_math_hypot(one_l, one_l))),
                        "and a long double where it is not");
        D_TESTS_EXPECT( d_tests_valued("sin(0)", zero) &&
                        d_tests_valued("tan(0)", zero) &&
                        d_tests_valued("asin(0)", zero) &&
                        d_tests_valued("atan(0)", zero) &&
                        d_tests_valued("sinh(0)", zero) &&
                        d_tests_valued("tanh(0)", zero) &&
                        d_tests_valued("cos(0)", one) &&
                        d_tests_valued("cosh(0)", one) &&
                        d_tests_valued("exp(0)", one) &&
                        d_tests_valued("acos(1)", zero) &&
                        d_tests_valued("log(1)", zero) &&
                        d_tests_valued("atan2(0, 5)", zero),
                        "the transcendental functions exact at their one "
                        "rational point each");
        D_TESTS_EXPECT( d_tests_valued("sin(1/2)",
                                       m::value(d_math_sin(half_l))) &&
                        d_tests_valued("cos(2)",
                                       m::value(d_math_cos(two_l))) &&
                        d_tests_valued("cosh(1)",
                                       m::value(d_math_cosh(one_l))) &&
                        d_tests_valued("acos(0)",
                                       m::value(d_math_acos(zero_l))) &&
                        d_tests_valued("acos(1/2)",
                                       m::value(d_math_acos(half_l))) &&
                        d_tests_valued("log(1/2)",
                                       m::value(d_math_log(half_l))) &&
                        d_tests_valued("exp(1)",
                                       m::value(d_math_exp(one_l))) &&
                        d_tests_valued("atan2(0, -1)",
                                       m::value(d_math_atan2(zero_l,
                                                             minus_one_l))) &&
                        d_tests_valued("atan2(1, 1)",
                                       m::value(d_math_atan2(one_l, one_l))),
                        "and a long double elsewhere, beside the point too");
        D_TESTS_EXPECT( d_tests_valued("exp2(10)", m::value(1024)) &&
                        d_tests_valued("exp2(-2)", m::value::ratio(1, 4)) &&
                        d_tests_valued("log2(8)", m::value(3)) &&
                        d_tests_valued("log2(1/8)", m::value(-3)) &&
                        d_tests_valued("log10(1000)", m::value(3)) &&
                        d_tests_valued("log10(1/100)", m::value(-2)) &&
                        d_tests_valued("log10(2)",
                                       m::value(d_math_log10(two_l))) &&
                        d_tests_valued("exp2(1/2)",
                                       m::value(d_math_exp2(half_l))),
                        "exp2 at a whole number, log2 and log10 at a power "
                        "of 2 or 10: exact");
        D_TESTS_EXPECT( d_tests_valued("floor(-7/2)", m::value(-4)) &&
                        d_tests_valued("ceil(-7/2)", m::value(-3)) &&
                        d_tests_valued("round(-7/2)", m::value(-4)) &&
                        d_tests_valued("trunc(-7/2)", m::value(-3)) &&
                        d_tests_valued("round(5/2)", m::value(3)) &&
                        d_tests_valued("round(7/3)", m::value(2)) &&
                        d_tests_valued("ceil(7/3)", m::value(3)) &&
                        d_tests_valued("floor(3)", m::value(3)) &&
                        d_tests_valued("floor(-3)", m::value(-3)) &&
                        d_tests_valued("ceil(3)", m::value(3)) &&
                        d_tests_valued("abs(-3/4)", m::value::ratio(3, 4)),
                        "floor, ceil, round (halves away from 0), trunc and "
                        "abs: exact");
        D_TESTS_EXPECT( d_tests_valued("fmod(7, -3)", one) &&
                        d_tests_valued("fmod(-7, 3)", m::value(-1)) &&
                        d_tests_valued("fmod(7/2, 1)", m::value::ratio(1, 2)) &&
                        d_tests_valued("remainder(7, 2)", m::value(-1)) &&
                        d_tests_valued("remainder(5, 2)", one) &&
                        d_tests_valued("remainder(-7, 2)", one) &&
                        d_tests_valued("remainder(8, 3)", m::value(-1)),
                        "fmod truncated, remainder to the nearest, ties to "
                        "even: exact");
        D_TESTS_EXPECT( d_tests_valued("factorial(0)", one) &&
                        d_tests_valued("5!", m::value(120)) &&
                        d_tests_valued("20!",
                                       m::value(static_cast<dj_math_imax>(
                                           2432902008176640000LL))),
                        "factorial: exact, of a whole number");

        const m::value past     = d_tests_value_of("21!");
        const m::value past_all = d_tests_value_of("factorial(3000)");

        D_TESTS_EXPECT( (past.kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                        ((past.flags() & D_MATH_VALUE_FLAG_INEXACT) != 0) &&
                        (past == m::value(51090942171709440000.0L)) &&
                        (past_all.kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                        ((past_all.flags() & D_MATH_VALUE_FLAG_INEXACT) !=
                             0) &&
                        (past_all ==
                             m::value(std::numeric_limits<long double>::
                                          infinity())),
                        "21!, past the family: G2's long double, flagged; "
                        "3000!, past long double's range, its infinity");
        D_TESTS_EXPECT( d_tests_valued("factorial(-1)", domain) &&
                        d_tests_valued("(1/2)!", domain) &&
                        d_tests_valued("fmod(1, 0)", domain) &&
                        d_tests_valued("remainder(1, 0)", domain) &&
                        d_tests_valued("atan2(0, 0)", domain),
                        "an exact argument outside the domain, where no "
                        "complex value is either: a domain error");
        D_TESTS_EXPECT( d_tests_valued("sqrt(-4)",
                                       m::expr_complex(m::value(0),
                                                       m::value(2))) &&
                        m::expr_is_complex(d_tests_value_of("asin(2)")) &&
                        m::expr_is_complex(d_tests_value_of("acos(-3/2)")) &&
                        m::expr_is_complex(d_tests_value_of("log(-1)")),
                        "where the value is complex alone, the principal "
                        "complex one (S272)");
        D_TESTS_EXPECT( d_tests_valued("log(0)", pole) &&
                        d_tests_valued("log10(0)", pole),
                        "the logarithm of exact 0: a pole");

        // the tower's kind: the widest inexact argument's
        volatile float  f_half = 0.5f;
        volatile double d_half = 0.5;
        volatile double d_two  = 2.0;

        D_TESTS_EXPECT( d_tests_valued("sin(x)",
                                       m::value(d_tests_stored(
                                           d_math_sin(f_half))),
                                       m::at("x", f_half)) &&
                        d_tests_valued("sin(x)",
                                       m::value(d_tests_stored(
                                           d_math_sin(d_half))),
                                       m::at("x", d_half)) &&
                        d_tests_valued("atan2(x, y)",
                                       m::value(d_tests_stored(
                                           d_math_atan2(d_half, d_two))),
                                       m::at("x", f_half).at("y", 2.0)) &&
                        d_tests_valued("sqrt(x)",
                                       m::expr_complex(m::value(0.0),
                                                       m::value(1.0)),
                                       m::at("x", -1.0)) &&
                        d_tests_valued("fmod(x, 0)",
                                       m::value(d_math_fmod(1.5, 0.0)),
                                       m::at("x", 1.5)) &&
                        d_tests_valued("x mod 0",
                                       m::value(d_math_fmod(1.5, 0.0)),
                                       m::at("x", 1.5)),
                        "an inexact argument: its kind, the wider of two; a "
                        "NaN where the C core gives one, by 0 among them, "
                        "but where the value is complex alone (S272)");

        const m::value flagged = d_tests_value_of("9223372036854775807 + 1");

        D_TESTS_EXPECT( ((d_tests_value_of("sin(x)", m::at("x", flagged))
                              .flags() & D_MATH_VALUE_FLAG_INEXACT) != 0) &&
                        ((d_tests_value_of("atan2(1, x)",
                                           m::at("x", flagged)).flags() &
                          D_MATH_VALUE_FLAG_INEXACT) != 0),
                        "a function's result carries its arguments' flags");
        D_TESTS_EXPECT( d_tests_valued("sin(x)", pole, m::at("x", pole)) &&
                        d_tests_valued("atan2(x, y)", pole,
                                       m::at("x", pole).at("y", domain)) &&
                        d_tests_valued("abs(x)", pole, m::at("x", pole)) &&
                        d_tests_valued("floor(x)", domain,
                                       m::at("x", true)) &&
                        d_tests_valued("hypot(1, x)", domain,
                                       m::at("x", m::value())),
                        "an error passes, the first; a boolean or none, a "
                        "domain error");
        D_TESTS_EXPECT( d_tests_valued("pi",
                                       m::value(m::constants::of<long double>::
                                                    pi())) &&
                        d_tests_valued("e",
                                       m::value(m::constants::of<long double>::
                                                    e())) &&
                        d_tests_valued("2 * pi",
                                       m::value(2.0L *
                                                m::constants::of<long double>::
                                                    pi())),
                        "pi and e: the long double nearest each");

        // a boxed argument, a wide integer
        const m::value wide = d_tests_wide_of("-100000000000000000000");

        D_TESTS_EXPECT( (d_tests_value_of("abs(x)", m::at("x", wide)) ==
                             d_tests_wide_of("100000000000000000000")) &&
                        (d_tests_value_of("sqrt(-x)",
                                          m::at("x", wide)).kind() ==
                             D_MATH_VALUE_LONG_DOUBLE),
                        "a wide integer: abs exact, by its kind; sqrt in "
                        "long double");

        // a boxed argument not exact, of the test's own kind
        static const long double nine_quarters   = 2.25L;
        volatile long double     nine_quarters_l = nine_quarters;
        const m::value           real = d_tests_real_of(&nine_quarters);

        D_TESTS_EXPECT( d_tests_valued("sqrt(x)", m::value(1.5L),
                                       m::at("x", real)) &&
                        d_tests_valued("atan2(x, y)",
                                       m::value(d_math_atan2(nine_quarters_l,
                                                             half_l)),
                                       m::at("x", real).at("y", f_half)) &&
                        d_tests_typed<double>("x", 2.25, m::at("x", real)),
                        "a boxed kind not exact: in long double, beside a "
                        "float too; in a type, its nearest");

        return;
    }

    // d_tests_relations_in
    //   the relations in Type: C's comparisons, 1 where one holds and 0
    // where it does not, none holding of a NaN but !=.
    template<typename Type>
    bool
    d_tests_relations_in()
    {
        const m::expr_bindings unknown =
            m::at("u", std::numeric_limits<double>::quiet_NaN());
        const Type             one     = Type(1);
        const Type             zero    = Type(0);

        return ( (d_tests_typed<Type>("1 < 2", one))            &&
                 (d_tests_typed<Type>("2 < 1", zero))           &&
                 (d_tests_typed<Type>("1 < 1", zero))           &&
                 (d_tests_typed<Type>("1 <= 1", one))           &&
                 (d_tests_typed<Type>("2 <= 1", zero))          &&
                 (d_tests_typed<Type>("2 > 1", one))            &&
                 (d_tests_typed<Type>("1 > 1", zero))           &&
                 (d_tests_typed<Type>("1 >= 1", one))           &&
                 (d_tests_typed<Type>("1 >= 2", zero))          &&
                 (d_tests_typed<Type>("1 = 1", one))            &&
                 (d_tests_typed<Type>("1 = 2", zero))           &&
                 (d_tests_typed<Type>("1 != 2", one))           &&
                 (d_tests_typed<Type>("1 != 1", zero))          &&
                 (d_tests_typed<Type>("u < 1", zero, unknown))  &&
                 (d_tests_typed<Type>("u >= 1", zero, unknown)) &&
                 (d_tests_typed<Type>("u = u", zero, unknown))  &&
                 (d_tests_typed<Type>("u != u", one, unknown)) );
    }

    // d_tests_logic_in
    //   and, or, not, all_of and any_of in Type, Kleene's: 0 false, a NaN
    // unknown, any other number true; true and false 1 and 0.
    template<typename Type>
    bool
    d_tests_logic_in()
    {
        const m::expr_bindings unknown =
            m::at("u", std::numeric_limits<double>::quiet_NaN());
        const Type             nan     = std::numeric_limits<Type>::quiet_NaN();
        const Type             one     = Type(1);
        const Type             zero    = Type(0);

        return ( (d_tests_typed<Type>("1 and 0", zero))                     &&
                 (d_tests_typed<Type>("2 and 3", one))                      &&
                 (d_tests_typed<Type>("0 or 0", zero))                      &&
                 (d_tests_typed<Type>("0 or -2", one))                      &&
                 (d_tests_typed<Type>("not 0", one))                        &&
                 (d_tests_typed<Type>("not 4", zero))                       &&
                 (d_tests_typed<Type>("u and 0", zero, unknown))            &&
                 (d_tests_typed<Type>("0 and u", zero, unknown))            &&
                 (d_tests_typed<Type>("u and 1", nan, unknown))             &&
                 (d_tests_typed<Type>("u or 1", one, unknown))              &&
                 (d_tests_typed<Type>("1 or u", one, unknown))              &&
                 (d_tests_typed<Type>("u or 0", nan, unknown))              &&
                 (d_tests_typed<Type>("not u", nan, unknown))               &&
                 (d_tests_typed<Type>("all_of(1, 2, 3)", one))              &&
                 (d_tests_typed<Type>("all_of(1, u, 0)", zero, unknown))    &&
                 (d_tests_typed<Type>("all_of(1, u)", nan, unknown))        &&
                 (d_tests_typed<Type>("any_of(0, 0, 0)", zero))             &&
                 (d_tests_typed<Type>("any_of(0, u, 5)", one, unknown))     &&
                 (d_tests_typed<Type>("any_of(0, u)", nan, unknown))        &&
                 (d_tests_typed<Type>("true and not false", one))           &&
                 (d_tests_typed<Type>("true + true", Type(2))) );
    }

    // d_tests_piecewise_in
    //   piecewise in Type: the value of the first branch whose condition is
    // true, a nonzero number; a NaN where a NaN condition comes first, or
    // where no branch holds.
    template<typename Type>
    bool
    d_tests_piecewise_in()
    {
        const m::expr_bindings unknown =
            m::at("u", std::numeric_limits<double>::quiet_NaN());
        const Type             nan     = std::numeric_limits<Type>::quiet_NaN();
        const char* const      abs_x   =
            "piecewise(when(x < 0, -x), otherwise(x))";
        const char* const      sign_x  =
            "piecewise(when(x > 0, 1), when(x < 0, -1))";

        return ( (d_tests_typed<Type>(abs_x, Type(3), m::at("x", -3)))      &&
                 (d_tests_typed<Type>(abs_x, Type(2), m::at("x", 2)))       &&
                 (d_tests_typed<Type>(sign_x, Type(-1), m::at("x", -5)))    &&
                 (d_tests_typed<Type>(sign_x, nan, m::at("x", 0)))          &&
                 (d_tests_typed<Type>("piecewise(when(u, 1), otherwise(2))",
                                      nan,
                                      unknown))                             &&
                 (d_tests_typed<Type>(
                      "piecewise(when(0, 1), when(u, 2), otherwise(3))",
                      nan,
                      unknown))                                             &&
                 (d_tests_typed<Type>("piecewise(when(1, 5), when(u, 6))",
                                      Type(5),
                                      unknown))                             &&
                 (d_tests_typed<Type>("piecewise(when(2, 5))", Type(5)))    &&
                 (d_tests_typed<Type>("piecewise(otherwise(7))", Type(7))) );
    }

    // d_tests_fold_logic
    //   the relations, logic and piecewise in both folds (2d): in a type,
    // 1 and 0; as a value, booleans, by value.h's exact order and Kleene's
    // logic.
    void
    d_tests_fold_logic()
    {
        // in a type
        D_TESTS_EXPECT( (d_tests_relations_in<float>()) &&
                        (d_tests_relations_in<double>()) &&
                        (d_tests_relations_in<long double>()),
                        "the relations in each type: C's comparisons, 1 "
                        "and 0, none holding of a NaN but !=");
        D_TESTS_EXPECT( (d_tests_logic_in<float>()) &&
                        (d_tests_logic_in<double>()) &&
                        (d_tests_logic_in<long double>()),
                        "and, or, not, all_of and any_of in each type: "
                        "Kleene's, 0 false, a NaN unknown; true and false 1 "
                        "and 0");
        D_TESTS_EXPECT( (d_tests_piecewise_in<float>()) &&
                        (d_tests_piecewise_in<double>()) &&
                        (d_tests_piecewise_in<long double>()),
                        "piecewise in each type: the first branch that "
                        "holds; a NaN where a NaN condition comes first, or "
                        "none holds");
        D_TESTS_EXPECT( (d_tests_typed<double>("0.1 + 0.2 = 0.3", 0.0)) &&
                        (d_tests_typed<double>("0.1 + 0.2 != 0.3", 1.0)),
                        "0.1 + 0.2 = 0.3 in double: false, each literal "
                        "rounded");

        // as a value: relations
        const m::value         yes     = m::value(true);
        const m::value         no      = m::value(false);
        const m::value         domain  = m::value(D_MATH_VALUE_ERROR_DOMAIN);
        const m::value         pole    = m::value(D_MATH_VALUE_ERROR_POLE);
        const m::expr_bindings unknown =
            m::at("u", std::numeric_limits<double>::quiet_NaN());
        const m::expr_bindings doubles = m::at("x", 0.5).at("y", 0.1);

        D_TESTS_EXPECT( (d_tests_valued("0.1 + 0.2 = 0.3", yes)) &&
                        (d_tests_valued("1/3 < 0.34", yes)) &&
                        (d_tests_valued("2/4 = 1/2", yes)) &&
                        (d_tests_valued("1 != 1.0", no)) &&
                        (d_tests_valued("3 >= 3", yes)) &&
                        (d_tests_valued("3 > 3", no)) &&
                        (d_tests_valued("1/2 <= 0.5", yes)) &&
                        (d_tests_valued("-1 <= -2", no)),
                        "a relation of two exact numbers: their exact "
                        "order, so 0.1 + 0.2 = 0.3 holds");
        D_TESTS_EXPECT( (d_tests_valued("x = 1/2", yes, doubles)) &&
                        (d_tests_valued("y = 1/10", no, doubles)) &&
                        (d_tests_valued("y > 1/10", yes, doubles)),
                        "across kinds, exactly: the double 0.1 is more than "
                        "1/10");
        D_TESTS_EXPECT( (d_tests_valued("u = u", no, unknown)) &&
                        (d_tests_valued("u != u", yes, unknown)) &&
                        (d_tests_valued("u < 1", no, unknown)) &&
                        (d_tests_valued("u >= 1", no, unknown)),
                        "a NaN: unordered, none holding but !=");

        const m::expr_bindings wide =
            m::at("a", d_tests_wide_of("1267650600228229401496703205376"))
                .at("b", d_tests_wide_of("1267650600228229401496703205377"));

        D_TESTS_EXPECT( (d_tests_valued("a < b", yes, wide)) &&
                        (d_tests_valued("a = b - 1", yes, wide)) &&
                        (d_tests_valued("a = 2 ^ 100", yes, wide)) &&
                        (d_tests_valued("b > 1e30", yes, wide)),
                        "wide integers past 64 bits, a boxed kind: by "
                        "value.h's exact order");
        D_TESTS_EXPECT( (d_tests_valued("true = true", yes)) &&
                        (d_tests_valued("true = false", no)) &&
                        (d_tests_valued("false != true", yes)) &&
                        (d_tests_valued("(1 < 2) = (2 < 3)", yes)),
                        "two booleans: = and != by their truth");
        D_TESTS_EXPECT( (d_tests_valued("sqrt(-1) < 1", domain)) &&
                        (d_tests_valued("1 < 1 / 0", pole)) &&
                        (d_tests_valued("1 / 0 = sqrt(-1)", pole)) &&
                        (d_tests_valued("1 < true", domain)) &&
                        (d_tests_valued("true < false", domain)) &&
                        (d_tests_valued("x = 1", domain, m::at("x",
                                                               m::value()))),
                        "an error passes, the first; a boolean ordered, or "
                        "a boolean or none against a number, a domain "
                        "error");

        // as a value: logic
        D_TESTS_EXPECT( (d_tests_valued("true and false", no)) &&
                        (d_tests_valued("true or false", yes)) &&
                        (d_tests_valued("not false", yes)) &&
                        (d_tests_valued("not not true", yes)) &&
                        (d_tests_valued("true and true", yes)) &&
                        (d_tests_valued("false or false", no)),
                        "and, or and not of booleans");
        D_TESTS_EXPECT( (d_tests_valued("false and sqrt(-1) > 0", no)) &&
                        (d_tests_valued("sqrt(-1) > 0 and false", no)) &&
                        (d_tests_valued("true or 1 / 0 > 0", yes)) &&
                        (d_tests_valued("false and 1", no)),
                        "Kleene's: a false operand makes and false, a true "
                        "one or true, whatever the other is");
        D_TESTS_EXPECT( (d_tests_valued("true and 1 / 0 > 0", pole)) &&
                        (d_tests_valued("1 / 0 > 0 or sqrt(-1) > 0", pole)) &&
                        (d_tests_valued("1 and true", domain)) &&
                        (d_tests_valued("not 1", domain)) &&
                        (d_tests_valued("not 1 / 0 > 0", pole)),
                        "otherwise an error passes, the first; a number in "
                        "logic, a domain error");
        D_TESTS_EXPECT( (d_tests_valued("all_of(true, 1 < 2, 2 < 3)", yes)) &&
                        (d_tests_valued("all_of(true, sqrt(-1) > 0, false)",
                                        no)) &&
                        (d_tests_valued("all_of(true, sqrt(-1) > 0)",
                                        domain)) &&
                        (d_tests_valued("any_of(false, false)", no)) &&
                        (d_tests_valued("any_of(1 / 0 > 0, true)", yes)) &&
                        (d_tests_valued("any_of(false, 2)", domain)),
                        "all_of and any_of: and and or of any number of "
                        "operands");

        // as a value: piecewise, the constants
        const char* const abs_x = "piecewise(when(x < 0, -x), otherwise(x))";

        D_TESTS_EXPECT( (d_tests_valued(abs_x,
                                        m::value::ratio(1, 3),
                                        m::at("x", m::value::ratio(-1, 3)))) &&
                        (d_tests_valued(abs_x,
                                        m::value::ratio(2, 3),
                                        m::at("x", m::value::ratio(2, 3)))),
                        "piecewise as a value: the first branch that holds, "
                        "exactly");
        D_TESTS_EXPECT( (d_tests_valued("piecewise(when(x > 0, 1), "
                                        "when(x < 0, -1))",
                                        domain,
                                        m::at("x", 0))) &&
                        (d_tests_valued("piecewise(when(1 / 0 > 0, 1), "
                                        "otherwise(2))",
                                        pole)) &&
                        (d_tests_valued("piecewise(when(1, 5), "
                                        "otherwise(3))",
                                        domain)) &&
                        (d_tests_valued("piecewise(when(true, fmod(1, 0)))",
                                        domain)),
                        "no branch holding, a domain error; a condition's "
                        "error passes, a number for one a domain error; the "
                        "branch's value as it is");
        D_TESTS_EXPECT( (d_tests_valued("piecewise(when(false, 1 / 0), "
                                        "otherwise(3))",
                                        m::value(3))) &&
                        (d_tests_valued("piecewise(when(true, 3), "
                                        "when(1 / 0 > 0, 4))",
                                        m::value(3))),
                        "what no branch reaches before the one that holds "
                        "matters not");
        D_TESTS_EXPECT( (d_tests_valued("true", yes)) &&
                        (d_tests_valued("false", no)) &&
                        (d_tests_valued("x", yes, m::at("x", true))),
                        "true and false, and a bound boolean, as values");

        return;
    }

    // d_tests_fold_errors
    //   what neither fold computes: an unbound variable, a constant or a
    // function the tables have not, an operator or a call on the wrong
    // number of operands, indexing by none or by three among them, an
    // operator of no kind, and a term made by default; and in a type, what
    // a literal, indexing and a number's transpose give.
    void
    d_tests_fold_errors()
    {
        const m::expr_term sum = d_tests_x_plus_1();
        const m::expr_term one = m::expr_number(m::value(1), "1");
        const m::expr_term two = m::expr_number(m::value(2), "2");

        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<double>(sum),
                                         m::expr_status::unbound_variable,
                                         m::expr_no_offset,
                                         "the variable x is bound to no "
                                         "value") &&
                        d_tests_is_error(m::expr_evaluate(sum,
                                                          m::expr_bindings(),
                                                          m::expr_context()),
                                         m::expr_status::unbound_variable,
                                         m::expr_no_offset,
                                         "the variable x is bound to no "
                                         "value"),
                        "an unbound variable, in either fold");

        const m::expr_result<m::expr_term> both = m::expr_parse("q + r");

        D_TESTS_EXPECT( (both.is_ok()) &&
                        (d_tests_is_error(m::expr_evaluate<float>(
                                              both.value()),
                                          m::expr_status::unbound_variable,
                                          m::expr_no_offset,
                                          "the variable q is bound to no "
                                          "value")),
                        "the first error, left to right");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<double>(
                                             m::expr_constant("tau")),
                                         m::expr_status::unknown_name,
                                         m::expr_no_offset,
                                         "no constant of the language is "
                                         "named tau") &&
                        d_tests_is_error(m::expr_evaluate(
                                             m::expr_constant("tau"),
                                             m::expr_bindings(),
                                             m::expr_context()),
                                         m::expr_status::unknown_name,
                                         m::expr_no_offset,
                                         "no constant of the language is "
                                         "named tau"),
                        "a constant the table has not: unknown_name");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<double>(
                                             m::expr_call("foo", { one })),
                                         m::expr_status::unknown_name,
                                         m::expr_no_offset,
                                         "no function of the language is "
                                         "named foo") &&
                        d_tests_is_error(m::expr_evaluate(
                                             m::expr_call("foo", { one }),
                                             m::expr_bindings(),
                                             m::expr_context()),
                                         m::expr_status::unknown_name,
                                         m::expr_no_offset,
                                         "no function of the language is "
                                         "named foo"),
                        "a call of a function the table has not: "
                        "unknown_name");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<float>(
                                             m::expr_call("atan2", { one })),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "atan2 takes 2 arguments, not 1") &&
                        d_tests_is_error(m::expr_evaluate(
                                             m::expr_call("sin", {}),
                                             m::expr_bindings(),
                                             m::expr_context()),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "sin takes 1 argument, not 0") &&
                        d_tests_is_error(m::expr_evaluate<double>(
                                             m::expr_call("sin",
                                                          { one, two })),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "sin takes 1 argument, not 2") &&
                        d_tests_is_error(m::expr_evaluate(
                                             m::expr_call("atan2",
                                                          { one, two, one }),
                                             m::expr_bindings(),
                                             m::expr_context()),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "atan2 takes 2 arguments, not 3") &&
                        d_tests_is_error(m::expr_evaluate<long double>(
                                             d_tests_apply(
                                                 m::expr_op_kind::factorial,
                                                 one,
                                                 two)),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "'!' takes 1 operand, not 2"),
                        "a call, or the factorial, of a number of operands "
                        "it does not take, fewer or more: wrong_arity");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<double>(
                                             m::expr_call(
                                                 "foo",
                                                 { m::expr_variable("y") })),
                                         m::expr_status::unbound_variable,
                                         m::expr_no_offset,
                                         "the variable y is bound to no "
                                         "value"),
                        "an argument's error before the call's own");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<float>(
                                             d_tests_apply(
                                                 m::expr_op_kind::vector,
                                                 one,
                                                 two)),
                                         m::expr_status::bad_shape,
                                         m::expr_no_offset,
                                         "expr_evaluate<Type>: a vector or a "
                                         "matrix has no value in a type"),
                        "a literal: no value in a type");

        const m::expr_result<double> indexed    =
            m::expr_evaluate<double>(d_tests_apply(m::expr_op_kind::index,
                                                   one,
                                                   two));
        const m::expr_result<double> transposed =
            m::expr_evaluate<double>(d_tests_apply(
                                         m::expr_op_kind::transpose,
                                         two));

        D_TESTS_EXPECT( (indexed.is_ok()) &&
                        (indexed.value() != indexed.value()) &&
                        (transposed.is_ok()) &&
                        (transposed.value() == 2.0),
                        "in a type, indexing a NaN, as no number takes "
                        "it, and a number's transpose the number");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<double>(
                                             m::expr_application(
                                                 m::expr_op_kind::index,
                                                 { one })),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "indexing takes 1 or 2 indices, "
                                         "not 0") &&
                        d_tests_is_error(m::expr_evaluate(
                                             m::expr_application(
                                                 m::expr_op_kind::index,
                                                 { one, two, two, two }),
                                             m::expr_bindings(),
                                             m::expr_context()),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "indexing takes 1 or 2 indices, "
                                         "not 3") &&
                        d_tests_is_error(m::expr_evaluate(
                                             m::expr_application(
                                                 m::expr_op_kind::index,
                                                 std::vector<m::expr_term>()),
                                             m::expr_bindings(),
                                             m::expr_context()),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "indexing takes 1 or 2 indices, "
                                         "not 0"),
                        "indexing by none, or by three, made by hand: "
                        "wrong_arity, in both folds");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<double>(
                                             d_tests_apply(
                                                 m::expr_op_kind::none,
                                                 one,
                                                 two)),
                                         m::expr_status::unknown_name,
                                         m::expr_no_offset,
                                         "no operator of the language is of "
                                         "kind 0") &&
                        d_tests_is_error(m::expr_evaluate(
                                             d_tests_apply(
                                                 m::expr_op_kind::none,
                                                 one),
                                             m::expr_bindings(),
                                             m::expr_context()),
                                         m::expr_status::unknown_name,
                                         m::expr_no_offset,
                                         "no operator of the language is of "
                                         "kind 0"),
                        "an operator of no kind the language has: "
                        "unknown_name, in both folds");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<double>(
                                             d_tests_apply(m::expr_op_kind::add,
                                                           one)),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "'+' takes 2 operands, not 1") &&
                        d_tests_is_error(m::expr_evaluate(
                                             d_tests_apply(m::expr_op_kind::neg,
                                                           one,
                                                           two),
                                             m::expr_bindings(),
                                             m::expr_context()),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "'-' takes 1 operand, not 2"),
                        "an operator on a number of operands it does not "
                        "take: wrong_arity");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<double>(
                                             d_tests_apply(m::expr_op_kind::lt,
                                                           one)),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "'<' takes 2 operands, not 1") &&
                        d_tests_is_error(m::expr_evaluate(
                                             d_tests_apply(
                                                 m::expr_op_kind::logical_not,
                                                 one,
                                                 two),
                                             m::expr_bindings(),
                                             m::expr_context()),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "'not' takes 1 operand, not 2"),
                        "a relation, or logic, on a number of operands it "
                        "does not take: wrong_arity");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<double>(
                                             m::expr_call("all_of", {})),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "all_of takes at least 1 argument, "
                                         "not 0") &&
                        d_tests_is_error(m::expr_evaluate(
                                             m::expr_call("piecewise", {}),
                                             m::expr_bindings(),
                                             m::expr_context()),
                                         m::expr_status::wrong_arity,
                                         m::expr_no_offset,
                                         "piecewise takes at least 1 "
                                         "argument, not 0"),
                        "a function of any number of arguments from 1, of "
                        "none: wrong_arity");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<double>(
                                             m::expr_application(m::expr_op(),
                                                                 one)),
                                         m::expr_status::unknown_name,
                                         m::expr_no_offset,
                                         "no operator of the language is of "
                                         "kind 0"),
                        "an operator of no kind: unknown_name");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_evaluate<double>(
                                             m::expr_term()),
                                         m::expr_status::missing_operand,
                                         m::expr_no_offset,
                                         "a term made by default has no "
                                         "value") &&
                        d_tests_is_error(m::expr_evaluate(m::expr_term(),
                                                          m::expr_bindings(),
                                                          m::expr_context()),
                                         m::expr_status::missing_operand,
                                         m::expr_no_offset,
                                         "a term made by default has no "
                                         "value"),
                        "a term made by default: missing_operand");

        return;
    }


    //==========================================================================
    // 9.  PARSING AND RENDERING
    //==========================================================================

    // d_tests_parse_numbers
    //   a number read exactly, its text its literal; a name a variable, or
    // a constant the standard table has.
    void
    d_tests_parse_numbers()
    {
        const m::expr_result<m::expr_term> tenth = m::expr_parse("0.1");
        const m::expr_result<m::expr_term> far   = m::expr_parse("1e30");
        const m::expr_result<m::expr_term> past  = m::expr_parse("1e5000");

        D_TESTS_EXPECT( d_tests_parses("42",
                                       m::expr_number(m::value(42), "42")),
                        "42: the integer 42, written 42");
        D_TESTS_EXPECT( (tenth.is_ok()) &&
                        (m::expr_is_number(tenth.value())) &&
                        (m::expr_atom_of(tenth.value()).text == "0.1") &&
                        (m::internal::expr_same_number(
                             m::expr_atom_of(tenth.value()).number,
                             m::value::ratio(1, 10))),
                        "0.1: exactly 1/10, written 0.1 (G3)");
        D_TESTS_EXPECT( (far.is_ok()) &&
                        (m::expr_atom_of(far.value()).number.kind() ==
                             D_MATH_VALUE_LONG_DOUBLE) &&
                        ( (m::expr_atom_of(far.value()).number.flags() &
                           D_MATH_VALUE_FLAG_INEXACT) != 0 ) &&
                        (past.is_ok()) &&
                        (m::internal::expr_same_number(
                             m::expr_atom_of(past.value()).number,
                             m::value(D_MATH_VALUE_ERROR_RANGE))),
                        "a literal past the family: G2's long double; past "
                        "every long double, a range error");
        D_TESTS_EXPECT( d_tests_parses("x", m::expr_variable("x")) &&
                        d_tests_parses("_a1", m::expr_variable("_a1")) &&
                        d_tests_parses("pie", m::expr_variable("pie")) &&
                        d_tests_parses("E", m::expr_variable("E")),
                        "a name: a variable");
        D_TESTS_EXPECT( d_tests_parses("pi", m::expr_constant("pi")) &&
                        d_tests_parses("e", m::expr_constant("e")),
                        "pi and e: the standard table's constants");

        return;
    }

    // d_tests_parse_grammar
    //   G5's precedence and associativity; the unary - and + by place; the
    // brackets.
    void
    d_tests_parse_grammar()
    {
        const m::expr_term a = m::expr_variable("a");
        const m::expr_term b = m::expr_variable("b");
        const m::expr_term c = m::expr_variable("c");
        const m::expr_term x = m::expr_variable("x");
        const m::expr_term n1 = d_tests_literal("1");
        const m::expr_term n2 = d_tests_literal("2");
        const m::expr_term n3 = d_tests_literal("3");

        D_TESTS_EXPECT( d_tests_parses("1 + 2 * 3",
                                       d_tests_apply(m::expr_op_kind::add, n1,
                                           d_tests_apply(m::expr_op_kind::mul,
                                                         n2, n3))) &&
                        d_tests_parses("1 - 2 - 3",
                                       d_tests_apply(m::expr_op_kind::sub,
                                           d_tests_apply(m::expr_op_kind::sub,
                                                         n1, n2),
                                           n3)) &&
                        d_tests_parses("a / b / c",
                                       d_tests_apply(m::expr_op_kind::div,
                                           d_tests_apply(m::expr_op_kind::div,
                                                         a, b),
                                           c)),
                        "* over +; - and / to the left");
        D_TESTS_EXPECT( d_tests_parses("2 ^ 3 ^ 2",
                                       d_tests_apply(m::expr_op_kind::pow, n2,
                                           d_tests_apply(m::expr_op_kind::pow,
                                                         n3, n2))),
                        "^ to the right: 2 ^ 3 ^ 2 is 2 ^ 9");
        D_TESTS_EXPECT( d_tests_parses("a mod b * c",
                                       d_tests_apply(m::expr_op_kind::mul,
                                           d_tests_apply(m::expr_op_kind::mod,
                                                         a, b),
                                           c)) &&
                        d_tests_parses("a + b mod c",
                                       d_tests_apply(m::expr_op_kind::add, a,
                                           d_tests_apply(m::expr_op_kind::mod,
                                                         b, c))),
                        "mod as tight as *, to the left");
        D_TESTS_EXPECT( d_tests_parses("-x ^ 2",
                                       d_tests_apply(m::expr_op_kind::neg,
                                           d_tests_apply(m::expr_op_kind::pow,
                                                         x, n2))) &&
                        d_tests_parses("-a * b",
                                       d_tests_apply(m::expr_op_kind::mul,
                                           d_tests_apply(m::expr_op_kind::neg,
                                                         a),
                                           b)) &&
                        d_tests_parses("2 ^ -3",
                                       d_tests_apply(m::expr_op_kind::pow, n2,
                                           d_tests_apply(m::expr_op_kind::neg,
                                                         n3))),
                        "the unary - below ^ and above *: -x ^ 2 is "
                        "-(x ^ 2)");
        D_TESTS_EXPECT( d_tests_parses("a - -b",
                                       d_tests_apply(m::expr_op_kind::sub, a,
                                           d_tests_apply(m::expr_op_kind::neg,
                                                         b))) &&
                        d_tests_parses("a * -b",
                                       d_tests_apply(m::expr_op_kind::mul, a,
                                           d_tests_apply(m::expr_op_kind::neg,
                                                         b))) &&
                        d_tests_parses("--a",
                                       d_tests_apply(m::expr_op_kind::neg,
                                           d_tests_apply(m::expr_op_kind::neg,
                                                         a))) &&
                        d_tests_parses("(-a)",
                                       d_tests_apply(m::expr_op_kind::neg,
                                                     a)),
                        "- unary at the start, after an operator and after "
                        "an open bracket; binary after an operand");
        D_TESTS_EXPECT( d_tests_parses("+a",
                                       d_tests_apply(m::expr_op_kind::pos,
                                                     a)) &&
                        d_tests_parses("a + +b",
                                       d_tests_apply(m::expr_op_kind::add, a,
                                           d_tests_apply(m::expr_op_kind::pos,
                                                         b))),
                        "+ unary and binary, as -");
        D_TESTS_EXPECT( d_tests_parses("((x))", x) &&
                        d_tests_parses("(1 + 2) * 3",
                                       d_tests_apply(m::expr_op_kind::mul,
                                           d_tests_apply(m::expr_op_kind::add,
                                                         n1, n2),
                                           n3)),
                        "brackets group, and leave no node");

        return;
    }

    // d_tests_parse_implied
    //   G5's implied multiplication: on, a * between an operand's end and
    // the next one's start, as tight as *; never between two numbers, nor
    // before a call's bracket; off, none.
    void
    d_tests_parse_implied()
    {
        m::expr_syntax     implied;
        const m::expr_term a  = m::expr_variable("a");
        const m::expr_term b  = m::expr_variable("b");
        const m::expr_term x  = m::expr_variable("x");
        const m::expr_term y  = m::expr_variable("y");
        const m::expr_term n1 = d_tests_literal("1");
        const m::expr_term n2 = d_tests_literal("2");

        implied.implied_multiplication = true;

        D_TESTS_EXPECT( d_tests_parses("2x",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     n2, x),
                                       implied) &&
                        d_tests_parses("2(x + 1)",
                                       d_tests_apply(m::expr_op_kind::mul, n2,
                                           d_tests_apply(m::expr_op_kind::add,
                                                         x, n1)),
                                       implied) &&
                        d_tests_parses("(a)(b)",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     a, b),
                                       implied) &&
                        d_tests_parses("x y",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     x, y),
                                       implied) &&
                        d_tests_parses("(a)2",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     a, n2),
                                       implied),
                        "2x, 2(x + 1), (a)(b), x y, (a)2: products");
        D_TESTS_EXPECT( d_tests_parses("a mod b",
                                       d_tests_apply(m::expr_op_kind::mod,
                                                     a, b),
                                       implied),
                        "mod an operator still: no product before it");
        D_TESTS_EXPECT( d_tests_parses("2x^2",
                                       d_tests_apply(m::expr_op_kind::mul, n2,
                                           d_tests_apply(m::expr_op_kind::pow,
                                                         x, n2)),
                                       implied) &&
                        d_tests_parses("-2x",
                                       d_tests_apply(m::expr_op_kind::mul,
                                           d_tests_apply(m::expr_op_kind::neg,
                                                         n2),
                                           x),
                                       implied) &&
                        d_tests_parses("1/2x",
                                       d_tests_apply(m::expr_op_kind::mul,
                                           d_tests_apply(m::expr_op_kind::div,
                                                         n1, n2),
                                           x),
                                       implied),
                        "as tight as *: 2x^2 is 2 * x ^ 2, 1/2x is (1/2) "
                        "* x");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("2 3", implied),
                                         m::expr_status::unexpected_token,
                                         2,
                                         "unexpected '3'") &&
                        d_tests_parses("x(y)",
                                       m::expr_call("x", { y }),
                                       implied),
                        "never between two numbers, nor before a call's "
                        "bracket: x(y) is a call");
        D_TESTS_EXPECT( d_tests_parses("2pi",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     n2,
                                                     m::expr_constant("pi")),
                                       implied) &&
                        d_tests_parses("2sin(x)",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     n2,
                                                     m::expr_call("sin",
                                                                  { x })),
                                       implied) &&
                        d_tests_parses("sin(x)cos(x)",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     m::expr_call("sin",
                                                                  { x }),
                                                     m::expr_call("cos",
                                                                  { x })),
                                       implied) &&
                        d_tests_parses("3!x",
                                       d_tests_apply(m::expr_op_kind::mul,
                                           d_tests_apply(
                                               m::expr_op_kind::factorial,
                                               m::expr_number(m::value(3),
                                                              "3")),
                                           x),
                                       implied),
                        "2pi, 2sin(x), sin(x)cos(x), 3!x: products before "
                        "a constant and a call, after a call and the "
                        "factorial");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("2x"),
                                         m::expr_status::unexpected_token,
                                         1,
                                         "unexpected 'x'") &&
                        d_tests_is_error(m::expr_parse("(a)(b)"),
                                         m::expr_status::unexpected_token,
                                         3,
                                         "unexpected '('"),
                        "off: no product implied");

        return;
    }

    // d_tests_parse_calls
    //   calls, their arguments and their place in the grammar; the
    // factorial's place; the constants.
    void
    d_tests_parse_calls()
    {
        const m::expr_term x  = m::expr_variable("x");
        const m::expr_term y  = m::expr_variable("y");
        const m::expr_term n1 = d_tests_literal("1");
        const m::expr_term n2 = d_tests_literal("2");
        const m::expr_term n3 = d_tests_literal("3");
        const m::expr_term sx = m::expr_call("sin", { x });

        D_TESTS_EXPECT( d_tests_parses("sin(x)", sx) &&
                        d_tests_parses("atan2(y, x)",
                                       m::expr_call("atan2", { y, x })) &&
                        d_tests_parses("f()", m::expr_call("f", {})) &&
                        d_tests_parses("g(1, 2, 3)",
                                       m::expr_call("g", { n1, n2, n3 })),
                        "a call: its function, and its arguments, none to "
                        "three");
        D_TESTS_EXPECT( d_tests_parses("f(sin(x), g(y, (1)))",
                                       m::expr_call("f",
                                           { sx,
                                             m::expr_call("g",
                                                          { y, n1 }) })) &&
                        d_tests_parses("f(-1, -x)",
                                       m::expr_call("f",
                                           { d_tests_apply(
                                                 m::expr_op_kind::neg, n1),
                                             d_tests_apply(
                                                 m::expr_op_kind::neg,
                                                 x) })),
                        "calls inside calls and brackets; a minus after a "
                        "comma unary");
        D_TESTS_EXPECT( d_tests_parses("sin(x) ^ 2",
                                       d_tests_apply(m::expr_op_kind::pow,
                                                     sx, n2)) &&
                        d_tests_parses("-sin(x)",
                                       d_tests_apply(m::expr_op_kind::neg,
                                                     sx)) &&
                        d_tests_parses("2 * sin(x + 1)",
                                       d_tests_apply(m::expr_op_kind::mul, n2,
                                           m::expr_call("sin",
                                               { d_tests_apply(
                                                     m::expr_op_kind::add,
                                                     x, n1) }))),
                        "a call a primary: sin(x) ^ 2 is (sin(x)) ^ 2");

        const m::expr_term three_fact = d_tests_apply(
                                            m::expr_op_kind::factorial, n3);

        D_TESTS_EXPECT( d_tests_parses("2 ^ 3!",
                                       d_tests_apply(m::expr_op_kind::pow,
                                                     n2, three_fact)) &&
                        d_tests_parses("-3!",
                                       d_tests_apply(m::expr_op_kind::neg,
                                                     three_fact)) &&
                        d_tests_parses("3! ^ 2",
                                       d_tests_apply(m::expr_op_kind::pow,
                                                     three_fact, n2)) &&
                        d_tests_parses("3!!",
                                       d_tests_apply(
                                           m::expr_op_kind::factorial,
                                           three_fact)) &&
                        d_tests_parses("sin(x)!",
                                       d_tests_apply(
                                           m::expr_op_kind::factorial,
                                           sx)) &&
                        d_tests_parses("3! - 1",
                                       d_tests_apply(m::expr_op_kind::sub,
                                                     three_fact, n1)),
                        "! postfix, tighter than ^ and the unary minus; n!! "
                        "is (n!)!; a minus after it binary");
        D_TESTS_EXPECT( d_tests_parses("pi(2)",
                                       m::expr_call("pi", { n2 })) &&
                        d_tests_parses("e ^ x",
                                       d_tests_apply(m::expr_op_kind::pow,
                                                     m::expr_constant("e"),
                                                     x)) &&
                        d_tests_parses("sin",
                                       m::expr_variable("sin")),
                        "a name before a bracket a call, a constant's too; "
                        "a function's name alone a variable");
        D_TESTS_EXPECT( d_tests_parses("f[1]",
                                       m::expr_application(
                                           m::expr_op_kind::index,
                                           m::expr_variable("f"),
                                           n1)) &&
                        d_tests_is_error(m::expr_parse_tokens(
                                             { m::expr_token(
                                                   m::expr_token_kind::name,
                                                   "f",
                                                   0),
                                               m::expr_token(
                                                   m::expr_token_kind::open,
                                                   "[",
                                                   1) }),
                                         m::expr_status::unbalanced,
                                         1,
                                         "'[' is never closed"),
                        "a name before a square bracket no call: a "
                        "variable indexed");

        return;
    }

    // d_tests_parse_bars
    //   G5's bars: |x| the call abs(x), each bar opening or closing by where
    // it stands.
    void
    d_tests_parse_bars()
    {
        const m::expr_term x  = m::expr_variable("x");
        const m::expr_term y  = m::expr_variable("y");
        const m::expr_term n1 = d_tests_literal("1");
        const m::expr_term n2 = d_tests_literal("2");
        const m::expr_term ax = m::expr_call("abs", { x });
        m::expr_syntax     bars;
        m::expr_syntax     both;

        bars.absolute_bars          = true;
        both.absolute_bars          = true;
        both.implied_multiplication = true;

        D_TESTS_EXPECT( d_tests_parses("|x - 1|",
                                       m::expr_call("abs",
                                           { d_tests_apply(
                                                 m::expr_op_kind::sub,
                                                 x, n1) }),
                                       bars) &&
                        d_tests_parses("||x| - 1|",
                                       m::expr_call("abs",
                                           { d_tests_apply(
                                                 m::expr_op_kind::sub,
                                                 ax, n1) }),
                                       bars) &&
                        d_tests_parses("|-x|",
                                       m::expr_call("abs",
                                           { d_tests_apply(
                                                 m::expr_op_kind::neg,
                                                 x) }),
                                       bars),
                        "|x - 1| is abs(x - 1); a bar opens after a bar or "
                        "an operator, and closes after an operand");
        D_TESTS_EXPECT( d_tests_parses("-|x| ^ 2",
                                       d_tests_apply(m::expr_op_kind::neg,
                                           d_tests_apply(
                                               m::expr_op_kind::pow,
                                               ax, n2)),
                                       bars) &&
                        d_tests_parses("|x|!",
                                       d_tests_apply(
                                           m::expr_op_kind::factorial, ax),
                                       bars) &&
                        d_tests_parses("f(|x|, 2)",
                                       m::expr_call("f", { ax, n2 }),
                                       bars) &&
                        d_tests_parses("|(x)|", ax, bars),
                        "bars a primary, in a call, around brackets");
        D_TESTS_EXPECT( d_tests_parses("|x| |y|",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     ax,
                                                     m::expr_call("abs",
                                                                  { y })),
                                       both) &&
                        d_tests_parses("2|x|",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     n2, ax),
                                       both) &&
                        d_tests_parses("(2|x|)",
                                       d_tests_apply(m::expr_op_kind::mul,
                                                     n2, ax),
                                       both),
                        "with implied multiplication: |x| |y| and 2|x| "
                        "products, in brackets too");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("|x, y|", bars),
                                         m::expr_status::unexpected_token,
                                         2,
                                         "unexpected ','") &&
                        d_tests_is_error(m::expr_parse("||", bars),
                                         m::expr_status::unbalanced,
                                         1,
                                         "'|' is never closed") &&
                        d_tests_is_error(m::expr_parse("|1 + |", bars),
                                         m::expr_status::unbalanced,
                                         5,
                                         "'|' is never closed"),
                        "no comma between bars; a bar after a bar or an "
                        "operator opens, and an open never closed is "
                        "unbalanced");

        return;
    }

    // d_tests_parse_logic
    //   2d's relations and logic in the parser layer: logic below the
    // relations (S213); a chain the conjunction of its links (S214); a
    // bracketed relation one operand (S215); the words operators wherever
    // they stand; true and false constants.
    void
    d_tests_parse_logic()
    {
        const m::expr_term a  = m::expr_variable("a");
        const m::expr_term b  = m::expr_variable("b");
        const m::expr_term c  = m::expr_variable("c");
        const m::expr_term x  = m::expr_variable("x");
        const m::expr_term y  = m::expr_variable("y");
        const m::expr_term n0 = d_tests_literal("0");
        const m::expr_term n1 = d_tests_literal("1");
        const m::expr_term n2 = d_tests_literal("2");
        const m::expr_term n3 = d_tests_literal("3");
        const m::expr_term n6 = d_tests_literal("6");
        m::expr_syntax     implied;

        implied.implied_multiplication = true;

        D_TESTS_EXPECT( d_tests_parses("x > 0 and x < 1",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_and,
                                           d_tests_apply(m::expr_op_kind::gt,
                                                         x, n0),
                                           d_tests_apply(m::expr_op_kind::lt,
                                                         x, n1))) &&
                        d_tests_parses("a + 1 < b * 2",
                                       d_tests_apply(m::expr_op_kind::lt,
                                           d_tests_apply(m::expr_op_kind::add,
                                                         a, n1),
                                           d_tests_apply(m::expr_op_kind::mul,
                                                         b, n2))) &&
                        d_tests_parses("-a <= b",
                                       d_tests_apply(m::expr_op_kind::le,
                                           d_tests_apply(m::expr_op_kind::neg,
                                                         a),
                                           b)),
                        "the relations below the arithmetic, and logic "
                        "below the relations: x > 0 and x < 1");
        D_TESTS_EXPECT( d_tests_parses("a or b and c",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_or, a,
                                           d_tests_apply(
                                               m::expr_op_kind::logical_and,
                                               b, c))) &&
                        d_tests_parses("a and b and c",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_and,
                                           d_tests_apply(
                                               m::expr_op_kind::logical_and,
                                               a, b),
                                           c)) &&
                        d_tests_parses("a or b or c",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_or,
                                           d_tests_apply(
                                               m::expr_op_kind::logical_or,
                                               a, b),
                                           c)),
                        "and over or, each to the left");
        D_TESTS_EXPECT( d_tests_parses("not a = b",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_not,
                                           d_tests_apply(m::expr_op_kind::eq,
                                                         a, b))) &&
                        d_tests_parses("not a and b",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_and,
                                           d_tests_apply(
                                               m::expr_op_kind::logical_not,
                                               a),
                                           b)) &&
                        d_tests_parses("not not a",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_not,
                                           d_tests_apply(
                                               m::expr_op_kind::logical_not,
                                               a))) &&
                        d_tests_parses("a = not b",
                                       d_tests_apply(m::expr_op_kind::eq, a,
                                           d_tests_apply(
                                               m::expr_op_kind::logical_not,
                                               b))),
                        "not between and and the relations: not a = b is "
                        "not (a = b); after an operator, at its own level");
        D_TESTS_EXPECT( d_tests_parses("a != b",
                                       d_tests_apply(m::expr_op_kind::ne,
                                                     a, b)) &&
                        d_tests_parses("a>=b",
                                       d_tests_apply(m::expr_op_kind::ge,
                                                     a, b)) &&
                        d_tests_parses("3!=6",
                                       d_tests_apply(m::expr_op_kind::ne,
                                                     n3, n6)) &&
                        d_tests_parses("3! = 6",
                                       d_tests_apply(m::expr_op_kind::eq,
                                           d_tests_apply(
                                               m::expr_op_kind::factorial,
                                               n3),
                                           n6)),
                        "the longer symbol first: 3!=6 is 3 != 6");
        D_TESTS_EXPECT( d_tests_parses("not(a)",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_not,
                                           a)) &&
                        d_tests_parses("a and(b)",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_and,
                                           a, b)) &&
                        d_tests_parses("a mod(b)",
                                       d_tests_apply(m::expr_op_kind::mod,
                                                     a, b)),
                        "a word before a bracket still its operator, never "
                        "a call");
        D_TESTS_EXPECT( d_tests_parses("a < b <= c",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_and,
                                           d_tests_apply(m::expr_op_kind::lt,
                                                         a, b),
                                           d_tests_apply(m::expr_op_kind::le,
                                                         b, c))) &&
                        d_tests_parses("a = b = c",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_and,
                                           d_tests_apply(m::expr_op_kind::eq,
                                                         a, b),
                                           d_tests_apply(m::expr_op_kind::eq,
                                                         b, c))) &&
                        d_tests_parses("a < b > c",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_and,
                                           d_tests_apply(m::expr_op_kind::lt,
                                                         a, b),
                                           d_tests_apply(m::expr_op_kind::gt,
                                                         b, c))) &&
                        d_tests_parses("a >= b != c",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_and,
                                           d_tests_apply(m::expr_op_kind::ge,
                                                         a, b),
                                           d_tests_apply(m::expr_op_kind::ne,
                                                         b, c))),
                        "a chain, the conjunction of its links: a = b = c "
                        "is a = b and b = c");
        D_TESTS_EXPECT( d_tests_parses("0 < 1 < 2 < 3",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_and,
                                           d_tests_apply(
                                               m::expr_op_kind::logical_and,
                                               d_tests_apply(
                                                   m::expr_op_kind::lt,
                                                   n0, n1),
                                               d_tests_apply(
                                                   m::expr_op_kind::lt,
                                                   n1, n2)),
                                           d_tests_apply(m::expr_op_kind::lt,
                                                         n2, n3))) &&
                        d_tests_parses("0 <= x + 1 < 2 * y",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_and,
                                           d_tests_apply(m::expr_op_kind::le,
                                               n0,
                                               d_tests_apply(
                                                   m::expr_op_kind::add,
                                                   x, n1)),
                                           d_tests_apply(m::expr_op_kind::lt,
                                               d_tests_apply(
                                                   m::expr_op_kind::add,
                                                   x, n1),
                                               d_tests_apply(
                                                   m::expr_op_kind::mul,
                                                   n2, y)))),
                        "the links joined to the left, the operand between "
                        "two in both");
        D_TESTS_EXPECT( d_tests_parses("(a < b) < c",
                                       d_tests_apply(m::expr_op_kind::lt,
                                           d_tests_apply(m::expr_op_kind::lt,
                                                         a, b),
                                           c)) &&
                        d_tests_parses("a < (b < c)",
                                       d_tests_apply(m::expr_op_kind::lt, a,
                                           d_tests_apply(m::expr_op_kind::lt,
                                                         b, c))) &&
                        d_tests_parses("(a < b) = (b < c)",
                                       d_tests_apply(m::expr_op_kind::eq,
                                           d_tests_apply(m::expr_op_kind::lt,
                                                         a, b),
                                           d_tests_apply(m::expr_op_kind::lt,
                                                         b, c))),
                        "a bracketed relation one operand, no link of a "
                        "chain");
        D_TESTS_EXPECT( d_tests_parses("not a < b < c",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_not,
                                           d_tests_apply(
                                               m::expr_op_kind::logical_and,
                                               d_tests_apply(
                                                   m::expr_op_kind::lt,
                                                   a, b),
                                               d_tests_apply(
                                                   m::expr_op_kind::lt,
                                                   b, c)))) &&
                        d_tests_parses("f(a = b = c)",
                                       m::expr_call("f", {
                                           d_tests_apply(
                                               m::expr_op_kind::logical_and,
                                               d_tests_apply(
                                                   m::expr_op_kind::eq,
                                                   a, b),
                                               d_tests_apply(
                                                   m::expr_op_kind::eq,
                                                   b, c)) })) &&
                        d_tests_parses("a < not b < c",
                                       d_tests_apply(m::expr_op_kind::lt, a,
                                           d_tests_apply(
                                               m::expr_op_kind::logical_not,
                                               d_tests_apply(
                                                   m::expr_op_kind::lt,
                                                   b, c)))) &&
                        d_tests_parses("a < not b < c < x",
                                       d_tests_apply(m::expr_op_kind::lt, a,
                                           d_tests_apply(
                                               m::expr_op_kind::logical_not,
                                               d_tests_apply(
                                                   m::expr_op_kind::
                                                       logical_and,
                                                   d_tests_apply(
                                                       m::expr_op_kind::lt,
                                                       b, c),
                                                   d_tests_apply(
                                                       m::expr_op_kind::lt,
                                                       c, x))))),
                        "a chain under not, or in an argument, joined "
                        "there; not ends a chain");
        D_TESTS_EXPECT( d_tests_parses("2x < 3",
                                       d_tests_apply(m::expr_op_kind::lt,
                                           d_tests_apply(m::expr_op_kind::mul,
                                                         n2, x),
                                           n3),
                                       implied) &&
                        d_tests_parses("x y = y x",
                                       d_tests_apply(m::expr_op_kind::eq,
                                           d_tests_apply(m::expr_op_kind::mul,
                                                         x, y),
                                           d_tests_apply(m::expr_op_kind::mul,
                                                         y, x)),
                                       implied) &&
                        d_tests_is_error(m::expr_parse("2 not x", implied),
                                         m::expr_status::unexpected_token,
                                         2,
                                         "unexpected 'not'"),
                        "implied multiplication around the relations, and "
                        "never at a word");
        D_TESTS_EXPECT( d_tests_parses("true or false",
                                       d_tests_apply(
                                           m::expr_op_kind::logical_or,
                                           m::expr_constant("true"),
                                           m::expr_constant("false"))) &&
                        d_tests_parses("true(1)",
                                       m::expr_call("true", { n1 })),
                        "true and false constants; before a bracket, a "
                        "call");

        return;
    }

    // d_tests_parse_piecewise
    //   piecewise in the vocabulary of math/function/function.hpp: its
    // branches read into its arguments (S216), and their errors at their
    // places.
    void
    d_tests_parse_piecewise()
    {
        const m::expr_term a  = m::expr_variable("a");
        const m::expr_term b  = m::expr_variable("b");
        const m::expr_term x  = m::expr_variable("x");
        const m::expr_term n0 = d_tests_literal("0");
        const m::expr_term n1 = d_tests_literal("1");
        const m::expr_term n2 = d_tests_literal("2");
        const m::expr_term n3 = d_tests_literal("3");
        const m::expr_term n5 = d_tests_literal("5");

        D_TESTS_EXPECT( d_tests_parses(
                            "piecewise(when(x < 0, -x), otherwise(x))",
                            m::expr_call("piecewise", {
                                d_tests_apply(m::expr_op_kind::lt, x, n0),
                                d_tests_apply(m::expr_op_kind::neg, x),
                                x })) &&
                        d_tests_parses("piecewise(when(a, 1), when(b, 2))",
                                       m::expr_call("piecewise",
                                                    { a, n1, b, n2 })) &&
                        d_tests_parses("piecewise(otherwise(5))",
                                       m::expr_call("piecewise", { n5 })) &&
                        d_tests_parses("piecewise()",
                                       m::expr_call("piecewise", {})),
                        "piecewise(when(c, v), otherwise(d)): the call "
                        "piecewise(c, v, d)");
        D_TESTS_EXPECT( d_tests_parses(
                            "piecewise(when(a, piecewise(when(b, 1), "
                            "otherwise(2))), otherwise(3))",
                            m::expr_call("piecewise", {
                                a,
                                m::expr_call("piecewise", { b, n1, n2 }),
                                n3 })) &&
                        d_tests_parses("when + otherwise",
                                       d_tests_apply(m::expr_op_kind::add,
                                           m::expr_variable("when"),
                                           m::expr_variable("otherwise"))),
                        "piecewise in a branch's value; when and otherwise "
                        "with no bracket, variables");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("piecewise(x)"),
                                         m::expr_status::unexpected_token,
                                         10,
                                         "an argument of piecewise is "
                                         "when(c, v) or otherwise(v)") &&
                        d_tests_is_error(m::expr_parse(
                                             "piecewise(when(a, 1), x + 1)"),
                                         m::expr_status::unexpected_token,
                                         22,
                                         "an argument of piecewise is "
                                         "when(c, v) or otherwise(v)"),
                        "an argument of piecewise no branch: unexpected, at "
                        "its first token");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("piecewise(when(a))"),
                                         m::expr_status::wrong_arity,
                                         10,
                                         "when takes 2 arguments, not 1") &&
                        d_tests_is_error(m::expr_parse(
                                             "piecewise(when(a, b, x))"),
                                         m::expr_status::wrong_arity,
                                         10,
                                         "when takes 2 arguments, not 3") &&
                        d_tests_is_error(m::expr_parse("piecewise(when())"),
                                         m::expr_status::wrong_arity,
                                         10,
                                         "when takes 2 arguments, not 0") &&
                        d_tests_is_error(m::expr_parse(
                                             "piecewise(otherwise())"),
                                         m::expr_status::wrong_arity,
                                         10,
                                         "otherwise takes 1 argument, not "
                                         "0") &&
                        d_tests_is_error(m::expr_parse(
                                             "piecewise(otherwise(1, 2))"),
                                         m::expr_status::wrong_arity,
                                         10,
                                         "otherwise takes 1 argument, not "
                                         "2"),
                        "a branch of a number of arguments it does not take: "
                        "wrong_arity, at the branch");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse(
                                             "piecewise(otherwise(1), "
                                             "when(a, 2))"),
                                         m::expr_status::unexpected_token,
                                         10,
                                         "otherwise is the last branch of "
                                         "piecewise"),
                        "an otherwise before the last argument: unexpected");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("when(a, b)"),
                                         m::expr_status::unexpected_token,
                                         0,
                                         "when stands only as a whole "
                                         "argument of piecewise") &&
                        d_tests_is_error(m::expr_parse("1 + otherwise(a)"),
                                         m::expr_status::unexpected_token,
                                         4,
                                         "otherwise stands only as a whole "
                                         "argument of piecewise") &&
                        d_tests_is_error(m::expr_parse(
                                             "piecewise(when(a, b) + 1)"),
                                         m::expr_status::unexpected_token,
                                         10,
                                         "when stands only as a whole "
                                         "argument of piecewise") &&
                        d_tests_is_error(m::expr_parse(
                                             "piecewise((when(a, b)))"),
                                         m::expr_status::unexpected_token,
                                         11,
                                         "when stands only as a whole "
                                         "argument of piecewise") &&
                        d_tests_is_error(m::expr_parse(
                                             "piecewise(-when(a, b))"),
                                         m::expr_status::unexpected_token,
                                         11,
                                         "when stands only as a whole "
                                         "argument of piecewise") &&
                        d_tests_is_error(m::expr_parse("f(when(a, b))"),
                                         m::expr_status::unexpected_token,
                                         2,
                                         "when stands only as a whole "
                                         "argument of piecewise") &&
                        d_tests_is_error(m::expr_parse(
                                             "piecewise(when(otherwise(1), "
                                             "2))"),
                                         m::expr_status::unexpected_token,
                                         15,
                                         "otherwise stands only as a whole "
                                         "argument of piecewise"),
                        "a when or an otherwise anywhere else: unexpected, "
                        "at its name");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse(
                                             "piecewise(when(a), x)"),
                                         m::expr_status::wrong_arity,
                                         10,
                                         "when takes 2 arguments, not 1") &&
                        d_tests_is_error(m::expr_parse(
                                             "piecewise(when(a +, 1))"),
                                         m::expr_status::missing_operand,
                                         18,
                                         "an operand is missing before "
                                         "','") &&
                        d_tests_is_error(m::expr_parse(
                                             "piecewise(x, when(a +, 1))"),
                                         m::expr_status::unexpected_token,
                                         10,
                                         "an argument of piecewise is "
                                         "when(c, v) or otherwise(v)"),
                        "of two errors the further left; a branch's own "
                        "error, at its place");

        return;
    }

    // d_tests_parse_tokens
    //   expr_parse_tokens: the same terms from tokens; an end token ends
    // them; a token of no role, unexpected.
    void
    d_tests_parse_tokens()
    {
        typedef m::expr_token tok;
        const m::expr_token_kind  number = m::expr_token_kind::number;
        const m::expr_token_kind  name   = m::expr_token_kind::name;
        const m::expr_token_kind  symbol = m::expr_token_kind::symbol;
        const std::vector<tok>    sum    = { tok(name, "x", 0),
                                             tok(symbol, "+", 2),
                                             tok(number, "1", 4) };
        std::vector<tok>          ended  = sum;

        ended.push_back(tok());
        ended.back().offset = 5;

        const m::expr_result<m::expr_term> from_tokens =
            m::expr_parse_tokens(sum);
        const m::expr_result<m::expr_term> from_ended =
            m::expr_parse_tokens(ended);

        D_TESTS_EXPECT( (from_tokens.is_ok()) &&
                        (d_tests_same(from_tokens.value(),
                                      d_tests_x_plus_1())) &&
                        (from_ended.is_ok()) &&
                        (d_tests_same(from_ended.value(),
                                      d_tests_x_plus_1())),
                        "x + 1 from its tokens, with an end token or not");

        ended.push_back(tok(number, "2", 7));

        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse_tokens(ended),
                                         m::expr_status::unexpected_token,
                                         7,
                                         "unexpected '2'"),
                        "a token after an end token: unexpected");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse_tokens(
                                             { tok(number, "1/2", 0) }),
                                         m::expr_status::unexpected_token,
                                         0,
                                         "unexpected '1/2'") &&
                        d_tests_is_error(m::expr_parse_tokens(
                                             { tok(number, "-1", 0) }),
                                         m::expr_status::unexpected_token,
                                         0,
                                         "unexpected '-1'") &&
                        d_tests_is_error(m::expr_parse_tokens(
                                             { tok(symbol, "%", 0) }),
                                         m::expr_status::unexpected_token,
                                         0,
                                         "unexpected '%'") &&
                        d_tests_is_error(m::expr_parse_tokens(
                                             { tok(m::expr_token_kind::open,
                                                   "{",
                                                   0) }),
                                         m::expr_status::unexpected_token,
                                         0,
                                         "unexpected '{'"),
                        "a number no unsigned decimal, and a symbol and a "
                        "bracket of no role: unexpected");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse_tokens(
                                             { tok(symbol, "|", 0) }),
                                         m::expr_status::unexpected_token,
                                         0,
                                         "unexpected '|'") &&
                        d_tests_is_error(m::expr_parse_tokens(
                                             { tok(m::expr_token_kind::comma,
                                                   ",",
                                                   3) }),
                                         m::expr_status::unexpected_token,
                                         3,
                                         "unexpected ','") &&
                        d_tests_is_error(m::expr_parse_tokens(
                                             { tok(m::expr_token_kind::
                                                       semicolon,
                                                   ";",
                                                   4) }),
                                         m::expr_status::unexpected_token,
                                         4,
                                         "unexpected ';'"),
                        "| with the bars off, no operator; a comma outside "
                        "a call, and a semicolon outside a literal, "
                        "unexpected");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse_tokens(
                                             { tok(static_cast<
                                                       m::expr_token_kind>(
                                                       99),
                                                   "?",
                                                   1) }),
                                         m::expr_status::unexpected_token,
                                         1,
                                         "unexpected '?'"),
                        "a token of no kind: unexpected");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse_tokens(
                                             std::vector<tok>()),
                                         m::expr_status::missing_operand,
                                         0,
                                         "an operand is missing at the "
                                         "end") &&
                        d_tests_is_error(m::expr_parse_tokens(
                                             { tok(number, "1", 0),
                                               tok(symbol, "+", 2) }),
                                         m::expr_status::missing_operand,
                                         3,
                                         "an operand is missing at the "
                                         "end"),
                        "the end past the last token's text");

        return;
    }

    // d_tests_parse_errors
    //   each error at its place: the first a parse meets, a token of a later
    // step, then a bracket, then the grammar.
    void
    d_tests_parse_errors()
    {
        m::expr_syntax bars;

        bars.absolute_bars = true;

        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse(""),
                                         m::expr_status::missing_operand,
                                         0,
                                         "an operand is missing at the "
                                         "end") &&
                        d_tests_is_error(m::expr_parse("1 +  "),
                                         m::expr_status::missing_operand,
                                         5,
                                         "an operand is missing at the "
                                         "end") &&
                        d_tests_is_error(m::expr_parse("x mod"),
                                         m::expr_status::missing_operand,
                                         5,
                                         "an operand is missing at the "
                                         "end"),
                        "the text ending where an operand must stand");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("()"),
                                         m::expr_status::missing_operand,
                                         1,
                                         "an operand is missing before "
                                         "')'") &&
                        d_tests_is_error(m::expr_parse("(1 +)"),
                                         m::expr_status::missing_operand,
                                         4,
                                         "an operand is missing before "
                                         "')'"),
                        "a bracket closing where an operand must stand");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("(1 +"),
                                         m::expr_status::unbalanced,
                                         0,
                                         "'(' is never closed") &&
                        d_tests_is_error(m::expr_parse("(1 + (2"),
                                         m::expr_status::unbalanced,
                                         5,
                                         "'(' is never closed") &&
                        d_tests_is_error(m::expr_parse("1 + 2)"),
                                         m::expr_status::unbalanced,
                                         5,
                                         "')' closes no bracket") &&
                        d_tests_is_error(m::expr_parse(")("),
                                         m::expr_status::unbalanced,
                                         0,
                                         "')' closes no bracket"),
                        "unbalanced: a close that opens nothing, or the "
                        "innermost open never closed");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("(1 2)"),
                                         m::expr_status::unexpected_token,
                                         3,
                                         "unexpected '2'") &&
                        d_tests_is_error(m::expr_parse("1 2"),
                                         m::expr_status::unexpected_token,
                                         2,
                                         "unexpected '2'") &&
                        d_tests_is_error(m::expr_parse("* 2"),
                                         m::expr_status::unexpected_token,
                                         0,
                                         "unexpected '*'") &&
                        d_tests_is_error(m::expr_parse("1 * * 2"),
                                         m::expr_status::unexpected_token,
                                         4,
                                         "unexpected '*'") &&
                        d_tests_is_error(m::expr_parse("mod x"),
                                         m::expr_status::unexpected_token,
                                         0,
                                         "unexpected 'mod'") &&
                        d_tests_is_error(m::expr_parse("1.2.3"),
                                         m::expr_status::unexpected_token,
                                         3,
                                         "unexpected '.3'"),
                        "a token out of place: unexpected, at its offset");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("1 + #"),
                                         m::expr_status::bad_character,
                                         4,
                                         "no token begins with '#'"),
                        "a character no token begins with, from the "
                        "tokenizer");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("sin(,)"),
                                         m::expr_status::missing_operand,
                                         4,
                                         "an operand is missing before "
                                         "','") &&
                        d_tests_is_error(m::expr_parse("f(1,)"),
                                         m::expr_status::missing_operand,
                                         4,
                                         "an operand is missing before "
                                         "')'") &&
                        d_tests_is_error(m::expr_parse("f(1 +)"),
                                         m::expr_status::missing_operand,
                                         5,
                                         "an operand is missing before "
                                         "')'"),
                        "an argument missing, or ending where an operand "
                        "must stand: before its comma or its close");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("(1, 2)"),
                                         m::expr_status::unexpected_token,
                                         2,
                                         "unexpected ','") &&
                        d_tests_is_error(m::expr_parse("1, 2"),
                                         m::expr_status::unexpected_token,
                                         1,
                                         "unexpected ','") &&
                        d_tests_is_error(m::expr_parse("!x"),
                                         m::expr_status::unexpected_token,
                                         0,
                                         "unexpected '!'") &&
                        d_tests_is_error(m::expr_parse("f (x"),
                                         m::expr_status::unbalanced,
                                         2,
                                         "'(' is never closed"),
                        "a comma outside a call's brackets, and the "
                        "factorial before an operand: unexpected; a call "
                        "never closed: unbalanced");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("|x", bars),
                                         m::expr_status::unbalanced,
                                         0,
                                         "'|' is never closed") &&
                        d_tests_is_error(m::expr_parse("(|x)", bars),
                                         m::expr_status::unbalanced,
                                         1,
                                         "'|' is never closed") &&
                        d_tests_is_error(m::expr_parse("|x| |y|", bars),
                                         m::expr_status::unexpected_token,
                                         4,
                                         "unexpected '|'"),
                        "a bar never closed, or open where a bracket "
                        "closes: unbalanced; a bar after an operand, with "
                        "no product implied: unexpected");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("1 + * 2 + sin(,)"),
                                         m::expr_status::unexpected_token,
                                         4,
                                         "unexpected '*'") &&
                        d_tests_is_error(m::expr_parse("sin(,) + 1 + * 2"),
                                         m::expr_status::missing_operand,
                                         4,
                                         "an operand is missing before "
                                         "','"),
                        "of an argument's error and the climb's, the "
                        "further left");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("a <"),
                                         m::expr_status::missing_operand,
                                         3,
                                         "an operand is missing at the "
                                         "end") &&
                        d_tests_is_error(m::expr_parse("< a"),
                                         m::expr_status::unexpected_token,
                                         0,
                                         "unexpected '<'") &&
                        d_tests_is_error(m::expr_parse("a < < b"),
                                         m::expr_status::unexpected_token,
                                         4,
                                         "unexpected '<'") &&
                        d_tests_is_error(m::expr_parse("a and"),
                                         m::expr_status::missing_operand,
                                         5,
                                         "an operand is missing at the "
                                         "end") &&
                        d_tests_is_error(m::expr_parse("a not b"),
                                         m::expr_status::unexpected_token,
                                         2,
                                         "unexpected 'not'") &&
                        d_tests_is_error(m::expr_parse("or b"),
                                         m::expr_status::unexpected_token,
                                         0,
                                         "unexpected 'or'"),
                        "a relation or a word out of place: unexpected, or "
                        "an operand missing");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse("'"),
                                         m::expr_status::unexpected_token,
                                         0,
                                         "unexpected '''") &&
                        d_tests_is_error(m::expr_parse("x + '"),
                                         m::expr_status::unexpected_token,
                                         4,
                                         "unexpected '''") &&
                        d_tests_is_error(m::expr_parse("(')"),
                                         m::expr_status::unexpected_token,
                                         1,
                                         "unexpected '''"),
                        "a transpose where no operand ends before it: "
                        "unexpected");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_parse_tokens(
                                             { m::expr_token(
                                                   m::expr_token_kind::open,
                                                   "[",
                                                   0) }),
                                         m::expr_status::unbalanced,
                                         0,
                                         "'[' is never closed") &&
                        d_tests_is_error(m::expr_parse_tokens(
                                             { m::expr_token(
                                                   m::expr_token_kind::close,
                                                   "]",
                                                   2) }),
                                         m::expr_status::unbalanced,
                                         2,
                                         "']' closes no bracket"),
                        "a square bracket alone: unbalanced");

        return;
    }

    // d_tests_render
    //   expr_render: G5's spellings, brackets only where operands regroup,
    // literals as written.
    void
    d_tests_render()
    {
        m::expr_syntax bars;

        bars.absolute_bars = true;

        D_TESTS_EXPECT( d_tests_renders("2*x^2+1", "2 * x ^ 2 + 1") &&
                        d_tests_renders("x mod 3", "x mod 3") &&
                        d_tests_renders("((x))", "x"),
                        "infix with a space each side; no bracket kept "
                        "that needs none");
        D_TESTS_EXPECT( d_tests_renders("-x^2", "-x ^ 2") &&
                        d_tests_renders("(-x)^2", "(-x) ^ 2") &&
                        d_tests_renders("2^-3", "2 ^ (-3)") &&
                        d_tests_renders("-(a*b)", "-(a * b)") &&
                        d_tests_renders("--x", "--x") &&
                        d_tests_renders("a--b", "a - -b") &&
                        d_tests_renders("a*+b", "a * +b"),
                        "the unary - and + against their operands, "
                        "bracketed where they would regroup");
        D_TESTS_EXPECT( d_tests_renders("a-(b-c)", "a - (b - c)") &&
                        d_tests_renders("(a-b)-c", "a - b - c") &&
                        d_tests_renders("(a^b)^c", "(a ^ b) ^ c") &&
                        d_tests_renders("a^(b^c)", "a ^ b ^ c") &&
                        d_tests_renders("a/(b*c)", "a / (b * c)") &&
                        d_tests_renders("(1+2)*3", "(1 + 2) * 3"),
                        "brackets on the side an operator does not group "
                        "to");
        D_TESTS_EXPECT( d_tests_renders("0.10 + 1e3 + 007",
                                        "0.10 + 1e3 + 007"),
                        "literals as written");
        D_TESTS_EXPECT( (m::expr_render(m::expr_application(
                             m::expr_op_kind::pow,
                             m::expr_number(m::value(-1), "-1"),
                             d_tests_literal("2"))) == "(-1) ^ 2") &&
                        (m::expr_render(m::expr_number(m::value::ratio(1, 3),
                                                       "1/3")) == "(1/3)"),
                        "a literal no unsigned decimal, in brackets");
        D_TESTS_EXPECT( d_tests_renders("sin(x)", "sin(x)") &&
                        d_tests_renders("atan2(y,x)", "atan2(y, x)") &&
                        d_tests_renders("f()", "f()") &&
                        d_tests_renders("foo(a+b,-c)", "foo(a + b, -c)") &&
                        d_tests_renders("sin(x)^2", "sin(x) ^ 2") &&
                        d_tests_renders("-f(x)", "-f(x)"),
                        "a call: its function's name and its arguments, a "
                        "primary, never bracketed");
        D_TESTS_EXPECT( d_tests_renders("n!", "n!") &&
                        d_tests_renders("(x+1)!", "(x + 1)!") &&
                        d_tests_renders("(-x)!", "(-x)!") &&
                        d_tests_renders("x!!", "x!!") &&
                        d_tests_renders("2^3!", "2 ^ 3!") &&
                        d_tests_renders("(2^3)!", "(2 ^ 3)!") &&
                        d_tests_renders("-3!", "-3!"),
                        "the factorial after its operand, bracketed where "
                        "it would regroup");
        D_TESTS_EXPECT( d_tests_renders("2*pi*e", "2 * pi * e") &&
                        d_tests_renders("|x-1|", "abs(x - 1)", bars),
                        "a constant its name; bars the call abs");
        D_TESTS_EXPECT( (m::expr_render(m::expr_term()).empty()) &&
                        (m::expr_render(d_tests_apply(
                             m::expr_op_kind::none,
                             d_tests_literal("1"),
                             d_tests_literal("2"))) == "?(1, 2)") &&
                        (m::expr_render(m::expr_application(
                             m::expr_op_kind::index,
                             std::vector<m::expr_term>())) == "?()"),
                        "a term made by default, the empty string; an "
                        "operator of no kind the table has, ?(1, 2), "
                        "indexing of no operand among them");
        D_TESTS_EXPECT( d_tests_renders("a<b", "a < b") &&
                        d_tests_renders("a!=b", "a != b") &&
                        d_tests_renders("a<=b", "a <= b") &&
                        d_tests_renders("a>=b", "a >= b") &&
                        d_tests_renders("a=b", "a = b") &&
                        d_tests_renders("a>b", "a > b") &&
                        d_tests_renders("x > 0 and x < 1",
                                        "x > 0 and x < 1") &&
                        d_tests_renders("a or b and c", "a or b and c") &&
                        d_tests_renders("(a or b) and c", "(a or b) and c"),
                        "the relations and logic as G5 spells them, "
                        "bracketed only where they would regroup");
        D_TESTS_EXPECT( d_tests_renders("not(a)", "not a") &&
                        d_tests_renders("not (a and b)", "not (a and b)") &&
                        d_tests_renders("not a = b", "not a = b") &&
                        d_tests_renders("-(not a)", "-(not a)") &&
                        d_tests_renders("not -a", "not -a") &&
                        d_tests_renders("a = not b", "a = (not b)"),
                        "not before its operand, spaced as a word");
        D_TESTS_EXPECT( d_tests_renders("(a < b) < c", "(a < b) < c") &&
                        d_tests_renders("a < (b < c)", "a < (b < c)") &&
                        d_tests_renders("a < b <= c", "a < b and b <= c") &&
                        d_tests_renders("true and false", "true and false"),
                        "a relation of a relation, bracketed; a chain, as "
                        "its links");
        D_TESTS_EXPECT( d_tests_renders("piecewise(when(x<0,-x),otherwise(x))",
                                        "piecewise(when(x < 0, -x), "
                                        "otherwise(x))") &&
                        (m::expr_render(m::expr_call(
                             "piecewise",
                             { m::expr_variable("a"),
                               m::expr_variable("b"),
                               m::expr_variable("c") })) ==
                             "piecewise(when(a, b), otherwise(c))") &&
                        (m::expr_render(m::expr_call(
                             "piecewise",
                             { m::expr_variable("a"),
                               m::expr_variable("b") })) ==
                             "piecewise(when(a, b))") &&
                        (m::expr_render(m::expr_call("piecewise", {})) ==
                             "piecewise()"),
                        "piecewise in its branches: its arguments two by "
                        "two, an odd last one otherwise");

        return;
    }

    // d_tests_renders_number
    //   whether a number with no literal renders as _want.
    bool
    d_tests_renders_number(
        const m::value& _number,
        const char*     _want
    )
    {
        return (m::expr_render(m::expr_number(_number)) == _want);
    }

    // d_tests_render_numbers
    //   numbers a fold made: exact where exact, the shortest that reads back
    // where floating, and names for what has no literal.
    void
    d_tests_render_numbers()
    {
        const double inf = std::numeric_limits<double>::infinity();

        D_TESTS_EXPECT( d_tests_renders_number(m::value(5), "5") &&
                        d_tests_renders_number(m::value(-5), "(-5)") &&
                        d_tests_renders_number(
                            m::value(std::numeric_limits<dj_math_imax>::min()),
                            "(-9223372036854775807 - 1)") &&
                        d_tests_renders_number(m::value::ratio(1, 3),
                                               "(1/3)") &&
                        d_tests_renders_number(m::value::ratio(-1, 3),
                                               "(-1/3)") &&
                        d_tests_renders_number(
                            m::value::ratio(
                                std::numeric_limits<dj_math_imax>::min(),
                                3),
                            "((-9223372036854775807 - 1)/3)"),
                        "exact: an integer, a fraction, in brackets where "
                        "negative or a fraction; the least numerator as one "
                        "more less 1");
        D_TESTS_EXPECT( d_tests_renders_number(m::value(0.1), "0.1") &&
                        d_tests_renders_number(m::value(0.1f), "0.1") &&
                        d_tests_renders_number(m::value(0.1L), "0.1") &&
                        d_tests_renders_number(m::value(-2.5), "(-2.5)") &&
                        d_tests_renders_number(m::value(1e30), "1e+30") &&
                        d_tests_renders_number(m::value(-0.0), "(-0)"),
                        "floating: the shortest that reads back in its "
                        "type");
        D_TESTS_EXPECT( d_tests_renders_number(m::value(inf), "inf") &&
                        d_tests_renders_number(m::value(-inf), "(-inf)") &&
                        d_tests_renders_number(
                            m::value(std::numeric_limits<double>::quiet_NaN()),
                            "nan") &&
                        d_tests_renders_number(
                            m::value(std::numeric_limits<float>::
                                         infinity()),
                            "inf") &&
                        d_tests_renders_number(
                            m::value(-std::numeric_limits<long double>::
                                          infinity()),
                            "(-inf)") &&
                        d_tests_renders_number(
                            m::value(std::numeric_limits<float>::
                                         quiet_NaN()),
                            "nan") &&
                        d_tests_renders_number(
                            m::value(std::numeric_limits<long double>::
                                         quiet_NaN()),
                            "nan") &&
                        d_tests_renders_number(m::value(true), "true") &&
                        d_tests_renders_number(m::value(false), "false") &&
                        d_tests_renders_number(m::value(), "none"),
                        "inf, nan, the booleans and none by name");
        D_TESTS_EXPECT( d_tests_renders_number(
                            m::value(D_MATH_VALUE_ERROR_DOMAIN),
                            "domain_error") &&
                        d_tests_renders_number(
                            m::value(D_MATH_VALUE_ERROR_POLE),
                            "pole_error") &&
                        d_tests_renders_number(
                            m::value(D_MATH_VALUE_ERROR_RANGE),
                            "range_error") &&
                        d_tests_renders_number(
                            m::value(D_MATH_VALUE_ERROR_INVALID),
                            "invalid_error") &&
                        d_tests_renders_number(
                            m::value(D_MATH_VALUE_ERROR_MEMORY),
                            "memory_error"),
                        "each error by name");
        D_TESTS_EXPECT( d_tests_renders_number(
                            d_tests_wide_of("123456789012345678901234567890"),
                            "123456789012345678901234567890") &&
                        d_tests_renders_number(
                            d_tests_value_of("-x",
                                             m::at("x",
                                                   d_tests_wide_of(
                                                       "12345678901234567890"
                                                       "1"))),
                            "(-123456789012345678901)"),
                        "a boxed number as its kind writes it, in brackets "
                        "where negative");

        return;
    }


    // d_tests_rng
    //   struct: a small generator of the test's own, a 64-bit linear
    // congruential one, so that every build draws the same terms.
    struct d_tests_rng
    {
        dj_math_umax m_state;

        explicit d_tests_rng(
            dj_math_umax _seed
        )
            : m_state(_seed)
        {}

        // below
        //   a number in [0, _n).
        unsigned
        below(
            unsigned _n
        )
        {
            const dj_math_umax word = 0xFFFFFFFFFFFFFFFFu;

            m_state = ( (m_state * 6364136223846793005u) +
                        1442695040888963407u ) & word;

            return static_cast<unsigned>((m_state >> 33) % _n);
        }
    };

    // d_tests_alphabet
    //   struct: what a random term is drawn from: literals, names and the
    // kinds of operator; and, where any, constants and the functions of
    // calls.
    struct d_tests_alphabet
    {
        const char* const*     m_literals;
        unsigned               m_literal_count;
        const char* const*     m_names;
        unsigned               m_name_count;
        const m::expr_op_kind* m_kinds;
        unsigned               m_kind_count;
        const char* const*     m_constants;
        unsigned               m_constant_count;
        const char* const*     m_functions;
        unsigned               m_function_count;
    };

    // d_tests_random_term
    //   a term drawn from _alphabet, at most _depth operators deep; its left
    // operand drawn before its right, so that the draw is one on every
    // compiler.
    m::expr_term
    d_tests_random_term(
        d_tests_rng&            _rng,
        const d_tests_alphabet& _alphabet,
        unsigned                _depth
    )
    {
        const unsigned leaves = ( (_alphabet.m_constant_count != 0) ? 3u
                                                                    : 2u );
        const unsigned calls  = ( (_alphabet.m_function_count != 0) ? 1u
                                                                    : 0u );
        const unsigned pick   = _rng.below( (_depth == 0)
                                                ? leaves
                                                : (leaves + 4u + calls) );

        // a leaf: a literal, a name, or a constant
        if (pick == 0)
        {
            return d_tests_literal(
                _alphabet.m_literals[_rng.below(_alphabet.m_literal_count)]);
        }

        if (pick == 1)
        {
            return m::expr_variable(
                _alphabet.m_names[_rng.below(_alphabet.m_name_count)]);
        }

        if (pick + 1 == leaves)
        {
            return m::expr_constant(
                _alphabet.m_constants[_rng.below(
                    _alphabet.m_constant_count)]);
        }

        // a call, of none to three arguments, each drawn in turn
        if (pick == leaves + 4u)
        {
            const char* const      function =
                _alphabet.m_functions[_rng.below(
                    _alphabet.m_function_count)];
            const unsigned         count    = _rng.below(4u);
            std::vector<m::expr_term> arguments;

            for (unsigned i = 0; i < count; ++i)
            {
                arguments.push_back(d_tests_random_term(_rng,
                                                        _alphabet,
                                                        _depth - 1));
            }

            return m::expr_call(function, arguments);
        }

        const m::expr_op_kind kind =
            _alphabet.m_kinds[_rng.below(_alphabet.m_kind_count)];
        const m::expr_term    left = d_tests_random_term(_rng,
                                                         _alphabet,
                                                         _depth - 1);

        // the unary, not and the factorial, on one operand
        if ( (kind == m::expr_op_kind::neg) ||
             (kind == m::expr_op_kind::pos) ||
             (kind == m::expr_op_kind::logical_not) ||
             (kind == m::expr_op_kind::factorial) )
        {
            return d_tests_apply(kind, left);
        }

        const m::expr_term right = d_tests_random_term(_rng,
                                                       _alphabet,
                                                       _depth - 1);

        return d_tests_apply(kind, left, right);
    }

    // d_tests_language
    //   the alphabet of 2d's language, calls, constants, the factorial, the
    // relations, logic and piecewise among it: what the round trip, and the
    // handle's agreement with the entries, draw their terms from.
    d_tests_alphabet
    d_tests_language()
    {
        static const char* const     literals[]  = { "0", "1", "2", "10",
                                                     "0.5", "1.25", "3e2",
                                                     "007", ".5", "5.",
                                                     "1e-3", "123456789" };
        static const char* const     names[]     = { "a", "b", "x", "y_1",
                                                     "mod2", "when" };
        static const m::expr_op_kind kinds[]     = {
            m::expr_op_kind::add, m::expr_op_kind::sub, m::expr_op_kind::mul,
            m::expr_op_kind::div, m::expr_op_kind::mod, m::expr_op_kind::pow,
            m::expr_op_kind::neg, m::expr_op_kind::pos,
            m::expr_op_kind::factorial, m::expr_op_kind::eq,
            m::expr_op_kind::ne, m::expr_op_kind::lt, m::expr_op_kind::le,
            m::expr_op_kind::gt, m::expr_op_kind::ge,
            m::expr_op_kind::logical_and, m::expr_op_kind::logical_or,
            m::expr_op_kind::logical_not };
        static const char* const     constants[] = { "pi", "e", "true",
                                                     "false" };
        static const char* const     functions[] = { "sin", "atan2", "f",
                                                     "g_2", "piecewise",
                                                     "all_of" };
        const d_tests_alphabet       alphabet    = { literals, 12,
                                                     names, 6,
                                                     kinds, 18,
                                                     constants, 4,
                                                     functions, 6 };

        return alphabet;
    }

    // d_tests_round_trip
    //   roadmap Phase 2's round trip: random terms of 2d's language
    // (d_tests_language) rendered and parsed back as themselves, implied
    // multiplication on or off, and rendered the same again.
    void
    d_tests_round_trip()
    {
        const d_tests_alphabet alphabet     = d_tests_language();
        d_tests_rng            rng(20261023u);
        m::expr_syntax         implied;
        std::size_t            back         = 0;
        std::size_t            back_implied = 0;
        std::size_t            stable       = 0;

        implied.implied_multiplication = true;

        // 2,000 terms, up to five operators deep
        for (unsigned i = 0; i < 2000; ++i)
        {
            const m::expr_term term = d_tests_random_term(rng,
                                                          alphabet,
                                                          1 + (i % 5));
            const std::string  text = m::expr_render(term);
            const m::expr_result<m::expr_term> parsed = m::expr_parse(text);
            const m::expr_result<m::expr_term> parsed_implied =
                m::expr_parse(text, implied);

            back         += ( ( (parsed.is_ok()) &&
                                (d_tests_same(parsed.value(), term)) )
                                  ? 1 : 0 );
            back_implied += ( ( (parsed_implied.is_ok()) &&
                                (d_tests_same(parsed_implied.value(),
                                              term)) )
                                  ? 1 : 0 );
            stable       += ( ( (parsed.is_ok()) &&
                                (m::expr_render(parsed.value()) == text) )
                                  ? 1 : 0 );
        }

        D_TESTS_EXPECT( (back == 2000),
                        "2,000 random terms rendered and parsed back as "
                        "themselves");
        D_TESTS_EXPECT( (back_implied == 2000),
                        "the same with implied multiplication on");
        D_TESTS_EXPECT( (stable == 2000),
                        "each parsed back renders as it did");

        return;
    }

    // d_tests_folds_agree
    //   random terms of whole numbers under + - * and the unary - and +:
    // each type's value the exact one, as each is exact there.
    void
    d_tests_folds_agree()
    {
        static const char* const     literals[] = { "0", "1", "2", "3", "4",
                                                    "5", "6", "7", "8",
                                                    "9" };
        static const char* const     names[]    = { "a", "b" };
        static const m::expr_op_kind kinds[]    = {
            m::expr_op_kind::add, m::expr_op_kind::sub, m::expr_op_kind::mul,
            m::expr_op_kind::neg, m::expr_op_kind::pos };
        const d_tests_alphabet       alphabet   = { literals, 10,
                                                    names, 2,
                                                    kinds, 5,
                                                    D_NULLPTR, 0,
                                                    D_NULLPTR, 0 };
        const m::expr_bindings       at         = m::at("a", 7).at("b", -3);
        d_tests_rng                  rng(1429u);
        std::size_t                  agree      = 0;

        // 1,000 terms, up to three operators deep, each value below 2^53
        for (unsigned i = 0; i < 1000; ++i)
        {
            const m::expr_term term = d_tests_random_term(rng,
                                                          alphabet,
                                                          1 + (i % 3));
            const m::expr_result<m::value> exact =
                m::expr_evaluate(term, at, m::expr_context());
            const m::expr_result<double> in_double =
                m::expr_evaluate<double>(term, at);
            const m::expr_result<long double> in_long =
                m::expr_evaluate<long double>(term, at);
            dj_math_imax whole = 0;

            agree += ( ( (exact.is_ok()) &&
                         (exact.value().to(whole)) &&
                         (in_double.is_ok()) &&
                         (d_tests_same_real(in_double.value() + 0.0,
                                            static_cast<double>(whole))) &&
                         (in_long.is_ok()) &&
                         (d_tests_same_real(in_long.value() + 0.0L,
                                            static_cast<long double>(
                                                whole))) )
                           ? 1 : 0 );
        }

        D_TESTS_EXPECT( (agree == 1000),
                        "1,000 random whole terms: double's and long "
                        "double's values the exact ones");

        return;
    }

    // d_tests_numbers_read_back
    //   a number a fold made, rendered and parsed back: the same number,
    // exactly where exact, in its type where floating.
    void
    d_tests_numbers_read_back()
    {
        const m::value exact[] = {
            m::value(0), m::value(5), m::value(-5),
            m::value(std::numeric_limits<dj_math_imax>::min()),
            m::value::ratio(1, 3), m::value::ratio(-7, 2),
            m::value::ratio(std::numeric_limits<dj_math_imax>::min(), 3) };
        const double   doubles[] = { 0.1, -2.5, 1e30, 1e-310,
                                     std::numeric_limits<double>::max() };
        const float    floats[]  = { 0.1f, 3.4028235e38f, -1e-40f };
        std::size_t    same      = 0;

        // exact
        for (std::size_t i = 0; i < 7; ++i)
        {
            const std::string text = m::expr_render(m::expr_number(exact[i]));

            same += ( d_tests_valued(text.c_str(), exact[i]) ? 1 : 0 );
        }

        // floating, in their own types
        for (std::size_t i = 0; i < 5; ++i)
        {
            const std::string text = m::expr_render(m::expr_number(
                                         m::value(doubles[i])));

            same += ( d_tests_typed<double>(text.c_str(), doubles[i])
                          ? 1 : 0 );
        }

        for (std::size_t i = 0; i < 3; ++i)
        {
            const std::string text = m::expr_render(m::expr_number(
                                         m::value(floats[i])));

            same += ( d_tests_typed<float>(text.c_str(), floats[i]) ? 1 : 0 );
        }

        const long double longs[] = { 0.1L,
                                      std::numeric_limits<long double>::max(),
                                      -std::numeric_limits<long double>::
                                          denorm_min() };

        for (std::size_t i = 0; i < 3; ++i)
        {
            const std::string text = m::expr_render(m::expr_number(
                                         m::value(longs[i])));

            same += ( d_tests_typed<long double>(text.c_str(), longs[i])
                          ? 1 : 0 );
        }

        D_TESTS_EXPECT( (same == 18),
                        "numbers a fold made read back as the same number: "
                        "exactly where exact, in their types where "
                        "floating");

        return;
    }
}  // namespace

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_TESTS_CPP_MATH_EXPR_T_EXPR_FACE_PARSING_HPP
