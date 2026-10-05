/*******************************************************************************
* djinterp [net]                                                smtp_transport.c
*
* djinterp SMTP transport -- implementation.
*   The transport helpers, the line reader, and the built-in transport: BSD
* sockets with an optional OpenSSL layer. Everything platform-specific is
* selected at function level from the D_ENV_SMTP_* flags, so a build without
* sockets or without TLS still compiles every function, and the missing ones
* report D_SMTP_ERROR_UNSUPPORTED.
*   TLS runs over a custom BIO rather than SSL_set_fd, so that encrypted
* writes go through the same send() call -- and the same SIGPIPE suppression
* -- as plaintext, and so the socket's I/O timeouts bound the handshake as
* well as the session.
*
*
* path:      /src/djinterp/net/smtp/smtp_transport.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.29
*******************************************************************************/
// enable POSIX.1-2008 (getaddrinfo, poll, gethostname, inet_ntop) under a
// strict C compiler; a feature-test macro must precede every include
#if !defined(_WIN32) && !defined(_WIN64)
    #ifndef _POSIX_C_SOURCE
        #define _POSIX_C_SOURCE 200809L
    #endif  // _POSIX_C_SOURCE
#endif

#include "../../../../inc/djinterp/net/smtp/smtp_transport.h"
// std
#include <errno.h>    // errno, EAGAIN, EINTR, EINPROGRESS, ETIMEDOUT
#include <limits.h>   // INT_MAX
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdio.h>    // snprintf
#include <string.h>   // strchr, strlen
// djinterp
#include "../../../../inc/djinterp/net/smtp/smtp_common.h"       // errors
#include "../../../../inc/djinterp/env/net/smtp/env_smtp.h"      // D_ENV_SMTP_*

#if D_ENV_SMTP_HAS_SOCKETS
    // posix
    #include <arpa/inet.h>   // inet_ntop, inet_pton, ntohs
    #include <fcntl.h>       // fcntl, O_NONBLOCK, FD_CLOEXEC
    #include <netdb.h>       // getaddrinfo, freeaddrinfo
    #include <netinet/in.h>  // sockaddr_in, sockaddr_in6
    #include <poll.h>        // poll
    #include <sys/socket.h>  // socket, connect, send, recv, setsockopt
    #include <sys/time.h>    // struct timeval
    #include <unistd.h>      // close, gethostname
#endif

#if D_ENV_SMTP_HAS_TLS
    // openssl
    #include <openssl/bio.h>     // BIO_meth_*, BIO_set_data
    #include <openssl/err.h>     // ERR_clear_error
    #include <openssl/ssl.h>     // SSL_*, SSL_CTX_*
    #include <openssl/x509v3.h>  // X509_VERIFY_PARAM_set1_ip_asc
#endif


// D_INTERNAL_SMTP_SEND_FLAGS
//   macro: flags for every send(): MSG_NOSIGNAL where the platform has it, so
// a write to a closed peer fails with EPIPE instead of raising SIGPIPE.
#if D_ENV_SMTP_HAS_MSG_NOSIGNAL
    #define D_INTERNAL_SMTP_SEND_FLAGS MSG_NOSIGNAL
#else
    #define D_INTERNAL_SMTP_SEND_FLAGS 0
#endif

// D_INTERNAL_SMTP_BACKLOG
//   macro: pending-connection queue length for listeners.
#define D_INTERNAL_SMTP_BACKLOG 64

void
d_smtp_tls_options_init(
    struct d_smtp_tls_options* _options
)
{
    // parameter validation first
    if (!_options)
    {
        return;
    }

    _options->server_name      = NULL;
    _options->ca_file          = NULL;
    _options->certificate_file = NULL;
    _options->private_key_file = NULL;
    _options->verify_peer      = true;

    return;
}

/*
d_smtp_transport_write_all
  Loops over short writes. A write that reports success but accepts nothing
would loop forever, so it is treated as a transport failure.
*/
enum d_smtp_error
d_smtp_transport_write_all(
    const struct d_smtp_transport* _transport,
    const void*                    _data,
    size_t                         _length
)
{
    // parameter validation first
    if ( (!_transport)        ||
         (!_transport->write) ||
         ( (!_data) &&
           (_length > 0) ) )
    {
        return D_SMTP_ERROR_INVALID;
    }

    const char* cursor    = _data;
    size_t      remaining = _length;

    while (remaining > 0)
    {
        size_t                  sent   = 0;
        const enum d_smtp_error result = _transport->write(_transport->context,
                                                           cursor,
                                                           remaining,
                                                           &sent);

        if (result != D_SMTP_OK)
        {
            return result;
        }

        // no progress, or more than was asked: the transport is broken
        if ( (sent == 0) ||
             (sent > remaining) )
        {
            return D_SMTP_ERROR_IO;
        }

        cursor    += sent;
        remaining -= sent;
    }

    return D_SMTP_OK;
}

void
d_smtp_reader_init(
    struct d_smtp_reader*          _reader,
    const struct d_smtp_transport* _transport
)
{
    // parameter validation first
    if ( (!_reader) ||
         (!_transport) )
    {
        return;
    }

    _reader->transport = *_transport;
    _reader->start     = 0;
    _reader->end       = 0;
    _reader->crlf      = false;

    return;
}

// d_internal_smtp_line
//   struct: a line being assembled by the reader.
struct d_internal_smtp_line
{
    char*  data;
    size_t capacity;
    size_t used;
    bool   overflow;
};

/*
d_internal_smtp_keep
  Stores one byte of a line, keeping room for the NUL; once full, further
bytes are counted as overflow and dropped.
*/
static void
d_internal_smtp_keep(
    struct d_internal_smtp_line* _line,
    char                         _character
)
{
    if (_line->used + 1 < _line->capacity)
    {
        _line->data[_line->used] = _character;
        _line->used             += 1;

        return;
    }

    _line->overflow = true;

    return;
}

