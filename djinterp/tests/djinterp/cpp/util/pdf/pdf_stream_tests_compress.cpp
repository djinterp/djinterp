/*******************************************************************************
* djinterp [test]                                  pdf_stream_tests_compress.cpp
*
*   The built-in PDF backend's stream compression: FlateDecode on every
* stream, PNG's predictors on an image's samples, and the setting that turns
* it off. One scene, two pages of text, shapes and three images, is written
* both ways. Raw, it is the file the backend wrote before it compressed: its
* length and CRC-32 were taken from that backend, with the creation date
* masked. Compressed, each stream is FlateDecode, and inflates, and for an
* image un-predicts, back to the raw file's stream exactly.
*   Where the build has no zlib, the compressed groups are skipped, and say
* why.
*
*
* path:      /tests/djinterp/cpp/util/pdf/pdf_stream_tests_compress.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.18
*                                                            revised: 2026.11.18
*******************************************************************************/
#include "pdf_stream_tests.hpp"  // the suite's entry points, DTest's C face
// std
#include <cstddef>  // std::size_t
#include <cstdlib>  // std::strtoul
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../../../../../inc/djinterp/cpp/djinterp.hpp"  // framework root
#include "../../../../../inc/djinterp/c/util/compress/compress_common.h"  // crc
#include "../../../../../inc/djinterp/cpp/util/pdf/pdf_options.hpp"  // all PDF


// g_compress_suite
//   the suite being built, for the checks; set by d_tests_pdf_compress_block.
static dj_test_suite* g_compress_suite = NULL;


