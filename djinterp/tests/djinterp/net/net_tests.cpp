/*******************************************************************************
* djinterp [net]                                                   net_tests.cpp
*
* djinterp net foundation C++ tests.
*   Every test runs over in-memory connections: a C++ one defined here, and
* the scripted C connection of net_tests_sa_support.c, so each bridge is
* driven from both of its sides. Checks are tallied here, as the SMTP facade
* tests tally theirs.
*
*
* path:      /tests/djinterp/net/net_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.09.27
*******************************************************************************/
#include "./net_tests.hpp"  // corresponding header
// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf
#include <cstring>  // std::memcmp, std::memcpy, std::strcmp
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "./net_tests_sa_support.h"  // the scripted C connection


NS_DJINTERP
NS_TESTING

namespace net = ::djinterp::net;

NS_INTERNAL

    // pattern
    //   function: `_size` bytes of d_tests_net_pattern's sequence.
    static std::vector<net::byte>
    pattern(
        std::size_t  _size,
        unsigned int _seed
    )
    {
        std::vector<net::byte> data(_size);

        ::d_tests_net_pattern(data.data(),
                              data.size(),
                              _seed);

        return data;
    }

    // c_frames
    //   function: the bytes net.h's writer produces for `_payload` followed
    // by "hi", written through the scripted C connection.
    static std::vector<net::byte>
    c_frames(
        const std::vector<net::byte>& _payload
    )
    {
        std::vector<net::byte> wire(_payload.size() + 16u);
        d_tests_net_mock       mock = {};

        ::d_tests_net_mock_init(&mock,
                                false);
        ::d_tests_net_mock_output(&mock,
                                  wire.data(),
                                  wire.size());
        (void)::d_net_write_frame(&mock.base,
                                  _payload.data(),
                                  _payload.size(),
                                  D_NET_FRAME_MAX);
        (void)::d_net_write_frame(&mock.base,
                                  "hi",
                                  2u,
                                  D_NET_FRAME_MAX);
        wire.resize(mock.output_length);

        return wire;
    }

    // memory_connection
    //   class: a C++ connection over two byte vectors. Reads drain `input`
    // and writes append to `output`, each at most `_limit` bytes at a time,
    // 0 meaning no limit. It knows its peer, 198.51.100.7:443, and counts
    // shutdowns.
    class memory_connection : public net::connection
    {
    public:
        explicit memory_connection(
            std::size_t _limit
        )
            : net::connection(),
              input(),
              output(),
              shutdowns(0u),
              m_at(0u),
              m_limit(_limit),
              m_open(true)
        {}

        // feed -- replaces the bytes reads deliver, from their start
        void
        feed(
            const std::vector<net::byte>& _input
        )
        {
            input = _input;
            m_at  = 0u;

            return;
        }

        net::io_result
        read(
            void*       _buffer,
            std::size_t _size
        ) override
        {
            const std::size_t take = m_cut(_size,
                                           input.size() - m_at);

            // a closed connection reads nothing
            if (!m_open)
            {
                return net::io_result(0u,
                                      net::io_error::closed);
            }

            // an empty slice has no bytes to copy
            if (take > 0u)
            {
                std::memcpy(_buffer,
                            input.data() + m_at,
                            take);
            }

            m_at += take;

            return net::io_result(take,
                                  net::io_error::none);
        }

        net::io_result
        write(
            const void* _data,
            std::size_t _size
        ) override
        {
            const net::byte* const in   = static_cast<const net::byte*>(_data);
            const std::size_t      take = m_cut(_size,
                                                _size);

            // a closed connection writes nothing
            if (!m_open)
            {
                return net::io_result(0u,
                                      net::io_error::closed);
            }

            output.insert(output.end(),
                          in,
                          in + take);

            return net::io_result(take,
                                  net::io_error::none);
        }

        bool
        is_open() const override
        {
            return m_open;
        }

        void
        close() override
        {
            m_open = false;

            return;
        }

        net::io_error
        shutdown(
            net::shutdown_mode _mode
        ) override
        {
            (void)_mode;
            shutdowns += 1u;

            return net::io_error::none;
        }

        net::endpoint
        remote_endpoint() const override
        {
            return net::endpoint("198.51.100.7",
                                 443u);
        }

        std::vector<net::byte> input;      // the bytes reads deliver
        std::vector<net::byte> output;     // where writes land
        std::size_t            shutdowns;  // shutdown calls

    private:
        // m_cut -- a request cut to what is available and to the limit
        std::size_t
        m_cut(
            std::size_t _wanted,
            std::size_t _available
        ) const
        {
            const std::size_t take = (_wanted < _available) ? _wanted
                                                            : _available;

            return ( (m_limit != 0u) &&
                     (take > m_limit) ) ? m_limit
                                        : take;
        }

        std::size_t m_at;     // how many input bytes were read
        std::size_t m_limit;  // most bytes per read or write
        bool        m_open;   // cleared by close
    };

