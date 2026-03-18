/******************************************************************************
* djinterp [core]                                             text_parser.hpp
*
* Text parser specializations:
*   This header extends the generic parser framework with types and
* utilities specific to parsing character-based (text) input.  It
* provides:
*   - text_parse_state     — a parse_state<char> with line/column tracking
*   - text_parser_base     — CRTP base that threads text_parse_state through
*                            the derived parser's do_parse
*   - text_match           — result descriptor carrying the matched span
*   - character predicate helpers for common matching patterns
*
*
* path:      /inc/cpp/parse/text_parser.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2025.03.15
******************************************************************************/

#ifndef DJINTERP_TEXT_PARSER_
#define DJINTERP_TEXT_PARSER_ 1

#include <cstddef>
#include <type_traits>
#include "./parser.hpp"


NS_DJINTERP
NS_PARSE


// ================================================================
//  text_match
// ================================================================

// text_match
//   struct: describes a successfully matched span of text,
// recording the start offset and the length of the match.
struct text_match
{
    std::size_t     start;
    std::size_t     length;

    text_match()
        : start  (0)
        , length (0)
    {
    }

    text_match(std::size_t _start,
               std::size_t _length)
        : start  (_start)
        , length (_length)
    {
    }
};


// ================================================================
//  text_position
// ================================================================

// text_position
//   struct: human-readable position within a text stream,
// tracking line number and column (both 1-based).
struct text_position
{
    std::size_t     line;
    std::size_t     column;

    text_position()
        : line   (1)
        , column (1)
    {
    }

    text_position(std::size_t _line,
                  std::size_t _column)
        : line   (_line)
        , column (_column)
    {
    }
};


// ================================================================
//  text_parse_state
// ================================================================

// text_parse_state
//   struct: extends parse_state<char> with line/column tracking.
// Maintains a text_position that is updated as the cursor
// advances through the input.
struct text_parse_state : public parse_state<char>
{
private:
    using base_type = parse_state<char>;

public:
    using element_type = char;

    text_position   position;

    text_parse_state()
        : base_type()
        , position ()
    {
    }

    text_parse_state(const char* _data,
                     std::size_t _length,
                     std::size_t _offset = 0)
        : base_type(_data, _length, _offset)
        , position ()
    {
    }

    // advance_tracking
    //   advances the cursor by _count characters, updating the
    // line and column counters for each newline encountered.
    void advance_tracking(std::size_t _count = 1)
    {
        for (std::size_t i = 0; i < _count; ++i)
        {
            if (at_end())
            {
                break;
            }

            if (data[offset] == '\n')
            {
                position.line   += 1;
                position.column  = 1;
            }
            else
            {
                position.column += 1;
            }

            offset += 1;
        }

        return;
    }

    // peek
    //   returns the character at the current offset, or '\0' if
    // at the end.
    char peek() const
    {
        return at_end()
                    ? '\0'
                    : data[offset];
    }

    // peek_at
    //   returns the character at offset + _ahead, or '\0' if
    // out of bounds.
    char peek_at(std::size_t _ahead) const
    {
        std::size_t target = offset + _ahead;

        return (target < length)
                    ? data[target]
                    : '\0';
    }
};


// ================================================================
//  character predicates
// ================================================================
// Stateless predicate types suitable for use as template
// parameters.  Each exposes a static `test(char)` method.

// char_predicate_is_digit
//   predicate: matches ASCII decimal digits '0'-'9'.
struct char_predicate_is_digit
{
    D_STATIC_CONSTEXPR_INLINE bool test(char _c)
    {
        return (_c >= '0' && _c <= '9');
    }
};

// char_predicate_is_alpha
//   predicate: matches ASCII alphabetic characters.
struct char_predicate_is_alpha
{
    D_STATIC_CONSTEXPR_INLINE bool test(char _c)
    {
        return ( (_c >= 'a' && _c <= 'z') ||
                 (_c >= 'A' && _c <= 'Z') );
    }
};

// char_predicate_is_alnum
//   predicate: matches ASCII alphanumeric characters.
struct char_predicate_is_alnum
{
    D_STATIC_CONSTEXPR_INLINE bool test(char _c)
    {
        return ( char_predicate_is_alpha::test(_c) ||
                 char_predicate_is_digit::test(_c) );
    }
};

// char_predicate_is_whitespace
//   predicate: matches ASCII whitespace characters.
struct char_predicate_is_whitespace
{
    D_STATIC_CONSTEXPR_INLINE bool test(char _c)
    {
        return ( _c == ' '  ||
                 _c == '\t' ||
                 _c == '\n' ||
                 _c == '\r' ||
                 _c == '\f' ||
                 _c == '\v' );
    }
};