#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// D_TESTS_PDF_Z_CHECK
//   macro: one check, recorded against the test open in the suite.
#define D_TESTS_PDF_Z_CHECK(expr)                                              \
    (void)d_test_suite_check(g_compress_suite, ((expr) ? true : false),       \
                             #expr, __FILE__, __LINE__)

// the raw scene as the backend wrote it before it compressed, its creation
// date's fourteen digits made zeros
static const std::size_t   d_tests_pdf_raw_length = 3363u;
static const unsigned long d_tests_pdf_raw_crc    = 0x48F6EB12ul;

/*
DTestsPdfStream
  One stream of a written file: its dictionary, and its bytes.
*/
struct DTestsPdfStream
{
    std::string dictionary;
    std::string bytes;
};


/*
d_tests_pdf_compress_scene
  The scene: two pages, text in two faces with characters that need
escapes, a line, a rectangle, a path with curves, and three images (gray, an
RGB gradient drawn twice, a noisy RGBA).
*/
static void
d_tests_pdf_compress_scene(
    djinterp::DPdfDocument& _doc
)
{
    using namespace djinterp;

    DPdfTextOptions plain;
    DPdfTextOptions sans;
    DPdfPaint       paint;
    DPdfImage       gray(5u, 3u, DPdfImageFormat::gray);
    DPdfImage       ramp(16u, 8u, DPdfImageFormat::rgb);
    DPdfImage       noise(7u, 5u, DPdfImageFormat::rgba);
    unsigned        seed = 2026u;

    sans.font  = pdf_font(DPdfBaseFont::helvetica_bold, 14.0);
    sans.color = pdf_color(0.2, 0.4, 0.6);
    for (std::size_t i = 0; i < gray.samples.size(); ++i)
    {
        gray.samples[i] = static_cast<unsigned char>((i * 17u) & 0xffu);
    }
    for (std::size_t y = 0; y < ramp.height; ++y)
    {
        for (std::size_t x = 0; x < ramp.width; ++x)
        {
            unsigned char* px = &ramp.samples[((y * ramp.width) + x) * 3u];

            px[0] = static_cast<unsigned char>(x * 16u);
            px[1] = static_cast<unsigned char>(y * 32u);
            px[2] = static_cast<unsigned char>((x + y) * 8u);
        }
    }
    for (std::size_t i = 0; i < noise.samples.size(); ++i)
    {
        seed             = (seed * 1103515245u) + 12345u;
        noise.samples[i] = static_cast<unsigned char>((seed >> 16) & 0xffu);
    }

    _doc.metadata("title", "Flate (and raw) \\ streams");
    _doc.add_page();
    _doc.text(DPdfPoint(72.0, 720.0), "Hello, (PDF) \\ world", plain);
    _doc.text(DPdfPoint(72.0, 700.0), "Sans bold 14", sans);
    paint.line_width = 1.5;
    _doc.line(DPdfPoint(72.0, 690.0), DPdfPoint(540.0, 690.0), paint);
    paint.do_fill = true;
    paint.fill    = pdf_color(0.9, 0.1, 0.1);
    _doc.rect(DPdfRect(72.0, 600.0, 120.0, 60.0), paint);
    _doc.path(DPdfPath::circle(DPdfPoint(300.0, 630.0), 25.0), paint);
    _doc.image(ramp, DPdfRect(72.0, 400.0, 160.0, 80.0));
    _doc.image(gray, DPdfRect(250.0, 400.0, 50.0, 30.0));
    _doc.add_page(DPdfPageSize::a4());
    _doc.text(DPdfPoint(50.0, 800.0), "Page two", plain);
    _doc.image(noise, DPdfRect(50.0, 600.0, 70.0, 50.0));
    _doc.image(ramp, DPdfRect(150.0, 600.0, 80.0, 40.0));

    return;
}

/*
d_tests_pdf_compress_write
  The scene written under a compression setting, its creation date's digits
made zeros, so that two runs compare.
*/
static std::string
d_tests_pdf_compress_write(
    djinterp::DPdfCompression _compression
)
{
    djinterp::DPdfDocument doc;
    std::string            pdf;
    std::size_t            at;

    doc.set_compression(_compression);
    d_tests_pdf_compress_scene(doc);
    pdf = doc.to_bytes();
    at  = pdf.find("/CreationDate (D:");

    if (at != std::string::npos)
    {
        for (std::size_t i = at + 17u; (i < at + 31u) && (i < pdf.size()); ++i)
        {
            pdf[i] = '0';
        }
    }

    return pdf;
}

/*
d_tests_pdf_streams
  Every stream of a written file, in order: the dictionary before it, and as
many bytes as its /Length says.
*/
static std::vector<DTestsPdfStream>
d_tests_pdf_streams(
    const std::string& _pdf
)
{
    std::vector<DTestsPdfStream> found;
    std::size_t                  at = 0;

    while ((at = _pdf.find(">>\nstream\n", at)) != std::string::npos)
    {
        const std::size_t open   = _pdf.rfind(" obj\n<<", at);
        const std::size_t length = _pdf.find("/Length ", open);
        DTestsPdfStream   stream;

        if ( (open == std::string::npos) ||
             (length == std::string::npos) ||
             (length > at) )
        {
            break;
        }

        stream.dictionary = _pdf.substr(open + 5u, (at + 2u) - (open + 5u));
        stream.bytes      = _pdf.substr(at + 10u,
                                        std::strtoul(&_pdf[length + 8u],
                                                     NULL, 10));
        found.push_back(stream);
        at += 10u;
    }

    return found;
}

/*
d_tests_pdf_dictionary_number
  The number after a key in a dictionary, or 0.
*/
static std::size_t
d_tests_pdf_dictionary_number(
    const std::string& _dictionary,
    const char*        _key
)
{
    const std::size_t at = _dictionary.find(_key);

    return (at == std::string::npos)
               ? 0u
               : std::strtoul(&_dictionary[at + std::string(_key).size()],
                              NULL, 10);
}

/*
d_tests_pdf_inflate
  A FlateDecode stream inflated, sized for what it should hold, or "(none)"
when it doesn't inflate to exactly that size.
*/
static std::string
d_tests_pdf_inflate(
    const std::string& _bytes,
    std::size_t        _expected
)
{
    std::string out(_expected + 16u, '\0');
    std::size_t size = 0;

    if ( (d_decompress(D_CODEC_ID_ZLIB, _bytes.data(), _bytes.size(),
                       &out[0], out.size(), &size) != D_PACK_STATUS_OK) ||
         (size != _expected) )
    {
        return std::string("(none)");
    }

    out.resize(size);

    return out;
}

/*
d_tests_pdf_unpredict
  PNG's filters undone, as FlateDecode's /Predictor 15 undoes them: each row
of _columns pixels, _colors bytes each, after its filter's byte. "(bad)" for
a filter byte past 4 or a short row.
*/
static std::string
d_tests_pdf_unpredict(
    const std::string& _predicted,
    std::size_t        _columns,
    std::size_t        _colors
)
{
    const std::size_t stride = _columns * _colors;
    std::string       out;

    for (std::size_t at = 0; at < _predicted.size(); at += stride + 1u)
    {
        const unsigned    filter = static_cast<unsigned char>(_predicted[at]);
        const std::size_t base   = out.size();

        if ( (filter > 4u) || (at + stride + 1u > _predicted.size()) )
        {
            return std::string("(bad)");
        }

        for (std::size_t i = 0; i < stride; ++i)
        {
            const unsigned x =
                static_cast<unsigned char>(_predicted[at + 1u + i]);
            const unsigned a = (i >= _colors)
                ? static_cast<unsigned char>(out[base + i - _colors]) : 0u;
            const unsigned b = (base >= stride)
                ? static_cast<unsigned char>(out[base - stride + i]) : 0u;
            const unsigned c = ( (i >= _colors) && (base >= stride) )
                ? static_cast<unsigned char>(out[base - stride + i - _colors])
                : 0u;
            const unsigned p = (filter == 1u) ? a
                             : (filter == 2u) ? b
                             : (filter == 3u) ? ((a + b) / 2u)
                             : (filter == 4u)
                                 ? djinterp::internal::pdf_png_paeth(a, b, c)
                                 : 0u;

            out.push_back(static_cast<char>((x + p) & 0xffu));
        }
    }

    return out;
}


/*
d_tests_pdf_compress_raw
  Tests the following:
  - DPdfCompression::none writes the file the backend wrote before it
    compressed: its length and CRC-32, the creation date masked
  - pdf_options::set_compression(none), applied to a document, writes the
    same file; its streams carry no /Filter
  - capabilities().compression is false under none
*/
static void
d_tests_pdf_compress_raw(void)
{
    using djinterp::DPdfCompression;

    const std::string raw = d_tests_pdf_compress_write(DPdfCompression::none);
    djinterp::DPdfDocument doc;
    std::string            through;
    std::size_t            at;

    D_TESTS_PDF_Z_CHECK(raw.size() == d_tests_pdf_raw_length);
    D_TESTS_PDF_Z_CHECK(d_pack_crc32(0u, raw.data(), raw.size()) ==
                        d_tests_pdf_raw_crc);
    D_TESTS_PDF_Z_CHECK(raw.find("/Filter") == std::string::npos);

    djinterp::pdf_options().set_compression(DPdfCompression::none)
                           .apply_to(doc);
    d_tests_pdf_compress_scene(doc);
    D_TESTS_PDF_Z_CHECK(!doc.backend().capabilities().compression);
    through = doc.to_bytes();
    at      = through.find("/CreationDate (D:");
    D_TESTS_PDF_Z_CHECK(at != std::string::npos);
    through.replace(at + 17u, 14u, 14u, '0');
    D_TESTS_PDF_Z_CHECK(through == raw);

    return;
}


/*
d_tests_pdf_compress_flate
  Tests the following:
  - flate writes every stream with /Filter /FlateDecode, and as many
    streams as raw
  - each page's content inflates to the raw file's content exactly
  - each image says /Predictor 15 with its colors and columns, inflates,
    and un-predicts to the raw file's samples exactly
  - the compressed file is smaller than the raw one
*/
static void
d_tests_pdf_compress_flate(void)
{
    using djinterp::DPdfCompression;

    const std::string raw   = d_tests_pdf_compress_write(DPdfCompression::none);
    const std::string flate =
        d_tests_pdf_compress_write(DPdfCompression::flate);
    const std::vector<DTestsPdfStream> raws  = d_tests_pdf_streams(raw);
    const std::vector<DTestsPdfStream> flats = d_tests_pdf_streams(flate);
    std::size_t                        same  = 0;

    D_TESTS_PDF_Z_CHECK((raws.size() == 6u) && (flats.size() == 6u));
    D_TESTS_PDF_Z_CHECK(flate.size() < raw.size());

    for (std::size_t i = 0; (i < raws.size()) && (i < flats.size()); ++i)
    {
        const std::string& dict    = flats[i].dictionary;
        const bool         image   = (dict.find("/Image") != std::string::npos);
        std::string        decoded =
            d_tests_pdf_inflate(flats[i].bytes,
                                image ? (raws[i].bytes.size() +
                                         d_tests_pdf_dictionary_number(
                                             dict, "/Height "))
                                      : raws[i].bytes.size());

        if ( (dict.find("/Filter /FlateDecode") == std::string::npos) ||
             (image && (dict.find("/Predictor 15") == std::string::npos)) )
        {
            continue;
        }

        if (image)
        {
            decoded = d_tests_pdf_unpredict(
                          decoded,
                          d_tests_pdf_dictionary_number(dict, "/Columns "),
                          d_tests_pdf_dictionary_number(dict, "/Colors "));
        }

        same += (decoded == raws[i].bytes) ? 1u : 0u;
    }

    D_TESTS_PDF_Z_CHECK(same == 6u);

    return;
}


/*
d_tests_pdf_compress_predictors
  Tests the following:
  - each row's filter is the one with the least sum of its bytes'
    magnitudes as signed bytes, the lowest filter on a tie
  - every row un-predicts to its samples, for one to four colors and
    widths of one to nine pixels
  - a 256 by 256 RGB gradient deflates to under 1,000 bytes predicted,
    and to over 100,000 without, as measured when Flate was chosen
*/
static void
d_tests_pdf_compress_predictors(void)
{
    using djinterp::internal::pdf_png_filter_row;
    using djinterp::internal::pdf_png_predict;

    unsigned    seed      = 1118u;
    bool        round     = true;
    bool        least     = true;
    std::string ramp(256u * 256u * 3u, '\0');
    std::string packed;
    std::string plain;

    for (std::size_t colors = 1; colors <= 4u; ++colors)
    {
        for (std::size_t width = 1; width <= 9u; ++width)
        {
            std::string samples(width * colors * 6u, '\0');
            std::string predicted;

            for (std::size_t i = 0; i < samples.size(); ++i)
            {
                seed       = (seed * 1103515245u) + 12345u;
                samples[i] = static_cast<char>(((i % 3u) == 0u)
                                 ? ((seed >> 16) & 0xffu) : (i * 7u));
            }

            predicted = pdf_png_predict(samples, width, 6u, colors);
            round     = round && (d_tests_pdf_unpredict(predicted, width,
                                                        colors) == samples);

            for (std::size_t y = 0; y < 6u; ++y)
            {
                const std::size_t stride = width * colors;
                const unsigned char* row =
                    reinterpret_cast<const unsigned char*>(
                        samples.data() + (y * stride));
                std::vector<unsigned char> above(stride, 0u);
                std::vector<unsigned char> out(stride, 0u);
                unsigned long              sums[5];
                unsigned                   chosen = 0;

                if (y > 0u)
                {
                    above.assign(row - stride, row);
                }
                for (unsigned f = 0; f < 5u; ++f)
                {
                    sums[f] = pdf_png_filter_row(f, row, above.data(), stride,
                                                 colors, out.data());
                    chosen  = (sums[f] < sums[chosen]) ? f : chosen;
                }
                least = least &&
                        (static_cast<unsigned char>(
                             predicted[y * (stride + 1u)]) == chosen);
            }
        }
    }

    D_TESTS_PDF_Z_CHECK(round);
    D_TESTS_PDF_Z_CHECK(least);

    for (std::size_t y = 0; y < 256u; ++y)
    {
        for (std::size_t x = 0; x < 256u; ++x)
        {
            ramp[((y * 256u) + x) * 3u]      = static_cast<char>(x);
            ramp[((y * 256u) + x) * 3u + 1u] = static_cast<char>(y);
            ramp[((y * 256u) + x) * 3u + 2u] = static_cast<char>((x + y) / 2u);
        }
    }

    D_TESTS_PDF_Z_CHECK(djinterp::internal::pdf_deflate(
                            pdf_png_predict(ramp, 256u, 256u, 3u), 9, packed) &&
                        (packed.size() < 1000u));
    D_TESTS_PDF_Z_CHECK(djinterp::internal::pdf_deflate(ramp, 9, plain) &&
                        (plain.size() > 100000u));

    return;
}


/*
d_tests_pdf_compress_setting
  Tests the following:
  - capabilities().compression follows the setting: flate and the default
    (with the knob on) compress, none doesn't; a later setting wins
  - pdf_options carries the setting to a template, which reports it, and
    leaves a template's own alone at by_default
  - a template set to none renders without a /Filter, and to flate with one
*/
static void
d_tests_pdf_compress_setting(void)
{
    using djinterp::DPdfCompression;

    djinterp::DPdfDocument      doc;
    djinterp::pdf_template      tpl;
    djinterp::DTemplateContext  context;

    doc.set_compression(DPdfCompression::flate);
    D_TESTS_PDF_Z_CHECK(doc.backend().capabilities().compression);
    doc.set_compression(DPdfCompression::none);
    D_TESTS_PDF_Z_CHECK(!doc.backend().capabilities().compression);
    doc.set_compression(DPdfCompression::by_default);
    D_TESTS_PDF_Z_CHECK(doc.backend().capabilities().compression ==
                        (D_INTERNAL_PDF_COMPRESS != 0));

    D_TESTS_PDF_Z_CHECK(tpl.compression() == DPdfCompression::by_default);
    djinterp::pdf_options().set_compression(DPdfCompression::none)
                           .apply_to(tpl);
    D_TESTS_PDF_Z_CHECK(tpl.compression() == DPdfCompression::none);
    djinterp::pdf_options().apply_to(tpl);
    D_TESTS_PDF_Z_CHECK(tpl.compression() == DPdfCompression::none);
    D_TESTS_PDF_Z_CHECK(tpl.render_pdf(context).find("/Filter") ==
                        std::string::npos);
    tpl.compression(DPdfCompression::flate);
    D_TESTS_PDF_Z_CHECK(tpl.render_pdf(context).find("/Filter /FlateDecode") !=
                        std::string::npos);

    return;
}

#endif  // C++17


/*
d_tests_pdf_compress_run
  Opens a test, then runs its group, or skips it below C++17 or, for a group
that needs it, without zlib.
*/
static void
d_tests_pdf_compress_run(
    dj_test_suite* _suite,
    const char*    _name,
    const char*    _description,
    void         (*_group)(void),
    bool           _needs_zlib
)
{
    (void)d_test_suite_test(_suite, _name);
    (void)d_test_suite_describe(_suite, _description);

    if (!_group)
    {
        (void)d_test_suite_skip(_suite, "below C++17, the PDF module's floor");
    }
    else if (_needs_zlib && !d_codec_can_compress(D_CODEC_ID_ZLIB))
    {
        (void)d_test_suite_skip(_suite, "the build has no zlib");
    }
    else
    {
        _group();
    }

    return;
}

// D_TESTS_PDF_Z_GROUP
//   macro: a group, or NULL below C++17.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #define D_TESTS_PDF_Z_GROUP(group) (group)
#else
    #define D_TESTS_PDF_Z_GROUP(group) NULL
#endif


/*
d_tests_pdf_compress_block
  The block for pdf_builtin_backend.hpp's stream compression.
*/
void
d_tests_pdf_compress_block(
    dj_test_suite* _suite
)
{
    g_compress_suite = _suite;

    (void)d_test_suite_block(_suite, "pdf_builtin_backend.hpp");
    d_tests_pdf_compress_run(_suite, "raw",
        "none writes the file the backend wrote before it compressed, byte "
        "for byte; so does pdf_options",
        D_TESTS_PDF_Z_GROUP(d_tests_pdf_compress_raw), false);
    d_tests_pdf_compress_run(_suite, "flate",
        "every stream FlateDecode; each inflates, and each image "
        "un-predicts, to the raw file's stream",
        D_TESTS_PDF_Z_GROUP(d_tests_pdf_compress_flate), true);
    d_tests_pdf_compress_run(_suite, "predictors",
        "each row's filter has the least sum; rows round-trip; a gradient "
        "shrinks as Flate's measurement said",
        D_TESTS_PDF_Z_GROUP(d_tests_pdf_compress_predictors), true);
    d_tests_pdf_compress_run(_suite, "setting",
        "capabilities follow the setting; pdf_options and the template "
        "carry it",
        D_TESTS_PDF_Z_GROUP(d_tests_pdf_compress_setting), true);

    g_compress_suite = NULL;

    return;
}
