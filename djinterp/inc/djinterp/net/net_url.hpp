/*******************************************************************************
* djinterp [net]                                                     net_url.hpp
*
* URLs in C++, over net/net_url.h.
*   An owning url holds its text and the C parse of it; every accessor returns
* a view into that text, so reading a component never allocates. Copies and
* moves rebase the views onto the copy's own text, so no url ever points into
* another. It supplies
*     - the error, host, and part vocabulary as enumerations scoped in
*       structs (decision 3.4), each enumerator defined as its C
*       counterpart                                                        [1]
*     - url, and url_result for parses that may fail                       [2]
*     - parsing, resolution, normalization, percent-encoding, schemes, and
*       endpoints, each a thin call into the C foundation                  [3]
*   C++98 and later; moves arrive with C++11, and below it copies and swap do
* their work. Text the C foundation produces is written first into a buffer
* on the stack, and copied out once; only text longer than that buffer costs
* a second call, at the exact length the first one reported.
*
*
* path:      /inc/djinterp/net/net_url.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VOCABULARY
    ----------
    1.  Enumerations
         1.  url_error
         2.  url_host
         3.  url_part
    2.  Conversions
2.  THE URL
    -------
    1.  The class
         1.  url
    2.  Results
         1.  url_result
         2.  url_text_result
3.  OPERATIONS
    ----------
    1.  Parsing
    2.  Resolution and normalization
    3.  Percent-encoding
    4.  Schemes and endpoints
4.  SCHEMES, QUERIES AND URL PARTS
    ------------------------------
    1.  Schemes
         1.  url_scheme
         2.  to_string(url_scheme)
         3.  scheme_from_string
         4.  is_secure
         5.  default_port
    2.  Percent-encoding and queries
         1.  is_unreserved
         2.  percent_encode
         3.  percent_decode
         4.  query_param
         5.  query_list
         6.  encode_query
         7.  parse_query
    3.  URL parts
         1.  url_parts
         2.  internal::url_text
         3.  split_url
*/

#ifndef DJINTERP_NET_NET_URL_HPP
#define DJINTERP_NET_NET_URL_HPP 1

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string
#include <utility>  // std::pair
#include <vector>   // std::vector
// djinterp
#include "../djinterp.hpp"  // framework root
#include "./net_common.hpp"  // endpoint, io_error
#include "./net_url.h"      // the C URL API
// re_std
#include "../../re_std/string_view/string_view_typedefs.hpp"  // string_view


NS_DJINTERP
NS_NET


//==============================================================================
// 1.  VOCABULARY
//==============================================================================
// The C URL vocabulary under C++ names. Each enumerator is defined as its C
// counterpart, so the two cannot disagree.


// 1.1    Enumerations
//------------------------------------------------------------------------------
// 1.1.1
// url_error
//   enum: why a URL was refused, or why an output did not fit. Scoped in a
// struct at every level (decision 3.4): the type is url_error::value.
struct url_error
{
    enum value
    {
        none             = D_NET_URL_OK,
        invalid_argument = D_NET_URL_ERROR_INVALID_ARGUMENT,
        scheme           = D_NET_URL_ERROR_SCHEME,
        userinfo         = D_NET_URL_ERROR_USERINFO,
        host             = D_NET_URL_ERROR_HOST,
        ip_literal       = D_NET_URL_ERROR_IP_LITERAL,
        port             = D_NET_URL_ERROR_PORT,
        path             = D_NET_URL_ERROR_PATH,
        query            = D_NET_URL_ERROR_QUERY,
        fragment         = D_NET_URL_ERROR_FRAGMENT,
        percent          = D_NET_URL_ERROR_PERCENT,
        base             = D_NET_URL_ERROR_BASE,
        buffer_too_small = D_NET_URL_ERROR_BUFFER_TOO_SMALL
    };
};

// 1.1.2
// url_host
//   enum: what kind of host a URL's authority names; the type is
// url_host::value.
struct url_host
{
    enum value
    {
        none       = D_NET_URL_HOST_NONE,
        name       = D_NET_URL_HOST_NAME,
        ipv4       = D_NET_URL_HOST_IPV4,
        ipv6       = D_NET_URL_HOST_IPV6,
        ipv_future = D_NET_URL_HOST_IPVFUTURE
    };
};

