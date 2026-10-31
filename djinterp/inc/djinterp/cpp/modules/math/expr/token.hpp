/*******************************************************************************
* djinterp [cpp/math]                                                  token.hpp
*
* Tokens: math text cut into numbers, names, symbols, brackets, commas and
* semicolons, the first layer of the run-time expression language.
*   A TOKEN is its kind, its text, and the offset of its first character in
* the text it came from, so that a later step can say where a fault is.
*   THE TOKENIZER knows no operator. A symbol is its text (- * <= '), and the
* parser layer (parse.hpp) tells what it is: a unary minus from a binary one,
* by where it stands. A name is read whole, whether a function's, a
* variable's or a constant's; a number as its digits, a point and an
* exponent, and the parser layer reads its value.
*   WHAT IT READS. Space between tokens, any of the six spaces of C. A number
* by decimal.h's grammar, from a digit, or from a point before one: digits,
* a point, an exponent (0.1, .5, 5., 6.02e23), never a sign, which is an
* operator, nor a fraction, which is a division. A name: a letter or _, then
* letters, digits and _. A symbol: + - * / ^ ! ' = < > and the two-character
* != <= >=, the longer first; | too, where gate G5's bars are on. The round
* and the square brackets; the comma, which separates a call's arguments and
* a literal's elements; and the semicolon, which separates a matrix
* literal's rows. The characters are ASCII; no token begins with any other.
*   STEPS. Step 14 (2b) filled expr_tokenize: numbers, names, symbols and
* round brackets; step 15 (2c) the comma; step 19 (3a) the square brackets
* and the semicolon.
*
*
* path:      /inc/djinterp/cpp/modules/math/expr/token.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.25
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TOKENS
    ------
    1.  The token
         1.  expr_token_kind
         2.  expr_token
    2.  The tokenizer
         1.  The characters
         2.  expr_tokenize
    3.  A number's text
         1.  expr_unsigned_decimal
         2.  expr_read_decimal
*/

#ifndef DJINTERP_MATH_EXPR_TOKEN_HPP
#define DJINTERP_MATH_EXPR_TOKEN_HPP 1

// djinterp
#include "../../../djinterp.hpp"  // framework root

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../../../../c/modules/math/decimal.h"  // d_math_decimal_length, _to_*
#if (defined(__cplusplus))
    #include "../../../../../re_std/cpp/cfloat/cfloat.hpp"
#endif
#include "../math_common.hpp"      // NS_MATH
#include "./expr_common.hpp"       // expr_result, expr_syntax


NS_DJINTERP
NS_MATH


//==============================================================================
// 1.  TOKENS
//==============================================================================


// 1.1    The token
//------------------------------------------------------------------------------
// 1.1.1
// expr_token_kind
//   enum: what a token is.
enum class expr_token_kind
{
    number    = 1,   // 42, 0.1, 1e-3
    name      = 2,   // x, sin, pi
    symbol    = 3,   // + - * / ^ ! ' = != < <= > >=
    open      = 4,   // ( or [
    close     = 5,   // ) or ]
    comma     = 6,   // ,
    semicolon = 7,   // ;
    end       = 8    // past the last token: what a look beyond the
                     // text finds, and a token made by default
};

// 1.1.2
// expr_token
//   struct: one token: its kind, its text, and the offset of its first
// character in the text it came from. A token made by default is the end,
// with no text, at offset 0. Two tokens are equal when all three are.
struct expr_token
{
    expr_token_kind kind;
    std::string     text;
    std::size_t     offset;

    // the end, with no text, at offset 0.
    expr_token()
        : kind(expr_token_kind::end),
          text(),
          offset(0)
    {}

    expr_token(
        expr_token_kind    _kind,
        const std::string& _text,
        std::size_t        _offset
    )
        : kind(_kind),
          text(_text),
          offset(_offset)
    {}
};

// operator==, operator!= (expr_token)
//   function: two tokens are equal when their kinds, texts and offsets are.
inline
bool operator==(
    const expr_token& _a,
    const expr_token& _b
)
{
    return ( (_a.kind == _b.kind)     &&
             (_a.text == _b.text)     &&
             (_a.offset == _b.offset) );
}

