/*******************************************************************************
* djinterp [net]                                               claude_tests.cpp
*
* Tests of the Claude clients, net/api/claude.hpp and claude_api.hpp.
*   No call reaches Anthropic: a stand-in of net's TCP layer answers on a
* loopback port, on a thread of dmutex.h's, with the shapes the Messages and
* Message Batches APIs document -- a message, an error, an event stream, and
* a batch that is in progress, then ended, then has results. It keeps the
* last request it was sent, for the tests to read back. The suite runs from
* C++98. Link curl.c, net.c, tcp.c and dmutex.c, and -lcurl.
*
*
* path:      /tests/djinterp/net/api/claude_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/
#include "./claude_tests.hpp"  // corresponding header
// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf, std::sprintf
#include <cstdlib>  // std::strtoul
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../../../../inc/djinterp/c/sync/dmutex.h"  // d_thread_create
#include "../../../../inc/djinterp/net/tcp/tcp.hpp"  // tcp_acceptor


NS_DJINTERP
NS_TESTING

namespace net    = ::djinterp::net;
namespace claude = ::djinterp::net::vendor::claude;
namespace batch  = ::djinterp::net::vendor::claude::batch;

NS_INTERNAL

    // tally
    //   struct: checks run and failed across the suite.
    struct tally
    {
        tally()
            : checks(0u),
              failures(0u)
        {}

        std::size_t checks;
        std::size_t failures;
    };

    // counts
    //   function: the suite's tally.
    static tally&
    counts() D_NOEXCEPT
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
    ) D_NOEXCEPT
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

    // decimal
    //   function: an unsigned number as text.
    static std::string
    decimal(
        unsigned long _number
    )
    {
        char      digits[32];
        const int length = std::sprintf(digits,
                                        "%lu",
                                        _number);

        return std::string(digits,
                           static_cast<std::size_t>(length));
    }

    // stand_in
    //   struct: the loopback API: an acceptor, how many connections to
    // serve, how often the batch has been retrieved, and the last request.
    struct stand_in
    {
        stand_in()
            : acceptor(),
              serving(0u),
              retrieved(0u),
              last(),
              thread(),
              started(false)
        {}

        net::tcp_acceptor acceptor;
        std::size_t       serving;
        std::size_t       retrieved;
        std::string       last;
        d_thread_t        thread;
        bool              started;
    };

    // base
    //   function: the stand-in's URL.
    static std::string
    base(
        const stand_in& _api
    )
    {
        return "http://127.0.0.1:" +
               decimal(_api.acceptor.local_endpoint().port);
    }

    // reply
    //   function: an HTTP/1.1 response with a body, closing the connection.
    static std::string
    reply(
        const char*        _status,
        const char*        _type,
        const std::string& _body
    )
    {
        return std::string("HTTP/1.1 ") + _status + "\r\n"         +
               "Content-Type: " + _type + "\r\n"                   +
               "Content-Length: " + decimal(_body.size()) + "\r\n" +
               "Connection: close\r\n\r\n" + _body;
    }

    // MESSAGE, FAILURE, EVENTS
    //   constant: a Messages API reply, an error reply, and an event stream.
    static const char* const MESSAGE =
        "{\"id\":\"msg_1\",\"type\":\"message\",\"role\":\"assistant\","
        "\"model\":\"claude-test\",\"content\":[{\"type\":\"text\","
        "\"text\":\"Hello\"},{\"type\":\"text\",\"text\":\" world\"}],"
        "\"stop_reason\":\"end_turn\"}";

    static const char* const FAILURE =
        "{\"type\":\"error\",\"error\":{\"type\":\"invalid_request_error\","
        "\"message\":\"no such model\"}}";

    static const char* const EVENTS =
        "event: message_start\ndata: {\"type\":\"message_start\"}\n\n"
        "event: content_block_delta\n"
        "data: {\"delta\":{\"text\":\"Hel\"}}\n\n"
        "event: content_block_delta\r\n"
        "data: {\"delta\":{\"text\":\"lo\"}}\r\n\r\n"
        "event: message_stop\ndata: {\"type\":\"message_stop\"}\n\n";

    // answer
    //   function: the stand-in's answer to one request. It runs past 75
    // lines because it answers every request the tests send, one branch
    // each, as the guides allow a dispatch.
    static std::string
    answer(
        stand_in&          _api,
        const std::string& _request
    )
    {
        const std::string head = _request.substr(0u,
                                                 _request.find("\r\n"));

        // the Messages API: a stream, an error, or a message
        if (head.find("POST /v1/messages ") == 0u)
        {
            // a streamed call
            if (_request.find("\"stream\":true") != std::string::npos)
            {
                return reply("200 OK",
                             "text/event-stream",
                             EVENTS);
            }

            // a model the stand-in refuses
            if (_request.find("\"model\":\"bad\"") != std::string::npos)
            {
                return reply("400 Bad Request",
                             "application/json",
                             FAILURE);
            }

            return reply("200 OK",
                         "application/json",
                         MESSAGE);
        }

        // the batch is created in progress
        if (head.find("POST /v1/messages/batches ") == 0u)
        {
            return reply("200 OK",
                         "application/json",
                         "{\"id\":\"msgbatch_1\",\"type\":\"message_batch\","
                         "\"processing_status\":\"in_progress\"}");
        }

        // retrieved once in progress, then ended with results
        if (head.find("GET /v1/messages/batches/msgbatch_1 ") == 0u)
        {
            _api.retrieved += 1u;

            // the first look still finds it running
            if (_api.retrieved < 2u)
            {
                return reply("200 OK",
                             "application/json",
                             "{\"id\":\"msgbatch_1\","
                             "\"processing_status\":\"in_progress\"}");
            }

            return reply("200 OK",
                         "application/json",
                         "{\"id\":\"msgbatch_1\","
                         "\"processing_status\":\"ended\","
                         "\"request_counts\":{\"processing\":0,"
                         "\"succeeded\":1,\"errored\":1,\"canceled\":0,"
                         "\"expired\":0},\"results_url\":\"" + base(_api) +
                         "/results/msgbatch_1\"}");
        }

        // the results, one JSON line each
        if (head.find("GET /results/msgbatch_1 ") == 0u)
        {
            return reply("200 OK",
                         "application/binary",
                         "{\"custom_id\":\"first\",\"result\":{\"type\":"
                         "\"succeeded\",\"message\":{\"id\":\"msg_9\","
                         "\"model\":\"claude-test\",\"content\":[{\"type\":"
                         "\"text\",\"text\":\"done\"}],\"stop_reason\":"
                         "\"end_turn\"}}}\n"
                         "{\"custom_id\":\"second\",\"result\":{\"type\":"
                         "\"errored\",\"error\":{\"type\":\"error\","
                         "\"error\":{\"type\":\"invalid_request_error\","
                         "\"message\":\"too long\"}}}}\n");
        }

        return reply("404 Not Found",
                     "text/plain",
                     "missing");
    }

    // read_request
    //   function: one request's head and its Content-Length body.
    static std::string
    read_request(
        net::connection& _connection
    )
    {
        std::string request;
        char        chunk[1024];
        std::size_t needed = std::string::npos;

        // until the head, and then the body it announces, are in
        while (request.size() < needed)
        {
            const net::io_result got = _connection.read(chunk,
                                                        sizeof(chunk));

            // the client went away
            if ( (got.error != net::io_error::none) ||
                 (got.count == 0u) )
            {
                break;
            }

            request.append(chunk,
                           got.count);

            const std::size_t end = request.find("\r\n\r\n");

            // the head is in: how much more is there
            if (end != std::string::npos)
            {
                const std::size_t field = request.find("Content-Length: ");
                const std::size_t body  =
                    (field != std::string::npos)
                        ? std::strtoul(request.c_str() + field + 16u,
                                       D_NULLPTR,
                                       10)
                        : 0u;

                needed = end + 4u + body;
            }
        }

        return request;
    }

    // serve_main
    //   function: the stand-in's thread: one request per connection.
    static d_thread_result_t
    serve_main(
        void* _api
    )
    {
        stand_in* const api = static_cast<stand_in*>(_api);

        // as many connections as asked for
        for (std::size_t i = 0u; i < api->serving; ++i)
        {
            net::open_result accepted;

            // a closed acceptor ends the serving
            if (api->acceptor.accept(accepted) != net::io_error::none)
            {
                break;
            }

            api->last = read_request(*accepted);
            (void)net::write_all(*accepted,
                                 answer(*api,
                                        api->last));
            (void)accepted->shutdown(net::shutdown_mode::write);
        }

        return D_THREAD_SUCCESS;
    }

    // start, finish
    //   function: binds loopback and serves `_count` connections; then
    // closes the acceptor and joins the thread.
    static bool
    start(
        stand_in&   _api,
        std::size_t _count
    )
    {
        _api.serving = _count;
        _api.started =
            ( (_api.acceptor.bind(net::endpoint("127.0.0.1",
                                                0u)) ==
               net::io_error::none) &&
              (::d_thread_create(&_api.thread,
                                 &serve_main,
                                 &_api) == 0) );

        return _api.started;
    }

    static void
    finish(
        stand_in& _api
    )
    {
        _api.acceptor.close();

        // only a started thread is joined
        if (_api.started)
        {
            (void)::d_thread_join(_api.thread,
                                  D_NULLPTR);
        }

        return;
    }

    // local
    //   function: a client of the stand-in that takes no proxy.
    static claude::client
    local(
        const stand_in& _api
    )
    {
        claude::client client("test-key",
                              base(_api));

        client.options().bypass_proxy = true;
        client.options().timeout_ms   = 10000;

        return client;
    }

    // events
    //   struct: the stream test's callback: records each event's name and
    // data, and the deltas' text.
    struct events
    {
        explicit events(
            std::vector<std::string>* _names,
            std::string*              _text
        )
            : names(_names),
              text(_text)
        {}

        bool
        operator()(
            const std::string& _name,
            const std::string& _data
        ) const
        {
            names->push_back(_name);

            // a delta carries text
            if (_name == "content_block_delta")
            {
                *text += claude::internal::json_string(_data,
                                                       "text");
            }

            return true;
        }

        std::vector<std::string>* names;
        std::string*              text;
    };

NS_END  // internal


/*
tests_claude_request
  Tests the following:
  - the request goes to /v1/messages as a POST with the key, the version
    and a JSON content type
  - its body carries the model, max_tokens, the messages in order, and the
    sampling settings that are set, numbers written as std::to_string would
*/
bool
tests_claude_request()
{
    claude::client          client("test-key",
                                   "http://example.invalid");
    claude::message_request request;
    bool                    result = true;

    request.model       = "claude-test";
    request.max_tokens  = 64;
    request.temperature = 0.5;
    request.top_k       = 40;
    request.add_user("Hi \"there\"").add_assistant("Hello").add_user("Again");

    const net::request built = client.build_request(request);

    result = internal::check( (built.method == net::http_method::post)  &&
                              (built.url ==
                               "http://example.invalid/v1/messages")    &&
                              (built.header("x-api-key") == "test-key") &&
                              (built.header("anthropic-version") ==
                               claude::default_version)                 &&
                              (built.header("content-type") ==
                               "application/json"),
                             "a POST to /v1/messages with its headers") &&
             result;
    result = internal::check( (built.body.find("\"model\":\"claude-test\"") !=
                               std::string::npos) &&
                              (built.body.find("\"max_tokens\":64") !=
                               std::string::npos) &&
                              (built.body.find("\"temperature\":0.500000") !=
                               std::string::npos) &&
                              (built.body.find("\"top_k\":40") !=
                               std::string::npos) &&
                              (built.body.find("Hi \\\"there\\\"") !=
                               std::string::npos) &&
                              (built.body.find("\"role\":\"assistant\"") !=
                               std::string::npos) &&
                              (built.body.find("top_p") ==
                               std::string::npos),
                             "the body carries what was set") && result;

    return result;
}