// 1.1.3
// url_part
//   enum: the component a percent-encoding is for; the type is
// url_part::value.
struct url_part
{
    enum value
    {
        userinfo  = D_NET_URL_PART_USERINFO,
        host      = D_NET_URL_PART_HOST,
        path      = D_NET_URL_PART_PATH,
        segment   = D_NET_URL_PART_SEGMENT,
        query     = D_NET_URL_PART_QUERY,
        fragment  = D_NET_URL_PART_FRAGMENT,
        component = D_NET_URL_PART_COMPONENT
    };
};

// 1.2    Conversions
//------------------------------------------------------------------------------
// to_c / from_c -- the same values under the other language's names; the
// enumerators are defined as each other, so these are casts
D_NODISCARD inline d_net_url_error
to_c(
    url_error::value _error
) D_NOEXCEPT
{
    return static_cast<d_net_url_error>(_error);
}

D_NODISCARD inline url_error::value
from_c(
    d_net_url_error _error
) D_NOEXCEPT
{
    return static_cast<url_error::value>(_error);
}

D_NODISCARD inline d_net_url_host
to_c(
    url_host::value _host
) D_NOEXCEPT
{
    return static_cast<d_net_url_host>(_host);
}

D_NODISCARD inline url_host::value
from_c(
    d_net_url_host _host
) D_NOEXCEPT
{
    return static_cast<url_host::value>(_host);
}

D_NODISCARD inline d_net_url_part
to_c(
    url_part::value _part
) D_NOEXCEPT
{
    return static_cast<d_net_url_part>(_part);
}

D_NODISCARD inline url_part::value
from_c(
    d_net_url_part _part
) D_NOEXCEPT
{
    return static_cast<url_part::value>(_part);
}

// url_error_name -- a short phrase for an error, for messages
D_NODISCARD inline const char*
url_error_name(
    url_error::value _error
) D_NOEXCEPT
{
    return ::d_net_url_error_name(to_c(_error));
}

// url_error_to_net -- the io_error an error maps to, by the rules net.h
// gives with d_net_error
D_NODISCARD inline io_error::value
url_error_to_net(
    url_error::value _error
) D_NOEXCEPT
{
    return from_c(::d_net_url_error_to_net(to_c(_error)));
}

