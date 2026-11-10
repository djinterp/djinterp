/*******************************************************************************
* djinterp [test]                                                  bench_cpp.cpp
*
*   The C++ face's cost against the standard library's, for the same
* operations: iostreams (a std::ofstream to /dev/null with a 64 KiB buffer),
* std::to_chars, std::format_to, and DOutputStream, DOutputCursor and
* DOutputStreambuf over a discarding stream with a 64 KiB buffer. Best of
* seven runs, nanoseconds per operation. C++20.
*
*
* path:      /tests/bench/util/ostream/bench_cpp.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.11.09
*******************************************************************************/
// std
#include <charconv>   // std::to_chars
#include <chrono>     // std::chrono::steady_clock
#include <cstdio>     // std::printf
#include <format>     // std::format_to
#include <fstream>    // std::ofstream
#include <iterator>   // std::back_inserter
#include <string>     // std::string
// djinterp
#include "djinterp/cpp/util/output/output_stream.hpp"
#include "djinterp/cpp/util/output/output_stream_std.hpp"

namespace
{

constexpr long RUNS  = 7;
constexpr long CHARS = 100000000L;

unsigned char     g_buffer[1 << 16];
char              g_iobuffer[1 << 16];
volatile unsigned g_sink;

extern "C" std::size_t discard(
    void*       _context,
    const void* _data,
    std::size_t _size
)
{
    (void)_context;
    g_sink += static_cast<const unsigned char*>(_data)[0];

    return _size;
}

void open(
    djinterp::DOutputStream& _out
)
{
    struct dj_pack_sink sink;

    sink.write   = &discard;
    sink.context = nullptr;
    (void)_out.attach(sink);
    (void)_out.set_buffer(g_buffer, sizeof(g_buffer));
}

template<typename Setup,
         typename Body,
         typename Teardown>
void bench(
    const char* _name,
    long        _count,
    Setup       _setup,
    Body        _body,
    Teardown    _teardown
)
{
    double best = 1e30;

    for (long run = 0; run < RUNS; ++run)
    {
        _setup();

        const auto start = std::chrono::steady_clock::now();

        for (long i = 0; i < _count; ++i)
        {
            _body(i);
        }

        const std::chrono::duration<double> took =
            std::chrono::steady_clock::now() - start;

        _teardown();
        best = (took.count() < best) ? took.count() : best;
    }

    std::printf("%-40s %7.2f ns\n", _name, best * 1e9 / double(_count));
}

}  // namespace

int main()
{
    std::ofstream file;

    file.rdbuf()->pubsetbuf(g_iobuffer, sizeof(g_iobuffer));
    file.open("/dev/null", std::ios::binary);

    djinterp::DOutputStream out;
    auto none = [] {};

    std::printf("C++ (%s), to /dev/null or a discarding stream\n",
                __VERSION__);

    bench("std::ofstream::put", CHARS, none,
          [&](long i) { file.put(char(i)); }, [&] { file.flush(); });
    bench("std::ofstream << char", CHARS, none,
          [&](long i) { file << char(i); }, [&] { file.flush(); });
    bench("DOutputStream << char", CHARS, [&] { open(out); },
          [&](long i) { out << char(i); }, [&] { out.close(); });
    {
        djinterp::DOutputCursor it(nullptr);

        bench("DOutputCursor *it++ = c", CHARS,
              [&] { open(out); it = out.cursor(); },
              [&](long i) { *it++ = char(i); },
              [&] { (void)out.give(it); out.close(); });
    }
    {
        djinterp::DOutputStreambuf* buf = nullptr;
        std::ostream*               os  = nullptr;

        bench("std::ostream(DOutputStreambuf) << c", CHARS,
              [&] { open(out); buf = new djinterp::DOutputStreambuf(out);
                    os = new std::ostream(buf); },
              [&](long i) { *os << char(i); },
              [&] { delete os; delete buf; out.close(); });
    }

    bench("std::ofstream << long", CHARS / 10, none,
          [&](long i) { file << i; }, [&] { file.flush(); });
    bench("DOutputStream << long", CHARS / 10, [&] { open(out); },
          [&](long i) { out << i; }, [&] { out.close(); });
    {
        char text[32];

        bench("std::to_chars(long) to an array", CHARS / 10, none,
              [&](long i) { auto r = std::to_chars(text, text + 32, i);
                            g_sink += unsigned(r.ptr - text); },
              none);
    }

    {
        std::string s;

        bench("std::format_to(back_inserter) {},{};", CHARS / 50,
              [&] { s.clear(); },
              [&](long i) { std::format_to(std::back_inserter(s), "{},{};",
                                           int(i), int(i >> 3)); },
              [&] { g_sink += unsigned(s.size()); });
    }
    {
        djinterp::DOutputCursor it(nullptr);

        bench("std::format_to(DOutputCursor) {},{};", CHARS / 50,
              [&] { open(out); it = out.cursor(); },
              [&](long i) { it = std::format_to(it, "{},{};", int(i),
                                                int(i >> 3)); },
              [&] { (void)out.give(it); out.close(); });
    }

    return (g_sink == 12345u) ? 1 : 0;
}
