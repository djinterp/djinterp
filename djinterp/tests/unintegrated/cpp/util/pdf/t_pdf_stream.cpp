/*******************************************************************************
* djinterp [test]                                               t_pdf_stream.cpp
*
*   The PDF writer on the output stream, from C++: the decimal-point ruling and
* save_to (stream 1), both tiers in one change. internal::pdf_num is held to
* the C tier's d_pdf_num at random, under the C locale and under
* de_DE.UTF-8, whose decimal point is ','; a document the built-in backend
* writes under that locale has its coordinates with '.', where "%g" wrote
* "1,5"; and DPdfDocument::save_to, to a std::string through a DOutputStream,
* writes the bytes to_bytes() gives. C++17 and up, the PDF module's floor;
* below it the program says so and passes.
*   A standalone program, as the other tests not yet in DTest are
* (tests/unintegrated/README.md): a line per group and per failed check, then
* "N checks, M failed", and exit status 0 only when nothing failed.
*
*
* path:      /tests/unintegrated/cpp/util/pdf/t_pdf_stream.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.05
*******************************************************************************/
// std
#include <clocale>  // std::setlocale, LC_ALL
#include <cstdio>   // std::printf
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


// the counts every check adds to
static unsigned long g_checks   = 0;
static unsigned long g_failures = 0;

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// D_TESTS_PDF_CHECK
//   macro: one check: counts it, and prints the expression and line on
// failure.
#define D_TESTS_PDF_CHECK(expr)                                                \
    d_tests_pdf_check(((expr) ? true : false), #expr, __LINE__)


/*
d_tests_pdf_check
  Counts one check, and prints it where it failed.
*/
static void d_tests_pdf_check(
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
d_tests_pdf_parity
  Tests the following:
  - internal::pdf_num writes what d_pdf_num writes, for 20,000 random values,
    under the C locale and under de_DE.UTF-8 where it is installed
*/
static void d_tests_pdf_parity()
{
    const char* const locales[] = { "C", "de_DE.UTF-8" };

    std::printf("  pdf_num against d_pdf_num\n");

    for (std::size_t l = 0; l < (sizeof(locales) / sizeof(locales[0])); ++l)
    {
        if (std::setlocale(LC_ALL, locales[l]) == NULL)
        {
            std::printf("    (skipped: locale %s not installed)\n", locales[l]);

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
                std::printf("    %s: %s against %s\n",
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
    std::printf("  documents\n");

    const bool german = (std::setlocale(LC_ALL, "de_DE.UTF-8") != NULL);

    if (!german)
    {
        std::printf("    (de_DE.UTF-8 not installed: the C locale only)\n");
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
main
  Every group, then the count.
*/
int main()
{
    std::printf("t_pdf_stream (C++%ld)\n", static_cast<long>(__cplusplus));

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    d_tests_pdf_parity();
    d_tests_pdf_document();
#else
    std::printf("  (below C++17, the PDF module's floor: nothing to run)\n");
#endif  // C++17

    std::printf("%lu checks, %lu failed\n", g_checks, g_failures);

    return (g_failures == 0u) ? 0 : 1;
}