NS_INTERNAL

    // url_pack
    //   function: a C view of a string's characters.
    D_NODISCARD inline ::d_pack_text
    url_pack(
        const std::string& _text
    ) D_NOEXCEPT
    {
        const ::d_pack_text text = { _text.data(),
                                     _text.size() };

        return text;
    }

    // url_view
    //   function: a C++ view of a C view; an absent part is an empty view.
    D_NODISCARD inline ::re_std::string_view
    url_view(
        ::d_pack_text _text
    ) D_NOEXCEPT
    {
        return (_text.data) ? ::re_std::string_view(_text.data,
                                                    _text.length)
                            : ::re_std::string_view();
    }

    // url_rebased
    //   function: a view moved from one copy of a text to another, at the
    // same offset; an absent part points nowhere in either.
    D_NODISCARD inline ::d_pack_text
    url_rebased(
        ::d_pack_text _view,
        const char*   _from,
        const char*   _to
    ) D_NOEXCEPT
    {
        const ::d_pack_text moved = { (_view.data) ? _to + (_view.data - _from)
                                                   : D_NULLPTR,
                                      _view.length };

        return moved;
    }

    // url_produce
    //   function: runs one of the C text producers, whose signature is
    // (buffer, capacity, length) -> d_net_url_error, into a string: a stack
    // buffer first, then, if that was too small, the length it reported.
    template<typename Producer>
    D_NODISCARD url_error::value
    url_produce(
        std::string& _out,
        Producer     _produce
    )
    {
        std::size_t length = 0u;

        // most URLs fit here, costing one copy and no second call
        char local[256];

        d_net_url_error found = _produce(local,
                                         sizeof(local),
                                         &length);

        // it fit
        if (found == D_NET_URL_OK)
        {
            _out.assign(local,
                        length);

            return url_error::none;
        }

        // an error other than size is final
        if (found != D_NET_URL_ERROR_BUFFER_TOO_SMALL)
        {
            return from_c(found);
        }

        std::string text(length + 1u,
                         '\0');

        found = _produce(&text[0],
                         text.size(),
                         &length);

        // the reported length suffices, by the C contract
        if (found == D_NET_URL_OK)
        {
            text.resize(length);
            _out.swap(text);
        }

        return from_c(found);
    }

    // url_resolver, url_normalizer, url_encoder, url_decoder
    //   struct: url_produce's producers, each the C call it is named for
    // over the arguments it was made with.
    struct url_resolver
    {
        url_resolver(
            const ::d_net_url* _base,
            const ::d_net_url* _reference
        )
            : base(_base),
              reference(_reference)
        {}

        d_net_url_error
        operator()(
            char*        _buffer,
            std::size_t  _capacity,
            std::size_t* _length
        ) const
        {
            return ::d_net_url_resolve(base,
                                       reference,
                                       _buffer,
                                       _capacity,
                                       _length);
        }

        const ::d_net_url* base;
        const ::d_net_url* reference;
    };

    struct url_normalizer
    {
        explicit url_normalizer(
            const ::d_net_url* _parts
        )
            : parts(_parts)
        {}

        d_net_url_error
        operator()(
            char*        _buffer,
            std::size_t  _capacity,
            std::size_t* _length
        ) const
        {
            return ::d_net_url_normalize(parts,
                                         _buffer,
                                         _capacity,
                                         _length);
        }

        const ::d_net_url* parts;
    };

    struct url_encoder
    {
        url_encoder(
            ::d_pack_text  _raw,
            d_net_url_part _part
        )
            : raw(_raw),
              part(_part)
        {}

        d_net_url_error
        operator()(
            char*        _buffer,
            std::size_t  _capacity,
            std::size_t* _length
        ) const
        {
            return ::d_net_url_encode(raw,
                                      part,
                                      _buffer,
                                      _capacity,
                                      _length);
        }

        ::d_pack_text  raw;
        d_net_url_part part;
    };

    struct url_decoder
    {
        explicit url_decoder(
            ::d_pack_text _text
        )
            : text(_text)
        {}

        d_net_url_error
        operator()(
            char*        _buffer,
            std::size_t  _capacity,
            std::size_t* _length
        ) const
        {
            return ::d_net_url_decode(text,
                                      _buffer,
                                      _capacity,
                                      _length);
        }

        ::d_pack_text text;
    };

NS_END  // internal


//==============================================================================
// 2.  THE URL
//==============================================================================
// An owning URI reference, and the results of calls that may refuse one.


// 2.1    The class
//------------------------------------------------------------------------------
struct url_result;

// 2.1.1
// url
//   class: an owning URI reference. Its accessors return views into its own
// text, valid until the url is changed or destroyed; copies and moves rebase
// the views onto the new owner's text. A default url is the empty reference.
class url
{
public:
    typedef ::re_std::string_view view_type;

    url()
        : m_text(),
          m_parts()
    {
        m_parse();
    }

    url(
        const url& _other
    )
        : m_text(_other.m_text),
          m_parts(_other.m_parts)
    {
        m_rebase(_other.m_text.data());
    }

#if (D_MOVE_ENABLED == 1)
    url(
        url&& _other
    ) D_NOEXCEPT
        : m_text(),
          m_parts(_other.m_parts)
    {
        const char* const from = _other.m_text.data();

        m_text = static_cast<std::string&&>(_other.m_text);
        m_rebase(from);
        _other.m_clear();
    }
#endif  // D_MOVE_ENABLED

    url&
    operator=(
        const url& _other
    )
    {
        // copied first, so a failed allocation leaves this url as it was
        url copy(_other);

        swap(copy);

        return *this;
    }

#if (D_MOVE_ENABLED == 1)
    url&
    operator=(
        url&& _other
    ) D_NOEXCEPT
    {
        // assigning a url to itself leaves it unchanged
        if (this != &_other)
        {
            const char* const from = _other.m_text.data();

            m_text  = static_cast<std::string&&>(_other.m_text);
            m_parts = _other.m_parts;
            m_rebase(from);
            _other.m_clear();
        }

        return *this;
    }
#endif  // D_MOVE_ENABLED

    ~url() {}

