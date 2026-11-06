/*******************************************************************************
* djinterp [test]                                            t_output_stream.cpp
*
*   The output stream's C++ harness: DOutputStream, DOutputCursor,
* DBufferedOutputStream and the standard-library adapters
* (output_stream_std.hpp), at every level from C++98. A standalone program,
* as the other tests not yet in DTest are: a line per group, one per failed
* check, "N checks, M failed", and exit status 0 only when nothing failed.
*   The moves are checked from C++11, std::format_to from C++20, and the
* writer.hpp sink from C++14, where those exist.
*
*
* path:      /tests/unintegrated/cpp/util/output/t_output_stream.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.11.05
*******************************************************************************/
// std
#include <algorithm>  // std::copy
#include <cstdio>     // std::printf
#include <cstring>    // std::strcmp, std::strlen
#include <ios>        // std::ios_base
#include <ostream>    // std::ostream
#include <sstream>    // std::ostringstream
#include <streambuf>  // std::streambuf
#include <string>     // std::string
// djinterp
#include "../../../../../inc/djinterp/cpp/djinterp.hpp"  // framework root
#include "../../../../../inc/djinterp/cpp/util/output/output_stream.hpp"
#include "../../../../../inc/djinterp/cpp/util/output/output_stream_std.hpp"
#include "../../../../../inc/djinterp/cpp/text/output_format.hpp"  // doubles
#include "../../../../../inc/djinterp/c/dstring.h"                 // dj_string
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    #include "../../../../../inc/djinterp/cpp/text/writer.hpp"   // make_writer
#endif
#if ( (D_ENV_LANG_IS_CPP20_OR_HIGHER) &&                                       \
      (defined(__has_include)) )
    #if __has_include(<format>)
        #include <format>  // std::format_to
        #if defined(__cpp_lib_format)
            #define D_TESTS_OUTPUT_HAS_FORMAT 1
        #endif
    #endif
#endif


using djinterp::DOutputStream;
using djinterp::DOutputCursor;
using djinterp::DBufferedOutputStream;
using djinterp::DOutputStreambuf;

