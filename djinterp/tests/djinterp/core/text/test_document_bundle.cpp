/*******************************************************************************
* djinterp [core]                                       test_document_bundle.cpp
*
*
* path:      /tests/djinterp/core/text/test_document_bundle.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// T3 verification harness — document_bundle + output_packaging (assembly).
//
// Proves the assembly half of the document-output stack against deterministic
// stub backends (the real compression.hpp / archive.hpp dispatch leaves plug in
// unchanged — same internal:: signatures, real codecs/containers):
//
//   * an empty bundle is a successful no-op;
//   * producers are DEFERRED — never run until write(), exactly once per write();
//   * `none` mode writes each document verbatim, in order, named by the policy;
//   * the default naming policy disambiguates a multi-document run by 1-based
//     index, and a caller policy overrides it;
//   * `compress` mode runs each document through the codec and suffixes its name;
//   * `archive` mode collapses every document into ONE container with the
//     entries named by the policy, written under one (name + ext) pair;
//   * a backend failure (compress or archive) aborts the write as false;
//   * the buffer sink concatenates with a between-blobs separator;
//   * the disk sink writes the exact bytes to the path the policy names;
//   * the bundle assembles fluently.

// std
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
// djinterp
#include "djinterp/core/util/document/document_bundle.hpp"


using djinterp::byte_blob;
using djinterp::bundle_item;
using djinterp::document_bundle;
using djinterp::output_config;
using djinterp::output_sink;
using djinterp::buffer_output_sink;
using djinterp::disk_output_sink;
using djinterp::pack_mode;
using djinterp::write;
using djinterp::write_to_buffer;
using djinterp::write_to_disk;
using djinterp::write_custom;


// a sink that records every (name, bytes) write, for exact-order assertions.
struct record
{
    std::string name;
    byte_blob bytes;
};

class recording_sink : public output_sink
{
public:
    bool
    write_item(
        const std::string&  _name,
        const byte_blob&  _bytes
    ) override
    {
        record _r;
        _r.name  = _name;
        _r.bytes = _bytes;
        records.push_back(_r);

        return true;
    }

    std::vector<record> records;
};


// a producer that tags its output and counts how many times it ran.
static byte_blob
make_payload(
    const std::string&  _tag,
    int&                _counter
)
{
    ++_counter;
    return byte_blob(_tag);
}


int
main()
{
    // 1. empty bundle: a successful no-op; nothing reaches the sink.
    {
        document_bundle _b;
        output_config   _cfg;
        byte_blob     _out;

        assert(_b.empty());
        assert(_b.size() == 0u);
        assert(write_to_buffer(_b, _cfg, _out));
        assert(_out.empty());
    }

    // 2. producers are deferred: not run on add(), run exactly once per write().
    {
        int             _hits_a = 0;
        int             _hits_b = 0;
        document_bundle _b;
        output_config   _cfg;
        recording_sink  _sink;

        _b.add("a", ".txt",
               [&_hits_a]() -> byte_blob
               {
                   return make_payload("AAA", _hits_a);
               })
          .add("b", ".txt",
               [&_hits_b]() -> byte_blob
               {
                   return make_payload("BBB", _hits_b);
               });

        // nothing rendered yet
        assert(_hits_a == 0);
        assert(_hits_b == 0);

        assert(write_custom(_b, _cfg, _sink));
        assert(_hits_a == 1);
        assert(_hits_b == 1);

        // a second write renders once more (each write is a fresh render)
        assert(write_custom(_b, _cfg, _sink));
        assert(_hits_a == 2);
        assert(_hits_b == 2);
    }

    // 3. none mode, default naming: verbatim bytes, in order, 1-based indexed
    //    names for a multi-document run.
    {
        int             _ha = 0;
        int             _hb = 0;
        document_bundle _b;
        output_config   _cfg;   // pack == none by default
        recording_sink  _sink;

        _b.add("alpha", ".txt",
               [&_ha]() -> byte_blob { return make_payload("AAA", _ha); })
          .add("beta", ".md",
               [&_hb]() -> byte_blob { return make_payload("BBB", _hb); });

        assert(write_custom(_b, _cfg, _sink));
        assert(_sink.records.size() == 2u);
        assert(_sink.records[0].name  == "alpha_1.txt");
        assert(_sink.records[0].bytes == "AAA");
        assert(_sink.records[1].name  == "beta_2.md");
        assert(_sink.records[1].bytes == "BBB");
    }

    // 3b. a single-document run gets no index suffix.
    {
        int             _h = 0;
        document_bundle _b;
        output_config   _cfg;
        recording_sink  _sink;

        _b.add("solo", ".txt",
               [&_h]() -> byte_blob { return make_payload("SOLO", _h); });

        assert(write_custom(_b, _cfg, _sink));
        assert(_sink.records.size() == 1u);
        assert(_sink.records[0].name  == "solo.txt");
        assert(_sink.records[0].bytes == "SOLO");
    }

    // 3c. a caller naming policy overrides the default.
    {
        int             _ha = 0;
        int             _hb = 0;
        document_bundle _b;
        output_config   _cfg;
        recording_sink  _sink;

        // ignore index/total: name files exactly "<name><ext>"
        _cfg.naming =
            [](const std::string&  _name,
               const std::string&  _ext,
               std::size_t         _index,
               std::size_t         _total) -> std::string
            {
                (void)_index;
                (void)_total;
                return _name + _ext;
            };

        _b.add("alpha", ".txt",
               [&_ha]() -> byte_blob { return make_payload("A", _ha); })
          .add("beta", ".txt",
               [&_hb]() -> byte_blob { return make_payload("B", _hb); });

        assert(write_custom(_b, _cfg, _sink));
        assert(_sink.records.size() == 2u);
        assert(_sink.records[0].name == "alpha.txt");
        assert(_sink.records[1].name == "beta.txt");
    }

    // 4. compress mode: codec runs and the codec suffix is appended to the name.
    {
        int             _h = 0;
        document_bundle _b;
        output_config   _cfg;
        recording_sink  _sink;

        _cfg.pack  = pack_mode::compress;
        _cfg.codec = djinterp::codec_id_gzip;

        _b.add("r", ".xml",
               [&_h]() -> byte_blob { return make_payload("<x/>", _h); });

        assert(write_custom(_b, _cfg, _sink));
        assert(_sink.records.size() == 1u);
        assert(_sink.records[0].name  == "r.xml.gz");
        assert(_sink.records[0].bytes == "Z(gzip){<x/>}");
    }

    // 5. compress failure aborts the write (lz4 is "unavailable" in the stub).
    {
        int             _h = 0;
        document_bundle _b;
        output_config   _cfg;
        recording_sink  _sink;

        _cfg.pack  = pack_mode::compress;
        _cfg.codec = djinterp::codec_id_lz4;

        _b.add("r", ".xml",
               [&_h]() -> byte_blob { return make_payload("<x/>", _h); });

        assert(!write_custom(_b, _cfg, _sink));
    }

    // 6. archive mode: ONE container, every document an entry (policy-named),
    //    written under one (archive_name + format extension) pair.
    {
        int             _ha = 0;
        int             _hb = 0;
        document_bundle _b;
        output_config   _cfg;
        recording_sink  _sink;

        _cfg.pack         = pack_mode::archive;
        _cfg.format       = djinterp::format_id_zip;
        _cfg.archive_name = "report";

        _b.add("a", ".txt",
               [&_ha]() -> byte_blob { return make_payload("AAA", _ha); })
          .add("b", ".txt",
               [&_hb]() -> byte_blob { return make_payload("BBB", _hb); });

        assert(write_custom(_b, _cfg, _sink));
        assert(_sink.records.size() == 1u);             // a single container
        assert(_sink.records[0].name == "report.zip");
        assert(_sink.records[0].bytes ==
               "A(zip){a_1.txt=AAA|b_2.txt=BBB}");
        assert(_ha == 1);                               // each rendered once
        assert(_hb == 1);
    }

    // 7. archive failure aborts the write (rar is "unavailable" in the stub).
    {
        int             _h = 0;
        document_bundle _b;
        output_config   _cfg;
        recording_sink  _sink;

        _cfg.pack   = pack_mode::archive;
        _cfg.format = djinterp::format_id_rar;

        _b.add("a", ".txt",
               [&_h]() -> byte_blob { return make_payload("AAA", _h); });

        assert(!write_custom(_b, _cfg, _sink));
    }

    // 8. buffer sink: separator between blobs, none leading or trailing.
    {
        int             _ha = 0;
        int             _hb = 0;
        int             _hc = 0;
        document_bundle _b;
        output_config   _cfg;   // none mode
        byte_blob     _out;

        _b.add("a", ".txt",
               [&_ha]() -> byte_blob { return make_payload("A", _ha); })
          .add("b", ".txt",
               [&_hb]() -> byte_blob { return make_payload("B", _hb); })
          .add("c", ".txt",
               [&_hc]() -> byte_blob { return make_payload("C", _hc); });

        assert(write_to_buffer(_b, _cfg, _out, "\n---\n"));
        assert(_out == "A\n---\nB\n---\nC");
    }

    // 9. disk sink: exact bytes (NUL included) written to the policy's path.
    {
        document_bundle   _b;
        output_config     _cfg;   // none mode
        const std::string _path = "/tmp/djt3_disk.txt";

        std::remove(_path.c_str());

        // a payload that really carries an embedded NUL, to exercise binary
        // writing (a text-mode write would truncate or corrupt it).
        byte_blob _bin;
        _bin.push_back('D');
        _bin.push_back('\0');
        _bin.push_back('E');

        _b.add("disk", ".txt",
               [_bin]() -> byte_blob { return _bin; });

        assert(write_to_disk(_b, _cfg,
                   [&_path](const std::string& _name) -> std::string
                   {
                       (void)_name;
                       return _path;   // single file: fixed destination
                   }));

        std::ifstream _in(_path.c_str(), std::ios::binary);
        assert(_in);
        std::ostringstream _ss;
        _ss << _in.rdbuf();
        const std::string _read = _ss.str();
        assert(_read.size() == 3u);
        assert(_read[0] == 'D');
        assert(_read[1] == '\0');
        assert(_read[2] == 'E');

        std::remove(_path.c_str());
    }

    // 10. fluent assembly: chained add() builds an ordered bundle.
    {
        document_bundle _b;

        assert(_b.empty());

        _b.add("x", ".a", []() -> byte_blob { return "X"; })
          .add("y", ".b", []() -> byte_blob { return "Y"; })
          .add(bundle_item("z", ".c",
                           []() -> byte_blob { return "Z"; }));

        assert(!_b.empty());
        assert(_b.size() == 3u);
        assert(_b.items()[0].name == "x");
        assert(_b.items()[1].name == "y");
        assert(_b.items()[2].name == "z");
    }

    std::cout << "all document_bundle / output_packaging cases passed.\n";
    return 0;
}