/*
d_smtp_reader_line
  A CR is held back until the next byte shows whether it begins a CRLF; this
keeps a line that exactly fills the buffer from being reported as overflowing
because of its own terminator.
*/
enum d_smtp_error
d_smtp_reader_line(
    struct d_smtp_reader* _reader,
    char*                 _line,
    size_t                _capacity,
    size_t*               _length
)
{
    // parameter validation first
    if ( (!_reader)                 ||
         (!_reader->transport.read) ||
         (!_line)                   ||
         (_capacity == 0)           ||
         (!_length) )
    {
        return D_SMTP_ERROR_INVALID;
    }

    struct d_internal_smtp_line line = { .data     = _line,
                                         .capacity = _capacity,
                                         .used     = 0,
                                         .overflow = false };
    bool pending_cr = false;

    for (;;)
    {
        // consume buffered bytes up to a line feed
        while (_reader->start < _reader->end)
        {
            const char character = _reader->buffer[_reader->start];

            _reader->start += 1;

            if (character == '\n')
            {
                _line[line.used] = '\0';
                *_length         = line.used;
                _reader->crlf    = pending_cr;

                return (line.overflow) ? D_SMTP_ERROR_TOO_LONG : D_SMTP_OK;
            }

            // a held-back CR that did not precede a LF is part of the line
            if (pending_cr)
            {
                d_internal_smtp_keep(&line, '\r');
            }

            pending_cr = (character == '\r');

            if (!pending_cr)
            {
                d_internal_smtp_keep(&line, character);
            }
        }

        size_t                  received = 0;
        const enum d_smtp_error result   = _reader->transport.read(
                                               _reader->transport.context,
                                               _reader->buffer,
                                               sizeof(_reader->buffer),
                                               &received);

        if (result != D_SMTP_OK)
        {
            _line[line.used] = '\0';
            *_length         = line.used;

            return result;
        }

        // success without data, or more than was asked, breaks the contract
        if ( (received == 0) ||
             (received > sizeof(_reader->buffer)) )
        {
            return D_SMTP_ERROR_IO;
        }

        _reader->start = 0;
        _reader->end   = received;
    }
}

size_t
d_smtp_reader_pending(
    const struct d_smtp_reader* _reader
)
{
    return (_reader) ? (_reader->end - _reader->start) : 0;
}

void
d_smtp_reader_discard(
    struct d_smtp_reader* _reader
)
{
    // parameter validation first
    if (!_reader)
    {
        return;
    }

    _reader->start = 0;
    _reader->end   = 0;

    return;
}

void
d_smtp_socket_init(
    struct d_smtp_socket* _socket
)
{
    // parameter validation first
    if (!_socket)
    {
        return;
    }

    _socket->descriptor  = -1;
    _socket->server      = false;
    _socket->tls         = NULL;
    _socket->tls_context = NULL;
    _socket->tls_method  = NULL;

    return;
}

bool
d_smtp_socket_is_secure(
    const struct d_smtp_socket* _socket
)
{
    return ( (_socket) &&
             (_socket->tls != NULL) );
}

void
d_smtp_listener_init(
    struct d_smtp_listener* _listener
)
{
    // parameter validation first
    if (!_listener)
    {
        return;
    }

    _listener->descriptor = -1;
    _listener->port       = 0;

    return;
}


#if D_ENV_SMTP_HAS_SOCKETS

/*
d_internal_smtp_errno
  Folds errno into the kernel's vocabulary. EAGAIN and EWOULDBLOCK reach here
only from a blocking socket whose SO_RCVTIMEO or SO_SNDTIMEO expired, so they
mean a timeout rather than "try again".
*/
static enum d_smtp_error
d_internal_smtp_errno(
    int _code
)
{
    if ( (_code == EAGAIN)      ||
         (_code == EWOULDBLOCK) ||
         (_code == ETIMEDOUT) )
    {
        return D_SMTP_ERROR_TIMEOUT;
    }

    if ( (_code == EPIPE) ||
         (_code == ECONNRESET) )
    {
        return D_SMTP_ERROR_CLOSED;
    }

    return D_SMTP_ERROR_IO;
}

/*
d_internal_smtp_clamp
  Milliseconds for poll(): negative waits forever, and values beyond int are
capped.
*/
static int
d_internal_smtp_clamp(
    long _milliseconds
)
{
    if (_milliseconds < 0)
    {
        return -1;
    }

    return (_milliseconds > (long)INT_MAX) ? INT_MAX : (int)_milliseconds;
}

/*
d_internal_smtp_wait
  poll() for one descriptor, retried across signals.
*/
static enum d_smtp_error
d_internal_smtp_wait(
    int   _descriptor,
    short _events,
    long  _timeout_ms
)
{
    struct pollfd waiter = { .fd      = _descriptor,
                             .events  = _events,
                             .revents = 0 };
    const int     limit  = d_internal_smtp_clamp(_timeout_ms);
    int           ready  = 0;

    do
    {
        ready = poll(&waiter,
                     1,
                     limit);
    } while ( (ready < 0) &&
              (errno == EINTR) );

    if (ready == 0)
    {
        return D_SMTP_ERROR_TIMEOUT;
    }

    return (ready > 0) ? D_SMTP_OK : D_SMTP_ERROR_IO;
}

#if D_ENV_SMTP_HAS_SO_NOSIGPIPE

/*
d_internal_smtp_nosigpipe
  Sets SO_NOSIGPIPE: this platform suppresses SIGPIPE per socket.
*/
static void
d_internal_smtp_nosigpipe(
    int _descriptor
)
{
    const int on = 1;

    (void)setsockopt(_descriptor,
                     SOL_SOCKET,
                     SO_NOSIGPIPE,
                     &on,
                     (socklen_t)sizeof(on));

    return;
}

#else

/*
d_internal_smtp_nosigpipe
  Nothing to set: SIGPIPE is suppressed per call (MSG_NOSIGNAL), or not at
all.
*/
static void
d_internal_smtp_nosigpipe(
    int _descriptor
)
{
    (void)_descriptor;

    return;
}

#endif  // D_ENV_SMTP_HAS_SO_NOSIGPIPE

/*
d_internal_smtp_prepare
  Options every connection socket carries: blocking mode (an accepted socket
inherits O_NONBLOCK from the listener on BSD systems), close-on-exec, SIGPIPE
suppression, and the send and receive timeouts that turn a stalled peer into
D_SMTP_ERROR_TIMEOUT instead of a hang.
*/
static enum d_smtp_error
d_internal_smtp_prepare(
    int  _descriptor,
    long _io_timeout_ms
)
{
    const int flags = fcntl(_descriptor,
                            F_GETFL,
                            0);

    if ( (flags < 0)                                  ||
         (fcntl(_descriptor,
                F_SETFL,
                flags & ~O_NONBLOCK) != 0)            ||
         (fcntl(_descriptor,
                F_SETFD,
                FD_CLOEXEC) != 0) )
    {
        return D_SMTP_ERROR_IO;
    }

    d_internal_smtp_nosigpipe(_descriptor);

    // no deadline: the socket blocks without limit
    if (_io_timeout_ms <= 0)
    {
        return D_SMTP_OK;
    }

    const struct timeval limit =
    {
        .tv_sec  = (time_t)(_io_timeout_ms / 1000),
        .tv_usec = (suseconds_t)((_io_timeout_ms % 1000) * 1000)
    };

    if ( (setsockopt(_descriptor,
                     SOL_SOCKET,
                     SO_RCVTIMEO,
                     &limit,
                     (socklen_t)sizeof(limit)) != 0) ||
         (setsockopt(_descriptor,
                     SOL_SOCKET,
                     SO_SNDTIMEO,
                     &limit,
                     (socklen_t)sizeof(limit)) != 0) )
    {
        return D_SMTP_ERROR_IO;
    }

    return D_SMTP_OK;
}

