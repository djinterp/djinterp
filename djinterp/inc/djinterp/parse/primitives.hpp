/******************************************************************************
* djinterp [parse]                                       parser/primitives.hpp
*
* Atomic parsers — the leaves of every CRTP composition tree.
*   Each primitive is a concrete class inheriting from parser_expr,
* with the parsing logic in parse_impl.  A factory function returns
* that class by value, so call sites read as combinator literature
* expects (`literal('a')`, `digit()`) while the compiler sees the
* exact concrete type.  Composition through these is fully visible,
* fully inlinable, and pays zero virtual dispatch.
*
*   Erasure into parser<R, E> happens only when the call site wants
* a uniform type (storage, recursive references, return values) or
* the four functional protocols.
*
* CONTENTS
*   I.    nullary success / failure
*           succeed_parser  / succeed(v)
*           fail_parser     / fail<R>(msg)
*   II.   input-position observers
*           eof_parser       / eof<E>()
*           position_parser  / position<E>()
*           remaining_parser / remaining<E>()
*   III.  single-element consumers
*           any_parser       / any<E>()
*           satisfy_parser   / satisfy(pred)
*           one_of_parser    / one_of<E>({...})
*           none_of_parser   / none_of<E>({...})
*   IV.   text-specific shorthands  (E == char)
*           literal_parser / literal(c)
*           plus inline factories: digit(), alpha(), alnum(), space()
*
* path:      /inc/djinterp/parse/parser/primitives.hpp
* link(s):   ch-parsing.tex
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_PARSE_PARSER_PRIMITIVES_
#define DJINTERP_PARSE_PARSER_PRIMITIVES_ 1

// std
#include <cstddef>
#include <initializer_list>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../core/djinterp.hpp"
#include "../parse.hpp"
#include "./parser.hpp"


NS_DJINTERP
NS_PARSE


// ================================================================
//  I.   nullary success / failure
// ================================================================

// succeed_parser
//   class: a parser that always succeeds with a stored value and
// consumes no input.  The CRTP form of monad_traits::unit /
// applicative pure for the parser carrier.
template<typename _Result,
         typename _Element = char>
class succeed_parser
    : public parser_expr<succeed_parser<_Result, _Element>>
{
public:
    using input_type   = _Element;
    using element_type = _Element;
    using result_type  = _Result;
    using value_type   = _Result;
    using state_type   = parse_state<_Element>;
    using output_type  = parse_result<_Result>;

    explicit succeed_parser(
        _Result _value
    )
        : m_value(static_cast<_Result&&>(_value))
    {}

    // parse_impl
    //   method: returns the stored value without advancing _state.
    output_type
    parse_impl(
        state_type& /*_state*/
    ) const
    {
        return output_type(m_value);
    }

private:
    _Result m_value;
};

// succeed
//   factory: builds a succeed_parser by deducing the value type.
template<typename _Result,
         typename _Element = char>
D_NODISCARD
succeed_parser<_Result, _Element>
succeed(
    _Result _value
)
{
    return succeed_parser<_Result, _Element>(
        static_cast<_Result&&>(_value));
}


// fail_parser
//   class: a parser that always fails with a stored message and
// status code.  alternative_traits::empty is the no-message form;
// fail labels the failure explicitly.
template<typename _Result,
         typename _Element = char>
class fail_parser
    : public parser_expr<fail_parser<_Result, _Element>>
{
public:
    using input_type   = _Element;
    using element_type = _Element;
    using result_type  = _Result;
    using value_type   = _Result;
    using state_type   = parse_state<_Element>;
    using output_type  = parse_result<_Result>;

    fail_parser(
        const std::string& _message,
        parse_status       _status
    )
        : m_message(_message),
          m_status (_status)
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        return output_type::make_error(
            m_status,
            _state.offset,
            m_message);
    }

private:
    std::string  m_message;
    parse_status m_status;
};

// fail
//   factory: builds a fail_parser.  _Result must be supplied
// explicitly since it can't be deduced.
template<typename _Result,
         typename _Element = char>
