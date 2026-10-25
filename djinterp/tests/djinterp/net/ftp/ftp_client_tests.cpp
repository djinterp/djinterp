/*******************************************************************************
* djinterp [net]                                            ftp_client_tests.cpp
*
* C++ tests for net/ftp/ftp_client.hpp: the interface over the C client,
* against the threaded FTP and FTPS server of ftps_tests_support.c. The core
* compiles at C++98; the section guarded by C++11 adds lambdas and the
* compile-time guarantees C++11 can state. The enumerations are scoped in
* structs at every level (decision 3.4): an error is an ftp::error::value.
*
*
* path:      /tests/djinterp/net/ftp/ftp_client_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.29
*                                                            revised: 2026.10.23
*******************************************************************************/
// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf
#include <cstring>  // std::memcmp, std::memcpy, std::memset, std::strcmp
#include <string>   // std::string
// djinterp
#include "../../../../inc/djinterp/net/ftp/ftp.hpp"  // ftp::client

// the clients need a 64-bit integer type; where there is none (ISO strict C++98
// wherever int64_t is `long long`: a 32-bit target, 64-bit Windows or macOS)
// they compile to nothing, and the tests with them
#if (D_INTERNAL_NET_HAS_INT64 == 1)
#include "./ftps_tests_support.h"  // the server, the toy engine
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
#include <algorithm>    // std::find, std::min
#include <type_traits>  // std::is_same, std::is_enum, ...
#endif


namespace ftp = djinterp::net::ftp;

// each enumerator is its C counterpart, at compile time at every level; at
// namespace scope, since below C++11 D_STATIC_ASSERT is a typedef, which a
// block would warn of as unused
D_STATIC_ASSERT((static_cast<int>(ftp::error::timed_out) ==
                 D_FTP_ERROR_TIMED_OUT),
                "an error's values are d_ftp_error's");
D_STATIC_ASSERT((static_cast<int>(ftp::data_mode::extended_active) ==
                 D_FTP_DATA_EXTENDED_ACTIVE),
                "a data mode's values are d_ftp_data_mode's");

static int g_fail   = 0;
static int g_checks = 0;

/*
tests_ftp_client_check
  Counts one check, and names it when it fails.
*/
static void
tests_ftp_client_check(
    bool        _ok,
    const char* _text
)
{
    g_checks++;

    // a failed check says which
    if (!_ok)
    {
        g_fail++;
        std::printf("FAIL: %s\n",
                    _text);
    }

    return;
}

// D_TESTS_FTP_CLIENT_CHECK
//   macro: one check, its condition's text naming it when it fails.
#define D_TESTS_FTP_CLIENT_CHECK(c) tests_ftp_client_check((c), #c)

/*
checksum
  A simple sum of a string's octets, to compare a transfer with its source.
*/
static unsigned long
checksum(
    const char* _data,
    std::size_t _size
)
{
    unsigned long sum = 0u;
    for (std::size_t i = 0u; i < _size; i++)
    {
        sum = sum * 31u + (unsigned char)_data[i];
    }

    return sum;
}

// a functor sink: counts the bytes and folds them into a checksum
struct tally
{
    std::size_t   size;
    unsigned long sum;

    tally() : size(0u), sum(0u)
    {
    }

    ftp::error::value operator()(const char* _data, std::size_t _size)
    {
        for (std::size_t i = 0u; i < _size; i++)
        {
            sum = sum * 31u + (unsigned char)_data[i];
        }

        size += _size;

        return ftp::error::none;
    }
};

// a const functor sink that refuses the data
struct refuser
{
    ftp::error::value operator()(const char*, std::size_t) const
    {
        return ftp::error::local_error;
    }
};

/*
login_options
  The options every session logs in with: alice and her password, and a
five-second connect timeout.
*/
static ftp::options
login_options(
    void
)
{
    ftp::options o;
    o.user("alice").password("secret").connect_timeout(5000u);

    return o;
}

/*
start
  Starts the shared test server with its default configuration; false when it
could not.
*/
static bool
start(
    struct d_tests_ftps_server& _server
)
{
    struct d_tests_ftps_server_config config;
    std::memset(&config,
                0,
                sizeof config);

    return d_tests_ftps_server_start(&_server,
                                     config) != 0;
}

