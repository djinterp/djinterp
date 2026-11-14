/*******************************************************************************
* djinterp [cpp/math]                                               function.hpp
*
* The functions and named constants of the run-time expression language: the
* tables a fold looks a call's function and a constant's value up in, and the
* arithmetic beyond value.h's that both they and the operators need.
*   A FUNCTION is an entry of the function table: its name; the numbers of
* arguments it takes; an evaluator for each type a fold computes in, float,
* double and long double from the C core's kernels, and math::value exactly
* where it can be, each given the arguments and the fold's context
* (expr_context, S340); and a slot for the rule of its partial derivatives,
* which the derivative (Phase 4, derivative.hpp) asks of it. A call names
* its function, and its function is the table's entry of that name.
*   THE CONTEXT's real_results is an option of each evaluator (S340): where
* it is set, every standard function's evaluator as a value gives a domain
* error where its real arguments' value is complex alone or a quaternion --
* sqrt(-4), log(-1), asin(2), log2(-8), pow(-8, 1/3), eig of a real matrix
* with a complex pair -- complex(x, y) and quat(w, x, y, z) apart, as Julia's
* and C's are (S272); in a type it changes no evaluator's answer, a real
* type's being a NaN there already, and evaluate.hpp's fold in a complex type
* keeps a real argument's NaN rather than ask the complex evaluator. A
* caller's table gives its evaluators the same context, to read as its
* functions need.
*   A NAMED CONSTANT is an entry of the constant table: its name, and its value
* in each type: the 40-digit literal of constants::of<T> (constants.hpp),
* rounded to each type as of<T> rounds it, and as a value the long double
* nearest it; or, for a constant whose step has not come, the message of the
* expr_status::not_yet a fold answers it with, which no standard constant has
* from step 24 (3f). constants.hpp itself is not included: from C++14 it
* brings math/expression.hpp, whose names x, y, z, t, r, rho, theta and phi
* would stand in djinterp::math beside this language's.
*   THE TABLES are one kind, expr_table, of entries kept by name. The
* standard ones are the language's own; a caller may build others.
*   THE STANDARD FUNCTIONS are the C core's: sqrt, cbrt, sin, cos, tan, asin,
* acos, atan, atan2, sinh, cosh, tanh, exp, exp2, log, log2, log10, pow,
* hypot, fmod, remainder, floor, ceil, round and trunc, each of the number of
* arguments C gives it, and log1p, expm1, asinh, acosh and atanh, which the C
* core has from chunk 37 (S203); the language's abs, factorial and sign, and
* min and max of any number of arguments from 1 (S338); the complex numbers'
* complex(x, y), real, imag, conj and arg (S271, complex.hpp); from the
* vocabulary of math/function/function.hpp, piecewise, all_of and any_of, each
* of any number of arguments from 1; and linear algebra's, transpose, det,
* inv, trace, norm and eig of one argument and dot, cross and solve of two
* (S256, S350); and the quaternions' quat(w, x, y, z), parts of one argument
* and rotate of two (S275, quaternion.hpp), and rotmat and rotquat of one and
* slerp of three (S391, rotation.hpp): 58 in all. log is the natural
* logarithm, as in C (gate G5). The constants are pi and e, as G5 says, and
* the booleans true and false; im, the imaginary unit, so that 3 + 4im is a
* complex number (S271); and jm and km, the quaternions' units, so that 1 +
* 2jm - km is a quaternion (S275).
*   IN A TYPE, a function is the C core's plain form in it, the C library's, as
* ^ is (S194): fast, and within an ulp of the _c forms (gate G6); abs is |x|.
* factorial(n) is n! for a whole n >= 0, the product rounded to the type as it
* is made, an infinity past the type's range; a NaN for any other n, as the C
* core has no gamma function. sign(x) is 1 or -1 by x's sign, a 0 or a NaN
* itself, as Julia's is; min and max IEEE 754-2019's minimum and maximum, a NaN
* where any argument is one and -0 below +0 (S338), where C's fmin and fmax give
* the other argument for a NaN, as IEEE 754-2008's minNum and maxNum did, and
* either of two zeros. Every number in a real type is real: real(x) is x,
* imag(x) 0, conj(x) x, arg(x) carg(x + 0i), pi where x is negative or -0 and +0
* otherwise, complex(x, y) x where y is 0 and a NaN otherwise, and im a NaN, as
* sqrt(-1) is (S271, S332); so too quat(w, x, y, z) w where x, y and z are 0,
* and a NaN otherwise, and jm and km NaNs; parts, rotate, rotmat, rotquat and
* slerp, which give or take vectors, matrices and quaternions, are NaNs, as
* cross is (S275, S391).
*   IN A COMPLEX TYPE, math::complex<T> of float, double or long double (S339),
* a function with a principal complex value is the face's by its own name, so
* cplx.h's plain form at run time, on the C library's real functions, as the
* owner ruled (S336), as a real type takes the C library's: sqrt, exp, log, pow,
* the trigonometric and hyperbolic functions and their inverses, and exp2, log2,
* log10, log1p and expm1, whose complex forms cplx.h has from chunk 42 (S335,
* S342), the last five's plain forms their _c forms' bits; one of the real
* line's order is its real form of the real parts, plus +0i, where every
* argument is real, and a NaN in both parts otherwise; abs, real, imag and arg
* are the face's, plus +0i, conj its conjugate, complex(x, y) of real x and y
* the number x + yi, sign(z) z / |z| and a 0 itself, and min and max of real
* numbers alone; quat(w, x, y, z) of real arguments w + xi where y and z are 0,
* and a NaN in both parts otherwise; abs and arg there are the plain forms too.
* The constants are their real values plus +0i, im 0 + 1i, and jm and km NaNs in
* both parts, as no complex number is a unit past im. evaluate.hpp's fold there
* keeps a real number real, as the value does.
*   WHERE A VALUE AND math::complex<T> MAY DIFFER (S336): a function with a
* principal complex value, computed densely as a value, is cplx.h's _c form,
* one long double kernel rounded once, the same bits at every level and on
* every compiler, within a step of the correctly rounded parts for float and
* double; computed in a complex type it is the plain form, its bits the C
* library's, within the plain bounds cplx.h's banner states, up to 6 steps.
* So sqrt(z), exp(z) and the rest of one inexact z may differ in their last
* bits between a value and the fold in math::complex<T>, each within its
* bound; an exact value, a Gaussian rational (S337), is the value's alone;
* the modulus and the argument of a dense number as their own forms; and
* the arithmetic + - * /, which comes once, and exp2, log2, log10, log1p and
* expm1, whose plain forms are their _c forms, never differ.
*   AS A VALUE, a function is exact wherever its value is a rational number
* of the family: abs, floor, ceil, round, trunc, fmod and remainder always;
* sqrt and cbrt where the argument's terms are whole squares or cubes, hypot
* where the sum of the squares is a rational square, pow as ^ is; factorial
* for a whole n >= 0; exp2 at a whole number, log2 and log10 at a power of
* 2 or 10; and the others at the one point each where their value is
* rational: sin, tan, asin, atan, sinh, tanh, log1p, expm1, asinh and atanh
* at 0, which is 0; cos, cosh and exp at 0, which is 1; acos, log and acosh
* at 1, which is 0; atan2(0, x) for x > 0, which is 0; and sign, min and max
* everywhere. Nowhere else is any of them rational (Lindemann and
* Weierstrass), so nowhere else is a value lost. An exact argument outside
* a function's domain, where it has no complex value either, is a domain
* error: factorial of a negative number or a fraction, fmod or remainder by
* 0, atan2(0, 0); the logarithm of 0, log1p(-1), and atanh(1) and
* atanh(-1) are poles. Elsewhere a function's value is the C core's in the
* kind the tower
* gives: the widest of the inexact arguments' kinds, a long double where
* all are exact, as ^'s is. Errors pass, the first; a boolean or none is a
* domain error; every result carries its arguments' flags.
*   COMPLEX NUMBERS (complex.hpp, step 23, 3e) are values too. complex(x, y) is
* the number of two real parts, exact where both are, a real number where they
* are exact and y is 0; real, imag and conj give a number's parts and conjugate,
* a real number's imaginary part a 0 of its kind; arg its angle in (-pi, pi],
* exactly 0 for an exact real number not negative, and otherwise cplx.h's arg_c
* in the tower's kind; abs and the bars its modulus, exact where it is rational,
* |3 + 4im| being 5. Each function of a complex argument gives its principal
* value, with C99 Annex G's cuts (S271): exact where it is a Gaussian rational,
* sqrt(2im) being 1 + im, sqrt(3 + 4im) 2 + im and sign(3 + 4im) 3/5 + 4/5im, an
* exact number to a whole power by repeated squaring and to a rational p/q where
* its principal q-th root is a Gaussian rational, (2im) ^ (1/2) being 1 + im
* (S337); and otherwise the face's _c form in the tower's type, a long double
* where every argument is exact, so cplx.h's _c form, dense and never made real
* (S336). So too a real argument where the function has no real value but a
* complex one (S272): sqrt and log of a negative number, sqrt(-4) being 2im and
* log(-1) pi im; asin, acos and atanh past -1 or 1, and acosh below 1, each
* taken as x + 0i, so the value Annex G gives on the upper side of the cut,
* asin(2) being pi/2 + 1.317...im and acos(2) -1.317...im, as C, Python, NumPy
* and Scilab give them, where MATLAB takes asin's and acos's cuts past 1 from
* below, its asin(2) 1.5708 - 1.3170i and acos(2) 0 + 1.3170i, the imaginary
* parts' signs the opposite (S341); and a negative base to a finite fractional
* power, (-8) ^ (1/3) being 1 + 1.732...im, where cbrt(-8) is -2. Of a complex
* argument, the functions of the real line's order, cbrt, floor, ceil, round,
* trunc, fmod, remainder, mod, atan2, hypot, the factorial, min and max, are a
* domain error (S271). exp2, log2, log10, log1p and expm1 of a complex argument,
* and log2 and log10 of a negative number and log1p of one below -1, are their
* principal values with cplx.h's forms of them (S342), dense, none being a
* Gaussian rational: log2(-8) is 3 + 4.532...im, its real part exactly 3, the
* real function's on the axis (S343). real, imag, conj and arg of a matrix are
* its elements', each element's, and complex(A, B) of two matrices of one shape
* the matrix of complex(a_ij, b_ij), of a matrix and a number a domain error
* (S350).
*   QUATERNIONS (quaternion.hpp, step 24, 3f) are values too. quat(w, x, y, z)
* is the number of four real parts, exact where all four are, a complex
* number where they are exact and y and z are 0, and a real one where x is 0
* too (S275); parts(q) the vector [w, x, y, z], of a real or complex number
* too, its missing parts 0; real and conj its real part and conjugate; abs
* and the bars its norm, exact where it is rational, |1 + 2im + 2jm + 4km|
* being 5; inv(q) its inverse, q* / |q|^2, exact for exact parts; and sign(q)
* q / |q|, exact where |q| is rational. exp, log and sqrt of a quaternion are
* their principal values, the complex ones in q's plane, w + |v| u for u = v
* / |v| (S364), in the tower's type, quat.h's _c forms, dense; sqrt exact
* where the root is a quaternion of rationals, sqrt(-2 + 2im + 2jm + 2km)
* being 1 + im + jm + km. q ^ r takes a real exponent: an exact whole r by
* repeated squaring, inv(q)'s for a negative one; an exact rational p/s
* where q's principal s-th root is a quaternion of rationals, (-46 + 3im +
* 6jm + 6km) ^ (1/3) being 2 + im + 2jm + 2km; otherwise quat.h's pow_c in
* the tower's type. A quaternion exponent, of any base, and a complex one of
* a quaternion are a domain error, as their two orders give two values; so
* are imag, arg and complex(x, y) of a quaternion, whose parts parts gives,
* the functions of the real line's order, and the trigonometric and
* hyperbolic functions and their inverses, which the language does not give
* a quaternion. rotate(q, v) is the 3-vector q v q^-1, of a vector of 1 by 3
* or 3 by 1 of real numbers, of its shape: exact where q and v are, and
* otherwise quat.h's accurate rotation (S360); q may be any number but an
* exact 0, which names no rotation, a real one rotating nothing. rotmat(q) is
* that rotation's 3 by 3 matrix, rotquat(R) the unit quaternion of a
* rotation's matrix, w >= 0, and slerp(p, q, t) the shorter arc between two
* rotations at a constant speed (S391, rotation.hpp). Linear algebra takes
* quaternions (S388, S390, matrix.hpp): det of one, or of a matrix of them,
* is a domain error (S277); eig of one its standard eigenvalue, w + |v|i,
* and of a matrix of them Zhang's standard right eigenvalues (S389); dot(p,
* q) conj(p) q; solve(q, B) q^-1 B, on the left.
*   THE ARITHMETIC of ^ and mod, which value.h does not have, is here too,
* beside the functions': evaluate.hpp's folds apply it. A ^ n of a matrix is
* the matrix kind's power, a square matrix to a whole n, a negative one its
* inverse's (matrix.hpp, S248, S259), and so is pow(A, n); a matrix in mod, or
* an argument of any other function but a value of piecewise's, an argument of
* linear algebra's and one of complex, real, imag, conj and arg (S350), is a
* domain error.
*   LINEAR ALGEBRA'S FUNCTIONS are matrix.hpp's as values: transpose, det,
* inv, trace, norm, eig, dot, cross and solve of matrices, of complex elements
* too, exact for exact elements, a number taken as a scalar: transpose(A) the
* transpose, where A' conjugates (S350); norm of the moduli, dot(a, b) the sum
* of conj(a_i) b_i, and eig every eigenvalue, complex where it is
* (S351, S352); inv(q) of a quaternion its inverse (S275). In a type, where
* every value is a number, a scalar's, as in
* Julia: transpose, det, trace and eig of x are x, inv(x) 1 / x, norm(x) |x|,
* dot(x, y) x y, solve(a, b) b / a, and cross a NaN, as it takes no number; in
* a complex type the same, dot(x, y) conj(x) y, and inv(z) 1 / z, a real 1
* taken as itself (G.5, S351).
*   THE RELATIONS AND LOGIC, = != < <= > >= and and, or and not, are here as
* well. As a value a relation is a boolean: of two numbers, by value.h's exact
* order, none holding of a NaN but !=; of a complex number or a quaternion
* and a number, = and != by their parts, and <, <=, > and >= a domain error,
* as no order of the complex numbers or the quaternions is a field's (S271,
* S275); of two booleans, = and != by their
* truth; of two matrices, = and != element by element (S242); of anything else
* a domain error. Logic takes booleans, and is Kleene's: a false operand makes
* and false, and a true one makes or true, whatever the other operands are,
* errors among them; otherwise an error passes, the first, and what is no
* boolean is a domain error. all_of and any_of are and and or of any number of
* operands. In a type the booleans are 1 and 0: a relation is C's comparison,
* a NaN holding of none but !=; logic reads 0 as false, a NaN as unknown, as
* an error is as a value, and any other number as true.
*   PIECEWISE is a function of the vocabulary's branches: the text
* piecewise(when(c1, v1), when(c2, v2), otherwise(d)) is the term
* piecewise(c1, v1, c2, v2, d), as parse.hpp reads it and render.hpp writes
* it back. Its value is that of the first branch whose condition holds, the
* conditions taken in turn; the otherwise, an odd last argument, always
* holds. A condition that is an error, or no boolean, as a value, or a NaN
* in a type, gives its error, a domain error or a NaN, there; where no
* branch holds, piecewise is a domain error as a value and a NaN in a type,
* its arguments outside its domain.
*   STEPS. Step 15 (2c) filled the standard tables; step 16 (2d) gave them
* piecewise, all_of and any_of, and true and false, and this header the
* relations' and logic's arithmetic; step 20 (3b) gave ^ and the relations
* matrices; step 21 (3c) the functions of linear algebra; step 22 (3d) the
* names im, jm and km, answering not_yet; step 23 (3e), its first part, im's
* value, complex, real, imag, conj and arg, and complex numbers' powers and
* relations, its second, in chunk 41, each function's principal complex value
* where cplx.h has its complex form, log1p, expm1, asinh, acosh, atanh, sign,
* min and max, and the complex types' evaluators, and in chunk 42 exp2, log2,
* log10, log1p and expm1 with cplx.h's own (S335, S342); its third, in chunk
* 43, matrices of complex numbers: transpose, the parts of a matrix and
* complex(A, B), dot conjugating and linear algebra's functions of complex
* numbers in each type (S350, S351); step 24 (3f), its second part, in chunk
* 45, jm's and km's values, quat, parts and rotate, and the functions,
* powers and relations of quaternions (S275), its third, in chunk 46,
* matrices of them, linear algebra's functions of them, rotmat, rotquat and
* slerp (S386 to S391); step 25 (4a) brings the rules of the derivatives. A
* function that binds a variable of its own, as sum(f, k, a, b) does, is
* step 27's (4c) to shape.
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/function.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.11.13
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  ENTRIES
    -------
    1.  Functions
         1.  expr_variadic
         2.  expr_evaluator
         3.  expr_partial
         4.  expr_function
    2.  Named constants
         1.  expr_named_constant
2.  TABLES
    ------
    1.  The table
         1.  expr_table
         2.  expr_function_table, expr_constant_table
3.  THE ARITHMETIC
    --------------
    1.  The operations
         1.  expr_fn
    2.  In a floating-point type
         1.  A value in a type
         2.  The power, the remainder and the factorial
         3.  The parts
         4.  expr_fn_in
         5.  In a complex type
    3.  As a value
         1.  Exact powers and roots
         2.  Exact integers and remainders
         3.  Exact functions
         4.  In the tower's kind
         5.  expr_value_pow, expr_value_mod, expr_value_fn
         6.  expr_holds_complex, expr_real_only
4.  THE RELATIONS AND LOGIC
    -----------------------
    1.  In a floating-point type
         1.  expr_truth_in
         2.  expr_relation_in
         3.  expr_all_in, expr_any_in, expr_not_in, expr_all_of_in,
             expr_any_of_in
         4.  expr_piecewise_in
    2.  As a value
         1.  expr_relation_value
         2.  expr_all_value, expr_any_value, expr_all_of_value,
             expr_any_of_value, expr_not_value
         3.  expr_piecewise_value
5.  LINEAR ALGEBRA
    --------------
    1.  In a floating-point type
         1.  expr_itself_in, expr_reciprocal_in, expr_magnitude_in,
             expr_product_in, expr_quotient_in, expr_no_scalar_in
    2.  As a value
         1.  expr_transpose_fn_value, expr_det_value, expr_inv_value,
             expr_trace_value, expr_norm_value, expr_eig_value,
             expr_dot_value, expr_cross_value, expr_solve_value
    3.  The quaternions' own
         1.  expr_quat_in, expr_quat_complex, expr_quat_value
         2.  expr_parts_value, expr_rotate_value
         3.  expr_rotmat_value, expr_rotquat_value, expr_slerp_value
6.  THE STANDARD TABLES
    -------------------
    1.  The functions
         1.  expr_standard_functions
    2.  The constants
         1.  expr_standard_constants
*/

