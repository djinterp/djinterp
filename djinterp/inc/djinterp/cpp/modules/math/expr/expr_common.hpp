/*******************************************************************************
* djinterp [cpp/math]                                            expr_common.hpp
*
* The common part of math/expr/, the run-time expression language: the error
* every fallible step reports, and the switches of the syntax.
*   THE LANGUAGE stands on parse/expression/, the framework's shared
* expression core (roadmap gate G4), in six layers:
*     1. tokens         token.hpp
*     2. the grammar    operator.hpp, the operator table; parse.hpp, the
*                       parser layer
*     3. the term       term.hpp
*     4. the folds      evaluate.hpp, to numbers and values, with
*                       function.hpp's functions and constants; render.hpp,
*                       to text
*     5. the rewrites   substitute.hpp, simplify.hpp; Phase 4's
*                       derivative.hpp
*     6. the handle     expr.hpp: math::expr, which includes every header.
* Beside them stand Phase 3's matrix.hpp and Phase 4's numeric.hpp,
* series.hpp and antiderivative.hpp. Step 13 (2a) made every header, with
* its declarations; each step from 14 on fills one or two.
*   ERRORS. A step that can fail returns expr_result<T>, which is result<T,
* expr_error> (core/functional/result.hpp): its answer, or an expr_error,
* which says what went wrong, where in the text when there is a place, and in
* words. Arithmetic gives no expr_error: its errors are values, as
* math::value's are (a pole, a domain error, a NaN).
*   NOT YET. Until the step that fills it, a step that can fail answers
* expr_status::not_yet, its message naming that step; one that cannot fail
* gives the placeholder its contract names.
*   SYNTAX. Gate G5's two options, implied multiplication (2x) and bars for
* the absolute value (|x|), are off unless an expr_syntax turns them on.
*   LEVELS. C++11 and up, by the owner's ruling N1: below C++11 every header
* of math/expr/ is empty.
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/expr_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.24
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  ERRORS
    ------
    1.  The error
         1.  expr_status
         2.  expr_no_offset
         3.  expr_error
    2.  The answer of a step
         1.  expr_result
         2.  expr_not_yet
2.  SYNTAX
    ------
    1.  The switches
         1.  expr_syntax
3.  TYPES
    -----
    1.  The floating-point types
         1.  expr_floating
*/

#ifndef DJINTERP_MATH_EXPR_EXPR_COMMON_HPP
#define DJINTERP_MATH_EXPR_EXPR_COMMON_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>      // std::size_t
#include <string>       // std::string
#include <type_traits>  // std::is_same
// djinterp
#include "../../../functional/result.hpp"  // result, ok, err
#include "../math_common.hpp"                 // NS_MATH


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  ERRORS
//==============================================================================
// A step that can fail returns its answer or an expr_error. The statuses are
// those the steps' contracts name; each is marked with the step that first
// gives it, and a later step may add its own.


// 1.1    The error
//------------------------------------------------------------------------------
// 1.1.1
// expr_status
//   enum: what went wrong in a step of the language.
enum class expr_status
{
    not_yet            = 1,   // the step that fills this one has not come
                              // yet (2a; gone once Phase 4 is done)
    bad_character      = 2,   // a character no token begins with (2b)
    unexpected_token   = 3,   // a token where none of its kind may stand
                              // (2b)
    missing_operand    = 4,   // the text ends, or a bracket closes, where
                              // an operand must stand; a term made by
                              // default, with none (2b)
    unbalanced         = 5,   // a bracket never closed, or closed but never
                              // opened (2b)
    unbound_variable   = 6,   // a variable the bindings give no value (2b)
    unknown_name       = 7,   // an operator of no kind the language has
                              // (2b); a function or a constant neither
                              // table has (2c)
    wrong_arity        = 8,   // an operator applied to a number of
                              // operands it does not take (2b); a call to
                              // a number of arguments its function does
                              // not take (2c); a branch of piecewise, in
                              // the text, to a number it does not take
                              // (2d)
    bad_shape          = 9,   // a matrix whose rows differ in length, or a
                              // shape an operation does not take (3a)
    not_differentiable = 10,  // a function with no rule for its derivative
                              // (4a)
    no_convergence     = 11,  // a numeric method stopped at its limit
                              // without converging (4b, 4c)
    no_closed_form     = 12   // an antiderivative the table of elementary
                              // forms does not give (4d)
};