D_NODISCARD
fail_parser<_Result, _Element>
fail(
    const std::string& _message = std::string("fail"),
    parse_status       _status  = DParseStatusFailure
)
{
    return fail_parser<_Result, _Element>(_message, _status);
}


// ================================================================
//  II.  input-position observers
// ================================================================

// eof_parser
//   class: succeeds with `true` iff the state is at end of input.
// The natural terminator for a grammar that must consume its whole
// input.
template<typename _Element = char>
class eof_parser
    : public parser_expr<eof_parser<_Element>>
{
public:
    using input_type   = _Element;
    using element_type = _Element;
    using result_type  = bool;
    using value_type   = bool;
    using state_type   = parse_state<_Element>;
    using output_type  = parse_result<bool>;

    eof_parser() = default;

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        if (_state.at_end())
        {
            return output_type(true);
        }

        return output_type::make_error(
            DParseStatusFailure,
            _state.offset,
            "eof: input remaining");
    }
};

// eof
//   factory: builds an eof_parser.
template<typename _Element = char>
D_NODISCARD
eof_parser<_Element>
eof()
{
    return eof_parser<_Element>();
}


// position_parser
//   class: succeeds with the current offset, consuming nothing.
template<typename _Element = char>
class position_parser
    : public parser_expr<position_parser<_Element>>
{
public:
    using input_type   = _Element;
    using element_type = _Element;
    using result_type  = std::size_t;
    using value_type   = std::size_t;
    using state_type   = parse_state<_Element>;
    using output_type  = parse_result<std::size_t>;

    position_parser() = default;

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        return output_type(_state.offset);
    }
};

// position
//   factory: builds a position_parser.
template<typename _Element = char>
D_NODISCARD
position_parser<_Element>
position()
{
    return position_parser<_Element>();
}


// remaining_parser
//   class: succeeds with the count of unconsumed elements.
template<typename _Element = char>
class remaining_parser
    : public parser_expr<remaining_parser<_Element>>
{
public:
    using input_type   = _Element;
    using element_type = _Element;
    using result_type  = std::size_t;
    using value_type   = std::size_t;
    using state_type   = parse_state<_Element>;
    using output_type  = parse_result<std::size_t>;

    remaining_parser() = default;

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        return output_type(_state.remaining());
    }
};

// remaining
//   factory: builds a remaining_parser.
template<typename _Element = char>
D_NODISCARD
remaining_parser<_Element>
remaining()
{
    return remaining_parser<_Element>();
}


// ================================================================
//  III. single-element consumers
// ================================================================

// any_parser
//   class: consumes one element and returns it.  Fails at end of
// input.
template<typename _Element = char>
class any_parser
    : public parser_expr<any_parser<_Element>>
{
public:
    using input_type   = _Element;
    using element_type = _Element;
    using result_type  = _Element;
    using value_type   = _Element;
    using state_type   = parse_state<_Element>;
    using output_type  = parse_result<_Element>;

    any_parser() = default;

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        _Element e;

        if (_state.at_end())
        {
            return output_type::make_error(
                DParseStatusEndOfInput,
                _state.offset,
                "any: unexpected end of input");
        }

        e = *_state.current();
        _state.advance(1);

        return output_type(e);
    }
};

// any
//   factory: builds an any_parser.
template<typename _Element = char>
D_NODISCARD
any_parser<_Element>
any()
{
    return any_parser<_Element>();
}


// satisfy_parser
//   class: consumes one element passing a stored predicate; fails
// otherwise (and on end of input).  The atomic conditional
// consumer — every text-class shorthand below is satisfy with a
// different predicate.
template<typename _Predicate,
         typename _Element = char>
