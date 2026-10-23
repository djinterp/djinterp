/*******************************************************************************
* djinterp [net]                                                      claude.hpp
*
*   Vendor integration for automating Anthropic's Claude via the Messages API.
* It layers a typed request/response surface and a client over net's neutral
* HTTP vocabulary (http.hpp) and the libcurl transport (curl/curl.hpp).
*
* CONTENTS (all in namespace djinterp::net::vendor::claude):
*   0.  endpoint / header / model constants
*   I.   minimal JSON writer + string reader (internal; we own the request
*        JSON entirely, and read only the few response fields we expose)
*   II.  role_type + message, message_request (+ to_json), message_response
*   III. server-sent-events parser (streaming)
*   IV.  client + quick_message convenience
*
*   JSON NOTE: request bodies are serialized by the small writer here (fully
* controlled and correct). For responses, only a handful of scalar fields and
* the concatenated assistant text are surfaced via a targeted string reader --
* the complete JSON is always available as response.raw() for callers that want
* to parse it with a full JSON library.
*
*   SECURITY NOTE: the API key is sent in the x-api-key header over TLS (which
* curl.hpp verifies by default). Keep keys out of source; prefer supplying them
* from the environment or a secret store at construction time.
*
*   Requires:  curl/curl.hpp (and therefore libcurl) and http.hpp, whose
*              header helpers link http/http.c and the codec's sources;
*              C++98 and later. The SSE callback is a re_std::function, the
*              JSON writer's integers are long, and role_type is scoped in a
*              struct (decision 3.4): role_type::value.
*
*
* path:      /inc/djinterp/net/api/claude.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.16
*                                                            revised: 2026.10.16
*******************************************************************************/

#ifndef DJINTERP_NET_API_CLAUDE_HPP
#define DJINTERP_NET_API_CLAUDE_HPP 1

// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::sprintf
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp -- libcurl transport (pulls http.hpp: neutral vocabulary and types)
#include "../net_common.h"  // d_net_ascii_is_space
#include "../codec/codec_hex.h"    // d_net_hex_digit, d_net_hex_value
#include "../codec/codec_line.h"   // d_net_line_find
#include "../curl/curl.hpp"        // curl::perform, perform_stream, body_sink
// re_std
#include "../../../re_std/functional/function.hpp"  // re_std::function


// D_KEYWORD_VENDOR
//   keyword: resolves to `vendor`. Namespace tag grouping third-party service
// integrations. Guarded for forward compatibility with the core.
#ifndef D_KEYWORD_VENDOR
    #define D_KEYWORD_VENDOR            vendor
#endif  // D_KEYWORD_VENDOR

// D_KEYWORD_CLAUDE
//   keyword: resolves to `claude`. Namespace tag for the Anthropic integration.
#ifndef D_KEYWORD_CLAUDE
    #define D_KEYWORD_CLAUDE            claude
#endif  // D_KEYWORD_CLAUDE


NS_DJINTERP
NS_NET
D_NAMESPACE(D_KEYWORD_VENDOR)
D_NAMESPACE(D_KEYWORD_CLAUDE)


///////////////////////////////////////////////////////////////////////////////
///                        0.   CONSTANTS                                  ///
///////////////////////////////////////////////////////////////////////////////

// api_base_url
//   constant: default Anthropic API origin.
D_CONSTEXPR const char* const api_base_url    = "https://api.anthropic.com";

// messages_path
//   constant: the Messages API resource path.
D_CONSTEXPR const char* const messages_path   = "/v1/messages";

// default_version
//   constant: default value for the anthropic-version header.
D_CONSTEXPR const char* const default_version = "2023-06-01";

// header_api_key / header_version / header_beta
//   constant: Anthropic-specific request header names.
D_CONSTEXPR const char* const header_api_key  = "x-api-key";
D_CONSTEXPR const char* const header_version  = "anthropic-version";
D_CONSTEXPR const char* const header_beta     = "anthropic-beta";