// char_predicate_is_hex_digit
//   predicate: matches hexadecimal digit characters.
struct char_predicate_is_hex_digit
{
    D_STATIC_CONSTEXPR_INLINE bool test(char _c)
    {
        return ( (_c >= '0' && _c <= '9') ||
                 (_c >= 'a' && _c <= 'f') ||
                 (_c >= 'A' && _c <= 'F') );
    }
};

// char_predicate_exact
//   predicate: matches a single compile-time character constant.
template<char _C>
struct char_predicate_exact
{
    D_STATIC_CONSTEXPR_INLINE bool test(char _c)
    {
        return (_c == _C);
    }
};


// ================================================================
//  text_parser_base
// ================================================================

// text_parser_base
//   class: CRTP base for text parsers.  Threads a
// text_parse_state (with line/column tracking) through the
// derived parser's do_parse.
//
//   _Derived must expose:
//     - `using result_type = ...;`
//     - `parse_result<result_type>
//        do_parse(text_parse_state&);`
template<typename _Derived>
class text_parser_base
{
private:
    using derived_type = _Derived;

    derived_type& self()
    {
        return static_cast<derived_type&>(*this);
    }

    const derived_type& self() const
    {
        return static_cast<const derived_type&>(*this);
    }

protected:
    text_parser_base()
    {
    }

    ~text_parser_base()
    {
    }

public:
    using input_type = char;

    // parse
    //   delegates to the derived do_parse with a text_parse_state.
    auto parse(text_parse_state& _state)
        -> parse_result<typename derived_type::result_type>
    {
        return self().do_parse(_state);
    }

    // parse (convenience — raw pointer + length)
    //   constructs a text_parse_state and delegates.
    auto parse(const char* _data,
               std::size_t _length)
        -> parse_result<typename derived_type::result_type>
    {
        text_parse_state state(_data, _length);

        return parse(state);
    }
};


// ================================================================
//  built-in text parsers
// ================================================================

// match_while
//   parser: greedily consumes characters satisfying _Predicate.
// Succeeds (possibly with an empty match) unconditionally.
template<typename _Predicate>
class match_while : public text_parser_base<match_while<_Predicate>>
{
public:
    using result_type = text_match;

    parse_result<text_match> do_parse(text_parse_state& _state)
    {
        std::size_t start = _state.offset;

        while ( (!_state.at_end()) &&
                (_Predicate::test(_state.peek())) )
        {
            _state.advance_tracking(1);
        }

        return parse_result<text_match>(
            text_match(start, _state.offset - start)
        );
    }
};

// match_while1
//   parser: greedily consumes one or more characters satisfying
// _Predicate.  Fails if no character matches.
template<typename _Predicate>
class match_while1 : public text_parser_base<match_while1<_Predicate>>
{
public:
    using result_type = text_match;

    parse_result<text_match> do_parse(text_parse_state& _state)
    {
        std::size_t start = _state.offset;

        while ( (!_state.at_end()) &&
                (_Predicate::test(_state.peek())) )
        {
            _state.advance_tracking(1);
        }

        if (_state.offset == start)
        {
            return parse_result<text_match>::make_error(
                DParseStatusFailure,
                start,
                "expected at least one matching character"
            );
        }

        return parse_result<text_match>(
            text_match(start, _state.offset - start)
        );
    }
};

// match_literal
//   parser: matches an exact string literal.  The literal is
// provided at construction time and must outlive the parser.
class match_literal : public text_parser_base<match_literal>
{
public:
    using result_type = text_match;

    match_literal(const char* _literal,
                  std::size_t _length)
        : m_literal (_literal)
        , m_length  (_length)
    {
    }

    parse_result<text_match> do_parse(text_parse_state& _state)
    {
        if (_state.remaining() < m_length)
        {
            return parse_result<text_match>::make_error(
                DParseStatusEndOfInput,
                _state.offset,
                "insufficient input for literal match"
            );
        }

        std::size_t start = _state.offset;

        for (std::size_t i = 0; i < m_length; ++i)
        {
            if (_state.data[_state.offset + i] != m_literal[i])
            {
                return parse_result<text_match>::make_error(
                    DParseStatusFailure,
                    _state.offset + i,
                    "literal mismatch"
                );
            }
        }

        _state.advance_tracking(m_length);

        return parse_result<text_match>(
            text_match(start, m_length)
        );
    }

private:
    const char*     m_literal;
    std::size_t     m_length;
};


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_TEXT_PARSER_