#ifndef DJINTERP_MATH_EXPR_FUNCTION_HPP
#define DJINTERP_MATH_EXPR_FUNCTION_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cmath>    // std::fabs, std::isinf, std::signbit
#include <cstddef>  // std::size_t
#include <limits>   // std::numeric_limits
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../../../../c/modules/math/elementary.h"       // d_math_sqrt, _sin, _cos, ...
#include "../../../../c/modules/math/exponential.h"      // d_math_exp, _exp2, _log, ...
#include "../../../../c/modules/math/hyperbolic.h"       // d_math_sinh, _cosh, _tanh
#include "../../../../c/modules/math/nearest_integer.h"  // d_math_floor, _ceil, ...
#include "../../../../c/modules/math/power.h"            // d_math_pow, _cbrt, _hypot
#include "../../../../c/modules/math/remainder.h"        // d_math_fmod, _remainder
#include "../../../../c/modules/math/tangent.h"          // d_math_tan, _atan, _atan2
#include "../math_common.hpp"                            // NS_MATH
#include "../value.hpp"                                  // value
#include "./complex.hpp"                                 // expr_complex, expr_real, ...
#include "./expr_common.hpp"                             // expr_result
#include "./matrix.hpp"                                  // expr_is_matrix, the powers
#include "./quaternion.hpp"                              // expr_quaternion, ...
#include "./rotation.hpp"                                // parts, rotate, rotmat, ...
#include "./term.hpp"                                    // expr_term


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  ENTRIES
//==============================================================================


// 1.1    Functions
//------------------------------------------------------------------------------
// 1.1.1
// expr_variadic
//   constant: the greatest number of arguments of a function that takes any
// number from its least.
D_CONSTEXPR_VAR std::size_t expr_variadic = static_cast<std::size_t>(-1);

// 1.1.2
// expr_evaluator
//   type: a function's evaluator in one type: its value at _count arguments
// of that type, the first at _arguments, under the fold's _context, whose
// real_results it reads where its function's value can be complex alone
// (S340). The C core's kernels are reached through functions of the table's
// own, never by pointer to a kernel, which is forced inline outside testing
// builds (S165).
template<typename Type>
using expr_evaluator = Type (*)(const Type*         _arguments,
                                std::size_t         _count,
                                const expr_context& _context);

// 1.1.3
// expr_partial
//   type: the rule of a function's partial derivatives: the derivative of
// f(a1, ..., an) by its argument _index, as a term in the argument terms
// _arguments; or an error where f has none there.
typedef expr_result<expr_term> (*expr_partial)(
                                   const std::vector<expr_term>& _arguments,
                                   std::size_t                   _index);

// 1.1.4
// expr_function
//   struct: a function of the table: its name; the least and the greatest
// number of arguments it takes (expr_variadic for no greatest); its
// evaluators in float, double, long double, the math::complex of each, and
// math::value, each given the fold's context (S340); and the rule of its
// partial derivatives. A slot that is null
// is one the function has not; a complex type's that is null takes the
// real type's evaluator where every argument is real, and is a NaN
// otherwise, as a function of the real line is there (S339). Each standard
// function has its seven evaluators, which the folds call, and its rule
// from step 25 (4a).
struct expr_function
{
    std::string                                 name;
    std::size_t                                 min_arity;
    std::size_t                                 max_arity;
    expr_evaluator<float>                       evaluate_float;
    expr_evaluator<double>                      evaluate_double;
    expr_evaluator<long double>                 evaluate_long_double;
    expr_evaluator<math::complex<float> >       evaluate_complex_float;
    expr_evaluator<math::complex<double> >      evaluate_complex_double;
    expr_evaluator<math::complex<long double> > evaluate_complex_long_double;
    expr_evaluator<value>                       evaluate_value;
    expr_partial                                partial;

    // no name, no argument, every slot null.
    expr_function()
        : name(),
          min_arity(0),
          max_arity(0),
          evaluate_float(D_NULLPTR),
          evaluate_double(D_NULLPTR),
          evaluate_long_double(D_NULLPTR),
          evaluate_complex_float(D_NULLPTR),
          evaluate_complex_double(D_NULLPTR),
          evaluate_complex_long_double(D_NULLPTR),
          evaluate_value(D_NULLPTR),
          partial(D_NULLPTR)
    {}

    // the name and the arity, every slot null.
    expr_function(
        const std::string& _name,
        std::size_t        _min_arity,
        std::size_t        _max_arity
    )
        : name(_name),
          min_arity(_min_arity),
          max_arity(_max_arity),
          evaluate_float(D_NULLPTR),
          evaluate_double(D_NULLPTR),
          evaluate_long_double(D_NULLPTR),
          evaluate_complex_float(D_NULLPTR),
          evaluate_complex_double(D_NULLPTR),
          evaluate_complex_long_double(D_NULLPTR),
          evaluate_value(D_NULLPTR),
          partial(D_NULLPTR)
    {}

    // takes
    //   whether the function takes _count arguments.
    D_NODISCARD bool
    takes(
        std::size_t _count
    ) const D_NOEXCEPT
    {
        return ( (_count >= min_arity) &&
                 (_count <= max_arity) );
    }
};

// 1.2    Named constants
//------------------------------------------------------------------------------
// 1.2.1
// expr_named_constant
//   struct: a constant of the table: its name, and its value in float,
// double, long double, the math::complex of each, and math::value; or, where
// not_yet is not empty, the message of the expr_status::not_yet a fold
// answers it with, its step not come, its values then unread. A real
// constant's complex values are its real ones and +0 (S339).
struct expr_named_constant
{
    std::string                name;
    float                      as_float;
    double                     as_double;
    long double                as_long_double;
    math::complex<float>       as_complex_float;
    math::complex<double>      as_complex_double;
    math::complex<long double> as_complex_long_double;
    value                      as_value;
    std::string                not_yet;

    // no name; 0 in each type, and none as a value.
    expr_named_constant()
        : name(),
          as_float(0.0f),
          as_double(0.0),
          as_long_double(0.0L),
          as_complex_float(),
          as_complex_double(),
          as_complex_long_double(),
          as_value(),
          not_yet()
    {}

    // a real constant: its value in each real type, the same and +0 in
    // each complex type, and its value.
    expr_named_constant(
        const std::string& _name,
        float              _as_float,
        double             _as_double,
        long double        _as_long_double,
        const value&       _as_value
    )
        : name(_name),
          as_float(_as_float),
          as_double(_as_double),
          as_long_double(_as_long_double),
          as_complex_float(_as_float),
          as_complex_double(_as_double),
          as_complex_long_double(_as_long_double),
          as_value(_as_value),
          not_yet()
    {}

    // a constant whose step has not come: _name, answering not_yet with
    // _not_yet; 0 in each type, and none as a value, never read.
    expr_named_constant(
        const std::string& _name,
        const std::string& _not_yet
    )
        : name(_name),
          as_float(0.0f),
          as_double(0.0),
          as_long_double(0.0L),
          as_complex_float(),
          as_complex_double(),
          as_complex_long_double(),
          as_value(),
          not_yet(_not_yet)
    {}
};


//==============================================================================
// 2.  TABLES
//==============================================================================


// 2.1    The table
//------------------------------------------------------------------------------
// 2.1.1
// expr_table
//   class: entries kept by name, each a struct with a member name: at most
// one entry for a name, in the order they were first defined. A lookup
// walks the entries, as parse/expression's signature does.
template<typename Entry>
class expr_table
{
public:
    // a table of no entry.
    expr_table()
        : m_entries()
    {}

    // define
    //   adds _entry, or puts it in place of the entry of its name. Returns
    // the table, for the next definition.
    expr_table&
    define(
        const Entry& _entry
    )
    {
        // an entry of that name already
        for (std::size_t i = 0; i < m_entries.size(); ++i)
        {
            if (m_entries[i].name == _entry.name)
            {
                m_entries[i] = _entry;

                return *this;
            }
        }

        m_entries.push_back(_entry);

        return *this;
    }

    // find
    //   the entry of the name, or null where the table has none; the
    // pointer is good until the table next changes.
    D_NODISCARD const Entry*
    find(
        const std::string& _name
    ) const
    {
        // the entry of that name
        for (std::size_t i = 0; i < m_entries.size(); ++i)
        {
            if (m_entries[i].name == _name)
            {
                return &m_entries[i];
            }
        }

        return D_NULLPTR;
    }

    D_NODISCARD std::size_t
    size() const D_NOEXCEPT
    {
        return m_entries.size();
    }

    D_NODISCARD bool
    empty() const D_NOEXCEPT
    {
        return m_entries.empty();
    }

private:
    std::vector<Entry> m_entries;
};

// 2.1.2
// expr_function_table, expr_constant_table
//   type: the two tables a fold looks names up in.
typedef expr_table<expr_function>       expr_function_table;
typedef expr_table<expr_named_constant> expr_constant_table;


//==============================================================================
// 3.  THE ARITHMETIC
//==============================================================================
// What the standard functions and the operators ^ and mod compute, beyond
// value.h's + - * / and |x|: in a floating-point type, and as a value.


// 3.1    The operations
//------------------------------------------------------------------------------
NS_INTERNAL

    // 3.1.1
    // expr_fn
    //   enum: each operation of this section: the standard functions, the
    // complex numbers' parts and complex(x, y) among them (S271), the five
    // the C core gave in chunk 37 and min, max and sign (S203), and the
    // floored remainder of the operator mod, which no function is.
    enum class expr_fn
    {
        sqrt      = 1,
        cbrt      = 2,
        sin       = 3,
        cos       = 4,
        tan       = 5,
        asin      = 6,
        acos      = 7,
        atan      = 8,
        sinh      = 9,
        cosh      = 10,
        tanh      = 11,
        exp       = 12,
        exp2      = 13,
        log       = 14,
        log2      = 15,
        log10     = 16,
        floor     = 17,
        ceil      = 18,
        round     = 19,
        trunc     = 20,
        abs       = 21,
        factorial = 22,
        atan2     = 23,
        hypot     = 24,
        pow       = 25,
        fmod      = 26,
        remainder = 27,
        mod       = 28,
        real      = 29,
        imag      = 30,
        conj      = 31,
        arg       = 32,
        complex   = 33,
        log1p     = 34,
        expm1     = 35,
        asinh     = 36,
        acosh     = 37,
        atanh     = 38,
        sign      = 39,
        min       = 40,
        max       = 41
    };

NS_END  // internal