    /**
     * @brief Exchanges two urls' texts and parts.
     *
     * @param[in,out] _other  the url to exchange with.
     * @post each url's views lie in its own text, wherever the exchange put
     *       the characters.
     */
    void
    swap(
        url& _other
    )
    {
        const char* const mine   = m_text.data();
        const char* const theirs = _other.m_text.data();
        const ::d_net_url parts  = m_parts;

        m_text.swap(_other.m_text);
        m_parts        = _other.m_parts;
        _other.m_parts = parts;

        // each side's views still point where the other's text was
        m_rebase(theirs);
        _other.m_rebase(mine);

        return;
    }

    // the text, whole and by component; absent components are empty views,
    // told from empty present ones by the has_ accessors
    D_NODISCARD const std::string&
    str() const D_NOEXCEPT
    {
        return m_text;
    }

    D_NODISCARD view_type
    scheme() const D_NOEXCEPT
    {
        return internal::url_view(m_parts.scheme);
    }

    D_NODISCARD view_type
    userinfo() const D_NOEXCEPT
    {
        return internal::url_view(m_parts.userinfo);
    }

    D_NODISCARD view_type
    host() const D_NOEXCEPT
    {
        return internal::url_view(m_parts.host);
    }

    D_NODISCARD view_type
    port_text() const D_NOEXCEPT
    {
        return internal::url_view(m_parts.port_text);
    }

    D_NODISCARD view_type
    path() const D_NOEXCEPT
    {
        return internal::url_view(m_parts.path);
    }

    D_NODISCARD view_type
    query() const D_NOEXCEPT
    {
        return internal::url_view(m_parts.query);
    }

    D_NODISCARD view_type
    fragment() const D_NOEXCEPT
    {
        return internal::url_view(m_parts.fragment);
    }

    // presence, kind, and port
    D_NODISCARD bool
    has_authority() const D_NOEXCEPT
    {
        return m_parts.has_authority;
    }

    D_NODISCARD bool
    has_userinfo() const D_NOEXCEPT
    {
        return m_parts.has_userinfo;
    }

    D_NODISCARD bool
    has_port() const D_NOEXCEPT
    {
        return m_parts.has_port;
    }

    D_NODISCARD bool
    has_query() const D_NOEXCEPT
    {
        return m_parts.has_query;
    }

    D_NODISCARD bool
    has_fragment() const D_NOEXCEPT
    {
        return m_parts.has_fragment;
    }

    D_NODISCARD bool
    is_absolute() const D_NOEXCEPT
    {
        return ::d_net_url_is_absolute(&m_parts);
    }

    D_NODISCARD url_host::value
    host_kind() const D_NOEXCEPT
    {
        return from_c(m_parts.host_kind);
    }

    // port -- the explicit port, or 0; effective_port -- that, or else the
    // scheme's default; is_tls -- whether the scheme begins with TLS
    D_NODISCARD port_type
    port() const D_NOEXCEPT
    {
        return m_parts.port;
    }

    D_NODISCARD port_type
    effective_port() const D_NOEXCEPT
    {
        return ( (m_parts.has_port) &&
                 (m_parts.port_text.length > 0u) )
                   ? m_parts.port
                   : ::d_net_url_default_port(m_parts.scheme);
    }

    D_NODISCARD bool
    is_tls() const D_NOEXCEPT
    {
        return ::d_net_url_scheme_is_tls(m_parts.scheme);
    }

    // the C parse, for calls into the C foundation
    D_NODISCARD const ::d_net_url&
    c_url() const D_NOEXCEPT
    {
        return m_parts;
    }

    /**
     * @brief Parses a URI reference into a url that owns its text.
     *
     * @param[in] _text  the reference, moved into the url.
     * @return the url; or the error and the offset of the first character
     *         refused, with the empty reference as the url.
     */
    D_NODISCARD static url_result
    parse(std::string _text);

    /**
     * @brief Resolves a reference against this url (RFC 3986, 5.2).
     *
     * @param[in] _reference  the reference to resolve.
     * @return the target; or url_error::base when this url is relative.
     */
    D_NODISCARD url_result
    resolve(const url& _reference) const;

    /**
     * @brief This url's normal form, for comparison (RFC 3986, 6.2).
     *
     * @return the normalized url.
     */
    D_NODISCARD url_result
    normalized() const;

private:
    std::string  m_text;
    ::d_net_url  m_parts;

