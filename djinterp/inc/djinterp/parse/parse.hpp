/******************************************************************************
* djinterp [parse]                                                   parse.hpp
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
* path:      /inc/djinterp/parse/parse.hpp
* link(s):   ch-parsing.tex
* author(s): Samuel 'teer' Neal-Blim                       created: 2025.01.11
******************************************************************************/

#ifndef DJINTERP_PARSE_
#define DJINTERP_PARSE_ 1

// std
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
// djinterp
#include "../core/djinterp.hpp"
#include "../core/functional/result.hpp"


// D_KEYWORD_PARSE
//   keyword: resolves to `parse`.  Marks a unit of code as part of the
// parsing subsystem.
#define D_KEYWORD_PARSE             parse

// NS_PARSE
//   namespace: the parse subsystem namespace.
#define NS_PARSE                    D_NAMESPACE(D_KEYWORD_PARSE)


NS_DJINTERP
NS_PARSE


// ================================================================
//  I.   parseable  /  parse_traits
// ================================================================

// parseable
//   trait: associates a minor (token) type with a major (parsed
// aggregate) type for a given parseable domain.
template<typename _Minor,
         typename _Major>
struct parseable
{
    using minor = _Minor;
    using major = _Major;
};

// is_parseable
//   trait: primary template — not parseable by default.
template<typename _Type,
         typename = void>
struct is_parseable : std::false_type
{};

// is_parseable (SFINAE specialisation)
//   trait: true when _Type exposes nested minor and major aliases.
template<typename _Type>
struct is_parseable<
    _Type,
    void_t<typename _Type::minor,
           typename _Type::major>
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
template<typename _Type>
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
typedef std::int32_t parse_status;

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
// _ValueType (success) or a parse_error (failure).  A refinement of
// functional::result<_ValueType, parse_error>: it IS a result and
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
template<typename _ValueType>
class parse_result
    : public functional::result<_ValueType, parse_error>
{
private:
    using base_type = functional::result<_ValueType, parse_error>;

public:
    using value_type = _ValueType;
    using error_type = parse_error;

    parse_result(
        const _ValueType& _value
    )
        : base_type(
              functional::internal::ok_tag(
                  functional::internal::ok_tag::construct_tag()),
              _value)
    {}

    parse_result(
        _ValueType&& _value
    )
        : base_type(
              functional::internal::ok_tag(
                  functional::internal::ok_tag::construct_tag()),
              static_cast<_ValueType&&>(_value))
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
    const _ValueType& value() const
    {
        return base_type::value();
    }

    D_NODISCARD
    _ValueType& value()
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
        const _ValueType& _value
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
// type _ElementType.  Threaded by reference through the parser
// function as the operational counterpart of the formal residual:
// a successful parse advances `offset`; Alternative's `alt`
// combinator saves and restores `offset` so failures don't leak
// consumed input to the next branch.
//
//   Agnostic to the nature of the data — character streams, byte
// buffers, token sequences all instantiate it.
template<typename _ElementType>
struct parse_state
{
    using element_type = _ElementType;

    const _ElementType* data;
    std::size_t         length;
    std::size_t         offset;

    parse_state()
        : data   (nullptr),
          length (0),
          offset (0)
    {}

    parse_state(
        const _ElementType* _data,
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
    const _ElementType* current() const
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


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_PARSE_