// 3.2    In a floating-point type
//------------------------------------------------------------------------------
NS_INTERNAL

    // 3.2.1
    // A value in a type
    //   functions: a NaN of Type, real or complex, the complex one's parts
    // both NaNs; whether a number of Type is a NaN, a complex one where
    // either part is; and a value's nearest Type, a boolean's 1 or 0, as
    // true's and false's, a NaN for an error or none, which have neither.
    template<typename Type>
    struct expr_nan_of
    {
        static Type
        make() D_NOEXCEPT
        {
            return std::numeric_limits<Type>::quiet_NaN();
        }
    };

    template<typename Type>
    struct expr_nan_of<math::complex<Type> >
    {
        static math::complex<Type>
        make() D_NOEXCEPT
        {
            return math::complex<Type>(std::numeric_limits<Type>::quiet_NaN(),
                                       std::numeric_limits<Type>::quiet_NaN());
        }
    };

    template<typename Type>
    bool
    expr_is_nan_of(
        Type _x
    )
    {
        return std::isnan(_x);
    }

    template<typename Type>
    bool
    expr_is_nan_of(
        const math::complex<Type>& _z
    )
    {
        return ( (std::isnan(_z.real())) ||
                 (std::isnan(_z.imag())) );
    }

    template<typename Type>
    Type
    expr_value_in(
        const value& _value
    )
    {
        Type nearest = Type(0);
        bool truth   = false;

        // a number: its nearest
        if (_value.to(nearest))
        {
            return nearest;
        }

        // a boolean: 1 or 0
        if (_value.to(truth))
        {
            return ( truth ? Type(1) : Type(0) );
        }

        return std::numeric_limits<Type>::quiet_NaN();
    }

    // 3.2.2
    // The power, the remainder and the factorial
    //   the C core's pow in Type, the library's; the floored remainder, fmod
    // plus the divisor where their signs differ, 0 with its sign; and n!,
    // as the banner says.
    template<typename Type>
    Type
    expr_pow_in(
        Type _base,
        Type _exponent
    )
    {
        return d_math_pow(_base, _exponent);
    }

    template<typename Type>
    Type
    expr_mod_in(
        Type _a,
        Type _b
    )
    {
        const Type rest = d_math_fmod(_a, _b);

        // a remainder of the other sign: one divisor more
        if ( (rest != Type(0)) &&
             ( (rest < Type(0)) != (_b < Type(0)) ) )
        {
            return rest + _b;
        }

        // none: 0, with the divisor's sign
        if (rest == Type(0))
        {
            return ( (_b < Type(0)) ? -Type(0) : Type(0) );
        }

        return rest;
    }

    template<typename Type>
    Type
    expr_factorial_in(
        Type _n
    )
    {
        Type product = Type(1);

        // a whole number, not negative: a NaN fails both
        if ( (!(_n >= Type(0))) ||
             (d_math_trunc(_n) != _n) )
        {
            return std::numeric_limits<Type>::quiet_NaN();
        }

        // each factor in turn, until the product passes the type's range,
        // and at the latest until the type's largest exponent: n! is past
        // 2^n from n = 4, and so past the range by then, where std::isinf
        // does not say so, as under valgrind's x87; each product stored, so
        // that the x87 rounds it to Type
        for (Type factor = Type(2);
             ( (factor <= _n) &&
               (factor <= Type(std::numeric_limits<Type>::max_exponent)) );
             factor += Type(1))
        {
            volatile Type stored = product * factor;

            product = stored;

            if (std::isinf(product))
            {
                break;
            }
        }

        return product;
    }

    // 3.2.3
    // The parts
    //   expr_arg_in, expr_complex_in: a real number's argument in Type,
    // carg(x + 0i), pi where x is negative or -0, a NaN for a NaN and +0
    // otherwise, pi the constant table's in Type; and complex(x, y), which
    // in Type is x where y is 0, of either sign, and otherwise a NaN, as a
    // number that is no real one is there (S332).
    template<typename Type>
    Type
    expr_arg_in(
        Type _x
    )
    {
        // a NaN: no angle
        if (_x != _x)
        {
            return _x;
        }

        return ( (std::signbit(_x))
                     ? static_cast<Type>(
                           3.141592653589793238462643383279502884197L)
                     : Type(0) );
    }

    template<typename Type>
    Type
    expr_complex_in(
        Type _re,
        Type _im
    )
    {
        return ( (_im == Type(0)) ? _re
                                  : std::numeric_limits<Type>::quiet_NaN() );
    }

    // expr_sign_in, expr_extreme_in, expr_min_in, expr_max_in
    //   functions: sign(x) in Type, 1 or -1 by x's sign, x itself for a 0 of
    // either sign and for a NaN, as Julia's sign is; and the least or the
    // greatest of _count numbers, the first at _x, as IEEE 754-2019's
    // minimum and maximum: a NaN, the first, where any is one, and -0 below
    // +0 (S338).
    template<typename Type>
    Type
    expr_sign_in(
        Type _x
    )
    {
        // a 0, of either sign, or a NaN: itself
        if ( (_x == Type(0)) ||
             (std::isnan(_x)) )
        {
            return _x;
        }

        return ( (_x < Type(0)) ? Type(-1) : Type(1) );
    }

    template<typename Type>
    Type
    expr_extreme_in(
        const Type* _x,
        std::size_t _count,
        bool        _greatest
    )
    {
        Type best = _x[0];

        // a NaN: the first
        for (std::size_t i = 0; i < _count; ++i)
        {
            if (std::isnan(_x[i]))
            {
                return _x[i];
            }
        }

        // each number past the best, or a 0 of the sign the extreme takes
        for (std::size_t i = 1; i < _count; ++i)
        {
            const bool past  = ( _greatest ? (_x[i] > best) : (_x[i] < best) );
            const bool other = ( (_x[i] == best) &&
                                 (std::signbit(_x[i]) != std::signbit(best)) &&
                                 (std::signbit(_x[i]) != _greatest) );

            if ( (past) ||
                 (other) )
            {
                best = _x[i];
            }
        }

        return best;
    }

    template<typename Type>
    Type
    expr_min_in(
        const Type*         _x,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_context);

        return expr_extreme_in(_x, _count, false);
    }

    template<typename Type>
    Type
    expr_max_in(
        const Type*         _x,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_context);

        return expr_extreme_in(_x, _count, true);
    }

    // 3.2.4
    // expr_fn_in
    //   function: an operation in Type, of the arguments it takes, the first
    // at _x: a real number's parts its own and 0, its conjugate itself. min
    // and max, of any number of arguments, are expr_min_in's and
    // expr_max_in's.
    template<typename Type>
    Type
    expr_fn_in(
        expr_fn     _fn,
        const Type* _x
    )
    {
        // by operation
        switch (_fn)
        {
            case expr_fn::sqrt:      return d_math_sqrt(_x[0]);
            case expr_fn::cbrt:      return d_math_cbrt(_x[0]);
            case expr_fn::sin:       return d_math_sin(_x[0]);
            case expr_fn::cos:       return d_math_cos(_x[0]);
            case expr_fn::tan:       return d_math_tan(_x[0]);
            case expr_fn::asin:      return d_math_asin(_x[0]);
            case expr_fn::acos:      return d_math_acos(_x[0]);
            case expr_fn::atan:      return d_math_atan(_x[0]);
            case expr_fn::sinh:      return d_math_sinh(_x[0]);
            case expr_fn::cosh:      return d_math_cosh(_x[0]);
            case expr_fn::tanh:      return d_math_tanh(_x[0]);
            case expr_fn::exp:       return d_math_exp(_x[0]);
            case expr_fn::exp2:      return d_math_exp2(_x[0]);
            case expr_fn::log:       return d_math_log(_x[0]);
            case expr_fn::log2:      return d_math_log2(_x[0]);
            case expr_fn::log10:     return d_math_log10(_x[0]);
            case expr_fn::floor:     return d_math_floor(_x[0]);
            case expr_fn::ceil:      return d_math_ceil(_x[0]);
            case expr_fn::round:     return d_math_round(_x[0]);
            case expr_fn::trunc:     return d_math_trunc(_x[0]);
            case expr_fn::abs:       return std::fabs(_x[0]);
            case expr_fn::factorial: return expr_factorial_in(_x[0]);
            case expr_fn::atan2:     return d_math_atan2(_x[0], _x[1]);
            case expr_fn::hypot:     return d_math_hypot(_x[0], _x[1]);
            case expr_fn::pow:       return expr_pow_in(_x[0], _x[1]);
            case expr_fn::fmod:      return d_math_fmod(_x[0], _x[1]);
            case expr_fn::remainder: return d_math_remainder(_x[0], _x[1]);
            case expr_fn::real:      return _x[0];
            case expr_fn::imag:      return Type(0);
            case expr_fn::conj:      return _x[0];
            case expr_fn::arg:       return expr_arg_in(_x[0]);
            case expr_fn::complex:   return expr_complex_in(_x[0], _x[1]);
            case expr_fn::log1p:     return d_math_log1p(_x[0]);
            case expr_fn::expm1:     return d_math_expm1(_x[0]);
            case expr_fn::asinh:     return d_math_asinh(_x[0]);
            case expr_fn::acosh:     return d_math_acosh(_x[0]);
            case expr_fn::atanh:     return d_math_atanh(_x[0]);
            case expr_fn::sign:      return expr_sign_in(_x[0]);
            case expr_fn::mod:
            default:
                break;
        }

        return expr_mod_in(_x[0], _x[1]);
    }

    // 3.2.5
    // In a complex type
    //   functions: an operation in math::complex<Type>, of the arguments it
    // takes, the first at _z (S339). A function with a principal complex
    // value is math::complex<Type>'s: by its own name in a complex type, so
    // cplx.h's plain form at run time, and by its _c name, cplx.h's _c form,
    // as a value (S336); a function of the real line's order its real form
    // of the real parts, plus +0i, where every argument is real, its
    // imaginary part 0 of either sign, and a NaN in both parts otherwise;
    // the parts, the modulus and the argument as the face's, in the same
    // form, complex(x, y) of two real numbers x + yi, and sign(z) z / |z|, a
    // 0 itself.

    // expr_is_real_of
    //   function: whether a complex number is real, its imaginary part 0 of
    // either sign.
    template<typename Type>
    bool
    expr_is_real_of(
        const math::complex<Type>& _z
    )
    {
        return (_z.imag() == Type(0));
    }

    // expr_real_line_complex
    //   function: _fn of the real line in math::complex<Type>: its real form
    // of the real parts, plus +0i, where each of the _count arguments is
    // real; a NaN otherwise.
    template<typename Type>
    math::complex<Type>
    expr_real_line_complex(
        expr_fn                    _fn,
        const math::complex<Type>* _z,
        std::size_t                _count
    )
    {
        Type x[2] = { Type(0), Type(0) };

        // each argument real, its real part taken
        for (std::size_t i = 0; i < _count; ++i)
        {
            if (!expr_is_real_of(_z[i]))
            {
                return expr_nan_of<math::complex<Type> >::make();
            }

            x[i] = _z[i].real();
        }

        return math::complex<Type>(expr_fn_in<Type>(_fn, x));
    }

    // expr_fn_complex
    //   function: an operation in math::complex<Type>, as 3.2.5 says: in
    // cplx.h's plain form, as a complex type takes it, or where _exact in
    // its _c form, as a value takes it (S336).
    template<typename Type>
    math::complex<Type>
    expr_fn_complex(
        expr_fn                    _fn,
        const math::complex<Type>* _z,
        bool                       _exact = false
    )
    {
        typedef math::complex<Type> number;

        // by operation
        switch (_fn)
        {
            case expr_fn::sqrt:  return ( _exact ? sqrt_c(_z[0])
                                                 : sqrt(_z[0]) );
            case expr_fn::sin:   return ( _exact ? sin_c(_z[0])
                                                 : sin(_z[0]) );
            case expr_fn::cos:   return ( _exact ? cos_c(_z[0])
                                                 : cos(_z[0]) );
            case expr_fn::tan:   return ( _exact ? tan_c(_z[0])
                                                 : tan(_z[0]) );
            case expr_fn::asin:  return ( _exact ? asin_c(_z[0])
                                                 : asin(_z[0]) );
            case expr_fn::acos:  return ( _exact ? acos_c(_z[0])
                                                 : acos(_z[0]) );
            case expr_fn::atan:  return ( _exact ? atan_c(_z[0])
                                                 : atan(_z[0]) );
            case expr_fn::sinh:  return ( _exact ? sinh_c(_z[0])
                                                 : sinh(_z[0]) );
            case expr_fn::cosh:  return ( _exact ? cosh_c(_z[0])
                                                 : cosh(_z[0]) );
            case expr_fn::tanh:  return ( _exact ? tanh_c(_z[0])
                                                 : tanh(_z[0]) );
            case expr_fn::asinh: return ( _exact ? asinh_c(_z[0])
                                                 : asinh(_z[0]) );
            case expr_fn::acosh: return ( _exact ? acosh_c(_z[0])
                                                 : acosh(_z[0]) );
            case expr_fn::atanh: return ( _exact ? atanh_c(_z[0])
                                                 : atanh(_z[0]) );
            case expr_fn::exp:   return ( _exact ? exp_c(_z[0])
                                                 : exp(_z[0]) );
            case expr_fn::log:   return ( _exact ? log_c(_z[0])
                                                 : log(_z[0]) );
            case expr_fn::exp2:  return ( _exact ? exp2_c(_z[0])
                                                 : exp2(_z[0]) );
            case expr_fn::log2:  return ( _exact ? log2_c(_z[0])
                                                 : log2(_z[0]) );
            case expr_fn::log10: return ( _exact ? log10_c(_z[0])
                                                 : log10(_z[0]) );
            case expr_fn::log1p: return ( _exact ? log1p_c(_z[0])
                                                 : log1p(_z[0]) );
            case expr_fn::expm1: return ( _exact ? expm1_c(_z[0])
                                                 : expm1(_z[0]) );
            case expr_fn::pow:   return ( _exact ? pow_c(_z[0], _z[1])
                                                 : pow(_z[0], _z[1]) );
            case expr_fn::abs:   return number( _exact ? abs_c(_z[0])
                                                       : abs(_z[0]) );
            case expr_fn::real:  return number(_z[0].real());
            case expr_fn::imag:  return number(_z[0].imag());
            case expr_fn::conj:  return conj(_z[0]);
            case expr_fn::arg:   return number( _exact ? arg_c(_z[0])
                                                       : arg(_z[0]) );
            case expr_fn::complex:
                // two real parts
                if ( (!expr_is_real_of(_z[0])) ||
                     (!expr_is_real_of(_z[1])) )
                {
                    return expr_nan_of<number>::make();
                }

                return number(_z[0].real(), _z[1].real());
            case expr_fn::sign:
                // a 0 itself, and otherwise z / |z|
                if (_z[0] == number(Type(0)))
                {
                    return _z[0];
                }

                return ( _z[0] / ( _exact ? abs_c(_z[0])
                                          : abs(_z[0]) ) );
            case expr_fn::atan2:
            case expr_fn::hypot:
            case expr_fn::fmod:
            case expr_fn::remainder:
            case expr_fn::mod:
                return expr_real_line_complex(_fn, _z, 2);
            default:
                break;
        }

        return expr_real_line_complex(_fn, _z, 1);
    }

    // expr_extreme_complex, expr_min_complex, expr_max_complex
    //   functions: min and max in math::complex<Type>: of real numbers the
    // real one's, plus +0i; a NaN where any is not real.
    template<typename Type>
    math::complex<Type>
    expr_extreme_complex(
        const math::complex<Type>* _z,
        std::size_t                _count,
        bool                       _greatest
    )
    {
        std::vector<Type> x;

        // each real, its real part taken
        for (std::size_t i = 0; i < _count; ++i)
        {
            if (!expr_is_real_of(_z[i]))
            {
                return expr_nan_of<math::complex<Type> >::make();
            }

            x.push_back(_z[i].real());
        }

        return math::complex<Type>(expr_extreme_in(x.data(),
                                                   _count,
                                                   _greatest));
    }

    template<typename Type>
    math::complex<Type>
    expr_min_complex(
        const math::complex<Type>* _z,
        std::size_t                _count,
        const expr_context&        _context
    )
    {
        static_cast<void>(_context);

        return expr_extreme_complex(_z, _count, false);
    }

    template<typename Type>
    math::complex<Type>
    expr_max_complex(
        const math::complex<Type>* _z,
        std::size_t                _count,
        const expr_context&        _context
    )
    {
        static_cast<void>(_context);

        return expr_extreme_complex(_z, _count, true);
    }

    // expr_complex_function
    //   struct: the operation fn as expr_complex_apply takes it (complex.hpp),
    // expr_fn_complex's in each type in the _c forms, as a value takes them:
    // one answer at every level, where the fold in math::complex<T> takes
    // the plain forms (S336).
    struct expr_complex_function
    {
        expr_fn fn;

        template<typename Type>
        math::complex<Type>
        operator()(
            const math::complex<Type>* _z
        ) const
        {
            return expr_fn_complex<Type>(fn,
                                         _z,
                                         true);
        }
    };

NS_END  // internal

