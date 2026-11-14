/*******************************************************************************
* djinterp [cpp/math]                                t_parse_expression_face.cpp
*
*   Tests for parse/expression/, the framework's shared expression core, on
* which the math module's run-time expression language is to be built
* (roadmap G4), over a small language of the test's own:
*   - the term: leaves and applications of any arity, the observers, the
*     detection traits, and operator_signature's lookups;
*   - expression_ops.hpp, every function: evaluate, size, depth, atoms,
*     operators, structural_equal, contains_operator, contains_atom,
*     map_atoms, map_operators, transform_bottom_up, rewrite_to_fixpoint
*     (its fixed point and its cap on passes) and substitute;
*   - the annotated term, cofree over the same signature functor;
*   - expression_render.hpp: parentheses by precedence and by associativity
*     on either side, prefix and postfix operators, symbolic and word-like
*     spellings, a nullary symbol, and the functional form for a matchfix
*     operator, an arity its fixity does not take, and an operator the
*     signature lacks;
*   - expression_parse.hpp, over a token sequence and over a cursor:
*     precedence, associativity, prefix and postfix operators, brackets,
*     and its four errors, each with its status, offset and message;
*   - round trips: 2,000 random terms rendered and parsed back, each the
*     term it was, its text rendered again the same;
*   - expression_static.hpp: static terms, their shape, their fold, reify
*     to the dynamic term, and detection; a static leaf as a constant from
*     C++11, and a static term folded in a constant expression from C++14.
* The module's floor is C++11. Below it each header is empty, and the test
* checks only that each was read.
*
*   Build (from the repo root), at any level from C++98:
*     c++ -std=c++11 -Wall -Wextra -Werror -Wshadow -pedantic-errors -Iinc    \
*         tests/djinterp/cpp/modules/math/t_parse_expression_face.cpp          \
*         -o t_parse_expression
*
*
* path:      /tests/djinterp/cpp/modules/math/t_parse_expression_face.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.18
*                                                            revised: 2026.11.07
*******************************************************************************/
#include "../../../../../inc/djinterp/cpp/modules/parse/expression/expression.hpp"
#include "../../../../../inc/djinterp/cpp/modules/parse/expression/expression_ops.hpp"
#include "../../../../../inc/djinterp/cpp/modules/parse/expression/expression_parse.hpp"
#include "../../../../../inc/djinterp/cpp/modules/parse/expression/expression_render.hpp"
#include "../../../../../inc/djinterp/cpp/modules/parse/expression/expression_static.hpp"
// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf
#include <string>   // std::string
#include <vector>   // std::vector


// D_TESTS_MODULE_READ
//   1 where each of the module's five headers was read: at every level, as
// each defines its guard before its floor.
#if ( defined(DJINTERP_PARSE_EXPRESSION_EXPRESSION_HPP)        &&             \
      defined(DJINTERP_PARSE_EXPRESSION_EXPRESSION_OPS_HPP)    &&             \
      defined(DJINTERP_PARSE_EXPRESSION_EXPRESSION_PARSE_HPP)  &&             \
      defined(DJINTERP_PARSE_EXPRESSION_EXPRESSION_RENDER_HPP) &&             \
      defined(DJINTERP_PARSE_EXPRESSION_EXPRESSION_STATIC_HPP) )
    #define D_TESTS_MODULE_READ 1
#else
    #define D_TESTS_MODULE_READ 0
#endif


namespace
{
    long g_checks   = 0;
    long g_failures = 0;


    //==========================================================================
    // 1.  REPORTING
    //==========================================================================

    // d_tests_expect
    //   one check, reported where it fails.
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

// std, from the module's floor
#include <array>    // std::array
#include <memory>   // std::shared_ptr, std::make_shared
#include <tuple>    // std::get


namespace
{
    namespace dj = djinterp;


    //==========================================================================
    // 2.  THE TEST'S LANGUAGE
    //==========================================================================
    // Numbers and the variables x, y and z as atoms, kept as their text; the
    // four arithmetic operators and a right-associative power; a unary minus
    // beside the binary one; the postfix factorial and word-like squared,
    // which bind tightest, and a postfix percent, which binds loosest; the
    // word-like prefix not and infix and; a non-associative =; the nullary
    // pi; and the matchfix abs.


    // 2.1    Operators
    //--------------------------------------------------------------------------

    // t_op
    //   enum: the operator ids; t_op_unknown is in no signature.
    enum t_op
    {
        t_op_add,
        t_op_sub,
        t_op_mul,
        t_op_div,
        t_op_pow,
        t_op_neg,
        t_op_fact,
        t_op_squared,
        t_op_not,
        t_op_and,
        t_op_eq,
        t_op_pi,
        t_op_abs,
        t_op_percent,
        t_op_unknown
    };

    using t_atom = std::string;
    using t_expr = dj::expression<t_op, t_atom>;

    // t_row
    //   struct: one operator's row of the signature.
    struct t_row
    {
        t_op              op;
        unsigned          arity;
        int               precedence;
        dj::associativity assoc;
        dj::fixity        fix;
        const char*       spelling;
    };

    // T_ROWS
    //   constant: the signature's rows, in the ids' order, so that an id's
    // row is T_ROWS[id].
    const t_row T_ROWS[] =
    {
        { t_op_add,     2, 10, dj::DAssocLeft,  dj::DFixInfix,    "+"       },
        { t_op_sub,     2, 10, dj::DAssocLeft,  dj::DFixInfix,    "-"       },
        { t_op_mul,     2, 20, dj::DAssocLeft,  dj::DFixInfix,    "*"       },
        { t_op_div,     2, 20, dj::DAssocLeft,  dj::DFixInfix,    "/"       },
        { t_op_pow,     2, 40, dj::DAssocRight, dj::DFixInfix,    "^"       },
        { t_op_neg,     1, 30, dj::DAssocRight, dj::DFixPrefix,   "-"       },
        { t_op_fact,    1, 50, dj::DAssocLeft,  dj::DFixPostfix,  "!"       },
        { t_op_squared, 1, 50, dj::DAssocLeft,  dj::DFixPostfix,  "squared" },
        { t_op_not,     1,  5, dj::DAssocRight, dj::DFixPrefix,   "not"     },
        { t_op_and,     2,  3, dj::DAssocLeft,  dj::DFixInfix,    "and"     },
        { t_op_eq,      2,  7, dj::DAssocNone,  dj::DFixInfix,    "="       },
        { t_op_pi,      0,  0, dj::DAssocNone,  dj::DFixNullary,  "pi"      },
        { t_op_abs,     1,  0, dj::DAssocNone,  dj::DFixMatchfix, "abs"     },
        { t_op_percent, 1,  1, dj::DAssocLeft,  dj::DFixPostfix,  "%"       }
    };

    // T_ROW_COUNT
    //   constant: the number of rows.
    const std::size_t T_ROW_COUNT = sizeof(T_ROWS) / sizeof(T_ROWS[0]);

    // t_signature
    //   the language's signature, defined row by row.
    dj::operator_signature<t_op>
    t_signature()
    {
        dj::operator_signature<t_op> signature;

        for (std::size_t i = 0; i < T_ROW_COUNT; ++i)
        {
            signature.define(T_ROWS[i].op,
                             dj::operator_descriptor(T_ROWS[i].arity,
                                                     T_ROWS[i].precedence,
                                                     T_ROWS[i].assoc,
                                                     T_ROWS[i].fix,
                                                     T_ROWS[i].spelling));
        }

        return signature;
    }

    // g_signature
    //   the signature every part of the test reads.
    const dj::operator_signature<t_op> g_signature = t_signature();


    // 2.2    Terms
    //--------------------------------------------------------------------------

    // L, A0, A1, A2
    //   a leaf, and an operator applied to none, one or two terms.
    t_expr
    L(
        const char* _atom
    )
    {
        return dj::expr_leaf<t_op, t_atom>(t_atom(_atom));
    }

    t_expr
    A0(
        t_op _op
    )
    {
        return dj::expr_apply<t_op, t_atom>(_op);
    }

    t_expr
    A1(
        t_op          _op,
        const t_expr& _child
    )
    {
        return dj::expr_apply<t_op, t_atom>(_op, _child);
    }

    t_expr
    A2(
        t_op          _op,
        const t_expr& _left,
        const t_expr& _right
    )
    {
        return dj::expr_apply<t_op, t_atom>(_op, _left, _right);
    }

