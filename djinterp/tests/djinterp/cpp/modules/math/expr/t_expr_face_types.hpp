/*******************************************************************************
* djinterp [cpp][math]                                     t_expr_face_types.hpp
*
* One part of t_expr_face.cpp, the expression language's face test (S438), its
* sections 3 to 7: errors and syntax, tokens, operators, atoms and terms, and
* the tables. The test includes the parts in order, one program; each part
* includes the one before it.
*
*
* path:      /tests/djinterp/cpp/modules/math/expr/t_expr_face_types.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.21
*                                                            revised: 2026.11.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_CPP_MATH_EXPR_T_EXPR_FACE_TYPES_HPP
#define DJINTERP_TESTS_CPP_MATH_EXPR_T_EXPR_FACE_TYPES_HPP 1

#include "./t_expr_face_helpers.hpp"    // the part before


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


namespace
{

    //==========================================================================
    // 3.  ERRORS AND SYNTAX
    //==========================================================================

    // d_tests_errors
    //   expr_status, expr_error, expr_result, expr_not_yet and expr_syntax.
    void
    d_tests_errors()
    {
        // the statuses, each its own
        const m::expr_status all[] =
        {
            m::expr_status::not_yet,
            m::expr_status::bad_character,
            m::expr_status::unexpected_token,
            m::expr_status::missing_operand,
            m::expr_status::unbalanced,
            m::expr_status::unbound_variable,
            m::expr_status::unknown_name,
            m::expr_status::wrong_arity,
            m::expr_status::bad_shape,
            m::expr_status::not_differentiable,
            m::expr_status::no_convergence,
            m::expr_status::no_closed_form
        };
        const std::size_t count = sizeof(all) / sizeof(all[0]);
        bool              apart = true;

        for (std::size_t i = 0; i < count; ++i)
        {
            for (std::size_t j = i + 1; j < count; ++j)
            {
                apart = (apart && (all[i] != all[j]));
            }
        }

        D_TESTS_EXPECT( (count == 12) && apart,
                        "twelve statuses, each its own");
        D_TESTS_EXPECT( (static_cast<int>(m::expr_status::not_yet) == 1) &&
                        (static_cast<int>(m::expr_status::no_closed_form) ==
                             12),
                        "the statuses numbered 1 to 12");

        // the error
        const m::expr_error by_default;
        const m::expr_error full(m::expr_status::unbalanced, 7, "a bracket");

        D_TESTS_EXPECT( (m::expr_no_offset ==
                             static_cast<std::size_t>(-1)),
                        "expr_no_offset is the greatest size");
        D_TESTS_EXPECT( (by_default.status == m::expr_status::not_yet) &&
                        (by_default.offset == m::expr_no_offset) &&
                        (by_default.message.empty()),
                        "an error made by default: not_yet, no place, no "
                        "message");
        D_TESTS_EXPECT( (full.status == m::expr_status::unbalanced) &&
                        (full.offset == 7) &&
                        (full.message == "a bracket"),
                        "an error's three fields");
        D_TESTS_EXPECT( (full == m::expr_error(m::expr_status::unbalanced,
                                               7,
                                               "a bracket")) &&
                        (!(full != m::expr_error(m::expr_status::unbalanced,
                                                 7,
                                                 "a bracket"))),
                        "errors alike are equal");
        D_TESTS_EXPECT( (full != m::expr_error(m::expr_status::unexpected_token,
                                               7,
                                               "a bracket")) &&
                        (full != m::expr_error(m::expr_status::unbalanced,
                                               8,
                                               "a bracket")) &&
                        (full != m::expr_error(m::expr_status::unbalanced,
                                               7,
                                               "a brace")),
                        "errors differing in any field are unequal");

        // the answer of a step
        D_STATIC_ASSERT( (std::is_same<m::expr_result<int>,
                                       dj::result<int,
                                                  m::expr_error> >::value),
                         "expr_result is result<T, expr_error>");

        const m::expr_result<int> stub =
            m::internal::expr_not_yet<int>("a step: not yet");

        D_TESTS_EXPECT( d_tests_not_yet(stub, "a step: not yet"),
                        "expr_not_yet: not_yet, no place, its message");
        D_TESTS_EXPECT( (m::internal::expr_not_yet_error("a step") ==
                             m::expr_error(m::expr_status::not_yet,
                                           m::expr_no_offset,
                                           "a step")),
                        "expr_not_yet_error: the error alone");

        // the syntax switches, off by default
        const m::expr_syntax syntax;

        D_TESTS_EXPECT( (!syntax.implied_multiplication) &&
                        (!syntax.absolute_bars),
                        "gate G5's two options off by default");

        // the floating-point types
        D_STATIC_ASSERT( (m::internal::expr_floating<float>::value) &&
                         (m::internal::expr_floating<double>::value) &&
                         (m::internal::expr_floating<long double>::value),
                         "float, double and long double are typed folds' "
                         "types");
        D_STATIC_ASSERT( (!m::internal::expr_floating<int>::value) &&
                         (!m::internal::expr_floating<const double>::value) &&
                         (!m::internal::expr_floating<m::value>::value),
                         "no other type is");

        return;
    }


    //==========================================================================
    // 4.  TOKENS
    //==========================================================================

    // d_tests_tokens
    //   expr_token.
    void
    d_tests_tokens()
    {
        const m::expr_token by_default;
        const m::expr_token number(m::expr_token_kind::number, "0.1", 4);

        D_TESTS_EXPECT( (by_default.kind == m::expr_token_kind::end) &&
                        (by_default.text.empty()) &&
                        (by_default.offset == 0),
                        "a token made by default: the end, no text, 0");
        D_TESTS_EXPECT( (number.kind == m::expr_token_kind::number) &&
                        (number.text == "0.1") &&
                        (number.offset == 4),
                        "a token's three fields");
        D_TESTS_EXPECT( (number == m::expr_token(m::expr_token_kind::number,
                                                 "0.1",
                                                 4)) &&
                        (!(number != m::expr_token(m::expr_token_kind::number,
                                                   "0.1",
                                                   4))),
                        "tokens alike are equal");
        D_TESTS_EXPECT( (number != m::expr_token(m::expr_token_kind::name,
                                                 "0.1",
                                                 4)) &&
                        (number != m::expr_token(m::expr_token_kind::number,
                                                 "0.2",
                                                 4)) &&
                        (number != m::expr_token(m::expr_token_kind::number,
                                                 "0.1",
                                                 5)),
                        "tokens differing in any field are unequal");

        // the kinds, each its own
        D_TESTS_EXPECT( (static_cast<int>(m::expr_token_kind::number) == 1) &&
                        (static_cast<int>(m::expr_token_kind::end) == 8),
                        "the token kinds numbered 1 to 8");

        return;
    }

    // d_tests_tokenize
    //   expr_tokenize: numbers, names, symbols, brackets, round and square,
    // commas and semicolons, each at its offset; the spaces between; the
    // bars by the option; and no token for any other character.
    void
    d_tests_tokenize()
    {
        typedef m::expr_token      tok;
        const m::expr_token_kind   number = m::expr_token_kind::number;
        const m::expr_token_kind   name   = m::expr_token_kind::name;
        const m::expr_token_kind   symbol = m::expr_token_kind::symbol;
        const m::expr_token_kind   open   = m::expr_token_kind::open;
        const m::expr_token_kind   close  = m::expr_token_kind::close;
        m::expr_syntax             bars;

        bars.absolute_bars = true;

        D_TESTS_EXPECT( d_tests_tokens_are("x + 1",
                                           { tok(name, "x", 0),
                                             tok(symbol, "+", 2),
                                             tok(number, "1", 4) }),
                        "x + 1: a name, a symbol and a number, at 0, 2, 4");
        D_TESTS_EXPECT( d_tests_tokens_are("0.1 .5 5. 6.02e23 1e-3 007 1e+5",
                                           { tok(number, "0.1", 0),
                                             tok(number, ".5", 4),
                                             tok(number, "5.", 7),
                                             tok(number, "6.02e23", 10),
                                             tok(number, "1e-3", 18),
                                             tok(number, "007", 23),
                                             tok(number, "1e+5", 27) }),
                        "numbers by decimal.h's grammar, as written");
        D_TESTS_EXPECT( d_tests_tokens_are("1e 2x x2 _a1",
                                           { tok(number, "1", 0),
                                             tok(name, "e", 1),
                                             tok(number, "2", 3),
                                             tok(name, "x", 4),
                                             tok(name, "x2", 6),
                                             tok(name, "_a1", 9) }),
                        "a number ends where its grammar does; a name runs "
                        "on through digits and _");
        D_TESTS_EXPECT( d_tests_tokens_are("+-*/^!'=<>",
                                           { tok(symbol, "+", 0),
                                             tok(symbol, "-", 1),
                                             tok(symbol, "*", 2),
                                             tok(symbol, "/", 3),
                                             tok(symbol, "^", 4),
                                             tok(symbol, "!", 5),
                                             tok(symbol, "'", 6),
                                             tok(symbol, "=", 7),
                                             tok(symbol, "<", 8),
                                             tok(symbol, ">", 9) }),
                        "the ten symbols of one character");
        D_TESTS_EXPECT( d_tests_tokens_are("!=<=>===",
                                           { tok(symbol, "!=", 0),
                                             tok(symbol, "<=", 2),
                                             tok(symbol, ">=", 4),
                                             tok(symbol, "=", 6),
                                             tok(symbol, "=", 7) }),
                        "!= <= >= the longer first; == two symbols");
        D_TESTS_EXPECT( d_tests_tokens_are("(a)",
                                           { tok(m::expr_token_kind::open,
                                                 "(",
                                                 0),
                                             tok(name, "a", 1),
                                             tok(m::expr_token_kind::close,
                                                 ")",
                                                 2) }),
                        "the round brackets");
        D_TESTS_EXPECT( d_tests_tokens_are(" \t\n\v\f\rx",
                                           { tok(name, "x", 6) }) &&
                        d_tests_tokens_are("", {}) &&
                        d_tests_tokens_are("   ", {}),
                        "the six spaces between tokens; none in no text");
        D_TESTS_EXPECT( d_tests_tokens_are("|x|",
                                           { tok(symbol, "|", 0),
                                             tok(name, "x", 1),
                                             tok(symbol, "|", 2) },
                                           bars),
                        "| a symbol with the bars on");

        // no token
        D_TESTS_EXPECT( d_tests_is_error(m::expr_tokenize("|x|"),
                                         m::expr_status::bad_character,
                                         0,
                                         "no token begins with '|'"),
                        "| no token with the bars off");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_tokenize("1 + #"),
                                         m::expr_status::bad_character,
                                         4,
                                         "no token begins with '#'") &&
                        d_tests_is_error(m::expr_tokenize("a\x01"),
                                         m::expr_status::bad_character,
                                         1,
                                         "no token begins with the byte "
                                         "0x01") &&
                        d_tests_is_error(m::expr_tokenize("\xc3\xa9"),
                                         m::expr_status::bad_character,
                                         0,
                                         "no token begins with the byte "
                                         "0xC3"),
                        "bad_character at the first character no token "
                        "begins with, printable or not");
        D_TESTS_EXPECT( d_tests_is_error(m::expr_tokenize("1.."),
                                         m::expr_status::bad_character,
                                         2,
                                         "no token begins with '.'"),
                        "a point before no digit begins no number");

        // the comma, step 15's
        D_TESTS_EXPECT( d_tests_tokens_are("f(x,y)",
                                           { tok(name, "f", 0),
                                             tok(m::expr_token_kind::open,
                                                 "(",
                                                 1),
                                             tok(name, "x", 2),
                                             tok(m::expr_token_kind::comma,
                                                 ",",
                                                 3),
                                             tok(name, "y", 4),
                                             tok(m::expr_token_kind::close,
                                                 ")",
                                                 5) }),
                        "the comma, at its place");

        // the square brackets and the semicolon, step 19's
        D_TESTS_EXPECT( d_tests_tokens_are(
                            "[1, 2; x]a]",
                            { tok(open, "[", 0),
                              tok(number, "1", 1),
                              tok(m::expr_token_kind::comma, ",", 2),
                              tok(number, "2", 4),
                              tok(m::expr_token_kind::semicolon, ";", 5),
                              tok(name, "x", 7),
                              tok(close, "]", 8),
                              tok(name, "a", 9),
                              tok(close, "]", 10) }),
                        "square brackets, open and close, and the "
                        "semicolon, at their places");

        return;
    }


    //==========================================================================
    // 5.  OPERATORS
    //==========================================================================

    // d_tests_operators
    //   expr_op_kind, expr_op and the operator table.
    void
    d_tests_operators()
    {
        // the kinds
        D_TESTS_EXPECT( (m::expr_op_kind_count == 25) &&
                        (static_cast<std::size_t>(m::expr_op_kind::row) ==
                             m::expr_op_kind_count - 1) &&
                        (static_cast<int>(m::expr_op_kind::none) == 0),
                        "25 kinds, none 0 and row the last");

        // the operator
        const m::expr_op none;
        const m::expr_op add = m::expr_op_kind::add;
        const m::expr_op sine = m::expr_op::call("sin");

        D_TESTS_EXPECT( (none.kind == m::expr_op_kind::none) &&
                        (none.function.empty()),
                        "an operator made by default: none");
        D_TESTS_EXPECT( (add.kind == m::expr_op_kind::add) &&
                        (add.function.empty()),
                        "an operator made from a kind, implicitly");
        D_TESTS_EXPECT( (sine.kind == m::expr_op_kind::call) &&
                        (sine.function == "sin"),
                        "a call: its kind and its function");
        D_TESTS_EXPECT( (add == m::expr_op(m::expr_op_kind::add)) &&
                        (sine == m::expr_op::call("sin")) &&
                        (!(sine != m::expr_op::call("sin"))),
                        "operators alike are equal");
        D_TESTS_EXPECT( (add != m::expr_op(m::expr_op_kind::sub)) &&
                        (sine != m::expr_op::call("cos")) &&
                        (sine != m::expr_op(m::expr_op_kind::call)),
                        "operators differing in kind or function are "
                        "unequal");

        return;
    }

    // d_tests_described_with
    //   whether the standard table describes a kind as given, each of its
    // three algebraic flags among it.
    bool
    d_tests_described_with(
        m::expr_op_kind   _kind,
        unsigned          _arity,
        int               _precedence,
        dj::associativity _assoc,
        dj::fixity        _fix,
        const char*       _spelling,
        bool              _commutative,
        bool              _associative,
        bool              _idempotent
    )
    {
        const dj::operator_descriptor* described =
            m::expr_signature::standard().describe(_kind);

        return ( (described != D_NULLPTR)                       &&
                 (described->arity == _arity)                   &&
                 (described->precedence == _precedence)         &&
                 (described->assoc == _assoc)                   &&
                 (described->fix == _fix)                       &&
                 (std::string(described->spelling) == _spelling) &&
                 (described->commutative == _commutative)       &&
                 (described->associative == _associative)       &&
                 (described->idempotent == _idempotent) );
    }

    // d_tests_described_as
    //   whether the standard table describes a kind as given: commutative
    // and associative both, or neither, where _algebraic says, and not
    // idempotent.
    bool
    d_tests_described_as(
        m::expr_op_kind   _kind,
        unsigned          _arity,
        int               _precedence,
        dj::associativity _assoc,
        dj::fixity        _fix,
        const char*       _spelling,
        bool              _algebraic
    )
    {
        return d_tests_described_with(_kind,
                                      _arity,
                                      _precedence,
                                      _assoc,
                                      _fix,
                                      _spelling,
                                      _algebraic,
                                      _algebraic,
                                      false);
    }

    // d_tests_table
    //   the operator table: G5's levels; the standard table's arithmetic,
    // relations and logic, and nothing else yet; define.
    void
    d_tests_table()
    {
        // G5's levels, loosest first, ten apart, logic below the relations
        // (S213)
        D_TESTS_EXPECT( (m::expr_precedence_or == 10) &&
                        (m::expr_precedence_and == 20) &&
                        (m::expr_precedence_not == 30) &&
                        (m::expr_precedence_relation == 40) &&
                        (m::expr_precedence_additive == 50) &&
                        (m::expr_precedence_multiplicative == 60) &&
                        (m::expr_precedence_unary == 70) &&
                        (m::expr_precedence_power == 80) &&
                        (m::expr_precedence_postfix == 90),
                        "G5's nine levels, loosest first, logic below the "
                        "relations");

        // the arithmetic, each as G5 has it
        D_TESTS_EXPECT( d_tests_described_as(m::expr_op_kind::add, 2,
                                             m::expr_precedence_additive,
                                             dj::DAssocLeft, dj::DFixInfix,
                                             "+", true) &&
                        d_tests_described_as(m::expr_op_kind::sub, 2,
                                             m::expr_precedence_additive,
                                             dj::DAssocLeft, dj::DFixInfix,
                                             "-", false),
                        "+ and -: infix, left, additive; + commutative "
                        "and associative");
        D_TESTS_EXPECT( d_tests_described_as(m::expr_op_kind::mul, 2,
                                             m::expr_precedence_multiplicative,
                                             dj::DAssocLeft, dj::DFixInfix,
                                             "*", true) &&
                        d_tests_described_as(m::expr_op_kind::div, 2,
                                             m::expr_precedence_multiplicative,
                                             dj::DAssocLeft, dj::DFixInfix,
                                             "/", false) &&
                        d_tests_described_as(m::expr_op_kind::mod, 2,
                                             m::expr_precedence_multiplicative,
                                             dj::DAssocLeft, dj::DFixInfix,
                                             "mod", false),
                        "* / mod: infix, left, multiplicative; * "
                        "commutative and associative");
        D_TESTS_EXPECT( d_tests_described_as(m::expr_op_kind::pow, 2,
                                             m::expr_precedence_power,
                                             dj::DAssocRight, dj::DFixInfix,
                                             "^", false),
                        "^: infix, right, the tightest of the binary");
        D_TESTS_EXPECT( d_tests_described_as(m::expr_op_kind::neg, 1,
                                             m::expr_precedence_unary,
                                             dj::DAssocRight, dj::DFixPrefix,
                                             "-", false) &&
                        d_tests_described_as(m::expr_op_kind::pos, 1,
                                             m::expr_precedence_unary,
                                             dj::DAssocRight, dj::DFixPrefix,
                                             "+", false),
                        "the unary - and +: prefix, below ^");
        D_TESTS_EXPECT( d_tests_described_as(m::expr_op_kind::factorial, 1,
                                             m::expr_precedence_postfix,
                                             dj::DAssocLeft, dj::DFixPostfix,
                                             "!", false) &&
                        d_tests_described_as(m::expr_op_kind::transpose, 1,
                                             m::expr_precedence_transpose,
                                             dj::DAssocLeft, dj::DFixPostfix,
                                             "'", false) &&
                        (m::expr_precedence_unary <
                             m::expr_precedence_transpose) &&
                        (m::expr_precedence_transpose <
                             m::expr_precedence_power) &&
                        (m::expr_precedence_index >
                             m::expr_precedence_postfix),
                        "!: postfix, left, above ^; ': postfix, left, "
                        "between the unary - and ^; indexing above them");

        // the relations and logic
        const int relation = m::expr_precedence_relation;

        D_TESTS_EXPECT( d_tests_described_with(m::expr_op_kind::eq, 2,
                                               relation, dj::DAssocNone,
                                               dj::DFixInfix, "=", true,
                                               false, false) &&
                        d_tests_described_with(m::expr_op_kind::ne, 2,
                                               relation, dj::DAssocNone,
                                               dj::DFixInfix, "!=", true,
                                               false, false) &&
                        d_tests_described_as(m::expr_op_kind::lt, 2,
                                             relation, dj::DAssocNone,
                                             dj::DFixInfix, "<", false) &&
                        d_tests_described_as(m::expr_op_kind::le, 2,
                                             relation, dj::DAssocNone,
                                             dj::DFixInfix, "<=", false) &&
                        d_tests_described_as(m::expr_op_kind::gt, 2,
                                             relation, dj::DAssocNone,
                                             dj::DFixInfix, ">", false) &&
                        d_tests_described_as(m::expr_op_kind::ge, 2,
                                             relation, dj::DAssocNone,
                                             dj::DFixInfix, ">=", false),
                        "= != < <= > >=: infix, non-associative, below "
                        "the arithmetic; = and != commutative");
        D_TESTS_EXPECT( d_tests_described_with(m::expr_op_kind::logical_or,
                                               2, m::expr_precedence_or,
                                               dj::DAssocLeft,
                                               dj::DFixInfix, "or", true,
                                               true, true) &&
                        d_tests_described_with(m::expr_op_kind::logical_and,
                                               2, m::expr_precedence_and,
                                               dj::DAssocLeft,
                                               dj::DFixInfix, "and", true,
                                               true, true) &&
                        d_tests_described_as(m::expr_op_kind::logical_not, 1,
                                             m::expr_precedence_not,
                                             dj::DAssocRight,
                                             dj::DFixPrefix, "not", false),
                        "or and and: infix, left, commutative, associative "
                        "and idempotent; not: prefix, above them");

        // nothing else yet
        const m::expr_signature& table     = m::expr_signature::standard();
        std::size_t              described = 0;

        for (std::size_t k = 0; k < m::expr_op_kind_count; ++k)
        {
            const m::expr_op op(static_cast<m::expr_op_kind>(k));

            // described, and has, agree
            if (table.describe(op) != D_NULLPTR)
            {
                described += ( table.has(op) ? 1 : 0 );
            }
        }

        D_TESTS_EXPECT( (described == 19) &&
                        (table.has(m::expr_op_kind::factorial)) &&
                        (table.has(m::expr_op_kind::eq)) &&
                        (table.has(m::expr_op_kind::logical_not)) &&
                        (table.has(m::expr_op_kind::transpose)) &&
                        (!table.has(m::expr_op_kind::none)) &&
                        (!table.has(m::expr_op_kind::index)) &&
                        (!table.has(m::expr_op_kind::row)),
                        "the standard table describes the eighteen kinds "
                        "of 2b to 2d and 3b's transpose, and no other: "
                        "indexing is the parser layer's");
        D_TESTS_EXPECT( (table.describe(m::expr_op::call("sin")) ==
                             D_NULLPTR) &&
                        (table.describe(m::expr_op(static_cast<m::expr_op_kind>(
                             m::expr_op_kind_count))) == D_NULLPTR),
                        "nor any call, which the parser layer reads, nor a "
                        "kind out of range");
        D_TESTS_EXPECT( (&m::expr_signature::standard() == &table),
                        "the standard table is one, built once");

        // define, anew, and out of range
        m::expr_signature own;
        const dj::operator_descriptor first(2, 5, dj::DAssocLeft,
                                            dj::DFixInfix, "&");
        const dj::operator_descriptor second(2, 6, dj::DAssocRight,
                                             dj::DFixInfix, "&&");

        D_TESTS_EXPECT( (!own.has(m::expr_op_kind::add)) &&
                        (&own.define(m::expr_op_kind::add, first) == &own) &&
                        (own.has(m::expr_op_kind::add)) &&
                        (own.describe(m::expr_op_kind::add)->precedence ==
                             5),
                        "define describes a kind, and returns the table");
        own.define(m::expr_op_kind::add, second)
           .define(static_cast<m::expr_op_kind>(m::expr_op_kind_count),
                   first);
        D_TESTS_EXPECT( (own.has(m::expr_op_kind::add)) &&
                        (own.describe(m::expr_op_kind::add)->precedence ==
                             6) &&
                        (std::string(own.describe(m::expr_op_kind::add)
                                         ->spelling) == "&&") &&
                        (!own.has(m::expr_op_kind::sub)) &&
                        (!own.has(m::expr_op(static_cast<m::expr_op_kind>(
                             m::expr_op_kind_count)))),
                        "define describes a kind anew, and leaves a kind "
                        "out of range undescribed");

        return;
    }


    //==========================================================================
    // 6.  ATOMS AND TERMS
    //==========================================================================

    // d_tests_atoms
    //   expr_atom: its fields, and when two atoms are equal.
    void
    d_tests_atoms()
    {
        const m::expr_atom by_default;

        D_TESTS_EXPECT( (by_default.kind == m::expr_atom_kind::number) &&
                        (by_default.number.kind() == D_MATH_VALUE_NONE) &&
                        (by_default.text.empty()),
                        "an atom made by default: a number of none, no "
                        "text");

        const m::expr_atom one(m::expr_atom_kind::number, m::value(1), "1");
        const m::expr_atom x(m::expr_atom_kind::variable, m::value(), "x");

        D_TESTS_EXPECT( (one.kind == m::expr_atom_kind::number) &&
                        (one.number == m::value(1)) &&
                        (one.text == "1"),
                        "an atom's three fields");

        // equal: kinds, texts and numbers alike
        D_TESTS_EXPECT( (one == m::expr_atom(m::expr_atom_kind::number,
                                             m::value(1),
                                             "1")) &&
                        (!(one != m::expr_atom(m::expr_atom_kind::number,
                                               m::value(1),
                                               "1"))) &&
                        (x == m::expr_atom(m::expr_atom_kind::variable,
                                           m::value(),
                                           "x")),
                        "atoms alike are equal");

        // unequal: a kind, a text or a number apart
        D_TESTS_EXPECT( (x != m::expr_atom(m::expr_atom_kind::constant,
                                           m::value(),
                                           "x")) &&
                        (x != m::expr_atom(m::expr_atom_kind::variable,
                                           m::value(),
                                           "y")),
                        "atoms of other kinds or names are unequal");
        D_TESTS_EXPECT( (one != m::expr_atom(m::expr_atom_kind::number,
                                             m::value(1),
                                             "1.0")) &&
                        (one != m::expr_atom(m::expr_atom_kind::number,
                                             m::value(2),
                                             "1")),
                        "numbers of other texts or values are unequal");

        // the same number: of one kind, then equal, or two NaNs
        const double nan  = std::numeric_limits<double>::quiet_NaN();
        const double zero = 0.0;

        D_TESTS_EXPECT( (m::expr_atom(m::expr_atom_kind::number,
                                      m::value(1),
                                      "") !=
                             m::expr_atom(m::expr_atom_kind::number,
                                          m::value(1.0),
                                          "")),
                        "1 and 1.0 are two numbers: two kinds");
        D_TESTS_EXPECT( (m::expr_atom(m::expr_atom_kind::number,
                                      m::value::ratio(1, 3),
                                      "") ==
                             m::expr_atom(m::expr_atom_kind::number,
                                          m::value::ratio(2, 6),
                                          "")),
                        "1/3 and 2/6 are one rational");
        D_TESTS_EXPECT( (m::expr_atom(m::expr_atom_kind::number,
                                      m::value(zero),
                                      "") ==
                             m::expr_atom(m::expr_atom_kind::number,
                                          m::value(-zero),
                                          "")),
                        "0.0 and -0.0 are the same number");
        D_TESTS_EXPECT( (m::expr_atom(m::expr_atom_kind::number,
                                      m::value(nan),
                                      "") ==
                             m::expr_atom(m::expr_atom_kind::number,
                                          m::value(nan),
                                          "")),
                        "two NaNs of one type are the same number");
        D_TESTS_EXPECT( (m::expr_atom(m::expr_atom_kind::number,
                                      m::value(static_cast<float>(nan)),
                                      "") !=
                             m::expr_atom(m::expr_atom_kind::number,
                                          m::value(nan),
                                          "")),
                        "NaNs of two types are not");
        D_TESTS_EXPECT( (m::expr_atom(m::expr_atom_kind::number,
                                      m::value(D_MATH_VALUE_ERROR_POLE),
                                      "") ==
                             m::expr_atom(m::expr_atom_kind::number,
                                          m::value(D_MATH_VALUE_ERROR_POLE),
                                          "")) &&
                        (m::expr_atom(m::expr_atom_kind::number,
                                      m::value(D_MATH_VALUE_ERROR_POLE),
                                      "") !=
                             m::expr_atom(m::expr_atom_kind::number,
                                          m::value(
                                              D_MATH_VALUE_ERROR_DOMAIN),
                                          "")),
                        "errors are the same number by their codes");
        D_TESTS_EXPECT( (m::expr_atom(m::expr_atom_kind::number,
                                      m::value(true),
                                      "") !=
                             m::expr_atom(m::expr_atom_kind::number,
                                          m::value(false),
                                          "")) &&
                        (by_default == m::expr_atom()),
                        "booleans by their truth; none is none");

        return;
    }

    // d_tests_terms
    //   the term: its constructors and observers, and parse/expression's
    // queries and transforms over it.
    void
    d_tests_terms()
    {
        D_STATIC_ASSERT( (dj::is_expression<m::expr_term>::value),
                         "expr_term is a parse/expression term");
        D_STATIC_ASSERT( (std::is_same<m::expr_term,
                                       dj::expression<m::expr_op,
                                                      m::expr_atom> >::value),
                         "expr_term is expression<expr_op, expr_atom>");
        D_STATIC_ASSERT( (std::is_default_constructible<m::expr_op>::value) &&
                         (std::is_default_constructible<m::expr_atom>::value),
                         "the term's two types made by default, as "
                         "parse/expression asks");

        // the leaves
        const m::expr_term made = m::expr_number(m::value::ratio(1, 2));
        const m::expr_term half = m::expr_number(m::value::ratio(1, 2),
                                                 "0.5");
        const m::expr_term x    = m::expr_variable("x");
        const m::expr_term pi   = m::expr_constant("pi");

        D_TESTS_EXPECT( (m::expr_is_number(made)) &&
                        (m::expr_atom_of(made).text.empty()) &&
                        (m::expr_atom_of(made).number ==
                             m::value::ratio(1, 2)),
                        "a number a fold made: its value, no literal");
        D_TESTS_EXPECT( (m::expr_is_number(half)) &&
                        (m::expr_atom_of(half).text == "0.5") &&
                        (m::expr_atom_of(half).number ==
                             m::value::ratio(1, 2)),
                        "a number as written: its value and its literal");
        D_TESTS_EXPECT( (m::expr_is_variable(x)) &&
                        (!m::expr_is_number(x)) &&
                        (!m::expr_is_constant(x)) &&
                        (!m::expr_is_application(x)) &&
                        (m::expr_atom_of(x).text == "x") &&
                        (m::expr_atom_of(x).number.kind() ==
                             D_MATH_VALUE_NONE),
                        "a variable, by name");
        D_TESTS_EXPECT( (m::expr_is_constant(pi)) &&
                        (!m::expr_is_variable(pi)) &&
                        (m::expr_atom_of(pi).text == "pi"),
                        "a named constant, by name");

        // the applications, of 0, 1, 2 and 3 operands, and a call
        const m::expr_term none = m::expr_application(m::expr_op_kind::vector);
        const m::expr_term neg  = m::expr_application(m::expr_op_kind::neg, x);
        const m::expr_term sum  = d_tests_x_plus_1();
        const m::expr_term row  =
            m::expr_application(m::expr_op_kind::row,
                                std::vector<m::expr_term>{ x, half, pi });
        const m::expr_term call =
            m::expr_call("atan2", std::vector<m::expr_term>{ x, sum });

        D_TESTS_EXPECT( (m::expr_is_application(none)) &&
                        (m::expr_op_of(none) == m::expr_op_kind::vector) &&
                        (m::expr_operands_of(none).empty()),
                        "an application of no operand");
        D_TESTS_EXPECT( (m::expr_op_of(neg) == m::expr_op_kind::neg) &&
                        (m::expr_operands_of(neg).size() == 1) &&
                        (d_tests_same(m::expr_operands_of(neg)[0], x)),
                        "an application of one");
        D_TESTS_EXPECT( (m::expr_op_of(sum) == m::expr_op_kind::add) &&
                        (m::expr_operands_of(sum).size() == 2) &&
                        (m::expr_is_variable(m::expr_operands_of(sum)[0])) &&
                        (m::expr_is_number(m::expr_operands_of(sum)[1])),
                        "an application of two, in order");
        D_TESTS_EXPECT( (m::expr_op_of(row) == m::expr_op_kind::row) &&
                        (m::expr_operands_of(row).size() == 3) &&
                        (d_tests_same(m::expr_operands_of(row)[2], pi)),
                        "an application of three");
        D_TESTS_EXPECT( (m::expr_op_of(call) == m::expr_op::call("atan2")) &&
                        (m::expr_operands_of(call).size() == 2) &&
                        (d_tests_same(m::expr_operands_of(call)[1], sum)),
                        "a call: its function's name, its arguments");
        D_TESTS_EXPECT( (!m::expr_is_number(call)) &&
                        (!m::expr_is_variable(call)) &&
                        (!m::expr_is_constant(call)),
                        "an application is no leaf");

        // parse/expression over the term
        D_TESTS_EXPECT( (dj::size<m::expr_op, m::expr_atom>(call) == 5) &&
                        (dj::depth<m::expr_op, m::expr_atom>(call) == 3),
                        "size and depth");
        D_TESTS_EXPECT( (d_tests_same(call,
                                      m::expr_call("atan2",
                                                   std::vector<m::expr_term>{
                                                       m::expr_variable("x"),
                                                       d_tests_x_plus_1() }))),
                        "structural equality of terms alike");
        D_TESTS_EXPECT( (!d_tests_same(call,
                                       m::expr_call("atan", std::vector<
                                           m::expr_term>{ x, sum }))) &&
                        (!d_tests_same(sum,
                                       m::expr_application(
                                           m::expr_op_kind::add,
                                           x,
                                           m::expr_number(m::value(1),
                                                          "1.0")))),
                        "and of terms a function or a literal apart");
        D_TESTS_EXPECT( (dj::contains_operator<m::expr_op, m::expr_atom>(
                             call, m::expr_op_kind::add)) &&
                        (!dj::contains_operator<m::expr_op, m::expr_atom>(
                             call, m::expr_op_kind::mul)),
                        "contains_operator");

        const m::expr_term moved =
            dj::substitute<m::expr_op, m::expr_atom>(
                sum,
                [](const m::expr_atom& _atom) -> m::expr_term
                {
                    return (_atom == m::expr_atom(m::expr_atom_kind::variable,
                                                  m::value(),
                                                  "x"))
                        ? m::expr_constant("pi")
                        : dj::expr_leaf<m::expr_op, m::expr_atom>(_atom);
                });

        D_TESTS_EXPECT( (d_tests_same(moved,
                                      m::expr_application(
                                          m::expr_op_kind::add,
                                          m::expr_constant("pi"),
                                          m::expr_number(m::value(1),
                                                         "1")))),
                        "parse/expression's substitute");

        return;
    }


    //==========================================================================
    // 7.  TABLES
    //==========================================================================

    // d_tests_standard_function
    //   whether the standard function table has _name, of _arity arguments,
    // with its seven evaluators, a complex type's among them from chunk 41
    // (S339), and no rule of its derivatives (4a's).
    bool
    d_tests_standard_function(
        const char* _name,
        std::size_t _arity
    )
    {
        const m::expr_function* function =
            m::expr_standard_functions().find(_name);

        return ( (function != D_NULLPTR)                       &&
                 (function->name == _name)                     &&
                 (function->min_arity == _arity)               &&
                 (function->max_arity == _arity)               &&
                 (function->evaluate_float != D_NULLPTR)       &&
                 (function->evaluate_double != D_NULLPTR)      &&
                 (function->evaluate_long_double != D_NULLPTR) &&
                 (function->evaluate_complex_float != D_NULLPTR)  &&
                 (function->evaluate_complex_double != D_NULLPTR) &&
                 (function->evaluate_complex_long_double !=
                      D_NULLPTR)                               &&
                 (function->evaluate_value != D_NULLPTR)       &&
                 (function->partial == D_NULLPTR) );
    }

    // d_tests_standard_variadic
    //   whether the standard function table has _name, of any number of
    // arguments from _least, with an evaluator for each type and none for
    // its derivatives.
    bool
    d_tests_standard_variadic(
        const char* _name,
        std::size_t _least
    )
    {
        const m::expr_function* function =
            m::expr_standard_functions().find(_name);

        return ( (function != D_NULLPTR)                       &&
                 (function->name == _name)                     &&
                 (function->min_arity == _least)               &&
                 (function->max_arity == m::expr_variadic)     &&
                 (function->evaluate_float != D_NULLPTR)       &&
                 (function->evaluate_double != D_NULLPTR)      &&
                 (function->evaluate_long_double != D_NULLPTR) &&
                 (function->evaluate_complex_float != D_NULLPTR)  &&
                 (function->evaluate_complex_double != D_NULLPTR) &&
                 (function->evaluate_complex_long_double !=
                      D_NULLPTR)                               &&
                 (function->evaluate_value != D_NULLPTR)       &&
                 (function->partial == D_NULLPTR) );
    }

    // d_tests_standard_boolean
    //   whether the standard constant table has _name, 1 or 0 in each type
    // as _truth is, and as a value the boolean _truth.
    bool
    d_tests_standard_boolean(
        const char* _name,
        bool        _truth
    )
    {
        const m::expr_named_constant* constant =
            m::expr_standard_constants().find(_name);
        bool                          truth    = !_truth;

        return ( (constant != D_NULLPTR) &&
                 (constant->as_float == (_truth ? 1.0f : 0.0f)) &&
                 (constant->as_double == (_truth ? 1.0 : 0.0)) &&
                 (constant->as_long_double == (_truth ? 1.0L : 0.0L)) &&
                 (constant->as_value.kind() == D_MATH_VALUE_BOOLEAN) &&
                 (constant->as_value.to(truth)) &&
                 (truth == _truth) );
    }

    // d_tests_standard_constant
    //   whether the standard constant table has _name, of the values given
    // in each type, and as a value the long double, of its kind.
    bool
    d_tests_standard_constant(
        const char* _name,
        float       _as_float,
        double      _as_double,
        long double _as_long_double
    )
    {
        const m::expr_named_constant* constant =
            m::expr_standard_constants().find(_name);

        return ( (constant != D_NULLPTR) &&
                 (d_tests_same_real(constant->as_float, _as_float)) &&
                 (d_tests_same_real(constant->as_double, _as_double)) &&
                 (d_tests_same_real(constant->as_long_double,
                                    _as_long_double)) &&
                 (constant->as_value.kind() == D_MATH_VALUE_LONG_DOUBLE) &&
                 (constant->as_value == m::value(_as_long_double)) );
    }

    // d_tests_tables
    //   expr_function, expr_named_constant and expr_table; the standard
    // tables are d_tests_standard_functions' and
    // d_tests_standard_constants'.
    void
    d_tests_tables()
    {
        // a function
        const m::expr_function by_default;
        const m::expr_function two_args("atan2", 2, 2);
        const m::expr_function variadic("max", 1, m::expr_variadic);

        D_TESTS_EXPECT( (by_default.name.empty()) &&
                        (by_default.min_arity == 0) &&
                        (by_default.max_arity == 0) &&
                        (by_default.evaluate_float == D_NULLPTR) &&
                        (by_default.evaluate_double == D_NULLPTR) &&
                        (by_default.evaluate_long_double == D_NULLPTR) &&
                        (by_default.evaluate_value == D_NULLPTR) &&
                        (by_default.partial == D_NULLPTR),
                        "a function made by default: no name, no "
                        "argument, every slot null");
        D_TESTS_EXPECT( (two_args.name == "atan2") &&
                        (two_args.min_arity == 2) &&
                        (two_args.max_arity == 2) &&
                        (two_args.evaluate_double == D_NULLPTR) &&
                        (two_args.partial == D_NULLPTR),
                        "a function's name and arity, its slots null");
        D_TESTS_EXPECT( (!two_args.takes(1)) &&
                        (two_args.takes(2)) &&
                        (!two_args.takes(3)),
                        "takes: its arity alone");
        D_TESTS_EXPECT( (!variadic.takes(0)) &&
                        (variadic.takes(1)) &&
                        (variadic.takes(1000)) &&
                        (variadic.takes(m::expr_variadic)),
                        "takes: any number from the least, where variadic");

        // a named constant
        const m::expr_named_constant zero;
        const m::expr_named_constant half("half",
                                          0.5f,
                                          0.5,
                                          0.5L,
                                          m::value::ratio(1, 2));

        D_TESTS_EXPECT( (zero.name.empty()) &&
                        (zero.as_float == 0.0f) &&
                        (zero.as_double == 0.0) &&
                        (zero.as_long_double == 0.0L) &&
                        (zero.as_value.kind() == D_MATH_VALUE_NONE),
                        "a constant made by default: no name, 0, none");
        D_TESTS_EXPECT( (half.name == "half") &&
                        (half.as_float == 0.5f) &&
                        (half.as_double == 0.5) &&
                        (half.as_long_double == 0.5L) &&
                        (half.as_value == m::value::ratio(1, 2)),
                        "a constant's name and values");

        // a table: defined, found, replaced, in order
        m::expr_function_table table;

        D_TESTS_EXPECT( (table.empty()) &&
                        (table.size() == 0) &&
                        (table.find("atan2") == D_NULLPTR),
                        "a table made by default: empty");

        table.define(two_args).define(variadic);

        D_TESTS_EXPECT( (table.size() == 2) &&
                        (table.find("atan2") != D_NULLPTR) &&
                        (table.find("atan2")->max_arity == 2) &&
                        (table.find("max") != D_NULLPTR) &&
                        (table.find("min") == D_NULLPTR),
                        "define adds, find finds by name");

        table.define(m::expr_function("atan2", 1, 2));

        D_TESTS_EXPECT( (table.size() == 2) &&
                        (table.find("atan2") != D_NULLPTR) &&
                        (table.find("atan2")->min_arity == 1),
                        "define puts an entry in place of its name's");

        m::expr_constant_table constants;
        constants.define(half);

        D_TESTS_EXPECT( (constants.size() == 1) &&
                        (constants.find("half") != D_NULLPTR) &&
                        (constants.find("half")->as_double == 0.5),
                        "the constant table");

        constants.define(m::expr_named_constant("half",
                                                0.5f,
                                                0.5,
                                                0.5L,
                                                m::value(0.5)));

        D_TESTS_EXPECT( (constants.size() == 1) &&
                        (constants.find("half") != D_NULLPTR) &&
                        (constants.find("half")->as_value.kind() ==
                             D_MATH_VALUE_DOUBLE),
                        "a constant put in place of its name's");

        return;
    }

    // d_tests_standard_functions
    //   the standard function table: the functions of steps 15 to 24, each
    // with its evaluators, and no other.
    void
    d_tests_standard_functions()
    {
        // the standard functions: the C core's 25, abs and factorial
        static const char* const ones[] = { "sqrt", "cbrt", "sin", "cos",
                                            "tan", "asin", "acos", "atan",
                                            "sinh", "cosh", "tanh", "exp",
                                            "exp2", "log", "log2", "log10",
                                            "floor", "ceil", "round",
                                            "trunc", "abs", "factorial" };
        static const char* const twos[] = { "atan2", "pow", "hypot", "fmod",
                                            "remainder" };
        std::size_t              found  = 0;

        for (std::size_t i = 0; i < 22; ++i)
        {
            found += ( d_tests_standard_function(ones[i], 1) ? 1 : 0 );
        }

        for (std::size_t i = 0; i < 5; ++i)
        {
            found += ( d_tests_standard_function(twos[i], 2) ? 1 : 0 );
        }

        D_TESTS_EXPECT( (found == 27),
                        "the C core's 25 functions, abs and factorial, "
                        "each of its arity and its evaluators, no rule of "
                        "derivatives before 4a");
        D_TESTS_EXPECT( (d_tests_standard_variadic("piecewise", 1)) &&
                        (d_tests_standard_variadic("all_of", 1)) &&
                        (d_tests_standard_variadic("any_of", 1)),
                        "piecewise, all_of and any_of, each of any number "
                        "of arguments from 1");
        D_TESTS_EXPECT( (d_tests_standard_function("det", 1)) &&
                        (d_tests_standard_function("inv", 1)) &&
                        (d_tests_standard_function("trace", 1)) &&
                        (d_tests_standard_function("norm", 1)) &&
                        (d_tests_standard_function("eig", 1)) &&
                        (d_tests_standard_function("dot", 2)) &&
                        (d_tests_standard_function("cross", 2)) &&
                        (d_tests_standard_function("solve", 2)),
                        "linear algebra's det, inv, trace, norm and eig of "
                        "one argument, dot, cross and solve of two, each "
                        "with its four evaluators");
        D_TESTS_EXPECT( (d_tests_standard_function("complex", 2)) &&
                        (d_tests_standard_function("real", 1)) &&
                        (d_tests_standard_function("imag", 1)) &&
                        (d_tests_standard_function("conj", 1)) &&
                        (d_tests_standard_function("arg", 1)),
                        "the complex numbers' complex of two arguments, "
                        "real, imag, conj and arg of one, each with its seven "
                        "evaluators");
        D_TESTS_EXPECT( (d_tests_standard_function("log1p", 1)) &&
                        (d_tests_standard_function("expm1", 1)) &&
                        (d_tests_standard_function("asinh", 1)) &&
                        (d_tests_standard_function("acosh", 1)) &&
                        (d_tests_standard_function("atanh", 1)) &&
                        (d_tests_standard_function("sign", 1)) &&
                        (d_tests_standard_variadic("min", 1)) &&
                        (d_tests_standard_variadic("max", 1)) &&
                        (d_tests_standard_function("transpose", 1)),
                        "log1p, expm1, asinh, acosh, atanh and sign of one "
                        "argument, min and max of any number from 1, each "
                        "with its seven evaluators (S203, S338), and "
                        "transpose (S350)");
        D_TESTS_EXPECT( (d_tests_standard_function("quat", 4)) &&
                        (d_tests_standard_function("parts", 1)) &&
                        (d_tests_standard_function("rotate", 2)) &&
                        (d_tests_standard_function("rotmat", 1)) &&
                        (d_tests_standard_function("rotquat", 1)) &&
                        (d_tests_standard_function("slerp", 3)) &&
                        (m::expr_standard_functions().size() == 58),
                        "the quaternions' quat of four arguments, parts of "
                        "one and rotate of two (S275), rotmat and rotquat "
                        "of one and slerp of three (S391), each with its "
                        "seven evaluators: 58 functions");
        D_TESTS_EXPECT( (m::expr_standard_functions().find("ln") ==
                             D_NULLPTR) &&
                        (m::expr_standard_functions().find("gamma") ==
                             D_NULLPTR) &&
                        (m::expr_standard_functions().find("erf") ==
                             D_NULLPTR),
                        "and no other: no ln, no gamma, no erf");

        return;
    }

    // d_tests_standard_constants
    //   the standard constant table: pi, e, true and false, im, jm and km;
    // and each standard table one, built once.
    void
    d_tests_standard_constants()
    {
        // the standard constants: pi and e, as constants::of<T> has them,
        // and the booleans
        D_TESTS_EXPECT( (d_tests_standard_constant(
                             "pi",
                             m::constants::of<float>::pi(),
                             m::constants::of<double>::pi(),
                             m::constants::of<long double>::pi())) &&
                        (d_tests_standard_constant(
                             "e",
                             m::constants::of<float>::e(),
                             m::constants::of<double>::e(),
                             m::constants::of<long double>::e())),
                        "the standard constants: pi and e, constants::of<T>'s "
                        "in each type, the long double as a value");
        D_TESTS_EXPECT( (d_tests_standard_boolean("true", true)) &&
                        (d_tests_standard_boolean("false", false)),
                        "true and false: 1 and 0 in each type, the "
                        "booleans as values");

        // im, from step 23 (3e), the imaginary unit, a NaN in each type;
        // jm and km, from step 24 (3f), the quaternions' units, NaNs in
        // each type (d_tests_units)
        const m::expr_named_constant* const units[3] = {
            m::expr_standard_constants().find("im"),
            m::expr_standard_constants().find("jm"),
            m::expr_standard_constants().find("km") };

        D_TESTS_EXPECT( (units[0] != D_NULLPTR) &&
                        (units[1] != D_NULLPTR) &&
                        (units[2] != D_NULLPTR) &&
                        (units[0]->not_yet.empty()) &&
                        (m::expr_is_complex(units[0]->as_value)) &&
                        (units[0]->as_value.is_exact()) &&
                        (units[0]->as_value ==
                             m::expr_complex(m::value(0), m::value(1))) &&
                        (units[0]->as_float != units[0]->as_float) &&
                        (units[0]->as_double != units[0]->as_double) &&
                        (units[0]->as_long_double !=
                             units[0]->as_long_double) &&
                        (units[1]->not_yet.empty()) &&
                        (units[2]->not_yet.empty()) &&
                        (m::expr_is_quaternion(units[1]->as_value)) &&
                        (units[1]->as_value.is_exact()) &&
                        (units[1]->as_value ==
                             m::expr_quaternion(m::value(0), m::value(0),
                                                m::value(1), m::value(0))) &&
                        (units[2]->as_value ==
                             m::expr_quaternion(m::value(0), m::value(0),
                                                m::value(0), m::value(1))) &&
                        (units[1]->as_double != units[1]->as_double) &&
                        (units[2]->as_long_double !=
                             units[2]->as_long_double) &&
                        (m::expr_standard_constants().find("pi")->not_yet
                             .empty()) &&
                        (m::expr_standard_constants().size() == 7),
                        "im the exact unit, a NaN in each type; jm and km "
                        "the exact units of the quaternions, NaNs in each "
                        "type; 7 constants");
        D_TESTS_EXPECT( (&m::expr_standard_functions() ==
                             &m::expr_standard_functions()) &&
                        (&m::expr_standard_constants() ==
                             &m::expr_standard_constants()),
                        "each standard table is one, built once");

        return;
    }
}  // namespace

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_TESTS_CPP_MATH_EXPR_T_EXPR_FACE_TYPES_HPP