    // m_parse -- parses the text in place; used only on text known to parse
    void
    m_parse() D_NOEXCEPT
    {
        (void)::d_net_url_parse(internal::url_pack(m_text),
                                &m_parts,
                                D_NULLPTR);

        return;
    }

    // m_clear -- becomes the empty reference
    void
    m_clear() D_NOEXCEPT
    {
        m_text.clear();
        m_parse();

        return;
    }

    // m_rebase -- moves every view from a copy of the text at _from onto
    // this url's own text, at the same offsets
    void
    m_rebase(
        const char* _from
    ) D_NOEXCEPT
    {
        const char* const to = m_text.data();

        // a view member of the C parse; the alias keeps "::d_pack_text" and
        // "::d_net_url" from reading as one qualified name
        typedef ::d_pack_text d_net_url::* view_member;

        // every view the C parse holds
        const view_member VIEWS[] =
        {
            &::d_net_url::text,
            &::d_net_url::scheme,
            &::d_net_url::userinfo,
            &::d_net_url::host,
            &::d_net_url::port_text,
            &::d_net_url::path,
            &::d_net_url::query,
            &::d_net_url::fragment
        };

        // each view, at the same offset in this url's text
        for (std::size_t i = 0u; i < sizeof(VIEWS) / sizeof(VIEWS[0]); ++i)
        {
            m_parts.*VIEWS[i] = internal::url_rebased(m_parts.*VIEWS[i],
                                                      _from,
                                                      to);
        }

        return;
    }
};

// 2.2    Results
//------------------------------------------------------------------------------
// 2.2.1
// url_result
//   struct: a url, or why none could be made: the error, and the offset of
// the first character refused. On failure the url is the empty reference.
struct url_result
{
    url              value;
    url_error::value error;
    std::size_t      position;

    url_result()
        : value(),
          error(url_error::none),
          position(0u)
    {}

    D_NODISCARD bool
    ok() const D_NOEXCEPT
    {
        return (error == url_error::none);
    }
};

// 2.2.2
// url_text_result
//   struct: text a call produced, or why it refused its input.
struct url_text_result
{
    std::string      text;
    url_error::value error;

    url_text_result()
        : text(),
          error(url_error::none)
    {}

    D_NODISCARD bool
    ok() const D_NOEXCEPT
    {
        return (error == url_error::none);
    }
};


//==============================================================================
// 3.  OPERATIONS
//==============================================================================
// Each a thin call into the C foundation.


// 3.1    Parsing
//------------------------------------------------------------------------------
// url::parse -- the text moved into the url, and parsed where it now lives
inline url_result
url::parse(
    std::string _text
)
{
    url_result  result;
    std::size_t at = 0u;

    result.value.m_text.swap(_text);

    const d_net_url_error found = ::d_net_url_parse(
                                      internal::url_pack(result.value.m_text),
                                      &result.value.m_parts,
                                      &at);

    // a refused text leaves the empty reference, and says why and where
    if (found != D_NET_URL_OK)
    {
        result.value.m_clear();
        result.error    = from_c(found);
        result.position = at;
    }

    return result;
}

// 3.2    Resolution and normalization
//------------------------------------------------------------------------------
// url::resolve -- the target's text produced by the C resolver, then parsed
// as a url of its own
inline url_result
url::resolve(
    const url& _reference
) const
{
    const ::d_net_url* const base      = &m_parts;
    const ::d_net_url* const reference = &_reference.m_parts;
    url_result               result;
    std::string              text;

    result.error = internal::url_produce(text,
                                         internal::url_resolver(base,
                                                                reference));

    // the target parses, since the resolver writes only valid references
    if (result.error == url_error::none)
    {
        result = url::parse(text);
    }

    return result;
}

// url::normalized -- the normal form's text, then parsed as a url
inline url_result
url::normalized() const
{
    const ::d_net_url* const parts = &m_parts;
    url_result               result;
    std::string              text;

    result.error = internal::url_produce(text,
                                         internal::url_normalizer(parts));

    // the normal form of a valid reference is valid
    if (result.error == url_error::none)
    {
        result = url::parse(text);
    }

    return result;
}

