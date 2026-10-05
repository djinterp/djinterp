/*******************************************************************************
* djinterp [net]                                                  ftp_client.hpp
*
* The C++ interface to the FTP and FTPS client.
*   A zero-cost view of ftp_client.h: `client` holds a d_ftp_client, and each
* member forwards to one C function, inline. Callbacks are templates, so any
* functor -- or, from C++11, any lambda -- serves as a sink or a source with
* no virtual call and no allocation.
*   The interface is tiered. C++98 is the floor: this header needs only the C
* headers, never djinterp.hpp, which requires C++11. From C++11 the
* enumerations are enum classes, copying is deleted rather than hidden, and
* the members are noexcept. The spelling is the same in both tiers:
* ftp::error::timeout names the same value either way.
*   Errors are values, never exceptions, so the interface works under
* -fno-exceptions. A callback must not throw: the C client beneath it cannot
* unwind, so the trampolines are declared not to throw, and a throw from a
* callback terminates the program.
*
*
* path:      /inc/djinterp/net/ftp/ftp_client.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  LANGUAGE TIER
    -------------
    1.  Qualifiers
         1.  D_FTP_NOEXCEPT
2.  VOCABULARY
    ----------
    1.  Errors
         1.  error
         2.  Conversions
    2.  Security
         1.  security
         2.  protection
         3.  Conversions
    3.  Data connections
         1.  data_mode
         2.  Conversions
3.  OPTIONS
    -------
    1.  Builder
         1.  options
4.  CLIENT
    ------
    1.  Callbacks
         1.  internal::string_sink
         2.  internal::string_source
         3.  internal::context_of
         4.  internal::sink_call
         5.  internal::source_call
    2.  Sessions
         1.  client
*/

#ifndef DJINTERP_NET_FTP_FTP_CLIENT_HPP
#define DJINTERP_NET_FTP_FTP_CLIENT_HPP 1

// std
#include <cstddef>   // std::size_t
#include <cstring>   // std::memcpy
#include <stdint.h>  // uint16_t, uint32_t
#include <string>    // std::string
// djinterp
#include "../../c/djinterp.h"  // framework root
#include "../net.h"            // d_net_error
#include "../ssl/ssl.h"        // d_ssl_context, d_ssl_status
#include "./ftp_client.h"      // the C client
#include "./ftp_command.h"     // d_ftp_command
#include "./ftp_common.h"      // d_ftp_error, d_ftp_error_string
#include "./ftp_listing.h"     // d_ftp_listing_format
#include "./ftp_options.h"     // d_ftp_options, d_ftp_data_mode
#include "./ftp_reply.h"       // d_ftp_reply
#include "./ftp_security.h"    // d_ftp_security, d_ftp_protection
#include "./ftp_transfer.h"    // d_ftp_data_type


//==============================================================================
// 1.  LANGUAGE TIER
//==============================================================================
// The one spelling that differs between the tiers.


// 1.1    Qualifiers
//------------------------------------------------------------------------------
// 1.1.1
// D_FTP_NOEXCEPT
//   qualifier: `noexcept` from C++11, and `throw()` below it, whose check
// table-driven unwinding makes free on the path that does not throw. Either
// way, an exception reaching it terminates the program instead of unwinding
// through the C client. Pre-definable.
#ifndef D_FTP_NOEXCEPT
    #if D_ENV_LANG_IS_CPP11_OR_HIGHER
        #define D_FTP_NOEXCEPT noexcept
    #else
        #define D_FTP_NOEXCEPT throw()
    #endif
#endif  // D_FTP_NOEXCEPT