/*
d_internal_smtp_recv
  recv() retried across signals; 0 bytes is the peer closing.
*/
static enum d_smtp_error
d_internal_smtp_recv(
    int     _descriptor,
    void*   _buffer,
    size_t  _capacity,
    size_t* _received
)
{
    for (;;)
    {
        const ssize_t count = recv(_descriptor,
                                   _buffer,
                                   _capacity,
                                   0);

        if (count > 0)
        {
            *_received = (size_t)count;

            return D_SMTP_OK;
        }

        if (count == 0)
        {
            return D_SMTP_ERROR_CLOSED;
        }

        // a signal interrupted the wait; anything else is final
        if (errno != EINTR)
        {
            return d_internal_smtp_errno(errno);
        }
    }
}

/*
d_internal_smtp_send
  send() with the SIGPIPE-suppressing flags, retried across signals.
*/
static enum d_smtp_error
d_internal_smtp_send(
    int         _descriptor,
    const void* _data,
    size_t      _length,
    size_t*     _sent
)
{
    for (;;)
    {
        const ssize_t count = send(_descriptor,
                                   _data,
                                   _length,
                                   D_INTERNAL_SMTP_SEND_FLAGS);

        if (count >= 0)
        {
            *_sent = (size_t)count;

            return D_SMTP_OK;
        }

        // a signal interrupted the call; anything else is final
        if (errno != EINTR)
        {
            return d_internal_smtp_errno(errno);
        }
    }
}

#if D_ENV_SMTP_HAS_IPV6

/*
d_internal_smtp_address_of
  The address bytes inside a socket address, for inet_ntop(); IPv4 or IPv6.
*/
static const void*
d_internal_smtp_address_of(
    const struct sockaddr_storage* _address
)
{
    const void* raw = _address;

    if (_address->ss_family == AF_INET6)
    {
        const struct sockaddr_in6* six = raw;

        return &six->sin6_addr;
    }

    if (_address->ss_family == AF_INET)
    {
        const struct sockaddr_in* four = raw;

        return &four->sin_addr;
    }

    return NULL;
}

/*
d_internal_smtp_port_of
  The port of a socket address in host order; IPv4 or IPv6.
*/
static unsigned int
d_internal_smtp_port_of(
    const struct sockaddr_storage* _address
)
{
    const void* raw = _address;

    if (_address->ss_family == AF_INET6)
    {
        const struct sockaddr_in6* six = raw;

        return ntohs(six->sin6_port);
    }

    if (_address->ss_family == AF_INET)
    {
        const struct sockaddr_in* four = raw;

        return ntohs(four->sin_port);
    }

    return 0u;
}

#else

/*
d_internal_smtp_address_of
  The address bytes inside a socket address; IPv4 only on this platform.
*/
static const void*
d_internal_smtp_address_of(
    const struct sockaddr_storage* _address
)
{
    const void*               raw  = _address;
    const struct sockaddr_in* four = raw;

    return (_address->ss_family == AF_INET) ? (const void*)&four->sin_addr
                                            : NULL;
}

/*
d_internal_smtp_port_of
  The port of a socket address in host order; IPv4 only on this platform.
*/
static unsigned int
d_internal_smtp_port_of(
    const struct sockaddr_storage* _address
)
{
    const void*               raw  = _address;
    const struct sockaddr_in* four = raw;

    return (_address->ss_family == AF_INET) ? ntohs(four->sin_port) : 0u;
}

#endif  // D_ENV_SMTP_HAS_IPV6


#if D_ENV_SMTP_HAS_TLS

#if D_ENV_SMTP_HAS_IPV6

/*
d_internal_smtp_is_literal
  Whether a name is an IPv4 or IPv6 address rather than a host name.
*/
static bool
d_internal_smtp_is_literal(
    const char* _name
)
{
    unsigned char scratch[16] = { 0 };

    return ( (inet_pton(AF_INET, _name, scratch) == 1) ||
             (inet_pton(AF_INET6, _name, scratch) == 1) );
}

#else

/*
d_internal_smtp_is_literal
  Whether a name is an IPv4 address rather than a host name.
*/
static bool
d_internal_smtp_is_literal(
    const char* _name
)
{
    unsigned char scratch[4] = { 0 };

    return (inet_pton(AF_INET, _name, scratch) == 1);
}

#endif  // D_ENV_SMTP_HAS_IPV6

/*
d_internal_smtp_bio_write
  BIO write over the socket. An expired send timeout is reported to OpenSSL as
a retry, which surfaces as SSL_ERROR_WANT_WRITE; on a blocking socket that can
only mean the deadline passed.
*/
static int
d_internal_smtp_bio_write(
    BIO*        _bio,
    const char* _data,
    int         _length
)
{
    const struct d_smtp_socket* owner = BIO_get_data(_bio);
    size_t                      sent  = 0;

    BIO_clear_retry_flags(_bio);

    if ( (!owner) ||
         (_length <= 0) )
    {
        return 0;
    }

    const enum d_smtp_error result = d_internal_smtp_send(owner->descriptor,
                                                          _data,
                                                          (size_t)_length,
                                                          &sent);

    if (result == D_SMTP_OK)
    {
        return (int)sent;
    }

    if (result == D_SMTP_ERROR_TIMEOUT)
    {
        BIO_set_retry_write(_bio);
    }

    return -1;
}

