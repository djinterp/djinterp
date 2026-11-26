/*******************************************************************************
* djinterp [cpp][utility]                       document_bundle_tests_bundle.cpp
*
* The document_bundle suite: every block. A recording sink keeps each name and
* each document's bytes in the order they come, and each producer counts how
* often it runs. Below C++17, document_bundle.hpp's floor, the header is empty
* and every test is skipped.
*
*
* path:      /tests/djinterp/cpp/util/document/document_bundle_tests_bundle.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.23
*******************************************************************************/
#include "./document_bundle_tests.hpp"  // corresponding header

// djinterp
#include "../../../../../inc/djinterp/cpp/util/document/document_bundle.hpp"
                                                       // DDocumentBundle

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>     // std::size_t
#include <cstdio>      // std::remove
#include <fstream>     // std::ifstream
#include <functional>  // std::function
#include <iterator>    // std::istreambuf_iterator
#include <string>      // std::string
#include <vector>      // std::vector


namespace
{

// DTestsRecord
//   struct: one write a sink took, its name and its bytes.
struct DTestsRecord
{
    std::string          name;   // the document's name
    djinterp::byte_blob  bytes;  // its bytes
};

// DTestsRecordingSink
//   class: a sink that keeps every write, in order.
class DTestsRecordingSink : public djinterp::output_sink
{
public:
    bool write_item(
        const std::string&         _name,
        const djinterp::byte_blob& _bytes
    ) override
    {
        DTestsRecord record;

        record.name  = _name;
        record.bytes = _bytes;
        m_records.push_back(record);

        return true;
    }

    const std::vector<DTestsRecord>& records() const
    {
        return m_records;
    }

private:
    std::vector<DTestsRecord> m_records;
};

/*
d_tests_document_bundle_producer
  A producer that gives _text and counts its runs in *_runs.
*/
std::function<djinterp::byte_blob()>
d_tests_document_bundle_producer(
    const char* _text,
    int*        _runs
)
{
    const std::string text(_text);

    return [text, _runs]() -> djinterp::byte_blob
           {
               ++*_runs;

               return djinterp::byte_blob(text);
           };
}

/*
d_tests_document_bundle_is
  Whether _record is named _name and holds _bytes.
*/
bool
d_tests_document_bundle_is(
    const DTestsRecord& _record,
    const char*         _name,
    const char*         _bytes
)
{
    return ( (_record.name == _name) &&
             (_record.bytes == _bytes) );
}

}  // namespace


/*
d_tests_document_bundle_empty
  An empty bundle.
  Tests the following:
  - it is empty and has no items
  - writing it to a buffer succeeds and writes nothing
*/
bool
d_tests_document_bundle_empty(void)
{
    djinterp::DDocumentBundle bundle;
    djinterp::output_config   config;
    djinterp::byte_blob       out;
    bool                      ok = true;

    ok = D_TEST_EXPECT(bundle.empty()) && ok;
    ok = D_TEST_EXPECT(bundle.size() == 0u) && ok;
    ok = D_TEST_EXPECT(djinterp::write_to_buffer(bundle, config, out)) && ok;
    ok = D_TEST_EXPECT(out.empty()) && ok;

    return ok;
}

/*
d_tests_document_bundle_deferred
  When producers run.
  Tests the following:
  - adding a document runs nothing
  - each write runs every producer once, a second write once more
*/
bool
d_tests_document_bundle_deferred(void)
{
    int                       runs_a = 0;
    int                       runs_b = 0;
    djinterp::DDocumentBundle bundle;
    djinterp::output_config   config;
    DTestsRecordingSink       sink;
    bool                      ok = true;

    bundle.add("a", ".txt", d_tests_document_bundle_producer("AAA", &runs_a))
          .add("b", ".txt", d_tests_document_bundle_producer("BBB", &runs_b));

    ok = D_TEST_EXPECT( (runs_a == 0) && (runs_b == 0) ) && ok;
    ok = D_TEST_EXPECT(djinterp::write_custom(bundle, config, sink)) && ok;
    ok = D_TEST_EXPECT( (runs_a == 1) && (runs_b == 1) ) && ok;
    ok = D_TEST_EXPECT(djinterp::write_custom(bundle, config, sink)) && ok;
    ok = D_TEST_EXPECT( (runs_a == 2) && (runs_b == 2) ) && ok;

    return ok;
}

