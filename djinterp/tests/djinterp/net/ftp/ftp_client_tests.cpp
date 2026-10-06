// C++ tests for net/ftp/ftp_client.hpp: the tiered interface over the C
// client, against the threaded FTP and FTPS server of ftps_tests_support.c.
// The core compiles at C++98; the section guarded by C++11 adds lambdas and
// the compile-time guarantees of the upper tier.
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include "../../../../inc/djinterp/net/ftp/ftp.hpp"
#include "./ftps_tests_support.h"
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <algorithm>
    #include <type_traits>
#endif

namespace ftp = djinterp::net::ftp;

static int g_fail   = 0;
static int g_checks = 0;
#define CHECK(c) do { g_checks++; if (!(c)) { g_fail++; std::printf("FAIL %d: %s\n", __LINE__, #c); } } while (0)

static unsigned long checksum(const char* _data, std::size_t _size)
{
    unsigned long sum = 0u;
    for (std::size_t i = 0u; i < _size; i++) sum = sum * 31u + (unsigned char)_data[i];
    return sum;
}

// a functor sink: counts the bytes and folds them into a checksum
struct tally
{
    std::size_t   size;
    unsigned long sum;
    tally() : size(0u), sum(0u) {}
    ftp::error operator()(const char* _data, std::size_t _size)
    {
        for (std::size_t i = 0u; i < _size; i++) sum = sum * 31u + (unsigned char)_data[i];
        size += _size;
        return ftp::error::none;
    }
};

// a const functor sink that refuses the data
struct refuser
{
    ftp::error operator()(const char*, std::size_t) const { return ftp::error::local_error; }
};

static ftp::options login_options()
{
    ftp::options o;
    o.user("alice").password("secret").connect_timeout(5000u);
    return o;
}

static bool start(struct srv& _server)
{
    struct srv_cfg config;
    std::memset(&config, 0, sizeof config);
    return srv_start(&_server, config) != 0;
}

// plain FTP: login, both callback styles, strings, listings, commands, quit
static void t_plain()
{
    struct srv s;
    CHECK(start(s));
    {
        ftp::client c(login_options());
        CHECK(c.connect("127.0.0.1", s.port) == ftp::error::none);
        CHECK(c.connected() && !c.logged_in() && !c.control_secured());
        CHECK(c.login() == ftp::error::none && c.logged_in());

        std::string hello;
        CHECK(c.retrieve("hello.txt", hello) == ftp::error::none && hello == HELLO);

        tally t;
        CHECK(c.retrieve("big.bin", t) == ftp::error::none);
        CHECK(t.size == sizeof g_big && t.sum == checksum(g_big, sizeof g_big));

        CHECK(c.store("up.txt", std::string("from C++\r\n")) == ftp::error::none);
        std::string       mutable_body("mutable\r\n");
        const std::string const_body("const\r\n");
        CHECK(c.store("m.txt", mutable_body) == ftp::error::none);  // a non-const string
        CHECK(c.store("c.txt", const_body) == ftp::error::none);    // a const one
        CHECK(c.store("up.txt", std::string("from C++\r\n")) == ftp::error::none);

        std::string          listing;
        d_ftp_listing_format format = D_FTP_LISTING_AUTO;
        CHECK(c.list(NULL, listing, &format) == ftp::error::none);
        CHECK(format == D_FTP_LISTING_MLSX && listing.find("hello.txt") != std::string::npos);

        CHECK(c.command(D_FTP_COMMAND_SIZE, "hello.txt") == ftp::error::none);
        CHECK(c.reply_code() == 213u && c.reply_text() == "24");
        CHECK(c.command(D_FTP_COMMAND_DELE, "x") == ftp::error::not_implemented);
        CHECK(c.command(D_FTP_COMMAND_PASV) == ftp::error::invalid_argument);

        // a const, temporary sink refusing the data stops the transfer
        CHECK(c.retrieve("hello.txt", refuser()) == ftp::error::local_error);
        // and the session goes on
        hello.clear();
        CHECK(c.retrieve("hello.txt", hello) == ftp::error::none && hello == HELLO);

        CHECK(c.quit() == ftp::error::none && !c.connected());
    }
    srv_stop(&s);
    CHECK(s.stored_len == 10u && std::memcmp(s.stored, "from C++\r\n", 10u) == 0);
}

// explicit FTPS with PROT P, through the toy engine: nothing in the clear
static void t_secure()
{
    ftp::options o = login_options();
    o.security(ftp::security::explicit_tls)
     .protection(ftp::protection::private_)
     .require_security(true);

    d_ssl_config config;
    ftp::client::tls_config(o, config);
    d_ssl_context context;
    CHECK(d_ssl_context_init(&context, &TOY, &config) == D_SSL_STATUS_OK);

    struct srv s;
    CHECK(start(s));
    {
        ftp::client c(o, &context);
        CHECK(c.connect("localhost", s.port) == ftp::error::none);
        CHECK(c.control_secured());
        CHECK(c.login() == ftp::error::none);
        CHECK(c.protection() == ftp::protection::private_);

        std::string hello;
        CHECK(c.retrieve("hello.txt", hello) == ftp::error::none && hello == HELLO);
        CHECK(c.quit() == ftp::error::none);
    }
    srv_stop(&s);
    CHECK(s.data_tls >= 1 && s.data_plain == 0 && !s.saw_user_in_clear);
    d_ssl_context_destroy(&context);
}