// model
//   namespace: documented API model identifier strings. Model availability and
// naming change over time; treat these as convenient defaults, not a fixed
// catalog, and pass any current identifier string directly when needed.
D_NAMESPACE(model)

    D_CONSTEXPR const char* const opus   = "claude-opus-4-8";
    D_CONSTEXPR const char* const sonnet = "claude-sonnet-5";
    D_CONSTEXPR const char* const haiku  = "claude-haiku-4-5-20251001";
    D_CONSTEXPR const char* const fable  = "claude-fable-5";

NS_END  // model

// default_model
//   constant: the model used when a request specifies none.
D_CONSTEXPR const char* const default_model   = model::opus;


///////////////////////////////////////////////////////////////////////////////
///              I.   MINIMAL JSON WRITER + STRING READER                   ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // append_json_escaped
    //   function: appends `_in` to `_out`, escaping it for inclusion inside a
    // JSON string literal (control characters below 0x20 become \u00XX; the
    // standard short escapes are used where defined). Hex digits are net's
    // codec's.
    inline void
    append_json_escaped(
        std::string&       _out,
        const std::string& _in
    )
    {
        // escape each byte per the JSON string grammar
        for (std::size_t i = 0; i < _in.size(); ++i)
        {
            const unsigned char c = static_cast<unsigned char>(_in[i]);

            switch (c)
            {
                case '"':  _out += "\\\""; break;
                case '\\': _out += "\\\\"; break;
                case '\b': _out += "\\b";  break;
                case '\f': _out += "\\f";  break;
                case '\n': _out += "\\n";  break;
                case '\r': _out += "\\r";  break;
                case '\t': _out += "\\t";  break;
                default:
                    if (c < 0x20)
                    {
                        _out += "\\u00";
                        _out.push_back(::d_net_hex_digit((c >> 4) & 0x0Fu,
                                                         D_NET_HEX_UPPER));
                        _out.push_back(::d_net_hex_digit(c & 0x0Fu,
                                                         D_NET_HEX_UPPER));
                    }
                    else
                    {
                        _out.push_back(static_cast<char>(c));
                    }
                    break;
            }
        }

        return;
    }

    // json_writer
    //   class: a tiny streaming JSON builder that tracks structural state so
    // callers need not manage commas. Values are emitted with value_* methods;
    // objects/arrays are bracketed with the begin_/end_ pairs.
    class json_writer
    {
    public:
        json_writer()
            : m_out(),
              m_need_comma(false)
        {
        }

        // begin_object / end_object
        void
        begin_object()
        {
            separate();
            m_out.push_back('{');
            m_need_comma = false;

            return;
        }

        void
        end_object()
        {
            m_out.push_back('}');
            m_need_comma = true;

            return;
        }

        // begin_array / end_array
        void
        begin_array()
        {
            separate();
            m_out.push_back('[');
            m_need_comma = false;

            return;
        }

        void
        end_array()
        {
            m_out.push_back(']');
            m_need_comma = true;

            return;
        }

        // key
        //   function: writes an object key; the next value follows without a
        // separating comma.
        void
        key(
            const std::string& _name
        )
        {
            separate();
            m_out.push_back('"');
            append_json_escaped(m_out,
                                _name);
            m_out.push_back('"');
            m_out.push_back(':');
            m_need_comma = false;

            return;
        }

        // value (string)
        void
        value(
            const std::string& _string
        )
        {
            separate();
            m_out.push_back('"');
            append_json_escaped(m_out,
                                _string);
            m_out.push_back('"');
            m_need_comma = true;

            return;
        }

        // value_int
        //   function: an integer, in decimal (std::to_string is C++11's).
        // long, which every level has, holds what the API counts --
        // max_tokens, top_k -- so the writer needs no 64-bit type, which
        // ISO strict C++98 on a 32-bit target lacks.
        void
        value_int(
            long _n
        )
        {
            char      digits[32];
            const int length = std::sprintf(digits,
                                            "%ld",
                                            _n);

            value_raw(std::string(digits,
                                  static_cast<std::size_t>(length)));

            return;
        }

        // value_double
        //   function: a real, as std::to_string wrote it ("%f").
        void
        value_double(
            double _d
        )
        {
            char      digits[512];
            const int length = std::sprintf(digits,
                                            "%f",
                                            _d);

            value_raw(std::string(digits,
                                  static_cast<std::size_t>(length)));

            return;
        }

        // value_bool
        void
        value_bool(
            bool _b
        )
        {
            value_raw(_b ? "true" : "false");

            return;
        }

        // value_raw
        //   function: emits pre-serialized JSON verbatim (numbers, literals, or
        // a nested document). The caller is responsible for its validity.
        void
        value_raw(
            const std::string& _raw
        )
        {
            separate();
            m_out += _raw;
            m_need_comma = true;

            return;
        }

        // str
        //   function: the accumulated JSON document.
        D_NODISCARD const std::string&
        str() const
        {
            return m_out;
        }

    private:
        // separate
        //   function: emits a comma when the previous token requires one before
        // the next sibling.
        void
        separate()
        {
            if (m_need_comma)
            {
                m_out.push_back(',');
            }

            return;
        }

        std::string m_out;
        bool        m_need_comma;
    };

    // append_utf8
    //   function: appends the UTF-8 encoding of a code point of the Basic
    // Multilingual Plane (below 0x10000): one, two or three octets.
    inline void
    append_utf8(
        std::string& _out,
        int          _code
    )
    {
        // the encoding's length follows the code point's size
        if (_code < 0x80)
        {
            _out.push_back(static_cast<char>(_code));
        }
        else if (_code < 0x800)
        {
            _out.push_back(static_cast<char>(0xC0 | (_code >> 6)));
            _out.push_back(static_cast<char>(0x80 | (_code & 0x3F)));
        }
        else
        {
            _out.push_back(static_cast<char>(0xE0 | (_code >> 12)));
            _out.push_back(static_cast<char>(0x80 | ((_code >> 6) & 0x3F)));
            _out.push_back(static_cast<char>(0x80 | (_code & 0x3F)));
        }

        return;
    }

    // read_unicode_escape
    //   function: for a \u escape whose four hex digits start at `_i`,
    // appends the code point as UTF-8 and moves `_i` past the digits. Without
    // four hex digits the escape stands for a plain 'u', and `_i` stays.
    inline void
    read_unicode_escape(
        const std::string& _s,
        std::size_t&       _i,
        std::string&       _value
    )
    {
        int code = 0;

        // four characters must remain
        if ((_i + 4) > _s.size())
        {
            _value.push_back('u');

            return;
        }

        // each of them a hex digit
        for (std::size_t k = 0; k < 4u; ++k)
        {
            const int d = ::d_net_hex_value(_s[_i + k]);

            if (d < 0)
            {
                _value.push_back('u');

                return;
            }

            code = (code << 4) | d;
        }

        _i += 4;
        append_utf8(_value,
                    code);

        return;
    }

    // read_escape
    //   function: decodes the escape whose letter is at `_i`, moving `_i`
    // past it; a letter JSON does not define stands for itself.
    inline void
    read_escape(
        const std::string& _s,
        std::size_t&       _i,
        std::string&       _value
    )
    {
        const char e = _s[_i++];

        switch (e)
        {
            case '"':  _value.push_back('"');  break;
            case '\\': _value.push_back('\\'); break;
            case '/':  _value.push_back('/');  break;
            case 'b':  _value.push_back('\b'); break;
            case 'f':  _value.push_back('\f'); break;
            case 'n':  _value.push_back('\n'); break;
            case 'r':  _value.push_back('\r'); break;
            case 't':  _value.push_back('\t'); break;
            case 'u':
                read_unicode_escape(_s,
                                    _i,
                                    _value);
                break;
            default:
                _value.push_back(e);
                break;
        }

        return;
    }

    // read_json_string
    //   function: reads a JSON string literal from `_s` beginning at `_pos`
    // (which must index the opening quote), decoding escapes into `_out` and
    // setting `_end` to the index just past the closing quote. Returns false on
    // a malformed / unterminated literal. \uXXXX escapes are decoded to UTF-8
    // for the Basic Multilingual Plane (surrogate pairs are passed through as
    // their raw code units, which is sufficient for the scalar fields read
    // here).
    inline bool
    read_json_string(
        const std::string& _s,
        std::size_t        _pos,
        std::string&       _out,
        std::size_t&       _end
    )
    {
        // require the opening quote
        if ( (_pos >= _s.size()) ||
             (_s[_pos] != '"') )
        {
            return false;
        }

        std::string value;
        std::size_t i = _pos + 1;

        // consume characters until the unescaped closing quote
        while (i < _s.size())
        {
            const char c = _s[i++];

            if (c == '"')
            {
                _out.swap(value);
                _end = i;

                return true;
            }

            // an escape, unless the backslash is the last character
            if ( (c == '\\') &&
                 (i < _s.size()) )
            {
                read_escape(_s,
                            i,
                            value);
            }
            else
            {
                value.push_back(c);
            }
        }

        return false;
    }

    // string_value_at_key
    //   function: from `_from`, finds the first occurrence of `"key"` used as a
    // key (i.e. followed by ':') and reads its string value. Returns false when
    // no such key/string pair is found. This is a targeted reader, not a full
    // JSON parser; it is adequate for the unique top-level scalar fields the
    // response surface exposes.
    inline bool
    string_value_at_key(
        const std::string& _s,
        const std::string& _key,
        std::size_t        _from,
        std::string&       _out
    )
    {
        const std::string needle = "\"" + _key + "\"";
        std::size_t       pos    = _s.find(needle,
                                           _from);

        // scan each candidate key occurrence
        while (pos != std::string::npos)
        {
            std::size_t i = pos + needle.size();

            // skip insignificant whitespace before the ':'
            while ( (i < _s.size()) &&
                    (::d_net_ascii_is_space(_s[i])) )
            {
                ++i;
            }

            // a ':' confirms this is a key, not a matching string value
            if ( (i < _s.size()) &&
                 (_s[i] == ':') )
            {
                ++i;

                // skip whitespace before the value
                while ( (i < _s.size()) &&
                        (::d_net_ascii_is_space(_s[i])) )
                {
                    ++i;
                }

                if ( (i < _s.size()) &&
                     (_s[i] == '"') )
                {
                    std::size_t end = 0;

                    if (read_json_string(_s,
                                         i,
                                         _out,
                                         end))
                    {
                        return true;
                    }
                }
            }

            pos = _s.find(needle,
                          pos + needle.size());
        }

        return false;
    }

    // json_string
    //   function: convenience -- the first string value for `_key`, or "".
    inline std::string
    json_string(
        const std::string& _s,
        const std::string& _key
    )
    {
        std::string out;

        return string_value_at_key(_s,
                                   _key,
                                   0,
                                   out) ? out : std::string();
    }

    // json_string_after
    //   function: the first string value for `_key` occurring AFTER the first
    // appearance of `"_anchor"` (used to reach a nested field such as the
    // subtype inside an error object), or "".
    inline std::string
    json_string_after(
        const std::string& _s,
        const std::string& _anchor,
        const std::string& _key
    )
    {
        const std::string anchor = "\"" + _anchor + "\"";
        const std::size_t pos    = _s.find(anchor);

        if (pos == std::string::npos)
        {
            return std::string();
        }

        std::string out;

        return string_value_at_key(_s,
                                   _key,
                                   pos,
                                   out) ? out : std::string();
    }

    // concat_string_values
    //   function: concatenates the string values of EVERY `"key":` occurrence
    // (used to join the text of all response content blocks). Returns the
    // concatenation, empty when the key never appears as a key.
    inline std::string
    concat_string_values(
        const std::string& _s,
        const std::string& _key
    )
    {
        const std::string needle = "\"" + _key + "\"";
        std::string       result;
        std::size_t       from = 0;

        // walk every key occurrence, appending each string value
        for (;;)
        {
            std::size_t pos = _s.find(needle,
                                      from);

            if (pos == std::string::npos)
            {
                break;
            }

            std::size_t i = pos + needle.size();

            // skip whitespace before ':'
            while ( (i < _s.size()) &&
                    (::d_net_ascii_is_space(_s[i])) )
            {
                ++i;
            }

            if ( (i < _s.size()) &&
                 (_s[i] == ':') )
            {
                ++i;

                while ( (i < _s.size()) &&
                        (::d_net_ascii_is_space(_s[i])) )
                {
                    ++i;
                }

                if ( (i < _s.size()) &&
                     (_s[i] == '"') )
                {
                    std::string piece;
                    std::size_t end = 0;

                    if (read_json_string(_s,
                                         i,
                                         piece,
                                         end))
                    {
                        result += piece;
                        from    = end;

                        continue;
                    }
                }
            }

            from = pos + needle.size();
        }

        return result;
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///            II.   MESSAGES: ROLE, MESSAGE, REQUEST, RESPONSE            ///
///////////////////////////////////////////////////////////////////////////////

// role_type
//   enum: the author of a message in a conversation. (The type is named
// role_type so that message may carry a natural `role` member without the name
// colliding with the type inside class scope.)
struct role_type
{
    enum value
    {
        user,
        assistant
    };
};

// to_string(role_type)
//   function: the API token for a role ("user" / "assistant").
D_NODISCARD inline const char*
to_string(
    role_type::value _role
)
{
    switch (_role)
    {
        case role_type::user:      return "user";
        case role_type::assistant: return "assistant";
    }

    return "user";
}

// message
//   struct: one conversational turn -- a role plus its text content. Content is
// serialized as a JSON string (the API's shorthand for a single text block).
struct message
{
    role_type::value role;
    std::string      content;

    message()
        : role(role_type::user),
          content()
    {
    }

    message(
        role_type::value   _role,
        const std::string& _content
    )
        : role(_role),
          content(_content)
    {
    }
};

// user_message
//   function: constructs a user-authored message.
D_NODISCARD inline message
user_message(
    const std::string& _content
)
{
    return message(role_type::user,
                   _content);
}

// assistant_message
//   function: constructs an assistant-authored message.
D_NODISCARD inline message
assistant_message(
    const std::string& _content
)
{
    return message(role_type::assistant,
                   _content);
}

// message_request
//   struct: the parameters of a Messages API call. `model`, `max_tokens`, and
// at least one message are required; the remaining fields are omitted from the
// serialized body when left at their unset sentinels (negative numbers / empty
// containers).
struct message_request
{
    std::string              model;
    std::vector<message>     messages;
    std::string              system;          // omitted when empty
    int                      max_tokens;
    double                   temperature;     // omitted when < 0
    double                   top_p;           // omitted when < 0
    int                      top_k;           // omitted when < 0
    std::vector<std::string> stop_sequences;  // omitted when empty
    bool                     stream;

    message_request()
        : model(default_model),
          messages(),
          system(),
          max_tokens(1024),
          temperature(-1.0),
          top_p(-1.0),
          top_k(-1),
          stop_sequences(),
          stream(false)
    {
    }

    // add_message
    //   function: appends a message; returns *this for chaining.
    message_request&
    add_message(
        role_type::value   _role,
        const std::string& _content
    )
    {
        messages.push_back(message(_role,
                                   _content));

        return *this;
    }

    // add_user / add_assistant
    //   function: append a user / assistant turn; return *this for chaining.
    message_request&
    add_user(
        const std::string& _content
    )
    {
        return add_message(role_type::user,
                           _content);
    }

    message_request&
    add_assistant(
        const std::string& _content
    )
    {
        return add_message(role_type::assistant,
                           _content);
    }

    // to_json
    //   function: serializes this request to the JSON body the Messages API
    // expects.
    D_NODISCARD std::string
    to_json() const
    {
        internal::json_writer w;

        w.begin_object();

        w.key("model");
        w.value(model);

        w.key("max_tokens");
        w.value_int(max_tokens);

        // optional top-level system prompt
        if (!system.empty())
        {
            w.key("system");
            w.value(system);
        }

        // the conversation, then the sampling controls that are set
        write_messages(w);
        write_sampling(w);

        // streaming toggle (only emitted when enabled)
        if (stream)
        {
            w.key("stream");
            w.value_bool(true);
        }

        w.end_object();

        return w.str();
    }

private:
    // write_messages
    //   function: the conversation, as to_json's "messages" array.
    void
    write_messages(
        internal::json_writer& _writer
    ) const
    {
        _writer.key("messages");
        _writer.begin_array();

        // each turn as its role and content
        for (std::size_t i = 0; i < messages.size(); ++i)
        {
            _writer.begin_object();
            _writer.key("role");
            _writer.value(to_string(messages[i].role));
            _writer.key("content");
            _writer.value(messages[i].content);
            _writer.end_object();
        }

        _writer.end_array();

        return;
    }

    // write_sampling
    //   function: the optional sampling controls and stop sequences, each
    // only when it is set.
    void
    write_sampling(
        internal::json_writer& _writer
    ) const
    {
        // optional sampling controls
        if (temperature >= 0.0)
        {
            _writer.key("temperature");
            _writer.value_double(temperature);
        }

        if (top_p >= 0.0)
        {
            _writer.key("top_p");
            _writer.value_double(top_p);
        }

        if (top_k >= 0)
        {
            _writer.key("top_k");
            _writer.value_int(top_k);
        }

        // optional stop sequences
        if (!stop_sequences.empty())
        {
            _writer.key("stop_sequences");
            _writer.begin_array();

            for (std::size_t i = 0; i < stop_sequences.size(); ++i)
            {
                _writer.value(stop_sequences[i]);
            }

            _writer.end_array();
        }

        return;
    }
};

// message_response
//   struct: the outcome of a Messages API call. It carries the neutral HTTP
// response (status, headers, raw JSON body, transport error) and exposes
// targeted accessors for the fields callers most often need. The complete
// body is always available via raw() for full JSON parsing.
struct message_response
{
    response http;

    // ok
    //   function: whether the call both completed transport-cleanly and
    // returned a 2xx status.
    D_NODISCARD bool
    ok() const
    {
        return ( (http.error == transport_error::none) &&
                 http.ok() );
    }

    // status
    //   function: the HTTP status code (0 if none was received).
    D_NODISCARD int
    status() const
    {
        return http.status;
    }

    // transport
    //   function: the library-neutral transport error (none on success).
    D_NODISCARD transport_error::value
    transport() const
    {
        return http.error;
    }

    // raw
    //   function: the raw JSON response body.
    D_NODISCARD const std::string&
    raw() const
    {
        return http.body;
    }

    // text
    //   function: the assistant's reply as the concatenation of every text
    // content block. Best-effort over the raw JSON; empty on an error response
    // or when no text block is present.
    D_NODISCARD std::string
    text() const
    {
        return internal::concat_string_values(http.body,
                                              "text");
    }

    // id / model / stop_reason
    //   function: the corresponding top-level response fields, or "".
    D_NODISCARD std::string
    id() const
    {
        return internal::json_string(http.body,
                                     "id");
    }

    D_NODISCARD std::string
    model() const
    {
        return internal::json_string(http.body,
                                     "model");
    }

    D_NODISCARD std::string
    stop_reason() const
    {
        return internal::json_string(http.body,
                                     "stop_reason");
    }

    // is_error
    //   function: whether the call did not succeed (transport failure or a
    // non-2xx status, which for this API carries an error document).
    D_NODISCARD bool
    is_error() const
    {
        return !ok();
    }

    // error_type
    //   function: the API error subtype (e.g. "invalid_request_error") read
    // from the error object, or "".
    D_NODISCARD std::string
    error_type() const
    {
        return internal::json_string_after(http.body,
                                           "error",
                                           "type");
    }

    // error_message
    //   function: the API error message read from the error object, or "".
    D_NODISCARD std::string
    error_message() const
    {
        return internal::json_string_after(http.body,
                                           "error",
                                           "message");
    }
};


///////////////////////////////////////////////////////////////////////////////
///                III.   SERVER-SENT-EVENTS PARSER                        ///
///////////////////////////////////////////////////////////////////////////////

// sse_parser
//   class: an incremental parser for the text/event-stream framing the
// streaming Messages API emits. Bytes are handed in via feed() as they arrive;
// each completed event (terminated by a blank line) invokes the callback with
// the event name and the accumulated data payload. The callback returns false
// to request that streaming stop (surfaced as transport_error::canceled).
class sse_parser
{
public:
    // event_fn
    //   type: the per-event callback -- (event_name, data) -> keep_going.
    typedef re_std::function<bool(const std::string&,
                                  const std::string&)> event_fn;

    explicit sse_parser(
        event_fn _on_event
    )
        : m_on_event(_on_event),
          m_buffer(),
          m_event(),
          m_data(),
          m_stopped(false)
    {
    }

    // feed
    //   function: consumes a chunk of stream bytes, dispatching any events it
    // completes. Returns false once the consumer has asked to stop (after which
    // further input is ignored).
    bool
    feed(
        const char* _data,
        std::size_t _length
    )
    {
        // honor an earlier stop request
        if (m_stopped)
        {
            return false;
        }

        m_buffer.append(_data,
                        _length);

        // process every complete line currently buffered, CRLF or LF
        for (;;)
        {
            const ::d_pack_text held  = { m_buffer.data(),
                                          m_buffer.size() };
            ::d_net_line        found = { { D_NULLPTR, 0u }, 0u, false };

            // the rest waits for its LF
            if (!::d_net_line_find(held,
                                   0u,
                                   &found))
            {
                break;
            }

            const std::string line(found.text.data,
                                   found.text.length);

            m_buffer.erase(0,
                           found.size);

            // a blank line terminates the current event
            if (line.empty())
            {
                if (!dispatch())
                {
                    m_stopped = true;

                    return false;
                }

                continue;
            }

            // a leading ':' marks a comment line
            if (line[0] == ':')
            {
                continue;
            }

            handle_field(line);
        }

        return true;
    }

    // finish
    //   function: flushes any final unterminated event at end of stream.
    // Returns false if the consumer asked to stop.
    bool
    finish()
    {
        if (m_stopped)
        {
            return false;
        }

        return dispatch();
    }

    // stopped
    //   function: whether the consumer has requested a stop.
    D_NODISCARD bool
    stopped() const
    {
        return m_stopped;
    }

private:
    // handle_field
    //   function: applies one "field: value" line to the pending event.
    void
    handle_field(
        const std::string& _line
    )
    {
        const std::size_t colon = _line.find(':');

        std::string field;
        std::string value;

        // split on the first ':'; a line without one is a bare field name
        if (colon == std::string::npos)
        {
            field = _line;
        }
        else
        {
            field = _line.substr(0,
                                 colon);

            std::size_t vstart = colon + 1;

            // a single leading space after ':' is not part of the value
            if ( (vstart < _line.size()) &&
                 (_line[vstart] == ' ') )
            {
                ++vstart;
            }

            value = _line.substr(vstart);
        }

        // accumulate the event name and data payload
        if (field == "event")
        {
            m_event = value;
        }
        else if (field == "data")
        {
            if (!m_data.empty())
            {
                m_data.push_back('\n');
            }

            m_data += value;
        }

        // "id" and "retry" fields are recognized by SSE but unused here

        return;
    }

    // dispatch
    //   function: delivers the pending event (if any) and resets the
    // accumulators. Returns whether streaming should continue.
    bool
    dispatch()
    {
        // nothing pending -> nothing to do
        if ( m_event.empty() &&
             m_data.empty() )
        {
            return true;
        }

        std::string ev;
        std::string da;
        ev.swap(m_event);
        da.swap(m_data);

        // no callback keeps the stream going
        return (!m_on_event.empty()) ? m_on_event(ev,
                                                  da)
                                     : true;
    }

    event_fn    m_on_event;
    std::string m_buffer;
    std::string m_event;
    std::string m_data;
    bool        m_stopped;
};


///////////////////////////////////////////////////////////////////////////////
///                  IV.   CLIENT + CONVENIENCE                            ///
///////////////////////////////////////////////////////////////////////////////

// client
//   class: a configured entry point to the Messages API. Holds the API key and
// endpoint/version settings plus the transport options, and turns a
// message_request into an HTTP exchange (buffered via send(), or streamed via
// stream()).
class client
{
public:
    explicit client(
        const std::string& _api_key,
        const std::string& _base_url = api_base_url,
        const std::string& _version  = default_version
    )
        : m_api_key(_api_key),
          m_base_url(_base_url),
          m_version(_version),
          m_beta(),
          m_options(),
          m_extra_headers()
    {
    }

    // set_beta
    //   function: sets the anthropic-beta header value (empty to clear).
    // Returns *this for chaining.
    client&
    set_beta(
        const std::string& _beta
    )
    {
        m_beta = _beta;

        return *this;
    }

    // add_header
    //   function: adds/overrides a default request header applied to every
    // call. Returns *this for chaining.
    client&
    add_header(
        const std::string& _name,
        const std::string& _value
    )
    {
        set_header(m_extra_headers,
                   _name,
                   _value);

        return *this;
    }

    // options
    //   function: mutable access to the transport options (timeouts, TLS
    // verification, verbosity, ...).
    D_NODISCARD curl::options&
    options()
    {
        return m_options;
    }

    // options (const)
    //   function: read-only access to the transport options.
    D_NODISCARD const curl::options&
    options() const
    {
        return m_options;
    }

    // build_request
    //   function: assembles the neutral net::request (URL, auth/version/content
    // headers, any defaults, and the serialized body) for a message_request.
    D_NODISCARD request
    build_request(
        const message_request& _request
    ) const
    {
        request req;

        req.method = net::http_method::post;
        req.url    = m_base_url + messages_path;

        req.set_header(header_api_key,
                       m_api_key);
        req.set_header(header_version,
                       m_version);
        req.set_header(http_field::content_type,
                       http_media::json);

        // optional beta opt-in header
        if (!m_beta.empty())
        {
            req.set_header(header_beta,
                           m_beta);
        }

        // caller-supplied default headers
        for (std::size_t i = 0; i < m_extra_headers.size(); ++i)
        {
            set_header(req.headers,
                       m_extra_headers[i].first,
                       m_extra_headers[i].second);
        }

        req.body = _request.to_json();

        return req;
    }

    // send
    //   function: performs a buffered (non-streaming) Messages call and returns
    // the wrapped response.
    D_NODISCARD message_response
    send(
        const message_request& _request
    ) const
    {
        message_response out;

        const request req = build_request(_request);
        out.http = curl::perform(req,
                                 m_options);

        return out;
    }

    // stream
    //   function: performs a streaming Messages call, forwarding each SSE event
    // to `_on_event` (which returns false to stop early). Response status and
    // headers land in `_meta`; the streamed body is delivered through the
    // callback rather than buffered. Returns the neutral transport error.
    // Note: `stream` is forced on in the sent request regardless of the flag on
    // `_request`.
    D_NODISCARD transport_error::value
    stream(
        const message_request&    _request,
        const sse_parser::event_fn& _on_event,
        message_response&         _meta
    ) const
    {
        message_request streamed = _request;
        streamed.stream = true;

        const request req = build_request(streamed);

        sse_parser parser(_on_event);

        // bridge libcurl body chunks into the SSE parser
        const curl::body_sink sink = sse_feeder(&parser);

        const transport_error::value err =
            curl::perform_stream(req,
                                 sink,
                                 _meta.http,
                                 m_options);

        parser.finish();

        return err;
    }

private:
    // sse_feeder
    //   struct: stream()'s sink: hands each body chunk to the SSE parser.
    struct sse_feeder
    {
        explicit sse_feeder(
            sse_parser* _parser
        )
            : parser(_parser)
        {}

        bool
        operator()(
            const char* _data,
            std::size_t _length
        ) const
        {
            return parser->feed(_data,
                                _length);
        }

        sse_parser* parser;
    };

    std::string     m_api_key;
    std::string     m_base_url;
    std::string     m_version;
    std::string     m_beta;
    curl::options   m_options;
    header_list     m_extra_headers;
};

// quick_message
//   function: one-shot convenience -- send a single user prompt with a default
// client and return the response. `_model` defaults to default_model.
D_NODISCARD inline message_response
quick_message(
    const std::string& _api_key,
    const std::string& _prompt,
    const std::string& _model      = default_model,
    int                _max_tokens = 1024
)
{
    client c(_api_key);

    message_request req;
    req.model      = _model;
    req.max_tokens = _max_tokens;
    req.add_user(_prompt);

    return c.send(req);
}


NS_END  // claude
NS_END  // vendor
NS_END  // net
NS_END  // djinterp


#endif  // DJINTERP_NET_API_CLAUDE_HPP
