/*******************************************************************************
* djinterp [cpp][math]                                        function_table.hpp
*
* The function and constant tables of the run-time expression language: the
* tables a fold looks a call's function and a constant's value up in, their
* entries, and the standard ones, the language's own.
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
* complex(x, y), real, imag, conj and arg (S271, complex_kind.hpp); from the
* vocabulary of math/function/function.hpp, piecewise, all_of and any_of, each
* of any number of arguments from 1; and linear algebra's, transpose, det,
* inv, trace, norm and eig of one argument and dot, cross and solve of two
* (S256, S350); and the quaternions' quat(w, x, y, z), parts of one argument
* and rotate of two (S275, quaternion_kind.hpp), and rotmat and rotquat of one
* and slerp of three (S391, rotation.hpp): 58 in all. log is the natural
* logarithm, as in C (gate G5). The constants are pi and e, as G5 says, and
* the booleans true and false; im, the imaginary unit, so that 3 + 4im is a
* complex number (S271); and jm and km, the quaternions' units, so that 1 +
* 2jm - km is a quaternion (S275).
*   WHERE THEY ARE COMPUTED. A standard function's evaluators in a type and
* as a value are arithmetic.hpp's, beside the operators' ^ and mod; the
* relations', logic's and piecewise's are logic.hpp's; linear algebra's and
* the quaternions' own are this header's, below, on matrix_function.hpp's
* and rotation.hpp's.
*   LINEAR ALGEBRA'S FUNCTIONS are matrix_function.hpp's as values: transpose,
* det, inv, trace, norm, eig, dot, cross and solve of matrices, of complex
* elements too, exact for exact elements, a number taken as a scalar:
* transpose(A) the transpose, where A' conjugates (S350); norm of the moduli,
* dot(a, b) the sum of conj(a_i) b_i, and eig every eigenvalue, complex where it
* is (S351, S352); inv(q) of a quaternion its inverse (S275). In a type, where
* every value is a number, a scalar's, as in Julia: transpose, det, trace and
* eig of x are x, inv(x) 1 / x, norm(x) |x|, dot(x, y) x y, solve(a, b) b / a,
* and cross a NaN, as it takes no number; in a complex type the same, dot(x, y)
* conj(x) y, and inv(z) 1 / z, a real 1 taken as itself (G.5, S351).
*   THE QUATERNIONS' OWN (quaternion_kind.hpp, step 24, 3f). quat(w, x, y,
* z) is the number of four real parts, exact where all four are, a complex
* number where they are exact and y and z are 0, and a real one where x is 0
* too (S275); parts(q) the vector [w, x, y, z], of a real or complex number
* too, its missing parts 0; and inv(q) its inverse, q* / |q|^2, exact for
* exact parts. rotate(q, v) is the 3-vector q v q^-1, of a vector of 1 by 3
* or 3 by 1 of real numbers, of its shape: exact where q and v are, and
* otherwise quat.h's accurate rotation (S360); q may be any number but an
* exact 0, which names no rotation, a real one rotating nothing. rotmat(q) is
* that rotation's 3 by 3 matrix, rotquat(R) the unit quaternion of a
* rotation's matrix, w >= 0, and slerp(p, q, t) the shorter arc between two
* rotations at a constant speed (S391, rotation.hpp). Linear algebra takes
* quaternions (S388, S390, matrix_function.hpp): det of one, or of a matrix
* of them, is a domain error (S277); eig of one its standard eigenvalue, w +
* |v|i, and of a matrix of them Zhang's standard right eigenvalues (S389);
* dot(p, q) conj(p) q; solve(q, B) q^-1 B, on the left.
*   ONE HEADER FOR EACH. Until chunk 48 the tables, the arithmetic and the
* logic were one header, function.hpp, of 4,363 lines, of the same name as
* the compile-time language's function/function.hpp; now the tables are
* this header's, the arithmetic arithmetic.hpp's and the relations and
* logic logic.hpp's, which this header includes (S400, S401).
*   STEPS. Step 15 (2c) filled the standard tables; step 16 (2d) gave them
* piecewise, all_of and any_of, and true and false; step 21 (3c) the
* functions of linear algebra; step 22 (3d) the names im, jm and km,
* answering not_yet; step 23 (3e), its first part, im's value, and complex,
* real, imag, conj and arg, and in its third, chunk 43, transpose and linear
* algebra's functions of complex numbers in each type (S350, S351); step 24
* (3f), its second part, in chunk 45, jm's and km's values, quat, parts and
* rotate (S275), and its third, in chunk 46, linear algebra's functions of
* quaternions, rotmat, rotquat and slerp (S386 to S391); step 25 (4a) brings
* the rules of the derivatives. A function that binds a variable of its own,
* as sum(f, k, a, b) does, is step 27's (4c) to shape.
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/function_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.11.14
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
3.  LINEAR ALGEBRA
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
4.  THE STANDARD TABLES
    -------------------
    1.  The functions
         1.  expr_standard_functions
    2.  The constants
         1.  expr_standard_constants
*/