// 1.1.2
// expr_no_offset
//   constant: the offset of an error that has no place in a text.
D_CONSTEXPR_VAR std::size_t expr_no_offset = static_cast<std::size_t>(-1);

// 1.1.3
// expr_error
//   struct: what went wrong in a step: its status; the offset, in characters,
// of the place in the text where it went wrong, or expr_no_offset where it
// has none; and a message in words. Two errors are equal when all three are.
struct expr_error
{
    expr_status status;
    std::size_t offset;
    std::string message;

    // a not_yet error with no place and no message.
    expr_error()
        : status(expr_status::not_yet),
          offset(expr_no_offset),
          message()
    {}

    expr_error(
        expr_status        _status,
        std::size_t        _offset,
        const std::string& _message
    )
        : status(_status),
          offset(_offset),
          message(_message)
    {}
};

// operator==, operator!= (expr_error)
//   function: two errors are equal when their statuses, offsets and messages
// are.
inline
bool operator==(
    const expr_error& _a,
    const expr_error& _b
)
{
    return ( (_a.status == _b.status) &&
             (_a.offset == _b.offset) &&
             (_a.message == _b.message) );
}

inline
bool operator!=(
    const expr_error& _a,
    const expr_error& _b
)
{
    return !(_a == _b);
}

// 1.2    The answer of a step
//------------------------------------------------------------------------------
// 1.2.1
// expr_result
//   type: what a step that can fail returns: its answer, or an expr_error.
template<typename Type>
using expr_result = result<Type, expr_error>;

// 1.2.2
// expr_not_yet
//   function: the answer of a step not filled yet: the error not_yet, with
// no place, and a message naming the step, as "expr_parse: not yet (step 14,
// 2b)"; expr_not_yet_error is the error alone. Each stub of math/expr/ gives
// it, and each goes with its step.
NS_INTERNAL

    inline
    expr_error expr_not_yet_error(
        const char* _what
    )
    {
        return expr_error(expr_status::not_yet,
                          expr_no_offset,
                          _what);
    }

    template<typename Type>
    D_NODISCARD expr_result<Type> expr_not_yet(
        const char* _what
    )
    {
        return err<Type, expr_error>(expr_not_yet_error(_what));
    }

NS_END  // internal


//==============================================================================
// 2.  SYNTAX
//==============================================================================


// 2.1    The switches
//------------------------------------------------------------------------------
// 2.1.1
// expr_syntax
//   struct: the two options gate G5 leaves to the caller, each off unless
// set: implied multiplication, 2x read as 2 * x; and bars for the absolute
// value, |x| read as abs(x).
struct expr_syntax
{
    bool implied_multiplication;
    bool absolute_bars;

    // both options off.
    expr_syntax() D_NOEXCEPT
        : implied_multiplication(false),
          absolute_bars(false)
    {}
};


//==============================================================================
// 3.  TYPES
//==============================================================================


// 3.1    The floating-point types
//------------------------------------------------------------------------------
// 3.1.1
// expr_floating
//   trait: whether Type is one a typed fold computes in: float, double or
// long double, the C core's three. Each typed entry asserts it.
NS_INTERNAL

    template<typename Type>
    struct expr_floating
    {
        static const bool value =
            ( (std::is_same<Type, float>::value)  ||
              (std::is_same<Type, double>::value) ||
              (std::is_same<Type, long double>::value) );
    };

NS_END  // internal


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_EXPR_EXPR_COMMON_HPP