    // t_same
    //   two terms of the same shape.
    bool
    t_same(
        const t_expr& _a,
        const t_expr& _b
    )
    {
        return dj::structural_equal<t_op, t_atom>(_a, _b);
    }


    // 2.3    Values
    //--------------------------------------------------------------------------

    // t_atom_value
    //   an atom's value: a variable's (x 2, y 3, z 5), or a number's.
    long
    t_atom_value(
        const t_atom& _atom
    )
    {
        // the three variables
        if (_atom == "x")
        {
            return 2;
        }

        if (_atom == "y")
        {
            return 3;
        }

        if (_atom == "z")
        {
            return 5;
        }

        long value = 0;

        // a number's digits
        for (std::size_t i = 0; i < _atom.size(); ++i)
        {
            value = (value * 10) + static_cast<long>(_atom[i] - '0');
        }

        return value;
    }

    // t_power
    //   _base to the whole power _exponent; 0 for a negative one.
    long
    t_power(
        long _base,
        long _exponent
    )
    {
        long result = 1;

        // a negative exponent has no whole power
        if (_exponent < 0)
        {
            return 0;
        }

        for (long i = 0; i < _exponent; ++i)
        {
            result = result * _base;
        }

        return result;
    }

    // t_factorial
    //   _n!, and 0 for a negative _n.
    long
    t_factorial(
        long _n
    )
    {
        long result = 1;

        // no factorial below 0
        if (_n < 0)
        {
            return 0;
        }

        for (long i = 2; i <= _n; ++i)
        {
            result = result * i;
        }

        return result;
    }

    // t_apply_value
    //   an operator applied to its operands' values.
    long
    t_apply_value(
        t_op                     _op,
        const std::vector<long>& _v
    )
    {
        switch (_op)
        {
            case t_op_add:
                return _v[0] + _v[1];
            case t_op_sub:
                return _v[0] - _v[1];
            case t_op_mul:
                return _v[0] * _v[1];
            case t_op_div:
                return (_v[1] != 0) ? (_v[0] / _v[1]) : 0;
            case t_op_pow:
                return t_power(_v[0], _v[1]);
            case t_op_neg:
                return -_v[0];
            case t_op_fact:
                return t_factorial(_v[0]);
            case t_op_squared:
                return _v[0] * _v[0];
            case t_op_not:
                return (_v[0] == 0) ? 1 : 0;
            case t_op_and:
                return ( (_v[0] != 0) &&
                         (_v[1] != 0) ) ? 1 : 0;
            case t_op_eq:
                return (_v[0] == _v[1]) ? 1 : 0;
            case t_op_pi:
                return 3;
            case t_op_abs:
                return (_v[0] < 0) ? -_v[0] : _v[0];
            case t_op_percent:
                return _v[0] / 100;
            case t_op_unknown:
                return 0;
        }

        return 0;
    }

    // t_value
    //   a term's value, by evaluate.
    long
    t_value(
        const t_expr& _e
    )
    {
        return dj::evaluate<long, t_op, t_atom>(
            _e,
            [](const t_atom& _atom) -> long
            {
                return t_atom_value(_atom);
            },
            [](const t_op& _op, const std::vector<long>& _v) -> long
            {
                return t_apply_value(_op, _v);
            });
    }


    // 2.4    Text
    //--------------------------------------------------------------------------

    // t_token
    //   struct: a token: a number, a word, a symbol or a bracket. A '-' is
    // unary or binary by what comes before it, decided here, as a real
    // language's tokenizer would: the parser asks op_of of a token alone.
    struct t_token
    {
        std::string text;
        bool        unary;
    };

    // t_is_letter, t_is_digit
    //   a character's class.
    bool t_is_letter(char _c) { return ( (_c >= 'a') && (_c <= 'z') ); }
    bool t_is_digit(char _c)  { return ( (_c >= '0') && (_c <= '9') ); }

    // t_is_keyword
    //   whether a word names an operator.
    bool
    t_is_keyword(
        const std::string& _word
    )
    {
        return ( (_word == "not")     ||
                 (_word == "and")     ||
                 (_word == "pi")      ||
                 (_word == "abs")     ||
                 (_word == "squared") );
    }

    // t_ends_operand
    //   whether a token ends an operand, so that a '-' after it is binary:
    // an atom, pi, a closing bracket, or a postfix operator.
    bool
    t_ends_operand(
        const t_token& _token
    )
    {
        const char first = _token.text[0];

        return ( t_is_digit(first)                    ||
                 ( t_is_letter(first)                 &&
                   (!t_is_keyword(_token.text)) )     ||
                 (_token.text == "pi")                ||
                 (_token.text == "squared")           ||
                 (_token.text == "!")                 ||
                 (_token.text == "%")                 ||
                 (_token.text == ")") );
    }

    // t_tokenize
    //   text into tokens: runs of letters and of digits, and each other
    // character but a space on its own.
    std::vector<t_token>
    t_tokenize(
        const std::string& _text
    )
    {
        std::vector<t_token> tokens;
        std::size_t          i = 0;

        while (i < _text.size())
        {
            const char  c   = _text[i];
            std::size_t end = i + 1;

            // a space ends nothing
            if (c == ' ')
            {
                i = end;

                continue;
            }

            // a word or a number runs on
            while ( (end < _text.size())                          &&
                    ( (t_is_letter(c) && t_is_letter(_text[end])) ||
                      (t_is_digit(c)  && t_is_digit(_text[end])) ) )
            {
                ++end;
            }

            t_token token;

            token.text  = _text.substr(i, end - i);
            token.unary = ( (token.text == "-") &&
                            ( (tokens.empty()) ||
                              (!t_ends_operand(tokens.back())) ) );
            tokens.push_back(token);
            i = end;
        }

        return tokens;
    }

    // t_atom_of
    //   a token's atom: a number, or a word that names no operator.
    dj::maybe<t_atom>
    t_atom_of(
        const t_token& _token
    )
    {
        const char first = _token.text[0];

        // a number, or a variable
        if ( (t_is_digit(first)) ||
             ( (t_is_letter(first)) &&
               (!t_is_keyword(_token.text)) ) )
        {
            return dj::just(_token.text);
        }

        return dj::nothing<t_atom>();
    }

    // t_op_of
    //   the operator a token names: a '-' by its place, a '?' the operator
    // no signature has, any other by its spelling.
    dj::maybe<t_op>
    t_op_of(
        const t_token& _token
    )
    {
        // a '-' by its place
        if (_token.text == "-")
        {
            return dj::just(_token.unary ? t_op_neg : t_op_sub);
        }

        // an id the signature does not describe
        if (_token.text == "?")
        {
            return dj::just(t_op_unknown);
        }

        for (std::size_t i = 0; i < T_ROW_COUNT; ++i)
        {
            // any other by its spelling
            if (_token.text == T_ROWS[i].spelling)
            {
                return dj::just(T_ROWS[i].op);
            }
        }

        return dj::nothing<t_op>();
    }

    // t_is_open, t_is_close
    //   the brackets.
    bool t_is_open(const t_token& _token)  { return (_token.text == "("); }
    bool t_is_close(const t_token& _token) { return (_token.text == ")"); }

    // t_grammar
    //   type: the language's grammar, as make_expression_grammar makes it.
    using t_grammar = decltype(dj::make_expression_grammar<t_op, t_atom>(
                                   g_signature,
                                   &t_atom_of,
                                   &t_op_of,
                                   &t_is_open,
                                   &t_is_close));

    // g_grammar
    //   the grammar every parse of the test uses.
    const t_grammar g_grammar =
        dj::make_expression_grammar<t_op, t_atom>(g_signature,
                                                  &t_atom_of,
                                                  &t_op_of,
                                                  &t_is_open,
                                                  &t_is_close);

    // t_parse
    //   text parsed: tokenized, then parsed by parse_expression.
    dj::parse::parse_result<t_expr>
    t_parse(
        const std::string& _text
    )
    {
        const std::vector<t_token> tokens = t_tokenize(_text);

        return dj::parse_expression(tokens, g_grammar);
    }

    // t_parses_to
    //   whether text parses to the term _want.
    bool
    t_parses_to(
        const std::string& _text,
        const t_expr&      _want
    )
    {
        const dj::parse::parse_result<t_expr> parsed = t_parse(_text);

        return ( (parsed.ok()) &&
                 (t_same(parsed.value(), _want)) );
    }

