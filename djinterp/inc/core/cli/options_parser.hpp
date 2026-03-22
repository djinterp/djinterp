/******************************************************************************
* djinterp [containers]                                     options_parser.hpp
*
* Option-set parser:
*   Provides a family of parsers that consume raw input, parse a key, and
* look up the corresponding value in an option_set.  The result of a
* successful parse is the mapped_type of the option_set; a failed parse
* is reported either because the key could not be parsed from the input
* or because the parsed key was not found in the option_set.
*
*   Two usage modes:
*
*   1. Automatic key parser (default):
*      When _KeyParser is void (the default), the parser inspects the
*      option_set's key_type via option_key_classify and instantiates
*      the lightest built-in key parser that can produce keys of that
*      type:
*        integral  -> integral_key_parser   (binary, fixed width)
*        enum_key  -> enum_key_parser       (binary, fixed width + cast)
*        trivial   -> trivial_key_parser    (binary, memcpy)
*        text      -> text_key_parser       (char, delimiter-terminated)
*        complex   -> static_assert failure (user must supply _KeyParser)
*
*   2. User-supplied key parser:
*      When _KeyParser is a non-void parser type, it is used directly.
*      The key parser must satisfy is_parser and its result_type must
*      be convertible to the option_set's key_type.
*
*
* TABLE OF CONTENTS
* =================
* I.    STATUS CODES
*       1.  option_parse_status
*
* II.   BUILT-IN KEY PARSERS
*       1.  integral_key_parser<_Key>
*       2.  enum_key_parser<_Key>
*       3.  trivial_key_parser<_Key>
*       4.  text_key_parser<_Key, _Delim>
*
* III.  KEY PARSER SELECTION
*       1.  auto_key_parser         (type-level selector)
*
* IV.   OPTIONS_PARSER
*       1.  options_parser<_OptionSet, _KeyParser>
*
*
* path:      /inc/containers/options_parser.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.XX.XX
******************************************************************************/

#ifndef DJINTERP_CONTAINERS_OPTIONS_PARSER_
#define DJINTERP_CONTAINERS_OPTIONS_PARSER_ 1

#include <cstddef>
#include <cstring>
#include <type_traits>
#include "../core/djinterp.hpp"
#include "../parse/parse.hpp"
#include "../parse/parser.hpp"
#include "./option_set.hpp"
#include "./options_parser_traits.hpp"

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include <string_view>
#endif


NS_DJINTERP
NS_PARSE


///////////////////////////////////////////////////////////////////////////////
///        I.    STATUS CODES                                               ///
///////////////////////////////////////////////////////////////////////////////

// option_parse_status
//   enum: status codes specific to option_set parsing failures.
// Extends the generic parse_status code space.
enum class option_parse_status : std::int32_t
{
    // the key was parsed successfully but was not found in
    // the option_set
    key_not_found       = -100,

    // the input was too short to contain a complete key
    insufficient_input  = -101,

    // the text key exceeded the maximum allowed length
    // without encountering a delimiter
    key_too_long        = -102,

    // the key parser reported an error
    key_parse_failed    = -103
};


///////////////////////////////////////////////////////////////////////////////
///        II.   BUILT-IN KEY PARSERS                                       ///
///////////////////////////////////////////////////////////////////////////////


// ================================================================
//  integral_key_parser
// ================================================================

// integral_key_parser
//   class: binary parser that reads sizeof(_Key) bytes from the
// input and reinterprets them as an integral _Key value using
// memcpy.  No endianness conversion is performed; the key is
// assumed to be in the host byte order.
template<typename _Key>
class integral_key_parser
    : public parser_base<integral_key_parser<_Key>>
{
    static_assert(std::is_integral<_Key>::value,
                  "integral_key_parser requires an integral key type.");

public:
    using input_type  = unsigned char;
    using result_type = _Key;

    // do_parse
    //   reads sizeof(_Key) bytes and reinterprets as _Key.
    parse_result<result_type>
    do_parse
    (
        parse_state<input_type>& _state
    )
    {
        if (_state.remaining() < sizeof(_Key))
        {
            return parse_result<result_type>::make_error(
                static_cast<parse_status>(
                    option_parse_status::insufficient_input),
                _state.offset,
                "insufficient input for integral key"
            );
        }

        result_type key{};

        std::memcpy(&key, _state.current(), sizeof(_Key));
        _state.advance(sizeof(_Key));

        return parse_result<result_type>(key);
    }
};


// ================================================================
//  enum_key_parser
// ================================================================