inline
bool operator!=(
    const expr_token& _a,
    const expr_token& _b
)
{
    return !(_a == _b);
}

// 1.2    The tokenizer
//------------------------------------------------------------------------------
// 1.2.1
// The characters
//   what each character begins, by its ASCII code, so that no locale changes
// a token; and the answer for a character no token of this step begins.
NS_INTERNAL

    // expr_is_space
    //   function: whether a character is a space between tokens: one of the
    // six spaces of C, as isspace's in the "C" locale.
    inline
    bool expr_is_space(
        char _c
    )
    {
        return ( (_c == ' ')  ||
                 (_c == '\t') ||
                 (_c == '\n') ||
                 (_c == '\v') ||
                 (_c == '\f') ||
                 (_c == '\r') );
    }

    // expr_is_digit
    //   function: whether a character is a decimal digit.
    inline
    bool expr_is_digit(
        char _c
    )
    {
        return ( (_c >= '0') &&
                 (_c <= '9') );
    }

    // expr_is_name_start, expr_is_name_part
    //   functions: whether a character begins a name, a letter or _, and
    // whether it goes on with one, a letter, a digit or _.
    inline
    bool expr_is_name_start(
        char _c
    )
    {
        return ( ( (_c >= 'a') &&
                   (_c <= 'z') )  ||
                 ( (_c >= 'A') &&
                   (_c <= 'Z') )  ||
                 (_c == '_') );
    }

    inline
    bool expr_is_name_part(
        char _c
    )
    {
        return ( (expr_is_name_start(_c)) ||
                 (expr_is_digit(_c)) );
    }

    // expr_symbol_length
    //   function: the length of the symbol at _at in _text, 0 where none
    // begins there: two for != <= >=, one for + - * / ^ ! ' = < >, and one
    // for | where the bars are on.
    inline
    std::size_t expr_symbol_length(
        const std::string& _text,
        std::size_t        _at,
        const expr_syntax& _syntax
    )
    {
        const char first = _text[_at];
        const char next  = ( (_at + 1 < _text.size()) ? _text[_at + 1]
                                                      : '\0' );

        // two characters, the longer first
        if ( (next == '=') &&
             ( (first == '!') ||
               (first == '<') ||
               (first == '>') ) )
        {
            return 2;
        }

        // one character
        if ( (first == '+')  ||
             (first == '-')  ||
             (first == '*')  ||
             (first == '/')  ||
             (first == '^')  ||
             (first == '!')  ||
             (first == '\'') ||
             (first == '=')  ||
             (first == '<')  ||
             (first == '>')  ||
             ( (first == '|') &&
               (_syntax.absolute_bars) ) )
        {
            return 1;
        }

        return 0;
    }

    // expr_character_name
    //   function: a character as an error's message names it: in quotes
    // where it is printable ASCII, by its code otherwise.
    inline
    std::string expr_character_name(
        char _c
    )
    {
        const unsigned code = static_cast<unsigned char>(_c);

        // printable: itself, in quotes
        if ( (code > 0x20u) &&
             (code < 0x7Fu) )
        {
            return std::string("'") + _c + "'";
        }

        const char hex[] = "0123456789ABCDEF";

        return std::string("the byte 0x") + hex[(code >> 4) & 0xFu] +
               hex[code & 0xFu];
    }

    // expr_no_token
    //   function: the answer for a character at _at that no token begins:
    // bad_character, at its place.
    inline
    expr_error expr_no_token(
        char        _c,
        std::size_t _at
    )
    {
        return expr_error(expr_status::bad_character,
                          _at,
                          "no token begins with " + expr_character_name(_c));
    }

    // expr_token_length
    //   function: the length of the token at _at, which is no space, and its
    // kind; 0 where no token begins there.
    inline
    std::size_t expr_token_length(
        const std::string& _text,
        std::size_t        _at,
        const expr_syntax& _syntax,
        expr_token_kind&   _kind
    )
    {
        const char first = _text[_at];

        // a number: a digit, or a point before one
        if ( (expr_is_digit(first)) ||
             ( (first == '.') &&
               (_at + 1 < _text.size()) &&
               (expr_is_digit(_text[_at + 1])) ) )
        {
            _kind = expr_token_kind::number;

            return d_math_decimal_length(_text.data() + _at,
                                         _text.size() - _at);
        }

        // a name, read whole
        if (expr_is_name_start(first))
        {
            std::size_t end = _at + 1;

            while ( (end < _text.size()) &&
                    (expr_is_name_part(_text[end])) )
            {
                ++end;
            }

            _kind = expr_token_kind::name;

            return end - _at;
        }

        // a bracket, round or square
        if ( (first == '(') ||
             (first == '[') )
        {
            _kind = expr_token_kind::open;

            return 1;
        }

        if ( (first == ')') ||
             (first == ']') )
        {
            _kind = expr_token_kind::close;

            return 1;
        }

        // the comma, and the semicolon
        if ( (first == ',') ||
             (first == ';') )
        {
            _kind = ( (first == ',') ? expr_token_kind::comma
                                     : expr_token_kind::semicolon );

            return 1;
        }

        _kind = expr_token_kind::symbol;

        return expr_symbol_length(_text, _at, _syntax);
    }

