/*******************************************************************************
* djinterp [cpp/math]                                               function.hpp
*
* The functions and named constants of the run-time expression language: the
* tables a fold looks a call's function and a constant's value up in, and the
* arithmetic beyond value.h's that both they and the operators need.
*   A FUNCTION is an entry of the function table: its name; the numbers of
* arguments it takes; an evaluator for each type a fold computes in, float,
* double and long double from the C core's kernels, and math::value exactly
* where it can be; and a slot for the rule of its partial derivatives, which
* the derivative (Phase 4, derivative.hpp) asks of it. A call names its
* function, and its function is the table's entry of that name.
*   A NAMED CONSTANT is an entry of the constant table: its name, and its
* value in each type: the 40-digit literal of constants::of<T>
* (constants.hpp), rounded to each type as of<T> rounds it, and as a value
* the long double nearest it. constants.hpp itself is not included: from
* C++14 it brings math/expression.hpp, whose names x, y, z, t, r, rho, theta
* and phi would stand in djinterp::math beside this language's.
*   THE TABLES are one kind, expr_table, of entries kept by name. The
* standard ones are the language's own; a caller may build others.
*   THE STANDARD FUNCTIONS are the C core's: sqrt, cbrt, sin, cos, tan, asin,
* acos, atan, atan2, sinh, cosh, tanh, exp, exp2, log, log2, log10, pow,
* hypot, fmod, remainder, floor, ceil, round and trunc, each of the number of
* arguments C gives it; the language's abs and factorial; and from the
* vocabulary of math/function/function.hpp, piecewise, all_of and any_of,
* each of any number of arguments from 1; and linear algebra's, det, inv,
* trace, norm and eig of one argument and dot, cross and solve of two
* (S256). log is the natural logarithm, as in C (gate G5). The constants are
* pi and e, as G5 says, and the booleans true and false.
*   IN A TYPE, a function is the C core's plain form in it, the C library's,
* as ^ is (S194): fast, and within an ulp of the _c forms (gate G6); abs is
* |x|. factorial(n) is n! for a whole n >= 0, the product rounded to the
* type as it is made, an infinity past the type's range; a NaN for any other
* n, as the C core has no gamma function.
*   AS A VALUE, a function is exact wherever its value is a rational number
* of the family: abs, floor, ceil, round, trunc, fmod and remainder always;
* sqrt and cbrt where the argument's terms are whole squares or cubes, hypot
* where the sum of the squares is a rational square, pow as ^ is; factorial
* for a whole n >= 0; exp2 at a whole number, log2 and log10 at a power of
* 2 or 10; and the others at the one point each where their value is
* rational: sin, tan, asin, atan, sinh and tanh at 0, which is 0; cos, cosh
* and exp at 0, which is 1; acos and log at 1, which is 0; atan2(0, x) for
* x > 0, which is 0. Nowhere else is any of them rational (Lindemann and
* Weierstrass), so nowhere else is a value lost. An exact argument outside
* a function's domain is a domain error: the root or the logarithm of a
* negative number, asin or acos past 1, factorial of a negative number or a
* fraction, fmod or remainder by 0, atan2(0, 0); the logarithm of 0 is a
* pole. Elsewhere a function's value is the C core's in the kind the tower
* gives: the widest of the inexact arguments' kinds, a long double where
* all are exact, as ^'s is. Errors pass, the first; a boolean or none is a
* domain error; every result carries its arguments' flags.
*   THE ARITHMETIC of ^ and mod, which value.h does not have, is here too,
* beside the functions': evaluate.hpp's folds apply it. A ^ n of a matrix is
* the matrix kind's power, a square matrix to a whole n, a negative one its
* inverse's (matrix.hpp, S248, S259), and so is pow(A, n); a matrix in mod,
* or an argument of any other function but a value of piecewise's and an
* argument of linear algebra's, is a domain error.
*   LINEAR ALGEBRA'S FUNCTIONS are matrix.hpp's as values: det, inv, trace,
* norm, eig, dot, cross and solve of matrices, exact for exact elements, a
* number taken as a scalar; and in a type, where every value is a number, a
* scalar's, as in Julia: det, trace and eig of x are x, inv(x) 1 / x,
* norm(x) |x|, dot(x, y) x y, solve(a, b) b / a, and cross a NaN, as it
* takes no number.
*   THE RELATIONS AND LOGIC, = != < <= > >= and and, or and not, are here as
* well. As a value a relation is a boolean: of two numbers, by value.h's exact
* order, none holding of a NaN but !=; of two booleans, = and != by their
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
* matrices; step 21 (3c) the functions of linear algebra; step 22 (4a) brings
* the rules of the derivatives. A function that binds a variable of its own,
* as sum(f, k, a, b) does, is step 24's (4c) to shape.
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/function.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.26
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
         3.  expr_fn_in
    3.  As a value
         1.  Exact powers and roots
         2.  Exact integers and remainders
         3.  Exact functions
         4.  In the tower's kind
         5.  expr_value_pow, expr_value_mod, expr_value_fn
4.  THE RELATIONS AND LOGIC
    -----------------------
    1.  In a floating-point type
         1.  expr_truth_in
         2.  expr_relation_in
         3.  expr_all_in, expr_any_in, expr_not_in
         4.  expr_piecewise_in
    2.  As a value
         1.  expr_relation_value
         2.  expr_all_value, expr_any_value, expr_not_value
         3.  expr_piecewise_value
5.  LINEAR ALGEBRA
    --------------
    1.  In a floating-point type
         1.  expr_itself_in, expr_reciprocal_in, expr_magnitude_in,
             expr_product_in, expr_quotient_in, expr_no_scalar_in
    2.  As a value
         1.  expr_det_value, expr_inv_value, expr_trace_value,
             expr_norm_value, expr_eig_value, expr_dot_value,
             expr_cross_value, expr_solve_value
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
#include <cmath>    // std::fabs, std::isinf
#include <cstddef>  // std::size_t
#include <limits>   // std::numeric_limits
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../../../../c/modules/math/elementary.h"       // d_math_sqrt, _sin, _cos, ...
#if (defined(__cplusplus))
    #include "../../../../../re_std/cpp/cfloat/cfloat.hpp"
#endif
#include "../../../../c/modules/math/exponential.h"      // d_math_exp, _exp2, _log, ...
#include "../../../../c/modules/math/hyperbolic.h"       // d_math_sinh, _cosh, _tanh
#include "../../../../c/modules/math/nearest_integer.h"  // d_math_floor, _ceil, ...
#include "../../../../c/modules/math/power.h"            // d_math_pow, _cbrt, _hypot
#include "../../../../c/modules/math/remainder.h"        // d_math_fmod, _remainder
#include "../../../../c/modules/math/tangent.h"          // d_math_tan, _atan, _atan2
#include "../math_common.hpp"              // NS_MATH
#include "../value.hpp"                    // value
#include "./expr_common.hpp"               // expr_result
#include "./matrix.hpp"                    // expr_is_matrix, the powers
#include "./term.hpp"                      // expr_term


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
// of that type, the first at _arguments. The C core's kernels are reached
// through functions of the table's own, never by pointer to a kernel, which
// is forced inline outside testing builds (S165).
template<typename Type>
using expr_evaluator = Type (*)(const Type* _arguments,
                                std::size_t _count);

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
// evaluators in float, double, long double and math::value; and the rule of
// its partial derivatives. A slot that is null is one the function has not.
// Each standard function has its four evaluators, which the folds call,
// and its rule from step 22 (4a).
struct expr_function
{
    std::string                 name;
    std::size_t                 min_arity;
    std::size_t                 max_arity;
    expr_evaluator<float>       evaluate_float;
    expr_evaluator<double>      evaluate_double;
    expr_evaluator<long double> evaluate_long_double;
    expr_evaluator<value>       evaluate_value;
    expr_partial                partial;

    // no name, no argument, every slot null.
    expr_function()
        : name(),
          min_arity(0),
          max_arity(0),
          evaluate_float(D_NULLPTR),
          evaluate_double(D_NULLPTR),
          evaluate_long_double(D_NULLPTR),
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
          evaluate_value(D_NULLPTR),
          partial(D_NULLPTR)
    {}

    // takes
    //   whether the function takes _count arguments.
    D_NODISCARD bool takes(
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
// double, long double and math::value.
struct expr_named_constant
{
    std::string name;
    float       as_float;
    double      as_double;
    long double as_long_double;
    value       as_value;

    // no name; 0 in each type, and none as a value.
    expr_named_constant()
        : name(),
          as_float(0.0f),
          as_double(0.0),
          as_long_double(0.0L),
          as_value()
    {}

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
          as_value(_as_value)
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
    expr_table& define(
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
    D_NODISCARD const Entry* find(
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

    D_NODISCARD std::size_t size() const D_NOEXCEPT
    {
        return m_entries.size();
    }

    D_NODISCARD bool empty() const D_NOEXCEPT
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
    //   enum: each operation of this section: the standard functions, and
    // the floored remainder of the operator mod, which no function is.
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
        mod       = 28
    };

NS_END  // internal

// 3.2    In a floating-point type
//------------------------------------------------------------------------------
NS_INTERNAL

    // 3.2.1
    // A value in a type
    //   function: a value's nearest Type; a boolean's 1 or 0, as true's and
    // false's; a NaN for an error or none, which have neither.
    template<typename Type>
    Type expr_value_in(
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
    Type expr_pow_in(
        Type _base,
        Type _exponent
    )
    {
        return d_math_pow(_base, _exponent);
    }

    template<typename Type>
    Type expr_mod_in(
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
    Type expr_factorial_in(
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
    // expr_fn_in
    //   function: an operation in Type, of the arguments it takes, the first
    // at _x.
    template<typename Type>
    Type expr_fn_in(
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
            case expr_fn::mod:
            default:
                break;
        }

        return expr_mod_in(_x[0], _x[1]);
    }

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
    inline
    bool expr_power_capped(
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
    inline
    bool expr_whole_root(
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
    inline
    bool expr_ratio_root(
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
    inline
    value expr_whole_power(
        const value& _base,
        dj_math_imax _n
    )
    {
        dj_math_umax left = ( (_n < 0)
                                 ? (dj_math_umax(0) - dj_math_umax(_n))
                                 : dj_math_umax(_n) );
        value       power(1);
        value       square = _base;

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
    //   function: whether _base ^ (p / q), for an exact base and an exact
    // rational exponent, is a domain error, for a negative base, or exact,
    // where the base's terms are whole q-th powers; and if so its value, in
    // _out. Where it is neither, its value is the tower's kind's.
    inline
    bool expr_ratio_power(
        const value&       _base,
        const dj_rational& _exponent,
        value&             _out
    )
    {
        dj_rational  terms    = { 0, 1 };
        dj_math_imax num_root = 0;
        dj_math_imax den_root = 1;

        // a negative base: no real power
        if (_base < value(0))
        {
            _out = value(D_MATH_VALUE_ERROR_DOMAIN);

            return true;
        }

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

    // expr_power_of
    //   function: whether _n >= 1 is a whole power of _base > 1, and if so
    // the exponent, in _k: _n divided by _base while it divides, then
    // compared with 1, the power 0.
    inline
    bool expr_power_of(
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
    inline
    dj_math_imax expr_whole_of(
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
    inline
    bool expr_exact_rest(
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
    inline
    value expr_exact_factorial(
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
    inline
    bool expr_exact_at(
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
    inline
    bool expr_exact_log(
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
    inline
    bool expr_exact_one(
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
    inline
    bool expr_exact_two(
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
    inline
    dj_math_value_kind expr_inexact_kind(
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
    value expr_tower_as(
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
        dj_math_value  held = result.detach();

        held.flags |= flags;

        return value::adopt(held);
    }

    inline
    value expr_tower(
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
    // (matrix.hpp), and in any other operation here a domain error.

    // expr_value_failed
    //   function: whether an argument is an error, which passes, the first,
    // or no number, a domain error; and if so that, in _out.
    inline
    bool expr_value_failed(
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

    inline
    value expr_value_pow(
        const value& _a,
        const value& _b
    )
    {
        const value x[2]     = { _a, _b };
        dj_math_imax whole    = 0;
        dj_rational  exponent = { 0, 1 };
        value       exact;

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

    inline
    value expr_value_fn(
        expr_fn      _fn,
        const value* _x,
        std::size_t  _count
    )
    {
        dj_rational one   = { 0, 1 };
        bool       exact = true;
        value      answer;

        // |x| is value.h's, and ^'s power pow's
        if (_fn == expr_fn::abs)
        {
            return abs(_x[0]);
        }

        if (_fn == expr_fn::pow)
        {
            return expr_value_pow(_x[0], _x[1]);
        }

        // an error, or what is no number
        if (expr_value_failed(_x, _count, answer))
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

    inline
    value expr_value_mod(
        const value& _a,
        const value& _b
    )
    {
        const value x[2] = { _a, _b };

        return expr_value_fn(expr_fn::mod, x, 2);
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
    Type expr_truth_in(
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
    Type expr_relation_in(
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
    Type expr_all_in(
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
            if (std::isnan(_x[i]))
            {
                return std::numeric_limits<Type>::quiet_NaN();
            }
        }

        return Type(1);
    }

    template<typename Type>
    Type expr_any_in(
        const Type* _x,
        std::size_t _count
    )
    {
        // a true truth: true, whatever the others are
        for (std::size_t i = 0; i < _count; ++i)
        {
            if ( (_x[i] != Type(0)) &&
                 (!std::isnan(_x[i])) )
            {
                return Type(1);
            }
        }

        // an unknown one: unknown
        for (std::size_t i = 0; i < _count; ++i)
        {
            if (std::isnan(_x[i]))
            {
                return std::numeric_limits<Type>::quiet_NaN();
            }
        }

        return Type(0);
    }

    template<typename Type>
    Type expr_not_in(
        Type _x
    )
    {
        // unknown, and known
        if (std::isnan(_x))
        {
            return _x;
        }

        return expr_truth_in<Type>(_x == Type(0));
    }

    // 4.1.4
    // expr_piecewise_in
    //   function: piecewise of _count arguments in Type, the first at _x: the
    // value of the first branch whose condition is true, a NaN where a
    // condition met first is unknown, or where none holds; an odd last
    // argument the otherwise.
    template<typename Type>
    Type expr_piecewise_in(
        const Type* _x,
        std::size_t _count
    )
    {
        std::size_t i = 0;

        // each branch in turn, by its condition
        for (; i + 1 < _count; i += 2)
        {
            if (std::isnan(_x[i]))
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

        return std::numeric_limits<Type>::quiet_NaN();
    }

NS_END  // internal

// 4.2    As a value
//------------------------------------------------------------------------------
NS_INTERNAL

    // 4.2.1
    // expr_relation_value
    //   function: the relation _kind of _a and _b as a value, a boolean: of
    // two numbers by value.h's exact order, unordered for a NaN, which none
    // holds of but !=; of two booleans, = and != by their truth; of two
    // matrices, = and != by the kind's comparison, equal where of one shape
    // and equal place by place (S242). An error passes, the first; any
    // other operands are a domain error.
    inline
    value expr_relation_value(
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

        // two numbers: their order
        if ( (_a.is_number()) &&
             (_b.is_number()) )
        {
            const dj_math_value_order order = _a.compare(_b);

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
    inline
    value expr_logic_value(
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

    inline
    value expr_all_value(
        const value* _x,
        std::size_t  _count
    )
    {
        return expr_logic_value(_x, _count, false);
    }

    inline
    value expr_any_value(
        const value* _x,
        std::size_t  _count
    )
    {
        return expr_logic_value(_x, _count, true);
    }

    inline
    value expr_not_value(
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
    inline
    value expr_piecewise_value(
        const value* _x,
        std::size_t  _count
    )
    {
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
// number, a scalar's, as in Julia: det, trace and eig of x are x, inv its
// reciprocal, norm its magnitude, dot(x, y) x y, solve(a, b) b / a, and
// cross a NaN, as it takes no number; as a value, matrix.hpp's.


// 5.1    In a floating-point type
//------------------------------------------------------------------------------
NS_INTERNAL

    // 5.1.1
    // expr_itself_in, expr_reciprocal_in, expr_magnitude_in,
    // expr_product_in, expr_quotient_in, expr_no_scalar_in
    //   functions: x; 1 / x; |x|; x y; the second over the first; a NaN.
    template<typename Type>
    Type expr_itself_in(
        const Type* _arguments,
        std::size_t _count
    )
    {
        static_cast<void>(_count);

        return _arguments[0];
    }

    template<typename Type>
    Type expr_reciprocal_in(
        const Type* _arguments,
        std::size_t _count
    )
    {
        static_cast<void>(_count);

        return (Type(1) / _arguments[0]);
    }

    template<typename Type>
    Type expr_magnitude_in(
        const Type* _arguments,
        std::size_t _count
    )
    {
        static_cast<void>(_count);

        return std::fabs(_arguments[0]);
    }

    template<typename Type>
    Type expr_product_in(
        const Type* _arguments,
        std::size_t _count
    )
    {
        static_cast<void>(_count);

        return (_arguments[0] * _arguments[1]);
    }

    template<typename Type>
    Type expr_quotient_in(
        const Type* _arguments,
        std::size_t _count
    )
    {
        static_cast<void>(_count);

        return (_arguments[1] / _arguments[0]);
    }

    template<typename Type>
    Type expr_no_scalar_in(
        const Type* _arguments,
        std::size_t _count
    )
    {
        static_cast<void>(_arguments);
        static_cast<void>(_count);

        return std::numeric_limits<Type>::quiet_NaN();
    }

NS_END  // internal

// 5.2    As a value
//------------------------------------------------------------------------------
NS_INTERNAL

    // 5.2.1
    // expr_det_value, expr_inv_value, expr_trace_value, expr_norm_value,
    // expr_eig_value, expr_dot_value, expr_cross_value, expr_solve_value
    //   functions: each function of linear algebra as a value, matrix.hpp's
    // (3.5): of a matrix, or of a number, a scalar; an error passes, the
    // first; a boolean or none, and a shape the function does not take, a
    // domain error.
    inline
    value expr_det_value(
        const value* _arguments,
        std::size_t  _count
    )
    {
        static_cast<void>(_count);

        return expr_matrix_det(_arguments[0]);
    }

    inline
    value expr_inv_value(
        const value* _arguments,
        std::size_t  _count
    )
    {
        static_cast<void>(_count);

        return expr_matrix_inv(_arguments[0]);
    }

    inline
    value expr_trace_value(
        const value* _arguments,
        std::size_t  _count
    )
    {
        static_cast<void>(_count);

        return expr_matrix_trace(_arguments[0]);
    }

    inline
    value expr_norm_value(
        const value* _arguments,
        std::size_t  _count
    )
    {
        static_cast<void>(_count);

        return expr_matrix_norm(_arguments[0]);
    }

    inline
    value expr_eig_value(
        const value* _arguments,
        std::size_t  _count
    )
    {
        static_cast<void>(_count);

        return expr_matrix_eig(_arguments[0]);
    }

    inline
    value expr_dot_value(
        const value* _arguments,
        std::size_t  _count
    )
    {
        static_cast<void>(_count);

        return expr_matrix_dot(_arguments[0],
                               _arguments[1]);
    }

    inline
    value expr_cross_value(
        const value* _arguments,
        std::size_t  _count
    )
    {
        static_cast<void>(_count);

        return expr_matrix_cross(_arguments[0],
                                 _arguments[1]);
    }

    inline
    value expr_solve_value(
        const value* _arguments,
        std::size_t  _count
    )
    {
        static_cast<void>(_count);

        return expr_matrix_solve(_arguments[0],
                                 _arguments[1]);
    }

NS_END  // internal


//==============================================================================
// 6.  THE STANDARD TABLES
//==============================================================================
// The language's own tables, each built once.


// 6.1    The functions
//------------------------------------------------------------------------------
NS_INTERNAL

    // expr_entry_in, expr_entry_value
    //   functions: the evaluators of the operation Fn, in a type and as a
    // value, as an expr_function holds them.
    template<expr_fn Fn,
             typename Type>
    Type expr_entry_in(
        const Type* _arguments,
        std::size_t _count
    )
    {
        static_cast<void>(_count);

        return expr_fn_in<Type>(Fn, _arguments);
    }

    template<expr_fn Fn>
    value expr_entry_value(
        const value* _arguments,
        std::size_t  _count
    )
    {
        return expr_value_fn(Fn, _arguments, _count);
    }

    // expr_entry
    //   function: the standard function of Fn: its name, the number of
    // arguments it takes, and its four evaluators; no rule of its partial
    // derivatives before step 22 (4a).
    template<expr_fn Fn>
    expr_function expr_entry(
        const char* _name,
        std::size_t _arity
    )
    {
        expr_function entry(_name, _arity, _arity);

        entry.evaluate_float       = &expr_entry_in<Fn, float>;
        entry.evaluate_double      = &expr_entry_in<Fn, double>;
        entry.evaluate_long_double = &expr_entry_in<Fn, long double>;
        entry.evaluate_value       = &expr_entry_value<Fn>;

        return entry;
    }

    // expr_entry_any
    //   function: a standard function of any number of arguments from 1, of
    // its four evaluators; no rule of its partial derivatives before step 22
    // (4a).
    inline
    expr_function expr_entry_any(
        const char*                 _name,
        expr_evaluator<float>       _in_float,
        expr_evaluator<double>      _in_double,
        expr_evaluator<long double> _in_long_double,
        expr_evaluator<value>       _as_value
    )
    {
        expr_function entry(_name, 1, expr_variadic);

        entry.evaluate_float       = _in_float;
        entry.evaluate_double      = _in_double;
        entry.evaluate_long_double = _in_long_double;
        entry.evaluate_value       = _as_value;

        return entry;
    }

    // expr_entry_algebra
    //   function: a function of linear algebra (S256), of its name, its
    // number of arguments and its four evaluators; no rule of its partial
    // derivatives before step 22 (4a).
    inline
    expr_function expr_entry_algebra(
        const char*                 _name,
        std::size_t                 _arity,
        expr_evaluator<float>       _in_float,
        expr_evaluator<double>      _in_double,
        expr_evaluator<long double> _in_long_double,
        expr_evaluator<value>       _as_value
    )
    {
        expr_function entry(_name,
                            _arity,
                            _arity);

        entry.evaluate_float       = _in_float;
        entry.evaluate_double      = _in_double;
        entry.evaluate_long_double = _in_long_double;
        entry.evaluate_value       = _as_value;

        return entry;
    }

    // expr_build_functions
    //   function: the standard function table, in the banner's order.
    inline
    expr_function_table expr_build_functions()
    {
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
             .define(expr_entry_any("piecewise",
                                    &expr_piecewise_in<float>,
                                    &expr_piecewise_in<double>,
                                    &expr_piecewise_in<long double>,
                                    &expr_piecewise_value))
             .define(expr_entry_any("all_of",
                                    &expr_all_in<float>,
                                    &expr_all_in<double>,
                                    &expr_all_in<long double>,
                                    &expr_all_value))
             .define(expr_entry_any("any_of",
                                    &expr_any_in<float>,
                                    &expr_any_in<double>,
                                    &expr_any_in<long double>,
                                    &expr_any_value))
             .define(expr_entry_algebra("det",
                                        1,
                                        &expr_itself_in<float>,
                                        &expr_itself_in<double>,
                                        &expr_itself_in<long double>,
                                        &expr_det_value))
             .define(expr_entry_algebra("inv",
                                        1,
                                        &expr_reciprocal_in<float>,
                                        &expr_reciprocal_in<double>,
                                        &expr_reciprocal_in<long double>,
                                        &expr_inv_value))
             .define(expr_entry_algebra("trace",
                                        1,
                                        &expr_itself_in<float>,
                                        &expr_itself_in<double>,
                                        &expr_itself_in<long double>,
                                        &expr_trace_value))
             .define(expr_entry_algebra("norm",
                                        1,
                                        &expr_magnitude_in<float>,
                                        &expr_magnitude_in<double>,
                                        &expr_magnitude_in<long double>,
                                        &expr_norm_value))
             .define(expr_entry_algebra("eig",
                                        1,
                                        &expr_itself_in<float>,
                                        &expr_itself_in<double>,
                                        &expr_itself_in<long double>,
                                        &expr_eig_value))
             .define(expr_entry_algebra("dot",
                                        2,
                                        &expr_product_in<float>,
                                        &expr_product_in<double>,
                                        &expr_product_in<long double>,
                                        &expr_dot_value))
             .define(expr_entry_algebra("cross",
                                        2,
                                        &expr_no_scalar_in<float>,
                                        &expr_no_scalar_in<double>,
                                        &expr_no_scalar_in<long double>,
                                        &expr_cross_value))
             .define(expr_entry_algebra("solve",
                                        2,
                                        &expr_quotient_in<float>,
                                        &expr_quotient_in<double>,
                                        &expr_quotient_in<long double>,
                                        &expr_solve_value));

        return table;
    }

NS_END  // internal

// 6.1.1
// expr_standard_functions
//   function: the language's functions, the banner's 38, built once.
inline
const expr_function_table& expr_standard_functions()
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
    inline
    expr_named_constant expr_constant_entry(
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

    // expr_boolean_entry
    //   function: a constant of the table from its truth: 1 or 0 in each
    // type, and as a value the boolean.
    inline
    expr_named_constant expr_boolean_entry(
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
// of constants::of<T>; and true and false. Built once.
inline
const expr_constant_table& expr_standard_constants()
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
            .define(internal::expr_boolean_entry("false", false));

    return table;
}


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_EXPR_FUNCTION_HPP