    // T_NO_VALUE
    //   constant: what t_parsed_value gives a text that does not parse, a
    // value no text of the test's has.
    const long T_NO_VALUE = -999999L;

    // t_parsed_value
    //   the value of text parsed, or T_NO_VALUE where it does not parse: a
    // failed parse is a failed check, never a result read that is not there.
    long
    t_parsed_value(
        const std::string& _text
    )
    {
        const dj::parse::parse_result<t_expr> parsed = t_parse(_text);

        // a text that does not parse has no value
        if (!parsed.ok())
        {
            return T_NO_VALUE;
        }

        return t_value(parsed.value());
    }

    // t_render
    //   a term's text, each atom as it is.
    std::string
    t_render(
        const t_expr& _e
    )
    {
        return dj::render<t_op, t_atom>(_e,
                                        g_signature,
                                        [](const t_atom& _atom) -> std::string
                                        {
                                            return _atom;
                                        });
    }


    //==========================================================================
    // 3.  THE TERM
    //==========================================================================

    // d_tests_signature
    //   operator_descriptor's constructors, and operator_signature's
    // lookups: describe, index_of and has, a hit and a miss each; the first
    // of two entries for one id wins; size, empty and entries; and the
    // constructor from a prebuilt sequence.
    void
    d_tests_signature()
    {
        const dj::operator_descriptor plain;
        const dj::operator_descriptor binary(2, 15);
        const dj::operator_descriptor flagged(2, 15, dj::DAssocRight,
                                              dj::DFixInfix, "<>", true,
                                              true, true);

        D_TESTS_EXPECT( (plain.arity == 0)                &&
                        (plain.precedence == 0)           &&
                        (plain.assoc == dj::DAssocNone)   &&
                        (plain.fix == dj::DFixNullary)    &&
                        (!plain.commutative)              &&
                        (!plain.associative)              &&
                        (!plain.idempotent)               &&
                        (std::string(plain.spelling).empty()),
                        "a descriptor by default: nullary, nothing set");
        D_TESTS_EXPECT( (binary.arity == 2)               &&
                        (binary.precedence == 15)         &&
                        (binary.assoc == dj::DAssocLeft)  &&
                        (binary.fix == dj::DFixInfix)     &&
                        (!binary.commutative)             &&
                        (std::string(binary.spelling).empty()),
                        "a descriptor of an arity: infix, left, no flags");
        D_TESTS_EXPECT( (flagged.assoc == dj::DAssocRight)    &&
                        (std::string(flagged.spelling) == "<>") &&
                        (flagged.commutative)                 &&
                        (flagged.associative)                 &&
                        (flagged.idempotent),
                        "a descriptor with every field given");

        const dj::operator_signature<t_op> none;

        D_TESTS_EXPECT( (none.empty())                          &&
                        (none.size() == 0)                      &&
                        (none.describe(t_op_add) == D_NULLPTR)  &&
                        (none.index_of(t_op_add) == dj::lookup_npos) &&
                        (!none.has(t_op_add)),
                        "an empty signature describes nothing");
        D_TESTS_EXPECT( (!g_signature.empty())              &&
                        (g_signature.size() == T_ROW_COUNT) &&
                        (g_signature.entries()[0].m_key == t_op_add),
                        "the signature: one entry a row, in order");

        const dj::operator_descriptor* power = g_signature.describe(t_op_pow);

        D_TESTS_EXPECT( (power != D_NULLPTR)                 &&
                        (power->arity == 2)                  &&
                        (power->precedence == 40)            &&
                        (power->assoc == dj::DAssocRight)    &&
                        (power->fix == dj::DFixInfix)        &&
                        (std::string(power->spelling) == "^"),
                        "describe: the power's row");
        D_TESTS_EXPECT( (g_signature.index_of(t_op_pi) == 11) &&
                        (g_signature.has(t_op_abs)),
                        "index_of and has: a hit");
        D_TESTS_EXPECT( (g_signature.describe(t_op_unknown) == D_NULLPTR)   &&
                        (g_signature.index_of(t_op_unknown) ==
                             dj::lookup_npos)                               &&
                        (!g_signature.has(t_op_unknown)),
                        "describe, index_of and has: a miss");

        dj::operator_signature<t_op> twice;

        twice.define(t_op_add, dj::operator_descriptor(2, 1))
             .define(t_op_add, dj::operator_descriptor(2, 2));

        D_TESTS_EXPECT( (twice.size() == 2) &&
                        (twice.describe(t_op_add)->precedence == 1) &&
                        (twice.index_of(t_op_add) == 0),
                        "define chains; of two entries the first wins");

        std::vector<dj::DKvPair<t_op, dj::operator_descriptor> > rows;

        rows.push_back(dj::DKvPair<t_op, dj::operator_descriptor>(
            t_op_mul, dj::operator_descriptor(2, 20)));
        rows.push_back(dj::DKvPair<t_op, dj::operator_descriptor>(
            t_op_neg, dj::operator_descriptor(1, 30, dj::DAssocRight,
                                              dj::DFixPrefix, "-")));

        const dj::operator_signature<t_op> prebuilt(rows);

        D_TESTS_EXPECT( (prebuilt.size() == 2) &&
                        (prebuilt.index_of(t_op_neg) == 1) &&
                        (prebuilt.describe(t_op_neg)->fix == dj::DFixPrefix) &&
                        (!prebuilt.has(t_op_add)),
                        "a signature from a prebuilt sequence");

        return;
    }

    // d_tests_term
    //   leaves and applications of no, one, two and three children, and
    // the observers: is_leaf, atom_of, operator_of, children_of; a term
    // made by default is empty; the detection traits, and from C++14 and
    // C++20 their variable templates and concepts, by static assertion.
    void
    d_tests_term()
    {
        const t_expr leaf = L("x");
        const t_expr pi   = A0(t_op_pi);
        const t_expr neg  = A1(t_op_neg, leaf);
        const t_expr sum  = A2(t_op_add, leaf, L("1"));

        std::vector<t_expr> three;

        three.push_back(L("x"));
        three.push_back(L("y"));
        three.push_back(L("z"));

        const t_expr wide = dj::expr_apply<t_op, t_atom>(t_op_add, three);

        D_TESTS_EXPECT( (dj::is_leaf<t_op, t_atom>(leaf)) &&
                        (dj::atom_of<t_op, t_atom>(leaf) == "x"),
                        "a leaf: its atom");
        D_TESTS_EXPECT( (!dj::is_leaf<t_op, t_atom>(pi))                    &&
                        (dj::operator_of<t_op, t_atom>(pi) == t_op_pi)      &&
                        (dj::children_of<t_op, t_atom>(pi).empty()),
                        "a nullary application: no children");
        D_TESTS_EXPECT( (dj::operator_of<t_op, t_atom>(neg) == t_op_neg) &&
                        (dj::children_of<t_op, t_atom>(neg).size() == 1) &&
                        (t_same(dj::children_of<t_op, t_atom>(neg)[0], leaf)),
                        "a unary application: its child");
        D_TESTS_EXPECT( (dj::children_of<t_op, t_atom>(sum).size() == 2) &&
                        (dj::atom_of<t_op, t_atom>(
                             dj::children_of<t_op, t_atom>(sum)[1]) == "1"),
                        "a binary application: its children, in order");
        D_TESTS_EXPECT( (dj::children_of<t_op, t_atom>(wide).size() == 3) &&
                        (dj::atom_of<t_op, t_atom>(
                             dj::children_of<t_op, t_atom>(wide)[2]) == "z") &&
                        (wide.out().arity() == 3),
                        "an application of a vector of children");

        const t_expr unset;

        D_TESTS_EXPECT( (unset.empty()) &&
                        (!leaf.empty()),
                        "a term made by default is empty");

        D_STATIC_ASSERT( (dj::is_expression<t_expr>::value),
                         "a term is an expression");
        D_STATIC_ASSERT( (dj::is_expression<const t_expr&>::value),
                         "a term is an expression, cv and reference apart");
        D_STATIC_ASSERT( (!dj::is_expression<int>::value),
                         "an int is no expression");
        D_STATIC_ASSERT( (dj::is_expr_layer<t_expr::layer_type>::value),
                         "a term's layer is an expression layer");
        D_STATIC_ASSERT( (!dj::is_expr_layer<t_expr>::value),
                         "a term is no layer");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
        D_STATIC_ASSERT( (dj::is_expression_v<t_expr>) &&
                         (dj::is_expr_layer_v<t_expr::layer_type>),
                         "the variable templates");
#endif

#if ( defined(D_ENV_CPP_FEATURE_LANG_CONCEPTS) &&                             \
      (D_ENV_CPP_FEATURE_LANG_CONCEPTS == 1) )
        D_STATIC_ASSERT( (dj::Expression<t_expr>) &&
                         (dj::ExprLayer<t_expr::layer_type>) &&
                         (!dj::Expression<int>),
                         "the concepts");
#endif

