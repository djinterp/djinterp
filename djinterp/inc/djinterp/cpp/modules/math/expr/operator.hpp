/*******************************************************************************
* djinterp [cpp/math]                                               operator.hpp
*
* Operators: the ids of every operator the run-time expression language has,
* and its operator table, the first half of the second layer.
*   AN OPERATOR is an expr_op: its kind and, for a call, the name of the
* function called. A function is an operator of the term, so that each fold
* and rewrite, and parse/expression's renderer, takes a call as it takes a
* sum. The kinds are those of Phases 2 to 4: arithmetic, the postfix
* factorial and transpose, relations, logic, calls, indexing, and vector and
* matrix literals.
*   THE TABLE, expr_signature, describes each operator as parse/expression's
* operator_signature does, through describe: its arity, precedence,
* associativity, fixity and spelling, by gate G5's table. It is the Signature
* that parse/expression's parser and renderer read. Its precedences are G5's
* levels, loosest first, ten apart: or, and, not, the relations, + and -,
* * / and mod, the unary - and +, ^, and the postfix !; and the postfix ',
* the transpose, between the unary - and ^ (S252). Logic binds below the
* relations, so that x > 0 and x < 1 is (x > 0) and (x < 1), as in C, Python
* and Mathematica, and not x = y is not (x = y), as in Python and Mathematica
* (S213). The relations are non-associative: the parser layer reads a chain of
* them, a < b <= c, as the conjunction of its links (parse.hpp). and and or
* are left-associative, and not is a prefix operator. The arithmetic is
* left-associative but ^, which is right-associative, so that 2 ^ 3 ^ 2 is
* 2 ^ 9, and the unary minus binds below ^, so that -x ^ 2 is -(x ^ 2). The
* factorial is postfix, tighter than ^, so that 2 ^ 3! is 2 ^ 6 and n!! is
* (n!)!. The transpose is postfix, below ^ and above the unary minus, so that
* A ^ 2' is (A ^ 2)' and A' ^ 2 is (A') ^ 2, as MATLAB reads both, -A' is
* -(A'), and A'' is (A')'.
*   A CALL is no operator of the table's: the parser layer reads a call as
* one operand, f(a, b), before the shared parser climbs (parse.hpp), and the
* renderer writes it as its function's name and its arguments (render.hpp).
* Nor is a literal, from step 19 (3a): the parser layer reads [a, b, c] and
* [a, b; c, d] as one operand, the term vector of its elements or matrix of
* its rows, each row the term row of its elements, and the renderer writes
* each back between square brackets.
*   INDEXING is no operator of the table's either, as its term has the
* indexed operand and one index or two, index(A, i, j), where an operator
* of the table has its arity: the parser layer reads A[i, j] as a postfix
* operator tighter than every other, its indices a group read before the
* climb, and the renderer writes it back as the operand and its indices
* between square brackets (parse.hpp, render.hpp).
*   STEPS. Step 14 (2b) filled the table's arithmetic: + - * / mod ^ and the
* unary - and +; step 15 (2c) its factorial; step 16 (2d) its relations and
* logic; step 20 (3b) the transpose. describe gives null for a call, a
* literal's kinds and indexing, which the parser layer and the renderer
* read and write themselves.
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/operator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  OPERATORS
    ---------
    1.  The operator
         1.  expr_op_kind
         2.  expr_op_kind_count
         3.  expr_op
    2.  The operator table
         1.  expr_precedence_*
         2.  expr_signature
*/

#ifndef DJINTERP_MATH_EXPR_OPERATOR_HPP
#define DJINTERP_MATH_EXPR_OPERATOR_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string
// djinterp
#include "../../parse/expression/expression.hpp"  // operator_descriptor
#include "../math_common.hpp"                      // NS_MATH


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  OPERATORS
//==============================================================================


// 1.1    The operator
//------------------------------------------------------------------------------
// 1.1.1
// expr_op_kind
//   enum: what an operator does. Each is marked with the step that gives it
// a place in the table, and its spelling in the text.
enum class expr_op_kind
{
    none        = 0,    // made by default; in no term
    add         = 1,    // a + b                                   (2b)
    sub         = 2,    // a - b                                   (2b)
    mul         = 3,    // a * b                                   (2b)
    div         = 4,    // a / b                                   (2b)
    mod         = 5,    // a mod b                                 (2b)
    pow         = 6,    // a ^ b                                   (2b)
    neg         = 7,    // -a                                      (2b)
    pos         = 8,    // +a                                      (2b)
    factorial   = 9,    // n!                                      (2c)
    transpose   = 10,   // A'                                      (3b)
    eq          = 11,   // a = b                                   (2d)
    ne          = 12,   // a != b                                  (2d)
    lt          = 13,   // a < b                                   (2d)
    le          = 14,   // a <= b                                  (2d)
    gt          = 15,   // a > b                                   (2d)
    ge          = 16,   // a >= b                                  (2d)
    logical_and = 17,   // p and q                                 (2d)
    logical_or  = 18,   // p or q                                  (2d)
    logical_not = 19,   // not p                                   (2d)
    call        = 20,   // f(a, b): a function of the table        (2c)
    index       = 21,   // v[i], A[i, j]                           (3b)
    vector      = 22,   // [a, b, c]                               (3a)
    matrix      = 23,   // [a, b; c, d], of its rows               (3a)
    row         = 24    // a row of a matrix literal               (3a)
};