/*
d_internal_smtp_bio_read
  BIO read over the socket: end of stream reads as 0, and an expired receive
timeout as a retry (SSL_ERROR_WANT_READ).
*/
static int
d_internal_smtp_bio_read(
    BIO*  _bio,
    char* _buffer,
    int   _length
)
{
    const struct d_smtp_socket* owner    = BIO_get_data(_bio);
    size_t                      received = 0;

    BIO_clear_retry_flags(_bio);

    if ( (!owner) ||
         (_length <= 0) )
    {
        return 0;
    }

    const enum d_smtp_error result = d_internal_smtp_recv(owner->descriptor,
                                                          _buffer,
                                                          (size_t)_length,
                                                          &received);

    if (result == D_SMTP_OK)
    {
        return (int)received;
    }

    if (result == D_SMTP_ERROR_CLOSED)
    {
        return 0;
    }

    if (result == D_SMTP_ERROR_TIMEOUT)
    {
        BIO_set_retry_read(_bio);
    }

    return -1;
}

/*
d_internal_smtp_bio_control
  Writes are unbuffered, so the flush OpenSSL issues after each handshake
flight always succeeds; every other control is unsupported.
*/
static long
d_internal_smtp_bio_control(
    BIO*  _bio,
    int   _command,
    long  _number,
    void* _pointer
)
{
    (void)_bio;
    (void)_number;
    (void)_pointer;

    return (_command == BIO_CTRL_FLUSH) ? 1 : 0;
}

/*
d_internal_smtp_bio_create
  A new BIO is ready as soon as its data (the socket) is attached.
*/
static int
d_internal_smtp_bio_create(
    BIO* _bio
)
{
    BIO_set_init(_bio, 1);

    return 1;
}

/*
d_internal_smtp_bio_method
  One BIO_METHOD per socket, released with it, so no process-wide state is
needed. The method uses the plain source/sink type rather than a value from
BIO_get_new_index(): that allocator is a small global counter a
per-connection method would exhaust, and nothing here looks BIOs up by type.
*/
static BIO_METHOD*
d_internal_smtp_bio_method(void)
{
    BIO_METHOD* method = BIO_meth_new(BIO_TYPE_SOURCE_SINK,
                                      "djinterp-smtp-socket");

    if (!method)
    {
        return NULL;
    }

    if ( (BIO_meth_set_write(method, d_internal_smtp_bio_write) != 1)  ||
         (BIO_meth_set_read(method, d_internal_smtp_bio_read) != 1)    ||
         (BIO_meth_set_ctrl(method, d_internal_smtp_bio_control) != 1) ||
         (BIO_meth_set_create(method, d_internal_smtp_bio_create) != 1) )
    {
        BIO_meth_free(method);

        return NULL;
    }

    return method;
}

/*
d_internal_smtp_tls_client
  Client verification: the caller's CA file, or the platform's trust store.
Turning verification off keeps encryption against passive observers only.
*/
static bool
d_internal_smtp_tls_client(
    SSL_CTX*                         _context,
    const struct d_smtp_tls_options* _options
)
{
    if (!_options->verify_peer)
    {
        SSL_CTX_set_verify(_context,
                           SSL_VERIFY_NONE,
                           NULL);

        return true;
    }

    SSL_CTX_set_verify(_context,
                       SSL_VERIFY_PEER,
                       NULL);

    const int loaded = (_options->ca_file)
                       ? SSL_CTX_load_verify_locations(_context,
                                                       _options->ca_file,
                                                       NULL)
                       : SSL_CTX_set_default_verify_paths(_context);

    return (loaded == 1);
}

/*
d_internal_smtp_tls_server
  Server identity: a PEM chain and its private key, checked to match.
*/
static bool
d_internal_smtp_tls_server(
    SSL_CTX*                         _context,
    const struct d_smtp_tls_options* _options
)
{
    return ( (SSL_CTX_use_certificate_chain_file(
                  _context,
                  _options->certificate_file) == 1) &&
             (SSL_CTX_use_PrivateKey_file(_context,
                                          _options->private_key_file,
                                          SSL_FILETYPE_PEM) == 1) &&
             (SSL_CTX_check_private_key(_context) == 1) );
}

/*
d_internal_smtp_tls_context
  A context for one connection, TLS 1.2 at minimum (RFC 8996 retires 1.0 and
1.1), configured for the socket's role.
*/
static SSL_CTX*
d_internal_smtp_tls_context(
    bool                             _server,
    const struct d_smtp_tls_options* _options
)
{
    SSL_CTX* context = SSL_CTX_new((_server) ? TLS_server_method()
                                             : TLS_client_method());

    if (!context)
    {
        return NULL;
    }

    const bool configured = ( (SSL_CTX_set_min_proto_version(
                                   context,
                                   TLS1_2_VERSION) == 1) &&
                              ( (_server)
                                ? d_internal_smtp_tls_server(context,
                                                             _options)
                                : d_internal_smtp_tls_client(context,
                                                             _options) ) );

    if (!configured)
    {
        SSL_CTX_free(context);

        return NULL;
    }

    return context;
}

/*
d_internal_smtp_tls_name
  SNI for host names (RFC 6066 forbids address literals there), and the name
or address the certificate must carry when verifying.
*/
static bool
d_internal_smtp_tls_name(
    SSL*                             _ssl,
    const struct d_smtp_tls_options* _options
)
{
    const char* name = _options->server_name;

    // without a name there is nothing to announce; verification needs one
    if (!name)
    {
        return !_options->verify_peer;
    }

    const bool literal = d_internal_smtp_is_literal(name);

    if ( (!literal) &&
         (SSL_set_tlsext_host_name(_ssl, name) != 1) )
    {
        return false;
    }

    if (!_options->verify_peer)
    {
        return true;
    }

    return (literal)
           ? (X509_VERIFY_PARAM_set1_ip_asc(SSL_get0_param(_ssl), name) == 1)
           : (SSL_set1_host(_ssl, name) == 1);
}

/*
d_internal_smtp_tls_error
  Maps an SSL_get_error() reason. A retry request can only come from an
expired timeout on these blocking sockets; SSL_ERROR_SYSCALL without errno is
an abrupt end of stream.
*/
static enum d_smtp_error
d_internal_smtp_tls_error(
    const SSL* _ssl,
    int        _status
)
{
    const int reason = SSL_get_error(_ssl, _status);

    if (reason == SSL_ERROR_ZERO_RETURN)
    {
        return D_SMTP_ERROR_CLOSED;
    }

    if ( (reason == SSL_ERROR_WANT_READ) ||
         (reason == SSL_ERROR_WANT_WRITE) )
    {
        return D_SMTP_ERROR_TIMEOUT;
    }

    if (reason == SSL_ERROR_SYSCALL)
    {
        return (errno != 0) ? d_internal_smtp_errno(errno)
                            : D_SMTP_ERROR_CLOSED;
    }

    return D_SMTP_ERROR_TLS;
}