        return;
    }


    //==========================================================================
    // 4.  THE FOLDS
    //==========================================================================

    // d_tests_queries
    //   evaluate, at a leaf, a nullary application and a deep term, and the
    // order it folds children in; size and depth; atoms in leaf order and
    // operators in pre-order; structural_equal, alike and unlike in each
    // way; contains_operator and contains_atom.
    void
    d_tests_queries()
    {
        // (2 * x ^ 2 + 1) - pi!, at x = 2: 9 - 6
        const t_expr term =
            A2(t_op_sub,
               A2(t_op_add,
                  A2(t_op_mul, L("2"), A2(t_op_pow, L("x"), L("2"))),
                  L("1")),
               A1(t_op_fact, A0(t_op_pi)));

        D_TESTS_EXPECT( (t_value(L("10")) == 10) &&
                        (t_value(A0(t_op_pi)) == 3) &&
                        (t_value(term) == 3),
                        "evaluate: a leaf, a nullary application, a term");

        const std::string polish = dj::evaluate<std::string, t_op, t_atom>(
            term,
            [](const t_atom& _atom) -> std::string
            {
                return _atom;
            },
            [](const t_op&                    _op,
               const std::vector<std::string>& _children) -> std::string
            {
                std::string text = std::string("(") + T_ROWS[_op].spelling;

                for (std::size_t i = 0; i < _children.size(); ++i)
                {
                    text += " " + _children[i];
                }

                return text + ")";
            });

        D_TESTS_EXPECT( (polish == "(- (+ (* 2 (^ x 2)) 1) (! (pi)))"),
                        "evaluate: children folded first, in order");

        D_TESTS_EXPECT( (dj::size<t_op, t_atom>(L("x")) == 1)          &&
                        (dj::size<t_op, t_atom>(A0(t_op_pi)) == 1)     &&
                        (dj::size<t_op, t_atom>(term) == 10),
                        "size: every node");
        D_TESTS_EXPECT( (dj::depth<t_op, t_atom>(L("x")) == 1)         &&
                        (dj::depth<t_op, t_atom>(A0(t_op_pi)) == 1)    &&
                        (dj::depth<t_op, t_atom>(term) == 5),
                        "depth: the longest path");

        const std::vector<t_atom> atoms = dj::atoms<t_op, t_atom>(term);
        const std::vector<t_op>   ops   = dj::operators<t_op, t_atom>(term);

        D_TESTS_EXPECT( (atoms.size() == 4) &&
                        (atoms[0] == "2")   &&
                        (atoms[1] == "x")   &&
                        (atoms[2] == "2")   &&
                        (atoms[3] == "1"),
                        "atoms: every leaf's, in order");
        D_TESTS_EXPECT( (ops.size() == 6)        &&
                        (ops[0] == t_op_sub)     &&
                        (ops[1] == t_op_add)     &&
                        (ops[2] == t_op_mul)     &&
                        (ops[3] == t_op_pow)     &&
                        (ops[4] == t_op_fact)    &&
                        (ops[5] == t_op_pi),
                        "operators: each ahead of its children's");
        D_TESTS_EXPECT( (dj::atoms<t_op, t_atom>(A0(t_op_pi)).empty()) &&
                        (dj::operators<t_op, t_atom>(L("x")).empty()),
                        "atoms and operators: none");

        D_TESTS_EXPECT( (t_same(term, term))                                &&
                        (t_same(A2(t_op_add, L("x"), L("1")),
                                A2(t_op_add, L("x"), L("1")))),
                        "structural_equal: a term and its like");
        D_TESTS_EXPECT( (!t_same(L("x"), L("y")))                          &&
                        (!t_same(L("x"), A1(t_op_neg, L("x"))))            &&
                        (!t_same(A1(t_op_neg, L("x")), L("x")))            &&
                        (!t_same(A2(t_op_add, L("x"), L("1")),
                                 A2(t_op_sub, L("x"), L("1"))))            &&
                        (!t_same(A2(t_op_add, L("x"), L("1")),
                                 A2(t_op_add, L("x"), L("2"))))            &&
                        (!t_same(A1(t_op_neg, L("x")),
                                 A2(t_op_neg, L("x"), L("x")))),
                        "structural_equal: an atom, a kind of node, an "
                        "operator, a child, an arity");

        D_TESTS_EXPECT( (dj::contains_operator<t_op, t_atom>(term, t_op_pi)) &&
                        (dj::contains_operator<t_op, t_atom>(term, t_op_sub)) &&
                        (!dj::contains_operator<t_op, t_atom>(term,
                                                              t_op_neg)) &&
                        (!dj::contains_operator<t_op, t_atom>(L("x"),
                                                              t_op_neg)),
                        "contains_operator");
        D_TESTS_EXPECT( (dj::contains_atom<t_op, t_atom>(term, "x"))  &&
                        (dj::contains_atom<t_op, t_atom>(term, "1"))  &&
                        (!dj::contains_atom<t_op, t_atom>(term, "y")) &&
                        (!dj::contains_atom<t_op, t_atom>(A0(t_op_pi),
                                                          "x")),
                        "contains_atom");

        return;
    }

    // t_fold_rule
    //   a rule for rewrite_to_fixpoint: a sum or a product of two numbers
    // becomes the number. One step at a node; repeated, it folds a term of
    // numbers to one.
    t_expr
    t_fold_rule(
        const t_expr& _e
    )
    {
        // only a sum or a product
        if ( (dj::is_leaf<t_op, t_atom>(_e))                        ||
             ( (dj::operator_of<t_op, t_atom>(_e) != t_op_add) &&
               (dj::operator_of<t_op, t_atom>(_e) != t_op_mul) ) )
        {
            return _e;
        }

        const std::vector<t_expr>& c = dj::children_of<t_op, t_atom>(_e);

        // of two leaves that are numbers
        if ( (!dj::is_leaf<t_op, t_atom>(c[0]))                 ||
             (!dj::is_leaf<t_op, t_atom>(c[1]))                 ||
             (!t_is_digit(dj::atom_of<t_op, t_atom>(c[0])[0]))  ||
             (!t_is_digit(dj::atom_of<t_op, t_atom>(c[1])[0])) )
        {
            return _e;
        }

        return dj::expr_leaf<t_op, t_atom>(
            std::to_string(t_value(_e)));
    }

    // t_rotate_rule
    //   a rule for rewrite_to_fixpoint: a sum whose left operand is a sum
    // regroups to the right, (a + b) + c to a + (b + c). The regrouped sum
    // is not seen again in the same pass, so a longer chain takes a pass
    // for each sum.
    t_expr
    t_rotate_rule(
        const t_expr& _e
    )
    {
        // only a sum
        if ( (dj::is_leaf<t_op, t_atom>(_e)) ||
             (dj::operator_of<t_op, t_atom>(_e) != t_op_add) )
        {
            return _e;
        }

        const std::vector<t_expr>& c = dj::children_of<t_op, t_atom>(_e);

        // whose left operand is a sum
        if ( (dj::is_leaf<t_op, t_atom>(c[0])) ||
             (dj::operator_of<t_op, t_atom>(c[0]) != t_op_add) )
        {
            return _e;
        }

        const std::vector<t_expr>& l = dj::children_of<t_op, t_atom>(c[0]);

        return A2(t_op_add, l[0], A2(t_op_add, l[1], c[1]));
    }