NS_END  // internal

// the trait, and from C++20 the concept, checked where they are resolved
static_assert(net::is_byte_stream<net::connection>::value,
              "the abstract connection is a byte stream");
static_assert(net::is_byte_stream<internal::memory_connection>::value,
              "a concrete backend is a byte stream");
static_assert(!net::is_byte_stream<std::string>::value,
              "a string is not a byte stream");
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
static_assert(net::byte_stream<net::c_connection>,
              "the concept agrees with the trait");
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

/*
tests_net_vocabulary
  Tests the following:
  - every io_error is its d_net_error, and describes itself identically
  - results, shutdown modes, and protocols convert to C and back
*/
bool
tests_net_vocabulary()
{
    const net::io_result  torn(3u,
                               net::io_error::connection_reset);
    const d_net_io_result c_torn = net::to_c(torn);
    const net::io_result  back   = net::from_c(c_torn);
    bool                  same   = true;
    bool                  result = true;

    // every enumerator is its C counterpart, and describes itself the same
    for (int i = 0; i <= static_cast<int>(D_NET_ERROR_UNKNOWN); ++i)
    {
        const d_net_error   c_error   = static_cast<d_net_error>(i);
        const net::io_error cpp_error = net::from_c(c_error);

        same = ( (same) &&
                 (static_cast<int>(cpp_error) == i) &&
                 (net::to_c(cpp_error) == c_error) &&
                 (std::strcmp(net::to_string(cpp_error),
                              ::d_net_error_string(c_error)) == 0) );
    }

    const bool results = ( (c_torn.count == 3u) &&
                           (c_torn.error == D_NET_ERROR_CONNECTION_RESET) &&
                           (back.count == 3u) &&
                           (back.error == net::io_error::connection_reset) &&
                           (!back.ok()) &&
                           (net::io_result().eof()) );
    const bool modes = ( (net::to_c(net::shutdown_mode::write) ==
                          D_NET_SHUTDOWN_WRITE) &&
                         (net::from_c(D_NET_SHUTDOWN_BOTH) ==
                          net::shutdown_mode::both) &&
                         (net::to_c(net::protocol::unix_socket) ==
                          D_NET_PROTOCOL_UNIX) &&
                         (net::from_c(D_NET_PROTOCOL_UDP) ==
                          net::protocol::udp) );

    result = internal::check(same,
                             "io_error is d_net_error, value and phrase") &&
             result;
    result = internal::check(results,
                             "results convert to C and back") && result;
    result = internal::check(modes,
                             "modes and protocols convert to C and back") &&
             result;

    return result;
}

/*
tests_net_endpoints
  Tests the following:
  - text parses and formats through net.h, zones and brackets included
  - malformed text, and a unix-domain protocol, leave the output alone
  - local() builds a path endpoint, which formats bare
  - endpoints convert to C records and back, and an overlong host is
    refused without touching the record
*/
bool
tests_net_endpoints()
{
    const net::endpoint local = net::endpoint::local("/run/d.sock");
    const std::string   longest(D_NET_HOST_MAX + 1u,
                                'a');
    net::endpoint       parsed;
    d_net_endpoint      record = {};
    bool                result = true;

    const bool zoned = ( (net::endpoint::parse("[fe80::1%e0]:443",
                                               parsed)) &&
                         (parsed.host == "fe80::1%e0") &&
                         (parsed.port == 443u) &&
                         (parsed.to_string() == "[fe80::1%e0]:443") );
    const bool plain = ( (net::endpoint::parse("example.com:80",
                                               parsed,
                                               net::protocol::udp)) &&
                         (parsed == net::endpoint("example.com",
                                                  80u,
                                                  net::protocol::udp)) &&
                         (parsed.to_string() == "example.com:80") );
    const bool refused = ( (!net::endpoint::parse("::1:80",
                                                  parsed)) &&
                           (!net::endpoint::parse(
                                "h:1",
                                parsed,
                                net::protocol::unix_socket)) &&
                           (!net::endpoint::parse("h:0",
                                                  parsed)) &&
                           (parsed.host == "example.com") );
    const bool paths = ( (local.is_unix()) &&
                         (local.port == 0u) &&
                         (local.to_string() == "/run/d.sock") &&
                         (local != parsed) );
    const bool bridged = ( (net::to_c(parsed,
                                      record) == net::io_error::none) &&
                           (std::strcmp(record.host,
                                        "example.com") == 0) &&
                           (net::from_c(record) == parsed) &&
                           (net::to_c(net::endpoint(longest,
                                                    1u),
                                      record) ==
                            net::io_error::address_invalid) &&
                           (net::from_c(record) == parsed) );

    result = internal::check( ( (zoned) &&
                                (plain) ),
                              "text parses and formats through net.h") &&
             result;
    result = internal::check(refused,
                             "malformed text leaves the output alone") &&
             result;
    result = internal::check(paths,
                             "local() builds a bare path endpoint") && result;
    result = internal::check(bridged,
                             "endpoints convert to C records and back") &&
             result;

    return result;
}

