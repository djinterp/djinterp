/*******************************************************************************
* djinterp [net]                                                     net_url.hpp
*
* URLs in C++, over net/net_url.h.
*   An owning url holds its text and the C parse of it; every accessor returns
* a view into that text, so reading a component never allocates. Copies and
* moves rebase the views onto the copy's own text, so no url ever points into
* another. It supplies
*     - the error, host, and part vocabulary as scoped enumerations, each
*       enumerator defined as its C counterpart                            [1]
*     - url, and url_result for parses that may fail                       [2]
*     - parsing, resolution, normalization, percent-encoding, schemes, and
*       endpoints, each a thin call into the C foundation                  [3]
*   C++11 and later. Text the C foundation produces is written first into a
* buffer on the stack, and copied out once; only text longer than that buffer
* costs a second call, at the exact length the first one reported.
*
*
* path:      /inc/djinterp/net/net_url.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
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
*/

#ifndef DJINTERP_NET_NET_URL_HPP
#define DJINTERP_NET_NET_URL_HPP 1

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string
#include <utility>  // std::move
// djinterp
#include "../djinterp.hpp"                                // framework root
#include "../../re_std/string_view/string_view_typedefs.hpp"  // re_std::string_view
#include "./net.hpp"                                      // endpoint, io_error
#include "./net_url.h"                                    // the C URL API


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
//   enum: why a URL was refused, or why an output did not fit.
enum class url_error : unsigned char
{
    none       = D_NET_URL_OK,
    argument   = D_NET_URL_ERROR_ARGUMENT,
    scheme     = D_NET_URL_ERROR_SCHEME,
    userinfo   = D_NET_URL_ERROR_USERINFO,
    host       = D_NET_URL_ERROR_HOST,
    ip_literal = D_NET_URL_ERROR_IP_LITERAL,
    port       = D_NET_URL_ERROR_PORT,
    path       = D_NET_URL_ERROR_PATH,
    query      = D_NET_URL_ERROR_QUERY,
    fragment   = D_NET_URL_ERROR_FRAGMENT,
    percent    = D_NET_URL_ERROR_PERCENT,
    base       = D_NET_URL_ERROR_BASE,
    buffer     = D_NET_URL_ERROR_BUFFER
};

// 1.1.2
// url_host
//   enum: what kind of host a URL's authority names.
enum class url_host : unsigned char
{
    none       = D_NET_URL_HOST_NONE,
    name       = D_NET_URL_HOST_NAME,
    ipv4       = D_NET_URL_HOST_IPV4,
    ipv6       = D_NET_URL_HOST_IPV6,
    ipv_future = D_NET_URL_HOST_IPVFUTURE
};

// 1.1.3
// url_part
//   enum: the component a percent-encoding is for.
enum class url_part : unsigned char
{
    userinfo  = D_NET_URL_PART_USERINFO,
    host      = D_NET_URL_PART_HOST,
    path      = D_NET_URL_PART_PATH,
    segment   = D_NET_URL_PART_SEGMENT,
    query     = D_NET_URL_PART_QUERY,
    fragment  = D_NET_URL_PART_FRAGMENT,
    component = D_NET_URL_PART_COMPONENT
};

// 1.2    Conversions
//------------------------------------------------------------------------------
// to_c / from_c -- the same values under the other language's names; the
// enumerators are defined as each other, so these are casts
D_NODISCARD inline d_net_url_error
to_c(
    url_error _error
) noexcept
{
    return static_cast<d_net_url_error>(_error);
}

D_NODISCARD inline url_error
from_c(
    d_net_url_error _error
) noexcept
{
    return static_cast<url_error>(_error);
}

D_NODISCARD inline d_net_url_host
to_c(
    url_host _host
) noexcept
{
    return static_cast<d_net_url_host>(_host);
}

D_NODISCARD inline url_host
from_c(
    d_net_url_host _host
) noexcept
{
    return static_cast<url_host>(_host);
}

D_NODISCARD inline d_net_url_part
to_c(
    url_part _part
) noexcept
{
    return static_cast<d_net_url_part>(_part);
}

D_NODISCARD inline url_part
from_c(
    d_net_url_part _part
) noexcept
{
    return static_cast<url_part>(_part);
}

