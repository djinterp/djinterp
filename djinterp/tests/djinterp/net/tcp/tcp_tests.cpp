/*******************************************************************************
* djinterp [net]                                                   tcp_tests.cpp
*
* Tests of the TCP transport's C++ layer.
*   Every test opens its own sockets: an ephemeral loopback port, or a
* unix-domain path of its own. A std::thread plays the second party where one
* is needed.
*
*
* path:      /tests/djinterp/net/tcp/tcp_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "./tcp_tests.hpp"  // corresponding header
// std
#include <cerrno>       // errno, EAGAIN, EBADF, ECONNREFUSED
#include <chrono>       // std::chrono::milliseconds
#include <cstddef>      // std::size_t
#include <cstdio>       // std::printf
#include <cstring>      // std::memcmp
#include <functional>   // std::cref, std::ref
#include <string>       // std::string, std::to_string
#include <thread>       // std::thread, std::this_thread
#include <type_traits>  // std::is_same, std::is_nothrow_move_constructible
#include <utility>      // std::declval, std::move
#include <vector>       // std::vector
// posix
#include <fcntl.h>      // fcntl, F_GETFD
#include <sys/stat.h>   // stat
#include <unistd.h>     // dup, getpid


NS_DJINTERP
NS_TESTING

namespace net = ::djinterp::net;

// the shapes the consumers rely on, checked where they are resolved
static_assert(std::is_same<decltype(std::declval<net::tcp_connector&>()
                                        .connect(std::declval<
                                            const net::endpoint&>())),
                           net::open_result>::value,
              "tcp_connector::connect must return open_result, as "
              "client.hpp's connector concept requires");
static_assert(std::is_same<decltype(std::declval<net::tcp_acceptor&>()
                                        .accept()),
                           net::open_result>::value,
              "tcp_acceptor::accept must return open_result");
static_assert(std::is_same<net::socket_connection::native_handle_type,
                           int>::value,
              "under BSD sockets the handle is an int, as reactor.hpp "
              "stores it");
static_assert(std::is_nothrow_move_constructible<
                  net::socket_connection>::value,
              "socket_connection must move without throwing");
static_assert(std::is_nothrow_move_constructible<net::tcp_acceptor>::value,
              "tcp_acceptor must move without throwing");
static_assert(!std::is_copy_constructible<net::socket_connection>::value,
              "socket_connection owns its socket and must not copy");

NS_INTERNAL

    // tally
    //   struct: checks run and failed across the suite.
    struct tally
    {
        std::size_t checks   = 0u;
        std::size_t failures = 0u;
    };

    // pair
    //   struct: an acceptor on a loopback port, and a connection each way.
    struct pair
    {
        net::tcp_acceptor acceptor;
        net::open_result  client;
        net::open_result  server;
    };

    // counts
    //   function: the suite's tally.
    static tally&
    counts() noexcept
    {
        static tally instance;

        return instance;
    }

    // check
    //   function: records one check, printing it if it failed.
    static bool
    check(
        bool        _condition,
        const char* _message
    ) noexcept
    {
        counts().checks += 1u;

        // only failures are worth a line
        if (!_condition)
        {
            counts().failures += 1u;
            std::printf("    FAIL %s\n",
                        _message);
        }

        return _condition;
    }

    // loopback
    //   function: 127.0.0.1 on an ephemeral port.
    static net::endpoint
    loopback()
    {
        return net::endpoint("127.0.0.1",
                             0u);
    }

    // open_pair
    //   function: binds, connects, and accepts on one thread; the connect
    // completes into the backlog before accept is called.
    static bool
    open_pair(
        pair& _pair
    )
    {
        net::tcp_connector connector;

        // the acceptor first, then one connection each way
        if (_pair.acceptor.bind(loopback()) != net::io_error::none)
        {
            return false;
        }

        _pair.client = connector.connect(_pair.acceptor.local_endpoint());
        _pair.server = _pair.acceptor.accept();

        return ( (_pair.client.ok()) &&
                 (_pair.server.ok()) );
    }

    // exchange
    //   function: writes a text on one connection and reads it whole on the
    // other, through net.hpp's templates.
    static bool
    exchange(
        net::connection&   _from,
        net::connection&   _to,
        const std::string& _text
    )
    {
        std::string arrived(_text.size(),
                            '\0');

        return ( (net::write_all(_from,
                                 _text) == net::io_error::none) &&
                 (net::read_exactly(_to,
                                    &arrived[0],
                                    arrived.size()) ==
                  net::io_error::none) &&
                 (arrived == _text) );
    }

    // descriptor_closed
    //   function: whether fcntl calls a descriptor bad.
    static bool
    descriptor_closed(
        int _fd
    )
    {
        return ( (::fcntl(_fd,
                          F_GETFD) < 0) &&
                 (errno == EBADF) );
    }

    // FRAME_SIZES
    //   constant: the frames the stream test sends, the empty one included.
    static const std::size_t FRAME_SIZES[] = { 0u, 5u, 70000u };

    // write_streams
    //   function: the stream test's writer: every byte, then one frame per
    // FRAME_SIZES entry, each a prefix of the same bytes, then a shutdown.
    static void
    write_streams(
        net::connection&                  _out,
        const std::vector<unsigned char>& _bytes
    )
    {
        (void)net::write_all(_out,
                             _bytes.data(),
                             _bytes.size());

        // each frame a prefix of the same bytes
        for (const std::size_t size : FRAME_SIZES)
        {
            (void)net::write_frame(_out,
                                   _bytes.data(),
                                   size);
        }

        (void)_out.shutdown(net::shutdown_mode::write);

        return;
    }

    // read_frames
    //   function: reads one frame per FRAME_SIZES entry, each checked against
    // the prefix sent, then expects the stream to have ended.
    static bool
    read_frames(
        net::connection&                  _in,
        const std::vector<unsigned char>& _sent
    )
    {
        std::vector<unsigned char> frame;
        bool                       framed = true;

        // each frame as the writer sent it; an empty one has no bytes
        for (const std::size_t size : FRAME_SIZES)
        {
            framed = ( (framed) &&
                       (net::read_frame(_in,
                                        frame) == net::io_error::none) &&
                       (frame.size() == size) &&
                       ( (size == 0u) ||
                         (std::memcmp(frame.data(),
                                      _sent.data(),
                                      size) == 0) ) );
        }

        return ( (framed) &&
                 (net::read_frame(_in,
                                  frame) == net::io_error::closed) );
    }

NS_END  // internal


/*
tests_tcp_options
  Tests the following:
  - socket_options has the replaced tcp.hpp's defaults
  - each field reaches the C options unchanged
*/
bool
tests_tcp_options()
{
    const net::socket_options defaults;
    net::socket_options       tuned;
    bool                      result = true;

    tuned.no_delay           = true;
    tuned.reuse_address      = false;
    tuned.reuse_port         = true;
    tuned.connect_timeout_ms = 250;
    tuned.listen_backlog     = 7;

    const ::d_tcp_options converted = net::internal::to_c_options(tuned);

    result = internal::check( (!defaults.no_delay) &&
                              (defaults.reuse_address) &&
                              (!defaults.reuse_port) &&
                              (defaults.connect_timeout_ms == 0) &&
                              (defaults.listen_backlog == 128),
                             "socket_options keeps tcp.hpp's defaults") &&
             result;
    result = internal::check( (converted.no_delay) &&
                              (!converted.reuse_address) &&
                              (converted.reuse_port) &&
                              (converted.connect_timeout_ms == 250) &&
                              (converted.listen_backlog == 7),
                             "every option reaches tcp.h") && result;
    result = internal::check( (net::internal::from_errno(0) ==
                               net::io_error::none) &&
                              (net::internal::from_errno(ECONNREFUSED) ==
                               net::io_error::connection_refused) &&
                              (net::internal::from_errno(EAGAIN) ==
                               net::io_error::would_block),
                             "the kept from_errno maps as tcp.h does") &&
             result;

    return result;
}

