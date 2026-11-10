/*******************************************************************************
* djinterp [test]                                           pdf_stream_tests.cpp
*
*   The PDF writer on the output stream, from C++: the decimal-point ruling and
* save_to (stream 1), both tiers in one change. internal::pdf_num is held to
* the C tier's d_pdf_num at random, under the C locale and under
* de_DE.UTF-8, whose decimal point is ','; a document the built-in backend
* writes under that locale has its coordinates with '.', where "%g" wrote
* "1,5"; and DPdfDocument::save_to, to a std::string through a DOutputStream,
* writes the bytes to_bytes() gives. C++17 and up, the PDF module's floor;
* below it each test is skipped, and says why.
*   In DTest's C face (c/modules/test/test_suite.h): each group is a test of
* the module "pdf_stream_cpp", and each check is recorded against it.
*
*
* path:      /tests/djinterp/cpp/util/pdf/pdf_stream_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.09
*******************************************************************************/
#include "pdf_stream_tests.hpp"  // the suite's entry point, DTest's C face
// std
#include <clocale>  // std::setlocale, LC_ALL
#include <cstdlib>  // std::rand, std::srand, RAND_MAX
#include <cstring>  // std::strcmp
#include <string>   // std::string
// djinterp
#include "../../../../../inc/djinterp/cpp/djinterp.hpp"  // framework root
#include "../../../../../inc/djinterp/c/util/pdf/pdf_builtin_backend.h"
                                                 // d_pdf_num
#include "../../../../../inc/djinterp/cpp/util/pdf/pdf_document.hpp"
                                                 // DPdfDocument, pdf_num
#include "../../../../../inc/djinterp/cpp/util/output/output_stream.hpp"
                                                 // DOutputStream
#include "../../../../../inc/djinterp/cpp/util/output/output_stream_std.hpp"
                                                 // attach_std_string


// g_suite
//   the suite being built, for the checks; set by d_tests_pdf_cpp_all.
static dj_test_suite* g_suite = NULL;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// D_TESTS_PDF_CHECK
//   macro: one check, recorded against the test open in the suite.
#define D_TESTS_PDF_CHECK(expr)                                                \
    d_tests_pdf_check(((expr) ? true : false), #expr, __LINE__)


/*
d_tests_pdf_check
  Records one check against the open test.
*/
static void d_tests_pdf_check(
    bool        _passed,
    const char* _expression,
    int         _line
)
{
    (void)d_test_suite_check(g_suite, _passed, _expression, __FILE__, _line);

    return;
}

/*
d_tests_pdf_parity
  Tests the following:
  - internal::pdf_num writes what d_pdf_num writes, for 20,000 random values,
    under the C locale and under de_DE.UTF-8 where it is installed
*/
static void d_tests_pdf_parity()
{
    const char* const locales[] = { "C", "de_DE.UTF-8" };

    for (std::size_t l = 0; l < (sizeof(locales) / sizeof(locales[0])); ++l)
    {
        if (std::setlocale(LC_ALL, locales[l]) == NULL)
        {
            (void)d_test_suite_note(g_suite,
                                    "left out: locale %s not installed",
                                    locales[l]);

            continue;
        }

        bool same = true;

        std::srand(20261105u);

        for (int i = 0; i < 20000; ++i)
        {
            const double value = (static_cast<double>(std::rand()) -
                                  (RAND_MAX / 2)) /
                                 static_cast<double>((std::rand() % 1000) + 1);
            char         c_tier[D_PDF_NUM_MAX + 1];

            (void)d_pdf_num(value, c_tier, sizeof(c_tier));

            if (djinterp::internal::pdf_num(value) != c_tier)
            {
                (void)d_test_suite_note(
                    g_suite,
                    "%s: %s against %s",
                    locales[l],
                    djinterp::internal::pdf_num(value).c_str(),
                    c_tier);
                same = false;

                break;
            }
        }

        D_TESTS_PDF_CHECK(same);
        D_TESTS_PDF_CHECK(djinterp::internal::pdf_num(1.5) == "1.5");
    }

    (void)std::setlocale(LC_ALL, "C");

    return;
}

/*
d_tests_pdf_document
  Tests the following:
  - a document written under de_DE.UTF-8 has its coordinates with '.': a
    line from (1.5, 2.25) shows "1.5 2.25 m", and no "1,5" anywhere
  - save_to a std::string through a DOutputStream writes to_bytes()'s bytes,
    and leaves the stream healthy
*/
static void d_tests_pdf_document()
{
    const bool german = (std::setlocale(LC_ALL, "de_DE.UTF-8") != NULL);

    if (!german)
    {
        (void)d_test_suite_note(g_suite,
                                "de_DE.UTF-8 not installed: the C locale "
                                "only");
    }

    djinterp::DPdfDocument doc;

    doc.add_page();
    doc.line(djinterp::DPdfPoint(1.5, 2.25), djinterp::DPdfPoint(100.5, 2.25));

    const std::string bytes = doc.to_bytes();

    D_TESTS_PDF_CHECK(bytes.find("1.5 2.25 m") != std::string::npos);
    D_TESTS_PDF_CHECK(bytes.find("1,5") == std::string::npos);

    std::string             streamed;
    djinterp::DOutputStream os;

    D_TESTS_PDF_CHECK(djinterp::attach_std_string(os, streamed));
    D_TESTS_PDF_CHECK(doc.save_to(os));
    D_TESTS_PDF_CHECK(os.close());
    D_TESTS_PDF_CHECK(streamed == bytes);
    D_TESTS_PDF_CHECK(streamed.compare(0, 5, "%PDF-") == 0);

    (void)std::setlocale(LC_ALL, "C");

    return;
}

#endif  // C++17


/*
d_tests_pdf_cpp_all
  Opens the module and its block, then runs every group, each as a test;
below C++17 each test is skipped.
*/
void
d_tests_pdf_cpp_all(
    dj_test_suite* _suite
)
{
    g_suite = _suite;

    (void)d_test_suite_module(_suite, "pdf_stream_cpp");
    (void)d_test_suite_describe(_suite,
                                "the C++ PDF writer's numbers and save_to on "
                                "the output stream");
    (void)d_test_suite_block(_suite, "pdf_document.hpp");

    (void)d_test_suite_test(_suite, "pdf_num against d_pdf_num");
    (void)d_test_suite_describe(_suite,
                                "internal::pdf_num writes what d_pdf_num "
                                "writes, at random, under two locales");
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    d_tests_pdf_parity();
#else
    (void)d_test_suite_skip(_suite, "below C++17, the PDF module's floor");
#endif

    (void)d_test_suite_test(_suite, "documents");
    (void)d_test_suite_describe(_suite,
                                "a document under de_DE keeps '.', and "
                                "save_to writes to_bytes()'s bytes");
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    d_tests_pdf_document();
#else
    (void)d_test_suite_skip(_suite, "below C++17, the PDF module's floor");
#endif

    g_suite = NULL;

    return;
}