/*
d_tests_document_bundle_fluent
  Assembly.
  Tests the following:
  - chained adds, the last of an item made whole, keep their order
*/
bool
d_tests_document_bundle_fluent(void)
{
    int                       runs = 0;
    djinterp::DDocumentBundle bundle;
    bool                      ok = true;

    bundle.add("x", ".a", d_tests_document_bundle_producer("X", &runs))
          .add("y", ".b", d_tests_document_bundle_producer("Y", &runs))
          .add(djinterp::DBundleItem(
                   "z", ".c", d_tests_document_bundle_producer("Z", &runs)));

    ok = D_TEST_EXPECT(!bundle.empty()) && ok;
    D_TEST_REQUIRE(bundle.size() == 3u);
    ok = D_TEST_EXPECT( (bundle.items()[0].name == "x") &&
                        (bundle.items()[1].name == "y") &&
                        (bundle.items()[2].name == "z") ) && ok;
    ok = D_TEST_EXPECT(runs == 0) && ok;

    return ok;
}

/*
d_tests_document_bundle_indexed
  Several documents, the default policy.
  Tests the following:
  - each is written verbatim, in order, its name indexed from 0, as
    output_packaging.hpp's default_base_name says
*/
bool
d_tests_document_bundle_indexed(void)
{
    int                       runs = 0;
    djinterp::DDocumentBundle bundle;
    djinterp::output_config   config;
    DTestsRecordingSink       sink;
    bool                      ok = true;

    bundle.add("alpha", ".txt", d_tests_document_bundle_producer("AAA", &runs))
          .add("beta", ".md", d_tests_document_bundle_producer("BBB", &runs));

    ok = D_TEST_EXPECT(djinterp::write_custom(bundle, config, sink)) && ok;
    D_TEST_REQUIRE(sink.records().size() == 2u);
    ok = D_TEST_EXPECT(d_tests_document_bundle_is(sink.records()[0],
                                                  "alpha_0.txt",
                                                  "AAA")) && ok;
    ok = D_TEST_EXPECT(d_tests_document_bundle_is(sink.records()[1],
                                                  "beta_1.md",
                                                  "BBB")) && ok;

    return ok;
}

/*
d_tests_document_bundle_single
  One document, the default policy.
  Tests the following:
  - its name is its name and extension, with no index
*/
bool
d_tests_document_bundle_single(void)
{
    int                       runs = 0;
    djinterp::DDocumentBundle bundle;
    djinterp::output_config   config;
    DTestsRecordingSink       sink;
    bool                      ok = true;

    bundle.add("solo", ".txt", d_tests_document_bundle_producer("SOLO", &runs));

    ok = D_TEST_EXPECT(djinterp::write_custom(bundle, config, sink)) && ok;
    D_TEST_REQUIRE(sink.records().size() == 1u);
    ok = D_TEST_EXPECT(d_tests_document_bundle_is(sink.records()[0],
                                                  "solo.txt",
                                                  "SOLO")) && ok;

    return ok;
}

/*
d_tests_document_bundle_policy
  A caller's naming policy.
  Tests the following:
  - it replaces the default: here, name and extension with no index
*/
bool
d_tests_document_bundle_policy(void)
{
    int                       runs = 0;
    djinterp::DDocumentBundle bundle;
    djinterp::output_config   config;
    DTestsRecordingSink       sink;
    bool                      ok = true;

    config.naming = [](const std::string& _name,
                       const std::string& _ext,
                       std::size_t        _index,
                       std::size_t        _total) -> std::string
                    {
                        (void)_index;
                        (void)_total;

                        return _name + _ext;
                    };
    bundle.add("alpha", ".txt", d_tests_document_bundle_producer("A", &runs))
          .add("beta", ".txt", d_tests_document_bundle_producer("B", &runs));

    ok = D_TEST_EXPECT(djinterp::write_custom(bundle, config, sink)) && ok;
    D_TEST_REQUIRE(sink.records().size() == 2u);
    ok = D_TEST_EXPECT( (sink.records()[0].name == "alpha.txt") &&
                        (sink.records()[1].name == "beta.txt") ) && ok;

    return ok;
}