class satisfy_parser
    : public parser_expr<satisfy_parser<_Predicate, _Element>>
{
public:
    using input_type   = _Element;
    using element_type = _Element;
    using result_type  = _Element;
    using value_type   = _Element;
    using state_type   = parse_state<_Element>;
    using output_type  = parse_result<_Element>;

    satisfy_parser(
        _Predicate         _predicate,
        const std::string& _label
    )
        : m_predicate(static_cast<_Predicate&&>(_predicate)),
          m_label    (_label)
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        _Element e;

        if (_state.at_end())
        {
            return output_type::make_error(
                DParseStatusEndOfInput,
                _state.offset,
                m_label + ": unexpected end of input");
        }

        e = *_state.current();

        if (!m_predicate(e))
        {
            return output_type::make_error(
                DParseStatusFailure,
                _state.offset,
                m_label + ": predicate failed");
        }

        _state.advance(1);

        return output_type(e);
    }

private:
    _Predicate  m_predicate;
    std::string m_label;
};

// satisfy
//   factory: builds a satisfy_parser, deducing the predicate type.
template<typename _Predicate,
         typename _Element = char>
D_NODISCARD
satisfy_parser<_Predicate, _Element>
satisfy(
    _Predicate         _predicate,
    const std::string& _label = std::string("satisfy")
)
{
    return satisfy_parser<_Predicate, _Element>(
        static_cast<_Predicate&&>(_predicate),
        _label);
}


// one_of_set_parser
//   class: consumes one element if it appears in a stored set;
// fails otherwise.  Named with `_set_` to disambiguate from the
// combinator `one_of_parser` factory (n-ary ordered choice) in
// combinators.hpp.
template<typename _Element = char>
class one_of_set_parser
    : public parser_expr<one_of_set_parser<_Element>>
{
public:
    using input_type   = _Element;
    using element_type = _Element;
    using result_type  = _Element;
    using value_type   = _Element;
    using state_type   = parse_state<_Element>;
    using output_type  = parse_result<_Element>;

    explicit one_of_set_parser(
        std::vector<_Element> _set
    )
        : m_set(static_cast<std::vector<_Element>&&>(_set))
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        _Element    e;
        std::size_t i;

        if (_state.at_end())
        {
            return output_type::make_error(
                DParseStatusEndOfInput,
                _state.offset,
                "one_of: unexpected end of input");
        }

        e = *_state.current();

        for (i = 0; i < m_set.size(); ++i)
        {
            if (m_set[i] == e)
            {
                _state.advance(1);
                return output_type(e);
            }
        }

        return output_type::make_error(
            DParseStatusFailure,
            _state.offset,
            "one_of: element not in set");
    }

private:
    std::vector<_Element> m_set;
};

// one_of
//   factory: builds a one_of_set_parser from an initializer list.
template<typename _Element = char>
D_NODISCARD
one_of_set_parser<_Element>
one_of(
    std::initializer_list<_Element> _set
)
{
    return one_of_set_parser<_Element>(
        std::vector<_Element>(_set.begin(), _set.end()));
}


// none_of_set_parser
//   class: consumes one element NOT in a stored set; fails on a
// member of the set and on end of input.
template<typename _Element = char>
class none_of_set_parser
    : public parser_expr<none_of_set_parser<_Element>>
{
public:
    using input_type   = _Element;
    using element_type = _Element;
    using result_type  = _Element;
    using value_type   = _Element;
    using state_type   = parse_state<_Element>;
    using output_type  = parse_result<_Element>;

    explicit none_of_set_parser(
        std::vector<_Element> _set
    )
        : m_set(static_cast<std::vector<_Element>&&>(_set))
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        _Element    e;
        std::size_t i;

        if (_state.at_end())
        {
            return output_type::make_error(
                DParseStatusEndOfInput,
                _state.offset,
                "none_of: unexpected end of input");
        }

        e = *_state.current();

        for (i = 0; i < m_set.size(); ++i)
        {
            if (m_set[i] == e)
            {
                return output_type::make_error(
                    DParseStatusFailure,
                    _state.offset,
                    "none_of: element in forbidden set");
            }
        }

        _state.advance(1);

        return output_type(e);
    }

private:
    std::vector<_Element> m_set;
};