namespace djinterp {
namespace net {
namespace ftp {


//==============================================================================
// 2.  VOCABULARY
//==============================================================================
// Each enumeration is an enum class from C++11 and, below it, a class
// scoping a plain enumeration, which converts to it for `switch` and
// comparison. Both are spelled type::enumerator, and each value is its C
// counterpart's.


// 2.1    Errors
//------------------------------------------------------------------------------
// 2.1.1
// error
//   enum: why an operation failed; `none` is success. The values are
// d_ftp_error's, whose documentation in ftp_common.h describes each.
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

enum class error : int
{
    none                 = D_FTP_OK,
    invalid_argument     = D_FTP_ERROR_INVALID_ARGUMENT,
    buffer_too_small     = D_FTP_ERROR_BUFFER_TOO_SMALL,
    malformed            = D_FTP_ERROR_MALFORMED,
    unsupported          = D_FTP_ERROR_UNSUPPORTED,
    out_of_memory        = D_FTP_ERROR_OUT_OF_MEMORY,
    resolve              = D_FTP_ERROR_RESOLVE,
    connect              = D_FTP_ERROR_CONNECT,
    timeout              = D_FTP_ERROR_TIMEOUT,
    connection_closed    = D_FTP_ERROR_CONNECTION_CLOSED,
    tls                  = D_FTP_ERROR_TLS,
    service_unavailable  = D_FTP_ERROR_SERVICE_UNAVAILABLE,
    data_connection      = D_FTP_ERROR_DATA_CONNECTION,
    transfer_aborted     = D_FTP_ERROR_TRANSFER_ABORTED,
    file_unavailable     = D_FTP_ERROR_FILE_UNAVAILABLE,
    local_error          = D_FTP_ERROR_LOCAL_ERROR,
    insufficient_storage = D_FTP_ERROR_INSUFFICIENT_STORAGE,
    command_unrecognized = D_FTP_ERROR_COMMAND_UNRECOGNIZED,
    syntax               = D_FTP_ERROR_SYNTAX,
    not_implemented      = D_FTP_ERROR_NOT_IMPLEMENTED,
    bad_sequence         = D_FTP_ERROR_BAD_SEQUENCE,
    protocol_unsupported = D_FTP_ERROR_PROTOCOL_UNSUPPORTED,
    login_denied         = D_FTP_ERROR_LOGIN_DENIED,
    account_required     = D_FTP_ERROR_ACCOUNT_REQUIRED,
    page_type_unknown    = D_FTP_ERROR_PAGE_TYPE_UNKNOWN,
    name_not_allowed     = D_FTP_ERROR_NAME_NOT_ALLOWED,
    security             = D_FTP_ERROR_SECURITY,
    rejected             = D_FTP_ERROR_REJECTED,
    unexpected_reply     = D_FTP_ERROR_UNEXPECTED_REPLY,
    unknown              = D_FTP_ERROR_UNKNOWN
};

#else

class error
{
public:
    enum value
    {
        none                 = D_FTP_OK,
        invalid_argument     = D_FTP_ERROR_INVALID_ARGUMENT,
        buffer_too_small     = D_FTP_ERROR_BUFFER_TOO_SMALL,
        malformed            = D_FTP_ERROR_MALFORMED,
        unsupported          = D_FTP_ERROR_UNSUPPORTED,
        out_of_memory        = D_FTP_ERROR_OUT_OF_MEMORY,
        resolve              = D_FTP_ERROR_RESOLVE,
        connect              = D_FTP_ERROR_CONNECT,
        timeout              = D_FTP_ERROR_TIMEOUT,
        connection_closed    = D_FTP_ERROR_CONNECTION_CLOSED,
        tls                  = D_FTP_ERROR_TLS,
        service_unavailable  = D_FTP_ERROR_SERVICE_UNAVAILABLE,
        data_connection      = D_FTP_ERROR_DATA_CONNECTION,
        transfer_aborted     = D_FTP_ERROR_TRANSFER_ABORTED,
        file_unavailable     = D_FTP_ERROR_FILE_UNAVAILABLE,
        local_error          = D_FTP_ERROR_LOCAL_ERROR,
        insufficient_storage = D_FTP_ERROR_INSUFFICIENT_STORAGE,
        command_unrecognized = D_FTP_ERROR_COMMAND_UNRECOGNIZED,
        syntax               = D_FTP_ERROR_SYNTAX,
        not_implemented      = D_FTP_ERROR_NOT_IMPLEMENTED,
        bad_sequence         = D_FTP_ERROR_BAD_SEQUENCE,
        protocol_unsupported = D_FTP_ERROR_PROTOCOL_UNSUPPORTED,
        login_denied         = D_FTP_ERROR_LOGIN_DENIED,
        account_required     = D_FTP_ERROR_ACCOUNT_REQUIRED,
        page_type_unknown    = D_FTP_ERROR_PAGE_TYPE_UNKNOWN,
        name_not_allowed     = D_FTP_ERROR_NAME_NOT_ALLOWED,
        security             = D_FTP_ERROR_SECURITY,
        rejected             = D_FTP_ERROR_REJECTED,
        unexpected_reply     = D_FTP_ERROR_UNEXPECTED_REPLY,
        unknown              = D_FTP_ERROR_UNKNOWN
    };

