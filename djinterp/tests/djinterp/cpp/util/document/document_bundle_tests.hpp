/*******************************************************************************
* djinterp [cpp][utility]                              document_bundle_tests.hpp
*
* The suite for cpp/util/document/document_bundle.hpp and the packaging it
* writes through, output_packaging.hpp, in DTest's C spec form: a bundle
* collects documents whose producers run only when it is written, and a write
* names each one by a policy and hands it to a sink verbatim, compressed, or
* gathered into one archive. The codecs and formats are the real ones; a test
* that needs one the build lacks checks that the write says so.
*
*
* path:      /tests/djinterp/cpp/util/document/document_bundle_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.23
*******************************************************************************/
/*
TABLE OF CONTENTS
=================
1.  TESTS
    -----
    1.  The bundle
    2.  Names
    3.  Packing
    4.  Sinks
2.  SPEC
    ----
    1.  The provider
*/
#ifndef DJINTERP_TESTS_CPP_UTIL_DOCUMENT_BUNDLE_TESTS_HPP
#define DJINTERP_TESTS_CPP_UTIL_DOCUMENT_BUNDLE_TESTS_HPP 1
// djinterp
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*


//==============================================================================
// 1.  TESTS
//==============================================================================


// 1.1    The bundle
//------------------------------------------------------------------------------
bool d_tests_document_bundle_empty(void);
bool d_tests_document_bundle_deferred(void);
bool d_tests_document_bundle_fluent(void);

// 1.2    Names
//------------------------------------------------------------------------------
bool d_tests_document_bundle_indexed(void);
bool d_tests_document_bundle_single(void);
bool d_tests_document_bundle_policy(void);

// 1.3    Packing
//------------------------------------------------------------------------------
bool d_tests_document_bundle_compress(void);
bool d_tests_document_bundle_compress_refused(void);
bool d_tests_document_bundle_archive(void);
bool d_tests_document_bundle_archive_refused(void);

// 1.4    Sinks
//------------------------------------------------------------------------------
bool d_tests_document_bundle_buffer(void);
bool d_tests_document_bundle_disk(void);


//==============================================================================
// 2.  SPEC
//==============================================================================


// 2.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE
// d_tests_document_bundle_bundle_rows
//   constant: the bundle block's rows.
static const struct dj_test_case_spec d_tests_document_bundle_bundle_rows[] =
{
    D_TEST_CASE(d_tests_document_bundle_empty,
                "an empty bundle writes nothing, successfully"),
    D_TEST_CASE(d_tests_document_bundle_deferred,
                "producers run at a write, not an add, once per write"),
    D_TEST_CASE(d_tests_document_bundle_fluent,
                "chained adds keep their order, an item added whole too")
};

// d_tests_document_bundle_name_rows
//   constant: the names block's rows.
static const struct dj_test_case_spec d_tests_document_bundle_name_rows[] =
{
    D_TEST_CASE(d_tests_document_bundle_indexed,
                "several documents: verbatim, in order, indexed from 0"),
    D_TEST_CASE(d_tests_document_bundle_single,
                "one document: its name and extension, no index"),
    D_TEST_CASE(d_tests_document_bundle_policy,
                "a caller's naming policy replaces the default")
};

// d_tests_document_bundle_pack_rows
//   constant: the packing block's rows.
static const struct dj_test_case_spec d_tests_document_bundle_pack_rows[] =
{
    D_TEST_CASE(d_tests_document_bundle_compress,
                "compress: gzip's bytes under the name plus .gz"),
    D_TEST_CASE(d_tests_document_bundle_compress_refused,
                "a codec no build has fails the write, and writes nothing"),
    D_TEST_CASE(d_tests_document_bundle_archive,
                "archive: one ZIP of every document, named by the policy"),
    D_TEST_CASE(d_tests_document_bundle_archive_refused,
                "a format no build has fails the write, and writes nothing")
};

// d_tests_document_bundle_sink_rows
//   constant: the sinks block's rows.
static const struct dj_test_case_spec d_tests_document_bundle_sink_rows[] =
{
    D_TEST_CASE(d_tests_document_bundle_buffer,
                "the buffer sink puts its separator between documents"),
    D_TEST_CASE(d_tests_document_bundle_disk,
                "the disk sink writes the exact bytes, a NUL among them")
};

// d_tests_document_bundle_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_document_bundle_blocks[] =
{
    { "bundle",
      "collecting documents, and when they are made",
      d_tests_document_bundle_bundle_rows,
      sizeof(d_tests_document_bundle_bundle_rows) /
          sizeof(d_tests_document_bundle_bundle_rows[0]) },
    { "names",
      "the names a write gives its documents",
      d_tests_document_bundle_name_rows,
      sizeof(d_tests_document_bundle_name_rows) /
          sizeof(d_tests_document_bundle_name_rows[0]) },
    { "packing",
      "documents compressed, or gathered into an archive",
      d_tests_document_bundle_pack_rows,
      sizeof(d_tests_document_bundle_pack_rows) /
          sizeof(d_tests_document_bundle_pack_rows[0]) },
    { "sinks",
      "where the documents go: a buffer, or the disk",
      d_tests_document_bundle_sink_rows,
      sizeof(d_tests_document_bundle_sink_rows) /
          sizeof(d_tests_document_bundle_sink_rows[0]) }
};

// d_tests_document_bundle_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_document_bundle_module =
{
    "document_bundle",
    "cpp/util/document: documents made late and written together",
    d_tests_document_bundle_blocks,
    sizeof(d_tests_document_bundle_blocks) /
        sizeof(d_tests_document_bundle_blocks[0])
};

// d_tests_document_bundle_spec
//   function: the suite's provider.
inline
const struct dj_test_module_spec*
d_tests_document_bundle_spec(void)
{
    return &d_tests_document_bundle_module;
}

#endif  // DTEST_SPEC_MODE


#endif  // DJINTERP_TESTS_CPP_UTIL_DOCUMENT_BUNDLE_TESTS_HPP