// 3.3    Percent-encoding
//------------------------------------------------------------------------------
// url_encode -- raw bytes encoded for a component; a url_part is always a
// valid part, so only the buffer's size can intervene, and never fatally
D_NODISCARD inline std::string
url_encode(
    const std::string& _raw,
    url_part::value    _part
)
{
    const ::d_pack_text  raw  = internal::url_pack(_raw);
    const d_net_url_part part = to_c(_part);
    std::string          text;

    (void)internal::url_produce(text,
                                internal::url_encoder(raw,
                                                      part));

    return text;
}

/**
 * @brief Decodes percent escapes; everything else is copied as it is.
 *
 * @param[in] _text  the encoded text.
 * @return the decoded bytes, which may hold NULs; or url_error::percent
 *         for a malformed escape, with no text.
 */
D_NODISCARD inline url_text_result
url_decode(
    const std::string& _text
)
{
    const ::d_pack_text text = internal::url_pack(_text);
    url_text_result     result;

    result.error = internal::url_produce(result.text,
                                         internal::url_decoder(text));

    return result;
}

// 3.4    Schemes and endpoints
//------------------------------------------------------------------------------
// url_default_port, url_scheme_is_tls -- net_url.h's, for a scheme string
D_NODISCARD inline port_type
url_default_port(
    const std::string& _scheme
) D_NOEXCEPT
{
    return ::d_net_url_default_port(internal::url_pack(_scheme));
}

D_NODISCARD inline bool
url_scheme_is_tls(
    const std::string& _scheme
) D_NOEXCEPT
{
    return ::d_net_url_scheme_is_tls(internal::url_pack(_scheme));
}

/**
 * @brief The TCP endpoint a URL names: its host, and its explicit port or
 *        its scheme's default.
 *
 * @param[in]  _url  the URL.
 * @param[out] _out  receives the endpoint; untouched on failure.
 * @return io_error::none, or io_error::address_invalid when the URL names
 *         no reachable host: no authority or host, an IPvFuture address, a
 *         non-ASCII name, or no port.
 */
D_NODISCARD inline io_error::value
url_endpoint(
    const url& _url,
    endpoint&  _out
)
{
    ::d_net_endpoint record = ::d_net_endpoint();

    const io_error::value found = from_c(::d_net_url_endpoint(&_url.c_url(),
                                                              &record));

    // only a found endpoint is copied out
    if (found == io_error::none)
    {
        _out = from_c(record);
    }

    return found;
}


//==============================================================================
// 4.  SCHEMES, QUERIES AND URL PARTS
//==============================================================================
// Schemes by name, percent-encoding and query strings over std::string, and
// an absolute URL split into its parts.


// 4.1    Schemes
//------------------------------------------------------------------------------
// 4.1.1
// url_scheme
//   enum: the URI schemes this vocabulary recognizes. `unknown` is the zero
// value for an unrecognized or absent scheme.
struct url_scheme
{
    enum value
    {
        unknown = 0,
        http,
        https,
        ws,
        wss,
        ftp,
        ftps,
        file,
        data
    };
};

// 4.1.2
// to_string(url_scheme)
//   function: the lowercase scheme token (e.g. "https"); "" when unknown.
D_NODISCARD inline D_CONSTEXPR_CPP14 const char*
to_string(
    url_scheme::value _scheme
)
{
    switch (_scheme)
    {
        case url_scheme::http:  return "http";
        case url_scheme::https: return "https";
        case url_scheme::ws:    return "ws";
        case url_scheme::wss:   return "wss";
        case url_scheme::ftp:   return "ftp";
        case url_scheme::ftps:  return "ftps";
        case url_scheme::file:  return "file";
        case url_scheme::data:  return "data";
        case url_scheme::unknown: return "";
    }

    return "";
}