    // d_tests_transforms
    //   map_atoms to another atom type, map_operators to another operator
    // type; transform_bottom_up, its rule seeing each node once its
    // children are rewritten; rewrite_to_fixpoint, to its fixed point and
    // stopped by its cap; substitute, a variable bound and the rest kept.
    void
    d_tests_transforms()
    {
        // x * (y + 1)
        const t_expr term = A2(t_op_mul, L("x"), A2(t_op_add, L("y"), L("1")));

        const dj::expression<t_op, long> valued =
            dj::map_atoms<t_op, t_atom>(term,
                                        [](const t_atom& _atom) -> long
                                        {
                                            return t_atom_value(_atom);
                                        });
        const std::vector<long> numbers = dj::atoms<t_op, long>(valued);

        D_TESTS_EXPECT( (numbers.size() == 3) &&
                        (numbers[0] == 2)     &&
                        (numbers[1] == 3)     &&
                        (numbers[2] == 1)     &&
                        (dj::operators<t_op, long>(valued).size() == 2),
                        "map_atoms: each atom mapped, the shape kept");

        const dj::expression<int, t_atom> renumbered =
            dj::map_operators<t_op, t_atom>(term,
                                            [](const t_op& _op) -> int
                                            {
                                                return 100 + _op;
                                            });
        const std::vector<int> ids = dj::operators<int, t_atom>(renumbered);

        D_TESTS_EXPECT( (ids.size() == 2)          &&
                        (ids[0] == 100 + t_op_mul) &&
                        (ids[1] == 100 + t_op_add) &&
                        (dj::atoms<int, t_atom>(renumbered).size() == 3),
                        "map_operators: each operator mapped, the shape "
                        "kept");

        // a rule that wraps each node in a negation, so the order shows
        const t_expr negated = dj::transform_bottom_up<t_op, t_atom>(
            term,
            [](const t_expr& _e) -> t_expr
            {
                return A1(t_op_neg, _e);
            });

        D_TESTS_EXPECT( (t_render(negated) == "-(-x * -(-y + -1))") &&
                        (dj::size<t_op, t_atom>(negated) == 10),
                        "transform_bottom_up: the rule at every node, "
                        "children first");

        // (1 + 2) * (3 + 4) + x: one pass folds the numbers, bottom up,
        // and a second changes nothing
        const t_expr numbers_term =
            A2(t_op_add,
               A2(t_op_mul,
                  A2(t_op_add, L("1"), L("2")),
                  A2(t_op_add, L("3"), L("4"))),
               L("x"));
        const t_expr folded =
            dj::rewrite_to_fixpoint<t_op, t_atom>(numbers_term, &t_fold_rule);

        D_TESTS_EXPECT( (t_same(folded, A2(t_op_add, L("21"), L("x")))) &&
                        (t_value(folded) == t_value(numbers_term)),
                        "rewrite_to_fixpoint: the numbers folded");

        // ((a + b) + c) + d regrouped: two passes, a third changes nothing
        const t_expr chain =
            A2(t_op_add,
               A2(t_op_add, A2(t_op_add, L("a"), L("b")), L("c")),
               L("d"));
        const t_expr regrouped =
            dj::rewrite_to_fixpoint<t_op, t_atom>(chain, &t_rotate_rule);
        const t_expr once =
            dj::rewrite_to_fixpoint<t_op, t_atom>(chain, &t_rotate_rule, 1);

        D_TESTS_EXPECT( (t_render(regrouped) == "a + (b + (c + d))"),
                        "rewrite_to_fixpoint: to the fixed point");
        D_TESTS_EXPECT( (t_render(once) == "a + (b + c + d)"),
                        "rewrite_to_fixpoint: its cap stops it after one "
                        "pass");

        // a rule with no fixed point: each pass adds a negation to each leaf
        const t_expr capped = dj::rewrite_to_fixpoint<t_op, t_atom>(
            A2(t_op_add, L("x"), L("y")),
            [](const t_expr& _e) -> t_expr
            {
                return (dj::is_leaf<t_op, t_atom>(_e)) ? A1(t_op_neg, _e)
                                                       : _e;
            },
            3);

        D_TESTS_EXPECT( (t_render(capped) == "---x + ---y"),
                        "rewrite_to_fixpoint: a rule with no fixed point, "
                        "three passes");

        const t_expr bound = dj::substitute<t_op, t_atom>(
            term,
            [](const t_atom& _atom) -> t_expr
            {
                return (_atom == "x") ? A2(t_op_sub, L("z"), L("1"))
                                      : L(_atom.c_str());
            });

        D_TESTS_EXPECT( (t_render(bound) == "(z - 1) * (y + 1)") &&
                        (t_value(bound) == 16),
                        "substitute: x bound to z - 1, y kept");

        return;
    }

    // t_annotated
    //   type: a term whose every node carries its sub-term's size.
    using t_annotated = dj::annotated_expression<t_op, t_atom, std::size_t>;

    // t_annotate
    //   a term annotated with each node's size, built node by node.
    t_annotated
    t_annotate(
        const t_expr& _e
    )
    {
        using layer = t_annotated::layer_type;

        // a leaf, of size 1
        if (dj::is_leaf<t_op, t_atom>(_e))
        {
            return t_annotated::make(
                1,
                layer::leaf(dj::atom_of<t_op, t_atom>(_e)));
        }

        std::vector<std::shared_ptr<t_annotated> > children;
        std::size_t                                 size = 1;

        // each child annotated, and its size counted in
        for (const t_expr& child : dj::children_of<t_op, t_atom>(_e))
        {
            children.push_back(
                std::make_shared<t_annotated>(t_annotate(child)));
            size += children.back()->head();
        }

        return t_annotated::make(
            size,
            layer::apply(dj::operator_of<t_op, t_atom>(_e), children));
    }

    // d_tests_annotated
    //   annotated_expression, cofree over the term's own layer: built with
    // each node's size, read through head and unwrap, and mapped by
    // cofree_map.
    void
    d_tests_annotated()
    {
        // x * (y + 1)
        const t_annotated sizes =
            t_annotate(A2(t_op_mul, L("x"), A2(t_op_add, L("y"), L("1"))));

        D_TESTS_EXPECT( (sizes.head() == 5)                                 &&
                        (sizes.unwrap().is_apply())                         &&
                        (sizes.unwrap().op() == t_op_mul)                   &&
                        (sizes.unwrap().children().size() == 2)             &&
                        (sizes.unwrap().children()[0]->head() == 1)         &&
                        (sizes.unwrap().children()[0]->unwrap().atom() ==
                             "x")                                           &&
                        (sizes.unwrap().children()[1]->head() == 3),
                        "an annotated term: each node its size");

        const auto doubled =
            dj::cofree_map(sizes,
                           [](const std::size_t& _size) -> long
                           {
                               return 2 * static_cast<long>(_size);
                           });

        D_TESTS_EXPECT( (doubled.head() == 10)                              &&
                        (doubled.unwrap().children()[1]->head() == 6)       &&
                        (doubled.unwrap().children()[1]->unwrap()
                             .children()[1]->head() == 2),
                        "cofree_map: every head mapped, the shape kept");

        return;
    }


    //==========================================================================
    // 5.  RENDERING
    //==========================================================================

    // t_render_case
    //   struct: a term and the text it renders to.
    struct t_render_case
    {
        t_expr      term;
        const char* text;
    };