/*
t_plain
  Plain FTP: login, both callback styles, strings, listings, commands, quit.
It runs past 75 lines because one session runs through every check.
*/
static void
t_plain(
    void
)
{
    struct d_tests_ftps_server s;
    D_TESTS_FTP_CLIENT_CHECK(start(s));
    {
        ftp::client c(login_options());
        D_TESTS_FTP_CLIENT_CHECK(c.connect("127.0.0.1",
                                           s.port) ==
                                 ftp::error::none);
        D_TESTS_FTP_CLIENT_CHECK( (c.connected())  &&
                                  (!c.logged_in()) &&
                                  (!c.control_secured()) );
        D_TESTS_FTP_CLIENT_CHECK( (c.login() == ftp::error::none) &&
                                  (c.logged_in()) );

        std::string hello;
        D_TESTS_FTP_CLIENT_CHECK( (c.retrieve("hello.txt",
                                              hello) == ftp::error::none) &&
                                  (hello == HELLO) );

        tally t;
        D_TESTS_FTP_CLIENT_CHECK(c.retrieve("big.bin",
                                            t) == ftp::error::none);
        D_TESTS_FTP_CLIENT_CHECK( (t.size == sizeof g_big) &&
                                  (t.sum == checksum(g_big,
                                                     sizeof g_big)) );

        D_TESTS_FTP_CLIENT_CHECK(
            c.store("up.txt",
                    std::string("from C++\r\n")) == ftp::error::none);
        std::string       mutable_body("mutable\r\n");
        const std::string const_body("const\r\n");
        D_TESTS_FTP_CLIENT_CHECK(c.store("m.txt",
                                         mutable_body) ==
                                 ftp::error::none);  // a non-const string
        D_TESTS_FTP_CLIENT_CHECK(c.store("c.txt",
                                         const_body) ==
                                 ftp::error::none);  // a const one
        D_TESTS_FTP_CLIENT_CHECK(
            c.store("up.txt",
                    std::string("from C++\r\n")) == ftp::error::none);

        std::string          listing;
        d_ftp_listing_format format = D_FTP_LISTING_AUTO;
        D_TESTS_FTP_CLIENT_CHECK(c.list(NULL,
                                        listing,
                                        &format) ==
                                 ftp::error::none);
        D_TESTS_FTP_CLIENT_CHECK( (format == D_FTP_LISTING_MLSX) &&
                                  (listing.find("hello.txt") !=
                                   std::string::npos) );

        D_TESTS_FTP_CLIENT_CHECK(c.command(D_FTP_COMMAND_SIZE,
                                           "hello.txt") ==
                                 ftp::error::none);
        D_TESTS_FTP_CLIENT_CHECK( (c.reply_code() == 213u) &&
                                  (c.reply_text() == "24") );
        D_TESTS_FTP_CLIENT_CHECK(c.command(D_FTP_COMMAND_DELE,
                                           "x") ==
                                 ftp::error::not_implemented);
        D_TESTS_FTP_CLIENT_CHECK(c.command(D_FTP_COMMAND_PASV) ==
                                 ftp::error::invalid_argument);

        // a const, temporary sink refusing the data stops the transfer
        D_TESTS_FTP_CLIENT_CHECK(c.retrieve("hello.txt",
                                            refuser()) ==
                                 ftp::error::local_error);
        // and the session goes on
        hello.clear();
        D_TESTS_FTP_CLIENT_CHECK( (c.retrieve("hello.txt",
                                              hello) == ftp::error::none) &&
                                  (hello == HELLO) );

        D_TESTS_FTP_CLIENT_CHECK( (c.quit() == ftp::error::none) &&
                                  (!c.connected()) );
    }
    d_tests_ftps_server_stop(&s);
    D_TESTS_FTP_CLIENT_CHECK( (s.stored_len == 10u) &&
                              (std::memcmp(s.stored,
                                           "from C++\r\n",
                                           10u) == 0) );

    return;
}