// D_TESTS_OUTPUT_CHECK
//   macro: one check: counts it, and prints the expression and line on
// failure.
#define D_TESTS_OUTPUT_CHECK(expr)                                             \
    check(((expr) ? true : false), #expr, __LINE__)

namespace
{

std::size_t g_checks   = 0;
std::size_t g_failures = 0;

// check
//   function: counts one check, and reports it when it failed.
bool check(
    bool        _ok,
    const char* _expression,
    int         _line
)
{
    ++g_checks;

    if (!_ok)
    {
        ++g_failures;
        std::printf("    FAILED line %d: %s\n", _line, _expression);
    }

    return _ok;
}

// group_fixed
//   function: operator<< and the writers into a caller's array.
void group_fixed()
{
    char          array[64];
    DOutputStream out;

    D_TESTS_OUTPUT_CHECK(out.attach_fixed(array, sizeof(array), true));
    out << "x=" << 42 << ' ' << -7L << ' ' << 3000000000UL << ' ' << 1.5;
    (void)out.print_hex(255, 4);
    (void)djinterp::write_formatted(out, " [%s]", "ok");
    D_TESTS_OUTPUT_CHECK(out.flush());
    D_TESTS_OUTPUT_CHECK(std::strcmp(array,
                                     "x=42 -7 3000000000 1.500ff [ok]") == 0);
    D_TESTS_OUTPUT_CHECK(out.fixed_length() == std::strlen(array));
    D_TESTS_OUTPUT_CHECK(!out.fixed_truncated());

    char small[4];

    D_TESTS_OUTPUT_CHECK(out.attach_fixed(small, sizeof(small), true));
    out << "abcdef";
    D_TESTS_OUTPUT_CHECK(out.close());
    D_TESTS_OUTPUT_CHECK( (std::strcmp(small, "abc") == 0) &&
                          (out.written() == 6) &&
                          (out.fixed_truncated()) );

    D_TESTS_OUTPUT_CHECK(out.attach_counter());
    out << "counted" << 12345;
    D_TESTS_OUTPUT_CHECK(out.written() == 12);
    D_TESTS_OUTPUT_CHECK(out.is_valid());
    D_TESTS_OUTPUT_CHECK(out.close());
    D_TESTS_OUTPUT_CHECK(!out.is_valid());
    D_TESTS_OUTPUT_CHECK(!out.write("x"));
    D_TESTS_OUTPUT_CHECK(out.status() == D_OSTREAM_CLOSED);

    return;
}

// group_std_string
//   function: a std::string as a transport, unbuffered and buffered.
void group_std_string()
{
    std::string text;

    {
        DOutputStream out;

        D_TESTS_OUTPUT_CHECK(djinterp::attach_std_string(out, text));
        out << "one " << std::string("two") << ' ' << 3;
        D_TESTS_OUTPUT_CHECK(text == "one two 3");
    }

    {
        DBufferedOutputStream<16> out;

        D_TESTS_OUTPUT_CHECK(djinterp::attach_std_string(out, text));
        out << " four";
        D_TESTS_OUTPUT_CHECK(text == "one two 3");
        D_TESTS_OUTPUT_CHECK(out.flush());
        D_TESTS_OUTPUT_CHECK(text == "one two 3 four");

        for (int i = 0; i < 100; ++i)
        {
            out << 'x';
        }
    }

    D_TESTS_OUTPUT_CHECK(text.size() == 14 + 100);

    return;
}

// failing_buffer
//   class: a streambuf that refuses every byte, so an ostream over it fails.
class failing_buffer : public std::streambuf
{
protected:
    virtual int_type overflow(
        int_type _c
    )
    {
        (void)_c;

        return traits_type::eof();
    }
};

// group_std_ostream
//   function: a std::ostream as a transport, and failures that must not
// throw across the C boundary.
void group_std_ostream()
{
    std::ostringstream text;

    {
        DOutputStream out;

        D_TESTS_OUTPUT_CHECK(djinterp::attach_std_ostream(out, text));
        out << "alpha" << 1;
        D_TESTS_OUTPUT_CHECK(out.flush());
    }

    D_TESTS_OUTPUT_CHECK(text.str() == "alpha1");

    failing_buffer refuse;
    std::ostream   broken(&refuse);

    broken.exceptions(std::ios_base::badbit);

    DOutputStream out;

    D_TESTS_OUTPUT_CHECK(djinterp::attach_std_ostream(out, broken));
    D_TESTS_OUTPUT_CHECK(!out.write("never"));
    D_TESTS_OUTPUT_CHECK(out.status() == D_OSTREAM_FAILED);

    return;
}

// group_streambuf
//   function: std::ostream writing through DOutputStreambuf, whose put area
// is the stream's window.
void group_streambuf()
{
    struct dj_string* const string = d_string_new();

    {
        DOutputStream out;

        D_TESTS_OUTPUT_CHECK(out.attach_string(string));

        {
            DOutputStreambuf buffer(out);
            std::ostream     os(&buffer);

            os.imbue(std::locale::classic());
            os << "pi=" << 3.25 << ' ' << 1234567;

            for (int i = 0; i < 5000; ++i)
            {
                os << static_cast<char>('a' + (i % 26));
            }

            os << std::flush;
            D_TESTS_OUTPUT_CHECK(d_string_length(string) == 15 + 5000);
            os << "tail";
        }

        D_TESTS_OUTPUT_CHECK(out.close());
    }

    D_TESTS_OUTPUT_CHECK(d_string_length(string) == 15 + 5000 + 4);
    D_TESTS_OUTPUT_CHECK(std::strncmp(d_string_cstr(string),
                                      "pi=3.25 1234567abc",
                                      18) == 0);
    D_TESTS_OUTPUT_CHECK(std::strcmp(d_string_cstr(string) + 5015,
                                     "tail") == 0);
    d_string_free(string);

    return;
}

// group_cursor
//   function: DOutputCursor as an output iterator.
void group_cursor()
{
    std::string                text;
    DBufferedOutputStream<8>   out;
    const std::string          source(1000, 'q');

    D_TESTS_OUTPUT_CHECK(djinterp::attach_std_string(out, text));

    DOutputCursor it = std::copy(source.begin(), source.end(), out.cursor());

    *it = '!';
    ++it;
    D_TESTS_OUTPUT_CHECK(out.give(it));

#if defined(D_TESTS_OUTPUT_HAS_FORMAT)
    it = std::format_to(out.cursor(), "{}+{}={}", 2, 3, 5);
    D_TESTS_OUTPUT_CHECK(it.give());
#endif

    D_TESTS_OUTPUT_CHECK(out.close());
    D_TESTS_OUTPUT_CHECK(text.size() >= 1001);
    D_TESTS_OUTPUT_CHECK(text[1000] == '!');

#if defined(D_TESTS_OUTPUT_HAS_FORMAT)
    D_TESTS_OUTPUT_CHECK(text.substr(1001) == "2+3=5");
#endif

    return;
}

#if (D_MOVE_ENABLED == 1)
// group_moves
//   function: moves re-point what pointed into the object they left.
void group_moves()
{
    // a fixed array spilled into its scratch, inside the object
    char          array[4];
    DOutputStream first;

    D_TESTS_OUTPUT_CHECK(first.attach_fixed(array, sizeof(array), true));
    first << "abcdefgh";

    DOutputStream second(static_cast<DOutputStream&&>(first));

    second << "ij";
    D_TESTS_OUTPUT_CHECK(second.close());
    D_TESTS_OUTPUT_CHECK( (std::strcmp(array, "abc") == 0) &&
                          (second.written() == 10) );
    D_TESTS_OUTPUT_CHECK(!first.is_valid());

    // a buffer the object carries, with bytes in it
    std::string               text;
    DBufferedOutputStream<16> carried;

    D_TESTS_OUTPUT_CHECK(djinterp::attach_std_string(carried, text));
    carried << "held";

    DBufferedOutputStream<16> moved(
        static_cast<DBufferedOutputStream<16>&&>(carried));

    D_TESTS_OUTPUT_CHECK(text.empty());
    moved << "+more";
    D_TESTS_OUTPUT_CHECK(moved.close());
    D_TESTS_OUTPUT_CHECK(text == "held+more");

    // into an object without a buffer: delivered at the move
    text.clear();
    D_TESTS_OUTPUT_CHECK(djinterp::attach_std_string(carried, text));
    carried << "sliced";

    DOutputStream plain(static_cast<DOutputStream&&>(carried));

    D_TESTS_OUTPUT_CHECK(text == "sliced");
    plain << "!";
    D_TESTS_OUTPUT_CHECK(text == "sliced!");

    // assignment between buffered streams
    DBufferedOutputStream<16> target;
    std::string               other;

    D_TESTS_OUTPUT_CHECK(djinterp::attach_std_string(target, other));
    target << "x";
    D_TESTS_OUTPUT_CHECK(djinterp::attach_std_string(moved, text));
    moved << "y";
    target = static_cast<DBufferedOutputStream<16>&&>(moved);
    D_TESTS_OUTPUT_CHECK(other == "x");
    target << "z";
    D_TESTS_OUTPUT_CHECK(target.close());
    D_TESTS_OUTPUT_CHECK(text == "sliced!yz");

    return;
}
#endif  // D_MOVE_ENABLED

// group_sink
//   function: DOutputStreamSink, the callable sink shape, and the C sink.
void group_sink()
{
    char          array[32];
    DOutputStream out;

    D_TESTS_OUTPUT_CHECK(out.attach_fixed(array, sizeof(array), true));

    djinterp::DOutputStreamSink sink(out);

    sink("call", 4);
    D_TESTS_OUTPUT_CHECK(d_sink_emit(out.as_sink(), "/emit", 5) != 0);

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    djinterp::make_writer(djinterp::DOutputStreamSink(out))
        .write(std::string("/writer"));
#else
    out << "/writer";
#endif

    D_TESTS_OUTPUT_CHECK(out.close());
    D_TESTS_OUTPUT_CHECK(std::strcmp(array, "call/emit/writer") == 0);

    return;
}

}  // namespace


/*
main
  Runs every group in order and reports the totals.
*/
int main()
{
    std::printf("fixed\n");
    group_fixed();
    std::printf("std::string\n");
    group_std_string();
    std::printf("std::ostream\n");
    group_std_ostream();
    std::printf("streambuf\n");
    group_streambuf();
    std::printf("cursor\n");
    group_cursor();
#if (D_MOVE_ENABLED == 1)
    std::printf("moves\n");
    group_moves();
#endif
    std::printf("sink\n");
    group_sink();

    std::printf("%lu checks, %lu failed\n",
                static_cast<unsigned long>(g_checks),
                static_cast<unsigned long>(g_failures));

    return (g_failures == 0) ? 0 : 1;
}