// 1.1.2
// expr_op_kind_count
//   constant: the number of kinds, none among them, so that a table indexed
// by kind has a place for each.
D_CONSTEXPR_VAR std::size_t expr_op_kind_count = 25;

// 1.1.3
// expr_op
//   struct: an operator of a term: its kind, and for a call the name of its
// function, empty for any other kind. An operator made by default is of
// kind none; one made from a kind alone, implicitly, is that kind, so that
// expr_op_kind::add serves where an expr_op is asked for. Two operators are
// equal when their kinds and functions are.
struct expr_op
{
    expr_op_kind kind;
    std::string  function;

    // kind none.
    expr_op()
        : kind(expr_op_kind::none),
          function()
    {}

    // the kind, with no function.
    expr_op(
        expr_op_kind _kind
    )
        : kind(_kind),
          function()
    {}

    // call
    //   a call of the function named.
    static
    expr_op call(
        const std::string& _function
    )
    {
        expr_op op(expr_op_kind::call);
        op.function = _function;

        return op;
    }
};

// operator==, operator!= (expr_op)
//   function: two operators are equal when their kinds and functions are.
inline
bool operator==(
    const expr_op& _a,
    const expr_op& _b
)
{
    return ( (_a.kind == _b.kind) &&
             (_a.function == _b.function) );
}

inline
bool operator!=(
    const expr_op& _a,
    const expr_op& _b
)
{
    return !(_a == _b);
}

// 1.2    The operator table
//------------------------------------------------------------------------------
// 1.2.1
// expr_precedence_*
//   constants: gate G5's levels, loosest first, as the table's precedences:
// an operator of a higher one binds tighter. Each is marked with the step
// whose operators stand on it. Logic stands below the relations (S213).
D_CONSTEXPR_VAR int expr_precedence_or             = 10;  // or (2d)
D_CONSTEXPR_VAR int expr_precedence_and            = 20;  // and (2d)
D_CONSTEXPR_VAR int expr_precedence_not            = 30;  // not (2d)
D_CONSTEXPR_VAR int expr_precedence_relation       = 40;  // = != < <= > >=
                                                          // (2d)
D_CONSTEXPR_VAR int expr_precedence_additive       = 50;  // + - (2b)
D_CONSTEXPR_VAR int expr_precedence_multiplicative = 60;  // * / mod (2b)
D_CONSTEXPR_VAR int expr_precedence_unary          = 70;  // unary - + (2b)
D_CONSTEXPR_VAR int expr_precedence_transpose      = 75;  // ' (3b, S252)
D_CONSTEXPR_VAR int expr_precedence_power          = 80;  // ^ (2b)
D_CONSTEXPR_VAR int expr_precedence_postfix        = 90;  // ! (2c)
D_CONSTEXPR_VAR int expr_precedence_index          = 100; // A[i, j] (3b),
                                                          // the parser
                                                          // layer's alone

// 1.2.2
// expr_signature
//   class: the operator table: for each kind, its descriptor
// (parse/expression's operator_descriptor) where the table describes it. It
// is the Signature parse/expression's parser and renderer read, through
// describe. define describes a kind, or describes it anew; standard() is
// the language's own, built once.
class expr_signature
{
public:
    // a table that describes nothing: every flag value-initialized, false.
    expr_signature()
        : m_descriptors(),
          m_described()
    {}

    // define
    //   describes _kind by _descriptor, in place of any descriptor it had; a
    // kind out of range is left undescribed. Returns the table, for the
    // next.
    expr_signature& define(
        expr_op_kind               _kind,
        const operator_descriptor& _descriptor
    )
    {
        const std::size_t index = static_cast<std::size_t>(_kind);

        // a kind in range
        if (index < expr_op_kind_count)
        {
            m_descriptors[index] = _descriptor;
            m_described[index]   = true;
        }

        return *this;
    }

    /**
     * @brief The descriptor of an operator, where the table has one.
     *
     * @note The table describes no call: a call is read by the parser layer
     *       and written by the renderer, each by its function's name.
     *
     * @param[in] _op  the operator.
     * @return the descriptor of _op's kind, or null where the table has
     *         none.
     */
    D_NODISCARD const operator_descriptor* describe(
        const expr_op& _op
    ) const
    {
        const std::size_t index = static_cast<std::size_t>(_op.kind);

        // a kind out of range, or not described
        if ( (index >= expr_op_kind_count) ||
             (!m_described[index]) )
        {
            return D_NULLPTR;
        }

        return &m_descriptors[index];
    }

