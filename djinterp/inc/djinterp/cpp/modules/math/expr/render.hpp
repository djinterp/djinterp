/*******************************************************************************
* djinterp [cpp/math]                                                 render.hpp
*
* The fold to text: a term written back as math text; the fourth layer's
* second half.
*   RENDERING is parse/expression's renderer over the operator table, so an
* operator is written as the table spells it, an infix one with a space on
* each side, a prefix one against its operand, a postfix one after it, and
* wrapped in brackets only where its operands would otherwise regroup:
* 2 * x ^ 2 + 1, -x ^ 2, (-x) ^ 2, a - (b - c), 2 ^ (-3), (n + 1)!. A word is
* spaced as a symbol is, and a prefix one before its operand: x > 0 and x < 1,
* not a = b, (a < b) = c. A chain was read as its links, so a < b <= c is
* written a < b and b <= c. A call is written f(a, b), as its function's name
* and its arguments, never as a prefix operator (chunk 12's sin x ^ 2): the
* renderer describes a call to parse/expression's renderer by its function, as
* the table describes none. Bars are written as the call they are, abs(x). A
* call of piecewise is written in its branches, as parse.hpp reads it: its
* arguments two by two, each pair a call of when, and an odd last one a call
* of otherwise, piecewise(when(x < 0, -x), otherwise(x)). A constant is its
* name. A literal is written as parse.hpp reads it, its elements in turn
* between square brackets, apart by commas, and a matrix literal's rows apart
* by semicolons: [x, 2 * y], [1, 2; 3, 4]; as one operand, it is wrapped in no
* brackets of its own, -[1, 2], [1, 2] ^ 2. The transpose is the table's
* postfix ', A', (a + b)', A ^ 2', A ^ (2'); as parse/expression's renderer
* brackets an operand of a looser operator, a transpose before ^ is written
* (A') ^ 2, and before ! (x')!, each reading back as itself. Indexing is
* written as parse.hpp reads it, its operand and then its indices between
* square brackets, apart by commas, A[1, 2], v[i + 1]: an operand that is a
* leaf, a call, a literal, an index or a postfix operator's as it is, A'[1],
* f(x)[2], [1, 2][1], and any other in round brackets, (A + B)[1]; as the
* tightest of operators, its text is wrapped in none, -v[1], v[1] ^ 2.
*   A NUMBER is written as its literal, where it has one that is an unsigned
* decimal number, and in brackets where its literal is anything else. One a
* fold made, with no literal, is written exactly where it is exact: an
* integer, 42, or a fraction, (1/3), in brackets where negative, (-5); the
* least dj_math_imax, whose magnitude no literal of the family has, as
* (-9223372036854775807 - 1) with 64-bit words. A float, double or long double
* is written as the shortest decimal that reads back to it in its type, 0.1,
* (-2.5), 1e+30. What has no literal of the language is written by a name: inf
* and nan, true and false, none, and an error as domain_error, pole_error,
* range_error, invalid_error or memory_error. A matrix a fold made is written
* as a literal of its elements, each as a number with no literal is:
* [1, (1/2); (-3), 0.25].
*   THE ROUND TRIP. expr_parse reads the text of any term it made back as
* that term, every literal kept: roadmap Phase 2's round trip. A number a
* fold made reads back as the same number, a term of its value: exactly,
* where it is exact and a rational of the family; to the same number in its
* type, where it is a float, a double or a long double; and a boolean as
* the constant true or false, whose value it is; a matrix as a literal,
* whose value is the matrix where its elements are exact, and of the same
* numbers in their type otherwise.
*   STEPS. Step 14 (2b) filled expr_render for numbers, names and the
* arithmetic, step 15 (2c) for calls and the factorial, step 16 (2d) for
* relations, logic and piecewise, step 19 (3a) for the literals and matrix
* values, and step 20 (3b) for indexing and the transpose.
* A term made by default, which is no term, is written as the empty string.
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/render.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  RENDERING
    ---------
    1.  Atoms
         1.  Whole numbers
         2.  Floating-point numbers
         3.  Numbers a fold made
         4.  expr_atom_text
    2.  Calls and literals
         1.  expr_render_signature
         2.  expr_branches
         3.  expr_literals
    3.  The entry
         1.  expr_render
*/