NS_END  // internal

// 1.2.2
// expr_tokenize
/**
 * @brief Cuts math text into tokens.
 *
 * @param[in] _text    the text.
 * @param[in] _syntax  gate G5's options; with bars for the absolute value
 *                     on, | is a symbol, and with them off, no token.
 * @return the tokens in order, each with its offset in _text, and no end
 *         token after them; or the error bad_character, at the first
 *         character that no token begins with.
 */
D_NODISCARD inline expr_result<std::vector<expr_token> > expr_tokenize(
    const std::string& _text,
    const expr_syntax& _syntax = expr_syntax()
)
{
    std::vector<expr_token> tokens;
    std::size_t             at = 0;

    // a token at a time, the spaces between them passed over
    while (at < _text.size())
    {
        // a space
        if (internal::expr_is_space(_text[at]))
        {
            ++at;

            continue;
        }

        expr_token_kind   kind   = expr_token_kind::end;
        const std::size_t length = internal::expr_token_length(_text,
                                                               at,
                                                               _syntax,
                                                               kind);

        // no token begins here
        if (length == 0)
        {
            return err<std::vector<expr_token>, expr_error>(
                internal::expr_no_token(_text[at], at));
        }

        tokens.push_back(expr_token(kind, _text.substr(at, length), at));
        at += length;
    }

    return ok<std::vector<expr_token>, expr_error>(tokens);
}

// 1.3    A number's text
//------------------------------------------------------------------------------
// What the parser layer, the folds and the renderer ask of a number's text:
// whether it is one number token, and its value in a type, rounded once.
NS_INTERNAL

    // 1.3.1
    // expr_unsigned_decimal
    //   function: whether a text is one unsigned decimal number, as the
    // tokenizer reads one: from a digit or a point, wholly decimal.h's.
    inline
    bool expr_unsigned_decimal(
        const std::string& _text
    )
    {
        return ( (!_text.empty()) &&
                 ( (expr_is_digit(_text[0])) ||
                   (_text[0] == '.') ) &&
                 (d_math_decimal_length(_text.data(), _text.size()) ==
                      _text.size()) );
    }

    // 1.3.2
    // expr_read_decimal
    //   functions: a decimal text as the nearest float, double or long
    // double, ties to even, into _out, by decimal.h, and true; false, and
    // _out as it was, where the text is no decimal number.
    inline
    bool expr_read_decimal(
        float&             _out,
        const std::string& _text
    )
    {
        return (d_math_decimal_to_f(&_out, _text.data(), _text.size()) !=
                    D_MATH_DECIMAL_SYNTAX);
    }

    inline
    bool expr_read_decimal(
        double&            _out,
        const std::string& _text
    )
    {
        return (d_math_decimal_to_d(&_out, _text.data(), _text.size()) !=
                    D_MATH_DECIMAL_SYNTAX);
    }

    inline
    bool expr_read_decimal(
        long double&       _out,
        const std::string& _text
    )
    {
        return (d_math_decimal_to_ld(&_out, _text.data(), _text.size()) !=
                    D_MATH_DECIMAL_SYNTAX);
    }

NS_END  // internal


NS_END  // math
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_MATH_EXPR_TOKEN_HPP