/*
d_internal_smtp_tls_handshake
  SSL_accept() or SSL_connect(), with the error queue and errno cleared so
the outcome is not confused with an earlier failure.
*/
static enum d_smtp_error
d_internal_smtp_tls_handshake(
    SSL* _ssl,
    bool _server
)
{
    ERR_clear_error();
    errno = 0;

    const int status = (_server) ? SSL_accept(_ssl) : SSL_connect(_ssl);

    return (status == 1) ? D_SMTP_OK : d_internal_smtp_tls_error(_ssl, status);
}

/*
d_internal_smtp_tls_begin
  Builds the context, BIO method and SSL for one socket and runs the
handshake. On success the socket owns all three; on failure all are freed.
*/
static enum d_smtp_error
d_internal_smtp_tls_begin(
    struct d_smtp_socket*            _socket,
    const struct d_smtp_tls_options* _options
)
{
    SSL_CTX*          context = d_internal_smtp_tls_context(_socket->server,
                                                            _options);
    BIO_METHOD*       method  = d_internal_smtp_bio_method();
    SSL*              ssl     = (context) ? SSL_new(context) : NULL;
    BIO*              bio     = (method) ? BIO_new(method) : NULL;
    enum d_smtp_error result  = D_SMTP_ERROR_TLS;

    if ( (!ssl) ||
         (!bio) )
    {
        goto cleanup;
    }

    // the BIO carries the socket; the SSL takes ownership of the BIO
    BIO_set_data(bio, _socket);
    SSL_set_bio(ssl,
                bio,
                bio);
    bio = NULL;

    if ( (!_socket->server) &&
         (!d_internal_smtp_tls_name(ssl, _options)) )
    {
        goto cleanup;
    }

    result = d_internal_smtp_tls_handshake(ssl,
                                           _socket->server);

    if (result == D_SMTP_OK)
    {
        _socket->tls         = ssl;
        _socket->tls_context = context;
        _socket->tls_method  = method;

        return D_SMTP_OK;
    }

cleanup:
    BIO_free(bio);
    SSL_free(ssl);
    SSL_CTX_free(context);
    BIO_meth_free(method);

    return result;
}

/*
d_internal_smtp_tls_read
  SSL_read(), with the request clamped to int.
*/
static enum d_smtp_error
d_internal_smtp_tls_read(
    void*   _tls,
    void*   _buffer,
    size_t  _capacity,
    size_t* _received
)
{
    SSL*      ssl     = _tls;
    const int request = (_capacity > (size_t)INT_MAX) ? INT_MAX
                                                      : (int)_capacity;

    ERR_clear_error();
    errno = 0;

    const int count = SSL_read(ssl,
                               _buffer,
                               request);

    if (count > 0)
    {
        *_received = (size_t)count;

        return D_SMTP_OK;
    }

    return d_internal_smtp_tls_error(ssl,
                                     count);
}

/*
d_internal_smtp_tls_write
  SSL_write(), with the request clamped to int. Without partial-write mode
OpenSSL writes all of the request or fails.
*/
static enum d_smtp_error
d_internal_smtp_tls_write(
    void*       _tls,
    const void* _data,
    size_t      _length,
    size_t*     _sent
)
{
    SSL*      ssl     = _tls;
    const int request = (_length > (size_t)INT_MAX) ? INT_MAX
                                                    : (int)_length;

    ERR_clear_error();
    errno = 0;

    const int count = SSL_write(ssl,
                                _data,
                                request);

    if (count > 0)
    {
        *_sent = (size_t)count;

        return D_SMTP_OK;
    }

    return d_internal_smtp_tls_error(ssl,
                                     count);
}

/*
d_internal_smtp_tls_end
  One close_notify attempt, whose outcome does not matter at close, then the
TLS objects are released.
*/
static void
d_internal_smtp_tls_end(
    struct d_smtp_socket* _socket
)
{
    if (_socket->tls)
    {
        ERR_clear_error();
        (void)SSL_shutdown(_socket->tls);
        SSL_free(_socket->tls);
    }

    SSL_CTX_free(_socket->tls_context);
    BIO_meth_free(_socket->tls_method);

    _socket->tls         = NULL;
    _socket->tls_context = NULL;
    _socket->tls_method  = NULL;

    return;
}

#else

/*
d_internal_smtp_tls_begin
  TLS is not compiled in (D_ENV_SMTP_HAS_TLS is 0).
*/
static enum d_smtp_error
d_internal_smtp_tls_begin(
    struct d_smtp_socket*            _socket,
    const struct d_smtp_tls_options* _options
)
{
    (void)_socket;
    (void)_options;

    return D_SMTP_ERROR_UNSUPPORTED;
}

/*
d_internal_smtp_tls_read
  Unreachable without TLS: no socket ever has a TLS session.
*/
static enum d_smtp_error
d_internal_smtp_tls_read(
    void*   _tls,
    void*   _buffer,
    size_t  _capacity,
    size_t* _received
)
{
    (void)_tls;
    (void)_buffer;
    (void)_capacity;
    (void)_received;

    return D_SMTP_ERROR_UNSUPPORTED;
}

/*
d_internal_smtp_tls_write
  Unreachable without TLS: no socket ever has a TLS session.
*/
static enum d_smtp_error
d_internal_smtp_tls_write(
    void*       _tls,
    const void* _data,
    size_t      _length,
    size_t*     _sent
)
{
    (void)_tls;
    (void)_data;
    (void)_length;
    (void)_sent;

    return D_SMTP_ERROR_UNSUPPORTED;
}

/*
d_internal_smtp_tls_end
  Nothing to release without TLS.
*/
static void
d_internal_smtp_tls_end(
    struct d_smtp_socket* _socket
)
{
    (void)_socket;

    return;
}

#endif  // D_ENV_SMTP_HAS_TLS

/*
d_internal_smtp_socket_read
  Transport read: through TLS when active, else straight from the socket.
*/
static enum d_smtp_error
d_internal_smtp_socket_read(
    void*   _context,
    void*   _buffer,
    size_t  _capacity,
    size_t* _received
)
{
    const struct d_smtp_socket* owner = _context;

    if ( (!owner)                  ||
         (owner->descriptor < 0)   ||
         (!_buffer)                ||
         (_capacity == 0)          ||
         (!_received) )
    {
        return D_SMTP_ERROR_INVALID;
    }

    return (owner->tls) ? d_internal_smtp_tls_read(owner->tls,
                                                   _buffer,
                                                   _capacity,
                                                   _received)
                        : d_internal_smtp_recv(owner->descriptor,
                                               _buffer,
                                               _capacity,
                                               _received);
}