// url_error_name -- a short phrase for an error, for messages
D_NODISCARD inline const char*
url_error_name(
    url_error _error
) noexcept
{
    return ::d_net_url_error_name(to_c(_error));
}

NS_INTERNAL

    // url_pack
    //   function: a C view of a string's characters.
    D_NODISCARD inline ::d_pack_text
    url_pack(
        const std::string& _text
    ) noexcept
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
    ) noexcept
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
    ) noexcept
    {
        const ::d_pack_text moved = { (_view.data) ? _to + (_view.data - _from)
                                                   : nullptr,
                                      _view.length };

        return moved;
    }

    // url_produce
    //   function: runs one of the C text producers, whose signature is
    // (buffer, capacity, length) -> d_net_url_error, into a string: a stack
    // buffer first, then, if that was too small, the length it reported.
    template<typename Producer>
    D_NODISCARD url_error
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
        if (found != D_NET_URL_ERROR_BUFFER)
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
            _out = std::move(text);
        }

        return from_c(found);
    }

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
    using view_type = ::re_std::string_view;

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

    url(
        url&& _other
    ) noexcept
        : m_text(),
          m_parts(_other.m_parts)
    {
        const char* const from = _other.m_text.data();

        m_text = std::move(_other.m_text);
        m_rebase(from);
        _other.m_clear();
    }

    url&
    operator=(
        const url& _other
    )
    {
        // copied first, so a failed allocation leaves this url as it was
        url copy(_other);

        *this = std::move(copy);

        return *this;
    }

    url&
    operator=(
        url&& _other
    ) noexcept
    {
        // assigning a url to itself leaves it unchanged
        if (this != &_other)
        {
            const char* const from = _other.m_text.data();

            m_text  = std::move(_other.m_text);
            m_parts = _other.m_parts;
            m_rebase(from);
            _other.m_clear();
        }

        return *this;
    }

    ~url() = default;

    // the text, whole and by component; absent components are empty views,
    // told from empty present ones by the has_ accessors
    D_NODISCARD const std::string&
    str() const noexcept
    {
        return m_text;
    }

    D_NODISCARD view_type
    scheme() const noexcept
    {
        return internal::url_view(m_parts.scheme);
    }

    D_NODISCARD view_type
    userinfo() const noexcept
    {
        return internal::url_view(m_parts.userinfo);
    }

    D_NODISCARD view_type
    host() const noexcept
    {
        return internal::url_view(m_parts.host);
    }

    D_NODISCARD view_type
    port_text() const noexcept
    {
        return internal::url_view(m_parts.port_text);
    }

    D_NODISCARD view_type
    path() const noexcept
    {
        return internal::url_view(m_parts.path);
    }

    D_NODISCARD view_type
    query() const noexcept
    {
        return internal::url_view(m_parts.query);
    }

    D_NODISCARD view_type
    fragment() const noexcept
    {
        return internal::url_view(m_parts.fragment);
    }

    // presence, kind, and port
    D_NODISCARD bool
    has_authority() const noexcept
    {
        return m_parts.has_authority;
    }

    D_NODISCARD bool
    has_userinfo() const noexcept
    {
        return m_parts.has_userinfo;
    }

    D_NODISCARD bool
    has_port() const noexcept
    {
        return m_parts.has_port;
    }

    D_NODISCARD bool
    has_query() const noexcept
    {
        return m_parts.has_query;
    }

    D_NODISCARD bool
    has_fragment() const noexcept
    {
        return m_parts.has_fragment;
    }

    D_NODISCARD bool
    is_absolute() const noexcept
    {
        return ::d_net_url_is_absolute(&m_parts);
    }

    D_NODISCARD url_host
    host_kind() const noexcept
    {
        return from_c(m_parts.host_kind);
    }

    // port -- the explicit port, or 0; effective_port -- that, or else the
    // scheme's default; is_tls -- whether the scheme begins with TLS
    D_NODISCARD port_type
    port() const noexcept
    {
        return m_parts.port;
    }

    D_NODISCARD port_type
    effective_port() const noexcept
    {
        return ( (m_parts.has_port) &&
                 (m_parts.port_text.length > 0u) )
                   ? m_parts.port
                   : ::d_net_url_default_port(m_parts.scheme);
    }

    D_NODISCARD bool
    is_tls() const noexcept
    {
        return ::d_net_url_scheme_is_tls(m_parts.scheme);
    }

    // the C parse, for calls into the C foundation
    D_NODISCARD const ::d_net_url&
    c_url() const noexcept
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
    m_parse() noexcept
    {
        (void)::d_net_url_parse(internal::url_pack(m_text),
                                &m_parts,
                                nullptr);

        return;
    }

    // m_clear -- becomes the empty reference
    void
    m_clear() noexcept
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
    ) noexcept
    {
        const char* const to = m_text.data();

        // a view member of the C parse; the alias keeps "::d_pack_text" and
        // "::d_net_url" from reading as one qualified name
        using view_member = ::d_pack_text d_net_url::*;

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
        for (const view_member view : VIEWS)
        {
            m_parts.*view = internal::url_rebased(m_parts.*view,
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
    url         value;
    url_error   error;
    std::size_t position;

    url_result()
        : value(),
          error(url_error::none),
          position(0u)
    {}

    D_NODISCARD bool
    ok() const noexcept
    {
        return (error == url_error::none);
    }
};

// 2.2.2
// url_text_result
//   struct: text a call produced, or why it refused its input.
struct url_text_result
{
    std::string text;
    url_error   error;

    url_text_result()
        : text(),
          error(url_error::none)
    {}

    D_NODISCARD bool
    ok() const noexcept
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

    result.value.m_text = std::move(_text);

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

    result.error = internal::url_produce(
                       text,
                       [base, reference](char*        _buffer,
                                         std::size_t  _capacity,
                                         std::size_t* _length)
                       {
                           return ::d_net_url_resolve(base,
                                                      reference,
                                                      _buffer,
                                                      _capacity,
                                                      _length);
                       });

    // the target parses, since the resolver writes only valid references
    if (result.error == url_error::none)
    {
        result = url::parse(std::move(text));
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

    result.error = internal::url_produce(
                       text,
                       [parts](char*        _buffer,
                               std::size_t  _capacity,
                               std::size_t* _length)
                       {
                           return ::d_net_url_normalize(parts,
                                                        _buffer,
                                                        _capacity,
                                                        _length);
                       });

    // the normal form of a valid reference is valid
    if (result.error == url_error::none)
    {
        result = url::parse(std::move(text));
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
    url_part           _part
)
{
    const ::d_pack_text  raw  = internal::url_pack(_raw);
    const d_net_url_part part = to_c(_part);
    std::string          text;

    (void)internal::url_produce(text,
                                [raw, part](char*        _buffer,
                                            std::size_t  _capacity,
                                            std::size_t* _length)
                                {
                                    return ::d_net_url_encode(raw,
                                                              part,
                                                              _buffer,
                                                              _capacity,
                                                              _length);
                                });

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
                                         [text](char*        _buffer,
                                                std::size_t  _capacity,
                                                std::size_t* _length)
                                         {
                                             return ::d_net_url_decode(
                                                        text,
                                                        _buffer,
                                                        _capacity,
                                                        _length);
                                         });

    return result;
}

// 3.4    Schemes and endpoints
//------------------------------------------------------------------------------
// url_default_port, url_scheme_is_tls -- net_url.h's, for a scheme string
D_NODISCARD inline port_type
url_default_port(
    const std::string& _scheme
) noexcept
{
    return ::d_net_url_default_port(internal::url_pack(_scheme));
}

D_NODISCARD inline bool
url_scheme_is_tls(
    const std::string& _scheme
) noexcept
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
D_NODISCARD inline io_error
url_endpoint(
    const url& _url,
    endpoint&  _out
)
{
    ::d_net_endpoint record = ::d_net_endpoint();

    const io_error found = from_c(::d_net_url_endpoint(&_url.c_url(),
                                                       &record));

    // only a found endpoint is copied out
    if (found == io_error::none)
    {
        _out = from_c(record);
    }

    return found;
}


NS_END  // net
NS_END  // djinterp


#endif  // DJINTERP_NET_NET_URL_HPP
