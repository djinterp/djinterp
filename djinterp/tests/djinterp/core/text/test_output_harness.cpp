/*******************************************************************************
* djinterp [core]                                        test_output_harness.cpp
*
*
* path:      /tests/djinterp/core/text/test_output_harness.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// T4 verification harness — test_output (report -> multi-format bundle -> write).
//
// Compiles the REAL test_output.hpp against the REAL document_bundle /
// output_packaging and stub engines (pdf_template / document<_Policy>) + a stub
// test_document whose render_run_flow / render_run_tree emit recognizable
// markers and count their invocations.  The real engines plug in unchanged at
// the same named call sites; what is proven here is test_output's own logic:
//
//   * format_extension maps every format to its document extension;
//   * per-format dispatch: pdf + text/markdown take the FLOW skeleton, xml +
//     html take the TREE skeleton (asserted via the markers each emits);
//   * render is DEFERRED: building a bundle renders nothing; write() renders
//     each format exactly once;
//   * `none` mode writes one file per format, named with a 1-based index for a
//     multi-format run and the format's extension;
//   * a single-format run gets no index suffix;
//   * `archive` mode collapses every format into ONE container, entries named
//     by the policy;
//   * emit_report_to_buffer builds + writes in one call, separator between docs.

// std
#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>
// djinterp


using djinterp::byte_blob;
using djinterp::document_bundle;
using djinterp::output_config;
using djinterp::output_sink;
using djinterp::pack_mode;
using djinterp::write_custom;

using djinterp::test::test_report;
using djinterp::test::test_document;
using djinterp::test::doc_format;
using djinterp::test::format_extension;
using djinterp::test::render_report_bytes;
using djinterp::test::build_report_bundle;
using djinterp::test::emit_report_to_buffer;


struct rec
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
        rec _r;
        _r.name  = _name;
        _r.bytes = _bytes;
        records.push_back(_r);

        return true;
    }

    std::vector<rec> records;
};


static bool
starts_with(
    const std::string&  _s,
    const std::string&  _p
)
{
    return _s.size() >= _p.size() && _s.compare(0, _p.size(), _p) == 0;
}

static bool
contains(
    const std::string&  _s,
    const std::string&  _p
)
{
    return _s.find(_p) != std::string::npos;
}


int
main()
{
    test_report   _run;
    _run.title = "R";
    test_document _doc;

    // 1. format_extension: every format -> its document extension.
    {
        assert(std::string(format_extension(doc_format::text))     == ".txt");
        assert(std::string(format_extension(doc_format::markdown)) == ".md");
        assert(std::string(format_extension(doc_format::xml))      == ".xml");
        assert(std::string(format_extension(doc_format::html))     == ".html");
        assert(std::string(format_extension(doc_format::pdf))      == ".pdf");
    }

    // 2. per-format dispatch: pdf + text via FLOW, xml + html via TREE.
    {
        const byte_blob _pdf = render_report_bytes(_doc, _run, doc_format::pdf);
        assert(starts_with(_pdf, "PDF{"));        // pdf_template engine
        assert(contains(_pdf, "TITLE:R"));        // flow skeleton ran

        const byte_blob _txt = render_report_bytes(_doc, _run, doc_format::text);
        assert(contains(_txt, "TITLE:R"));        // same flow skeleton
        assert(contains(_txt, "\n"));             // newline-joined lines
        assert(!starts_with(_txt, "PDF{"));       // NOT the pdf engine

        const byte_blob _xml = render_report_bytes(_doc, _run, doc_format::xml);
        assert(starts_with(_xml, "xml{"));        // xml policy
        assert(contains(_xml, "<report><title>R<verdict>PASS"));  // tree ran

        const byte_blob _html = render_report_bytes(_doc, _run, doc_format::html);
        assert(starts_with(_html, "html{"));      // html policy

        const byte_blob _md = render_report_bytes(_doc, _run, doc_format::markdown);
        assert(contains(_md, "TITLE:R"));         // md rides the text path
    }

    // 3. DEFERRED render: building renders nothing; write renders once / format.
    {
        djinterp::test::g_flow_calls = 0;
        djinterp::test::g_tree_calls = 0;

        std::vector<doc_format> _fmts;
        _fmts.push_back(doc_format::pdf);         // flow
        _fmts.push_back(doc_format::xml);         // tree

        document_bundle _b = build_report_bundle(_doc, _run, _fmts, "report");

        assert(djinterp::test::g_flow_calls == 0);   // nothing rendered on build
        assert(djinterp::test::g_tree_calls == 0);

        recording_sink _sink;
        output_config  _cfg;                          // none mode, default naming
        assert(write_custom(_b, _cfg, _sink));

        assert(djinterp::test::g_flow_calls == 1);   // pdf rendered once
        assert(djinterp::test::g_tree_calls == 1);   // xml rendered once

        assert(_sink.records.size() == 2u);
        assert(_sink.records[0].name == "report_1.pdf");
        assert(starts_with(_sink.records[0].bytes, "PDF{"));
        assert(_sink.records[1].name == "report_2.xml");
        assert(starts_with(_sink.records[1].bytes, "xml{"));
    }

    // 4. single format: no index suffix.
    {
        std::vector<doc_format> _fmts;
        _fmts.push_back(doc_format::html);

        document_bundle _b = build_report_bundle(_doc, _run, _fmts, "summary");
        recording_sink  _sink;
        output_config   _cfg;
        assert(write_custom(_b, _cfg, _sink));

        assert(_sink.records.size() == 1u);
        assert(_sink.records[0].name == "summary.html");
        assert(starts_with(_sink.records[0].bytes, "html{"));
    }

    // 5. archive mode: every format an entry of ONE container.
    {
        std::vector<doc_format> _fmts;
        _fmts.push_back(doc_format::pdf);
        _fmts.push_back(doc_format::xml);

        document_bundle _b = build_report_bundle(_doc, _run, _fmts, "report");
        recording_sink  _sink;
        output_config   _cfg;
        _cfg.pack         = pack_mode::archive;
        _cfg.format       = djinterp::format_id_zip;
        _cfg.archive_name = "report";

        assert(write_custom(_b, _cfg, _sink));
        assert(_sink.records.size() == 1u);                    // one container
        assert(_sink.records[0].name == "report.zip");
        assert(starts_with(_sink.records[0].bytes, "A(zip){"));
        assert(contains(_sink.records[0].bytes, "report_1.pdf=PDF{"));
        assert(contains(_sink.records[0].bytes, "report_2.xml=xml{"));
    }

    // 6. emit to buffer: build + write in one call, separator between docs.
    {
        std::vector<doc_format> _fmts;
        _fmts.push_back(doc_format::text);
        _fmts.push_back(doc_format::xml);

        byte_blob   _out;
        output_config _cfg;                                    // none mode
        assert(emit_report_to_buffer(_doc, _run, _fmts, _cfg, _out,
                                     "\n====\n", "report"));
        assert(contains(_out, "TITLE:R"));                     // text part
        assert(contains(_out, "xml{"));                        // xml part
        assert(contains(_out, "\n====\n"));                    // separator
    }

    std::cout << "all test_output cases passed.\n";
    return 0;
}
