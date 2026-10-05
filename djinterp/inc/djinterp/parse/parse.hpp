/*******************************************************************************
* djinterp [parse]                                                     parse.hpp
*
* Common primitives of the parsing subframework.
*   Per the formal definition in ch-parsing.tex, a parser is a function
*
*       P A = Σ* → maybe⟨A × Σ*⟩          (the maybe arm)
*           = Σ* → result⟨A × Σ*, E⟩      (the result arm, with typed E)
*
* and the parsing subframework's job is to carry that function value-
* semantically and to expose it as an instance of the four protocols of
* the functional companion (Functor, Applicative, Alternative, Monad).
* This header carries only the support types that face into the parser:
*
*   parse_state<E>    the surface stream Σ*, threaded by reference as the
*                     operational counterpart of the formal residual.  The
*                     parser receives the state, may advance `offset`, and
*                     returns the produced value; on Alternative failure
*                     the offset is restored by `alt` so the caller sees
*                     the formal `match-or-restore` semantics.
*
*   parse_error       the error type E in the `result` arm.  Value-
*                     semantic, copyable without lifetime caveats.
*
*   parse_result<T>   a thin refinement of functional::result<T,
*                     parse_error>: it IS a result and inherits the
*                     monadic surface (map, and_then, or_else, match,
*                     value_or, operator|, the protocol specialisations
*                     in result.hpp).  The legacy ok()/value()/error()/
*                     make_ok()/make_error() face is preserved so the
*                     formal definition's `maybe⟨A × Σ*⟩` reads cleanly
*                     at the call site.
*
*   parseable / parse_traits     minor (token) ↔ major (aggregate) mapping
*   parse_status                 integral outcome classifier + codes
*
*   The parser carrier itself lives in parser/parser.hpp.  Combinators
* over it (Functor/Applicative/Alternative/Monad), atomic parsers, and
* the compose-parse prism live alongside it under parser/.  The grammar
* tuple (N, Σ, P, S) and the polynomial functor F whose initial algebra
* μF is the parsable carrier live in grammar/.
*
*
* path:      /inc/djinterp/parse/parse.hpp
* link(s):   ch-parsing.tex
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.11
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_PARSE_PARSE_HPP
#define DJINTERP_PARSE_PARSE_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"
#include "../core/meta/type_utility.hpp"  // void_t
#include "./substrate.hpp"  // NS_PARSE
#include "../core/functional/result.hpp"
// re_std
#include "../../re_std/cstdint/cstdint.hpp"  // re_std::int32_t


NS_DJINTERP
NS_PARSE


// ================================================================
//  I.   parseable  /  parse_traits
// ================================================================

// parseable
//   trait: associates a minor (token) type with a major (parsed
// aggregate) type for a given parseable domain.
template<typename Minor,
         typename Major>
struct parseable
{
    using minor = Minor;
    using major = Major;
};

// is_parseable
//   trait: primary template — not parseable by default.
template<typename Type,
         typename = void>
struct is_parseable : std::false_type
{};

// is_parseable (SFINAE specialisation)
//   trait: true when Type exposes nested minor and major aliases.
template<typename Type>
struct is_parseable<
    Type,
    void_t<typename Type::minor,
           typename Type::major>
> : std::true_type
{};

// is_parseable<std::string>
//   trait: std::string is parseable (chars → strings).
template<>
struct is_parseable<std::string, void> : std::true_type
{};

// is_parseable<const char*>
//   trait: const char* is parseable (chars → C-strings).
template<>
struct is_parseable<const char*, void> : std::true_type
{};

// is_parseable<char*>
//   trait: char* is parseable (chars → C-strings).
template<>
struct is_parseable<char*, void> : std::true_type
{};

// parse_traits
//   trait: primary template — maps a parseable type to its minor/major pair.
template<typename Type>
struct parse_traits;

// parse_traits<std::string>
//   trait: std::string parses from char to std::string.
template<>
struct parse_traits<std::string> : parseable<char, std::string>
{};

// parse_traits<const char*>
//   trait: const char* parses from char to const char*.
template<>
struct parse_traits<const char*> : parseable<char, const char*>
{};

// parse_traits<char*>
//   trait: char* parses from char to char*.
template<>
struct parse_traits<char*> : parseable<char, char*>
{};


// ================================================================
//  II.  parse_status
// ================================================================

// parse_status
//   typedef: classifies the outcome of a parse operation.
typedef re_std::int32_t parse_status;

// DParseStatus*
//   constants: standard parse status codes.  Derived parsers may
// define additional codes above DParseStatusUserBase.
D_CONSTEXPR parse_status DParseStatusSuccess    =  0;
D_CONSTEXPR parse_status DParseStatusFailure    =  1;
D_CONSTEXPR parse_status DParseStatusEndOfInput =  2;
D_CONSTEXPR parse_status DParseStatusOverflow   =  3;
D_CONSTEXPR parse_status DParseStatusMalformed  =  4;
D_CONSTEXPR parse_status DParseStatusUserBase   = 64;


// ================================================================
//  III. parse_error
// ================================================================

// parse_error
//   class: value-semantic descriptor of a parse failure — the input
// offset at which it occurred, a status code, and a human-readable
// message.  The message is an owning std::string so a parse_error
// can be safely copied without lifetime caveats; this is the E in
// the formal `result⟨A × Σ*, E⟩` arm of the parser carrier.
class parse_error
{
public:
    parse_error()
        : m_status (DParseStatusFailure),
          m_offset (0),
          m_message()
    {}

    parse_error(
        parse_status       _status,
        std::size_t        _offset,
        const std::string& _message = std::string()
    )
        : m_status (_status),
          m_offset (_offset),
          m_message(_message)
    {}

    parse_error(
        parse_status _status,
        std::size_t  _offset,
        const char*  _message
    )
        : m_status (_status),
          m_offset (_offset),
          m_message(_message ? _message : "")
    {}

    // status
    //   method: the status code classifying the failure.
    D_NODISCARD
    parse_status status() const
    {
        return m_status;
    }

    // offset
    //   method: the input offset at which the failure occurred.
    D_NODISCARD
    std::size_t offset() const
    {
        return m_offset;
    }

    // message
    //   method: the human-readable description of the failure.
    D_NODISCARD
    const std::string& message() const
    {
        return m_message;
    }

private:
    parse_status m_status;
    std::size_t  m_offset;
    std::string  m_message;
};

// operator== (parse_error)
//   function: two errors are equal iff status, offset, and message
// all match.  Enables parse_result equality (result<T, E> requires
// == on E).
inline bool
operator==(
    const parse_error& _a,
    const parse_error& _b
)
{
    return ( (_a.status()  == _b.status())  &&
             (_a.offset()  == _b.offset())  &&
             (_a.message() == _b.message()) );
}

// operator!= (parse_error)
//   function: negation of operator==.
inline bool
operator!=(
    const parse_error& _a,
    const parse_error& _b
)
{
    return (!(_a == _b));
}


// ================================================================
//  IV.  parse_result
// ================================================================

// parse_result
//   class: the outcome of a fallible parse — a value of type
// ValueType (success) or a parse_error (failure).  A refinement of
// functional::result<ValueType, parse_error>: it IS a result and
// inherits the whole monadic surface (map, and_then, or_else,
// match, value_or, operator|, plus the functor / monad protocol
// specialisations defined in result.hpp).  This is the C++ shape
// of the formal `result⟨A × Σ*, E⟩` arm of the parser carrier; the
// `× Σ*` part is threaded through the parse_state reference the
// parser is given rather than packed into the return.
//
//   The compact legacy face is preserved so call sites read as the
// formal definition does:
//
//     parse_result(value)         implicit success construction
//     parse_result(error)         implicit failure construction
//     .ok()                       success predicate
//     .value()                    contained value
//     .error()                    contained error
//     parse_result::make_ok(v)    success factory
//     parse_result::make_error(.) failure factory from fields
//
//   The inherited result<>::ok() returning maybe<T> is shadowed
// here by the boolean predicate the parse vocabulary wants; the
// inherited form is reachable via functional::to_maybe on the base.
template<typename ValueType>
class parse_result
    : public functional::result<ValueType, parse_error>
{
private:
    using base_type = functional::result<ValueType, parse_error>;

public:
    using value_type = ValueType;
    using error_type = parse_error;

    parse_result(
        const ValueType& _value
    )
        : base_type(
              functional::internal::ok_tag(
                  functional::internal::ok_tag::construct_tag()),
              _value)
    {}

    parse_result(
        ValueType&& _value
    )
        : base_type(
              functional::internal::ok_tag(
                  functional::internal::ok_tag::construct_tag()),
              static_cast<ValueType&&>(_value))
    {}

    parse_result(
        const parse_error& _error
    )
        : base_type(
              functional::internal::err_tag(
                  functional::internal::err_tag::construct_tag()),
              _error)
    {}

    parse_result(
        parse_error&& _error
    )
        : base_type(
              functional::internal::err_tag(
                  functional::internal::err_tag::construct_tag()),
              static_cast<parse_error&&>(_error))
    {}

    parse_result(
        const base_type& _base
    )
        : base_type(_base)
    {}

    parse_result(
        base_type&& _base
    )
        : base_type(static_cast<base_type&&>(_base))
    {}

    // ok
    //   method: returns true on success.  Shadows the inherited
    // result::ok() (which returns maybe<T>) with the boolean
    // predicate the parse vocabulary expects.
    D_NODISCARD
    bool ok() const
    {
        return this->is_ok();
    }

    // value
    //   method: returns a reference to the contained value.
    //   Precondition: ok() == true.
    D_NODISCARD
    const ValueType& value() const
    {
        return base_type::value();
    }

    D_NODISCARD
    ValueType& value()
    {
        return base_type::value();
    }

    // error
    //   method: returns the contained error descriptor.
    //   Precondition: ok() == false.
    D_NODISCARD
    const parse_error& error() const
    {
        return base_type::error();
    }

    // make_ok
    //   factory: creates a successful parse_result.
    D_NODISCARD
    static parse_result
    make_ok(
        const ValueType& _value
    )
    {
        return parse_result(_value);
    }

    // make_error
    //   factory: creates a failed parse_result from raw fields.
    D_NODISCARD
    static parse_result
    make_error(
        parse_status       _status,
        std::size_t        _offset,
        const std::string& _message = std::string()
    )
    {
        return parse_result(parse_error(_status, _offset, _message));
    }

    D_NODISCARD
    static parse_result
    make_error(
        parse_status _status,
        std::size_t  _offset,
        const char*  _message
    )
    {
        return parse_result(parse_error(_status, _offset, _message));
    }
};


// ================================================================
//  V.   parse_state
// ================================================================

// parse_state
//   struct: the surface stream Σ* the parser consumes.  Tracks the
// position and remaining extent of a parse over an input of element
// type ElementType.  Threaded by reference through the parser
// function as the operational counterpart of the formal residual:
// a successful parse advances `offset`; Alternative's `alt`
// combinator saves and restores `offset` so failures don't leak
// consumed input to the next branch.
//
//   Agnostic to the nature of the data — character streams, byte
// buffers, token sequences all instantiate it.
template<typename ElementType>
struct parse_state
{
    using element_type = ElementType;

    const ElementType* data;
    std::size_t         length;
    std::size_t         offset;

    parse_state()
        : data   (nullptr),
          length (0),
          offset (0)
    {}

    parse_state(
        const ElementType* _data,
        std::size_t         _length,
        std::size_t         _offset = 0
    )
        : data   (_data),
          length (_length),
          offset (_offset)
    {}

    // remaining
    //   method: the number of elements still available.
    D_NODISCARD
    std::size_t remaining() const
    {
        return (offset < length)
                    ? (length - offset)
                    : 0;
    }

    // at_end
    //   method: true when no input remains.
    D_NODISCARD
    bool at_end() const
    {
        return (offset >= length);
    }

    // current
    //   method: a pointer to the current element, or null at end.
    D_NODISCARD
    const ElementType* current() const
    {
        return at_end()
                    ? nullptr
                    : (data + offset);
    }

    // advance
    //   method: moves the offset forward by _count elements, clamped
    // to the end of the input.
    void
    advance(
        std::size_t _count = 1
    )
    {
        offset += _count;

        if (offset > length)
        {
            offset = length;
        }

        return;
    }
};

// text_parse_state
//   struct: the parse state over characters that the text parsers (the
// CSS parser among them) thread: parse_state<char>, plus the character
// conveniences they use and a line and column, kept current as
// advance_tracking moves the cursor (counted from where the state began).
struct text_parse_state : parse_state<char>
{
    std::size_t line;    // 1-based line of the cursor
    std::size_t column;  // 1-based column of the cursor

    text_parse_state()
        : parse_state<char>(),
          line(1),
          column(1)
    {}

    text_parse_state(
        const char* _data,
        std::size_t _length,
        std::size_t _offset = 0
    )
        : parse_state<char>(_data, _length, _offset),
          line(1),
          column(1)
    {}

    // peek
    //   accessor: the character at the cursor, or '\0' at the end.
    D_NODISCARD char
    peek() const
    {
        return (offset < length) ? data[offset] : '\0';
    }

    // peek_at
    //   accessor: the character _ahead places past the cursor, or '\0'
    // past the end.
    D_NODISCARD char
    peek_at(
        std::size_t _ahead
    ) const
    {
        return ((offset + _ahead) < length) ? data[offset + _ahead] : '\0';
    }

    // advance_tracking
    //   mutator: moves the cursor _count characters, stopping at the end,
    // and keeps line and column current: a line feed starts a new line.
    void
    advance_tracking(
        std::size_t _count
    )
    {
        // one character at a time, so each line feed is seen
        for (std::size_t i = 0; (i < _count) && (offset < length); ++i)
        {
            // a line feed moves to the start of the next line
            if (data[offset] == '\n')
            {
                ++line;
                column = 1;
            }
            else
            {
                ++column;
            }

            ++offset;
        }

        return;
    }
};

// text_parser_base
//   class: the CRTP base of the text parsers (the CSS parsers among them).
// Derived supplies do_parse(text_parse_state&); the base gives it parse()
// over a state, over a pointer and a length, and over a std::string. The
// return types name Self, which defaults to Derived, so that do_parse is
// looked up only when parse() is used: as Derived's base, Derived is
// still incomplete here.
template<typename Derived>
class text_parser_base
{
public:
    // parse (state)
    //   method: runs the parser on _state, advancing it.
    template<typename Self = Derived>
    auto
    parse(
        text_parse_state& _state
    ) -> decltype(std::declval<Self&>().do_parse(_state))
    {
        return static_cast<Derived&>(*this).do_parse(_state);
    }

    // parse (text)
    //   method: runs the parser on _length characters from _data.
    template<typename Self = Derived>
    auto
    parse(
        const char* _data,
        std::size_t _length
    ) -> decltype(std::declval<Self&>().do_parse(
                      std::declval<text_parse_state&>()))
    {
        text_parse_state state(_data, _length);

        return static_cast<Derived&>(*this).do_parse(state);
    }

    // parse (string)
    //   method: runs the parser on the whole of _text.
    template<typename Self = Derived>
    auto
    parse(
        const std::string& _text
    ) -> decltype(std::declval<Self&>().do_parse(
                      std::declval<text_parse_state&>()))
    {
        return this->template parse<Self>(_text.data(),
                                          _text.size());
    }
};

// char_predicate_is_whitespace
//   predicate: whether a character is ASCII whitespace -- space, tab,
// line feed, carriage return, vertical tab or form feed -- in the form
// the text parsers take predicates: a type with a static test().
struct char_predicate_is_whitespace
{
    static D_CONSTEXPR bool
    test(
        char _c
    ) D_NOEXCEPT
    {
        return ( (_c == ' ')  ||
                 (_c == '\t') ||
                 (_c == '\n') ||
                 (_c == '\r') ||
                 (_c == '\v') ||
                 (_c == '\f') );
    }
};


NS_END  // parse
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_PARSE_PARSE_HPP