// 3.3    As a value
//------------------------------------------------------------------------------
NS_INTERNAL

    // 3.3.1
    // Exact powers and roots
    //   a whole power by repeated squaring, exact as value.h's products and
    // quotients are; a whole root, for a rational exponent and the roots;
    // and whether a whole number is a power of another.

    // expr_power_capped
    //   function: whether _root ^ _k is at most _n, for _root >= 1, and if so
    // the power, in _power: each product checked before it is made.
    inline bool
    expr_power_capped(
        dj_math_imax  _root,
        dj_math_imax  _k,
        dj_math_imax  _n,
        dj_math_imax& _power
    )
    {
        dj_math_imax product = 1;

        // _k factors, none past _n
        for (dj_math_imax i = 0; i < _k; ++i)
        {
            if (product > _n / _root)
            {
                return false;
            }

            product *= _root;
        }

        _power = product;

        return true;
    }

    // expr_whole_root
    //   function: whether _n >= 0 is the _k-th power of a whole number, for
    // _k >= 1, and if so that number, in _root: the greatest whose power is
    // at most _n, by bisection, then its power compared.
    inline bool
    expr_whole_root(
        dj_math_imax  _n,
        dj_math_imax  _k,
        dj_math_imax& _root
    )
    {
        // 0 and 1 are their own roots
        if (_n < 2)
        {
            _root = _n;

            return true;
        }

        // past the word's width, no whole number's power is _n
        if (_k >= static_cast<dj_math_imax>(
                      std::numeric_limits<dj_math_imax>::digits))
        {
            return false;
        }

        dj_math_imax low   = 1;
        dj_math_imax high  = _n;
        dj_math_imax power = 0;

        // the greatest root whose power is at most _n
        while (low < high)
        {
            const dj_math_imax middle = low + (high - low + 1) / 2;

            if (expr_power_capped(middle, _k, _n, power))
            {
                low = middle;
            }
            else
            {
                high = middle - 1;
            }
        }

        _root = low;

        return ( (expr_power_capped(low, _k, _n, power)) &&
                 (power == _n) );
    }

    // expr_ratio_root
    //   function: whether the rational _q, its terms whole _k-th powers, has
    // an exact _k-th root, and if so the root, in _out: of a negative _q
    // only for an odd _k, and the negative of its magnitude's.
    inline bool
    expr_ratio_root(
        const dj_rational& _q,
        dj_math_imax       _k,
        value&             _out
    )
    {
        dj_math_imax num_root = 0;
        dj_math_imax den_root = 1;

        // a negative number: an odd root, of a magnitude a word holds
        if (_q.num < 0)
        {
            if ( ((_k % 2) == 0) ||
                 (_q.num == std::numeric_limits<dj_math_imax>::min()) ||
                 (!expr_whole_root(-_q.num, _k, num_root)) ||
                 (!expr_whole_root(_q.den, _k, den_root)) )
            {
                return false;
            }

            _out = value::ratio(-num_root, den_root);

            return true;
        }

        // terms that are whole _k-th powers
        if ( (expr_whole_root(_q.num, _k, num_root)) &&
             (expr_whole_root(_q.den, _k, den_root)) )
        {
            _out = value::ratio(num_root, den_root);

            return true;
        }

        return false;
    }

    // expr_whole_power
    //   function: _base ^ _n for an exact base and a whole _n: by repeated
    // squaring, exact; 1 for _n = 0, and the reciprocal for _n < 0, a pole
    // for a base of 0.
    inline value
    expr_whole_power(
        const value& _base,
        dj_math_imax _n
    )
    {
        dj_math_umax left = ( (_n < 0)
                                  ? (dj_math_umax(0) - dj_math_umax(_n))
                                  : dj_math_umax(_n) );
        value        power(1);
        value        square = _base;

        // a factor for each bit of the exponent
        while (left != 0)
        {
            if ((left & 1u) != 0)
            {
                power = power * square;
            }

            left >>= 1;

            if (left != 0)
            {
                square = square * square;
            }
        }

        // a negative exponent: the reciprocal
        if (_n < 0)
        {
            return (value(1) / power);
        }

        return power;
    }

    // expr_ratio_power
    //   function: whether _base ^ (p / q), for an exact base not negative and
    // an exact rational exponent, is exact, where the base's terms are whole
    // q-th powers; and if so its value, in _out. Where it is not, its value
    // is the tower's kind's. A negative base's power is complex (S272),
    // expr_value_pow's.
    inline bool
    expr_ratio_power(
        const value&       _base,
        const dj_rational& _exponent,
        value&             _out
    )
    {
        dj_rational  terms    = { 0, 1 };
        dj_math_imax num_root = 0;
        dj_math_imax den_root = 1;

        // terms that are whole q-th powers
        if ( (_base.to(terms)) &&
             (expr_whole_root(terms.num, _exponent.den, num_root)) &&
             (expr_whole_root(terms.den, _exponent.den, den_root)) )
        {
            _out = expr_whole_power(value::ratio(num_root, den_root),
                                    _exponent.num);

            return true;
        }

        return false;
    }

    // expr_gcd
    //   function: the greatest common divisor of two positive whole numbers,
    // Euclid's.
    inline dj_math_imax
    expr_gcd(
        dj_math_imax _a,
        dj_math_imax _b
    )
    {
        // each remainder in turn
        while (_b != 0)
        {
            const dj_math_imax rest = _a % _b;

            _a = _b;
            _b = rest;
        }

        return _a;
    }

    // expr_gaussian_root
    //   function: whether the exact number _z, real or complex, its parts
    // rationals of the family, has a principal _q-th root that is a Gaussian
    // rational, for 2 <= _q below the word's width, and if so that root, in
    // _out (S337). With D the least common denominator of z's parts, D times
    // a root w of z is a Gaussian integer: w = g / e in lowest terms, e^q
    // divides D's power and e divides D, so D w = (D / e) g. So the
    // principal root, exp(log z / q), is found in long double, D times it is
    // rounded part by part to whole numbers G, and w = G / D is taken where
    // its q-th power is z exactly, every step of it exact, and it lies
    // within 2^-10 of its modulus of the long double root, as each other
    // root lies 2 sin(pi / q), 2^-4 or more, of its modulus away. Where D w
    // passes the family, or the long double cannot round it, there is none.
    inline bool
    expr_gaussian_root(
        const value& _z,
        dj_math_imax _q,
        value&       _out
    )
    {
        dj_rational re  = { 0, 1 };
        dj_rational im  = { 0, 1 };
        long double zr  = 0.0L;
        long double zi  = 0.0L;

        // exact parts of the family, and a root of the word's width
        if ( (!_z.is_exact()) ||
             (!expr_real(_z).to(re)) ||
             (!expr_imag(_z).to(im)) ||
             (_q < 2) ||
             (_q >= static_cast<dj_math_imax>(
                        std::numeric_limits<dj_math_imax>::digits)) )
        {
            return false;
        }

        const value den = value(re.den / expr_gcd(re.den, im.den)) *
                          value(im.den);
        long double d   = 0.0L;

        static_cast<void>(expr_real(_z).to(zr));
        static_cast<void>(expr_imag(_z).to(zi));
        static_cast<void>(den.to(d));

        const math::complex<long double> principal =
            exp_c(log_c(math::complex<long double>(zr, zi)) /
                static_cast<long double>(_q));
        const long double gr    = std::floor((d * principal.real()) + 0.5L);
        const long double gi    = std::floor((d * principal.imag()) + 0.5L);
        const long double limit = 9.0e18L;

        // D w within the family
        if ( (!den.is_exact()) ||
             (!(std::fabs(gr) < limit)) ||
             (!(std::fabs(gi) < limit)) )
        {
            return false;
        }

        const value       root  = expr_complex(
                                      value(static_cast<dj_math_imax>(gr)),
                                      value(static_cast<dj_math_imax>(gi))) /
                                  den;
        const value       power = expr_whole_power(root, _q);
        const long double dr    = (gr / d) - principal.real();
        const long double di    = (gi / d) - principal.imag();
        const long double size  = ((gr * gr) + (gi * gi)) / (d * d);

        // the root's q-th power z, exactly, and the root the principal one
        if ( (!root.is_exact()) ||
             (!power.is_exact()) ||
             (!(power == _z)) ||
             ( ((dr * dr) + (di * di)) * 1048576.0L > size ) )
        {
            return false;
        }

        _out = root;

        return true;
    }

    // expr_power_of
    //   function: whether _n >= 1 is a whole power of _base > 1, and if so
    // the exponent, in _k: _n divided by _base while it divides, then
    // compared with 1, the power 0.
    inline bool
    expr_power_of(
        dj_math_imax  _n,
        dj_math_imax  _base,
        dj_math_imax& _k
    )
    {
        dj_math_imax exponent = 0;

        while ( (_n % _base) == 0 )
        {
            _n /= _base;
            ++exponent;
        }

        _k = exponent;

        return (_n == 1);
    }

    // 3.3.2
    // Exact integers and remainders
    //   a rational rounded to a whole number each way, and the remainder of
    // a quotient rounded so: floored for mod, truncated for fmod, to the
    // nearest, ties to even, for remainder.
    enum class expr_rounding
    {
        floor = 1,   // toward minus infinity
        ceil  = 2,   // toward plus infinity
        trunc = 3,   // toward 0
        away  = 4,   // to the nearest, halves away from 0, as C's round
        even  = 5    // to the nearest, halves to the even, as IEEE's
    };

    // expr_whole_of
    //   function: the rational _q rounded to a whole number by _how; the
    // quotient of terms of the family, so that no step leaves the word.
    inline dj_math_imax
    expr_whole_of(
        const dj_rational& _q,
        expr_rounding      _how
    )
    {
        const dj_math_imax whole = _q.num / _q.den;
        const dj_math_imax rest  = _q.num % _q.den;
        const dj_math_imax step  = ( (_q.num < 0) ? -1 : 1 );
        const dj_math_imax size  = ( (rest < 0) ? -rest : rest );

        // a whole number already
        if (rest == 0)
        {
            return whole;
        }

        // by the rounding: the truncated quotient, or one step from it
        switch (_how)
        {
            case expr_rounding::floor:
                return ( (_q.num < 0) ? (whole - 1) : whole );

            case expr_rounding::ceil:
                return ( (_q.num > 0) ? (whole + 1) : whole );

            case expr_rounding::away:
                return ( (size >= _q.den - size) ? (whole + step) : whole );

            case expr_rounding::even:
                // past the half, or at it from an odd quotient
                if ( (size > _q.den - size) ||
                     ( (size == _q.den - size) &&
                       ((whole % 2) != 0) ) )
                {
                    return whole + step;
                }

                return whole;

            case expr_rounding::trunc:
            default:
                break;
        }

        return whole;
    }

    // expr_exact_rest
    //   function: whether _a less _b times their quotient rounded by _how
    // is exact, and if so its value, in _out: for exact operands, a domain
    // error for b = 0.
    inline bool
    expr_exact_rest(
        const value&  _a,
        const value&  _b,
        expr_rounding _how,
        value&        _out
    )
    {
        dj_rational quotient = { 0, 1 };

        // an exact zero divisor: no remainder
        if (_b == value(0))
        {
            _out = value(D_MATH_VALUE_ERROR_DOMAIN);

            return true;
        }

        // an exact quotient, rounded
        if ((_a / _b).to(quotient))
        {
            _out = _a - (_b * value(expr_whole_of(quotient, _how)));

            return true;
        }

        return false;
    }

    // expr_exact_factorial
    //   function: n! for an exact whole n >= 0: value.h's products, past the
    // family G2's long double, until it passes long double's range.
    inline value
    expr_exact_factorial(
        dj_math_imax _n
    )
    {
        value       product(1);
        long double nearest = 0.0L;

        // each factor in turn, and at the latest until long double's largest
        // exponent, as expr_factorial_in counts
        for (dj_math_imax factor = 2;
             ( (factor <= _n) &&
               (factor <= std::numeric_limits<long double>::max_exponent) );
             ++factor)
        {
            product = product * value(factor);

            // past the family, and past long double's range
            if ( (!product.is_exact()) &&
                 (product.to(nearest)) &&
                 (std::isinf(nearest)) )
            {
                break;
            }
        }

        return product;
    }

    // 3.3.3
    // Exact functions
    //   function: whether _fn of exact arguments has an exact value, or an
    // error, as the banner says; and if so it, in _out. Where it has
    // neither, its value is the tower's kind's.

    // expr_exact_at
    //   function: whether the exact _x is the rational _at, and if so the
    // exact _value, in _out: the one point where a function is rational.
    inline bool
    expr_exact_at(
        const dj_rational& _x,
        dj_math_imax       _at,
        dj_math_imax       _value,
        value&             _out
    )
    {
        // the point itself
        if ( (_x.num == _at) &&
             (_x.den == 1) )
        {
            _out = value(_value);

            return true;
        }

        return false;
    }

    // expr_exact_log
    //   function: the logarithm to _base of the exact _x, where it is exact
    // or an error: a pole at 0, a domain error below it; and for a base of
    // 2 or 10, the exponent of a whole power, 1/_base^k among them.
    inline bool
    expr_exact_log(
        const dj_rational& _x,
        dj_math_imax       _base,
        value&             _out
    )
    {
        dj_math_imax k = 0;

        // 0, and below it
        if (_x.num <= 0)
        {
            _out = value( (_x.num == 0) ? D_MATH_VALUE_ERROR_POLE
                                        : D_MATH_VALUE_ERROR_DOMAIN );

            return true;
        }

        // a power of the base: a whole one, or its reciprocal
        if ( (_x.den == 1) &&
             (expr_power_of(_x.num, _base, k)) )
        {
            _out = value(k);

            return true;
        }

        if ( (_x.num == 1) &&
             (expr_power_of(_x.den, _base, k)) )
        {
            _out = value(-k);

            return true;
        }

        return false;
    }

    // expr_exact_one
    //   function: a function of one exact argument, a rational of the
    // family, by the banner's rules.
    inline bool
    expr_exact_one(
        expr_fn            _fn,
        const dj_rational& _x,
        value&             _out
    )
    {
        const bool past_one = ( (_x.num > _x.den) ||
                                (_x.num < -_x.den) );

        // by function
        switch (_fn)
        {
            case expr_fn::sqrt:
                // no real root of a negative number
                if (_x.num < 0)
                {
                    _out = value(D_MATH_VALUE_ERROR_DOMAIN);

                    return true;
                }

                return expr_ratio_root(_x, 2, _out);

            case expr_fn::cbrt:
                return expr_ratio_root(_x, 3, _out);

            case expr_fn::asin:
            case expr_fn::acos:
                // past 1, no angle
                if (past_one)
                {
                    _out = value(D_MATH_VALUE_ERROR_DOMAIN);

                    return true;
                }

                return ( (_fn == expr_fn::asin) ? expr_exact_at(_x, 0, 0, _out)
                                                : expr_exact_at(_x, 1, 0,
                                                                _out) );

            case expr_fn::sin:
            case expr_fn::tan:
            case expr_fn::atan:
            case expr_fn::sinh:
            case expr_fn::tanh:
                return expr_exact_at(_x, 0, 0, _out);

            case expr_fn::cos:
            case expr_fn::cosh:
            case expr_fn::exp:
                return expr_exact_at(_x, 0, 1, _out);

            case expr_fn::exp2:
                // a whole exponent
                if (_x.den == 1)
                {
                    _out = expr_whole_power(value(2), _x.num);

                    return true;
                }

                return false;

            case expr_fn::log:
                // the pole and the domain, then 1
                if (_x.num <= 0)
                {
                    return expr_exact_log(_x, 2, _out);
                }

                return expr_exact_at(_x, 1, 0, _out);

            case expr_fn::log2:
                return expr_exact_log(_x, 2, _out);

            case expr_fn::log10:
                return expr_exact_log(_x, 10, _out);

            case expr_fn::floor:
                _out = value(expr_whole_of(_x, expr_rounding::floor));

                return true;

            case expr_fn::ceil:
                _out = value(expr_whole_of(_x, expr_rounding::ceil));

                return true;

            case expr_fn::round:
                _out = value(expr_whole_of(_x, expr_rounding::away));

                return true;

            case expr_fn::trunc:
                _out = value(expr_whole_of(_x, expr_rounding::trunc));

                return true;

            case expr_fn::log1p:
                // -1 a pole, and below it no real value: the complex one,
                // which expr_value_fn takes first (S272)
                if (_x.num <= -_x.den)
                {
                    _out = value( (_x.num == -_x.den)
                                      ? D_MATH_VALUE_ERROR_POLE
                                      : D_MATH_VALUE_ERROR_DOMAIN );

                    return true;
                }

                return expr_exact_at(_x, 0, 0, _out);

            case expr_fn::atanh:
                // -1 and 1 poles; past them a complex value, taken before
                if ( (_x.num == _x.den) ||
                     (_x.num == -_x.den) )
                {
                    _out = value(D_MATH_VALUE_ERROR_POLE);

                    return true;
                }

                return expr_exact_at(_x, 0, 0, _out);

            case expr_fn::expm1:
            case expr_fn::asinh:
                return expr_exact_at(_x, 0, 0, _out);

            case expr_fn::acosh:
                return expr_exact_at(_x, 1, 0, _out);

            case expr_fn::sign:
                _out = value( (_x.num < 0) ? -1 : ( (_x.num > 0) ? 1 : 0 ) );

                return true;

            case expr_fn::factorial:
            default:
                break;
        }

        // the factorial: of a whole number, not negative
        if ( (_x.den != 1) ||
             (_x.num < 0) )
        {
            _out = value(D_MATH_VALUE_ERROR_DOMAIN);

            return true;
        }

        _out = expr_exact_factorial(_x.num);

        return true;
    }

    // expr_exact_two
    //   function: a function of two exact arguments, by the banner's rules:
    // atan2 and hypot of rationals of the family; fmod, remainder and mod
    // of any exact numbers.
    inline bool
    expr_exact_two(
        expr_fn      _fn,
        const value& _a,
        const value& _b,
        value&       _out
    )
    {
        dj_rational x   = { 0, 1 };
        dj_rational y   = { 0, 1 };
        dj_rational sum = { 0, 1 };

        // by function
        switch (_fn)
        {
            case expr_fn::atan2:
                // the angle of (b, a): 0 on the positive axis, none at 0
                if ( (!_a.to(y)) ||
                     (!_b.to(x)) ||
                     (y.num != 0) ||
                     (x.num < 0) )
                {
                    return false;
                }

                _out = ( (x.num == 0) ? value(D_MATH_VALUE_ERROR_DOMAIN)
                                      : value(0) );

                return true;

            case expr_fn::hypot:
                // the root of the sum of the squares, where it is exact
                return ( (((_a * _a) + (_b * _b)).to(sum)) &&
                         (expr_ratio_root(sum, 2, _out)) );

            case expr_fn::fmod:
                return expr_exact_rest(_a, _b, expr_rounding::trunc, _out);

            case expr_fn::remainder:
                return expr_exact_rest(_a, _b, expr_rounding::even, _out);

            case expr_fn::mod:
            default:
                break;
        }

        return expr_exact_rest(_a, _b, expr_rounding::floor, _out);
    }

    // 3.3.4
    // In the tower's kind
    //   an operation in the kind of the tower its arguments give: the widest
    // inexact kind of theirs, a long double where all are exact; each
    // argument its nearest there; the answer stored in that kind before it
    // becomes a value, as the x87 may hold more of it than the kind has;
    // and the result carrying every argument's flags.

    // expr_inexact_kind
    //   function: a number's kind, where it is an inline inexact one; a long
    // double for a boxed inexact kind; none for an exact one.
    inline dj_math_value_kind
    expr_inexact_kind(
        const value& _x
    )
    {
        // an inline inexact kind: its own
        if ( (_x.kind() == D_MATH_VALUE_FLOAT)  ||
             (_x.kind() == D_MATH_VALUE_DOUBLE) ||
             (_x.kind() == D_MATH_VALUE_LONG_DOUBLE) )
        {
            return _x.kind();
        }

        return ( (_x.is_exact()) ? D_MATH_VALUE_NONE
                                 : D_MATH_VALUE_LONG_DOUBLE );
    }

    template<typename Type>
    value
    expr_tower_as(
        expr_fn      _fn,
        const value* _x,
        std::size_t  _count
    )
    {
        Type     x[2]  = { Type(0), Type(0) };
        unsigned flags = 0u;

        // each argument its nearest Type, its flags kept
        for (std::size_t i = 0; i < _count; ++i)
        {
            x[i]   = expr_value_in<Type>(_x[i]);
            flags |= _x[i].flags();
        }

        // the answer stored, so that the x87 rounds it to Type: the C
        // library may hand it back with more precision than Type has
        volatile Type answer = expr_fn_in<Type>(_fn, x);
        value         result(static_cast<Type>(answer));
        dj_math_value held = result.detach();

        held.flags |= flags;

        return value::adopt(held);
    }

    inline value
    expr_tower(
        expr_fn      _fn,
        const value* _x,
        std::size_t  _count
    )
    {
        dj_math_value_kind widest = D_MATH_VALUE_NONE;

        // the widest inexact kind of the arguments
        for (std::size_t i = 0; i < _count; ++i)
        {
            const dj_math_value_kind kind = expr_inexact_kind(_x[i]);

            if (kind > widest)
            {
                widest = kind;
            }
        }

        // that kind; a long double for exact arguments alone
        if (widest == D_MATH_VALUE_FLOAT)
        {
            return expr_tower_as<float>(_fn, _x, _count);
        }

        if (widest == D_MATH_VALUE_DOUBLE)
        {
            return expr_tower_as<double>(_fn, _x, _count);
        }

        return expr_tower_as<long double>(_fn, _x, _count);
    }

    // 3.3.5
    // expr_value_pow, expr_value_mod, expr_value_fn
    //   functions: _a ^ _b, _a mod _b, and any operation of this section, as
    // values, as the banner says: an error passes, the first; a boolean or
    // none is a domain error; exact where the operation is; otherwise in
    // the tower's kind. A matrix to a power is the matrix kind's
    // (matrix.hpp); real, imag, conj and arg of a matrix its elements', and
    // complex(A, B) of two the matrix of complex(a_ij, b_ij) (S350); and in
    // any other operation here a matrix is a domain error. A complex
    // number's parts, its conjugate and argument and complex(x, y) are
    // complex.hpp's. A function of a complex argument, or of a real one
    // where it has no real value but a complex one (S272), is its principal
    // complex value, exact where it is a Gaussian rational (S337), and
    // otherwise expr_fn_complex's in the tower's type (S336), exp2, log2,
    // log10, log1p and expm1 among them (S342); of the real line's order, a
    // domain error (S271). min and max are exact by value.h's order, the
    // least or the greatest argument itself (S338).

    // expr_value_failed
    //   function: whether an argument is an error, which passes, the first,
    // or no number, a domain error; and if so that, in _out.
    inline bool
    expr_value_failed(
        const value* _x,
        std::size_t  _count,
        value&       _out
    )
    {
        // an error passes, the first
        for (std::size_t i = 0; i < _count; ++i)
        {
            if (_x[i].kind() == D_MATH_VALUE_ERROR)
            {
                _out = _x[i];

                return true;
            }
        }

        // what is no number has no value here
        for (std::size_t i = 0; i < _count; ++i)
        {
            if (!_x[i].is_number())
            {
                _out = value(D_MATH_VALUE_ERROR_DOMAIN);

                return true;
            }
        }

        return false;
    }

    // expr_value_negative, expr_value_fraction
    //   functions: whether a real number is below 0, by value.h's exact
    // order, a NaN not; and whether one is finite and not whole: an exact
    // rational not an integer, or an inexact number finite and not whole, a
    // NaN and an infinity not, as their powers are real.
    inline bool
    expr_value_negative(
        const value& _x
    )
    {
        return ( d_math_value_cmp(_x.c_value(),
                                  d_math_value_from_int(0)) ==
                     D_MATH_VALUE_LESS );
    }

    inline bool
    expr_value_fraction(
        const value& _x
    )
    {
        dj_rational q       = { 0, 1 };
        long double nearest = 0.0L;

        // exact: a rational of a denominator past 1
        if (_x.is_exact())
        {
            return ( (_x.to(q)) &&
                     (q.den != 1) );
        }

        return ( (_x.to(nearest)) &&
                 (std::isfinite(nearest)) &&
                 (std::floor(nearest) != nearest) );
    }

    // expr_beyond_real
    //   function: whether the real _x is where _fn has no real value but a
    // complex one (S272): sqrt, log, log2 and log10 below 0, log1p below -1,
    // asin, acos and atanh past -1 or 1, acosh below 1; a NaN nowhere.
    inline bool
    expr_beyond_real(
        expr_fn      _fn,
        const value& _x
    )
    {
        const dj_math_value_order below =
            d_math_value_cmp(_x.c_value(), d_math_value_from_int(-1));
        const dj_math_value_order above =
            d_math_value_cmp(_x.c_value(), d_math_value_from_int(1));

        // by function
        switch (_fn)
        {
            case expr_fn::sqrt:
            case expr_fn::log:
            case expr_fn::log2:
            case expr_fn::log10:
                return expr_value_negative(_x);

            case expr_fn::log1p:
                return (below == D_MATH_VALUE_LESS);

            case expr_fn::asin:
            case expr_fn::acos:
            case expr_fn::atanh:
                return ( (below == D_MATH_VALUE_LESS) ||
                         (above == D_MATH_VALUE_GREATER) );

            case expr_fn::acosh:
                return (above == D_MATH_VALUE_LESS);

            default:
                break;
        }

        return false;
    }

    // expr_complex_sign
    //   function: sign(z) of a complex number: z / |z|, exact where the
    // modulus is; otherwise expr_fn_complex's in the tower's type.
    inline value
    expr_complex_sign(
        const value& _z
    )
    {
        const value modulus = abs(_z);

        // an exact modulus: an exact quotient
        if (modulus.is_exact())
        {
            return (_z / modulus);
        }

        const expr_complex_function op = { expr_fn::sign };

        return expr_complex_apply(&_z, 1, op);
    }

    // expr_complex_value_fn
    //   function: _fn of _count arguments, one of them complex or a real one
    // where _fn has a complex value alone, as 3.3.5 says.
    inline value
    expr_complex_value_fn(
        expr_fn      _fn,
        const value* _x,
        std::size_t  _count
    )
    {
        const expr_complex_function op = { _fn };
        value                       root;

        // by function
        switch (_fn)
        {
            case expr_fn::sqrt:
                // an exact root, where it is a Gaussian rational
                if (expr_gaussian_root(_x[0], 2, root))
                {
                    return expr_complex_flagged(root, _x[0].flags());
                }

                return expr_complex_apply(_x, _count, op);

            case expr_fn::sin:
            case expr_fn::cos:
            case expr_fn::tan:
            case expr_fn::asin:
            case expr_fn::acos:
            case expr_fn::atan:
            case expr_fn::sinh:
            case expr_fn::cosh:
            case expr_fn::tanh:
            case expr_fn::asinh:
            case expr_fn::acosh:
            case expr_fn::atanh:
            case expr_fn::exp:
            case expr_fn::log:
            case expr_fn::exp2:
            case expr_fn::log2:
            case expr_fn::log10:
            case expr_fn::log1p:
            case expr_fn::expm1:
                return expr_complex_apply(_x, _count, op);

            case expr_fn::sign:
                return expr_complex_sign(_x[0]);

            default:
                break;
        }

        // the real line's order
        return value(D_MATH_VALUE_ERROR_DOMAIN);
    }

    // expr_quaternion_exact_root
    //   function: whether the principal _den-th root of the exact quaternion
    // _q is a quaternion of rationals, and if so it, in _out: the square
    // root by quaternion.hpp's expr_quaternion_root; any other where |v| is
    // rational, in q's plane, the Gaussian root of w + |v| im (S337) taken
    // back along u = v / |v|, as each function of a quaternion is the
    // complex one in its plane (S275).
    inline bool
    expr_quaternion_exact_root(
        const value& _q,
        dj_math_imax _den,
        value&       _out
    )
    {
        // the square root
        if (_den == 2)
        {
            return expr_quaternion_root(_q, _out);
        }

        const expr_quaternion_parts_held parts =
            expr_quaternion_parts_of(_q.c_value());
        const value r = value_root_of( ( (parts.x * parts.x) +
                                         (parts.y * parts.y) ) +
                                       (parts.z * parts.z) );
        value       root;

        // |v| irrational, or no Gaussian root in the plane
        if ( (!expr_is_quaternion(_q)) ||
             (!r.is_exact()) ||
             (!expr_gaussian_root(expr_complex(parts.w, r), _den, root)) )
        {
            return false;
        }

        const value c = expr_real(root);
        const value d = expr_imag(root);

        _out = expr_complex_flagged(
            expr_quaternion_made(c,
                                 (d * parts.x) / r,
                                 (d * parts.y) / r,
                                 (d * parts.z) / r,
                                 0u),
            _q.flags());

        return true;
    }

    // expr_quaternion_value_fn
    //   function: _fn of _count arguments, one of them a quaternion, as 3.3.5
    // says: exp and log their principal values, the complex ones in q's
    // plane, in the tower's type; sqrt too, but exact where the root is a
    // quaternion of rationals; and sign q / |q|; any other function a domain
    // error, of the real line's order or none a quaternion has (S275).
    inline value
    expr_quaternion_value_fn(
        expr_fn      _fn,
        const value* _x,
        std::size_t  _count
    )
    {
        value root;

        // of one argument: exp, log, sqrt and sign
        if (_count == 1)
        {
            switch (_fn)
            {
                case expr_fn::exp:
                    return expr_quaternion_apply(expr_quaternion_fn::exp,
                                                 _x[0],
                                                 value(0));

                case expr_fn::log:
                    return expr_quaternion_apply(expr_quaternion_fn::log,
                                                 _x[0],
                                                 value(0));

                case expr_fn::sqrt:
                    // an exact root, where it is a quaternion of rationals
                    if (expr_quaternion_root(_x[0], root))
                    {
                        return root;
                    }

                    return expr_quaternion_apply(expr_quaternion_fn::sqrt,
                                                 _x[0],
                                                 value(0));

                case expr_fn::sign:
                    return expr_quaternion_sign(_x[0]);

                default:
                    break;
            }
        }

        // none of a quaternion
        return value(D_MATH_VALUE_ERROR_DOMAIN);
    }

    // expr_value_signbit
    //   function: whether a real number's sign bit is set: an inexact
    // one's, -0 among them; never an exact one's.
    inline bool
    expr_value_signbit(
        const value& _x
    )
    {
        long double nearest = 0.0L;

        return ( (!_x.is_exact()) &&
                 (_x.to(nearest)) &&
                 (std::signbit(nearest)) );
    }

    // expr_extreme_value
    //   function: min or max of _count values, the first at _x: an error,
    // the first; a domain error for what is no number, and for a complex
    // number or a quaternion, as no order of them is a field's (S271,
    // S275); a NaN, the first,
    // where any is one; and otherwise the least or the greatest argument
    // itself, by value.h's exact order, the first of equal ones but -0
    // below 0 (S338); with every argument's flags.
    inline value
    expr_extreme_value(
        const value* _x,
        std::size_t  _count,
        bool         _greatest
    )
    {
        unsigned    flags = 0u;
        std::size_t best  = 0;
        value       failed;

        // an error, or what is no number
        if (expr_value_failed(_x, _count, failed))
        {
            return failed;
        }

        // a complex number or a quaternion: no order
        for (std::size_t i = 0; i < _count; ++i)
        {
            if ( (expr_is_complex(_x[i])) ||
                 (expr_is_quaternion(_x[i])) )
            {
                return value(D_MATH_VALUE_ERROR_DOMAIN);
            }

            flags |= _x[i].flags();
        }

        // a NaN, the first
        for (std::size_t i = 0; i < _count; ++i)
        {
            if (d_math_value_cmp(_x[i].c_value(), _x[i].c_value()) ==
                    D_MATH_VALUE_UNORDERED)
            {
                return expr_complex_flagged(_x[i], flags);
            }
        }

        // each argument past the best, or a 0 of the sign the extreme takes
        for (std::size_t i = 1; i < _count; ++i)
        {
            const dj_math_value_order order =
                d_math_value_cmp(_x[i].c_value(), _x[best].c_value());
            const bool past  = ( _greatest
                                     ? (order == D_MATH_VALUE_GREATER)
                                     : (order == D_MATH_VALUE_LESS) );
            const bool other =
                ( (order == D_MATH_VALUE_EQUAL) &&
                  (expr_value_signbit(_x[i]) !=
                       expr_value_signbit(_x[best])) &&
                  (expr_value_signbit(_x[i]) != _greatest) );

            if ( (past) ||
                 (other) )
            {
                best = i;
            }
        }

        return expr_complex_flagged(_x[best], flags);
    }

    inline value
    expr_min_value(
        const value*        _x,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_context);

        return expr_extreme_value(_x, _count, false);
    }

    inline value
    expr_max_value(
        const value*        _x,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_context);

        return expr_extreme_value(_x, _count, true);
    }

    // expr_quaternion_value_pow
    //   function: _a ^ _b, one of them a quaternion: to a real exponent
    // alone, a complex or quaternion one a domain error, as its two orders
    // give two values (S275); exact operands to a whole exponent exactly, by
    // repeated squaring, and to a rational one where the root is a
    // quaternion of rationals; otherwise the principal value, quat.h's in
    // the tower's type.
    inline value
    expr_quaternion_value_pow(
        const value& _a,
        const value& _b
    )
    {
        dj_math_imax whole    = 0;
        dj_rational  exponent = { 0, 1 };
        value        root;

        // no real exponent
        if (!expr_complex_real_number(_b))
        {
            return value(D_MATH_VALUE_ERROR_DOMAIN);
        }

        // exact operands: a whole exponent, or a rational one
        if ( (_a.is_exact()) &&
             (_b.is_exact()) )
        {
            if (_b.to(whole))
            {
                return expr_whole_power(_a, whole);
            }

            if ( (_b.to(exponent)) &&
                 (expr_quaternion_exact_root(_a, exponent.den, root)) )
            {
                return expr_whole_power(root, exponent.num);
            }
        }

        return expr_quaternion_apply(expr_quaternion_fn::pow, _a, _b);
    }

    inline value
    expr_value_pow(
        const value& _a,
        const value& _b
    )
    {
        const value  x[2]     = { _a, _b };
        dj_math_imax whole    = 0;
        dj_rational  exponent = { 0, 1 };
        value        exact;

        // a matrix: an error passes, the first; then a matrix's power
        if ( (expr_is_matrix(_a)) ||
             (expr_is_matrix(_b)) )
        {
            if (_a.kind() == D_MATH_VALUE_ERROR)
            {
                return _a;
            }

            if (_b.kind() == D_MATH_VALUE_ERROR)
            {
                return _b;
            }

            return expr_matrix_power(_a,
                                     _b);
        }

        // an error, or what is no number
        if (expr_value_failed(x, 2, exact))
        {
            return exact;
        }

        // a quaternion: expr_quaternion_value_pow's (S275)
        if ( (expr_is_quaternion(_a)) ||
             (expr_is_quaternion(_b)) )
        {
            return expr_quaternion_value_pow(_a, _b);
        }

        // a complex operand, or a negative real base to a power not whole
        // (S272): exact operands to a whole exponent exactly, by repeated
        // squaring, and to a rational one where the base's principal root is
        // a Gaussian rational (S337); otherwise the principal value,
        // expr_fn_complex's in the tower's type (S336)
        if ( (expr_is_complex(_a)) ||
             (expr_is_complex(_b)) ||
             ( (expr_value_negative(_a)) &&
               (expr_value_fraction(_b)) ) )
        {
            const expr_complex_function op = { expr_fn::pow };
            value                       root;

            if ( (_a.is_exact()) &&
                 (_b.is_exact()) )
            {
                if (_b.to(whole))
                {
                    return expr_whole_power(_a, whole);
                }

                if ( (_b.to(exponent)) &&
                     (expr_gaussian_root(_a, exponent.den, root)) )
                {
                    return expr_whole_power(root, exponent.num);
                }
            }

            return expr_complex_apply(x, 2, op);
        }

        // exact operands: a whole exponent, or a rational one
        if ( (_a.is_exact()) &&
             (_b.is_exact()) )
        {
            if (_b.to(whole))
            {
                return expr_whole_power(_a, whole);
            }

            if ( (_b.to(exponent)) &&
                 (expr_ratio_power(_a, exponent, exact)) )
            {
                return exact;
            }
        }

        return expr_tower(expr_fn::pow, x, 2);
    }

    // expr_value_fn_beyond_real
    //   function: whether _fn of the _count arguments at _x takes its value
    // past the real numbers: of a quaternion argument its principal value,
    // or none (S275); of a complex one likewise, none of the real line's
    // order (S271); and of a real one where its value is complex alone
    // (S272); and if so that value, in _out.
    inline bool
    expr_value_fn_beyond_real(
        expr_fn      _fn,
        const value* _x,
        std::size_t  _count,
        value&       _out
    )
    {
        // a quaternion argument
        for (std::size_t i = 0; i < _count; ++i)
        {
            if (expr_is_quaternion(_x[i]))
            {
                _out = expr_quaternion_value_fn(_fn, _x, _count);

                return true;
            }
        }

        // a complex argument
        for (std::size_t i = 0; i < _count; ++i)
        {
            if (expr_is_complex(_x[i]))
            {
                _out = expr_complex_value_fn(_fn, _x, _count);

                return true;
            }
        }

        // a real argument where the function's value is complex alone
        if ( (_count == 1) &&
             (expr_beyond_real(_fn, _x[0])) )
        {
            _out = expr_complex_value_fn(_fn, _x, _count);

            return true;
        }

        return false;
    }

    inline value
    expr_value_fn(
        expr_fn      _fn,
        const value* _x,
        std::size_t  _count
    )
    {
        dj_rational one   = { 0, 1 };
        bool        exact = true;
        value       answer;

        // of a matrix, each element's, element by element: matrix.hpp's
        // (S350), real and conj quaternion.hpp's, which take every number
        if ( ( (_fn == expr_fn::real) ||
               (_fn == expr_fn::imag) ||
               (_fn == expr_fn::conj) ||
               (_fn == expr_fn::arg) ) &&
             (expr_is_matrix(_x[0])) )
        {
            return expr_matrix_each(
                _x[0],
                ( (_fn == expr_fn::real) ? &expr_quaternion_real
                : (_fn == expr_fn::imag) ? &expr_imag
                : (_fn == expr_fn::conj) ? &expr_quaternion_conj
                                         : &expr_arg ));
        }

        // complex(A, B) of matrices: the matrix of complex(a_ij, b_ij)
        if ( (_fn == expr_fn::complex) &&
             ( (expr_is_matrix(_x[0])) ||
               (expr_is_matrix(_x[1])) ) )
        {
            return expr_matrix_complex_of(_x[0],
                                          _x[1]);
        }

        // a number's real part and conjugate, a quaternion's among them,
        // quaternion.hpp's; a complex number's imaginary part and argument,
        // and complex(x, y), complex.hpp's, of a quaternion a domain error
        switch (_fn)
        {
            case expr_fn::real:    return expr_quaternion_real(_x[0]);
            case expr_fn::imag:    return expr_imag(_x[0]);
            case expr_fn::conj:    return expr_quaternion_conj(_x[0]);
            case expr_fn::arg:     return expr_arg(_x[0]);
            case expr_fn::complex: return expr_complex(_x[0], _x[1]);
            default:
                break;
        }

        // |x| is value.h's, the modulus of a complex number, ^'s power
        // pow's, and min and max the least and the greatest
        if (_fn == expr_fn::abs)
        {
            return abs(_x[0]);
        }

        if (_fn == expr_fn::pow)
        {
            return expr_value_pow(_x[0], _x[1]);
        }

        if ( (_fn == expr_fn::min) ||
             (_fn == expr_fn::max) )
        {
            return expr_extreme_value(_x, _count, (_fn == expr_fn::max));
        }

        // an error, or what is no number
        if (expr_value_failed(_x, _count, answer))
        {
            return answer;
        }

        // a quaternion or complex argument, or a real one where the value
        // is complex alone: its principal value, or none
        if (expr_value_fn_beyond_real(_fn, _x, _count, answer))
        {
            return answer;
        }

        for (std::size_t i = 0; i < _count; ++i)
        {
            exact = ( (exact) && (_x[i].is_exact()) );
        }

        // exact arguments, exact where the operation is
        if ( (exact) &&
             ( (_count == 1) ? ( (_x[0].to(one)) &&
                                 (expr_exact_one(_fn, one, answer)) )
                             : expr_exact_two(_fn, _x[0], _x[1], answer) ) )
        {
            return answer;
        }

        return expr_tower(_fn, _x, _count);
    }

    inline value
    expr_value_mod(
        const value& _a,
        const value& _b
    )
    {
        const value x[2] = { _a, _b };

        return expr_value_fn(expr_fn::mod, x, 2);
    }

    // 3.3.6
    // expr_holds_complex, expr_real_only
    //   functions: whether a value is or holds a number not real, a complex
    // number, a quaternion, or a matrix holding either; and an
    // answer under _context: a domain error where it asks real results, none
    // of the _count arguments at _arguments holds a number not real, and the
    // answer does (S272, S340), so that sqrt(-4) and eig of a real matrix
    // with a complex pair (S352) are domain errors there, and a function of
    // a complex argument keeps its complex value. Each standard evaluator
    // as a value that can stray so applies it to its own answer.
    inline bool
    expr_holds_complex(
        const value& _x
    )
    {
        return ( (expr_is_complex(_x)) ||
                 (expr_is_quaternion(_x)) ||
                 ( (expr_is_matrix(_x)) &&
                   ( (expr_matrix_has_complex(_x)) ||
                     (expr_matrix_has_quaternion(_x)) ) ) );
    }

    inline value
    expr_real_only(
        const expr_context& _context,
        const value*        _arguments,
        std::size_t         _count,
        const value&        _answer
    )
    {
        // real results not asked
        if (!_context.real_results)
        {
            return _answer;
        }

        // an argument not real: a complex answer is its own
        for (std::size_t i = 0; i < _count; ++i)
        {
            if (expr_holds_complex(_arguments[i]))
            {
                return _answer;
            }
        }

        return ( (expr_holds_complex(_answer))
                     ? value(D_MATH_VALUE_ERROR_DOMAIN)
                     : _answer );
    }