#ifndef DJINTERP_MATH_EXPR_FUNCTION_TABLE_HPP
#define DJINTERP_MATH_EXPR_FUNCTION_TABLE_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cmath>    // std::fabs
#include <cstddef>  // std::size_t
#include <limits>   // std::numeric_limits
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../math_common.hpp"        // NS_MATH
#include "../value.hpp"              // value
#include "./arithmetic.hpp"          // expr_fn, expr_fn_in, expr_value_fn
#include "./complex_kind.hpp"        // expr_complex
#include "./expr_common.hpp"         // expr_result, expr_context
#include "./logic.hpp"               // expr_all_of_in, expr_piecewise_in,
                                     // ...
#include "./matrix_function.hpp"     // expr_matrix_det, _inv, _eig, ...
#include "./matrix_kind.hpp"         // expr_transpose_value
#include "./quaternion_kind.hpp"     // expr_quaternion, _inverse
#include "./rotation.hpp"            // parts, rotate, rotmat, rotquat,
                                     // slerp
#include "./term.hpp"                // expr_term


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
// 3.  LINEAR ALGEBRA
//==============================================================================
// The functions of linear algebra (S256): in a type, where every value is a
// number, a scalar's, as in Julia: transpose, det, trace and eig of x are
// x, inv its reciprocal, norm its magnitude, dot(x, y) conj(x) y, solve(a,
// b) b / a, and cross a NaN, as it takes no number; as a value,
// matrix_function.hpp's, of complex elements too (S350, S351).


// 3.1    In a floating-point type
//------------------------------------------------------------------------------
NS_INTERNAL

    // 3.1.1
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

// 3.2    As a value
//------------------------------------------------------------------------------
NS_INTERNAL

    // 3.2.1
    // expr_transpose_fn_value, expr_det_value, expr_inv_value,
    // expr_trace_value, expr_norm_value, expr_eig_value, expr_dot_value,
    // expr_cross_value, expr_solve_value
    //   functions: each function of linear algebra as a value,
    // matrix_kind.hpp's transpose (3.3) and matrix_function.hpp's: of a matrix,
    // or of a number, a scalar, transpose(A) the transpose, unconjugated
    // (S350); an error passes, the first; a boolean or none, and a shape the
    // function does not take, a domain error; eig of a real matrix with a
    // complex pair a domain error too where the context asks real results
    // (S340, S352).
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

// 3.3    The quaternions' own
//------------------------------------------------------------------------------
NS_INTERNAL

    // 3.3.1
    // expr_quat_in, expr_quat_complex, expr_quat_value
    //   functions: quat(w, x, y, z), the quaternion of four real parts
    // (S275): in a real type w where x, y and z are 0, of either sign, and a
    // NaN otherwise, as complex(x, y) is there (S332); in a complex type, of
    // real arguments, w + x i where y and z are 0, and a NaN in both parts
    // otherwise; as a value quaternion_kind.hpp's expr_quaternion.
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

    // 3.3.2
    // expr_parts_value, expr_rotate_value
    //   functions: parts(q), a number's four parts, the vector [w, x, y, z],
    // and rotate(q, v), a 3-vector rotated by q, q v q^-1 (S275), as values:
    // quaternion_kind.hpp's expr_quaternion_parts and expr_quaternion_rotate.
    // In a type, which holds no vector, each is a NaN, as cross is
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

    // 3.3.3
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
// 4.  THE STANDARD TABLES
//==============================================================================
// The language's own tables, each built once.


// 4.1    The functions
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

// 4.1.1
// expr_standard_functions
//   function: the language's functions, the banner's 58, built once.
inline const expr_function_table&
expr_standard_functions()
{
    static const expr_function_table table = internal::expr_build_functions();

    return table;
}

// 4.2    The constants
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

// 4.2.1
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


#endif  // DJINTERP_MATH_EXPR_FUNCTION_TABLE_HPP