    error()
        : m_value(none)
    {}

    error(
        value _value
    )
        : m_value(_value)
    {}

    explicit error(
        ::d_ftp_error _value
    )
        : m_value(static_cast<value>(_value))
    {}

    // value -- the enumerator, for switch and comparison
    operator value() const
    {
        return m_value;
    }

private:
    value m_value;
};

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// 2.1.2
// Conversions
//   to_c and from_c cross to and from d_ftp_error; error_string describes
// an error in a short static English phrase.
inline ::d_ftp_error
to_c(
    error _error
) D_FTP_NOEXCEPT
{
    return static_cast< ::d_ftp_error>(static_cast<int>(_error));
}

inline error
from_c(
    ::d_ftp_error _error
) D_FTP_NOEXCEPT
{
    return error(_error);
}

inline const char*
error_string(
    error _error
) D_FTP_NOEXCEPT
{
    return ::d_ftp_error_string(to_c(_error));
}

// 2.2    Security
//------------------------------------------------------------------------------
// 2.2.1
// security
//   enum: TLS on the control connection: none, explicit (AUTH TLS after the
// greeting), or implicit (from the first byte, port 990 by default).
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

enum class security : int
{
    none         = D_FTP_SECURITY_NONE,
    explicit_tls = D_FTP_SECURITY_EXPLICIT,
    implicit_tls = D_FTP_SECURITY_IMPLICIT
};

#else

class security
{
public:
    enum value
    {
        none         = D_FTP_SECURITY_NONE,
        explicit_tls = D_FTP_SECURITY_EXPLICIT,
        implicit_tls = D_FTP_SECURITY_IMPLICIT
    };

    security()
        : m_value(none)
    {}

    security(
        value _value
    )
        : m_value(_value)
    {}

    explicit security(
        ::d_ftp_security _value
    )
        : m_value(static_cast<value>(_value))
    {}

    // value -- the enumerator, for switch and comparison
    operator value() const
    {
        return m_value;
    }

private:
    value m_value;
};

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// 2.2.2
// protection
//   enum: the PROT level asked for data connections under TLS. Only clear
// and private_ mean anything to TLS (RFC 4217 section 9); `private` being a
// keyword, the last takes a trailing underscore.
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

enum class protection : int
{
    clear        = D_FTP_PROTECTION_CLEAR,
    safe         = D_FTP_PROTECTION_SAFE,
    confidential = D_FTP_PROTECTION_CONFIDENTIAL,
    private_     = D_FTP_PROTECTION_PRIVATE
};

#else

class protection
{
public:
    enum value
    {
        clear        = D_FTP_PROTECTION_CLEAR,
        safe         = D_FTP_PROTECTION_SAFE,
        confidential = D_FTP_PROTECTION_CONFIDENTIAL,
        private_     = D_FTP_PROTECTION_PRIVATE
    };

