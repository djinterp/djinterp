// C++ tests for net/sftp/sftp_client.hpp: the tiered interface over the C
// client, against OpenSSH's own sftp-server, run as a child process whose
// stdin and stdout carry the SFTP stream: a real server, reached through
// client::open(transport), with no SSH session in between. POSIX only; the
// suite skips where sftp-server is not installed. The core compiles at
// C++98; C++11 adds lambdas and C++14 re_std::expected.
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../../../../inc/djinterp/net/sftp/sftp.hpp"
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <type_traits>
#endif

namespace sftp = djinterp::net::sftp;

static int g_fail   = 0;
static int g_checks = 0;
#define CHECK(c) do { g_checks++; if (!(c)) { g_fail++; std::printf("FAIL %d: %s\n", __LINE__, #c); } } while (0)

// the child server, and the pipes that are its stdin and stdout
struct child { pid_t pid; int to; int from; };

static enum d_ssh_status pipe_read(void* _context, void* _buffer, size_t _capacity, size_t* _out)
{
    const ssize_t n = ::read(static_cast<child*>(_context)->from, _buffer, _capacity);
    if (n <= 0) return (n == 0) ? D_SSH_ERR_CLOSED : D_SSH_ERR_IO;
    *_out = (size_t)n;
    return D_SSH_OK;
}

static enum d_ssh_status pipe_write(void* _context, const void* _data, size_t _size)
{
    const char* p = static_cast<const char*>(_data);
    while (_size > 0u)
    {
        const ssize_t n = ::write(static_cast<child*>(_context)->to, p, _size);
        if (n <= 0) return D_SSH_ERR_IO;
        p += n; _size -= (size_t)n;
    }
    return D_SSH_OK;
}

static const char* SERVER = "/usr/lib/openssh/sftp-server";

static bool spawn(child& _child)
{
    int in[2], out[2];
    if (::pipe(in) != 0 || ::pipe(out) != 0) return false;
    _child.pid = ::fork();
    if (_child.pid == 0)
    {
        ::dup2(in[0], 0); ::dup2(out[1], 1);
        ::close(in[0]); ::close(in[1]); ::close(out[0]); ::close(out[1]);
        ::execl(SERVER, SERVER, (char*)NULL);
        ::_exit(127);
    }
    ::close(in[0]); ::close(out[1]);
    _child.to = in[1]; _child.from = out[0];
    return _child.pid > 0;
}

static void reap(child& _child)
{
    ::close(_child.to); ::close(_child.from);
    int status = 0;
    ::waitpid(_child.pid, &status, 0);
}

// a functor sink: counts and compares against the expected bytes
struct compare
{
    const std::string* expected; std::size_t at; bool same;
    explicit compare(const std::string& _expected) : expected(&_expected), at(0u), same(true) {}
    sftp::error operator()(const char* _data, std::size_t _size)
    {
        same = same && (at + _size <= expected->size()) && std::memcmp(expected->data() + at, _data, _size) == 0;
        at  += _size;
        return sftp::error::none;
    }
};

// a directory walker that looks for one name
struct finder
{
    const char* wanted; bool found; int entries;
    explicit finder(const char* _wanted) : wanted(_wanted), found(false), entries(0) {}
    sftp::error operator()(const sftp::name& _entry)
    {
        entries++;
        found = found || (_entry.filename.length == std::strlen(wanted) &&
                          std::memcmp(_entry.filename.data, wanted, _entry.filename.length) == 0);
        return sftp::error::none;
    }
};