/*
t_secure
  Explicit FTPS with PROT P, through the toy engine: nothing in the clear.
*/
static void
t_secure(
    void
)
{
    ftp::options o = login_options();
    o.security(ftp::security::explicit_tls)
        .protection(ftp::protection::private_)
        .require_security(true);

    d_ssl_config config;
    ftp::client::tls_config(o,
                            config);
    d_ssl_context context;
    D_TESTS_FTP_CLIENT_CHECK(d_ssl_context_init(&context,
                                                &TOY,
                                                &config) ==
                             D_SSL_OK);

    struct d_tests_ftps_server s;
    D_TESTS_FTP_CLIENT_CHECK(start(s));
    {
        ftp::client c(o,
                      &context);
        D_TESTS_FTP_CLIENT_CHECK(c.connect("localhost",
                                           s.port) ==
                                 ftp::error::none);
        D_TESTS_FTP_CLIENT_CHECK(c.control_secured());
        D_TESTS_FTP_CLIENT_CHECK(c.login() == ftp::error::none);
        D_TESTS_FTP_CLIENT_CHECK(c.protection() == ftp::protection::private_);

        std::string hello;
        D_TESTS_FTP_CLIENT_CHECK( (c.retrieve("hello.txt",
                                              hello) == ftp::error::none) &&
                                  (hello == HELLO) );
        D_TESTS_FTP_CLIENT_CHECK(c.quit() == ftp::error::none);
    }
    d_tests_ftps_server_stop(&s);
    D_TESTS_FTP_CLIENT_CHECK( (s.data_tls >= 1)   &&
                              (s.data_plain == 0) &&
                              (!s.saw_user_in_clear) );
    d_ssl_context_destroy(&context);

    return;
}

/*
t_active
  Active mode: the server connects back to the client's TCP listener.
*/
static void
t_active(
    void
)
{
    ftp::options o = login_options();
    o.data_mode(ftp::data_mode::active_auto);

    struct d_tests_ftps_server s;
    D_TESTS_FTP_CLIENT_CHECK(start(s));
    {
        ftp::client c(o);
        std::string hello;
        D_TESTS_FTP_CLIENT_CHECK(c.connect("127.0.0.1",
                                           s.port) ==
                                 ftp::error::none);
        D_TESTS_FTP_CLIENT_CHECK(c.login() == ftp::error::none);
        D_TESTS_FTP_CLIENT_CHECK( (c.retrieve("hello.txt",
                                              hello) == ftp::error::none) &&
                                  (hello == HELLO) );
        D_TESTS_FTP_CLIENT_CHECK(c.quit() == ftp::error::none);
    }
    d_tests_ftps_server_stop(&s);

    return;
}

