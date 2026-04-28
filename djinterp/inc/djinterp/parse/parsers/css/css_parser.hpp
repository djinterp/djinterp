/******************************************************************************
* djinterp [css]                                                css_parser.hpp
*
* CSS parser:
*   This header implements the parser for a CSS-3-shaped grammar atop the
* djinterp::parse text_parser_base framework.  Three public parser classes
* are exposed:
*
*   - css_stylesheet_parser     full stylesheets (the typical entry point)
*   - css_selector_parser       a single selector list
*   - css_declaration_parser    an inline declaration list (`style="..."`)
*
*   All three are CRTP-derived from text_parser_base and conform to the
* generic parser contract via their `do_parse` member.  The grammar
* productions themselves are implemented as free functions inside the
* `internal` namespace so they can be reused without going through the
* parser_base machinery.
*
*   Parsing is single-pass with no backtracking on the hot path.  Where
* the grammar is locally ambiguous (e.g. selector vs declaration in an
* at-rule body) the parser uses bounded look-ahead.  At top level the
* stylesheet parser performs simple error recovery by skipping to the
* next `}` or `;` so that one bad rule does not poison the rest of the
* input — matching the forgiving semantics of real CSS.
*
* Performance notes:
*   - No std::string allocations in the scanner hot path; strings are
*     materialised only when an actual ident/value/string is being
*     captured.
*   - Specificity is computed once per complex_selector and packed into
*     a uint32_t.
*   - Function-call values nest recursively but the nesting depth is
*     bounded by D_CSS_MAX_VALUE_DEPTH to prevent pathological input
*     from causing stack growth without bound.
*
*
* path:      /inc/cpp/css/css_parser.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.25
******************************************************************************/

#ifndef DJINTERP_CSS_PARSER_
#define DJINTERP_CSS_PARSER_ 1

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>
#include "../parse/parse.hpp"
#include "../parse/text_parser.hpp"
#include "./css_ast.hpp"


// D_CSS_MAX_VALUE_DEPTH
//   macro: maximum recursion depth allowed inside a value list.
// Bounds nested function-call expansion (e.g. calc(calc(...))).
#define D_CSS_MAX_VALUE_DEPTH        32


NS_DJINTERP
NS_CSS


// ================================================================
//  internal -- low-level scanner helpers
// ================================================================