NS_END  // internal


//==============================================================================
// 4.  THE RELATIONS AND LOGIC
//==============================================================================
// What the relations, logic and piecewise compute, as the banner says: in a
// floating-point type, its booleans 1 and 0; and as a value, booleans.


// 4.1    In a floating-point type
//------------------------------------------------------------------------------
NS_INTERNAL

    // 4.1.1
    // expr_truth_in
    //   function: a truth in Type: 1 for true, 0 for false.
    template<typename Type>
    Type
    expr_truth_in(
        bool _holds
    )
    {
        return ( _holds ? Type(1) : Type(0) );
    }

    // 4.1.2
    // expr_relation_in
    //   function: the relation _kind, one of = != < <= > >=, of _a and _b in
    // Type: C's comparison, which holds of no NaN but !=.
    template<typename Type>
    Type
    expr_relation_in(
        expr_op_kind _kind,
        Type         _a,
        Type         _b
    )
    {
        // by relation
        switch (_kind)
        {
            case expr_op_kind::eq: return expr_truth_in<Type>(_a == _b);
            case expr_op_kind::ne: return expr_truth_in<Type>(_a != _b);
            case expr_op_kind::lt: return expr_truth_in<Type>(_a < _b);
            case expr_op_kind::le: return expr_truth_in<Type>(_a <= _b);
            case expr_op_kind::gt: return expr_truth_in<Type>(_a > _b);
            case expr_op_kind::ge:
            default:
                break;
        }

        return expr_truth_in<Type>(_a >= _b);
    }

    // 4.1.3
    // expr_all_in, expr_any_in, expr_not_in
    //   functions: and of _count truths, the first at _x; or of them; and
    // not, in Type, Kleene's: 0 false, a NaN unknown, any other number true.
    // and is 0 where any truth is 0, then a NaN where any is a NaN, and 1
    // otherwise; or is 1 where any is true, then a NaN, then 0; not of a NaN
    // a NaN.
    template<typename Type>
    Type
    expr_all_in(
        const Type* _x,
        std::size_t _count
    )
    {
        // a false truth: false, whatever the others are
        for (std::size_t i = 0; i < _count; ++i)
        {
            if (_x[i] == Type(0))
            {
                return Type(0);
            }
        }

        // an unknown one: unknown
        for (std::size_t i = 0; i < _count; ++i)
        {
            if (expr_is_nan_of(_x[i]))
            {
                return expr_nan_of<Type>::make();
            }
        }

        return Type(1);
    }

    template<typename Type>
    Type
    expr_any_in(
        const Type* _x,
        std::size_t _count
    )
    {
        // a true truth: true, whatever the others are
        for (std::size_t i = 0; i < _count; ++i)
        {
            if ( (_x[i] != Type(0)) &&
                 (!expr_is_nan_of(_x[i])) )
            {
                return Type(1);
            }
        }

        // an unknown one: unknown
        for (std::size_t i = 0; i < _count; ++i)
        {
            if (expr_is_nan_of(_x[i]))
            {
                return expr_nan_of<Type>::make();
            }
        }

        return Type(0);
    }

    template<typename Type>
    Type
    expr_not_in(
        Type _x
    )
    {
        // unknown, and known
        if (expr_is_nan_of(_x))
        {
            return _x;
        }

        return expr_truth_in<Type>(_x == Type(0));
    }

    // expr_all_of_in, expr_any_of_in
    //   functions: all_of and any_of in Type as evaluators: and and or of
    // the _count truths at _x, the context unread.
    template<typename Type>
    Type
    expr_all_of_in(
        const Type*         _x,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_context);

        return expr_all_in(_x, _count);
    }

    template<typename Type>
    Type
    expr_any_of_in(
        const Type*         _x,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_context);

        return expr_any_in(_x, _count);
    }

    // 4.1.4
    // expr_piecewise_in
    //   function: piecewise of _count arguments in Type, the first at _x: the
    // value of the first branch whose condition is true, a NaN where a
    // condition met first is unknown, or where none holds; an odd last
    // argument the otherwise.
    template<typename Type>
    Type
    expr_piecewise_in(
        const Type*         _x,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_context);

        std::size_t i = 0;

        // each branch in turn, by its condition
        for (; i + 1 < _count; i += 2)
        {
            if (expr_is_nan_of(_x[i]))
            {
                return _x[i];
            }

            if (_x[i] != Type(0))
            {
                return _x[i + 1];
            }
        }

        // the otherwise, where there is one
        if (i < _count)
        {
            return _x[i];
        }

        return expr_nan_of<Type>::make();
    }