/*
t_errors
  Errors are values: compared, switched on, converted, and described. It runs
past 75 lines because each kind of error gets its own short check.
*/
static void
t_errors(
    void
)
{
    {
        ftp::client c;
        D_TESTS_FTP_CLIENT_CHECK(c.login() == ftp::error::bad_sequence);
        D_TESTS_FTP_CLIENT_CHECK(c.connect("") == ftp::error::invalid_argument);

        const ftp::error::value e = c.connect("127.0.0.1",
                                              1u);
        D_TESTS_FTP_CLIENT_CHECK(e == ftp::error::connect);
        D_TESTS_FTP_CLIENT_CHECK(c.net_error() ==
                                 D_NET_ERROR_CONNECTION_REFUSED);

        int seen = 0;
        switch (e)
        {
            case ftp::error::connect:
                seen = 1;
                break;
            case ftp::error::timed_out:
                seen = 2;
                break;
            default:
                seen = 3;
                break;
        }

        D_TESTS_FTP_CLIENT_CHECK(seen == 1);
    }

    struct d_tests_ftps_server s;
    D_TESTS_FTP_CLIENT_CHECK(start(s));
    {
        ftp::options o = login_options();
        o.password("wrong");
        ftp::client c(o);
        D_TESTS_FTP_CLIENT_CHECK(c.connect(std::string("127.0.0.1"),
                                           s.port) ==
                                 ftp::error::none);
        D_TESTS_FTP_CLIENT_CHECK( (c.login() == ftp::error::login_denied) &&
                                  (!c.logged_in()) );
        c.close();
        D_TESTS_FTP_CLIENT_CHECK(!c.connected());
    }
    d_tests_ftps_server_stop(&s);

    D_TESTS_FTP_CLIENT_CHECK(
        std::strcmp(ftp::error_string(ftp::error::timed_out),
                    d_ftp_error_string(D_FTP_ERROR_TIMED_OUT)) == 0);
    // to_net is d_ftp_error_to_net, error for error
    for (int e = 0; e <= static_cast<int>(D_FTP_ERROR_UNKNOWN); ++e)
    {
        D_TESTS_FTP_CLIENT_CHECK(
            djinterp::net::to_c(
                ftp::to_net(ftp::from_c(static_cast<d_ftp_error>(e)))) ==
            d_ftp_error_to_net(static_cast<d_ftp_error>(e)));
    }

    D_TESTS_FTP_CLIENT_CHECK(ftp::to_c(ftp::from_c(D_FTP_ERROR_TLS)) ==
                             D_FTP_ERROR_TLS);
    D_TESTS_FTP_CLIENT_CHECK(ftp::from_c(D_FTP_OK) == ftp::error::none);
    D_TESTS_FTP_CLIENT_CHECK(ftp::to_c(ftp::security::implicit_tls) ==
                             D_FTP_SECURITY_IMPLICIT);
    D_TESTS_FTP_CLIENT_CHECK(ftp::to_c(ftp::protection::private_) ==
                             D_FTP_PROTECTION_PRIVATE);
    D_TESTS_FTP_CLIENT_CHECK(ftp::to_c(ftp::data_mode::extended_active) ==
                             D_FTP_DATA_EXTENDED_ACTIVE);

    // a value-initialized error is success, d_ftp_error's zero
    ftp::error::value defaulted = ftp::error::value();
    D_TESTS_FTP_CLIENT_CHECK(defaulted == ftp::error::none);


    ftp::options o;
    o.ascii().use_utf8(false).verify_peer(false);
    D_TESTS_FTP_CLIENT_CHECK(o.c_options().type.data_type == D_FTP_TYPE_ASCII);
    D_TESTS_FTP_CLIENT_CHECK( (!o.c_options().use_utf8) &&
                              (!o.c_options().verify_peer) );

    return;
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
/*
t_lambdas
  C++11: lambdas written in place, and the tier's compile-time guarantees.
*/
static void
t_lambdas(
    void
)
{
    static_assert(!std::is_copy_constructible<ftp::client>::value,
                  "a client must not be copyable");
    static_assert(std::is_enum<ftp::error::value>::value,
                  "an error is a plain enumeration scoped in a struct");

    struct d_tests_ftps_server s;
    D_TESTS_FTP_CLIENT_CHECK(start(s));
    {
        ftp::client c(login_options());
        D_TESTS_FTP_CLIENT_CHECK(c.connect("127.0.0.1",
                                           s.port) ==
                                 ftp::error::none);
        D_TESTS_FTP_CLIENT_CHECK(c.login() == ftp::error::none);

        std::size_t total = 0u;
        D_TESTS_FTP_CLIENT_CHECK(
            c.retrieve("big.bin",
                       [&total](const char*, std::size_t _size)
                       {
                           total += _size;

                           return ftp::error::none;
                       }) == ftp::error::none);
        D_TESTS_FTP_CLIENT_CHECK(total == sizeof g_big);

        const std::string body   = "lambda upload\r\n";
        std::size_t       offset = 0u;
        D_TESTS_FTP_CLIENT_CHECK(
            c.store(
                "l.txt",
                [&](char* _buffer, std::size_t _capacity, std::size_t& _size)
                {
                    _size = std::min(_capacity,
                                     body.size() - offset);
                    std::memcpy(_buffer,
                                body.data() + offset,
                                _size);
                    offset += _size;

                    return ftp::error::none;
                }) == ftp::error::none);
        D_TESTS_FTP_CLIENT_CHECK(c.quit() == ftp::error::none);
    }
    d_tests_ftps_server_stop(&s);
    D_TESTS_FTP_CLIENT_CHECK( (s.stored_len == 15u) &&
                              (std::memcmp(s.stored,
                                           "lambda upload\r\n",
                                           15u) == 0) );

    return;
}
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

/*
main
  Runs every test, then prints the count; the exit status is whether any check
failed.
*/
int
main(
    void
)
{
    for (std::size_t i = 0u; i < sizeof g_big; i++)
    {
        g_big[i] = (char)(i * 31u + 7u);
    }

    t_plain();
    t_secure();
    t_active();
    t_errors();
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    t_lambdas();
#endif
    std::printf("%d checks, %d failures (%s)\n",
                g_checks,
                g_fail,
                D_ENV_LANG_CPP_STANDARD_NAME);

    return g_fail != 0;
}

#else

/*
main
  Below the 64-bit floor the clients compile to nothing, and so do the tests.
*/
int
main(
    void
)
{
    std::printf("no 64-bit integer type: nothing to test\n");

    return 0;
}

#endif  // D_INTERNAL_NET_HAS_INT64