/*
d_internal_smtp_socket_write
  Transport write: through TLS when active, else straight to the socket.
*/
static enum d_smtp_error
d_internal_smtp_socket_write(
    void*       _context,
    const void* _data,
    size_t      _length,
    size_t*     _sent
)
{
    const struct d_smtp_socket* owner = _context;

    if ( (!owner)                ||
         (owner->descriptor < 0) ||
         (!_data)                ||
         (!_sent) )
    {
        return D_SMTP_ERROR_INVALID;
    }

    return (owner->tls) ? d_internal_smtp_tls_write(owner->tls,
                                                    _data,
                                                    _length,
                                                    _sent)
                        : d_internal_smtp_send(owner->descriptor,
                                               _data,
                                               _length,
                                               _sent);
}

/*
d_internal_smtp_socket_starttls
  Transport upgrade: the handshake in the socket's own role.
*/
static enum d_smtp_error
d_internal_smtp_socket_starttls(
    void*                            _context,
    const struct d_smtp_tls_options* _options
)
{
    return d_smtp_socket_start_tls(_context,
                                   _options);
}

/*
d_internal_smtp_socket_close
  Transport release: closes the socket.
*/
static void
d_internal_smtp_socket_close(
    void* _context
)
{
    d_smtp_socket_close(_context);

    return;
}

/*
d_internal_smtp_connect_wait
  Completes a non-blocking connect: wait for writability, then read the
outcome from SO_ERROR.
*/
static enum d_smtp_error
d_internal_smtp_connect_wait(
    int  _descriptor,
    long _timeout_ms
)
{
    const enum d_smtp_error waited  = d_internal_smtp_wait(_descriptor,
                                                           POLLOUT,
                                                           _timeout_ms);
    int                     failure = 0;
    socklen_t               length  = (socklen_t)sizeof(failure);

    if (waited != D_SMTP_OK)
    {
        return (waited == D_SMTP_ERROR_TIMEOUT) ? D_SMTP_ERROR_TIMEOUT
                                                : D_SMTP_ERROR_CONNECT;
    }

    if ( (getsockopt(_descriptor,
                     SOL_SOCKET,
                     SO_ERROR,
                     &failure,
                     &length) != 0) ||
         (failure != 0) )
    {
        return (failure == ETIMEDOUT) ? D_SMTP_ERROR_TIMEOUT
                                      : D_SMTP_ERROR_CONNECT;
    }

    return D_SMTP_OK;
}

/*
d_internal_smtp_connect_address
  One connection attempt, made non-blocking so poll() can bound it.
*/
static enum d_smtp_error
d_internal_smtp_connect_address(
    int                    _descriptor,
    const struct addrinfo* _address,
    long                   _timeout_ms
)
{
    const int flags = fcntl(_descriptor,
                            F_GETFL,
                            0);

    if ( (flags < 0) ||
         (fcntl(_descriptor,
                F_SETFL,
                flags | O_NONBLOCK) != 0) )
    {
        return D_SMTP_ERROR_CONNECT;
    }

    enum d_smtp_error result = D_SMTP_OK;

    if (connect(_descriptor,
                _address->ai_addr,
                _address->ai_addrlen) != 0)
    {
        result = (errno == EINPROGRESS)
                 ? d_internal_smtp_connect_wait(_descriptor,
                                                _timeout_ms)
                 : D_SMTP_ERROR_CONNECT;
    }

    return result;
}

enum d_smtp_error
d_smtp_socket_connect(
    struct d_smtp_socket* _socket,
    const char*           _host,
    unsigned int          _port,
    long                  _connect_timeout_ms,
    long                  _io_timeout_ms
)
{
    // parameter validation first
    if ( (!_socket)          ||
         (!_host)            ||
         (_host[0] == '\0')  ||
         (_port == 0)        ||
         (_port > 65535u) )
    {
        return D_SMTP_ERROR_INVALID;
    }

    if (_socket->descriptor >= 0)
    {
        return D_SMTP_ERROR_STATE;
    }

    char             service[16] = { 0 };
    struct addrinfo  hints       = { .ai_family   = AF_UNSPEC,
                                     .ai_socktype = SOCK_STREAM };
    struct addrinfo* results     = NULL;

    (void)snprintf(service,
                   sizeof(service),
                   "%u",
                   _port);

    if (getaddrinfo(_host,
                    service,
                    &hints,
                    &results) != 0)
    {
        return D_SMTP_ERROR_RESOLVE;
    }

    enum d_smtp_error result = D_SMTP_ERROR_CONNECT;

    // the first address that accepts wins; each failure closes its socket
    for (const struct addrinfo* entry = results;
         entry != NULL;
         entry = entry->ai_next)
    {
        const int descriptor = socket(entry->ai_family,
                                      entry->ai_socktype,
                                      entry->ai_protocol);

        if (descriptor < 0)
        {
            continue;
        }

        result = d_internal_smtp_connect_address(descriptor,
                                                 entry,
                                                 _connect_timeout_ms);

        if (result == D_SMTP_OK)
        {
            result = d_internal_smtp_prepare(descriptor,
                                             _io_timeout_ms);
        }

        if (result == D_SMTP_OK)
        {
            d_smtp_socket_init(_socket);
            _socket->descriptor = descriptor;

            break;
        }

        (void)close(descriptor);
    }

    freeaddrinfo(results);

    return result;
}

enum d_smtp_error
d_smtp_socket_start_tls(
    struct d_smtp_socket*            _socket,
    const struct d_smtp_tls_options* _options
)
{
    // parameter validation first
    if ( (!_socket)                  ||
         (!_options)                 ||
         (_socket->descriptor < 0) )
    {
        return D_SMTP_ERROR_INVALID;
    }

    if (_socket->tls)
    {
        return D_SMTP_ERROR_STATE;
    }

    // a server needs its identity; a verifying client needs a name to check
    if ( ( (_socket->server) &&
           ( (!_options->certificate_file) ||
             (!_options->private_key_file) ) ) ||
         ( (!_socket->server)        &&
           (_options->verify_peer)   &&
           (!_options->server_name) ) )
    {
        return D_SMTP_ERROR_INVALID;
    }

    return d_internal_smtp_tls_begin(_socket,
                                     _options);
}