    // d_tests_render
    //   render: parentheses where precedence or associativity needs them,
    // on either side and for each associativity; prefix operators, symbolic
    // and word-like, over a looser, an equal and a tighter operand; postfix
    // ones likewise, the loosest-binding over a tighter operand and under a
    // tighter operator; a nullary symbol; the functional form for a
    // matchfix operator, an infix operator of one or three children, a
    // prefix one of two, and an operator the signature lacks; the atom
    // renderer's text.
    void
    d_tests_render()
    {
        const t_expr a = L("a");
        const t_expr b = L("b");
        const t_expr c = L("c");

        std::vector<t_expr> three;

        three.push_back(a);
        three.push_back(b);
        three.push_back(c);

        const t_render_case cases[] =
        {
            { A2(t_op_add, A2(t_op_mul, a, b), c),         "a * b + c"      },
            { A2(t_op_mul, A2(t_op_add, a, b), c),         "(a + b) * c"    },
            { A2(t_op_mul, a, A2(t_op_add, b, c)),         "a * (b + c)"    },
            { A2(t_op_sub, A2(t_op_sub, a, b), c),         "a - b - c"      },
            { A2(t_op_sub, a, A2(t_op_sub, b, c)),         "a - (b - c)"    },
            { A2(t_op_sub, a, A2(t_op_add, b, c)),         "a - (b + c)"    },
            { A2(t_op_add, A2(t_op_sub, a, b), c),         "a - b + c"      },
            { A2(t_op_pow, a, A2(t_op_pow, b, c)),         "a ^ b ^ c"      },
            { A2(t_op_pow, A2(t_op_pow, a, b), c),         "(a ^ b) ^ c"    },
            { A2(t_op_eq, A2(t_op_eq, a, b), c),           "(a = b) = c"    },
            { A2(t_op_eq, a, A2(t_op_eq, b, c)),           "a = (b = c)"    },
            { A2(t_op_and, A2(t_op_and, a, b), c),         "a and b and c"  },
            { A2(t_op_and, a, A2(t_op_and, b, c)),         "a and (b and c)" },
            { A1(t_op_neg, a),                             "-a"             },
            { A1(t_op_neg, A2(t_op_add, a, b)),            "-(a + b)"       },
            { A1(t_op_neg, A2(t_op_pow, a, b)),            "-a ^ b"         },
            { A2(t_op_pow, A1(t_op_neg, a), b),            "(-a) ^ b"       },
            { A2(t_op_pow, a, A1(t_op_neg, b)),            "a ^ (-b)"       },
            { A2(t_op_mul, A1(t_op_neg, a), b),            "-a * b"         },
            { A2(t_op_sub, a, A1(t_op_neg, b)),            "a - -b"         },
            { A1(t_op_neg, A1(t_op_neg, a)),               "--a"            },
            { A1(t_op_not, a),                             "not a"          },
            { A1(t_op_not, A2(t_op_and, a, b)),            "not (a and b)"  },
            { A2(t_op_and, A1(t_op_not, a), b),            "not a and b"    },
            { A1(t_op_not, A2(t_op_eq, a, b)),             "not a = b"      },
            { A2(t_op_eq, A1(t_op_not, a), b),             "(not a) = b"    },
            { A1(t_op_fact, a),                            "a!"             },
            { A1(t_op_fact, A2(t_op_add, a, b)),           "(a + b)!"       },
            { A1(t_op_fact, A1(t_op_fact, a)),             "a!!"            },
            { A1(t_op_neg, A1(t_op_fact, a)),              "-a!"            },
            { A1(t_op_fact, A1(t_op_neg, a)),              "(-a)!"          },
            { A1(t_op_squared, a),                         "a squared"      },
            { A1(t_op_squared, A1(t_op_fact, a)),          "a! squared"     },
            { A2(t_op_pow, a, A1(t_op_fact, b)),           "a ^ b!"         },
            { A0(t_op_pi),                                 "pi"             },
            { A2(t_op_mul, A0(t_op_pi), a),                "pi * a"         },
            { A1(t_op_fact, A0(t_op_pi)),                  "pi!"            },
            { A1(t_op_abs, A2(t_op_sub, a, b)),            "abs(a - b)"     },
            { A2(t_op_mul, A1(t_op_abs, a), b),            "abs(a) * b"     },
            { dj::expr_apply<t_op, t_atom>(t_op_add, three), "+(a, b, c)"   },
            { A1(t_op_add, a),                             "+(a)"           },
            { A2(t_op_neg, a, b),                          "-(a, b)"        },
            { A1(t_op_unknown, a),                         "?(a)"           },
            { A0(t_op_unknown),                            "?()"            },
            { A2(t_op_add, A1(t_op_unknown, a), b),        "?(a) + b"       },
            { A1(t_op_percent, A2(t_op_add, a, b)),        "a + b%"         },
            { A2(t_op_add, a, A1(t_op_percent, b)),        "a + (b%)"       },
            { A2(t_op_add, A1(t_op_percent, A2(t_op_mul, a, b)), c),
              "(a * b%) + c"                                                }
        };

        bool all = true;

        for (const t_render_case& each : cases)
        {
            const std::string text = t_render(each.term);
            const bool        same = (text == each.text);

            // each case, with its text where it differs
            if (!same)
            {
                std::printf("  render: \"%s\", not \"%s\"\n", text.c_str(),
                            each.text);
            }

            all = all && same;
            D_TESTS_EXPECT(same, "render: a case of the table");
        }

        D_TESTS_EXPECT(all, "render: every case of the table");

        const std::string upper = dj::render<t_op, t_atom>(
            A2(t_op_add, L("x"), L("y")),
            g_signature,
            [](const t_atom& _atom) -> std::string
            {
                return "<" + _atom + ">";
            });

        D_TESTS_EXPECT( (upper == "<x> + <y>"),
                        "render: each atom through the atom renderer");

        return;
    }


    //==========================================================================
    // 6.  PARSING
    //==========================================================================

    // t_parse_case
    //   struct: a text and the term it parses to.
    struct t_parse_case
    {
        const char* text;
        t_expr      term;
    };

    // d_tests_parse
    //   parse_expression over tokens: precedence and associativity each
    // way, prefix operators over tighter and looser operands, postfix ones,
    // a postfix after a prefix, a postfix looser than the operator before
    // it, which ends that operator's operand, brackets, the word-like
    // operators; values of parsed text; and over a cursor, from an offset
    // and to a length.
    void
    d_tests_parse()
    {
        const t_expr a = L("a");
        const t_expr b = L("b");
        const t_expr c = L("c");

        const t_parse_case cases[] =
        {
            { "a + b * c",       A2(t_op_add, a, A2(t_op_mul, b, c))     },
            { "a * b + c",       A2(t_op_add, A2(t_op_mul, a, b), c)     },
            { "(a + b) * c",     A2(t_op_mul, A2(t_op_add, a, b), c)     },
            { "a - b - c",       A2(t_op_sub, A2(t_op_sub, a, b), c)     },
            { "a - (b - c)",     A2(t_op_sub, a, A2(t_op_sub, b, c))     },
            { "a / b * c",       A2(t_op_mul, A2(t_op_div, a, b), c)     },
            { "a ^ b ^ c",       A2(t_op_pow, a, A2(t_op_pow, b, c))     },
            { "(a ^ b) ^ c",     A2(t_op_pow, A2(t_op_pow, a, b), c)     },
            { "-a ^ b",          A1(t_op_neg, A2(t_op_pow, a, b))        },
            { "-a * b",          A2(t_op_mul, A1(t_op_neg, a), b)        },
            { "a - -b",          A2(t_op_sub, a, A1(t_op_neg, b))        },
            { "--a",             A1(t_op_neg, A1(t_op_neg, a))           },
            { "a ^ -b",          A2(t_op_pow, a, A1(t_op_neg, b))        },
            { "a!",              A1(t_op_fact, a)                        },
            { "a!!",             A1(t_op_fact, A1(t_op_fact, a))         },
            { "-a!",             A1(t_op_neg, A1(t_op_fact, a))          },
            { "(-a)!",           A1(t_op_fact, A1(t_op_neg, a))          },
            { "a ^ b!",          A2(t_op_pow, a, A1(t_op_fact, b))       },
            { "a squared + 1",   A2(t_op_add, A1(t_op_squared, a), L("1")) },
            { "not a and b",     A2(t_op_and, A1(t_op_not, a), b)        },
            { "not a = b",       A1(t_op_not, A2(t_op_eq, a, b))         },
            { "a and not b",     A2(t_op_and, a, A1(t_op_not, b))        },
            { "((a))",           a                                       },
            { "a * (b - (c + 1))",
              A2(t_op_mul, a, A2(t_op_sub, b, A2(t_op_add, c, L("1")))) },
            { "a + b%",          A1(t_op_percent, A2(t_op_add, a, b))    },
            { "a + (b%)",        A2(t_op_add, a, A1(t_op_percent, b))    },
            { "a * b% + c",
              A2(t_op_add, A1(t_op_percent, A2(t_op_mul, a, b)), c)     }
        };

        bool all = true;

        for (const t_parse_case& each : cases)
        {
            const bool same = t_parses_to(each.text, each.term);

            // each case, with its text where it fails
            if (!same)
            {
                std::printf("  parse: \"%s\" fails\n", each.text);
            }

            all = all && same;
            D_TESTS_EXPECT(same, "parse: a case of the table");
        }

        D_TESTS_EXPECT(all, "parse: every case of the table");

        // values at x = 2, y = 3, z = 5
        D_TESTS_EXPECT( (t_parsed_value("2 * x ^ 2 + 1") == 9)   &&
                        (t_parsed_value("-x ^ 2") == -4)         &&
                        (t_parsed_value("2 ^ 3 ^ 2") == 512)     &&
                        (t_parsed_value("10 - 2 - 3") == 5)      &&
                        (t_parsed_value("(1 + 2) * y!") == 18)   &&
                        (t_parsed_value("z squared - 1") == 24)  &&
                        (t_parsed_value("not 0 and 1") == 1)     &&
                        (t_parsed_value("x = 2") == 1)           &&
                        (t_parsed_value("50 + 150%") == 2),
                        "parse: values of parsed text");

        // the cursor form, over "y = 2 * 3 )": tokens 2 to 4, then to 5
        const std::vector<t_token> tokens = t_tokenize("y = 2 * 3 )");

        dj::parse::parse_state<t_token> middle(tokens.data(), 5, 2);
        dj::parse::parse_state<t_token> rest(tokens.data(), 6, 2);

        const dj::parse::parse_result<t_expr> product =
            dj::parse_expression(middle, g_grammar);
        const dj::parse::parse_result<t_expr> trailing =
            dj::parse_expression(rest, g_grammar);

        D_TESTS_EXPECT( (product.ok())                                       &&
                        (t_same(product.value(),
                                A2(t_op_mul, L("2"), L("3"))))               &&
                        (middle.offset == 5)                                 &&
                        (middle.at_end()),
                        "parse: over a cursor, from an offset to a length");
        D_TESTS_EXPECT( (!trailing.ok())                                     &&
                        (trailing.error().offset() == 5)                     &&
                        (rest.offset == 5),
                        "parse: over a cursor, a trailing token by its "
                        "place in the whole");

        return;
    }