#ifndef DJINTERP_MATH_EXPR_RENDER_HPP
#define DJINTERP_MATH_EXPR_RENDER_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cmath>    // std::isnan, std::isinf, std::signbit
#include <cstddef>  // std::size_t
#include <ios>      // std::ios_base
#include <iomanip>  // std::setprecision
#include <limits>   // std::numeric_limits
#include <locale>   // std::locale
#include <sstream>  // std::ostringstream
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../../parse/expression/expression_ops.hpp"     // evaluate
#include "../../parse/expression/expression_render.hpp"  // render
#include "../math_common.hpp"                             // NS_MATH
#include "../value.hpp"                                   // value
#include "./matrix.hpp"                                   // expr_is_matrix
#include "./operator.hpp"                                 // expr_signature
#include "./term.hpp"                                     // expr_term
#include "./token.hpp"                                    // expr_read_decimal


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  RENDERING
//==============================================================================


// 1.1    Atoms
//------------------------------------------------------------------------------
NS_INTERNAL

    // 1.1.1
    // Whole numbers
    //   function: a whole number's decimal digits, a - before a negative
    // one's, by the ASCII digits, so that no locale changes them.
    inline
    std::string expr_digits(
        dj_math_imax _n
    )
    {
        dj_math_umax left   = ( (_n < 0)
                                   ? (dj_math_umax(0) - dj_math_umax(_n))
                                   : dj_math_umax(_n) );
        std::string digits;

        // the digits, the last first
        do
        {
            digits.insert(digits.begin(),
                          static_cast<char>('0' + (left % 10u)));
            left /= 10u;
        }
        while (left != 0);

        return ( (_n < 0) ? ("-" + digits) : digits );
    }

    // expr_signed_text
    //   function: a whole number's text that reads back as it: its digits,
    // a - before a negative one's; for the least dj_math_imax, whose
    // magnitude is past the family, the next one less 1.
    inline
    std::string expr_signed_text(
        dj_math_imax _n
    )
    {
        // the least: its magnitude no literal of the family
        if (_n == std::numeric_limits<dj_math_imax>::min())
        {
            return expr_digits(_n + 1) + " - 1";
        }

        return expr_digits(_n);
    }

    // 1.1.2
    // Floating-point numbers
    //   function: the shortest decimal text of a number in Type, finite and
    // not negative, that decimal.h reads back to it: at most max_digits10
    // significant digits, as std::ostream writes them in the classic
    // locale, which every reader of the text shares.
    template<typename Type>
    std::string expr_shortest(
        Type _x
    )
    {
        std::string text;

        // more digits, until they read back
        for (int digits = 1;
             digits <= std::numeric_limits<Type>::max_digits10;
             ++digits)
        {
            std::ostringstream out;
            Type               back = Type(0);

            out.imbue(std::locale::classic());
            out << std::setprecision(digits) << _x;
            text = out.str();

            if ( (expr_read_decimal(back, text)) &&
                 (back == _x) )
            {
                break;
            }
        }

        return text;
    }

    // expr_floating_text
    //   function: a float, double or long double as text: the shortest that
    // reads back, in brackets where negative, -0 among them; inf and nan by
    // name.
    template<typename Type>
    std::string expr_floating_text(
        Type _x
    )
    {
        // what has no literal
        if (std::isnan(_x))
        {
            return "nan";
        }

        if (std::isinf(_x))
        {
            return ( (_x < Type(0)) ? "(-inf)" : "inf" );
        }

        // a negative number, in brackets
        if (std::signbit(_x))
        {
            return "(-" + expr_shortest(-_x) + ")";
        }

        return expr_shortest(_x);
    }

    // 1.1.3
    // Numbers a fold made
    //   function: a number with no literal as text, as the banner says.

    // expr_error_text
    //   function: an error by name.
    inline
    std::string expr_error_text(
        const value& _x
    )
    {
        dj_math_value_error code = D_MATH_VALUE_ERROR_DOMAIN;

        static_cast<void>(_x.to(code));

        // by code
        switch (code)
        {
            case D_MATH_VALUE_ERROR_DOMAIN:  return "domain_error";
            case D_MATH_VALUE_ERROR_POLE:    return "pole_error";
            case D_MATH_VALUE_ERROR_RANGE:   return "range_error";
            case D_MATH_VALUE_ERROR_INVALID: return "invalid_error";
            case D_MATH_VALUE_ERROR_MEMORY:
            default:
                break;
        }

        return "memory_error";
    }

    // expr_boxed_text
    //   function: a boxed number as its kind writes it, in brackets where
    // negative.
    inline
    std::string expr_boxed_text(
        const value& _x
    )
    {
        const dj_math_value& held   = _x.c_value();
        const std::size_t   length = held.type->text(D_NULLPTR, 0, held);
        std::string         text(length + 1, '\0');

        static_cast<void>(held.type->text(&text[0], length + 1, held));
        text.resize(length);

        // a negative number, in brackets
        if ( (!text.empty()) &&
             (text[0] == '-') )
        {
            return "(" + text + ")";
        }

        return text;
    }

    // expr_number_text
    //   function: a number with no literal, by kind; a matrix as a literal
    // of its elements, each by kind.
    inline
    std::string expr_number_text(
        const value& _x
    )
    {
        dj_rational  exact = { 0, 1 };
        bool        truth = false;
        float       f     = 0.0f;
        double      d     = 0.0;
        long double ld    = 0.0L;

        // a matrix: its rows, each of its elements
        if (expr_is_matrix(_x))
        {
            const expr_shape         shape    = expr_shape_of(_x).value();
            const std::vector<value> elements = expr_matrix_elements(_x);
            std::string              text     = "[";

            for (std::size_t i = 0; i < elements.size(); ++i)
            {
                text += ( (i == 0)                      ? ""
                        : ((i % shape.columns) == 0)    ? "; "
                                                        : ", " );
                text += expr_number_text(elements[i]);
            }

            return text + "]";
        }

        // by kind: what has no literal, then the floating-point numbers
        switch (_x.kind())
        {
            case D_MATH_VALUE_NONE:
                return "none";

            case D_MATH_VALUE_BOOLEAN:
                static_cast<void>(_x.to(truth));

                return ( truth ? "true" : "false" );

            case D_MATH_VALUE_ERROR:
                return expr_error_text(_x);

            case D_MATH_VALUE_FLOAT:
                static_cast<void>(_x.to(f));

                return expr_floating_text(f);

            case D_MATH_VALUE_DOUBLE:
                static_cast<void>(_x.to(d));

                return expr_floating_text(d);

            case D_MATH_VALUE_LONG_DOUBLE:
                static_cast<void>(_x.to(ld));

                return expr_floating_text(ld);

            case D_MATH_VALUE_INTEGER:
            case D_MATH_VALUE_RATIONAL:
            case D_MATH_VALUE_BOXED:
            default:
                break;
        }

        // a boxed number: its kind's text
        if (!_x.to(exact))
        {
            return expr_boxed_text(_x);
        }

        // an integer, in brackets where negative
        if (exact.den == 1)
        {
            return ( (exact.num < 0) ? ("(" + expr_signed_text(exact.num) +
                                        ")")
                                     : expr_digits(exact.num) );
        }

        // a fraction, in brackets; the least numerator in its own
        if (exact.num == std::numeric_limits<dj_math_imax>::min())
        {
            return "((" + expr_signed_text(exact.num) + ")/" +
                   expr_digits(exact.den) + ")";
        }

        return "(" + expr_digits(exact.num) + "/" + expr_digits(exact.den) +
               ")";
    }

    // 1.1.4
    // expr_atom_text
    //   function: an atom as text: a variable's or a constant's name; a
    // number's literal, bare where it is one unsigned decimal number and in
    // brackets otherwise; a number with no literal by expr_number_text.
    inline
    std::string expr_atom_text(
        const expr_atom& _atom
    )
    {
        // a name
        if (_atom.kind != expr_atom_kind::number)
        {
            return _atom.text;
        }

        // a number with no literal
        if (_atom.text.empty())
        {
            return expr_number_text(_atom.number);
        }

        return ( (expr_unsigned_decimal(_atom.text)) ? _atom.text
                                                     : ("(" + _atom.text +
                                                        ")") );
    }