/*
tests_tcp_loopback
  Tests the following:
  - an acceptor bound to port 0 reports its ephemeral port
  - a connector reaches it and the acceptor accepts; both are open
  - the endpoints agree across the connection
  - bytes cross in both directions
*/
bool
tests_tcp_loopback()
{
    internal::pair pair;
    bool           result = true;

    const bool          opened = internal::open_pair(pair);
    const net::endpoint bound  = pair.acceptor.local_endpoint();

    result = internal::check( (opened) &&
                              (bound.host == "127.0.0.1") &&
                              (bound.port != 0u) &&
                              (pair.client.get()->remote_endpoint() ==
                               bound) &&
                              (pair.client.get()->local_endpoint() ==
                               pair.server.get()->remote_endpoint()),
                             "the endpoints agree across the connection") &&
             result;
    result = internal::check( (opened) &&
                              (internal::exchange(*pair.client.get(),
                                                  *pair.server.get(),
                                                  "ping")) &&
                              (internal::exchange(*pair.server.get(),
                                                  *pair.client.get(),
                                                  "pong")),
                             "bytes cross in both directions") && result;

    return result;
}

/*
tests_tcp_streams
  Tests the following:
  - write_all and read_exactly move 1 MiB, the writer on its own thread
  - write_frame and read_frame carry frames of 0, 5, and 70000 bytes
  - after the writer's shutdown, the next frame reads as closed
*/
bool
tests_tcp_streams()
{
    std::vector<unsigned char> sent(1024u * 1024u);
    std::vector<unsigned char> arrived(sent.size());
    internal::pair             pair;
    bool                       result = true;

    // bytes that differ at every short period
    for (std::size_t i = 0u; i < sent.size(); ++i)
    {
        sent[i] = static_cast<unsigned char>((i * 31u + 7u) & 0xFFu);
    }

    // without a connection there is nothing to stream
    if (!internal::open_pair(pair))
    {
        return internal::check(false,
                               "a loopback pair opens for streaming");
    }

    std::thread writer(internal::write_streams,
                       std::ref(*pair.client.get()),
                       std::cref(sent));
    const bool  bulk   = ( (net::read_exactly(*pair.server.get(),
                                              arrived.data(),
                                              arrived.size()) ==
                            net::io_error::none) &&
                           (arrived == sent) );
    const bool  framed = internal::read_frames(*pair.server.get(),
                                               sent);

    writer.join();
    result = internal::check(bulk,
                             "1 MiB arrives intact") && result;
    result = internal::check(framed,
                             "frames arrive whole; then the stream ends") &&
             result;

    return result;
}