    // t_error_case
    //   struct: a text the parser refuses, and its error.
    struct t_error_case
    {
        const char*           text;
        dj::parse::DParseStatus status;
        std::size_t           offset;
        const char*           message;
    };

    // d_tests_parse_errors
    //   each of the parser's four errors, at each place it can arise, an
    // operator id the signature does not describe among them: its status,
    // its offset (a token's index) and its message.
    void
    d_tests_parse_errors()
    {
        const char* const EXPRESSION = "expected an expression";
        const char* const CLOSE      = "expected a closing bracket";
        const char* const TOKEN      = "unexpected token in expression";
        const char* const TRAILING   =
            "unexpected trailing tokens after expression";

        const t_error_case cases[] =
        {
            { "",          dj::parse::DParseStatusEndOfInput, 0, EXPRESSION },
            { "1 +",       dj::parse::DParseStatusEndOfInput, 2, EXPRESSION },
            { "-",         dj::parse::DParseStatusEndOfInput, 1, EXPRESSION },
            { "not",       dj::parse::DParseStatusEndOfInput, 1, EXPRESSION },
            { "(",         dj::parse::DParseStatusEndOfInput, 1, EXPRESSION },
            { "(1 + 2",    dj::parse::DParseStatusMalformed,  4, CLOSE      },
            { "(1 2)",     dj::parse::DParseStatusMalformed,  2, CLOSE      },
            { "((1)",      dj::parse::DParseStatusMalformed,  4, CLOSE      },
            { ")",         dj::parse::DParseStatusMalformed,  0, TOKEN      },
            { "()",        dj::parse::DParseStatusMalformed,  1, TOKEN      },
            { "1 * * 2",   dj::parse::DParseStatusMalformed,  2, TOKEN      },
            { "! 1",       dj::parse::DParseStatusMalformed,  0, TOKEN      },
            { "? 1",       dj::parse::DParseStatusMalformed,  0, TOKEN      },
            { "1 2",       dj::parse::DParseStatusMalformed,  1, TRAILING   },
            { "1 ? 2",     dj::parse::DParseStatusMalformed,  1, TRAILING   },
            { "(1) )",     dj::parse::DParseStatusMalformed,  3, TRAILING   },
            { "x + 1 abs", dj::parse::DParseStatusMalformed,  3, TRAILING   }
        };

        bool all = true;

        for (const t_error_case& each : cases)
        {
            const dj::parse::parse_result<t_expr> parsed = t_parse(each.text);
            const bool                            same   =
                ( (!parsed.ok())                                 &&
                  (parsed.error().status() == each.status)       &&
                  (parsed.error().offset() == each.offset)       &&
                  (parsed.error().message() == each.message) );

            // each case, with its error where it differs
            if (!same)
            {
                std::printf("  parse error: \"%s\"\n", each.text);
            }

            all = all && same;
            D_TESTS_EXPECT(same, "parse: an error of the table");
        }

        D_TESTS_EXPECT(all, "parse: every error of the table");

        return;
    }

    // g_random
    //   the round trips' random numbers: a 32-bit linear congruential
    // sequence, the same on every build.
    unsigned long g_random = 20261018UL;

    // t_random
    //   a random number below _n, from the sequence's high bits.
    unsigned long
    t_random(
        unsigned long _n
    )
    {
        g_random = ((g_random * 1103515245UL) + 12345UL) & 0xFFFFFFFFUL;

        return (g_random >> 16) % _n;
    }

    // t_random_term
    //   a random term of the operators the parser reads, at most five deep:
    // each subterm made in turn, so the sequence is the same whatever order
    // a compiler evaluates arguments in.
    t_expr
    t_random_term(
        int _depth
    )
    {
        static const char* const LEAVES[] = { "x", "y", "z", "1", "2", "10" };
        static const t_op        OPS[]    =
        {
            t_op_add, t_op_sub, t_op_mul, t_op_div, t_op_pow, t_op_neg,
            t_op_fact, t_op_squared, t_op_not, t_op_and, t_op_eq,
            t_op_percent
        };

        // a leaf at the depth limit, or one time in three
        if ( (_depth >= 5) ||
             (t_random(3) == 0) )
        {
            return L(LEAVES[t_random(6)]);
        }

        const t_op   op = OPS[t_random(12)];
        const t_expr left = t_random_term(_depth + 1);

        // a unary operator takes one operand
        if (g_signature.describe(op)->arity == 1)
        {
            return A1(op, left);
        }

        const t_expr right = t_random_term(_depth + 1);

        return A2(op, left, right);
    }

    // d_tests_round_trips
    //   2,000 random terms: each rendered and parsed back is the term it
    // was, and the text parsed and rendered again is the text.
    void
    d_tests_round_trips()
    {
        long same_terms = 0;
        long same_texts = 0;

        for (int i = 0; i < 2000; ++i)
        {
            const t_expr                          term   = t_random_term(0);
            const std::string                     text   = t_render(term);
            const dj::parse::parse_result<t_expr> parsed = t_parse(text);
            const bool                            back   =
                ( (parsed.ok()) &&
                  (t_same(parsed.value(), term)) );
            const bool                            again  =
                ( (parsed.ok()) &&
                  (t_render(parsed.value()) == text) );

            // each trip, with its text where it fails
            if ( (!back) ||
                 (!again) )
            {
                std::printf("  round trip: \"%s\"\n", text.c_str());
            }

            same_terms += back ? 1 : 0;
            same_texts += again ? 1 : 0;
            D_TESTS_EXPECT(back, "round trip: the term parsed back");
            D_TESTS_EXPECT(again, "round trip: the text rendered again");
        }

        D_TESTS_EXPECT( (same_terms == 2000) &&
                        (same_texts == 2000),
                        "round trips: all 2,000");

        return;
    }


    //==========================================================================
    // 7.  THE STATIC FACE
    //==========================================================================

    // t_add_tag, t_mul_tag, t_neg_tag, t_pi_tag
    //   struct: operator tags for static terms.
    struct t_add_tag
    {
        typedef t_op op_id_type;
        static constexpr t_op value = t_op_add;
    };

    struct t_mul_tag
    {
        typedef t_op op_id_type;
        static constexpr t_op value = t_op_mul;
    };

    struct t_neg_tag
    {
        typedef t_op op_id_type;
        static constexpr t_op value = t_op_neg;
    };

    struct t_pi_tag
    {
        typedef t_op op_id_type;
        static constexpr t_op value = t_op_pi;
    };

    // t_static_leaf
    //   struct: a static fold's leaf handler: a number is itself.
    struct t_static_leaf
    {
        D_CONSTEXPR long operator()(const long& _atom) const { return _atom; }
    };

    // t_static_name
    //   struct: a static fold's leaf handler for atoms of text.
    struct t_static_name
    {
        long operator()(const char* const& _atom) const
        {
            return t_atom_value(_atom);
        }
    };