/*
tests_claude_send
  Tests the following:
  - send returns the message: its text across blocks, id, model and stop
    reason, and the request reached the stand-in with the key
  - a refused model is an error, with the API's type and message
*/
bool
tests_claude_send()
{
    internal::stand_in api;
    bool               result = true;

    const bool started = internal::start(api,
                                         2u);

    claude::client          client = internal::local(api);
    claude::message_request good;
    claude::message_request bad;

    good.add_user("Say hello");
    bad.model = "bad";
    bad.add_user("Say hello");

    const claude::message_response hello = client.send(good);
    const std::string              sent  = api.last;
    const claude::message_response error = client.send(bad);

    internal::finish(api);
    result = internal::check( (started)                        &&
                              (hello.ok())                     &&
                              (hello.status() == 200)          &&
                              (hello.text() == "Hello world")  &&
                              (hello.id() == "msg_1")          &&
                              (hello.model() == "claude-test") &&
                              (hello.stop_reason() == "end_turn"),
                             "a message comes back whole") && result;
    result = internal::check( (sent.find("x-api-key: test-key") !=
                               std::string::npos) &&
                              (sent.find("Say hello") != std::string::npos),
                             "the stand-in got the key and the prompt") &&
             result;
    result = internal::check( (!error.ok())             &&
                              (error.is_error())        &&
                              (error.status() == 400)   &&
                              (error.error_type() ==
                               "invalid_request_error") &&
                              (error.error_message() == "no such model"),
                             "a refusal carries the API's error") && result;

    return result;
}