#if D_ENV_SMTP_HAS_GETHOSTNAME

/*
d_internal_smtp_host_name
  The machine's name, if it is fully qualified -- RFC 5321 section 4.1.4 asks
for a fully qualified domain in EHLO, so a bare "localhost" or short name is
not offered.
*/
static enum d_smtp_error
d_internal_smtp_host_name(
    struct d_smtp_buffer* _buffer
)
{
    char name[D_SMTP_DOMAIN_MAX + 1u] = { 0 };

    if (gethostname(name,
                    sizeof(name) - 1u) != 0)
    {
        return D_SMTP_ERROR_UNSUPPORTED;
    }

    const size_t length = strlen(name);

    if ( (!strchr(name, '.')) ||
         (!d_smtp_domain_is_valid(name, length, false)) )
    {
        return D_SMTP_ERROR_UNSUPPORTED;
    }

    return d_smtp_buffer_append(_buffer,
                                name,
                                length);
}

#else

/*
d_internal_smtp_host_name
  No gethostname() on this platform; the caller falls back to an address
literal.
*/
static enum d_smtp_error
d_internal_smtp_host_name(
    struct d_smtp_buffer* _buffer
)
{
    (void)_buffer;

    return D_SMTP_ERROR_UNSUPPORTED;
}

#endif  // D_ENV_SMTP_HAS_GETHOSTNAME

/*
d_internal_smtp_local_literal
  "[a.b.c.d]" or "[IPv6:...]" for this end of the connection.
*/
static enum d_smtp_error
d_internal_smtp_local_literal(
    int                   _descriptor,
    struct d_smtp_buffer* _buffer
)
{
    struct sockaddr_storage local    = { .ss_family = AF_UNSPEC };
    socklen_t               length   = (socklen_t)sizeof(local);
    char                    text[64] = { 0 };

    if (getsockname(_descriptor,
                    (struct sockaddr*)&local,
                    &length) != 0)
    {
        return D_SMTP_ERROR_IO;
    }

    const void* address = d_internal_smtp_address_of(&local);

    if ( (!address) ||
         (!inet_ntop(local.ss_family,
                     address,
                     text,
                     (socklen_t)sizeof(text))) )
    {
        return D_SMTP_ERROR_IO;
    }

    const size_t saved  = _buffer->length;
    const char*  prefix = (local.ss_family == AF_INET) ? "[" : "[IPv6:";

    if ( (d_smtp_buffer_append_text(_buffer, prefix) != D_SMTP_OK) ||
         (d_smtp_buffer_append_text(_buffer, text) != D_SMTP_OK)   ||
         (d_smtp_buffer_append_text(_buffer, "]") != D_SMTP_OK) )
    {
        _buffer->length       = saved;
        _buffer->data[saved]  = '\0';

        return D_SMTP_ERROR_TOO_LONG;
    }

    return D_SMTP_OK;
}

enum d_smtp_error
d_smtp_socket_identity(
    const struct d_smtp_socket* _socket,
    struct d_smtp_buffer*       _buffer
)
{
    // parameter validation first
    if ( (!_socket)                ||
         (!_buffer)                ||
         (_socket->descriptor < 0) )
    {
        return D_SMTP_ERROR_INVALID;
    }

    // a fully qualified host name is preferred over an address literal
    if (d_internal_smtp_host_name(_buffer) == D_SMTP_OK)
    {
        return D_SMTP_OK;
    }

    return d_internal_smtp_local_literal(_socket->descriptor,
                                         _buffer);
}

void
d_smtp_socket_close(
    struct d_smtp_socket* _socket
)
{
    // parameter validation first
    if (!_socket)
    {
        return;
    }

    d_internal_smtp_tls_end(_socket);

    if (_socket->descriptor >= 0)
    {
        (void)close(_socket->descriptor);
    }

    d_smtp_socket_init(_socket);

    return;
}

/*
d_internal_smtp_bind
  One listening socket for one resolved address. It is non-blocking so that
a connection which vanishes between poll() and accept() cannot stall the
server; accepted sockets are made blocking again by d_internal_smtp_prepare.
*/
static int
d_internal_smtp_bind(
    const struct addrinfo* _address
)
{
    const int descriptor = socket(_address->ai_family,
                                  _address->ai_socktype,
                                  _address->ai_protocol);
    const int reuse      = 1;

    if (descriptor < 0)
    {
        return -1;
    }

    const int flags = fcntl(descriptor,
                            F_GETFL,
                            0);

    if ( (flags < 0)                                       ||
         (fcntl(descriptor,
                F_SETFL,
                flags | O_NONBLOCK) != 0)                  ||
         (fcntl(descriptor,
                F_SETFD,
                FD_CLOEXEC) != 0)                          ||
         (setsockopt(descriptor,
                     SOL_SOCKET,
                     SO_REUSEADDR,
                     &reuse,
                     (socklen_t)sizeof(reuse)) != 0)       ||
         (bind(descriptor,
               _address->ai_addr,
               _address->ai_addrlen) != 0)                 ||
         (listen(descriptor,
                 D_INTERNAL_SMTP_BACKLOG) != 0) )
    {
        (void)close(descriptor);

        return -1;
    }

    return descriptor;
}

enum d_smtp_error
d_smtp_listener_open(
    struct d_smtp_listener* _listener,
    const char*             _host,
    unsigned int            _port
)
{
    // parameter validation first
    if ( (!_listener) ||
         (_port > 65535u) )
    {
        return D_SMTP_ERROR_INVALID;
    }

    if (_listener->descriptor >= 0)
    {
        return D_SMTP_ERROR_STATE;
    }

    char             service[16] = { 0 };
    struct addrinfo  hints       = { .ai_family   = AF_UNSPEC,
                                     .ai_socktype = SOCK_STREAM,
                                     .ai_flags    = AI_PASSIVE };
    struct addrinfo* results     = NULL;
    int              descriptor  = -1;

    (void)snprintf(service,
                   sizeof(service),
                   "%u",
                   _port);

    if (getaddrinfo(_host,
                    service,
                    &hints,
                    &results) != 0)
    {
        return D_SMTP_ERROR_RESOLVE;
    }

    // the first address that binds wins
    for (const struct addrinfo* entry = results;
         ( (entry != NULL) &&
           (descriptor < 0) );
         entry = entry->ai_next)
    {
        descriptor = d_internal_smtp_bind(entry);
    }

    freeaddrinfo(results);

    if (descriptor < 0)
    {
        return D_SMTP_ERROR_IO;
    }

    struct sockaddr_storage bound  = { .ss_family = AF_UNSPEC };
    socklen_t               length = (socklen_t)sizeof(bound);

    _listener->descriptor = descriptor;
    _listener->port       = (getsockname(descriptor,
                                         (struct sockaddr*)&bound,
                                         &length) == 0)
                            ? d_internal_smtp_port_of(&bound)
                            : _port;

    return D_SMTP_OK;
}