NS_END  // internal

// 1.2    Calls and literals
//------------------------------------------------------------------------------
NS_INTERNAL

    // 1.2.1
    // expr_render_signature
    //   class: the Signature expr_render gives parse/expression's renderer:
    // the operator table's, and for a call a descriptor of its own, whose
    // layout is the functional form and whose spelling is the function's
    // name. The renderer reads each descriptor once, at its node, and
    // copies its spelling before it asks for the next, so that one
    // descriptor serves every call in turn.
    class expr_render_signature
    {
    public:
        explicit expr_render_signature(
            const expr_signature& _table
        )
            : m_table(_table),
              m_call(0,
                     expr_precedence_postfix,
                     DAssocLeft,
                     DFixMatchfix,
                     "")
        {}

        // describe
        //   a call's descriptor, spelled as its function; any other
        // operator's, the table's.
        D_NODISCARD const operator_descriptor* describe(
            const expr_op& _op
        ) const
        {
            // a call: its function's name
            if (_op.kind == expr_op_kind::call)
            {
                m_call.spelling = _op.function.c_str();

                return &m_call;
            }

            return m_table.describe(_op);
        }

    private:
        const expr_signature&       m_table;
        mutable operator_descriptor m_call;
    };

    // 1.2.2
    // expr_branches
    //   function: _term with each call of piecewise written in its
    // branches: its arguments two by two, each pair the arguments of a call
    // of when, and an odd last one the argument of a call of otherwise; the
    // term the renderer writes, which parse.hpp reads back as _term.
    inline
    expr_term expr_branches(
        const expr_term& _term
    )
    {
        return ::djinterp::evaluate<expr_term, expr_op, expr_atom>(
            _term,
            // a leaf: itself
            [](const expr_atom& _atom) -> expr_term
            {
                return expr_leaf<expr_op, expr_atom>(_atom);
            },
            // an application: a call of piecewise in its branches, or
            // itself
            [](const expr_op&                _op,
               const std::vector<expr_term>& _operands) -> expr_term
            {
                std::vector<expr_term> branches;
                std::size_t            i = 0;

                // any application but a call of piecewise: itself
                if ( (_op.kind != expr_op_kind::call) ||
                     (_op.function != "piecewise") )
                {
                    return expr_apply<expr_op, expr_atom>(_op,
                                                          _operands);
                }

                // the pairs, each a call of when
                for (; i + 1 < _operands.size(); i += 2)
                {
                    branches.push_back(
                        expr_call("when",
                                  std::vector<expr_term>{ _operands[i],
                                                          _operands[i + 1] }));
                }

                // an odd last argument, a call of otherwise
                if (i < _operands.size())
                {
                    branches.push_back(
                        expr_call("otherwise",
                                  std::vector<expr_term>{ _operands[i] }));
                }

                return expr_apply<expr_op, expr_atom>(_op,
                                                      branches);
            });
    }

    // 1.2.3
    // expr_literals
    //   function: _term with each literal and each index written as an atom
    // of its text, which the renderer writes as it is: a vector's elements,
    // each rendered with _signature, between square brackets and apart by
    // commas; a matrix's rows apart by semicolons, each its row's elements,
    // or a term that is no row as one element; a row of no matrix as a
    // vector; and an index's operand, in round brackets where it is no
    // leaf, call, literal, index or postfix operator's, then its indices
    // between square brackets, apart by commas. Each node is visited once.
    inline
    expr_term expr_literals(
        const expr_term&             _term,
        const expr_render_signature& _signature
    );

    // expr_element_text
    //   function: one element of a literal as text: its literals written
    // first, then the whole rendered.
    inline
    std::string expr_element_text(
        const expr_term&             _element,
        const expr_render_signature& _signature
    )
    {
        return ::djinterp::render<expr_op, expr_atom>(
            expr_literals(_element,
                          _signature),
            _signature,
            &expr_atom_text);
    }

    // expr_row_text
    //   function: a row's elements as text, apart by commas.
    inline
    std::string expr_row_text(
        const std::vector<expr_term>& _elements,
        const expr_render_signature&  _signature
    )
    {
        std::string text;

        for (std::size_t i = 0; i < _elements.size(); ++i)
        {
            text += ( (i == 0) ? "" : ", " );
            text += expr_element_text(_elements[i],
                                      _signature);
        }

        return text;
    }

    // expr_bare_operand
    //   function: whether an index's operand is written as it is: a leaf, a
    // call, a literal, an index, or a postfix operator's, the factorial's or
    // the transpose's, each of which reads back as one operand before the
    // indices; any other is wrapped in round brackets.
    inline
    bool expr_bare_operand(
        const expr_term& _operand
    )
    {
        // a leaf
        if (!expr_is_application(_operand))
        {
            return true;
        }

        const expr_op_kind kind = expr_op_of(_operand).kind;

        return ( (kind == expr_op_kind::call)      ||
                 (kind == expr_op_kind::vector)    ||
                 (kind == expr_op_kind::matrix)    ||
                 (kind == expr_op_kind::row)       ||
                 (kind == expr_op_kind::index)     ||
                 (kind == expr_op_kind::factorial) ||
                 (kind == expr_op_kind::transpose) );
    }

    inline
    expr_term expr_literals(
        const expr_term&             _term,
        const expr_render_signature& _signature
    )
    {
        // a leaf: itself
        if (!expr_is_application(_term))
        {
            return _term;
        }

        const expr_op&                op       = expr_op_of(_term);
        const std::vector<expr_term>& operands = expr_operands_of(_term);
        std::string                   text     = "[";

        // a vector, or a row of no matrix: its elements
        if ( (op.kind == expr_op_kind::vector) ||
             (op.kind == expr_op_kind::row) )
        {
            text += expr_row_text(operands,
                                  _signature);
        }
        // an index: its operand, then its indices
        else if ( (op.kind == expr_op_kind::index) &&
                  (!operands.empty()) )
        {
            const bool bare = expr_bare_operand(operands[0]);

            text = ( bare ? "" : "(" ) +
                   expr_element_text(operands[0],
                                     _signature) +
                   ( bare ? "[" : ")[" ) +
                   expr_row_text(std::vector<expr_term>(operands.begin() + 1,
                                                        operands.end()),
                                 _signature);
        }
        // a matrix: its rows
        else if (op.kind == expr_op_kind::matrix)
        {
            for (std::size_t r = 0; r < operands.size(); ++r)
            {
                const bool row = ( (expr_is_application(operands[r])) &&
                                   (expr_op_of(operands[r]).kind ==
                                        expr_op_kind::row) );

                text += ( (r == 0) ? "" : "; " );
                text += ( row ? expr_row_text(expr_operands_of(operands[r]),
                                              _signature)
                              : expr_element_text(operands[r],
                                                  _signature) );
            }
        }
        // any other application: itself, of its operands' literals written
        else
        {
            std::vector<expr_term> written;

            for (std::size_t i = 0; i < operands.size(); ++i)
            {
                written.push_back(expr_literals(operands[i],
                                                _signature));
            }

            return expr_apply<expr_op, expr_atom>(op,
                                                  written);
        }

        return expr_leaf<expr_op, expr_atom>(
            expr_atom(expr_atom_kind::variable,
                      value(),
                      text + "]"));
    }

NS_END  // internal

// 1.3    The entry
//------------------------------------------------------------------------------
// 1.3.1
// expr_render
/**
 * @brief A term as math text.
 *
 * @param[in] _term  the term.
 * @return the text, which expr_parse reads back as _term where expr_parse
 *         made _term; the empty string for a term made by default.
 */
D_NODISCARD inline std::string expr_render(
    const expr_term& _term
)
{
    // no term
    if (_term.empty())
    {
        return std::string();
    }

    const internal::expr_render_signature signature(
        expr_signature::standard());

    return ::djinterp::render<expr_op, expr_atom>(
        internal::expr_literals(internal::expr_branches(_term),
                                signature),
        signature,
        &internal::expr_atom_text);
}


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_EXPR_RENDER_HPP