// active mode: the server connects back to the client's TCP listener
static void t_active()
{
    ftp::options o = login_options();
    o.data_mode(ftp::data_mode::active_auto);

    struct srv s;
    CHECK(start(s));
    {
        ftp::client c(o);
        std::string hello;
        CHECK(c.connect("127.0.0.1", s.port) == ftp::error::none);
        CHECK(c.login() == ftp::error::none);
        CHECK(c.retrieve("hello.txt", hello) == ftp::error::none && hello == HELLO);
        CHECK(c.quit() == ftp::error::none);
    }
    srv_stop(&s);
}

// errors are values: compared, switched on, converted, and described
static void t_errors()
{
    {
        ftp::client c;
        CHECK(c.login() == ftp::error::bad_sequence);
        CHECK(c.connect("") == ftp::error::invalid_argument);

        const ftp::error e = c.connect("127.0.0.1", 1u);
        CHECK(e == ftp::error::connect);
        CHECK(c.net_error() == D_NET_ERROR_CONNECTION_REFUSED);

        int seen = 0;
        switch (e)
        {
            case ftp::error::connect: seen = 1; break;
            case ftp::error::timeout: seen = 2; break;
            default:                  seen = 3; break;
        }
        CHECK(seen == 1);
    }

    struct srv s;
    CHECK(start(s));
    {
        ftp::options o = login_options();
        o.password("wrong");
        ftp::client c(o);
        CHECK(c.connect(std::string("127.0.0.1"), s.port) == ftp::error::none);
        CHECK(c.login() == ftp::error::login_denied && !c.logged_in());
        c.close();
        CHECK(!c.connected());
    }
    srv_stop(&s);

    CHECK(std::strcmp(ftp::error_string(ftp::error::timeout), d_ftp_error_string(D_FTP_ERROR_TIMEOUT)) == 0);
    CHECK(ftp::to_c(ftp::from_c(D_FTP_ERROR_TLS)) == D_FTP_ERROR_TLS);
    CHECK(ftp::from_c(D_FTP_OK) == ftp::error::none);
    CHECK(ftp::to_c(ftp::security::implicit_tls) == D_FTP_SECURITY_IMPLICIT);
    CHECK(ftp::to_c(ftp::protection::private_) == D_FTP_PROTECTION_PRIVATE);
    CHECK(ftp::to_c(ftp::data_mode::extended_active) == D_FTP_DATA_EXTENDED_ACTIVE);

    ftp::error defaulted = ftp::error();
    CHECK(defaulted == ftp::error::none);

    ftp::options o;
    o.ascii().use_utf8(false).verify_peer(false);
    CHECK(o.c_options().type.data_type == D_FTP_TYPE_ASCII);
    CHECK(!o.c_options().use_utf8 && !o.c_options().verify_peer);
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
// C++11: lambdas written in place, and the tier's compile-time guarantees
static void t_lambdas()
{
    static_assert(!std::is_copy_constructible<ftp::client>::value,
                  "a client must not be copyable");
    static_assert(!std::is_convertible<ftp::error, int>::value,
                  "from C++11 an error is an enum class");
    static_assert(std::is_same<std::underlying_type<ftp::error>::type, int>::value,
                  "an error's values are d_ftp_error's");

    struct srv s;
    CHECK(start(s));
    {
        ftp::client c(login_options());
        CHECK(c.connect("127.0.0.1", s.port) == ftp::error::none);
        CHECK(c.login() == ftp::error::none);

        std::size_t total = 0u;
        CHECK(c.retrieve("big.bin",
                         [&total](const char*, std::size_t _size)
                         {
                             total += _size;
                             return ftp::error::none;
                         }) == ftp::error::none);
        CHECK(total == sizeof g_big);

        const std::string body   = "lambda upload\r\n";
        std::size_t       offset = 0u;
        CHECK(c.store("l.txt",
                      [&](char* _buffer, std::size_t _capacity, std::size_t& _size)
                      {
                          _size = std::min(_capacity, body.size() - offset);
                          std::memcpy(_buffer, body.data() + offset, _size);
                          offset += _size;
                          return ftp::error::none;
                      }) == ftp::error::none);
        CHECK(c.quit() == ftp::error::none);
    }
    srv_stop(&s);
    CHECK(s.stored_len == 15u && std::memcmp(s.stored, "lambda upload\r\n", 15u) == 0);
}
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

int main()
{
    for (std::size_t i = 0u; i < sizeof g_big; i++) g_big[i] = (char)(i * 31u + 7u);
    t_plain();
    t_secure();
    t_active();
    t_errors();
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    t_lambdas();
#endif
    std::printf("%d checks, %d failures (C++%ld)\n", g_checks, g_fail, (long)(__cplusplus / 100 % 100));
    return g_fail != 0;
}
