/*******************************************************************************
* djinterp [cpp][math]                                                 logic.hpp
*
* The relations and logic of the run-time expression language, in a
* floating-point type and as a value, and piecewise: the evaluators the
* operators = != < <= > >= and, or and not and the functions all_of, any_of
* and piecewise share, which evaluate.hpp's folds and the function table
* (function_table.hpp) apply.
*   THE RELATIONS AND LOGIC are = != < <= > >= and and, or and not. As a
* value a relation is a boolean: of two numbers, by value.h's exact
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
*   ONE HEADER FOR THEM. Until chunk 48 this was section 4 of function.hpp,
* with the tables and the arithmetic (S401).
*   STEPS. Step 16 (2d) gave the relations' and logic's arithmetic, and
* piecewise, all_of and any_of; step 20 (3b) the relations of matrices; step
* 23 (3e), its first part, complex numbers' relations; step 24 (3f), its
* second part, in chunk 45, quaternions' (S275).
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/logic.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.14
*                                                            revised: 2026.11.14
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE RELATIONS AND LOGIC
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
*/

#ifndef DJINTERP_MATH_EXPR_LOGIC_HPP
#define DJINTERP_MATH_EXPR_LOGIC_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>  // std::size_t
// djinterp
#include "../math_common.hpp"     // NS_MATH
#include "../value.hpp"           // value
#include "./arithmetic.hpp"       // expr_nan_of, expr_is_nan_of
#include "./complex_kind.hpp"     // expr_is_complex
#include "./expr_common.hpp"      // expr_context
#include "./matrix_kind.hpp"      // expr_is_matrix
#include "./operator.hpp"         // expr_op_kind
#include "./quaternion_kind.hpp"  // expr_is_quaternion


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  THE RELATIONS AND LOGIC
//==============================================================================
// What the relations, logic and piecewise compute, as the banner says: in a
// floating-point type, its booleans 1 and 0; and as a value, booleans.


// 1.1    In a floating-point type
//------------------------------------------------------------------------------
NS_INTERNAL

    // 1.1.1
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

    // 1.1.2
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

    // 1.1.3
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

    // 1.1.4
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

// 1.2    As a value
//------------------------------------------------------------------------------
NS_INTERNAL

    // 1.2.1
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

    // 1.2.2
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

    // 1.2.3
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


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_EXPR_LOGIC_HPP