// none_of
//   factory: builds a none_of_set_parser.
template<typename _Element = char>
D_NODISCARD
none_of_set_parser<_Element>
none_of(
    std::initializer_list<_Element> _set
)
{
    return none_of_set_parser<_Element>(
        std::vector<_Element>(_set.begin(), _set.end()));
}


// ================================================================
//  IV.  text-specific shorthands  (E == char)
// ================================================================

// literal_parser
//   class: matches exactly a stored character, returning it.  The
// text-parser analogue of `satisfy(== c)`.
class literal_parser
    : public parser_expr<literal_parser>
{
public:
    using input_type   = char;
    using element_type = char;
    using result_type  = char;
    using value_type   = char;
    using state_type   = parse_state<char>;
    using output_type  = parse_result<char>;

    explicit literal_parser(
        char _c
    )
        : m_c(_c)
    {}

    output_type
    parse_impl(
        state_type& _state
    ) const
    {
        char actual;

        if (_state.at_end())
        {
            return output_type::make_error(
                DParseStatusEndOfInput,
                _state.offset,
                "literal: unexpected end of input");
        }

        actual = *_state.current();

        if (actual != m_c)
        {
            return output_type::make_error(
                DParseStatusFailure,
                _state.offset,
                "literal: character mismatch");
        }

        _state.advance(1);

        return output_type(m_c);
    }

private:
    char m_c;
};

// literal
//   factory: builds a literal_parser.
D_NODISCARD
inline literal_parser
literal(
    char _c
)
{
    return literal_parser(_c);
}


// Text-class shorthands.  Each is a satisfy_parser with a lambda
// predicate, returned via the appropriate factory.  The auto-
// returned types differ per call site but all derive from
// parser_expr so they participate in CRTP composition.

// digit_predicate
//   internal: ASCII decimal-digit test.
NS_INTERNAL

    struct digit_predicate
    {
        bool operator()(char _c) const D_NOEXCEPT
        {
            return ( (_c >= '0') && (_c <= '9') );
        }
    };

    struct alpha_predicate
    {
        bool operator()(char _c) const D_NOEXCEPT
        {
            return ( ( (_c >= 'a') && (_c <= 'z') ) ||
                     ( (_c >= 'A') && (_c <= 'Z') ) );
        }
    };

    struct alnum_predicate
    {
        bool operator()(char _c) const D_NOEXCEPT
        {
            return ( ( (_c >= 'a') && (_c <= 'z') ) ||
                     ( (_c >= 'A') && (_c <= 'Z') ) ||
                     ( (_c >= '0') && (_c <= '9') ) );
        }
    };

    struct space_predicate
    {
        bool operator()(char _c) const D_NOEXCEPT
        {
            return ( (_c == ' ')  || (_c == '\t') ||
                     (_c == '\n') || (_c == '\r') );
        }
    };

NS_END  // internal

// digit
//   factory: matches an ASCII decimal digit.
D_NODISCARD
inline satisfy_parser<internal::digit_predicate, char>
digit()
{
    return satisfy_parser<internal::digit_predicate, char>(
        internal::digit_predicate(),
        "digit");
}

// alpha
//   factory: matches an ASCII letter.
D_NODISCARD
inline satisfy_parser<internal::alpha_predicate, char>
alpha()
{
    return satisfy_parser<internal::alpha_predicate, char>(
        internal::alpha_predicate(),
        "alpha");
}

// alnum
//   factory: matches an ASCII letter or decimal digit.
D_NODISCARD
inline satisfy_parser<internal::alnum_predicate, char>
alnum()
{
    return satisfy_parser<internal::alnum_predicate, char>(
        internal::alnum_predicate(),
        "alnum");
}

// space
//   factory: matches an ASCII whitespace character.
D_NODISCARD
inline satisfy_parser<internal::space_predicate, char>
space()
{
    return satisfy_parser<internal::space_predicate, char>(
        internal::space_predicate(),
        "space");
}


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_PARSE_PARSER_PRIMITIVES_