    protection()
        : m_value(clear)
    {}

    protection(
        value _value
    )
        : m_value(_value)
    {}

    explicit protection(
        ::d_ftp_protection _value
    )
        : m_value(static_cast<value>(_value))
    {}

    // value -- the enumerator, for switch and comparison
    operator value() const
    {
        return m_value;
    }

private:
    value m_value;
};

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// 2.2.3
// Conversions
//   to_c and from_c for security and protection.
inline ::d_ftp_security
to_c(
    security _security
) D_FTP_NOEXCEPT
{
    return static_cast< ::d_ftp_security>(static_cast<int>(_security));
}

inline security
from_c(
    ::d_ftp_security _security
) D_FTP_NOEXCEPT
{
    return security(_security);
}

inline ::d_ftp_protection
to_c(
    protection _protection
) D_FTP_NOEXCEPT
{
    return static_cast< ::d_ftp_protection>(static_cast<int>(_protection));
}

inline protection
from_c(
    ::d_ftp_protection _protection
) D_FTP_NOEXCEPT
{
    return protection(_protection);
}

// 2.3    Data connections
//------------------------------------------------------------------------------
// 2.3.1
// data_mode
//   enum: how data connections are made: passive, the client connecting, or
// active, the server connecting back; extended (EPSV, EPRT) or classic
// (PASV, PORT); and the automatic modes, which try extended first.
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

enum class data_mode : int
{
    passive_auto     = D_FTP_DATA_PASSIVE_AUTO,
    passive          = D_FTP_DATA_PASSIVE,
    extended_passive = D_FTP_DATA_EXTENDED_PASSIVE,
    active_auto      = D_FTP_DATA_ACTIVE_AUTO,
    active           = D_FTP_DATA_ACTIVE,
    extended_active  = D_FTP_DATA_EXTENDED_ACTIVE
};

#else

class data_mode
{
public:
    enum value
    {
        passive_auto     = D_FTP_DATA_PASSIVE_AUTO,
        passive          = D_FTP_DATA_PASSIVE,
        extended_passive = D_FTP_DATA_EXTENDED_PASSIVE,
        active_auto      = D_FTP_DATA_ACTIVE_AUTO,
        active           = D_FTP_DATA_ACTIVE,
        extended_active  = D_FTP_DATA_EXTENDED_ACTIVE
    };

    data_mode()
        : m_value(passive_auto)
    {}

    data_mode(
        value _value
    )
        : m_value(_value)
    {}

    explicit data_mode(
        ::d_ftp_data_mode _value
    )
        : m_value(static_cast<value>(_value))
    {}

    // value -- the enumerator, for switch and comparison
    operator value() const
    {
        return m_value;
    }

private:
    value m_value;
};

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// 2.3.2
// Conversions
//   to_c and from_c for data_mode.
inline ::d_ftp_data_mode
to_c(
    data_mode _mode
) D_FTP_NOEXCEPT
{
    return static_cast< ::d_ftp_data_mode>(static_cast<int>(_mode));
}

inline data_mode
from_c(
    ::d_ftp_data_mode _mode
) D_FTP_NOEXCEPT
{
    return data_mode(_mode);
}


//==============================================================================
// 3.  OPTIONS
//==============================================================================


// 3.1    Builder
//------------------------------------------------------------------------------
// 3.1.1
// options
//   class: a client's settings, built fluently over d_ftp_options, whose
// defaults it starts from: anonymous, plain, passive, binary, peer and host
// verified. The strings are borrowed, not copied: each must outlive every
// client made from these options.
class options
{
public:
    options() D_FTP_NOEXCEPT
    {
        ::d_ftp_options_init(&m_options);
    }

    explicit options(
        const ::d_ftp_options& _options
    ) D_FTP_NOEXCEPT
        : m_options(_options)
    {}