/*
tests_net_bridge_from_c
  Tests the following:
  - a c_connection over the scripted C connection serves the C++
    algorithms, across short reads
  - its shutdown and endpoint queries reach the C backend's operations
  - destroying it closes the C connection and releases it
*/
bool
tests_net_bridge_from_c()
{
    d_tests_net_mock mock   = {};
    char             text[5] = {};
    bool             result  = true;

    // room for what the C++ side writes
    unsigned char output[16] = {};

    ::d_tests_net_mock_init(&mock,
                            true);
    ::d_tests_net_mock_input(&mock,
                             "hello",
                             5u);
    ::d_tests_net_mock_output(&mock,
                              output,
                              sizeof(output));
    mock.read_limit = 2u;

    // the wrapper owns the C connection until the end of this block
    {
        net::c_connection wrapped(&mock.base);

        const bool read = ( (net::read_exactly(wrapped,
                                               text,
                                               sizeof(text)) ==
                             net::io_error::none) &&
                            (std::memcmp(text,
                                         "hello",
                                         5u) == 0) );
        const bool wrote = ( (net::write_all(wrapped,
                                             std::string("world")) ==
                              net::io_error::none) &&
                             (mock.output_length == 5u) );
        const bool state = ( (wrapped.is_open()) &&
                             (wrapped.shutdown(net::shutdown_mode::read) ==
                              net::io_error::none) &&
                             (mock.last_shutdown == D_NET_SHUTDOWN_READ) &&
                             (wrapped.remote_endpoint() ==
                              net::endpoint("192.0.2.1",
                                            80u)) &&
                             (wrapped.local_endpoint() == net::endpoint()) );

        result = internal::check( ( (read) &&
                                    (wrote) ),
                                  "a C connection serves C++ algorithms") &&
                 result;
        result = internal::check(state,
                                 "C++ queries reach the C operations") &&
                 result;
    }

    result = internal::check( ( (!mock.open) &&
                                (mock.destroys == 1u) ),
                              "destruction closes and releases it") &&
             result;

    return result;
}

/*
tests_net_bridge_to_c
  Tests the following:
  - an adapted C++ connection serves net.h's algorithms, across short reads
  - C endpoint queries report what it knows, and refuse what it does not
  - C shutdown and destroy reach its operations, closing it
*/
bool
tests_net_bridge_to_c()
{
    internal::memory_connection target(3u);
    net::c_connection_adapter   adapter(target);
    d_net_endpoint              address = {};
    unsigned char               got[4]  = {};
    std::size_t                 done    = 0u;
    bool                        result  = true;

    target.feed(internal::pattern(4u,
                                  7u));

    const bool read = ( (::d_net_read_exactly(adapter.get(),
                                              got,
                                              sizeof(got),
                                              &done) == D_NET_ERROR_NONE) &&
                        (done == 4u) &&
                        (std::memcmp(got,
                                     target.input.data(),
                                     4u) == 0) &&
                        (::d_net_write_all(adapter.get(),
                                           "xyz",
                                           3u,
                                           nullptr) == D_NET_ERROR_NONE) &&
                        (target.output.size() == 3u) );
    const bool known = ( (::d_net_connection_remote_endpoint(adapter.get(),
                                                             &address)) &&
                         (std::strcmp(address.host,
                                      "198.51.100.7") == 0) &&
                         (address.port == 443u) &&
                         (!::d_net_connection_local_endpoint(adapter.get(),
                                                             &address)) );
    const bool shut = ( (::d_net_connection_shutdown(adapter.get(),
                                                     D_NET_SHUTDOWN_WRITE) ==
                         D_NET_ERROR_NONE) &&
                        (target.shutdowns == 1u) );

    ::d_net_connection_destroy(adapter.get());

    result = internal::check(read,
                             "a C++ connection serves C algorithms") &&
             result;
    result = internal::check(known,
                             "C endpoint queries report what is known") &&
             result;
    result = internal::check( ( (shut) &&
                                (!target.is_open()) ),
                              "C shutdown and destroy reach it") && result;

    return result;
}