// enum_key_parser
//   class: binary parser that reads sizeof(underlying_type)
// bytes from the input, reinterprets them as the enum's
// underlying type, and static_casts to _Key.
template<typename _Key>
class enum_key_parser
    : public parser_base<enum_key_parser<_Key>>
{
    static_assert(std::is_enum<_Key>::value,
                  "enum_key_parser requires an enumeration key type.");

private:
    using underlying = typename std::underlying_type<_Key>::type;

public:
    using input_type  = unsigned char;
    using result_type = _Key;

    // do_parse
    //   reads sizeof(underlying) bytes and casts to _Key.
    parse_result<result_type>
    do_parse
    (
        parse_state<input_type>& _state
    )
    {
        if (_state.remaining() < sizeof(underlying))
        {
            return parse_result<result_type>::make_error(
                static_cast<parse_status>(
                    option_parse_status::insufficient_input),
                _state.offset,
                "insufficient input for enum key"
            );
        }

        underlying raw{};

        std::memcpy(&raw, _state.current(), sizeof(underlying));
        _state.advance(sizeof(underlying));

        return parse_result<result_type>(
            static_cast<result_type>(raw)
        );
    }
};


// ================================================================
//  trivial_key_parser
// ================================================================

// trivial_key_parser
//   class: binary parser that reads sizeof(_Key) bytes from the
// input and memcpy-constructs a _Key.  Requires _Key to be
// trivially copyable.
template<typename _Key>
class trivial_key_parser
    : public parser_base<trivial_key_parser<_Key>>
{
    static_assert(std::is_trivially_copyable<_Key>::value,
                  "trivial_key_parser requires a trivially copyable "
                  "key type.");

public:
    using input_type  = unsigned char;
    using result_type = _Key;

    // do_parse
    //   reads sizeof(_Key) bytes and memcpy-constructs a _Key.
    parse_result<result_type>
    do_parse
    (
        parse_state<input_type>& _state
    )
    {
        if (_state.remaining() < sizeof(_Key))
        {
            return parse_result<result_type>::make_error(
                static_cast<parse_status>(
                    option_parse_status::insufficient_input),
                _state.offset,
                "insufficient input for trivial key"
            );
        }

        result_type key{};

        std::memcpy(&key, _state.current(), sizeof(_Key));
        _state.advance(sizeof(_Key));

        return parse_result<result_type>(key);
    }
};


// ================================================================
//  text_key_parser
// ================================================================

// text_key_parser
//   class: text parser that reads characters from the input
// until a delimiter character is encountered or the input is
// exhausted.  The accumulated characters are used to construct
// a _Key (which must be constructible from a (const char*,
// size_t) pair — e.g. std::string, std::string_view).
//
//   The delimiter character defaults to '=' for key=value
// grammars but can be overridden via the _Delim non-type
// parameter.  The delimiter is consumed from the input but
// is NOT included in the key.
//
//   A maximum key length of 4096 characters is enforced to
// prevent unbounded accumulation.
template<typename _Key,
         char      _Delim  = '=',
         std::size_t _MaxLen = 4096>
class text_key_parser
    : public parser_base<text_key_parser<_Key, _Delim, _MaxLen>>
{
public:
    using input_type  = char;
    using result_type = _Key;

    // do_parse
    //   scans forward for the delimiter, then constructs a _Key
    // from the accumulated character range.
    parse_result<result_type>
    do_parse
    (
        parse_state<input_type>& _state
    )
    {
        if (_state.at_end())
        {
            return parse_result<result_type>::make_error(
                static_cast<parse_status>(
                    option_parse_status::insufficient_input),
                _state.offset,
                "empty input for text key"
            );
        }

        const char* start  = _state.current();
        std::size_t begin  = _state.offset;
        std::size_t length = 0;

        // scan for delimiter or end of input
        while ( (!_state.at_end()) &&
                (*_state.current() != _Delim) )
        {
            ++length;
            _state.advance(1);

            if (length > _MaxLen)
            {
                return parse_result<result_type>::make_error(
                    static_cast<parse_status>(
                        option_parse_status::key_too_long),
                    begin,
                    "text key exceeded maximum length"
                );
            }
        }

        // consume the delimiter if present
        if ( (!_state.at_end()) &&
             (*_state.current() == _Delim) )
        {
            _state.advance(1);
        }

        // zero-length key (delimiter was the first character)
        if (length == 0)
        {
            return parse_result<result_type>::make_error(
                static_cast<parse_status>(
                    option_parse_status::key_parse_failed),
                begin,
                "empty text key"
            );
        }

        return parse_result<result_type>(
            result_type(start, length)
        );
    }
};


///////////////////////////////////////////////////////////////////////////////
///        III.  KEY PARSER SELECTION                                       ///
///////////////////////////////////////////////////////////////////////////////
// auto_key_parser maps an option_key_class to the corresponding
// built-in key parser type.  When the key class is `complex`,
// the type is void and options_parser will static_assert.

NS_INTERNAL

    // auto_key_parser_helper
    //   trait: primary template — complex key, no automatic
    // parser available.
    template<typename         _Key,
             option_key_class _Class>
    struct auto_key_parser_helper
    {
        using type = void;
    };

    // auto_key_parser_helper<integral>
    //   trait: selects integral_key_parser.
    template<typename _Key>
    struct auto_key_parser_helper<_Key, option_key_class::integral>
    {
        using type = integral_key_parser<_Key>;
    };

    // auto_key_parser_helper<enum_key>
    //   trait: selects enum_key_parser.
    template<typename _Key>
    struct auto_key_parser_helper<_Key, option_key_class::enum_key>
    {
        using type = enum_key_parser<_Key>;
    };

    // auto_key_parser_helper<trivial>
    //   trait: selects trivial_key_parser.
    template<typename _Key>
    struct auto_key_parser_helper<_Key, option_key_class::trivial>
    {
        using type = trivial_key_parser<_Key>;
    };

    // auto_key_parser_helper<text>
    //   trait: selects text_key_parser with default delimiter.
    template<typename _Key>
    struct auto_key_parser_helper<_Key, option_key_class::text>
    {
        using type = text_key_parser<_Key>;
    };