NS_INTERNAL

    using parse::text_parse_state;
    using parse::parse_result;
    using parse::parse_error;


    // ---- character classification ----------------------------

    // is_ident_start
    //   function: tests whether _c may begin a CSS identifier.
    // CSS allows identifiers to start with a letter, underscore,
    // hyphen-followed-by-{letter,_}, or a high-bit byte (UTF-8
    // continuation) which we admit conservatively.
    D_STATIC_CONSTEXPR_INLINE bool
    is_ident_start(char _c)
    {
        return ( ( (_c >= 'a') && (_c <= 'z') ) ||
                 ( (_c >= 'A') && (_c <= 'Z') ) ||
                 (_c == '_')                    ||
                 (static_cast<unsigned char>(_c) >= 0x80) );
    }

    // is_ident_continue
    //   function: tests whether _c may continue a CSS identifier
    // after the first character.
    D_STATIC_CONSTEXPR_INLINE bool
    is_ident_continue(char _c)
    {
        return ( is_ident_start(_c)             ||
                 ( (_c >= '0') && (_c <= '9') ) ||
                 (_c == '-') );
    }

    // is_digit
    //   function: tests whether _c is an ASCII decimal digit.
    D_STATIC_CONSTEXPR_INLINE bool
    is_digit(char _c)
    {
        return ( (_c >= '0') && (_c <= '9') );
    }


    // ---- whitespace and comments -----------------------------

    // skip_ws_and_comments
    //   function: advances past any sequence of ASCII whitespace
    // and `/* ... */` block comments.  Updates the line/column
    // tracker on the underlying text_parse_state.
    inline void
    skip_ws_and_comments(text_parse_state& _s)
    {
        // loop: each iteration handles one whitespace run
        // followed by an optional comment.
        for (;;)
        {
            // skip whitespace
            while ( (!_s.at_end()) &&
                    (parse::char_predicate_is_whitespace::test(
                        _s.peek())) )
            {
                _s.advance_tracking(1);
            }

            // check for comment start
            if ( (_s.remaining() >= 2u)              &&
                 (_s.data[_s.offset]     == '/')     &&
                 (_s.data[_s.offset + 1] == '*') )
            {
                // consume the opening /*
                _s.advance_tracking(2);

                // advance until closing */ or EOF
                while (!_s.at_end())
                {
                    if ( (_s.remaining() >= 2u)              &&
                         (_s.data[_s.offset]     == '*')     &&
                         (_s.data[_s.offset + 1] == '/') )
                    {
                        _s.advance_tracking(2);

                        break;
                    }

                    _s.advance_tracking(1);
                }

                // loop to consume any trailing whitespace
                continue;
            }

            // no whitespace and no comment found
            break;
        }

        return;
    }


    // ---- identifier ------------------------------------------

    // parse_ident
    //   function: consumes a CSS identifier and returns it as a
    // std::string.  CSS allows a leading hyphen if followed by an
    // ident-start character; this function admits that form.
    inline parse_result<std::string>
    parse_ident(text_parse_state& _s)
    {
        std::size_t start;
        char        first;

        if (_s.at_end())
        {
            return parse_result<std::string>::make_error(
                parse::DParseStatusEndOfInput,
                _s.offset,
                "expected identifier"
            );
        }

        start = _s.offset;
        first = _s.peek();

        // accept leading '-' only if the next char is a valid
        // ident-start; this avoids consuming a stray '-' that
        // belongs to a sign or combinator
        if (first == '-')
        {
            if ( (_s.remaining() < 2u) ||
                 (!is_ident_start(_s.peek_at(1))) )
            {
                return parse_result<std::string>::make_error(
                    parse::DParseStatusFailure,
                    _s.offset,
                    "expected identifier"
                );
            }

            _s.advance_tracking(1);
        }
        else if (!is_ident_start(first))
        {
            return parse_result<std::string>::make_error(
                parse::DParseStatusFailure,
                _s.offset,
                "expected identifier"
            );
        }

        // consume continuation characters
        while ( (!_s.at_end()) &&
                (is_ident_continue(_s.peek())) )
        {
            _s.advance_tracking(1);
        }

        return parse_result<std::string>(
            std::string(_s.data + start, _s.offset - start)
        );
    }


    // ---- string literal --------------------------------------

    // parse_string
    //   function: consumes a quoted string literal and returns
    // its contents (with the surrounding quotes removed and
    // backslash escapes processed for the simple cases).
    //
    //   _expected_quote is either '"' or '\''.  If the current
    // char is not the expected quote, a failure is returned with
    // the state unchanged.
    inline parse_result<std::string>
    parse_string(text_parse_state& _s)
    {
        std::string out;
        char        quote;

        if (_s.at_end())
        {
            return parse_result<std::string>::make_error(
                parse::DParseStatusEndOfInput,
                _s.offset,
                "expected string literal"
            );
        }

        quote = _s.peek();

        if ( (quote != '"') &&
             (quote != '\'') )
        {
            return parse_result<std::string>::make_error(
                parse::DParseStatusFailure,
                _s.offset,
                "expected string literal"
            );
        }

        // consume opening quote
        _s.advance_tracking(1);

        // body
        while (!_s.at_end())
        {
            char c = _s.peek();

            // closing quote terminates the literal
            if (c == quote)
            {
                _s.advance_tracking(1);

                return parse_result<std::string>(out);
            }

            // unescaped newline inside an unbroken string is
            // a parse error per CSS syntax
            if (c == '\n')
            {
                return parse_result<std::string>::make_error(
                    parse::DParseStatusFailure,
                    _s.offset,
                    "unterminated string literal"
                );
            }

            // simple backslash escape: take the next char as-is.
            // Full CSS escape semantics (hex \xx, line cont.)
            // can be added without changing the interface.
            if ( (c == '\\') &&
                 (_s.remaining() >= 2u) )
            {
                _s.advance_tracking(1);
                out += _s.peek();
                _s.advance_tracking(1);

                continue;
            }

            out += c;
            _s.advance_tracking(1);
        }

        return parse_result<std::string>::make_error(
            parse::DParseStatusEndOfInput,
            _s.offset,
            "unterminated string literal"
        );
    }


    // ---- number ----------------------------------------------

    // parse_number_value
    //   function: consumes a numeric literal (with optional sign,
    // fractional part, and exponent) and any trailing dimension
    // unit or `%`.  Produces a css_value of the appropriate kind.
    //
    //   The numeric scan is hand-rolled rather than going through
    // strtod so we know exactly which input bytes were consumed,
    // and so we never overrun the bounded buffer.
    inline parse_result<css_value>
    parse_number_value(text_parse_state& _s)
    {
        std::size_t start;
        bool        saw_digit;
        char        c;

        if (_s.at_end())
        {
            return parse_result<css_value>::make_error(
                parse::DParseStatusEndOfInput,
                _s.offset,
                "expected number"
            );
        }

        start     = _s.offset;
        saw_digit = false;

        // optional sign
        c = _s.peek();

        if ( (c == '+') ||
             (c == '-') )
        {
            _s.advance_tracking(1);
        }

        // integer part
        while ( (!_s.at_end()) &&
                (is_digit(_s.peek())) )
        {
            saw_digit = true;
            _s.advance_tracking(1);
        }

        // fractional part
        if ( (!_s.at_end()) &&
             (_s.peek() == '.') &&
             (_s.peek_at(1) != '\0') &&
             (is_digit(_s.peek_at(1))) )
        {
            _s.advance_tracking(1); // consume '.'

            while ( (!_s.at_end()) &&
                    (is_digit(_s.peek())) )
            {
                saw_digit = true;
                _s.advance_tracking(1);
            }
        }

        // require at least one digit overall
        if (!saw_digit)
        {
            _s.offset = start; // best-effort rollback (line/col
                               // unchanged because no '\n' seen)
            return parse_result<css_value>::make_error(
                parse::DParseStatusFailure,
                start,
                "expected number"
            );
        }

        // exponent
        if ( (!_s.at_end()) &&
             ( (_s.peek() == 'e') ||
               (_s.peek() == 'E') ) )
        {
            std::size_t exp_save = _s.offset;
            bool        exp_ok   = false;

            _s.advance_tracking(1);

            if ( (!_s.at_end()) &&
                 ( (_s.peek() == '+') ||
                   (_s.peek() == '-') ) )
            {
                _s.advance_tracking(1);
            }

            while ( (!_s.at_end()) &&
                    (is_digit(_s.peek())) )
            {
                exp_ok = true;
                _s.advance_tracking(1);
            }

            if (!exp_ok)
            {
                // not actually an exponent; restore. (unit may
                // be 'em' or 'ex' starting with 'e'.)
                _s.offset = exp_save;
            }
        }

        // materialise the numeric value via std::strtod on a
        // temporary copy.  The byte range is bounded so this is
        // safe and constant-time in practice.
        css_value   v;
        std::string num_text(_s.data + start, _s.offset - start);

        v.number = std::strtod(num_text.c_str(), nullptr);

        // dimension unit or percentage
        if ( (!_s.at_end()) &&
             (_s.peek() == '%') )
        {
            _s.advance_tracking(1);
            v.kind = DCssValueKindPercentage;
        }
        else if ( (!_s.at_end()) &&
                  (is_ident_start(_s.peek())) )
        {
            std::size_t unit_start = _s.offset;

            while ( (!_s.at_end()) &&
                    (is_ident_continue(_s.peek())) )
            {
                _s.advance_tracking(1);
            }

            v.unit = std::string(_s.data + unit_start,
                                 _s.offset - unit_start);
            v.kind = DCssValueKindNumber;
        }
        else
        {
            v.kind = DCssValueKindNumber;
        }

        return parse_result<css_value>(v);
    }


    // ---- value component (forward-declared for recursion) ----

    inline parse_result<css_value>
    parse_value_component(text_parse_state& _s,
                          int                _depth);

    // parse_value_list_until
    //   function: consumes a sequence of value components up to
    // (but not including) one of the supplied delimiter chars,
    // ';', '}', '!', or end-of-input.
    inline parse_result<css_value_list>
    parse_value_list_until(text_parse_state& _s,
                           char               _stop_a,
                           char               _stop_b,
                           int                _depth)
    {
        css_value_list out;

        // bound recursion depth
        if (_depth > D_CSS_MAX_VALUE_DEPTH)
        {
            return parse_result<css_value_list>::make_error(
                parse::DParseStatusFailure,
                _s.offset,
                "value-list nesting too deep"
            );
        }

        for (;;)
        {
            skip_ws_and_comments(_s);

            if (_s.at_end())
            {
                break;
            }

            char c = _s.peek();

            // structural terminators
            if ( (c == _stop_a) ||
                 (c == _stop_b) ||
                 (c == ';')     ||
                 (c == '}')     ||
                 (c == '!') )
            {
                break;
            }

            auto comp = parse_value_component(_s, _depth);

            if (!comp.ok())
            {
                return parse_result<css_value_list>(comp.error());
            }

            out.push_back(comp.value());
        }

        return parse_result<css_value_list>(out);
    }

    // parse_value_component
    //   function: consumes a single value-list component
    // (number, ident-or-function, string, hash, or delimiter).
    inline parse_result<css_value>
    parse_value_component(text_parse_state& _s,
                          int                _depth)
    {
        char    c;

        if (_s.at_end())
        {
            return parse_result<css_value>::make_error(
                parse::DParseStatusEndOfInput,
                _s.offset,
                "expected value component"
            );
        }

        c = _s.peek();

        // number, including signed numbers
        if ( (is_digit(c))                                 ||
             (c == '.')                                    ||
             ( (c == '+') && (is_digit(_s.peek_at(1))) )   ||
             ( (c == '-') && (is_digit(_s.peek_at(1))) )   ||
             ( (c == '+') && (_s.peek_at(1) == '.') )      ||
             ( (c == '-') && (_s.peek_at(1) == '.') ) )
        {
            return parse_number_value(_s);
        }

        // string
        if ( (c == '"') ||
             (c == '\'') )
        {
            auto s = parse_string(_s);

            if (!s.ok())
            {
                return parse_result<css_value>(s.error());
            }

            css_value v;
            v.kind = DCssValueKindString;
            v.text = s.value();

            return parse_result<css_value>(v);
        }

        // hash (e.g. #fff or #1a2b3c)
        if (c == '#')
        {
            std::size_t start = _s.offset;

            _s.advance_tracking(1);

            // body characters: ident-continue chars only
            while ( (!_s.at_end()) &&
                    (is_ident_continue(_s.peek())) )
            {
                _s.advance_tracking(1);
            }

            if (_s.offset == start + 1u)
            {
                return parse_result<css_value>::make_error(
                    parse::DParseStatusFailure,
                    start,
                    "empty hash token"
                );
            }

            css_value v;
            v.kind = DCssValueKindHash;
            v.text = std::string(_s.data + start + 1,
                                 _s.offset - start - 1);

            return parse_result<css_value>(v);
        }

        // ident or function call
        if ( (is_ident_start(c)) ||
             ( (c == '-') && (is_ident_start(_s.peek_at(1))) ) )
        {
            auto id = parse_ident(_s);

            if (!id.ok())
            {
                return parse_result<css_value>(id.error());
            }

            // function call?
            if ( (!_s.at_end()) &&
                 (_s.peek() == '(') )
            {
                _s.advance_tracking(1); // consume '('

                auto args = parse_value_list_until(_s,
                                                   ')',
                                                   ')',
                                                   _depth + 1);

                if (!args.ok())
                {
                    return parse_result<css_value>(args.error());
                }

                skip_ws_and_comments(_s);

                if ( (_s.at_end()) ||
                     (_s.peek() != ')') )
                {
                    return parse_result<css_value>::make_error(
                        parse::DParseStatusFailure,
                        _s.offset,
                        "expected ')' in function call"
                    );
                }

                _s.advance_tracking(1); // consume ')'

                css_value v;
                v.kind = DCssValueKindFunction;
                v.text = id.value();
                v.args = args.value();

                return parse_result<css_value>(v);
            }

            css_value v;
            v.kind = DCssValueKindIdent;
            v.text = id.value();

            return parse_result<css_value>(v);
        }

        // delimiter ('/' and ',' are the common ones in values)
        if ( (c == '/') ||
             (c == ',') )
        {
            _s.advance_tracking(1);

            css_value v;
            v.kind = DCssValueKindDelim;
            v.text = std::string(1, c);

            return parse_result<css_value>(v);
        }

        return parse_result<css_value>::make_error(
            parse::DParseStatusFailure,
            _s.offset,
            "unexpected character in value"
        );
    }


    // ---- declarations ----------------------------------------

    // parse_declaration
    //   function: consumes a single property:value declaration,
    // optionally suffixed by `!important`.  The trailing `;` is
    // NOT consumed -- the caller handles it as a separator.
    inline parse_result<css_declaration>
    parse_declaration(text_parse_state& _s)
    {
        css_declaration decl;

        skip_ws_and_comments(_s);

        // property name
        auto name = parse_ident(_s);

        if (!name.ok())
        {
            return parse_result<css_declaration>(name.error());
        }

        decl.property = name.value();

        skip_ws_and_comments(_s);

        // mandatory ':'
        if ( (_s.at_end()) ||
             (_s.peek() != ':') )
        {
            return parse_result<css_declaration>::make_error(
                parse::DParseStatusFailure,
                _s.offset,
                "expected ':' after property name"
            );
        }

        _s.advance_tracking(1);

        // value list
        auto vl = parse_value_list_until(_s, ';', '}', 0);

        if (!vl.ok())
        {
            return parse_result<css_declaration>(vl.error());
        }

        decl.value = vl.value();

        skip_ws_and_comments(_s);

        // optional !important
        if ( (!_s.at_end()) &&
             (_s.peek() == '!') )
        {
            _s.advance_tracking(1);

            skip_ws_and_comments(_s);

            auto kw = parse_ident(_s);

            if ( (!kw.ok()) ||
                 (kw.value() != std::string("important")) )
            {
                return parse_result<css_declaration>::make_error(
                    parse::DParseStatusFailure,
                    _s.offset,
                    "expected 'important' after '!'"
                );
            }

            decl.important = true;

            skip_ws_and_comments(_s);
        }

        return parse_result<css_declaration>(decl);
    }


    // ---- selector pieces -------------------------------------

    // forward declarations for selector recursion (:not(...))
    inline parse_result<css_compound_selector>
    parse_compound(text_parse_state& _s);

    // parse_attribute_selector
    //   function: consumes a `[attr op value i?]` block, with the
    // opening `[` already consumed by the caller.
    inline parse_result<css_simple_selector>
    parse_attribute_selector(text_parse_state& _s)
    {
        css_simple_selector ss;

        ss.kind = DCssSimpleKindAttribute;

        skip_ws_and_comments(_s);

        auto name = parse_ident(_s);

        if (!name.ok())
        {
            return parse_result<css_simple_selector>(name.error());
        }

        ss.name = name.value();

        skip_ws_and_comments(_s);

        if (_s.at_end())
        {
            return parse_result<css_simple_selector>::make_error(
                parse::DParseStatusEndOfInput,
                _s.offset,
                "unterminated attribute selector"
            );
        }

        char c = _s.peek();

        // bare presence test: `[attr]`
        if (c == ']')
        {
            _s.advance_tracking(1);

            ss.attribute_op = DCssAttributeOpPresence;

            return parse_result<css_simple_selector>(ss);
        }

        // recognise the operator
        DCssAttributeOp op = DCssAttributeOpExact;

        if (c == '=')
        {
            _s.advance_tracking(1);
            op = DCssAttributeOpExact;
        }
        else if ( (_s.remaining() >= 2u) &&
                  (_s.peek_at(1) == '=') )
        {
            switch (c)
            {
                case '~': op = DCssAttributeOpWord;      break;
                case '|': op = DCssAttributeOpLang;      break;
                case '^': op = DCssAttributeOpPrefix;    break;
                case '$': op = DCssAttributeOpSuffix;    break;
                case '*': op = DCssAttributeOpSubstring; break;
                default:
                    return parse_result<css_simple_selector>::make_error(
                        parse::DParseStatusFailure,
                        _s.offset,
                        "unknown attribute selector operator"
                    );
            }

            _s.advance_tracking(2);
        }
        else
        {
            return parse_result<css_simple_selector>::make_error(
                parse::DParseStatusFailure,
                _s.offset,
                "expected attribute selector operator"
            );
        }

        ss.attribute_op = op;

        skip_ws_and_comments(_s);

        // value is either a string or an ident
        if (_s.at_end())
        {
            return parse_result<css_simple_selector>::make_error(
                parse::DParseStatusEndOfInput,
                _s.offset,
                "expected attribute value"
            );
        }

        char vc = _s.peek();

        if ( (vc == '"') ||
             (vc == '\'') )
        {
            auto str = parse_string(_s);

            if (!str.ok())
            {
                return parse_result<css_simple_selector>(str.error());
            }

            ss.attribute_value = str.value();
        }
        else
        {
            auto id = parse_ident(_s);

            if (!id.ok())
            {
                return parse_result<css_simple_selector>(id.error());
            }

            ss.attribute_value = id.value();
        }

        skip_ws_and_comments(_s);

        // optional case-insensitive flag (CSS Selectors 4: `i`)
        if ( (!_s.at_end()) &&
             ( (_s.peek() == 'i') || (_s.peek() == 'I') ) )
        {
            ss.attribute_case_insensitive = true;
            _s.advance_tracking(1);

            skip_ws_and_comments(_s);
        }

        if ( (_s.at_end()) ||
             (_s.peek() != ']') )
        {
            return parse_result<css_simple_selector>::make_error(
                parse::DParseStatusFailure,
                _s.offset,
                "expected ']' to close attribute selector"
            );
        }

        _s.advance_tracking(1);

        return parse_result<css_simple_selector>(ss);
    }

    // parse_simple
    //   function: consumes a single simple selector.  Recognises
    // type, *, #id, .class, [attr...], :pseudo, ::pseudo, and
    // :not(...).  Returns an error (with state unchanged) if no
    // simple selector starts at the current position.
    inline parse_result<css_simple_selector>
    parse_simple(text_parse_state& _s)
    {
        if (_s.at_end())
        {
            return parse_result<css_simple_selector>::make_error(
                parse::DParseStatusEndOfInput,
                _s.offset,
                "expected simple selector"
            );
        }

        char c = _s.peek();

        // universal
        if (c == '*')
        {
            _s.advance_tracking(1);

            css_simple_selector ss;
            ss.kind = DCssSimpleKindUniversal;

            return parse_result<css_simple_selector>(ss);
        }

        // id
        if (c == '#')
        {
            _s.advance_tracking(1);

            auto id = parse_ident(_s);

            if (!id.ok())
            {
                return parse_result<css_simple_selector>(id.error());
            }

            css_simple_selector ss;
            ss.kind = DCssSimpleKindId;
            ss.name = id.value();

            return parse_result<css_simple_selector>(ss);
        }

        // class
        if (c == '.')
        {
            _s.advance_tracking(1);

            auto id = parse_ident(_s);

            if (!id.ok())
            {
                return parse_result<css_simple_selector>(id.error());
            }

            css_simple_selector ss;
            ss.kind = DCssSimpleKindClass;
            ss.name = id.value();

            return parse_result<css_simple_selector>(ss);
        }

        // attribute
        if (c == '[')
        {
            _s.advance_tracking(1);

            return parse_attribute_selector(_s);
        }

        // pseudo (class or element)
        if (c == ':')
        {
            bool is_element = false;

            _s.advance_tracking(1);

            if ( (!_s.at_end()) &&
                 (_s.peek() == ':') )
            {
                _s.advance_tracking(1);
                is_element = true;
            }

            auto id = parse_ident(_s);

            if (!id.ok())
            {
                return parse_result<css_simple_selector>(id.error());
            }

            css_simple_selector ss;
            ss.name = id.value();

            // :not(...) is a special-cased pseudo-class that
            // hosts a nested compound selector.
            if ( (!is_element)                          &&
                 (id.value() == std::string("not"))     &&
                 (!_s.at_end())                         &&
                 (_s.peek() == '(') )
            {
                _s.advance_tracking(1); // consume '('

                skip_ws_and_comments(_s);

                auto inner = parse_compound(_s);

                if (!inner.ok())
                {
                    return parse_result<css_simple_selector>(
                        inner.error()
                    );
                }

                skip_ws_and_comments(_s);

                if ( (_s.at_end()) ||
                     (_s.peek() != ')') )
                {
                    return parse_result<css_simple_selector>::make_error(
                        parse::DParseStatusFailure,
                        _s.offset,
                        "expected ')' to close :not()"
                    );
                }

                _s.advance_tracking(1);

                ss.kind    = DCssSimpleKindNegation;
                ss.negated = std::make_shared<css_compound_selector>(
                    inner.value()
                );

                return parse_result<css_simple_selector>(ss);
            }

            // generic functional pseudo: capture argument as a
            // raw string for the host to interpret.
            if ( (!_s.at_end()) &&
                 (_s.peek() == '(') )
            {
                std::size_t arg_start;

                _s.advance_tracking(1); // consume '('

                arg_start = _s.offset;

                // simple greedy scan to matching ')'.  Nesting
                // and quoted strings inside pseudo args are not
                // common but we handle one level of parens.
                int depth = 1;

                while ( (!_s.at_end()) &&
                        (depth > 0) )
                {
                    char pc = _s.peek();

                    if (pc == '(')
                    {
                        depth += 1;
                    }
                    else if (pc == ')')
                    {
                        depth -= 1;

                        if (depth == 0)
                        {
                            break;
                        }
                    }

                    _s.advance_tracking(1);
                }

                if ( (_s.at_end()) ||
                     (_s.peek() != ')') )
                {
                    return parse_result<css_simple_selector>::make_error(
                        parse::DParseStatusFailure,
                        _s.offset,
                        "unterminated pseudo-class argument"
                    );
                }

                ss.pseudo_arg = std::string(
                    _s.data + arg_start,
                    _s.offset - arg_start
                );

                _s.advance_tracking(1); // consume ')'
            }

            ss.kind = is_element
                          ? DCssSimpleKindPseudoElement
                          : DCssSimpleKindPseudoClass;

            return parse_result<css_simple_selector>(ss);
        }

        // type selector (bare ident, possibly leading hyphen)
        if ( (is_ident_start(c)) ||
             ( (c == '-') && (is_ident_start(_s.peek_at(1))) ) )
        {
            auto id = parse_ident(_s);

            if (!id.ok())
            {
                return parse_result<css_simple_selector>(id.error());
            }

            css_simple_selector ss;
            ss.kind = DCssSimpleKindType;
            ss.name = id.value();

            return parse_result<css_simple_selector>(ss);
        }

        return parse_result<css_simple_selector>::make_error(
            parse::DParseStatusFailure,
            _s.offset,
            "no simple selector at position"
        );
    }

    // parse_compound
    //   function: consumes one or more simple selectors with no
    // whitespace between them.  Fails if no simple selector is
    // present at the current position.
    inline parse_result<css_compound_selector>
    parse_compound(text_parse_state& _s)
    {
        css_compound_selector cs;

        // first simple is mandatory
        auto first = parse_simple(_s);

        if (!first.ok())
        {
            return parse_result<css_compound_selector>(first.error());
        }

        cs.simples.push_back(first.value());

        // greedily collect adjacent simples until a non-simple
        // boundary is hit (whitespace, combinator, comma, '{', ...)
        for (;;)
        {
            if (_s.at_end())
            {
                break;
            }

            char c = _s.peek();

            // whitespace and structural chars terminate the
            // compound
            if ( (parse::char_predicate_is_whitespace::test(c)) ||
                 (c == ',')                                     ||
                 (c == '{')                                     ||
                 (c == '>')                                     ||
                 (c == '+')                                     ||
                 (c == '~')                                     ||
                 (c == ')') )
            {
                break;
            }

            auto next = parse_simple(_s);

            if (!next.ok())
            {
                break;
            }

            cs.simples.push_back(next.value());
        }

        return parse_result<css_compound_selector>(cs);
    }

    // parse_complex
    //   function: consumes a chain of compounds joined by
    // combinators.  Stops at ',', '{', or end-of-input.
    inline parse_result<css_complex_selector>
    parse_complex(text_parse_state& _s)
    {
        css_complex_selector cx;

        skip_ws_and_comments(_s);

        // head compound
        auto head = parse_compound(_s);

        if (!head.ok())
        {
            return parse_result<css_complex_selector>(head.error());
        }

        cx.head = head.value();

        // chain
        for (;;)
        {
            std::size_t before_ws = _s.offset;
            bool        had_ws    = false;

            // distinguish "whitespace then more selector" (descendant
            // combinator) from "whitespace at end of selector".
            while ( (!_s.at_end()) &&
                    (parse::char_predicate_is_whitespace::test(
                        _s.peek())) )
            {
                _s.advance_tracking(1);
                had_ws = true;
            }

            // also tolerate comments here
            if ( (_s.remaining() >= 2u)              &&
                 (_s.data[_s.offset]     == '/')     &&
                 (_s.data[_s.offset + 1] == '*') )
            {
                skip_ws_and_comments(_s);
                had_ws = true;
            }

            if (_s.at_end())
            {
                break;
            }

            char c = _s.peek();

            // end of selector -- caller resumes
            if ( (c == ',') ||
                 (c == '{') ||
                 (c == ')') )
            {
                break;
            }

            DCssCombinator       combinator = DCssCombinatorDescendant;
            css_complex_step    step;

            if ( (c == '>') ||
                 (c == '+') ||
                 (c == '~') )
            {
                _s.advance_tracking(1);

                combinator = (c == '>') ? DCssCombinatorChild
                           : (c == '+') ? DCssCombinatorAdjacentSibling
                                        : DCssCombinatorGeneralSibling;

                skip_ws_and_comments(_s);
            }
            else if (had_ws)
            {
                combinator = DCssCombinatorDescendant;
            }
            else
            {
                // no whitespace and no explicit combinator -- but
                // we did not match a structural terminator either.
                // Restore the offset and bail (the caller decides).
                _s.offset = before_ws;
                break;
            }

            auto comp = parse_compound(_s);

            if (!comp.ok())
            {
                return parse_result<css_complex_selector>(comp.error());
            }

            step.combinator = combinator;
            step.compound   = comp.value();

            cx.tail.push_back(step);
        }

        return parse_result<css_complex_selector>(cx);
    }


    // ---- specificity -----------------------------------------

    // pack_specificity
    //   function: composes the three specificity component counts
    // into a single uint32_t with saturation at 255 per byte.
    D_STATIC_CONSTEXPR_INLINE std::uint32_t
    pack_specificity(unsigned _a,
                     unsigned _b,
                     unsigned _c)
    {
        return ( ( ((_a > 255u) ? 255u : _a) << 16 ) |
                 ( ((_b > 255u) ? 255u : _b) <<  8 ) |
                 ( ((_c > 255u) ? 255u : _c)       ) );
    }

    // accumulate_compound_specificity
    //   function: walks every simple inside _cs and bumps the
    // appropriate specificity component counters.
    inline void
    accumulate_compound_specificity(const css_compound_selector& _cs,
                                    unsigned&                    _a,
                                    unsigned&                    _b,
                                    unsigned&                    _c)
    {
        std::size_t i;

        for (i = 0u; i < _cs.simples.size(); ++i)
        {
            const css_simple_selector& s = _cs.simples[i];

            switch (s.kind)
            {
                case DCssSimpleKindUniversal:
                    // universal contributes 0
                    break;

                case DCssSimpleKindType:
                case DCssSimpleKindPseudoElement:
                    _c += 1u;
                    break;

                case DCssSimpleKindClass:
                case DCssSimpleKindAttribute:
                case DCssSimpleKindPseudoClass:
                    _b += 1u;
                    break;

                case DCssSimpleKindId:
                    _a += 1u;
                    break;

                case DCssSimpleKindNegation:
                    // :not() contributes the specificity of its
                    // argument, with the wrapper itself adding 0.
                    if (s.negated)
                    {
                        accumulate_compound_specificity(*s.negated,
                                                        _a,
                                                        _b,
                                                        _c);
                    }
                    break;
            }
        }

        return;
    }

    // compute_specificity
    //   function: computes and stores the packed specificity for
    // a complex selector.
    inline void
    compute_specificity(css_complex_selector& _cx)
    {
        unsigned a = 0u;
        unsigned b = 0u;
        unsigned c = 0u;
        std::size_t i;

        accumulate_compound_specificity(_cx.head, a, b, c);

        for (i = 0u; i < _cx.tail.size(); ++i)
        {
            accumulate_compound_specificity(_cx.tail[i].compound,
                                            a,
                                            b,
                                            c);
        }

        _cx.specificity = pack_specificity(a, b, c);

        return;
    }


    // ---- selector list ---------------------------------------

    // parse_selector_list
    //   function: consumes a comma-separated list of complex
    // selectors.  Stops at '{' or end-of-input.
    inline parse_result<css_selector_list>
    parse_selector_list(text_parse_state& _s)
    {
        css_selector_list list;

        for (;;)
        {
            auto cx = parse_complex(_s);

            if (!cx.ok())
            {
                return parse_result<css_selector_list>(cx.error());
            }

            css_complex_selector resolved = cx.value();
            compute_specificity(resolved);

            list.selectors.push_back(resolved);

            skip_ws_and_comments(_s);

            if (_s.at_end())
            {
                break;
            }

            if (_s.peek() != ',')
            {
                break;
            }

            _s.advance_tracking(1);

            skip_ws_and_comments(_s);
        }

        return parse_result<css_selector_list>(list);
    }


    // ---- declaration block -----------------------------------

    // skip_to_recovery_point
    //   function: advances past the next ';' or '}' (whichever
    // comes first) for error-recovery in a declaration block.
    inline void
    skip_to_recovery_point(text_parse_state& _s)
    {
        while (!_s.at_end())
        {
            char c = _s.peek();

            if (c == ';')
            {
                _s.advance_tracking(1);

                return;
            }

            if (c == '}')
            {
                // do not consume '}' -- caller handles it
                return;
            }

            // tolerate strings and parens to avoid skipping into
            // a brace that lives inside a quoted value
            if ( (c == '"') ||
                 (c == '\'') )
            {
                auto _ = parse_string(_s);
                (void)_;
                continue;
            }

            _s.advance_tracking(1);
        }

        return;
    }

    // parse_declaration_block
    //   function: consumes `{ decl ; decl ; ... }` and returns
    // the decls.  The opening `{` must be the next non-ws char.
    inline parse_result<std::vector<css_declaration>>
    parse_declaration_block(text_parse_state& _s)
    {
        std::vector<css_declaration> decls;

        skip_ws_and_comments(_s);

        if ( (_s.at_end()) ||
             (_s.peek() != '{') )
        {
            return parse_result<std::vector<css_declaration>>::make_error(
                parse::DParseStatusFailure,
                _s.offset,
                "expected '{'"
            );
        }

        _s.advance_tracking(1);

        for (;;)
        {
            skip_ws_and_comments(_s);

            if (_s.at_end())
            {
                return parse_result<std::vector<css_declaration>>::make_error(
                    parse::DParseStatusEndOfInput,
                    _s.offset,
                    "unterminated declaration block"
                );
            }

            if (_s.peek() == '}')
            {
                _s.advance_tracking(1);

                break;
            }

            // tolerate stray ';'
            if (_s.peek() == ';')
            {
                _s.advance_tracking(1);
                continue;
            }

            auto decl = parse_declaration(_s);

            if (decl.ok())
            {
                decls.push_back(decl.value());
            }
            else
            {
                // recover: skip to ';' or '}' and continue
                skip_to_recovery_point(_s);
            }

            skip_ws_and_comments(_s);

            if ( (!_s.at_end()) &&
                 (_s.peek() == ';') )
            {
                _s.advance_tracking(1);
            }
        }

        return parse_result<std::vector<css_declaration>>(decls);
    }


    // ---- style rule ------------------------------------------

    // parse_style_rule
    //   function: consumes a `selector_list { declarations }`.
    inline parse_result<css_style_rule>
    parse_style_rule(text_parse_state& _s)
    {
        css_style_rule rule;

        auto sels = parse_selector_list(_s);

        if (!sels.ok())
        {
            return parse_result<css_style_rule>(sels.error());
        }

        rule.selectors = sels.value();

        auto block = parse_declaration_block(_s);

        if (!block.ok())
        {
            return parse_result<css_style_rule>(block.error());
        }

        rule.declarations = block.value();

        return parse_result<css_style_rule>(rule);
    }


    // ---- at-rule ---------------------------------------------

    // parse_at_rule
    //   function: consumes `@name prelude (; | { body })`.  The
    // `@` must be the next non-ws char on entry.  The body, when
    // present, is parsed as either a list of nested style rules
    // (if it contains `{`-introduced sub-rules) or a list of
    // declarations (if its first non-ws content looks like a
    // property:value pair).
    inline parse_result<css_at_rule>
    parse_at_rule(text_parse_state& _s)
    {
        css_at_rule at;

        if ( (_s.at_end()) ||
             (_s.peek() != '@') )
        {
            return parse_result<css_at_rule>::make_error(
                parse::DParseStatusFailure,
                _s.offset,
                "expected '@'"
            );
        }

        _s.advance_tracking(1);

        auto name = parse_ident(_s);

        if (!name.ok())
        {
            return parse_result<css_at_rule>(name.error());
        }

        at.name = name.value();

        // prelude: a value list ending at ';' or '{'
        auto prelude = parse_value_list_until(_s, ';', '{', 0);

        if (!prelude.ok())
        {
            return parse_result<css_at_rule>(prelude.error());
        }

        at.prelude = prelude.value();

        skip_ws_and_comments(_s);

        if (_s.at_end())
        {
            return parse_result<css_at_rule>(at);
        }

        // statement form: @name prelude ;
        if (_s.peek() == ';')
        {
            _s.advance_tracking(1);
            at.has_block = false;

            return parse_result<css_at_rule>(at);
        }

        // block form: @name prelude { ... }
        if (_s.peek() != '{')
        {
            return parse_result<css_at_rule>::make_error(
                parse::DParseStatusFailure,
                _s.offset,
                "expected ';' or '{' after at-rule prelude"
            );
        }

        at.has_block = true;

        // Heuristic:  peek inside the block.  If we see a `{`
        // before any `;` then this is a nested-rules block
        // (e.g. @media); otherwise it is a declarations block
        // (e.g. @font-face).  We do this with bounded scanning
        // so the cost stays linear.
        bool        nested_rules;
        std::size_t scan;
        int         depth;

        nested_rules = false;
        scan         = _s.offset + 1u; // skip the '{'
        depth        = 0;

        while (scan < _s.length)
        {
            char sc = _s.data[scan];

            if (sc == '"' || sc == '\'')
            {
                // skip quoted string
                char q = sc;

                scan += 1u;

                while ( (scan < _s.length) &&
                        (_s.data[scan] != q) )
                {
                    if ( (_s.data[scan] == '\\') &&
                         (scan + 1u < _s.length) )
                    {
                        scan += 2u;
                    }
                    else
                    {
                        scan += 1u;
                    }
                }

                if (scan < _s.length)
                {
                    scan += 1u;
                }

                continue;
            }

            if (sc == '{')
            {
                if (depth == 0)
                {
                    nested_rules = true;
                    break;
                }

                depth += 1;
            }
            else if (sc == '}')
            {
                if (depth == 0)
                {
                    break;
                }

                depth -= 1;
            }
            else if (sc == ';')
            {
                if (depth == 0)
                {
                    break;
                }
            }

            scan += 1u;
        }

        // commit the block parse
        if (nested_rules)
        {
            // consume '{', then parse a sequence of style rules
            // until we hit '}'.
            _s.advance_tracking(1);

            for (;;)
            {
                skip_ws_and_comments(_s);

                if (_s.at_end())
                {
                    return parse_result<css_at_rule>::make_error(
                        parse::DParseStatusEndOfInput,
                        _s.offset,
                        "unterminated at-rule body"
                    );
                }

                if (_s.peek() == '}')
                {
                    _s.advance_tracking(1);
                    break;
                }

                auto sub = parse_style_rule(_s);

                if (!sub.ok())
                {
                    // recover by scanning to next '}'
                    while ( (!_s.at_end()) &&
                            (_s.peek() != '}') )
                    {
                        _s.advance_tracking(1);
                    }

                    if (!_s.at_end())
                    {
                        _s.advance_tracking(1);
                    }

                    break;
                }

                at.rules.push_back(sub.value());
            }
        }
        else
        {
            // declarations block
            auto block = parse_declaration_block(_s);

            if (!block.ok())
            {
                return parse_result<css_at_rule>(block.error());
            }

            at.declarations = block.value();
        }

        return parse_result<css_at_rule>(at);
    }


    // ---- stylesheet ------------------------------------------

    // parse_stylesheet
    //   function: top-level driver.  Consumes a sequence of style
    // rules and at-rules, performing block-level error recovery
    // so that one malformed rule does not poison the whole input.
    inline parse_result<css_stylesheet>
    parse_stylesheet(text_parse_state& _s)
    {
        css_stylesheet sheet;

        // skip BOM
        if ( (_s.remaining() >= 3u)                                  &&
             (static_cast<unsigned char>(_s.data[_s.offset])     == 0xefu) &&
             (static_cast<unsigned char>(_s.data[_s.offset + 1]) == 0xbbu) &&
             (static_cast<unsigned char>(_s.data[_s.offset + 2]) == 0xbfu) )
        {
            _s.advance_tracking(3);
        }

        for (;;)
        {
            skip_ws_and_comments(_s);

            if (_s.at_end())
            {
                break;
            }

            if (_s.peek() == '@')
            {
                auto at = parse_at_rule(_s);

                if (at.ok())
                {
                    sheet.at_rules.push_back(at.value());
                }
                else
                {
                    // recover: scan to next ';' or matching '}'
                    while (!_s.at_end())
                    {
                        char rc = _s.peek();
                        _s.advance_tracking(1);

                        if (rc == ';' || rc == '}')
                        {
                            break;
                        }
                    }
                }

                continue;
            }

            auto rule = parse_style_rule(_s);

            if (rule.ok())
            {
                sheet.rules.push_back(rule.value());
            }
            else
            {
                // recover: scan to closing '}'
                while ( (!_s.at_end()) &&
                        (_s.peek() != '}') )
                {
                    _s.advance_tracking(1);
                }

                if (!_s.at_end())
                {
                    _s.advance_tracking(1);
                }
            }
        }

        return parse_result<css_stylesheet>(sheet);
    }