/*
tests_claude_stream
  Tests the following:
  - stream hands each server-sent event to the callback, in order, CRLF
    lines included, and the deltas join into the text
*/
bool
tests_claude_stream()
{
    internal::stand_in       api;
    std::vector<std::string> names;
    std::string              text;
    bool                     result = true;

    const bool started = internal::start(api,
                                         1u);

    claude::client           client = internal::local(api);
    claude::message_request  request;
    claude::message_response meta;

    request.add_user("Stream it");

    const net::transport_error::value streamed =
        client.stream(request,
                      claude::sse_parser::event_fn(
                          internal::events(&names,
                                           &text)),
                      meta);

    internal::finish(api);
    result = internal::check( (started)                                &&
                              (streamed == net::transport_error::none) &&
                              (names.size() == 4u)                     &&
                              (names[0] == "message_start")            &&
                              (names[3] == "message_stop")             &&
                              (text == "Hello"),
                             "every event, in order, and the deltas' "
                             "text") && result;

    return result;
}

/*
tests_claude_batches
  Tests the following:
  - submit_and_wait creates a batch, polls it until it has ended, and
    fetches its results: one succeeded with its text and message id, one
    errored with the API's error
  - the counts come back with the ended status
*/
bool
tests_claude_batches()
{
    internal::stand_in api;
    bool               result = true;

    const bool started = internal::start(api,
                                         4u);

    batch::client client("test-key",
                         internal::base(api));

    client.options().bypass_proxy = true;
    client.options().timeout_ms   = 10000;

    std::vector<batch::item> items;
    claude::message_request  params;

    params.add_user("one");
    items.push_back(batch::item("first",
                                params));
    items.push_back(batch::item("second",
                                params));

    const batch::outcome done = client.submit_and_wait(items,
                                                       batch::poll_config(
                                                           20,
                                                           5000));

    internal::finish(api);
    result = internal::check( (started)                                   &&
                              (done.final_status.is_ended())              &&
                              (done.final_status.counts().succeeded == 1) &&
                              (done.final_status.counts().errored == 1)   &&
                              (api.retrieved == 2u),
                             "the batch is polled until it ends") && result;
    result = internal::check( (done.results.size() == 2u)               &&
                              (done.results[0].custom_id == "first")    &&
                              (done.results[0].ok())                    &&
                              (done.results[0].text() == "done")        &&
                              (done.results[0].message_id() == "msg_9") &&
                              (done.results[1].custom_id == "second")   &&
                              (!done.results[1].ok()),
                             "its results come back, one each") && result;

    return result;
}

/*
tests_claude_run_all
  Runs every test, reporting each, and the tally.
*/
bool
tests_claude_run_all()
{
    struct entry
    {
        const char* name;
        bool        (*run)();
    };

    static const entry CASES[] =
    {
        { "request", tests_claude_request },
        { "send",    tests_claude_send },
        { "stream",  tests_claude_stream },
        { "batches", tests_claude_batches }
    };
    bool all = true;

    // run every case, reporting each
    for (std::size_t i = 0u; i < sizeof(CASES) / sizeof(CASES[0]); ++i)
    {
        const entry& test   = CASES[i];
        const bool   passed = test.run();

        all = ( (all) &&
                (passed) );
        std::printf("  [%s] %s\n",
                    (passed) ? "PASS" : "FAIL",
                    test.name);
    }

    std::printf("%lu/%lu checks\n",
                static_cast<unsigned long>(internal::counts().checks -
                                           internal::counts().failures),
                static_cast<unsigned long>(internal::counts().checks));

    return all;
}

NS_END  // testing
NS_END  // djinterp