/*
tests_tcp_moves
  Tests the following:
  - a moved connection keeps the socket; the source is closed, handle -1
  - move assignment closes the target's own socket first
  - moving an acceptor moves its listener, which goes on accepting
*/
bool
tests_tcp_moves()
{
    internal::pair pair;
    bool           result = true;

    const bool opened = internal::open_pair(pair);
    auto&      client = *static_cast<net::socket_connection*>(
                             pair.client.get());
    const int  native = client.native_handle();

    net::socket_connection moved(std::move(client));
    net::socket_connection target(::dup(native));
    const int              spare = target.native_handle();

    result = internal::check( (opened) &&
                              (client.native_handle() == -1) &&
                              (!client.is_open()) &&
                              (moved.native_handle() == native) &&
                              (internal::exchange(moved,
                                                  *pair.server.get(),
                                                  "moved")),
                             "a moved connection keeps its socket") && result;
    target = std::move(moved);
    result = internal::check( (internal::descriptor_closed(spare)) &&
                              (target.native_handle() == native) &&
                              (!moved.is_open()),
                             "move assignment closes the old socket") &&
             result;

    const int         listening = pair.acceptor.native_handle();
    net::tcp_acceptor second(std::move(pair.acceptor));
    net::tcp_connector connector;

    net::open_result again = connector.connect(second.local_endpoint());

    result = internal::check( (!pair.acceptor.is_open()) &&
                              (second.native_handle() == listening) &&
                              (again.ok()) &&
                              (second.accept().ok()),
                             "a moved acceptor goes on accepting") && result;

    return result;
}

/*
tests_tcp_adopt
  Tests the following:
  - socket_connection adopts a connected descriptor and records its
    endpoints, as reactor.hpp constructs it
  - an invalid descriptor leaves it closed, and reads report closed
  - the protocol given is recorded on the endpoints, udp included
*/
bool
tests_tcp_adopt()
{
    internal::pair pair;
    bool           result = true;

    const bool opened = internal::open_pair(pair);
    auto&      server = *static_cast<net::socket_connection*>(
                             pair.server.get());
    const int  raw    = static_cast<int>(
                            ::d_tcp_connection_release(&server.c_view()));

    net::socket_connection adopted(raw,
                                   net::protocol::tcp);
    net::socket_connection tagged(::dup(raw),
                                  net::protocol::udp);
    net::socket_connection none(-1);
    char                   byte = '\0';

    result = internal::check( (opened) &&
                              (adopted.is_open()) &&
                              (adopted.remote_endpoint() ==
                               pair.client.get()->local_endpoint()) &&
                              (internal::exchange(*pair.client.get(),
                                                  adopted,
                                                  "adopted")),
                             "an adopted descriptor works") && result;
    result = internal::check( (!none.is_open()) &&
                              (none.read(&byte,
                                         1u).error == net::io_error::closed) &&
                              (tagged.remote_endpoint().proto ==
                               net::protocol::udp),
                             "invalid handles close; udp is recorded") &&
             result;

    return result;
}

