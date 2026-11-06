/*******************************************************************************
* djinterp [test]                                            t_writer_stream.cpp
*
*   writer.hpp's sinks and printer.hpp's file printer on the output stream
* (stream 1, step 6): a sink returns nothing, so a failed write is kept by
* the stream it writes through, and ok() -- the sink's, the writer's, the
* file printer's -- says so. string_sink and stream_sink write as they did;
* a std::ostream gone bad, or one that throws, ends the sink; output_stream_sink
* writes into any output stream; file_printer reports a file that would not
* open, and print_document_to_file returns whether the document reached the
* file; a document's to_stream and to_file (cpp/util/document) return the
* same, and the document renderers -- plain, markdown, XML, HTML and the PDF
* canvas -- write what they rendered to a stream with write_to and save_to.
* C++17 and up, printer.hpp's floor; below it the program says so and
* passes.
*   A standalone program, as the other tests not yet in DTest are
* (tests/unintegrated/README.md): a line per group and per failed check, then
* "N checks, M failed", and exit status 0 only when nothing failed. Its
* temporary file is written in the working folder and removed.
*
*
* path:      /tests/unintegrated/cpp/text/t_writer_stream.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.05
*******************************************************************************/
// std
#include <cstdio>     // std::printf, std::fopen, std::fread, std::remove
#include <ostream>    // std::ostream
#include <sstream>    // std::ostringstream
#include <streambuf>  // std::streambuf
#include <string>     // std::string
#include <vector>     // std::vector
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"  // framework root
#include "../../../../inc/djinterp/env/env.h"         // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include "../../../../inc/djinterp/cpp/text/printer.hpp"
                                    // file_printer, node_to_string
    #include "../../../../inc/djinterp/cpp/text/writer.hpp"
                                    // the sinks, the writers
    #include "../../../../inc/djinterp/cpp/util/output/output_stream.hpp"
                                    // DOutputStream
    #include "../../../../inc/djinterp/cpp/util/document/document.hpp"
                                    // xml_document
    #include "../../../../inc/djinterp/cpp/util/document/templates/markdown_document_renderer.hpp"
                                    // markdown_document_renderer
    #include "../../../../inc/djinterp/cpp/util/document/templates/markup_document_renderer.hpp"
                                    // xml_ and html_document_renderer
    #include "../../../../inc/djinterp/cpp/util/pdf/pdf_canvas_document_renderer.hpp"
                                    // DPdfCanvasDocumentRenderer
#endif  // C++17