NS_END  // internal

// 4.2    As a value
//------------------------------------------------------------------------------
NS_INTERNAL

    // 4.2.1
    // expr_relation_value
    //   function: the relation _kind of _a and _b as a value, a boolean: of
    // two numbers by value.h's exact order, unordered for a NaN, which none
    // holds of but !=; of a complex number or a quaternion and a number, =
    // and != by their parts, and the others a domain error, as no order of
    // the complex numbers or the quaternions is a field's (S271, S275); of
    // two booleans, = and != by their truth;
    // of two matrices, = and != by the kind's comparison, equal where of one
    // shape and equal place by place (S242). An error passes, the first; any
    // other operands are a domain error.
    inline value
    expr_relation_value(
        expr_op_kind _kind,
        const value& _a,
        const value& _b
    )
    {
        bool truth_a = false;
        bool truth_b = false;

        // an error passes, the first
        if (_a.kind() == D_MATH_VALUE_ERROR)
        {
            return _a;
        }

        if (_b.kind() == D_MATH_VALUE_ERROR)
        {
            return _b;
        }

        // two matrices, equal or not: of one shape and their elements
        // equal, place by place, by the kind's comparison
        if ( (expr_is_matrix(_a)) &&
             (expr_is_matrix(_b)) &&
             ( (_kind == expr_op_kind::eq) ||
               (_kind == expr_op_kind::ne) ) )
        {
            return value( (_a.compare(_b) == D_MATH_VALUE_EQUAL) ==
                          (_kind == expr_op_kind::eq) );
        }

        // two numbers: their order; a complex one equal or not, in none
        if ( (_a.is_number()) &&
             (_b.is_number()) )
        {
            const dj_math_value_order order = _a.compare(_b);

            if ( ( (expr_is_complex(_a)) ||
                   (expr_is_complex(_b)) ||
                   (expr_is_quaternion(_a)) ||
                   (expr_is_quaternion(_b)) ) &&
                 (_kind != expr_op_kind::eq) &&
                 (_kind != expr_op_kind::ne) )
            {
                return value(D_MATH_VALUE_ERROR_DOMAIN);
            }

            switch (_kind)
            {
                case expr_op_kind::eq:
                    return value(order == D_MATH_VALUE_EQUAL);

                case expr_op_kind::ne:
                    return value(order != D_MATH_VALUE_EQUAL);

                case expr_op_kind::lt:
                    return value(order == D_MATH_VALUE_LESS);

                case expr_op_kind::le:
                    return value( (order == D_MATH_VALUE_LESS) ||
                                  (order == D_MATH_VALUE_EQUAL) );

                case expr_op_kind::gt:
                    return value(order == D_MATH_VALUE_GREATER);

                case expr_op_kind::ge:
                default:
                    break;
            }

            return value( (order == D_MATH_VALUE_GREATER) ||
                          (order == D_MATH_VALUE_EQUAL) );
        }

        // two booleans, equal or not
        if ( (_a.to(truth_a)) &&
             (_b.to(truth_b)) &&
             ( (_kind == expr_op_kind::eq) ||
               (_kind == expr_op_kind::ne) ) )
        {
            return value( (truth_a == truth_b) ==
                          (_kind == expr_op_kind::eq) );
        }

        return value(D_MATH_VALUE_ERROR_DOMAIN);
    }

    // 4.2.2
    // expr_all_value, expr_any_value, expr_not_value
    //   functions: and of _count values, the first at _x; or of them; and
    // not, as values, Kleene's: a false operand makes and false, a true one
    // makes or true, whatever the others are; otherwise the first operand
    // that is an error, or no boolean, gives that error, or a domain error.
    // expr_logic_value is either, by the truth _deciding that decides it.
    inline value
    expr_logic_value(
        const value* _x,
        std::size_t  _count,
        bool         _deciding
    )
    {
        bool truth = false;

        // an operand of the deciding truth decides
        for (std::size_t i = 0; i < _count; ++i)
        {
            if ( (_x[i].to(truth)) &&
                 (truth == _deciding) )
            {
                return value(_deciding);
            }
        }

        // otherwise the first error, or the first that is no boolean
        for (std::size_t i = 0; i < _count; ++i)
        {
            if (_x[i].kind() == D_MATH_VALUE_ERROR)
            {
                return _x[i];
            }

            if (!_x[i].to(truth))
            {
                return value(D_MATH_VALUE_ERROR_DOMAIN);
            }
        }

        return value(!_deciding);
    }

    inline value
    expr_all_value(
        const value* _x,
        std::size_t  _count
    )
    {
        return expr_logic_value(_x, _count, false);
    }

    inline value
    expr_any_value(
        const value* _x,
        std::size_t  _count
    )
    {
        return expr_logic_value(_x, _count, true);
    }

    // expr_all_of_value, expr_any_of_value
    //   functions: all_of and any_of as values, as evaluators: and and or of
    // the _count values at _x, the context unread.
    inline value
    expr_all_of_value(
        const value*        _x,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_context);

        return expr_all_value(_x, _count);
    }

    inline value
    expr_any_of_value(
        const value*        _x,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_context);

        return expr_any_value(_x, _count);
    }

    inline value
    expr_not_value(
        const value& _x
    )
    {
        bool truth = false;

        // an error passes; a boolean, negated
        if (_x.kind() == D_MATH_VALUE_ERROR)
        {
            return _x;
        }

        if (_x.to(truth))
        {
            return value(!truth);
        }

        return value(D_MATH_VALUE_ERROR_DOMAIN);
    }

    // 4.2.3
    // expr_piecewise_value
    //   function: piecewise of _count values, the first at _x: the value of
    // the first branch whose condition is true; a condition met first that is
    // an error passes, one that is no boolean is a domain error; where no
    // branch holds, a domain error. An odd last argument is the otherwise.
    inline value
    expr_piecewise_value(
        const value*        _x,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_context);

        bool        truth = false;
        std::size_t i     = 0;

        // each branch in turn, by its condition
        for (; i + 1 < _count; i += 2)
        {
            if (_x[i].kind() == D_MATH_VALUE_ERROR)
            {
                return _x[i];
            }

            if (!_x[i].to(truth))
            {
                return value(D_MATH_VALUE_ERROR_DOMAIN);
            }

            if (truth)
            {
                return _x[i + 1];
            }
        }

        // the otherwise, where there is one
        if (i < _count)
        {
            return _x[i];
        }

        return value(D_MATH_VALUE_ERROR_DOMAIN);
    }