/*
tests_tcp_accept_wake
  Tests the following:
  - an accept blocked on one thread returns io_error::closed when another
    closes the acceptor
  - accepting afterwards reports closed at once
*/
bool
tests_tcp_accept_wake()
{
    net::tcp_acceptor acceptor;
    net::io_error     outcome = net::io_error::unknown;
    bool              result  = true;

    const bool  bound = (acceptor.bind(internal::loopback()) ==
                         net::io_error::none);
    std::thread waiter([&acceptor, &outcome]()
                       {
                           outcome = acceptor.accept().error;
                       });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    acceptor.close();
    waiter.join();
    result = internal::check( (bound) &&
                              (outcome == net::io_error::closed),
                             "close wakes the blocked accept") && result;
    result = internal::check(acceptor.accept().error == net::io_error::closed,
                             "a closed acceptor refuses at once") && result;

    return result;
}

/*
tests_tcp_unix
  Tests the following:
  - an acceptor binds a unix-domain path, creating it
  - a connector reaches it by path; bytes cross both ways
  - the client records the path as its peer
  - closing the acceptor removes the path
*/
bool
tests_tcp_unix()
{
    const std::string   path  = "/tmp/djinterp_tcp_cpp_" +
                                std::to_string(::getpid()) + ".sock";
    const net::endpoint where = net::endpoint::local(path);
    struct stat         status;
    net::tcp_acceptor   acceptor;
    net::tcp_connector  connector;
    bool                result = true;

    const bool       bound  = ( (acceptor.bind(where) == net::io_error::none) &&
                                (::stat(path.c_str(),
                                        &status) == 0) );
    net::open_result client = connector.connect(where);
    net::open_result server = acceptor.accept();

    result = internal::check( (bound) &&
                              (client.ok()) &&
                              (server.ok()) &&
                              (client.get()->remote_endpoint() == where) &&
                              (internal::exchange(*client.get(),
                                                  *server.get(),
                                                  "by path")) &&
                              (internal::exchange(*server.get(),
                                                  *client.get(),
                                                  "and back")),
                             "a unix-domain connection carries bytes") &&
             result;
    acceptor.close();
    result = internal::check(::stat(path.c_str(),
                                    &status) != 0,
                             "closing the acceptor removes the path") &&
             result;

    return result;
}

/*
tests_tcp_errors
  Tests the following:
  - a port nothing listens on refuses, blocking or with a timeout
  - an endpoint C cannot hold, and a UDP one, are refused
  - binding a held port reports address_in_use
*/
bool
tests_tcp_errors()
{
    net::tcp_acceptor  held;
    net::tcp_acceptor  second;
    net::tcp_connector connector;
    bool               result = true;

    const bool          bound = (held.bind(internal::loopback()) ==
                                 net::io_error::none);
    const net::endpoint where = held.local_endpoint();
    const bool          taken = (second.bind(where) ==
                                 net::io_error::address_in_use);

    held.close();

    const bool refused = (connector.connect(where).error ==
                          net::io_error::connection_refused);

    connector.options().connect_timeout_ms = 1500;

    const bool bounded = (connector.connect(where).error ==
                          net::io_error::connection_refused);
    const net::endpoint huge(std::string(300u,
                                         'h'),
                             80u);
    const net::endpoint datagram("127.0.0.1",
                                 9u,
                                 net::protocol::udp);

    result = internal::check( (bound) &&
                              (taken) &&
                              (refused) &&
                              (bounded),
                             "refusals and a held port are reported") &&
             result;
    result = internal::check( (connector.connect(huge).error ==
                               net::io_error::address_invalid) &&
                              (second.bind(huge) ==
                               net::io_error::address_invalid) &&
                              (connector.connect(datagram).error ==
                               net::io_error::invalid_argument),
                             "unholdable and UDP endpoints are refused") &&
             result;

    return result;
}

/*
tests_tcp_run_all
  Runs every test, reporting each, and the tally.
*/
bool
tests_tcp_run_all()
{
    struct entry
    {
        const char* name;
        bool        (*run)();
    };

    static const entry CASES[] =
    {
        { "options",     tests_tcp_options },
        { "loopback",    tests_tcp_loopback },
        { "streams",     tests_tcp_streams },
        { "moves",       tests_tcp_moves },
        { "adoption",    tests_tcp_adopt },
        { "accept wake", tests_tcp_accept_wake },
        { "unix-domain", tests_tcp_unix },
        { "errors",      tests_tcp_errors }
    };
    bool all = true;

    for (const entry& test : CASES)
    {
        const bool passed = test.run();

        all = ( (all) &&
                (passed) );
        std::printf("  [%s] %s\n",
                    (passed) ? "PASS" : "FAIL",
                    test.name);
    }

    std::printf("%zu/%zu checks\n",
                internal::counts().checks - internal::counts().failures,
                internal::counts().checks);

    return all;
}

NS_END  // testing
NS_END  // djinterp