    // user -- the login name; NULL logs in as "anonymous"
    options&
    user(
        const char* _user
    ) D_FTP_NOEXCEPT
    {
        m_options.user = _user;

        return *this;
    }

    // password -- the password; NULL sends "anonymous@"
    options&
    password(
        const char* _password
    ) D_FTP_NOEXCEPT
    {
        m_options.password = _password;

        return *this;
    }

    // account -- the ACCT a server may ask for with 332
    options&
    account(
        const char* _account
    ) D_FTP_NOEXCEPT
    {
        m_options.account = _account;

        return *this;
    }

    // security -- TLS on the control connection
    options&
    security(
        ftp::security _security
    ) D_FTP_NOEXCEPT
    {
        m_options.security = to_c(_security);

        return *this;
    }

    // require_security -- no fallback to plaintext, ever
    options&
    require_security(
        bool _require
    ) D_FTP_NOEXCEPT
    {
        m_options.require_security = _require;

        return *this;
    }

    // protection -- the PROT level asked for data under TLS
    options&
    protection(
        ftp::protection _protection
    ) D_FTP_NOEXCEPT
    {
        m_options.data_protection = to_c(_protection);

        return *this;
    }

    // clear_control -- CCC after the login, for NAT-bound servers
    options&
    clear_control(
        bool _clear
    ) D_FTP_NOEXCEPT
    {
        m_options.clear_control = _clear;

        return *this;
    }

    // data_mode -- how data connections are made
    options&
    data_mode(
        ftp::data_mode _mode
    ) D_FTP_NOEXCEPT
    {
        m_options.data_mode = to_c(_mode);

        return *this;
    }

    // binary -- TYPE I: bytes as they are
    options&
    binary() D_FTP_NOEXCEPT
    {
        m_options.type.data_type = D_FTP_TYPE_IMAGE;

        return *this;
    }

    // ascii -- TYPE A: line ends converted to and from CR LF
    options&
    ascii() D_FTP_NOEXCEPT
    {
        m_options.type.data_type = D_FTP_TYPE_ASCII;

        return *this;
    }

    // ignore_pasv_address -- connect PASV data to the control peer
    options&
    ignore_pasv_address(
        bool _ignore
    ) D_FTP_NOEXCEPT
    {
        m_options.ignore_pasv_address = _ignore;

        return *this;
    }

    // verify_peer -- check the server's certificate
    options&
    verify_peer(
        bool _verify
    ) D_FTP_NOEXCEPT
    {
        m_options.verify_peer = _verify;

        return *this;
    }

    // verify_host -- check the certificate names the host
    options&
    verify_host(
        bool _verify
    ) D_FTP_NOEXCEPT
    {
        m_options.verify_host = _verify;

        return *this;
    }

    // use_utf8 -- OPTS UTF8 ON where the server offers it
    options&
    use_utf8(
        bool _use
    ) D_FTP_NOEXCEPT
    {
        m_options.use_utf8 = _use;

        return *this;
    }

    // connect_timeout -- milliseconds per connection attempt; 0: none
    options&
    connect_timeout(
        uint32_t _milliseconds
    ) D_FTP_NOEXCEPT
    {
        m_options.connect_timeout_ms = _milliseconds;

        return *this;
    }

    // response_timeout -- milliseconds to wait for a reply; 0: none
    options&
    response_timeout(
        uint32_t _milliseconds
    ) D_FTP_NOEXCEPT
    {
        m_options.response_timeout_ms = _milliseconds;

        return *this;
    }

    // idle_timeout -- milliseconds a transfer may stall; 0: none
    options&
    idle_timeout(
        uint32_t _milliseconds
    ) D_FTP_NOEXCEPT
    {
        m_options.idle_timeout_ms = _milliseconds;

        return *this;
    }

    // c_options -- the d_ftp_options beneath, for what has no member here
    ::d_ftp_options&
    c_options() D_FTP_NOEXCEPT
    {
        return m_options;
    }