NS_END  // internal


//==============================================================================
// 5.  LINEAR ALGEBRA
//==============================================================================
// The functions of linear algebra (S256): in a type, where every value is a
// number, a scalar's, as in Julia: transpose, det, trace and eig of x are
// x, inv its reciprocal, norm its magnitude, dot(x, y) conj(x) y, solve(a,
// b) b / a, and cross a NaN, as it takes no number; as a value, matrix.hpp's,
// of complex elements too (S350, S351).


// 5.1    In a floating-point type
//------------------------------------------------------------------------------
NS_INTERNAL

    // 5.1.1
    // expr_itself_in, expr_reciprocal_in, expr_magnitude_in,
    // expr_product_in, expr_quotient_in, expr_no_scalar_in
    //   functions: x; 1 / x; |x|; conj(x) y; the second over the first; a
    // NaN; in a real type or a complex one, where a complex number's |z| is
    // its modulus, plus +0i (S339), 1 / z takes its 1 as itself (G.5), and
    // dot's conj(x) y conjugates x, as dot of vectors does (S351).
    template<typename Type>
    Type
    expr_magnitude_of(
        Type _x
    )
    {
        return std::fabs(_x);
    }

    template<typename Type>
    math::complex<Type>
    expr_magnitude_of(
        const math::complex<Type>& _z
    )
    {
        return math::complex<Type>(abs(_z));
    }

    template<typename Type>
    Type
    expr_itself_in(
        const Type*         _arguments,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_count);
        static_cast<void>(_context);

        return _arguments[0];
    }

    template<typename Type>
    Type
    expr_one_over(
        Type _x
    )
    {
        return (Type(1) / _x);
    }

    template<typename Type>
    math::complex<Type>
    expr_one_over(
        const math::complex<Type>& _z
    )
    {
        return (Type(1) / _z);
    }

    template<typename Type>
    Type
    expr_reciprocal_in(
        const Type*         _arguments,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_count);
        static_cast<void>(_context);

        return expr_one_over(_arguments[0]);
    }

    template<typename Type>
    Type
    expr_magnitude_in(
        const Type*         _arguments,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_count);
        static_cast<void>(_context);

        return expr_magnitude_of(_arguments[0]);
    }

    template<typename Type>
    Type
    expr_conjugate_of(
        Type _x
    )
    {
        return _x;
    }

    template<typename Type>
    math::complex<Type>
    expr_conjugate_of(
        const math::complex<Type>& _z
    )
    {
        return conj(_z);
    }

    template<typename Type>
    Type
    expr_product_in(
        const Type*         _arguments,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_count);
        static_cast<void>(_context);

        return (expr_conjugate_of(_arguments[0]) * _arguments[1]);
    }

    template<typename Type>
    Type
    expr_quotient_in(
        const Type*         _arguments,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_count);
        static_cast<void>(_context);

        return (_arguments[1] / _arguments[0]);
    }

    template<typename Type>
    Type
    expr_no_scalar_in(
        const Type*         _arguments,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_arguments);
        static_cast<void>(_count);
        static_cast<void>(_context);

        return expr_nan_of<Type>::make();
    }

NS_END  // internal

// 5.2    As a value
//------------------------------------------------------------------------------
NS_INTERNAL

    // 5.2.1
    // expr_transpose_fn_value, expr_det_value, expr_inv_value,
    // expr_trace_value, expr_norm_value, expr_eig_value, expr_dot_value,
    // expr_cross_value, expr_solve_value
    //   functions: each function of linear algebra as a value, matrix.hpp's
    // (3.3, 3.5): of a matrix, or of a number, a scalar, transpose(A) the
    // transpose, unconjugated (S350); an error passes, the first; a boolean
    // or none, and a shape the function does not take, a domain error; eig
    // of a real matrix with a complex pair a domain error too where the
    // context asks real results (S340, S352).
    inline value
    expr_transpose_fn_value(
        const value*        _arguments,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_count);
        static_cast<void>(_context);

        return expr_transpose_value(_arguments[0],
                                    false);
    }

    inline value
    expr_det_value(
        const value*        _arguments,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_count);
        static_cast<void>(_context);

        return expr_matrix_det(_arguments[0]);
    }

    inline value
    expr_inv_value(
        const value*        _arguments,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_count);
        static_cast<void>(_context);

        // a quaternion: its inverse (S275)
        if (expr_is_quaternion(_arguments[0]))
        {
            return expr_quaternion_inverse(_arguments[0]);
        }

        return expr_matrix_inv(_arguments[0]);
    }

    inline value
    expr_trace_value(
        const value*        _arguments,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_count);
        static_cast<void>(_context);

        return expr_matrix_trace(_arguments[0]);
    }

    inline value
    expr_norm_value(
        const value*        _arguments,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_count);
        static_cast<void>(_context);

        return expr_matrix_norm(_arguments[0]);
    }

    inline value
    expr_eig_value(
        const value*        _arguments,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_count);

        // a real matrix's complex pair, a domain error where real results
        // are asked (S352)
        return expr_real_only(_context,
                              _arguments,
                              1,
                              expr_matrix_eig(_arguments[0]));
    }

    inline value
    expr_dot_value(
        const value*        _arguments,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_count);
        static_cast<void>(_context);

        return expr_matrix_dot(_arguments[0],
                               _arguments[1]);
    }

    inline value
    expr_cross_value(
        const value*        _arguments,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_count);
        static_cast<void>(_context);

        return expr_matrix_cross(_arguments[0],
                                 _arguments[1]);
    }

    inline value
    expr_solve_value(
        const value*        _arguments,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_count);
        static_cast<void>(_context);

        return expr_matrix_solve(_arguments[0],
                                 _arguments[1]);
    }

NS_END  // internal

// 5.3    The quaternions' own
//------------------------------------------------------------------------------
NS_INTERNAL

    // 5.3.1
    // expr_quat_in, expr_quat_complex, expr_quat_value
    //   functions: quat(w, x, y, z), the quaternion of four real parts
    // (S275): in a real type w where x, y and z are 0, of either sign, and a
    // NaN otherwise, as complex(x, y) is there (S332); in a complex type, of
    // real arguments, w + x i where y and z are 0, and a NaN in both parts
    // otherwise; as a value quaternion.hpp's expr_quaternion.
    template<typename Type>
    Type
    expr_quat_in(
        const Type*         _arguments,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_count);
        static_cast<void>(_context);

        // a part past w not 0, or a NaN: no real number
        if ( (_arguments[1] != Type(0)) ||
             (_arguments[2] != Type(0)) ||
             (_arguments[3] != Type(0)) )
        {
            return expr_nan_of<Type>::make();
        }

        return _arguments[0];
    }

    template<typename Type>
    math::complex<Type>
    expr_quat_complex(
        const math::complex<Type>* _arguments,
        std::size_t                _count,
        const expr_context&        _context
    )
    {
        static_cast<void>(_count);
        static_cast<void>(_context);

        // an argument not real: no part
        for (std::size_t i = 0; i < 4; ++i)
        {
            if (_arguments[i].imag() != Type(0))
            {
                return expr_nan_of<math::complex<Type> >::make();
            }
        }

        // a jm or km part not 0, or a NaN: no complex number
        if ( (_arguments[2].real() != Type(0)) ||
             (_arguments[3].real() != Type(0)) )
        {
            return expr_nan_of<math::complex<Type> >::make();
        }

        return math::complex<Type>(_arguments[0].real(),
                                   _arguments[1].real());
    }

    inline value
    expr_quat_value(
        const value*        _arguments,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_count);
        static_cast<void>(_context);

        return expr_quaternion(_arguments[0],
                               _arguments[1],
                               _arguments[2],
                               _arguments[3]);
    }

    // 5.3.2
    // expr_parts_value, expr_rotate_value
    //   functions: parts(q), a number's four parts, the vector [w, x, y, z],
    // and rotate(q, v), a 3-vector rotated by q, q v q^-1 (S275), as values:
    // quaternion.hpp's expr_quaternion_parts and expr_quaternion_rotate. In
    // a type, which holds no vector, each is a NaN, as cross is
    // (expr_no_scalar_in).
    inline value
    expr_parts_value(
        const value*        _arguments,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_count);
        static_cast<void>(_context);

        return expr_quaternion_parts(_arguments[0]);
    }

    inline value
    expr_rotate_value(
        const value*        _arguments,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_count);
        static_cast<void>(_context);

        return expr_quaternion_rotate(_arguments[0],
                                      _arguments[1]);
    }

    // 5.3.3
    // expr_rotmat_value, expr_rotquat_value, expr_slerp_value
    //   functions: rotmat(q), the 3 by 3 matrix of the rotation q names;
    // rotquat(R), the unit quaternion of a rotation's matrix, w >= 0; and
    // slerp(p, q, t), the shorter arc from p's rotation to q's (S391), as
    // values: rotation.hpp's expr_quaternion_matrix,
    // expr_quaternion_of_rotation and expr_quaternion_slerp. In a type each
    // is a NaN, as parts is (expr_no_scalar_in).
    inline value
    expr_rotmat_value(
        const value*        _arguments,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_count);
        static_cast<void>(_context);

        return expr_quaternion_matrix(_arguments[0]);
    }

    inline value
    expr_rotquat_value(
        const value*        _arguments,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_count);
        static_cast<void>(_context);

        return expr_quaternion_of_rotation(_arguments[0]);
    }

    inline value
    expr_slerp_value(
        const value*        _arguments,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_count);
        static_cast<void>(_context);

        return expr_quaternion_slerp(_arguments[0],
                                     _arguments[1],
                                     _arguments[2]);
    }

NS_END  // internal


//==============================================================================
// 6.  THE STANDARD TABLES
//==============================================================================
// The language's own tables, each built once.