    // has
    //   whether the table describes the operator.
    D_NODISCARD bool has(
        const expr_op& _op
    ) const
    {
        return (describe(_op) != D_NULLPTR);
    }

    // standard
    //   the language's table, built once: gate G5's arithmetic, + - * / mod
    // ^ and the unary - and +, from step 14 (2b); the factorial, from step
    // 15 (2c); the relations and logic, from step 16 (2d); the transpose,
    // from step 20 (3b).
    static
    const expr_signature& standard()
    {
        static const expr_signature table = build_standard();

        return table;
    }

private:
    // build_standard
    //   G5's arithmetic: each binary operator infix and left-associative but
    // ^, + and * commutative and associative; the unary - and + prefix; the
    // factorial postfix, above ^, and the transpose postfix, below it,
    // each left-associative, so that n!! is (n!)! and A'' is (A')'; the
    // relations infix and non-associative, = and != commutative; and and or
    // infix and left-associative, commutative, associative and idempotent;
    // not prefix.
    static
    expr_signature build_standard()
    {
        expr_signature table;

        table.define(expr_op_kind::logical_or,
                     operator_descriptor(2,
                                         expr_precedence_or,
                                         DAssocLeft,
                                         DFixInfix,
                                         "or",
                                         true,
                                         true,
                                         true));
        table.define(expr_op_kind::logical_and,
                     operator_descriptor(2,
                                         expr_precedence_and,
                                         DAssocLeft,
                                         DFixInfix,
                                         "and",
                                         true,
                                         true,
                                         true));
        table.define(expr_op_kind::logical_not,
                     operator_descriptor(1,
                                         expr_precedence_not,
                                         DAssocRight,
                                         DFixPrefix,
                                         "not"));
        table.define(expr_op_kind::eq,
                     operator_descriptor(2,
                                         expr_precedence_relation,
                                         DAssocNone,
                                         DFixInfix,
                                         "=",
                                         true));
        table.define(expr_op_kind::ne,
                     operator_descriptor(2,
                                         expr_precedence_relation,
                                         DAssocNone,
                                         DFixInfix,
                                         "!=",
                                         true));
        table.define(expr_op_kind::lt,
                     operator_descriptor(2,
                                         expr_precedence_relation,
                                         DAssocNone,
                                         DFixInfix,
                                         "<"));
        table.define(expr_op_kind::le,
                     operator_descriptor(2,
                                         expr_precedence_relation,
                                         DAssocNone,
                                         DFixInfix,
                                         "<="));
        table.define(expr_op_kind::gt,
                     operator_descriptor(2,
                                         expr_precedence_relation,
                                         DAssocNone,
                                         DFixInfix,
                                         ">"));
        table.define(expr_op_kind::ge,
                     operator_descriptor(2,
                                         expr_precedence_relation,
                                         DAssocNone,
                                         DFixInfix,
                                         ">="));
        table.define(expr_op_kind::add,
                     operator_descriptor(2,
                                         expr_precedence_additive,
                                         DAssocLeft,
                                         DFixInfix,
                                         "+",
                                         true,
                                         true));
        table.define(expr_op_kind::sub,
                     operator_descriptor(2,
                                         expr_precedence_additive,
                                         DAssocLeft,
                                         DFixInfix,
                                         "-"));
        table.define(expr_op_kind::mul,
                     operator_descriptor(2,
                                         expr_precedence_multiplicative,
                                         DAssocLeft,
                                         DFixInfix,
                                         "*",
                                         true,
                                         true));
        table.define(expr_op_kind::div,
                     operator_descriptor(2,
                                         expr_precedence_multiplicative,
                                         DAssocLeft,
                                         DFixInfix,
                                         "/"));
        table.define(expr_op_kind::mod,
                     operator_descriptor(2,
                                         expr_precedence_multiplicative,
                                         DAssocLeft,
                                         DFixInfix,
                                         "mod"));
        table.define(expr_op_kind::pow,
                     operator_descriptor(2,
                                         expr_precedence_power,
                                         DAssocRight,
                                         DFixInfix,
                                         "^"));
        table.define(expr_op_kind::neg,
                     operator_descriptor(1,
                                         expr_precedence_unary,
                                         DAssocRight,
                                         DFixPrefix,
                                         "-"));
        table.define(expr_op_kind::pos,
                     operator_descriptor(1,
                                         expr_precedence_unary,
                                         DAssocRight,
                                         DFixPrefix,
                                         "+"));
        table.define(expr_op_kind::factorial,
                     operator_descriptor(1,
                                         expr_precedence_postfix,
                                         DAssocLeft,
                                         DFixPostfix,
                                         "!"));
        table.define(expr_op_kind::transpose,
                     operator_descriptor(1,
                                         expr_precedence_transpose,
                                         DAssocLeft,
                                         DFixPostfix,
                                         "'"));

        return table;
    }

    operator_descriptor m_descriptors[expr_op_kind_count];
    bool                m_described[expr_op_kind_count];
};


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_EXPR_OPERATOR_HPP