/*
d_tests_document_bundle_compress
  Compress mode, through gzip.
  Tests the following:
  - the document is written once, its name the policy's plus ".gz"
  - its bytes are a gzip stream, which reads back as the document
  - where the build has no gzip, the write says it failed instead
*/
bool
d_tests_document_bundle_compress(void)
{
    int                       runs = 0;
    djinterp::DDocumentBundle bundle;
    djinterp::output_config   config;
    DTestsRecordingSink       sink;
    djinterp::byte_blob       back;
    bool                      ok = true;

    config.pack  = djinterp::pack_mode::compress;
    config.codec = djinterp::codec_id_gzip;
    bundle.add("r", ".xml", d_tests_document_bundle_producer("<x/>", &runs));

    if (!djinterp::internal::codec_available(djinterp::codec_id_gzip))
    {
        ok = D_TEST_EXPECT(!djinterp::write_custom(bundle, config, sink)) &&
             ok;
        ok = D_TEST_EXPECT(sink.records().empty()) && ok;

        return ok;
    }

    ok = D_TEST_EXPECT(djinterp::write_custom(bundle, config, sink)) && ok;
    D_TEST_REQUIRE(sink.records().size() == 1u);
    ok = D_TEST_EXPECT(sink.records()[0].name == "r.xml.gz") && ok;
    ok = D_TEST_EXPECT( (sink.records()[0].bytes.size() > 2u)         &&
                        (sink.records()[0].bytes[0] == '\x1f')        &&
                        (sink.records()[0].bytes[1] == '\x8b') ) && ok;
    ok = D_TEST_EXPECT(djinterp::try_decompress<djinterp::codecs::gzip>(
                           sink.records()[0].bytes, back) ==
                       djinterp::status_ok) && ok;
    ok = D_TEST_EXPECT(back == "<x/>") && ok;

    return ok;
}

/*
d_tests_document_bundle_compress_refused
  A codec no build has.
  Tests the following:
  - the write fails, and nothing reaches the sink
*/
bool
d_tests_document_bundle_compress_refused(void)
{
    int                       runs = 0;
    djinterp::DDocumentBundle bundle;
    djinterp::output_config   config;
    DTestsRecordingSink       sink;
    bool                      ok = true;

    config.pack  = djinterp::pack_mode::compress;
    // one past the last codec: inside the enum's range, so a valid value,
    // but no codec
    config.codec = static_cast<djinterp::DCodecId>(D_CODEC_ID_COUNT);
    bundle.add("r", ".xml", d_tests_document_bundle_producer("<x/>", &runs));

    ok = D_TEST_EXPECT(!djinterp::write_custom(bundle, config, sink)) && ok;
    ok = D_TEST_EXPECT(sink.records().empty()) && ok;

    return ok;
}

/*
d_tests_document_bundle_archive
  Archive mode, as a ZIP.
  Tests the following:
  - one container reaches the sink, named the archive's name plus ".zip"
  - it reads back as every document, each named by the policy, in order
  - each producer ran once
  - where the build cannot write ZIP, the write says it failed instead
*/
bool
d_tests_document_bundle_archive(void)
{
    int                       runs_a = 0;
    int                       runs_b = 0;
    djinterp::DDocumentBundle bundle;
    djinterp::output_config   config;
    DTestsRecordingSink       sink;
    djinterp::entry_list      members;
    bool                      ok = true;

    config.pack         = djinterp::pack_mode::archive;
    config.format       = djinterp::format_id_zip;
    config.archive_name = "report";
    bundle.add("a", ".txt", d_tests_document_bundle_producer("AAA", &runs_a))
          .add("b", ".txt", d_tests_document_bundle_producer("BBB", &runs_b));

    if (!djinterp::format_is_writable<djinterp::formats::zip>())
    {
        ok = D_TEST_EXPECT(!djinterp::write_custom(bundle, config, sink)) &&
             ok;

        return ok;
    }

    ok = D_TEST_EXPECT(djinterp::write_custom(bundle, config, sink)) && ok;
    D_TEST_REQUIRE(sink.records().size() == 1u);
    ok = D_TEST_EXPECT(sink.records()[0].name == "report.zip") && ok;
    ok = D_TEST_EXPECT(djinterp::try_extract<djinterp::formats::zip>(
                           sink.records()[0].bytes, members) ==
                       djinterp::status_ok) && ok;
    D_TEST_REQUIRE(members.size() == 2u);
    ok = D_TEST_EXPECT( (members[0].name == "a_0.txt") &&
                        (members[0].data == "AAA")     &&
                        (members[1].name == "b_1.txt") &&
                        (members[1].data == "BBB") ) && ok;
    ok = D_TEST_EXPECT( (runs_a == 1) && (runs_b == 1) ) && ok;

    return ok;
}