// 6.1    The functions
//------------------------------------------------------------------------------
NS_INTERNAL

    // expr_entry_in, expr_entry_complex, expr_entry_value
    //   functions: the evaluators of the operation Fn, in a real type, in a
    // complex one and as a value, as an expr_function holds them, each given
    // the fold's context (S340): in a type it changes no answer, and as a
    // value it holds the answer real where it asks (expr_real_only),
    // complex(x, y) apart, which makes a complex number of its asking.
    template<expr_fn Fn,
             typename Type>
    Type
    expr_entry_in(
        const Type*         _arguments,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        static_cast<void>(_count);
        static_cast<void>(_context);

        return expr_fn_in<Type>(Fn, _arguments);
    }

    template<expr_fn Fn,
             typename Type>
    math::complex<Type>
    expr_entry_complex(
        const math::complex<Type>* _arguments,
        std::size_t                _count,
        const expr_context&        _context
    )
    {
        static_cast<void>(_count);
        static_cast<void>(_context);

        return expr_fn_complex<Type>(Fn, _arguments);
    }

    template<expr_fn Fn>
    value
    expr_entry_value(
        const value*        _arguments,
        std::size_t         _count,
        const expr_context& _context
    )
    {
        const value answer = expr_value_fn(Fn, _arguments, _count);

        // complex(x, y): the complex number asked for
        if (Fn == expr_fn::complex)
        {
            return answer;
        }

        return expr_real_only(_context,
                              _arguments,
                              _count,
                              answer);
    }

    // expr_entry
    //   function: the standard function of Fn: its name, the number of
    // arguments it takes, and its seven evaluators; no rule of its partial
    // derivatives before step 25 (4a).
    template<expr_fn Fn>
    expr_function
    expr_entry(
        const char* _name,
        std::size_t _arity
    )
    {
        expr_function entry(_name, _arity, _arity);

        entry.evaluate_float               = &expr_entry_in<Fn, float>;
        entry.evaluate_double              = &expr_entry_in<Fn, double>;
        entry.evaluate_long_double         = &expr_entry_in<Fn, long double>;
        entry.evaluate_complex_float       = &expr_entry_complex<Fn, float>;
        entry.evaluate_complex_double      = &expr_entry_complex<Fn, double>;
        entry.evaluate_complex_long_double =
            &expr_entry_complex<Fn, long double>;
        entry.evaluate_value               = &expr_entry_value<Fn>;

        return entry;
    }

    // expr_entry_any
    //   function: a standard function of any number of arguments from 1, of
    // its seven evaluators; no rule of its partial derivatives before step 25
    // (4a).
    inline expr_function
    expr_entry_any(
        const char*                                 _name,
        expr_evaluator<float>                       _in_float,
        expr_evaluator<double>                      _in_double,
        expr_evaluator<long double>                 _in_long_double,
        expr_evaluator<math::complex<float> >       _in_complex_float,
        expr_evaluator<math::complex<double> >      _in_complex_double,
        expr_evaluator<math::complex<long double> > _in_complex_long_double,
        expr_evaluator<value>                       _as_value
    )
    {
        expr_function entry(_name, 1, expr_variadic);

        entry.evaluate_float               = _in_float;
        entry.evaluate_double              = _in_double;
        entry.evaluate_long_double         = _in_long_double;
        entry.evaluate_complex_float       = _in_complex_float;
        entry.evaluate_complex_double      = _in_complex_double;
        entry.evaluate_complex_long_double = _in_complex_long_double;
        entry.evaluate_value               = _as_value;

        return entry;
    }

    // expr_entry_algebra
    //   function: a function of linear algebra (S256), or of the
    // quaternions' own (S275), of its name, its number of arguments and its
    // seven evaluators; no rule of its partial derivatives before step 25
    // (4a).
    inline expr_function
    expr_entry_algebra(
        const char*                                 _name,
        std::size_t                                 _arity,
        expr_evaluator<float>                       _in_float,
        expr_evaluator<double>                      _in_double,
        expr_evaluator<long double>                 _in_long_double,
        expr_evaluator<math::complex<float> >       _in_complex_float,
        expr_evaluator<math::complex<double> >      _in_complex_double,
        expr_evaluator<math::complex<long double> > _in_complex_long_double,
        expr_evaluator<value>                       _as_value
    )
    {
        expr_function entry(_name,
                            _arity,
                            _arity);

        entry.evaluate_float               = _in_float;
        entry.evaluate_double              = _in_double;
        entry.evaluate_long_double         = _in_long_double;
        entry.evaluate_complex_float       = _in_complex_float;
        entry.evaluate_complex_double      = _in_complex_double;
        entry.evaluate_complex_long_double = _in_complex_long_double;
        entry.evaluate_value               = _as_value;

        return entry;
    }

    // expr_define_extensions
    //   function: into _table, the functions past the C core's first ones:
    // log1p, expm1, asinh, acosh, atanh and sign (S203, S338), min and max
    // of any number of arguments from 1, and the vocabulary's piecewise,
    // all_of and any_of, in the banner's order.
    inline void
    expr_define_extensions(
        expr_function_table& _table
    )
    {
        typedef math::complex<float>       complex_f;
        typedef math::complex<double>      complex_d;
        typedef math::complex<long double> complex_ld;

        _table.define(expr_entry<expr_fn::log1p>("log1p", 1))
              .define(expr_entry<expr_fn::expm1>("expm1", 1))
              .define(expr_entry<expr_fn::asinh>("asinh", 1))
              .define(expr_entry<expr_fn::acosh>("acosh", 1))
              .define(expr_entry<expr_fn::atanh>("atanh", 1))
              .define(expr_entry<expr_fn::sign>("sign", 1))
              .define(expr_entry_any("min",
                                     &expr_min_in<float>,
                                     &expr_min_in<double>,
                                     &expr_min_in<long double>,
                                     &expr_min_complex<float>,
                                     &expr_min_complex<double>,
                                     &expr_min_complex<long double>,
                                     &expr_min_value))
              .define(expr_entry_any("max",
                                     &expr_max_in<float>,
                                     &expr_max_in<double>,
                                     &expr_max_in<long double>,
                                     &expr_max_complex<float>,
                                     &expr_max_complex<double>,
                                     &expr_max_complex<long double>,
                                     &expr_max_value))
              .define(expr_entry_any("piecewise",
                                     &expr_piecewise_in<float>,
                                     &expr_piecewise_in<double>,
                                     &expr_piecewise_in<long double>,
                                     &expr_piecewise_in<complex_f>,
                                     &expr_piecewise_in<complex_d>,
                                     &expr_piecewise_in<complex_ld>,
                                     &expr_piecewise_value))
              .define(expr_entry_any("all_of",
                                     &expr_all_of_in<float>,
                                     &expr_all_of_in<double>,
                                     &expr_all_of_in<long double>,
                                     &expr_all_of_in<complex_f>,
                                     &expr_all_of_in<complex_d>,
                                     &expr_all_of_in<complex_ld>,
                                     &expr_all_of_value))
              .define(expr_entry_any("any_of",
                                     &expr_any_of_in<float>,
                                     &expr_any_of_in<double>,
                                     &expr_any_of_in<long double>,
                                     &expr_any_of_in<complex_f>,
                                     &expr_any_of_in<complex_d>,
                                     &expr_any_of_in<complex_ld>,
                                     &expr_any_of_value));

        return;
    }

    // expr_define_algebra
    //   function: into _table, linear algebra's functions, transpose, det,
    // inv, trace, norm, eig, dot, cross and solve (S256, S350), in the
    // banner's order.
    inline void
    expr_define_algebra(
        expr_function_table& _table
    )
    {
        typedef math::complex<float>       complex_f;
        typedef math::complex<double>      complex_d;
        typedef math::complex<long double> complex_ld;

        _table.define(expr_entry_algebra("transpose",
                                         1,
                                         &expr_itself_in<float>,
                                         &expr_itself_in<double>,
                                         &expr_itself_in<long double>,
                                         &expr_itself_in<complex_f>,
                                         &expr_itself_in<complex_d>,
                                         &expr_itself_in<complex_ld>,
                                         &expr_transpose_fn_value))
              .define(expr_entry_algebra("det",
                                         1,
                                         &expr_itself_in<float>,
                                         &expr_itself_in<double>,
                                         &expr_itself_in<long double>,
                                         &expr_itself_in<complex_f>,
                                         &expr_itself_in<complex_d>,
                                         &expr_itself_in<complex_ld>,
                                         &expr_det_value))
              .define(expr_entry_algebra("inv",
                                         1,
                                         &expr_reciprocal_in<float>,
                                         &expr_reciprocal_in<double>,
                                         &expr_reciprocal_in<long double>,
                                         &expr_reciprocal_in<complex_f>,
                                         &expr_reciprocal_in<complex_d>,
                                         &expr_reciprocal_in<complex_ld>,
                                         &expr_inv_value))
              .define(expr_entry_algebra("trace",
                                         1,
                                         &expr_itself_in<float>,
                                         &expr_itself_in<double>,
                                         &expr_itself_in<long double>,
                                         &expr_itself_in<complex_f>,
                                         &expr_itself_in<complex_d>,
                                         &expr_itself_in<complex_ld>,
                                         &expr_trace_value))
              .define(expr_entry_algebra("norm",
                                         1,
                                         &expr_magnitude_in<float>,
                                         &expr_magnitude_in<double>,
                                         &expr_magnitude_in<long double>,
                                         &expr_magnitude_in<complex_f>,
                                         &expr_magnitude_in<complex_d>,
                                         &expr_magnitude_in<complex_ld>,
                                         &expr_norm_value))
              .define(expr_entry_algebra("eig",
                                         1,
                                         &expr_itself_in<float>,
                                         &expr_itself_in<double>,
                                         &expr_itself_in<long double>,
                                         &expr_itself_in<complex_f>,
                                         &expr_itself_in<complex_d>,
                                         &expr_itself_in<complex_ld>,
                                         &expr_eig_value))
              .define(expr_entry_algebra("dot",
                                         2,
                                         &expr_product_in<float>,
                                         &expr_product_in<double>,
                                         &expr_product_in<long double>,
                                         &expr_product_in<complex_f>,
                                         &expr_product_in<complex_d>,
                                         &expr_product_in<complex_ld>,
                                         &expr_dot_value))
              .define(expr_entry_algebra("cross",
                                         2,
                                         &expr_no_scalar_in<float>,
                                         &expr_no_scalar_in<double>,
                                         &expr_no_scalar_in<long double>,
                                         &expr_no_scalar_in<complex_f>,
                                         &expr_no_scalar_in<complex_d>,
                                         &expr_no_scalar_in<complex_ld>,
                                         &expr_cross_value))
              .define(expr_entry_algebra("solve",
                                         2,
                                         &expr_quotient_in<float>,
                                         &expr_quotient_in<double>,
                                         &expr_quotient_in<long double>,
                                         &expr_quotient_in<complex_f>,
                                         &expr_quotient_in<complex_d>,
                                         &expr_quotient_in<complex_ld>,
                                         &expr_solve_value));

        return;
    }

    // expr_define_rotations
    //   function: into _table, the functions of a rotation, the quaternions'
    // rotate (S275), rotmat, rotquat and slerp (S391), in the banner's
    // order: each no function of numbers alone, so a NaN in a type.
    inline void
    expr_define_rotations(
        expr_function_table& _table
    )
    {
        typedef math::complex<float>       complex_f;
        typedef math::complex<double>      complex_d;
        typedef math::complex<long double> complex_ld;

        _table.define(expr_entry_algebra("rotate",
                                         2,
                                         &expr_no_scalar_in<float>,
                                         &expr_no_scalar_in<double>,
                                         &expr_no_scalar_in<long double>,
                                         &expr_no_scalar_in<complex_f>,
                                         &expr_no_scalar_in<complex_d>,
                                         &expr_no_scalar_in<complex_ld>,
                                         &expr_rotate_value))
              .define(expr_entry_algebra("rotmat",
                                         1,
                                         &expr_no_scalar_in<float>,
                                         &expr_no_scalar_in<double>,
                                         &expr_no_scalar_in<long double>,
                                         &expr_no_scalar_in<complex_f>,
                                         &expr_no_scalar_in<complex_d>,
                                         &expr_no_scalar_in<complex_ld>,
                                         &expr_rotmat_value))
              .define(expr_entry_algebra("rotquat",
                                         1,
                                         &expr_no_scalar_in<float>,
                                         &expr_no_scalar_in<double>,
                                         &expr_no_scalar_in<long double>,
                                         &expr_no_scalar_in<complex_f>,
                                         &expr_no_scalar_in<complex_d>,
                                         &expr_no_scalar_in<complex_ld>,
                                         &expr_rotquat_value))
              .define(expr_entry_algebra("slerp",
                                         3,
                                         &expr_no_scalar_in<float>,
                                         &expr_no_scalar_in<double>,
                                         &expr_no_scalar_in<long double>,
                                         &expr_no_scalar_in<complex_f>,
                                         &expr_no_scalar_in<complex_d>,
                                         &expr_no_scalar_in<complex_ld>,
                                         &expr_slerp_value));

        return;
    }

    // expr_build_functions
    //   function: the standard function table, in the banner's order: the
    // C core's and the language's first, the complex numbers' and the
    // quaternions' own, then expr_define_extensions',
    // expr_define_algebra's and expr_define_rotations'.
    inline expr_function_table
    expr_build_functions()
    {
        typedef math::complex<float>       complex_f;
        typedef math::complex<double>      complex_d;
        typedef math::complex<long double> complex_ld;

        expr_function_table table;

        table.define(expr_entry<expr_fn::sqrt>("sqrt", 1))
             .define(expr_entry<expr_fn::cbrt>("cbrt", 1))
             .define(expr_entry<expr_fn::sin>("sin", 1))
             .define(expr_entry<expr_fn::cos>("cos", 1))
             .define(expr_entry<expr_fn::tan>("tan", 1))
             .define(expr_entry<expr_fn::asin>("asin", 1))
             .define(expr_entry<expr_fn::acos>("acos", 1))
             .define(expr_entry<expr_fn::atan>("atan", 1))
             .define(expr_entry<expr_fn::atan2>("atan2", 2))
             .define(expr_entry<expr_fn::sinh>("sinh", 1))
             .define(expr_entry<expr_fn::cosh>("cosh", 1))
             .define(expr_entry<expr_fn::tanh>("tanh", 1))
             .define(expr_entry<expr_fn::exp>("exp", 1))
             .define(expr_entry<expr_fn::exp2>("exp2", 1))
             .define(expr_entry<expr_fn::log>("log", 1))
             .define(expr_entry<expr_fn::log2>("log2", 1))
             .define(expr_entry<expr_fn::log10>("log10", 1))
             .define(expr_entry<expr_fn::pow>("pow", 2))
             .define(expr_entry<expr_fn::hypot>("hypot", 2))
             .define(expr_entry<expr_fn::fmod>("fmod", 2))
             .define(expr_entry<expr_fn::remainder>("remainder", 2))
             .define(expr_entry<expr_fn::floor>("floor", 1))
             .define(expr_entry<expr_fn::ceil>("ceil", 1))
             .define(expr_entry<expr_fn::round>("round", 1))
             .define(expr_entry<expr_fn::trunc>("trunc", 1))
             .define(expr_entry<expr_fn::abs>("abs", 1))
             .define(expr_entry<expr_fn::factorial>("factorial", 1))
             .define(expr_entry<expr_fn::complex>("complex", 2))
             .define(expr_entry<expr_fn::real>("real", 1))
             .define(expr_entry<expr_fn::imag>("imag", 1))
             .define(expr_entry<expr_fn::conj>("conj", 1))
             .define(expr_entry<expr_fn::arg>("arg", 1))
             .define(expr_entry_algebra("quat",
                                        4,
                                        &expr_quat_in<float>,
                                        &expr_quat_in<double>,
                                        &expr_quat_in<long double>,
                                        &expr_quat_complex<float>,
                                        &expr_quat_complex<double>,
                                        &expr_quat_complex<long double>,
                                        &expr_quat_value))
             .define(expr_entry_algebra("parts",
                                        1,
                                        &expr_no_scalar_in<float>,
                                        &expr_no_scalar_in<double>,
                                        &expr_no_scalar_in<long double>,
                                        &expr_no_scalar_in<complex_f>,
                                        &expr_no_scalar_in<complex_d>,
                                        &expr_no_scalar_in<complex_ld>,
                                        &expr_parts_value));

        expr_define_extensions(table);
        expr_define_algebra(table);
        expr_define_rotations(table);

        return table;
    }

NS_END  // internal

// 6.1.1
// expr_standard_functions
//   function: the language's functions, the banner's 58, built once.
inline const expr_function_table&
expr_standard_functions()
{
    static const expr_function_table table = internal::expr_build_functions();

    return table;
}

// 6.2    The constants
//------------------------------------------------------------------------------
NS_INTERNAL

    // expr_constant_entry
    //   function: a constant of the table from its long double: each type's
    // value that long double rounded to it, as constants::of<T> rounds its
    // literal, and as a value the long double itself.
    inline expr_named_constant
    expr_constant_entry(
        const char* _name,
        long double _nearest
    )
    {
        return expr_named_constant(_name,
                                   static_cast<float>(_nearest),
                                   static_cast<double>(_nearest),
                                   _nearest,
                                   value(_nearest));
    }

    // expr_unit_entry
    //   function: im, the imaginary unit: as a value the exact complex number
    // 0 + 1im, one box, which every fold that reads it holds, counted
    // atomically (S134); in each real floating-point type a NaN, as sqrt(-1)
    // is there (S271); and in each complex one 0 + 1i (S339).
    inline expr_named_constant
    expr_unit_entry()
    {
        expr_named_constant unit("im",
                                 std::numeric_limits<float>::quiet_NaN(),
                                 std::numeric_limits<double>::quiet_NaN(),
                                 std::numeric_limits<long double>::quiet_NaN(),
                                 expr_complex(value(0), value(1)));

        unit.as_complex_float       = math::complex<float>(0.0f, 1.0f);
        unit.as_complex_double      = math::complex<double>(0.0, 1.0);
        unit.as_complex_long_double = math::complex<long double>(0.0L, 1.0L);

        return unit;
    }

    // expr_quaternion_unit_entry
    //   function: jm or km, where _km, a unit of the quaternions (S275): as a
    // value the exact quaternion 0 + 1jm or 0 + 1km, one box, which every
    // fold that reads it holds, counted atomically (S134); in each real
    // floating-point type a NaN, as im is there (S271); and in each complex
    // one a NaN in both parts, as no complex number is a unit past im.
    inline expr_named_constant
    expr_quaternion_unit_entry(
        const char* _name,
        bool        _km
    )
    {
        expr_named_constant unit(
            _name,
            std::numeric_limits<float>::quiet_NaN(),
            std::numeric_limits<double>::quiet_NaN(),
            std::numeric_limits<long double>::quiet_NaN(),
            expr_quaternion(value(0),
                            value(0),
                            value( _km ? 0 : 1 ),
                            value( _km ? 1 : 0 )));

        unit.as_complex_float       =
            expr_nan_of<math::complex<float> >::make();
        unit.as_complex_double      =
            expr_nan_of<math::complex<double> >::make();
        unit.as_complex_long_double =
            expr_nan_of<math::complex<long double> >::make();

        return unit;
    }

    // expr_boolean_entry
    //   function: a constant of the table from its truth: 1 or 0 in each
    // type, and as a value the boolean.
    inline expr_named_constant
    expr_boolean_entry(
        const char* _name,
        bool        _truth
    )
    {
        return expr_named_constant(_name,
                                   ( _truth ? 1.0f : 0.0f ),
                                   ( _truth ? 1.0 : 0.0 ),
                                   ( _truth ? 1.0L : 0.0L ),
                                   value(_truth));
    }

NS_END  // internal

// 6.2.1
// expr_standard_constants
//   function: the language's named constants: pi and e, from the literals
// of constants::of<T>; true and false; im, the imaginary unit, exact, and a
// NaN in each floating-point type (S271); and jm and km, the quaternions'
// units, exact, and a NaN in each floating-point type, real or complex
// (S275). Built once.
inline const expr_constant_table&
expr_standard_constants()
{
    static const expr_constant_table table =
        expr_constant_table()
            .define(internal::expr_constant_entry(
                        "pi",
                        3.141592653589793238462643383279502884197L))
            .define(internal::expr_constant_entry(
                        "e",
                        2.718281828459045235360287471352662497757L))
            .define(internal::expr_boolean_entry("true", true))
            .define(internal::expr_boolean_entry("false", false))
            .define(internal::expr_unit_entry())
            .define(internal::expr_quaternion_unit_entry("jm", false))
            .define(internal::expr_quaternion_unit_entry("km", true));

    return table;
}


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_EXPR_FUNCTION_HPP