NS_END  // internal

// auto_key_parser
//   trait: selects the lightest built-in key parser for a given
// key type based on its option_key_class.  Produces void when
// no automatic parser exists (complex keys).
template<typename _Key>
struct auto_key_parser
{
    using type = typename internal::auto_key_parser_helper<
        _Key,
        traits::option_key_classify<_Key>::value
    >::type;
};

// auto_key_parser_t
//   type: convenience alias for auto_key_parser<_Key>::type.
template<typename _Key>
using auto_key_parser_t = typename auto_key_parser<_Key>::type;


///////////////////////////////////////////////////////////////////////////////
///        IV.   OPTIONS_PARSER                                             ///
///////////////////////////////////////////////////////////////////////////////
// The main parser.  _OptionSet must satisfy is_option_set.
// _KeyParser defaults to void, which triggers automatic key
// parser selection via auto_key_parser.
//
// parse behaviour:
//   1. Use the key parser to extract a key from the input.
//   2. Look up the key in the option_set via find().
//   3. On match, return the mapped value.
//   4. On key-parse failure, propagate the error.
//   5. On key-not-found, report option_parse_status::key_not_found.

NS_INTERNAL

    // resolved_key_parser
    //   trait: resolves the effective key parser type.  When
    // _KeyParser is void, uses auto_key_parser; otherwise
    // passes through _KeyParser unchanged.
    template<typename _OptionSet,
             typename _KeyParser>
    struct resolved_key_parser
    {
        using type = _KeyParser;
    };

    // resolved_key_parser<_OptionSet, void>
    //   trait: automatic selection for defaulted _KeyParser.
    template<typename _OptionSet>
    struct resolved_key_parser<_OptionSet, void>
    {
        using type = auto_key_parser_t<
            typename _OptionSet::key_type
        >;
    };

    // resolved_key_parser_t
    //   type: convenience alias.
    template<typename _OptionSet,
             typename _KeyParser>
    using resolved_key_parser_t =
        typename resolved_key_parser<_OptionSet, _KeyParser>::type;

NS_END  // internal


// options_parser
//   class: CRTP parser that parses a key from the input stream
// and looks it up in an option_set, returning the mapped value
// on success.  The option_set is held by const reference and
// must outlive the parser.
template<typename _OptionSet,
         typename _KeyParser = void>
class options_parser
    : public parser_base<
          options_parser<_OptionSet, _KeyParser>
      >
{
private:
    using effective_key_parser =
        internal::resolved_key_parser_t<_OptionSet, _KeyParser>;

    static_assert(
        traits::is_option_set<_OptionSet>::value,
        "options_parser requires _OptionSet to satisfy "
        "is_option_set.");

    static_assert(
        !std::is_void<effective_key_parser>::value,
        "No automatic key parser is available for this "
        "option_set's key_type (classified as complex). "
        "Supply an explicit _KeyParser template argument.");

public:
    using input_type  = typename effective_key_parser::input_type;
    using result_type = typename _OptionSet::mapped_type;

    // options_parser (constructor)
    //   binds the parser to an option_set instance.
    explicit
    options_parser
    (
        const _OptionSet& _options
    )
        : m_options{_options},
          m_key_parser{}
    {
    }

    // options_parser (constructor, custom key parser)
    //   binds the parser to an option_set instance and a
    // user-supplied key parser instance.
    options_parser
    (
        const _OptionSet&         _options,
        const effective_key_parser& _key_parser
    )
        : m_options{_options},
          m_key_parser{_key_parser}
    {
    }

    // do_parse
    //   parses a key from the input state, looks it up in the
    // bound option_set, and returns the mapped value on success.
    parse_result<result_type>
    do_parse
    (
        parse_state<input_type>& _state
    )
    {
        // step 1: parse the key from input
        auto key_result = m_key_parser.parse(_state);

        if (!key_result.ok())
        {
            return parse_result<result_type>::make_error(
                static_cast<parse_status>(
                    option_parse_status::key_parse_failed),
                key_result.error().offset(),
                key_result.error().message()
            );
        }

        // step 2: look up the key in the option_set
        const auto* found = m_options.find(key_result.value());

        if (!found)
        {
            return parse_result<result_type>::make_error(
                static_cast<parse_status>(
                    option_parse_status::key_not_found),
                _state.offset,
                "key not found in option_set"
            );
        }

        // step 3: return the mapped value
        return parse_result<result_type>(found->value);
    }

private:
    const _OptionSet&      m_options;
    effective_key_parser   m_key_parser;
};


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_CONTAINERS_OPTIONS_PARSER_
