/*******************************************************************************
* djinterp [net]                                           sftp_client_tests.cpp
*
* C++ tests for net/sftp/sftp_client.hpp: the tiered interface over the C
* client, against OpenSSH's own sftp-server, run as a child process whose
* stdin and stdout are one end of a socket pair: a real server, reached by
* client::open over the other end -- tcp.hpp's socket_connection, handed to
* the C client by net.hpp's c_connection_adapter -- with no SSH session in
* between; and over C++ connections that fail as told. POSIX only; the
* server's tests skip where sftp-server is not installed. The core compiles at
* C++98, re_std::expected's value-returning forms included; C++11 adds
* lambdas. The error is scoped in a struct at every level (decision 3.4).
*
*
* path:      /tests/djinterp/net/sftp/sftp_client_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.29
*                                                            revised: 2026.10.23
*******************************************************************************/
// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf
#include <cstdlib>  // std::exit
#include <cstring>  // std::memcmp, std::memset
#include <string>   // std::string
// POSIX
#include <fcntl.h>       // open, O_*
#include <signal.h>      // signal, SIGPIPE
#include <sys/socket.h>  // socketpair
#include <sys/stat.h>    // stat
#include <sys/types.h>   // pid_t
#include <sys/wait.h>    // waitpid
#include <unistd.h>      // fork, dup2, execl, close
// djinterp
#include "../../../../inc/djinterp/net/net_common.hpp"  // net::connection
#include "../../../../inc/djinterp/net/sftp/sftp.hpp"   // sftp::client
#include "../../../../inc/djinterp/net/tcp/tcp.hpp"     // socket_connection

// the clients need a 64-bit integer type; where there is none (ISO strict C++98
// wherever int64_t is `long long`: a 32-bit target, 64-bit Windows or macOS)
// they compile to nothing, and the tests with them
#if (D_INTERNAL_NET_HAS_INT64 == 1)
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
#include <type_traits>  // std::is_same
#endif


namespace net  = djinterp::net;
namespace sftp = djinterp::net::sftp;

static int g_fail   = 0;
static int g_checks = 0;