/*
tests_net_wire
  Tests the following:
  - frames the C++ writer produces are byte for byte the C writer's, both
    for a frame that fits one write and for one that does not
  - the C reader takes the C++ writer's frames, and the C++ reader the C
    writer's, into every kind of result buffer
*/
bool
tests_net_wire()
{
    const std::vector<net::byte> payload = internal::pattern(
                                               D_NET_IO_CHUNK + 300u,
                                               9u);
    const std::vector<net::byte> c_wire  = internal::c_frames(payload);
    internal::memory_connection  wire(0u);
    net::c_connection_adapter    adapter(wire);
    d_tests_net_mock             mock    = {};
    std::vector<net::byte>       back(payload.size());
    std::string                  text;
    std::size_t                  length = 0u;
    bool                         result = true;

    // the C++ writer writes the frames the C writer wrote
    const bool written = ( (net::write_frame(wire,
                                             payload.data(),
                                             payload.size()) ==
                            net::io_error::none) &&
                           (net::write_frame(wire,
                                             std::string("hi")) ==
                            net::io_error::none) );

    const bool identical = (wire.output == c_wire);

    // each reader takes the other language's frames
    wire.feed(wire.output);
    ::d_tests_net_mock_init(&mock,
                            false);
    ::d_tests_net_mock_input(&mock,
                             c_wire.data(),
                             c_wire.size());

    net::c_connection reader(&mock.base);

    const bool c_read = ( (::d_net_read_frame(adapter.get(),
                                              back.data(),
                                              back.size(),
                                              &length) == D_NET_ERROR_NONE) &&
                          (back == payload) );
    const bool cpp_read = ( (net::read_frame(reader,
                                             back) == net::io_error::none) &&
                            (back == payload) &&
                            (net::read_frame(reader,
                                             text) == net::io_error::none) &&
                            (text == "hi") );

    result = internal::check( ( (written) &&
                                (identical) ),
                              "C++ frames are the C writer's bytes") &&
             result;
    result = internal::check( ( (c_read) &&
                                (cpp_read) ),
                              "each reader takes the other's frames") &&
             result;

    return result;
}

/*
tests_net_streams
  Tests the following:
  - read_all fills a vector, a string, and from C++17 a byte_buffer, through
    short reads and through a connection reference
  - pump copies one stream into another
  - zero chunks and NULL buffers are refused
*/
bool
tests_net_streams()
{
    internal::memory_connection source(700u);
    internal::memory_connection sink(333u);
    net::connection&            base   = source;
    std::vector<net::byte>      all;
    std::string                 text;
    bool                        filled = true;
    bool                        result = true;

    source.feed(internal::pattern(10000u,
                                  3u));

    const bool vector_read = ( (net::read_all(base,
                                              all) == net::io_error::none) &&
                               (all == source.input) );

    source.feed(source.input);

    const bool string_read = ( (net::read_all(source,
                                              text) == net::io_error::none) &&
                               (text.size() == 10000u) );

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    net::bytes buffer;

    source.feed(source.input);
    filled = ( (net::read_all(source,
                              buffer) == net::io_error::none) &&
               (buffer.size() == 10000u) );
#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

    source.feed(source.input);

    const bool pumped = ( (net::pump(source,
                                     sink) == net::io_error::none) &&
                          (sink.output == source.input) );
    const bool edges = ( (net::read_all(source,
                                        all,
                                        0u) ==
                          net::io_error::invalid_argument) &&
                         (net::read_exactly(source,
                                            nullptr,
                                            1u) ==
                          net::io_error::invalid_argument) );

    result = internal::check( ( (vector_read) &&
                                (string_read) &&
                                (filled) ),
                              "read_all fills every kind of buffer") &&
             result;
    result = internal::check( ( (pumped) &&
                                (edges) ),
                              "pump copies; bad requests are refused") &&
             result;

    return result;
}

/*
tests_net_run_all
  Runs every test, reporting each, and the tally.
*/
bool
tests_net_run_all()
{
    struct entry
    {
        const char* name;
        bool        (*run)();
    };

    static const entry CASES[] =
    {
        { "vocabulary",         tests_net_vocabulary },
        { "endpoints",          tests_net_endpoints },
        { "bridge from C",      tests_net_bridge_from_c },
        { "bridge to C",        tests_net_bridge_to_c },
        { "wire compatibility", tests_net_wire },
        { "streams",            tests_net_streams },
        { "url vocabulary",     tests_net_url_vocabulary },
        { "url parsing",        tests_net_url_parse },
        { "url ownership",      tests_net_url_ownership },
        { "url resolution",     tests_net_url_resolution },
        { "url encoding",       tests_net_url_encoding },
        { "url endpoints",      tests_net_url_endpoints }
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
