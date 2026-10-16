/*******************************************************************************
* djinterp [test]                                              archive_tests.hpp
*
*   Declarations for the archive.hpp / archive.cpp unit-test suite, plus the
* small shared helpers and the per-suite check macros the section translation
* units use.  Each tests_* function is defined in a section .cpp beside this
* header (one section per like-group of the module), flat in djinterp::testing,
* and returns true on success.  The runner (config tree) links the sections and
* calls each function.
*
*   The suite drives the facade's PUBLIC surface for behaviour and reaches into
* djinterp::internal only where 100% coverage demands it (the dispatch leaves'
* argument guards and unknown-format default arms, which the templated public
* API can never reach with a bad format id).  Structural checks on produced
* bytes go through the shared djinterp::test archive verifiers
* (test_archive.hpp).
*
*   CHECK MACROS:
*   Unique-lettered per suite (D_AR_) so co-compiled suites never collide.  Each
* test declares  bool _ok = true;  runs D_AR_CHECK / D_AR_CHECK_STATUS (which
* report a failing check with its source location and clear _ok, but keep
* going so every check in the body is exercised) and D_AR_REQUIRE (which bails
* out immediately when continuing would be unsafe -- e.g. an extract that must
* succeed before its entries are inspected), and ends with  return _ok;.
*
*
* path:      /tests/djinterp/core/util/archive/archive_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_ARCHIVE_TESTS_
#define DJINTERP_ARCHIVE_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>
// djinterp
#include "../../../../../inc/djinterp/core/util/archive/archive.hpp"                      // the module under test (+ internal)
#include "djinterp/test/archive/test_archive.hpp"   // shared archive verifiers


NS_DJINTERP
NS_TESTING

// =============================================================================
// SHARED HELPERS
// =============================================================================

// repeat_bytes
//   function: a buffer of _n copies of _c -- a highly compressible payload for
// exercising the DEFLATE path.
//
// Parameter(s):
//   _c: the byte to repeat.
//   _n: the length of the result.
// Return:
//   the constructed buffer.
inline byte_blob
repeat_bytes(
    char        _c,
    std::size_t _n
)
{
    return byte_blob(_n, _c);
}

// binary_sample
//   function: a fixed byte buffer containing embedded NULs and high-bit bytes,
// used to prove the containers are binary-clean (not NUL-terminated).
//
// Parameter(s):
//   none.
// Return:
//   the sample buffer (11 bytes).
inline byte_blob
binary_sample()
{
    static const char raw[] = { 'a', '\0', 'b', '\0', '\0', 'c',
                                (char)0xFF, (char)0x00, (char)0x80,
                                (char)0x7F, 'z' };

    return byte_blob(raw, sizeof(raw));
}


// =============================================================================
// TEST DECLARATIONS  (grouped by module section; defined in the section .cpp)
// =============================================================================

// entry model  (archive_tests_entry.cpp)
bool tests_entry_defaults();
bool tests_entry_list_basics();
bool tests_entry_helpers();

// format tags / traits / capability queries  (archive_tests_traits.cpp)
bool tests_format_traits_identity();
bool tests_format_traits_capability_flags();
bool tests_format_capability_queries();

// tar (ustar) writer / reader  (archive_tests_tar.cpp)
bool tests_tar_roundtrip_files();
bool tests_tar_directories();
bool tests_tar_metadata();
bool tests_tar_name_truncation();
bool tests_tar_checksum();
bool tests_tar_extract_truncated();
bool tests_tar_empty_archive();

// zip writer / reader  (archive_tests_zip.cpp)
bool tests_zip_roundtrip_files();
bool tests_zip_store_mode();
bool tests_zip_deflate_mode();
bool tests_zip_directories();
bool tests_zip_empty_archive();
bool tests_zip_unsupported_method();
bool tests_zip_extract_truncated();
bool tests_zip_extract_corrupt_deflate();

// gz and tar.gz  (archive_tests_gz.cpp)
bool tests_gz_requires_single_entry();
bool tests_gz_roundtrip();
bool tests_gz_extract_corrupt();
bool tests_targz_roundtrip_files();
bool tests_targz_empty_archive();
bool tests_targz_extract_corrupt();

// 7z / rar backends  (archive_tests_backends.cpp)
bool tests_sevenzip_capability_consistency();
bool tests_rar_capability_consistency();

// dispatch leaves + argument guards  (archive_tests_dispatch.cpp)
bool tests_dispatch_null_base_guard();
bool tests_dispatch_unknown_format();
bool tests_dispatch_capability_switches();

// non-throwing edges + throwing convenience API  (archive_tests_api.cpp)
bool tests_try_api_empty_list();
bool tests_throwing_roundtrip();
bool tests_throwing_error_on_create();
bool tests_throwing_error_on_extract();


NS_END  // testing
NS_END  // djinterp


// =============================================================================
// PER-SUITE CHECK MACROS  (letters: AR)
// =============================================================================

// D_AR_CHECK
//   macro: record a boolean expectation against the enclosing test's `_ok`
// flag.  On failure it prints the stringized expression with its source
// location and clears `_ok`, but execution continues so every check runs.
#define D_AR_CHECK(_cond)                                                      \
    do                                                                         \
    {                                                                          \
        if (!(_cond))                                                          \
        {                                                                      \
            std::printf("      [expect] FAIL: %s  (%s:%d)\n",                  \
                        #_cond, __FILE__, __LINE__);                           \
            _ok = false;                                                       \
        }                                                                      \
    }                                                                          \
    while (false)

// D_AR_REQUIRE
//   macro: a hard precondition.  On failure it prints and returns false at
// once -- for cases where continuing would be unsafe (an operation that must
// succeed before its result is inspected).
#define D_AR_REQUIRE(_cond)                                                    \
    do                                                                         \
    {                                                                          \
        if (!(_cond))                                                          \
        {                                                                      \
            std::printf("      [require] FAIL: %s  (%s:%d)\n",                 \
                        #_cond, __FILE__, __LINE__);                           \
            return false;                                                      \
        }                                                                      \
    }                                                                          \
    while (false)

// D_AR_CHECK_STATUS
//   macro: evaluate a status expression once and compare it to an expected
// status, printing both human-readable names on a mismatch and clearing
// `_ok`.  Keeps the common "call returned the right status code" assertion
// readable.
#define D_AR_CHECK_STATUS(_expr, _expected)                                   \
    do                                                                         \
    {                                                                          \
        ::djinterp::status _ar_got = (_expr);                                  \
        ::djinterp::status _ar_exp = (_expected);                              \
        if (_ar_got != _ar_exp)                                               \
        {                                                                      \
            std::printf("      [status] FAIL: %s  ->  got '%s', want '%s'"     \
                        "  (%s:%d)\n",                                         \
                        #_expr,                                                \
                        ::djinterp::status_message(_ar_got),                  \
                        ::djinterp::status_message(_ar_exp),                  \
                        __FILE__, __LINE__);                                   \
            _ok = false;                                                       \
        }                                                                      \
    }                                                                          \
    while (false)


#endif  // DJINTERP_ARCHIVE_TESTS_