static void t_session(sftp::client& c, const std::string& _dir)
{
    std::string where;
    CHECK(c.realpath(_dir.c_str(), where) == sftp::error::none && where == _dir);

    // a whole file up and back: 300 KB crosses many pipelined chunks
    std::string body(300000u, '\0');
    for (std::size_t i = 0u; i < body.size(); i++) body[i] = (char)(i * 131u + 17u);
    const std::string file = _dir + "/data.bin";
    CHECK(c.put(file.c_str(), body) == sftp::error::none);
    std::string back;
    CHECK(c.get(file.c_str(), back) == sftp::error::none && back == body);
    compare cmp(body);
    CHECK(c.get(file.c_str(), cmp) == sftp::error::none && cmp.same && cmp.at == body.size());

    sftp::attributes a;
    CHECK(c.stat(file.c_str(), a) == sftp::error::none);
    CHECK((a.flags & sftp::ATTR_SIZE) != 0u && a.size == 300000u);

    // an open file: write at an offset, read it back, fstat, close
    sftp::handle h;
    const std::string small = _dir + "/small.txt";
    CHECK(c.open_file(small.c_str(), sftp::OPEN_WRITE | sftp::OPEN_CREATE | sftp::OPEN_TRUNCATE, NULL, h) == sftp::error::none);
    CHECK(c.write(h, 0u, "hello, sftp", 11u) == sftp::error::none);
    CHECK(c.close_file(h) == sftp::error::none);
    CHECK(c.open_file(small.c_str(), sftp::OPEN_READ, NULL, h) == sftp::error::none);
    char        buffer[32];
    std::size_t got = 0u;
    CHECK(c.read(h, 7u, buffer, sizeof buffer, got) == sftp::error::none && got == 4u && std::memcmp(buffer, "sftp", 4u) == 0);
    CHECK(c.read(h, 11u, buffer, sizeof buffer, got) == sftp::error::eof);
    CHECK(c.fstat(h, a) == sftp::error::none && a.size == 11u);
    CHECK(c.close_file(h) == sftp::error::none);

    // names: rename with replace, links, directories, listings
    const std::string moved = _dir + "/moved.txt";
    CHECK(c.rename(small.c_str(), moved.c_str(), true) == sftp::error::none);
    const std::string link = _dir + "/link";
    CHECK(c.symlink("moved.txt", link.c_str()) == sftp::error::none);
    std::string target;
    CHECK(c.readlink(link.c_str(), target) == sftp::error::none && target == "moved.txt");
    CHECK(c.lstat(link.c_str(), a) == sftp::error::none && S_ISLNK(a.permissions));
    const std::string sub = _dir + "/sub";
    CHECK(c.mkdir(sub.c_str()) == sftp::error::none);
    finder f("sub");
    CHECK(c.list(_dir.c_str(), f) == sftp::error::none && f.found && f.entries >= 6);

    // errors are values, switched on and described
    const sftp::error e = c.stat((_dir + "/missing").c_str(), a);
    CHECK(e == sftp::error::no_such_file && c.status_code() == 2u);
    int seen = 0;
    switch (e)
    {
        case sftp::error::no_such_file:      seen = 1; break;
        case sftp::error::permission_denied: seen = 2; break;
        default:                             seen = 3; break;
    }
    CHECK(seen == 1);
    CHECK(std::strcmp(sftp::error_string(sftp::error::eof), d_sftp_error_string(D_SFTP_ERROR_EOF)) == 0);
    CHECK(sftp::to_c(sftp::from_c(D_SFTP_ERROR_TOO_LARGE)) == D_SFTP_ERROR_TOO_LARGE);

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    static_assert(!std::is_copy_constructible<sftp::client>::value, "a client must not be copyable");
    static_assert(!std::is_convertible<sftp::error, int>::value, "from C++11 an error is an enum class");
#endif
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    // the value-returning forms
    auto stat = c.stat(file.c_str());
    CHECK(stat.has_value() && (*stat).size == 300000u);
    auto missing = c.stat((_dir + "/missing").c_str());
    CHECK(!missing.has_value() && missing.error() == sftp::error::no_such_file);
    auto real = c.realpath(_dir.c_str());
    CHECK(real.has_value() && *real == _dir);
    auto opened = c.open_file(moved.c_str(), sftp::OPEN_READ);
    CHECK(opened.has_value());
    if (opened.has_value())
    {
        auto count = c.read(*opened, 0u, buffer, sizeof buffer);
        CHECK(count.has_value() && *count == 11u);
        CHECK(c.close_file(*opened) == sftp::error::none);
    }
    auto pointed = c.readlink(link.c_str());
    CHECK(pointed.has_value() && *pointed == "moved.txt");
#endif
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    // lambdas written in place
    std::size_t total = 0u;
    CHECK(c.get(file.c_str(), [&total](const char*, std::size_t _size) { total += _size; return sftp::error::none; })
          == sftp::error::none && total == 300000u);
    int entries = 0;
    CHECK(c.list(_dir.c_str(), [&entries](const sftp::name&) { entries++; return sftp::error::none; })
          == sftp::error::none && entries >= 6);
#endif

    CHECK(c.remove(link.c_str()) == sftp::error::none);
    CHECK(c.remove(moved.c_str()) == sftp::error::none);
    CHECK(c.remove(file.c_str()) == sftp::error::none);
    CHECK(c.rmdir(sub.c_str()) == sftp::error::none);
}

int main()
{
    if (::access(SERVER, X_OK) != 0)
    {
        std::printf("skipped: %s not found\n", SERVER);
        return 0;
    }
    ::signal(SIGPIPE, SIG_IGN);

    char        pattern[] = "/tmp/sftp_cpp_XXXXXX";
    const char* made      = ::mkdtemp(pattern);
    CHECK(made != NULL);
    char resolved[4096];
    const std::string dir = (made && ::realpath(made, resolved)) ? resolved : "";

    child server;
    CHECK(spawn(server));
    {
        sftp::client c;
        CHECK(!c.is_open());
        const d_sftp_transport transport = { pipe_read, pipe_write, &server };
        CHECK(c.open(transport) == sftp::error::none);
        CHECK(c.is_open() && c.version() == 3u);
        if (c.is_open() && !dir.empty()) t_session(c, dir);
        c.close();
        CHECK(!c.is_open());
    }
    reap(server);
    ::rmdir(dir.c_str());

    std::printf("%d checks, %d failures (C++%ld)\n", g_checks, g_fail, (long)(__cplusplus / 100 % 100));
    return g_fail != 0;
}