// the counts every check adds to
static unsigned long g_checks   = 0;
static unsigned long g_failures = 0;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// D_TESTS_WRITER_CHECK
//   macro: one check: counts it, and prints the expression and line on
// failure.
#define D_TESTS_WRITER_CHECK(expr)                                             \
    d_tests_writer_check(((expr) ? true : false), #expr, __LINE__)

// a node of the printer's protocol: a name and its children
struct DTestsNode
{
    std::string             m_name;
    std::vector<DTestsNode> m_children;

    std::string name() const
    {
        return m_name;
    }

    const std::vector<DTestsNode>& children() const
    {
        return m_children;
    }
};

// a std::streambuf that refuses every byte, so its std::ostream goes bad
class DTestsRefusingBuf : public std::streambuf
{
protected:
    int_type overflow(
        int_type _c
    ) override
    {
        (void)_c;

        return traits_type::eof();
    }

    std::streamsize xsputn(
        const char*     _data,
        std::streamsize _size
    ) override
    {
        (void)_data;
        (void)_size;

        return 0;
    }
};


/*
d_tests_writer_check
  Counts one check, and prints it where it failed.
*/
static void d_tests_writer_check(
    bool        _passed,
    const char* _expression,
    int         _line
)
{
    ++g_checks;

    if (!_passed)
    {
        ++g_failures;
        std::printf("    FAILED line %d: %s\n", _line, _expression);
    }

    return;
}

/*
d_tests_writer_sinks
  Tests the following:
  - string_sink appends, and stays ok
  - stream_sink writes to a std::ostringstream, and stays ok
  - stream_sink over a std::ostream that refuses bytes is not ok after a
    write, and stays not ok; one that throws on badbit is the same, and
    nothing is thrown through the sink
  - output_stream_sink writes into a caller's stream, and its ok() is the
    stream's
*/
static void d_tests_writer_sinks()
{
    std::printf("  sinks\n");

    std::string           text;
    djinterp::string_sink to_text(text);

    to_text("abc", 3);
    to_text("de", 2);
    D_TESTS_WRITER_CHECK(text == "abcde");
    D_TESTS_WRITER_CHECK(to_text.ok());

    std::ostringstream    out;
    djinterp::stream_sink to_out(out);

    to_out("xyz", 3);
    D_TESTS_WRITER_CHECK(out.str() == "xyz");
    D_TESTS_WRITER_CHECK(to_out.ok());

    // a std::ostream that refuses: the failure is kept
    DTestsRefusingBuf     refusing;
    std::ostream          refused(&refusing);
    djinterp::stream_sink to_refused(refused);

    D_TESTS_WRITER_CHECK(to_refused.ok());
    to_refused("lost", 4);
    D_TESTS_WRITER_CHECK(!to_refused.ok());
    to_refused("x", 1);
    D_TESTS_WRITER_CHECK(!to_refused.ok());

    // a std::ostream that throws: caught, and kept
    DTestsRefusingBuf     throwing_buf;
    std::ostream          throwing(&throwing_buf);
    bool                  thrown = false;

    throwing.exceptions(std::ios::badbit);

    djinterp::stream_sink to_throwing(throwing);

    try
    {
        to_throwing("boom", 4);
    }
    catch (...)
    {
        thrown = true;
    }

    D_TESTS_WRITER_CHECK(!thrown);
    D_TESTS_WRITER_CHECK(!to_throwing.ok());

    // a caller's stream, over a fixed array
    char                    buffer[16];
    djinterp::DOutputStream stream;

    D_TESTS_WRITER_CHECK(stream.attach_fixed(buffer, sizeof(buffer)));

    djinterp::output_stream_sink to_stream(stream);

    to_stream("fixed", 5);
    D_TESTS_WRITER_CHECK(to_stream.ok());
    D_TESTS_WRITER_CHECK(stream.close());
    D_TESTS_WRITER_CHECK(std::string(buffer) == "fixed");

    return;
}

/*
d_tests_writer_writers
  Tests the following:
  - make_writer over a string_sink renders and appends, and its ok() is the
    sink's
  - stream_writer over a refusing std::ostream reports !ok() after a write
  - output_writer writes into a caller's stream
  - a writer over a lambda, which cannot say, is ok()
*/
static void d_tests_writer_writers()
{
    std::printf("  writers\n");

    std::string text;
    auto        to_text = djinterp::make_writer(djinterp::string_sink(text));

    to_text.write("one").write(std::string(" two"));
    D_TESTS_WRITER_CHECK(text == "one two");
    D_TESTS_WRITER_CHECK(to_text.ok());

    DTestsRefusingBuf refusing;
    std::ostream      refused(&refusing);
    auto              to_refused = djinterp::stream_writer(refused);

    D_TESTS_WRITER_CHECK(to_refused.ok());
    to_refused.write("lost");
    D_TESTS_WRITER_CHECK(!to_refused.ok());

    std::string             streamed;
    djinterp::DOutputStream stream;

    D_TESTS_WRITER_CHECK(djinterp::attach_std_string(stream, streamed));

    auto to_stream = djinterp::output_writer(stream);

    to_stream("a")("b")("c");
    D_TESTS_WRITER_CHECK(to_stream.ok());
    D_TESTS_WRITER_CHECK(streamed == "abc");

    std::size_t counted = 0;
    auto        to_lambda = djinterp::make_writer(
        [&counted](const char* _data, std::size_t _size)
        {
            (void)_data;
            counted += _size;
        });

    to_lambda.write("four");
    D_TESTS_WRITER_CHECK(counted == 4u);
    D_TESTS_WRITER_CHECK(to_lambda.ok());

    return;
}

/*
d_tests_writer_file_printer
  Tests the following:
  - file_printer prints a document as node_to_string does, and closes ok
  - print_document_to_file returns true, and writes the same bytes
  - a file that will not open: good(), print() and close() are false, and
    print_document_to_file returns false
*/
static void d_tests_writer_file_printer()
{
    std::printf("  file printer\n");

    const char* const path = "t_writer_stream.tmp";
    const DTestsNode  root = { "a",
                               { DTestsNode{ "b", {} },
                                 DTestsNode{ "c", {} } } };
    const std::string want = djinterp::node_to_string(root);

    // print, close, read back
    {
        djinterp::file_printer printer{ std::string(path) };

        D_TESTS_WRITER_CHECK(printer.good());
        D_TESTS_WRITER_CHECK(printer.print(root));
        D_TESTS_WRITER_CHECK(printer.close());
    }

    std::string got;
    std::FILE*  in = std::fopen(path, "rb");

    if (in != NULL)
    {
        char        chunk[256];
        std::size_t n = 0;

        while ((n = std::fread(chunk, 1, sizeof(chunk), in)) > 0)
        {
            got.append(chunk, n);
        }

        (void)std::fclose(in);
    }

    D_TESTS_WRITER_CHECK(got == want);

    // in one call
    D_TESTS_WRITER_CHECK(djinterp::print_document_to_file(path, root));
    (void)std::remove(path);

    // a file that will not open
    djinterp::file_printer nowhere{ std::string("no_such_dir/t_writer.tmp") };

    D_TESTS_WRITER_CHECK(!nowhere.good());
    D_TESTS_WRITER_CHECK(!nowhere.print(root));
    D_TESTS_WRITER_CHECK(!nowhere.close());
    D_TESTS_WRITER_CHECK(
        !djinterp::print_document_to_file("no_such_dir/t_writer.tmp", root));

    return;
}

/*
d_tests_writer_document
  Tests the following:
  - an xml_document's to_stream into a std::ostringstream writes what
    to_string gives, and returns true; into a refusing std::ostream, false
  - to_stream into a DOutputStream over a std::string writes the same
  - to_file writes the same and returns true; to a file that will not open,
    false
*/
static void d_tests_writer_document()
{
    std::printf("  documents\n");

    const char* const      path = "t_writer_document.tmp";
    djinterp::xml_document doc;

    doc.root("a").append_child("b").append_child("c");

    const std::string want = doc.to_string();

    std::ostringstream out;

    D_TESTS_WRITER_CHECK(doc.to_stream(out));
    D_TESTS_WRITER_CHECK(out.str() == want);

    DTestsRefusingBuf refusing;
    std::ostream      refused(&refusing);

    D_TESTS_WRITER_CHECK(!doc.to_stream(refused));

    std::string             streamed;
    djinterp::DOutputStream stream;

    D_TESTS_WRITER_CHECK(djinterp::attach_std_string(stream, streamed));
    D_TESTS_WRITER_CHECK(doc.to_stream(stream));
    D_TESTS_WRITER_CHECK(streamed == want);

    D_TESTS_WRITER_CHECK(doc.to_file(path));

    std::string got;
    std::FILE*  in = std::fopen(path, "rb");

    if (in != NULL)
    {
        char        chunk[256];
        std::size_t n = 0;

        while ((n = std::fread(chunk, 1, sizeof(chunk), in)) > 0)
        {
            got.append(chunk, n);
        }

        (void)std::fclose(in);
    }

    D_TESTS_WRITER_CHECK(got == want);
    (void)std::remove(path);

    D_TESTS_WRITER_CHECK(!doc.to_file("no_such_dir/t_writer_document.tmp"));

    return;
}

/*
d_tests_writer_render_to
  Whether a renderer, given a heading, a paragraph and a list, writes what it
rendered (str()) to a std::string through a stream, and fails to a stream
that refuses.
*/
template<typename Renderer>
static bool d_tests_writer_render_to(
    Renderer& _renderer
)
{
    const djinterp::doc_attributes none;

    _renderer.begin_document(none);
    _renderer.heading(1, "Report", none);
    _renderer.paragraph("All <ok> & well.", none);
    _renderer.begin_list(false, none);
    _renderer.list_item("one", none);
    _renderer.end_list();
    _renderer.end_document();

    std::string             streamed;
    djinterp::DOutputStream stream;
    DTestsRefusingBuf       refusing;
    std::ostream            refused(&refusing);
    djinterp::DOutputStream refusing_stream;

    (void)djinterp::attach_std_string(stream, streamed);
    (void)djinterp::attach_std_ostream(refusing_stream, refused);

    const bool written = _renderer.write_to(stream);
    const bool lost    = _renderer.write_to(refusing_stream);

    return ( (written)                       &&
             (streamed == _renderer.str())   &&
             (!streamed.empty())             &&
             (!lost)                         &&
             (!refusing_stream.is_valid()) );
}

/*
d_tests_writer_renderers
  Tests the following:
  - DPlainDocumentRenderer, markdown_document_renderer, xml_ and
    html_document_renderer write their document to a stream with write_to,
    and report a stream that refuses
  - DPdfCanvasDocumentRenderer's save_to writes what to_bytes gives
*/
static void d_tests_writer_renderers()
{
    std::printf("  renderers\n");

    djinterp::DPlainDocumentRenderer     plain;
    djinterp::markdown_document_renderer markdown;
    djinterp::xml_document_renderer      xml;
    djinterp::html_document_renderer     html;

    D_TESTS_WRITER_CHECK(d_tests_writer_render_to(plain));
    D_TESTS_WRITER_CHECK(d_tests_writer_render_to(markdown));
    D_TESTS_WRITER_CHECK(d_tests_writer_render_to(xml));
    D_TESTS_WRITER_CHECK(d_tests_writer_render_to(html));

    const djinterp::doc_attributes       none;
    djinterp::DPdfCanvasDocumentRenderer pdf;

    pdf.heading(1, "Report", none);
    pdf.paragraph("Page one.", none);

    std::string             streamed;
    djinterp::DOutputStream stream;

    (void)djinterp::attach_std_string(stream, streamed);
    D_TESTS_WRITER_CHECK(pdf.save_to(stream));
    D_TESTS_WRITER_CHECK(streamed == pdf.to_bytes());
    D_TESTS_WRITER_CHECK(streamed.compare(0, 5, "%PDF-") == 0);

    return;
}

#endif  // C++17


/*
main
  Every group, then the count.
*/
int main()
{
    std::printf("t_writer_stream (C++%ld)\n", static_cast<long>(__cplusplus));

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    d_tests_writer_sinks();
    d_tests_writer_writers();
    d_tests_writer_file_printer();
    d_tests_writer_document();
    d_tests_writer_renderers();
#else
    std::printf("  (below C++17, printer.hpp's floor: nothing to run)\n");
#endif  // C++17

    std::printf("%lu checks, %lu failed\n", g_checks, g_failures);

    return (g_failures == 0u) ? 0 : 1;
}