    const ::d_ftp_options&
    c_options() const D_FTP_NOEXCEPT
    {
        return m_options;
    }

private:
    ::d_ftp_options m_options;
};


//==============================================================================
// 4.  CLIENT
//==============================================================================


// 4.1    Callbacks
//------------------------------------------------------------------------------
// A sink is called as `error sink(const char* data, std::size_t size)` for
// each piece of a download, and returns error::none to continue; a source
// as `error source(char* buffer, std::size_t capacity, std::size_t& size)`,
// setting `size` to the bytes it wrote and 0 at the end of the data. The
// trampolines below adapt either to the C client's callbacks. Each member
// taking one has a `const` overload too, so a callback written in place --
// a temporary functor, or an unnamed lambda -- binds as well as a named one.
namespace internal {

    // 4.1.1
    // internal::string_sink
    //   class: a sink that appends to a string.
    class string_sink
    {
    public:
        explicit string_sink(
            std::string& _out
        ) D_FTP_NOEXCEPT
            : m_out(&_out)
        {}

        // operator() -- appends the data
        error
        operator()(
            const char* _data,
            std::size_t _size
        )
        {
            m_out->append(_data,
                          _size);

            return error::none;
        }

    private:
        std::string* m_out;
    };

    // 4.1.2
    // internal::string_source
    //   class: a source that supplies a string's bytes, in order.
    class string_source
    {
    public:
        explicit string_source(
            const std::string& _data
        ) D_FTP_NOEXCEPT
            : m_data(&_data),
              m_offset(0u)
        {}

        // operator() -- supplies the next bytes, and 0 at the end
        error
        operator()(
            char*        _buffer,
            std::size_t  _capacity,
            std::size_t& _size
        ) D_FTP_NOEXCEPT
        {
            const std::size_t left = m_data->size() - m_offset;

            _size = (left < _capacity) ? left : _capacity;

            // a round with bytes left copies them
            if (_size > 0u)
            {
                std::memcpy(_buffer,
                            m_data->data() + m_offset,
                            _size);
            }

            m_offset += _size;

            return error::none;
        }

    private:
        const std::string* m_data;
        std::size_t        m_offset;
    };

    // 4.1.3
    // internal::context_of
    //   function: a callback's address as the C client's context, which is
    // never written through; const is restored by the trampoline's Sink.
    template<typename Callback>
    void*
    context_of(
        Callback& _callback
    ) D_FTP_NOEXCEPT
    {
        return const_cast<void*>(static_cast<const void*>(&_callback));
    }

    // 4.1.4
    // internal::sink_call
    //   function: the d_ftp_sink_fn for a Sink, whose address is the context;
    // Sink may be const-qualified.
    template<typename Sink>
    ::d_ftp_error
    sink_call(
        void*       _context,
        const void* _data,
        std::size_t _size
    ) D_FTP_NOEXCEPT
    {
        Sink& sink = *static_cast<Sink*>(_context);

        return to_c(sink(static_cast<const char*>(_data),
                         _size));
    }

    // 4.1.5
    // internal::source_call
    //   function: the d_ftp_source_fn for a Source, whose address is the
    // context; Source may be const-qualified.
    template<typename Source>
    ::d_ftp_error
    source_call(
        void*        _context,
        void*        _buffer,
        std::size_t  _capacity,
        std::size_t* _out_size
    ) D_FTP_NOEXCEPT
    {
        Source& source = *static_cast<Source*>(_context);

        return to_c(source(static_cast<char*>(_buffer),
                           _capacity,
                           *_out_size));
    }

}  // namespace internal

// 4.2    Sessions
//------------------------------------------------------------------------------
// 4.2.1
// client
//   class: an FTP or FTPS session: connect, login, then commands and
// transfers, then quit. Every member blocks, as the C client does, and
// reports its outcome as an error; the destructor closes whatever is open,
// sending nothing. The C client holds pointers into itself, so a client can
// be neither copied nor moved: hold it where it is made, or on the heap.
class client
{
public:
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    client(const client&)            = delete;
    client& operator=(const client&) = delete;
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    client() D_FTP_NOEXCEPT
    {
        ::d_ftp_client_init(&m_client,
                            NULL,
                            NULL);
    }