NS_END  // internal


// ================================================================
//  css_selector_parser
// ================================================================

// css_selector_parser
//   class: parses a single comma-separated list of complex
// selectors with no surrounding declaration block.  Suitable for
// the CSS_OM `querySelector(...)` use-case where the caller
// supplies just the selector text.
class css_selector_parser
    : public parse::text_parser_base<css_selector_parser>
{
public:
    using result_type = css_selector_list;

    // do_parse
    //   member: parses a selector list and returns it; trailing
    // whitespace is consumed but trailing non-selector text is
    // NOT.
    parse::parse_result<css_selector_list>
    do_parse(parse::text_parse_state& _state)
    {
        internal::skip_ws_and_comments(_state);

        return internal::parse_selector_list(_state);
    }
};


// ================================================================
//  css_declaration_parser
// ================================================================

// css_declaration_parser
//   class: parses a sequence of declarations terminated by ';'
// or end-of-input -- the shape of an inline `style="..."`
// attribute value (without the surrounding `{}`).
class css_declaration_parser
    : public parse::text_parser_base<css_declaration_parser>
{
public:
    using result_type = std::vector<css_declaration>;

    // do_parse
    //   member: parses zero or more declarations.  Malformed
    // declarations are skipped via local error recovery.
    parse::parse_result<std::vector<css_declaration>>
    do_parse(parse::text_parse_state& _state)
    {
        std::vector<css_declaration> decls;

        for (;;)
        {
            internal::skip_ws_and_comments(_state);

            if (_state.at_end())
            {
                break;
            }

            if (_state.peek() == ';')
            {
                _state.advance_tracking(1);
                continue;
            }

            auto decl = internal::parse_declaration(_state);

            if (decl.ok())
            {
                decls.push_back(decl.value());
            }
            else
            {
                internal::skip_to_recovery_point(_state);
            }

            internal::skip_ws_and_comments(_state);

            if ( (!_state.at_end()) &&
                 (_state.peek() == ';') )
            {
                _state.advance_tracking(1);
            }
        }

        return parse::parse_result<std::vector<css_declaration>>(
            decls
        );
    }
};


// ================================================================
//  css_stylesheet_parser
// ================================================================

// css_stylesheet_parser
//   class: parses a complete CSS stylesheet.  This is the
// typical entry point: construct an instance and feed it the
// source via `parse(text_parse_state&)` (inherited from
// text_parser_base) or one of its convenience overloads.
class css_stylesheet_parser
    : public parse::text_parser_base<css_stylesheet_parser>
{
public:
    using result_type = css_stylesheet;

    // do_parse
    //   member: parses a full stylesheet using error recovery
    // so that the entire input is always consumed.
    parse::parse_result<css_stylesheet>
    do_parse(parse::text_parse_state& _state)
    {
        return internal::parse_stylesheet(_state);
    }
};


NS_END  // css
NS_END  // djinterp


#endif  // DJINTERP_CSS_PARSER_