// 4.1.3
// scheme_from_string
//   function: maps a (case-insensitive) scheme token to a url_scheme, or
// url_scheme::unknown when unrecognized.
D_NODISCARD inline url_scheme::value
scheme_from_string(
    const std::string& _scheme
)
{
    static const struct
    {
        const char*       token;
        url_scheme::value scheme;
    } schemes[] =
    {
        { "http",  url_scheme::http  },
        { "https", url_scheme::https },
        { "ws",    url_scheme::ws    },
        { "wss",   url_scheme::wss   },
        { "ftp",   url_scheme::ftp   },
        { "ftps",  url_scheme::ftps  },
        { "file",  url_scheme::file  },
        { "data",  url_scheme::data  }
    };
    const ::d_pack_text scheme = { _scheme.data(),
                                   _scheme.size() };

    // the first token that matches names the scheme
    for (std::size_t i = 0; i < (sizeof(schemes) / sizeof(schemes[0])); ++i)
    {
        if (::d_net_ascii_ieq_cstr(scheme,
                                   schemes[i].token))
        {
            return schemes[i].scheme;
        }
    }

    return url_scheme::unknown;
}

// 4.1.4
// is_secure
//   function: whether a scheme implies transport security (TLS).
D_NODISCARD inline D_CONSTEXPR_CPP14 bool
is_secure(
    url_scheme::value _scheme
)
{
    return ( (_scheme == url_scheme::https) ||
             (_scheme == url_scheme::wss)   ||
             (_scheme == url_scheme::ftps) );
}

// 4.1.5
// default_port
//   function: the well-known default port for a scheme, or 0 when the scheme
// has no network port (e.g. file/data) or is unknown.
D_NODISCARD inline D_CONSTEXPR_CPP14 port_type
default_port(
    url_scheme::value _scheme
)
{
    switch (_scheme)
    {
        case url_scheme::http:  return 80;
        case url_scheme::https: return 443;
        case url_scheme::ws:    return 80;
        case url_scheme::wss:   return 443;
        case url_scheme::ftp:   return 21;
        case url_scheme::ftps:  return 990;
        case url_scheme::file:  return 0;
        case url_scheme::data:  return 0;
        case url_scheme::unknown: return 0;
    }

    return 0;
}

// 4.2    Percent-encoding and queries
//------------------------------------------------------------------------------
// 4.2.1
// is_unreserved
//   function: whether a byte is an RFC 3986 "unreserved" character
// (ALPHA / DIGIT / '-' / '.' / '_' / '~') and thus never percent-encoded.
D_NODISCARD inline D_CONSTEXPR bool
is_unreserved(
    char _c
)
{
    return ( ( (_c >= 'A') && (_c <= 'Z') ) ||
             ( (_c >= 'a') && (_c <= 'z') ) ||
             ( (_c >= '0') && (_c <= '9') ) ||
             (_c == '-') || (_c == '.')     ||
             (_c == '_') || (_c == '~') );
}

// 4.2.2
// percent_encode
//   function: percent-encodes `_input` per RFC 3986, escaping every byte that
// is not "unreserved" as %XX (uppercase): net_url.hpp's url_encode for a
// component. Suitable for a single URL component (path segment, query
// key/value).
D_NODISCARD inline std::string
percent_encode(
    const std::string& _input
)
{
    return ::djinterp::net::url_encode(_input,
                                       ::djinterp::net::url_part::component);
}

// 4.2.3
// percent_decode
//   function: reverses percent-encoding, turning each valid %XX into its byte.
// A '+' becomes a space only when `_plus_as_space` is true (the
// application/x-www-form-urlencoded convention). Malformed escapes are copied
// through verbatim.
D_NODISCARD inline std::string
percent_decode(
    const std::string& _input,
    bool               _plus_as_space = false
)
{
    std::string result;
    result.reserve(_input.size());

    std::size_t i = 0;

    // walk the input, decoding %XX escapes
    while (i < _input.size())
    {
        const char c = _input[i];

        if ( (c == '%') && ((i + 2) < _input.size()) )
        {
            const int hi = ::d_net_hex_value(_input[i + 1]);
            const int lo = ::d_net_hex_value(_input[i + 2]);

            if ( (hi >= 0) && (lo >= 0) )
            {
                result.push_back(static_cast<char>((hi << 4) | lo));
                i += 3;

                continue;
            }
        }

        if (_plus_as_space && (c == '+'))
        {
            result.push_back(' ');
        }
        else
        {
            result.push_back(c);
        }

        ++i;
    }

    return result;
}

// 4.2.4
// query_param
//   type: a single URL query parameter as a (key, value) pair (values as yet
// undecoded/decoded per the caller).
typedef std::pair<std::string, std::string> query_param;

// 4.2.5
// query_list
//   type: an ordered collection of query parameters.
typedef std::vector<query_param> query_list;