/*
tests_sftp_client_check
  Counts one check, and names it when it fails.
*/
static void
tests_sftp_client_check(
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

// D_TESTS_SFTP_CLIENT_CHECK
//   macro: one check, its condition's text naming it when it fails.
#define D_TESTS_SFTP_CLIENT_CHECK(c) tests_sftp_client_check((c), #c)

// the child server, and our end of the socket pair its stdin and stdout are
struct child
{
    pid_t pid;
    int   fd;
};

static const char* SERVER = "/usr/lib/openssh/sftp-server";

/*
spawn
  Starts OpenSSH's sftp-server as a child whose stdin and stdout are one end
of a socket pair; false where it is not installed.
*/
static bool
spawn(
    child& _child
)
{
    int pair[2];
    if (::socketpair(AF_UNIX,
                     SOCK_STREAM,
                     0,
                     pair) != 0)
    {
        return false;
    }

    _child.pid = ::fork();
    if (_child.pid == 0)
    {
        ::dup2(pair[1],
               0);
        ::dup2(pair[1],
               1);
        ::close(pair[0]);
        ::close(pair[1]);
        ::execl(SERVER,
                SERVER,
                (char*)NULL);
        ::_exit(127);
    }

    ::close(pair[1]);
    _child.fd = pair[0];

    return _child.pid > 0;
}

/*
reap
  The server exits at the end of its input, once our end is closed.
*/
static void
reap(
    child& _child
)
{
    int status = 0;
    ::waitpid(_child.pid,
              &status,
              0);

    return;
}

// a connection whose reads fail as told -- io_error::none ends the stream --
// and whose writes all succeed; it counts its closes
class failing : public net::connection
{
  public:
    explicit failing(net::io_error::value _error) : m_error(_error), m_closes(0)
    {
    }

    net::io_result read(void*,
                        std::size_t) D_OVERRIDE
    {
        return net::io_result(0u,
                              m_error);
    }

    net::io_result write(const void*,
                         std::size_t _size) D_OVERRIDE
    {
        return net::io_result(_size,
                              net::io_error::none);
    }

    bool is_open() const D_OVERRIDE
    {
        return m_closes == 0;
    }

    void close() D_OVERRIDE
    {
        m_closes++;
    }

    int closes() const
    {
        return m_closes;
    }

  private:
    net::io_error::value m_error;
    int                  m_closes;
};

/*
t_failing
  A stream that fails beneath the client: the error, net_error() behind it, no
SSH status without a channel, and the connection left to its owner.
*/
static void
t_failing(
    void
)
{
    failing                   timed(net::io_error::timed_out);
    failing                   ended(net::io_error::none);
    failing                   reset(net::io_error::connection_reset);
    net::c_connection_adapter timed_adapter(timed);
    net::c_connection_adapter ended_adapter(ended);
    net::c_connection_adapter reset_adapter(reset);
    sftp::client              c;
    D_TESTS_SFTP_CLIENT_CHECK(c.open(timed_adapter.get()) ==
                              sftp::error::transport);
    D_TESTS_SFTP_CLIENT_CHECK( (c.net_error() == D_NET_ERROR_TIMED_OUT) &&
                               (c.ssh_status() == D_SSH_OK)             &&
                               (!c.is_open()) );
    D_TESTS_SFTP_CLIENT_CHECK( (c.open(ended_adapter.get()) ==
                                sftp::error::connection_lost) &&
                               (c.net_error() == D_NET_ERROR_CLOSED) );
    D_TESTS_SFTP_CLIENT_CHECK( (c.open(reset_adapter.get()) ==
                                sftp::error::connection_lost) &&
                               (c.net_error() ==
                                D_NET_ERROR_CONNECTION_RESET) );
    D_TESTS_SFTP_CLIENT_CHECK(c.open(NULL) == sftp::error::invalid_argument);
    c.close();
    D_TESTS_SFTP_CLIENT_CHECK( (timed.closes() == 0) &&
                               (ended.closes() == 0) &&
                               (reset.closes() == 0) );

    return;
}

// a functor sink: counts and compares against the expected bytes
struct compare
{
    const std::string* expected;
    std::size_t        at;
    bool               same;

    explicit compare(const std::string& _expected)
        : expected(&_expected), at(0u), same(true)
    {
    }

    sftp::error::value operator()(const char* _data, std::size_t _size)
    {
        same = same && (at + _size <= expected->size()) &&
               std::memcmp(expected->data() + at,
                           _data,
                           _size) == 0;
        at += _size;

        return sftp::error::none;
    }
};

// a directory walker that looks for one name
struct finder
{
    const char* wanted;
    bool        found;
    int         entries;

    explicit finder(const char* _wanted)
        : wanted(_wanted), found(false), entries(0)
    {
    }

    sftp::error::value operator()(const sftp::name& _entry)
    {
        entries++;
        found = found || (_entry.filename.length == std::strlen(wanted) &&
                          std::memcmp(_entry.filename.data,
                                      wanted,
                                      _entry.filename.length) == 0);

        return sftp::error::none;
    }
};

/*
t_session
  A whole session against the real server: a file opened, written, read back
and stat'ed; realpath, a symlink and readlink; a rename, a listing, get, and
the removals. It runs past 75 lines because the one session runs through
every check.
*/
static void
t_session(
    sftp::client&      _c,
    const std::string& _dir
)
{
    std::string where;
    D_TESTS_SFTP_CLIENT_CHECK( (_c.realpath(_dir.c_str(),
                                            where) == sftp::error::none) &&
                               (where == _dir) );

    // a whole file up and back: 300 KB crosses many pipelined chunks
    std::string body(300000u,
                     '\0');
    for (std::size_t i = 0u; i < body.size(); i++)
    {
        body[i] = (char)(i * 131u + 17u);
    }

    const std::string file = _dir + "/data.bin";
    D_TESTS_SFTP_CLIENT_CHECK(_c.put(file.c_str(),
                                     body) == sftp::error::none);
    std::string back;
    D_TESTS_SFTP_CLIENT_CHECK( (_c.get(file.c_str(),
                                       back) == sftp::error::none) &&
                               (back == body) );
    compare cmp(body);
    D_TESTS_SFTP_CLIENT_CHECK( (_c.get(file.c_str(),
                                       cmp) == sftp::error::none) &&
                               (cmp.same)                         &&
                               (cmp.at == body.size()) );

    sftp::attributes a;
    D_TESTS_SFTP_CLIENT_CHECK(_c.stat(file.c_str(),
                                      a) == sftp::error::none);
    D_TESTS_SFTP_CLIENT_CHECK( ((a.flags & sftp::ATTR_SIZE) != 0u) &&
                               (a.size == 300000u) );

    // an open file: write at an offset, read it back, fstat, close
    sftp::handle      h;
    const std::string small = _dir + "/small.txt";
    D_TESTS_SFTP_CLIENT_CHECK(
        _c.open_file(small.c_str(),
                     sftp::OPEN_WRITE | sftp::OPEN_CREATE | sftp::OPEN_TRUNCATE,
                     NULL,
                     h) == sftp::error::none);
    D_TESTS_SFTP_CLIENT_CHECK(_c.write(h,
                                       0u,
                                       "hello, sftp",
                                       11u) ==
                              sftp::error::none);
    D_TESTS_SFTP_CLIENT_CHECK(_c.close_file(h) == sftp::error::none);
    D_TESTS_SFTP_CLIENT_CHECK(
        _c.open_file(small.c_str(),
                     sftp::OPEN_READ,
                     NULL,
                     h) ==
        sftp::error::none);
    char        buffer[32];
    std::size_t got = 0u;
    D_TESTS_SFTP_CLIENT_CHECK( (_c.read(h,
                                        7u,
                                        buffer,
                                        sizeof buffer,
                                        got) == sftp::error::none) &&
                               (got == 4u)                         &&
                               (std::memcmp(buffer,
                                            "sftp",
                                            4u) == 0) );
    D_TESTS_SFTP_CLIENT_CHECK(_c.read(h,
                                      11u,
                                      buffer,
                                      sizeof buffer,
                                      got) ==
                              sftp::error::eof);
    D_TESTS_SFTP_CLIENT_CHECK( (_c.fstat(h,
                                         a) == sftp::error::none) &&
                               (a.size == 11u) );
    D_TESTS_SFTP_CLIENT_CHECK(_c.close_file(h) == sftp::error::none);

    // names: rename with replace, links, directories, listings
    const std::string moved = _dir + "/moved.txt";
    D_TESTS_SFTP_CLIENT_CHECK(_c.rename(small.c_str(),
                                        moved.c_str(),
                                        true) ==
                              sftp::error::none);
    const std::string link = _dir + "/link";
    D_TESTS_SFTP_CLIENT_CHECK(_c.symlink("moved.txt",
                                         link.c_str()) ==
                              sftp::error::none);
    std::string target;
    D_TESTS_SFTP_CLIENT_CHECK( (_c.readlink(link.c_str(),
                                            target) == sftp::error::none) &&
                               (target == "moved.txt") );
    D_TESTS_SFTP_CLIENT_CHECK( (_c.lstat(link.c_str(),
                                         a) == sftp::error::none) &&
                               (S_ISLNK(a.permissions)) );
    const std::string sub = _dir + "/sub";
    D_TESTS_SFTP_CLIENT_CHECK(_c.mkdir(sub.c_str()) == sftp::error::none);
    finder f("sub");
    D_TESTS_SFTP_CLIENT_CHECK( (_c.list(_dir.c_str(),
                                        f) == sftp::error::none) &&
                               (f.found)                         &&
                               (f.entries >= 6) );

    // errors are values, switched on and described
    const sftp::error::value e = _c.stat((_dir + "/missing").c_str(),
                                         a);
    D_TESTS_SFTP_CLIENT_CHECK( (e == sftp::error::no_such_file) &&
                               (_c.status_code() == 2u) );
    int seen = 0;
    switch (e)
    {
        case sftp::error::no_such_file:
            seen = 1;
            break;
        case sftp::error::permission_denied:
            seen = 2;
            break;
        default:
            seen = 3;
            break;
    }

    D_TESTS_SFTP_CLIENT_CHECK(seen == 1);
    D_TESTS_SFTP_CLIENT_CHECK(
        std::strcmp(sftp::error_string(sftp::error::eof),
                    d_sftp_error_string(D_SFTP_ERROR_EOF)) == 0);
    D_TESTS_SFTP_CLIENT_CHECK(
        sftp::to_c(sftp::from_c(D_SFTP_ERROR_MESSAGE_TOO_LARGE)) ==
        D_SFTP_ERROR_MESSAGE_TOO_LARGE);
    // to_net is d_sftp_error_to_net, error for error: the status codes, then
    // the client's own
    for (int n = 0; n <= 0x106; n = (n == 8) ? 0x100 : n + 1)
    {
        D_TESTS_SFTP_CLIENT_CHECK(
            djinterp::net::to_c(
                sftp::to_net(sftp::from_c(static_cast<d_sftp_error>(n)))) ==
            d_sftp_error_to_net(static_cast<d_sftp_error>(n)));
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    static_assert(!std::is_copy_constructible<sftp::client>::value,
                  "a client must not be copyable");
    static_assert(std::is_enum<sftp::error::value>::value,
                  "an error is an enumeration scoped in a struct");
#endif
    // the value-returning forms, at every level
    typedef re_std::expected<sftp::attributes, sftp::error::value>
                                                              attributes_result;
    typedef re_std::expected<std::string, sftp::error::value> text_result;
    typedef re_std::expected<sftp::handle, sftp::error::value> handle_result;
    typedef re_std::expected<std::size_t, sftp::error::value>  count_result;
    const attributes_result stat = _c.stat(file.c_str());
    D_TESTS_SFTP_CLIENT_CHECK( (stat.has_value()) &&
                               ((*stat).size == 300000u) );
    const attributes_result missing = _c.stat((_dir + "/missing").c_str());
    D_TESTS_SFTP_CLIENT_CHECK( (!missing.has_value()) &&
                               (missing.error() == sftp::error::no_such_file) );
    const text_result real = _c.realpath(_dir.c_str());
    D_TESTS_SFTP_CLIENT_CHECK( (real.has_value()) &&
                               (*real == _dir) );
    const handle_result opened = _c.open_file(moved.c_str(),
                                              sftp::OPEN_READ);
    D_TESTS_SFTP_CLIENT_CHECK(opened.has_value());
    if (opened.has_value())
    {
        const count_result count = _c.read(*opened,
                                           0u,
                                           buffer,
                                           sizeof buffer);
        D_TESTS_SFTP_CLIENT_CHECK( (count.has_value()) &&
                                   (*count == 11u) );
        D_TESTS_SFTP_CLIENT_CHECK(_c.close_file(*opened) == sftp::error::none);
    }

    const text_result pointed = _c.readlink(link.c_str());
    D_TESTS_SFTP_CLIENT_CHECK( (pointed.has_value()) &&
                               (*pointed == "moved.txt") );
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    // lambdas written in place
    std::size_t total = 0u;
    D_TESTS_SFTP_CLIENT_CHECK( (_c.get(file.c_str(),
                                       [&total](const char*,
                                                std::size_t _size)
                                       {
                                           total += _size;

                                           return sftp::error::none;
                                       }) == sftp::error::none) &&
                               (total == 300000u) );
    int entries = 0;
    D_TESTS_SFTP_CLIENT_CHECK( (_c.list(_dir.c_str(),
                                        [&entries](const sftp::name&)
                                        {
                                            entries++;

                                            return sftp::error::none;
                                        }) == sftp::error::none) &&
                               (entries >= 6) );
#endif

    D_TESTS_SFTP_CLIENT_CHECK(_c.remove(link.c_str()) == sftp::error::none);
    D_TESTS_SFTP_CLIENT_CHECK(_c.remove(moved.c_str()) == sftp::error::none);
    D_TESTS_SFTP_CLIENT_CHECK(_c.remove(file.c_str()) == sftp::error::none);
    D_TESTS_SFTP_CLIENT_CHECK(_c.rmdir(sub.c_str()) == sftp::error::none);

    return;
}

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
    ::signal(SIGPIPE,
             SIG_IGN);
    t_failing();
    if (::access(SERVER,
                 X_OK) != 0)
    {
        std::printf("server skipped: %s not found\n",
                    SERVER);
        std::printf("%d checks, %d failures (%s)\n",
                    g_checks,
                    g_fail,
                    D_ENV_LANG_CPP_STANDARD_NAME);

        return g_fail != 0;
    }

    char        pattern[] = "/tmp/sftp_cpp_XXXXXX";
    const char* made      = ::mkdtemp(pattern);
    D_TESTS_SFTP_CLIENT_CHECK(made != NULL);
    char              resolved[4096];
    const std::string dir =
        (made && ::realpath(made,
                            resolved)) ? resolved : "";

    child server;
    D_TESTS_SFTP_CLIENT_CHECK(spawn(server));
    {
        // the client is made last, so it goes first, before what it borrows
        net::socket_connection    conn(server.fd,
                                       net::protocol::unix_socket);
        net::c_connection_adapter adapter(conn);
        sftp::client              c;
        D_TESTS_SFTP_CLIENT_CHECK( (!c.is_open()) &&
                                   (conn.is_open()) );
        D_TESTS_SFTP_CLIENT_CHECK(c.open(adapter.get()) == sftp::error::none);
        D_TESTS_SFTP_CLIENT_CHECK( (c.is_open()) &&
                                   (c.version() == 3u) );
        if (c.is_open() && !dir.empty())
        {
            t_session(c,
                      dir);
        }

        c.close();
        D_TESTS_SFTP_CLIENT_CHECK( (!c.is_open()) &&
                                   (conn.is_open()) );
    }
    reap(server);
    ::rmdir(dir.c_str());

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