enum d_smtp_error
d_smtp_listener_accept(
    struct d_smtp_listener* _listener,
    long                    _wait_ms,
    long                    _io_timeout_ms,
    struct d_smtp_socket*   _socket
)
{
    // parameter validation first
    if ( (!_listener) ||
         (!_socket) )
    {
        return D_SMTP_ERROR_INVALID;
    }

    if ( (_listener->descriptor < 0) ||
         (_socket->descriptor >= 0) )
    {
        return D_SMTP_ERROR_STATE;
    }

    const enum d_smtp_error waited = d_internal_smtp_wait(_listener->descriptor,
                                                          POLLIN,
                                                          _wait_ms);

    if (waited != D_SMTP_OK)
    {
        return waited;
    }

    int descriptor = -1;

    // retried across signals; EAGAIN means the peer left after poll()
    do
    {
        descriptor = accept(_listener->descriptor,
                            NULL,
                            NULL);
    } while ( (descriptor < 0) &&
              (errno == EINTR) );

    if (descriptor < 0)
    {
        return ( (errno == EAGAIN) ||
                 (errno == EWOULDBLOCK) ) ? D_SMTP_ERROR_TIMEOUT
                                          : D_SMTP_ERROR_IO;
    }

    const enum d_smtp_error prepared = d_internal_smtp_prepare(descriptor,
                                                               _io_timeout_ms);

    if (prepared != D_SMTP_OK)
    {
        (void)close(descriptor);

        return prepared;
    }

    d_smtp_socket_init(_socket);
    _socket->descriptor = descriptor;
    _socket->server     = true;

    return D_SMTP_OK;
}

void
d_smtp_listener_close(
    struct d_smtp_listener* _listener
)
{
    // parameter validation first
    if (!_listener)
    {
        return;
    }

    if (_listener->descriptor >= 0)
    {
        (void)close(_listener->descriptor);
    }

    d_smtp_listener_init(_listener);

    return;
}

#else

/*
d_internal_smtp_socket_read
  No socket transport on this platform.
*/
static enum d_smtp_error
d_internal_smtp_socket_read(
    void*   _context,
    void*   _buffer,
    size_t  _capacity,
    size_t* _received
)
{
    (void)_context;
    (void)_buffer;
    (void)_capacity;
    (void)_received;

    return D_SMTP_ERROR_UNSUPPORTED;
}

/*
d_internal_smtp_socket_write
  No socket transport on this platform.
*/
static enum d_smtp_error
d_internal_smtp_socket_write(
    void*       _context,
    const void* _data,
    size_t      _length,
    size_t*     _sent
)
{
    (void)_context;
    (void)_data;
    (void)_length;
    (void)_sent;

    return D_SMTP_ERROR_UNSUPPORTED;
}

/*
d_internal_smtp_socket_starttls
  No socket transport on this platform.
*/
static enum d_smtp_error
d_internal_smtp_socket_starttls(
    void*                            _context,
    const struct d_smtp_tls_options* _options
)
{
    (void)_context;
    (void)_options;

    return D_SMTP_ERROR_UNSUPPORTED;
}

/*
d_internal_smtp_socket_close
  No socket transport on this platform; the socket is only reset.
*/
static void
d_internal_smtp_socket_close(
    void* _context
)
{
    d_smtp_socket_init(_context);

    return;
}

enum d_smtp_error
d_smtp_socket_connect(
    struct d_smtp_socket* _socket,
    const char*           _host,
    unsigned int          _port,
    long                  _connect_timeout_ms,
    long                  _io_timeout_ms
)
{
    (void)_socket;
    (void)_host;
    (void)_port;
    (void)_connect_timeout_ms;
    (void)_io_timeout_ms;

    return D_SMTP_ERROR_UNSUPPORTED;
}

enum d_smtp_error
d_smtp_socket_start_tls(
    struct d_smtp_socket*            _socket,
    const struct d_smtp_tls_options* _options
)
{
    (void)_socket;
    (void)_options;

    return D_SMTP_ERROR_UNSUPPORTED;
}

enum d_smtp_error
d_smtp_socket_identity(
    const struct d_smtp_socket* _socket,
    struct d_smtp_buffer*       _buffer
)
{
    (void)_socket;
    (void)_buffer;

    return D_SMTP_ERROR_UNSUPPORTED;
}

void
d_smtp_socket_close(
    struct d_smtp_socket* _socket
)
{
    d_smtp_socket_init(_socket);

    return;
}

enum d_smtp_error
d_smtp_listener_open(
    struct d_smtp_listener* _listener,
    const char*             _host,
    unsigned int            _port
)
{
    (void)_listener;
    (void)_host;
    (void)_port;

    return D_SMTP_ERROR_UNSUPPORTED;
}

enum d_smtp_error
d_smtp_listener_accept(
    struct d_smtp_listener* _listener,
    long                    _wait_ms,
    long                    _io_timeout_ms,
    struct d_smtp_socket*   _socket
)
{
    (void)_listener;
    (void)_wait_ms;
    (void)_io_timeout_ms;
    (void)_socket;

    return D_SMTP_ERROR_UNSUPPORTED;
}

void
d_smtp_listener_close(
    struct d_smtp_listener* _listener
)
{
    d_smtp_listener_init(_listener);

    return;
}

#endif  // D_ENV_SMTP_HAS_SOCKETS


void
d_smtp_socket_transport(
    struct d_smtp_socket*    _socket,
    struct d_smtp_transport* _transport
)
{
    // parameter validation first
    if (!_transport)
    {
        return;
    }

    _transport->context  = _socket;
    _transport->read     = d_internal_smtp_socket_read;
    _transport->write    = d_internal_smtp_socket_write;
    _transport->starttls = d_internal_smtp_socket_starttls;
    _transport->close    = d_internal_smtp_socket_close;

    return;
}