// 4.2.6
// encode_query
//   function: serializes query parameters into an encoded query string
// ("k1=v1&k2=v2"), percent-encoding each key and value. No leading '?'.
D_NODISCARD inline std::string
encode_query(
    const query_list& _params
)
{
    std::string result;

    // join encoded key=value pairs with '&'
    for (std::size_t i = 0; i < _params.size(); ++i)
    {
        if (i != 0)
        {
            result.push_back('&');
        }

        result += percent_encode(_params[i].first);
        result.push_back('=');
        result += percent_encode(_params[i].second);
    }

    return result;
}

// 4.2.7
// parse_query
//   function: splits an encoded query string (with or without a leading '?')
// into decoded key/value pairs. A parameter with no '=' yields an empty value;
// '+' is decoded as space (form convention).
D_NODISCARD inline query_list
parse_query(
    const std::string& _query
)
{
    query_list result;

    std::size_t start = 0;

    // skip an optional leading '?'
    if ( (!_query.empty()) && (_query[0] == '?') )
    {
        start = 1;
    }

    // split on '&', then each token on its first '='
    while (start <= _query.size())
    {
        std::size_t amp = _query.find('&',
                                      start);

        if (amp == std::string::npos)
        {
            amp = _query.size();
        }

        if (amp > start)
        {
            const std::string token = _query.substr(start,
                                                    amp - start);
            const std::size_t eq    = token.find('=');

            if (eq == std::string::npos)
            {
                result.push_back(
                    query_param(percent_decode(token,
                                               true),
                                std::string()));
            }
            else
            {
                result.push_back(query_param(
                    percent_decode(token.substr(0,
                                                eq),
                                   true),
                    percent_decode(token.substr(eq + 1),
                                   true)));
            }
        }

        start = amp + 1;
    }

    return result;
}

// 4.3    URL parts
//------------------------------------------------------------------------------
// 4.3.1
// url_parts
//   struct: the components of an absolute URL with an authority, as
// split_url produces them. All fields are raw (still percent-encoded where
// the source was); an IP literal's host comes without its brackets, and
// `port` is 0 when the URL carried no explicit port.
struct url_parts
{
    std::string scheme;
    std::string userinfo;
    std::string host;
    port_type   port;
    std::string path;
    std::string query;
    std::string fragment;

    url_parts()
        : scheme(),
          userinfo(),
          host(),
          port(0),
          path(),
          query(),
          fragment()
    {
    }
};

NS_INTERNAL

    // 4.3.2
    // internal::url_text
    //   function: a parsed URL's view as a string; an absent part is empty.
    D_NODISCARD inline std::string
    url_text(
        ::d_pack_text _view
    )
    {
        return (_view.data) ? std::string(_view.data,
                                          _view.length)
                            : std::string();
    }

NS_END  // internal

// 4.3.3
// split_url
//   function: an absolute URL with an authority -- scheme "://" [userinfo
// "@"] host [":" port] path ["?" query] ["#" fragment] -- split into
// url_parts by net_url.h's RFC 3986 parser, so IP literals, zones,
// userinfo and ports read as the RFC has them. False, with `_out`
// unchanged, for a text that does not parse, a relative reference, or a
// URL without an authority.
D_NODISCARD inline bool
split_url(
    const std::string& _url,
    url_parts&         _out
)
{
    const ::d_pack_text text = { _url.data(),
                                 _url.size() };
    ::d_net_url         url  = ::d_net_url();

    // RFC 3986 decides; only an absolute URL with "//" is split
    if ( (::d_net_url_parse(text,
                            &url,
                            D_NULLPTR) != D_NET_URL_OK) ||
         (!::d_net_url_is_absolute(&url))               ||
         (!url.has_authority) )
    {
        return false;
    }

    _out.scheme   = internal::url_text(url.scheme);
    _out.userinfo = internal::url_text(url.userinfo);
    _out.host     = internal::url_text(url.host);
    _out.port     = url.port;
    _out.path     = internal::url_text(url.path);
    _out.query    = internal::url_text(url.query);
    _out.fragment = internal::url_text(url.fragment);

    return true;
}

NS_END  // net
NS_END  // djinterp


#endif  // DJINTERP_NET_NET_URL_HPP