/*
d_tests_document_bundle_archive_refused
  A format no build has.
  Tests the following:
  - the write fails, and nothing reaches the sink
*/
bool
d_tests_document_bundle_archive_refused(void)
{
    int                       runs = 0;
    djinterp::DDocumentBundle bundle;
    djinterp::output_config   config;
    DTestsRecordingSink       sink;
    bool                      ok = true;

    config.pack   = djinterp::pack_mode::archive;
    // one past the last format: a valid value of the enum, but no format
    config.format = static_cast<djinterp::format_id>(D_FORMAT_ID_COUNT);
    bundle.add("a", ".txt", d_tests_document_bundle_producer("AAA", &runs));

    ok = D_TEST_EXPECT(!djinterp::write_custom(bundle, config, sink)) && ok;
    ok = D_TEST_EXPECT(sink.records().empty()) && ok;

    return ok;
}

/*
d_tests_document_bundle_buffer
  The buffer sink.
  Tests the following:
  - its separator goes between documents, never before the first or after
    the last
*/
bool
d_tests_document_bundle_buffer(void)
{
    int                       runs = 0;
    djinterp::DDocumentBundle bundle;
    djinterp::output_config   config;
    djinterp::byte_blob       out;
    bool                      ok = true;

    bundle.add("a", ".txt", d_tests_document_bundle_producer("A", &runs))
          .add("b", ".txt", d_tests_document_bundle_producer("B", &runs))
          .add("c", ".txt", d_tests_document_bundle_producer("C", &runs));

    ok = D_TEST_EXPECT(djinterp::write_to_buffer(bundle,
                                                 config,
                                                 out,
                                                 "\n---\n")) && ok;
    ok = D_TEST_EXPECT(out == "A\n---\nB\n---\nC") && ok;

    return ok;
}

/*
d_tests_document_bundle_disk
  The disk sink.
  Tests the following:
  - it writes the exact bytes, an embedded NUL among them, to the path the
    caller's function gives, in the working directory
*/
bool
d_tests_document_bundle_disk(void)
{
    static const char         path[] = "document_bundle_tests_disk.bin";
    const djinterp::byte_blob bytes("D\0E", 3u);
    djinterp::DDocumentBundle bundle;
    djinterp::output_config   config;
    std::string               read;
    bool                      ok = true;

    (void)std::remove(path);
    bundle.add("disk", ".txt",
               [bytes]() -> djinterp::byte_blob { return bytes; });

    ok = D_TEST_EXPECT(djinterp::write_to_disk(
                           bundle,
                           config,
                           [](const std::string& _name) -> std::string
                           {
                               (void)_name;

                               return std::string(path);
                           })) && ok;

    {
        std::ifstream in(path, std::ios::binary);

        read.assign(std::istreambuf_iterator<char>(in),
                    std::istreambuf_iterator<char>());
    }

    ok = D_TEST_EXPECT(read == bytes) && ok;
    (void)std::remove(path);

    return ok;
}

#else

// D_TESTS_DOCUMENT_BUNDLE_BELOW_FLOOR
//   macro: a test that is skipped below C++17, document_bundle.hpp's floor.
#define D_TESTS_DOCUMENT_BUNDLE_BELOW_FLOOR(_name)                            \
    bool _name(void)                                                          \
    {                                                                         \
        D_TEST_SKIP("below C++17, document_bundle.hpp's floor");             \
    }

D_TESTS_DOCUMENT_BUNDLE_BELOW_FLOOR(d_tests_document_bundle_empty)
D_TESTS_DOCUMENT_BUNDLE_BELOW_FLOOR(d_tests_document_bundle_deferred)
D_TESTS_DOCUMENT_BUNDLE_BELOW_FLOOR(d_tests_document_bundle_fluent)
D_TESTS_DOCUMENT_BUNDLE_BELOW_FLOOR(d_tests_document_bundle_indexed)
D_TESTS_DOCUMENT_BUNDLE_BELOW_FLOOR(d_tests_document_bundle_single)
D_TESTS_DOCUMENT_BUNDLE_BELOW_FLOOR(d_tests_document_bundle_policy)
D_TESTS_DOCUMENT_BUNDLE_BELOW_FLOOR(d_tests_document_bundle_compress)
D_TESTS_DOCUMENT_BUNDLE_BELOW_FLOOR(d_tests_document_bundle_compress_refused)
D_TESTS_DOCUMENT_BUNDLE_BELOW_FLOOR(d_tests_document_bundle_archive)
D_TESTS_DOCUMENT_BUNDLE_BELOW_FLOOR(d_tests_document_bundle_archive_refused)
D_TESTS_DOCUMENT_BUNDLE_BELOW_FLOOR(d_tests_document_bundle_buffer)
D_TESTS_DOCUMENT_BUNDLE_BELOW_FLOOR(d_tests_document_bundle_disk)

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