    // t_static_apply
    //   struct: a static fold's application handler, for each arity the
    // test's static terms have: a constant expression from C++14, where
    // std::array's access is.
    struct t_static_apply
    {
        D_CONSTEXPR_CPP14
        long
        operator()(
            t_op                       _op,
            const std::array<long, 0>& /*_operands*/
        ) const
        {
            return (_op == t_op_pi) ? 3 : 0;
        }

        D_CONSTEXPR_CPP14
        long
        operator()(
            t_op                       _op,
            const std::array<long, 1>& _operands
        ) const
        {
            return (_op == t_op_neg) ? -_operands[0] : 0;
        }

        D_CONSTEXPR_CPP14
        long
        operator()(
            t_op                       _op,
            const std::array<long, 2>& _operands
        ) const
        {
            return (_op == t_op_add) ? (_operands[0] + _operands[1])
                 : (_op == t_op_mul) ? (_operands[0] * _operands[1])
                                     : 0;
        }
    };

    // C_LEAF
    //   constant: a static leaf, a constant from C++11.
    D_CONSTEXPR_VAR dj::static_leaf<long> C_LEAF = dj::make_static_leaf(7L);

    D_STATIC_ASSERT( (C_LEAF.atom() == 7) &&
                     (dj::static_size(C_LEAF) == 1) &&
                     (dj::static_depth(C_LEAF) == 1) &&
                     (dj::static_arity(C_LEAF) == 0),
                     "a static leaf is a constant from C++11");

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    // C_TERM
    //   constant: 1 + 2 * -5, a static term, a constant from C++14.
    constexpr auto C_TERM =
        dj::make_static_apply<t_add_tag>(
            dj::make_static_leaf(1L),
            dj::make_static_apply<t_mul_tag>(
                dj::make_static_leaf(2L),
                dj::make_static_apply<t_neg_tag>(dj::make_static_leaf(5L))));

    D_STATIC_ASSERT( (dj::static_evaluate<long>(C_TERM,
                                                t_static_leaf(),
                                                t_static_apply()) == -9) &&
                     (dj::static_size(C_TERM) == 6) &&
                     (dj::static_depth(C_TERM) == 4) &&
                     (C_TERM.op_id() == t_op_add),
                     "a static term folds in a constant expression from "
                     "C++14");
#endif

    // d_tests_static
    //   static terms: their shape, by static_arity, static_size and
    // static_depth; static_evaluate at run time, a nullary application
    // included; reify to the dynamic term, its shape, text and value as the
    // static term's, and to another operator type; detection by the traits,
    // and from C++14 and C++20 by the variable templates and concepts.
    void
    d_tests_static()
    {
        // 1 + 2 * -5, and pi
        const auto term =
            dj::make_static_apply<t_add_tag>(
                dj::make_static_leaf(1L),
                dj::make_static_apply<t_mul_tag>(
                    dj::make_static_leaf(2L),
                    dj::make_static_apply<t_neg_tag>(
                        dj::make_static_leaf(5L))));
        const auto pi = dj::make_static_apply<t_pi_tag>();

        D_TESTS_EXPECT( (dj::static_arity(term) == 2) &&
                        (dj::static_size(term) == 6)  &&
                        (dj::static_depth(term) == 4) &&
                        (dj::static_arity(pi) == 0)   &&
                        (dj::static_size(pi) == 1)    &&
                        (dj::static_depth(pi) == 1),
                        "a static term's shape");
        D_TESTS_EXPECT( (dj::static_evaluate<long>(term,
                                                   t_static_leaf(),
                                                   t_static_apply()) == -9) &&
                        (dj::static_evaluate<long>(pi,
                                                   t_static_leaf(),
                                                   t_static_apply()) == 3)  &&
                        (term.op_id() == t_op_add)                          &&
                        (std::get<0>(term.operands()).atom() == 1),
                        "static_evaluate, at run time");

        // x + 2 * y, of text atoms, into the dynamic term; each atom a
        // const char*, as make_static_leaf would decay a literal's type to
        // char* (section 6 of the record)
        const char* const x   = "x";
        const char* const two = "2";
        const char* const y   = "y";
        const char* const z   = "z";
        const auto        named =
            dj::make_static_apply<t_add_tag>(
                dj::make_static_leaf(x),
                dj::make_static_apply<t_mul_tag>(dj::make_static_leaf(two),
                                                 dj::make_static_leaf(y)));
        const t_expr reified = dj::reify<t_op, t_atom>(named);

        D_TESTS_EXPECT( (t_same(reified,
                                A2(t_op_add,
                                   L("x"),
                                   A2(t_op_mul, L("2"), L("y")))))          &&
                        (t_render(reified) == "x + 2 * y")                  &&
                        (t_value(reified) ==
                             dj::static_evaluate<long>(named,
                                                       t_static_name(),
                                                       t_static_apply())),
                        "reify: the static term's shape, text and value");
        D_TESTS_EXPECT( (t_same(dj::reify<t_op, t_atom>(pi), A0(t_op_pi))) &&
                        (t_same(dj::reify<t_op, t_atom>(
                                    dj::make_static_leaf(z)),
                                L("z"))),
                        "reify: a nullary application, a leaf");

        const dj::expression<int, long> numbered = dj::reify<int, long>(term);
        const std::vector<int>          ids      =
            dj::operators<int, long>(numbered);

        D_TESTS_EXPECT( (ids.size() == 3)        &&
                        (ids[0] == t_op_add)     &&
                        (ids[2] == t_op_neg)     &&
                        (dj::atoms<int, long>(numbered).size() == 3),
                        "reify: to another operator type");

        D_STATIC_ASSERT( (dj::is_static_leaf<dj::static_leaf<long> >::value) &&
                         (!dj::is_static_leaf<int>::value),
                         "is_static_leaf");
        D_STATIC_ASSERT( (dj::is_static_apply<
                              dj::static_apply<t_pi_tag> >::value) &&
                         (!dj::is_static_apply<
                              dj::static_leaf<long> >::value),
                         "is_static_apply");
        D_STATIC_ASSERT( (dj::is_static_expr<dj::static_leaf<long> >::value) &&
                         (dj::is_static_expr<
                              const dj::static_apply<t_pi_tag>&>::value) &&
                         (!dj::is_static_expr<t_expr>::value) &&
                         (!dj::is_static_expr<int>::value),
                         "is_static_expr");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
        D_STATIC_ASSERT( (dj::is_static_leaf_v<dj::static_leaf<long> >) &&
                         (dj::is_static_apply_v<dj::static_apply<t_pi_tag> >) &&
                         (dj::is_static_expr_v<dj::static_leaf<long> >),
                         "the static variable templates");
#endif

#if ( defined(D_ENV_CPP_FEATURE_LANG_CONCEPTS) &&                             \
      (D_ENV_CPP_FEATURE_LANG_CONCEPTS == 1) )
        D_STATIC_ASSERT( (dj::StaticLeaf<dj::static_leaf<long> >) &&
                         (dj::StaticApply<dj::static_apply<t_pi_tag> >) &&
                         (dj::StaticExpr<dj::static_leaf<long> >) &&
                         (!dj::StaticExpr<int>),
                         "the static concepts");
#endif

        return;
    }


    //==========================================================================
    // 8.  THE MODULE
    //==========================================================================

    // d_tests_module
    //   every part of the module, from its floor.
    void
    d_tests_module()
    {
        D_TESTS_EXPECT( (D_TESTS_MODULE_READ == 1),
                        "the module's five headers read");

        d_tests_signature();
        d_tests_term();
        d_tests_queries();
        d_tests_transforms();
        d_tests_annotated();
        d_tests_render();
        d_tests_parse();
        d_tests_parse_errors();
        d_tests_round_trips();
        d_tests_static();

        return;
    }
}  // namespace

#else

namespace
{
    //==========================================================================
    // 8.  THE MODULE
    //==========================================================================

    // d_tests_module
    //   below the module's floor: each header read, and nothing more to
    // test, as each is empty here.
    void
    d_tests_module()
    {
        D_TESTS_EXPECT( (D_TESTS_MODULE_READ == 1),
                        "the module's five headers read");

        return;
    }
}  // namespace

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


int
main()
{
    d_tests_module();

    std::printf("t_parse_expression_face: %ld checks, %ld failures\n",
                g_checks,
                g_failures);

    return (g_failures == 0) ? 0 : 1;
}