    // _tls: the TLS context for FTPS, borrowed for the client's lifetime;
    // NULL for plain FTP
    explicit client(
        const ftp::options&          _options,
        const ::d_ssl_context* const _tls = NULL
    ) D_FTP_NOEXCEPT
    {
        ::d_ftp_client_init(&m_client,
                            &_options.c_options(),
                            _tls);
    }

    ~client()
    {
        ::d_ftp_client_close(&m_client);
    }

    // tls_config -- fills a TLS configuration from the options
    static void
    tls_config(
        const ftp::options&  _options,
        ::d_ssl_config&      _config
    ) D_FTP_NOEXCEPT
    {
        ::d_ftp_client_tls_config(&_options.c_options(),
                                  &_config);
    }

    // connect -- connects and reads the greeting; FTPS secures the control
    // connection first; port 0 picks 21, or 990 for implicit TLS
    error
    connect(
        const char* _host,
        uint16_t    _port = 0u
    ) D_FTP_NOEXCEPT
    {
        return from_c(::d_ftp_client_connect(&m_client,
                                             _host,
                                             _port));
    }

    error
    connect(
        const std::string& _host,
        uint16_t           _port = 0u
    ) D_FTP_NOEXCEPT
    {
        return connect(_host.c_str(),
                       _port);
    }

    // login -- USER, PASS, and ACCT as asked; under TLS, PBSZ and PROT
    error
    login() D_FTP_NOEXCEPT
    {
        return from_c(::d_ftp_client_login(&m_client));
    }

    // command -- one command that neither changes session state nor opens a
    // data connection: CWD, MKD, DELE, SIZE, MDTM, NOOP, and the like
    error
    command(
        ::d_ftp_command _command,
        const char*     _argument = NULL
    ) D_FTP_NOEXCEPT
    {
        return from_c(::d_ftp_client_command(&m_client,
                                             _command,
                                             _argument));
    }

    // retrieve -- downloads a file into a sink
    template<typename Sink>
    error
    retrieve(
        const char* _path,
        Sink&       _sink
    ) D_FTP_NOEXCEPT
    {
        return from_c(::d_ftp_client_retrieve(&m_client,
                                              _path,
                                              &internal::sink_call<Sink>,
                                              internal::context_of(_sink)));
    }

    template<typename Sink>
    error
    retrieve(
        const char* _path,
        const Sink& _sink
    ) D_FTP_NOEXCEPT
    {
        return from_c(::d_ftp_client_retrieve(
                          &m_client,
                          _path,
                          &internal::sink_call<const Sink>,
                          internal::context_of(_sink)));
    }

    // retrieve -- downloads a file onto the end of a string
    error
    retrieve(
        const char*  _path,
        std::string& _out
    ) D_FTP_NOEXCEPT
    {
        internal::string_sink sink(_out);

        return retrieve(_path,
                        sink);
    }

    // store -- uploads a file from a source
    template<typename Source>
    error
    store(
        const char* _path,
        Source&     _source
    ) D_FTP_NOEXCEPT
    {
        return from_c(::d_ftp_client_store(&m_client,
                                           _path,
                                           &internal::source_call<Source>,
                                           internal::context_of(_source)));
    }

    template<typename Source>
    error
    store(
        const char*   _path,
        const Source& _source
    ) D_FTP_NOEXCEPT
    {
        return from_c(::d_ftp_client_store(
                          &m_client,
                          _path,
                          &internal::source_call<const Source>,
                          internal::context_of(_source)));
    }

    // store -- uploads a string's bytes as a file
    error
    store(
        const char*        _path,
        const std::string& _data
    ) D_FTP_NOEXCEPT
    {
        internal::string_source source(_data);

        return store(_path,
                     source);
    }

    // a string that is not const binds here: to the template above it would
    // bind more tightly, as a callback
    error
    store(
        const char*  _path,
        std::string& _data
    ) D_FTP_NOEXCEPT
    {
        return store(_path,
                     static_cast<const std::string&>(_data));
    }

    // list -- a directory listing into a sink: MLSD where FEAT offered it,
    // LIST otherwise, the format chosen reported through _out_format
    template<typename Sink>
    error
    list(
        const char*             _path,
        Sink&                   _sink,
        ::d_ftp_listing_format* _out_format = NULL
    ) D_FTP_NOEXCEPT
    {
        return from_c(::d_ftp_client_list(&m_client,
                                          _path,
                                          &internal::sink_call<Sink>,
                                          internal::context_of(_sink),
                                          _out_format));
    }

    template<typename Sink>
    error
    list(
        const char*             _path,
        const Sink&             _sink,
        ::d_ftp_listing_format* _out_format = NULL
    ) D_FTP_NOEXCEPT
    {
        return from_c(::d_ftp_client_list(&m_client,
                                          _path,
                                          &internal::sink_call<const Sink>,
                                          internal::context_of(_sink),
                                          _out_format));
    }

    // list -- a directory listing onto the end of a string
    error
    list(
        const char*             _path,
        std::string&            _out,
        ::d_ftp_listing_format* _out_format = NULL
    ) D_FTP_NOEXCEPT
    {
        internal::string_sink sink(_out);

        return list(_path,
                    sink,
                    _out_format);
    }

    // quit -- QUIT, then closes every connection in order
    error
    quit() D_FTP_NOEXCEPT
    {
        return from_c(::d_ftp_client_quit(&m_client));
    }

    // close -- closes every connection, sending nothing
    void
    close() D_FTP_NOEXCEPT
    {
        ::d_ftp_client_close(&m_client);

        return;
    }

    // state -- O(1) reads of the session
    bool
    connected() const D_FTP_NOEXCEPT
    {
        return m_client.connected;
    }

    bool
    logged_in() const D_FTP_NOEXCEPT
    {
        return m_client.logged_in;
    }

    unsigned
    reply_code() const D_FTP_NOEXCEPT
    {
        return m_client.reply.code;
    }

    std::string
    reply_text() const
    {
        return std::string(m_client.reply.text.data,
                           m_client.reply.text.length);
    }

    const ::d_ftp_reply&
    reply() const D_FTP_NOEXCEPT
    {
        return m_client.reply;
    }

    uint32_t
    features() const D_FTP_NOEXCEPT
    {
        return m_client.features;
    }

    ftp::protection
    protection() const D_FTP_NOEXCEPT
    {
        return from_c(m_client.protection);
    }

    bool
    control_secured() const D_FTP_NOEXCEPT
    {
        return m_client.control.secured;
    }

    ::d_ssl_status
    tls_status() const D_FTP_NOEXCEPT
    {
        return m_client.tls_status;
    }

    ::d_net_error
    net_error() const D_FTP_NOEXCEPT
    {
        return m_client.net_error;
    }

    // c_client -- the d_ftp_client beneath, for what has no member here
    ::d_ftp_client&
    c_client() D_FTP_NOEXCEPT
    {
        return m_client;
    }

    const ::d_ftp_client&
    c_client() const D_FTP_NOEXCEPT
    {
        return m_client;
    }

private:
#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    client(const client&);
    client& operator=(const client&);
#endif  // !D_ENV_LANG_IS_CPP11_OR_HIGHER

    ::d_ftp_client m_client;
};


}  // namespace ftp
}  // namespace net
}  // namespace djinterp


#endif  // DJINTERP_NET_FTP_FTP_CLIENT_HPP
