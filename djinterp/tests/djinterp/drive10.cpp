/*******************************************************************************
* djinterp [core]                                                    drive10.cpp
*
*
* path:      /tests/djinterp/drive10.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <fstream>
#include <iostream>
// djinterp
#include "djinterp/core/util/pdf/pdf_canvas_document_renderer.hpp"
#include "djinterp/core/util/document/templates/document_dialect.hpp"
#include "djinterp/test/output/test_report_document.hpp"

using namespace djinterp;
using namespace djinterp::test;

static test_report make_report()
{
    test_report rep;
    rep.title = "Nightly Regression"; rep.subtitle = "build 4412";
    rep.author = "teer";
    rep.description = "Sweep over <core> & util.";
    const char* mods[] = { "meta", "container" };
    for (int mi = 0; mi < 2; ++mi)
    {
        report_module m; m.name = mods[mi]; m.description = "subsystem";
        for (int ui = 0; ui < 2; ++ui)
        {
            report_unit u;
            u.name = std::string(mods[mi]) + "_unit_" + std::to_string(ui);
            u.description = "detection idiom";
            test_status st[] = { test_status::passed, test_status::failed,
                                 test_status::skipped };
            for (int i = 0; i < 3; ++i)
                u.checks.push_back({"c", st[i],
                                    std::string("check_") + std::to_string(i),
                                    "true", (i==1?"false":"true")});
            m.units.push_back(u);
        }
        rep.modules.push_back(m);
    }
    return rep;
}

int main()
{
    const test_report rep = make_report();
    report_document_options opts;
    opts.page_break_per_module = true;

    // --- PDF, canvas-backed -------------------------------------------------
    pdf_canvas_document_renderer pr;
    canvas_style pass; pass.color = pdf_color(0.0, 0.5, 0.15);
    canvas_style fail; fail.color = pdf_color(0.8, 0.05, 0.05);
    canvas_style skip; skip.color = pdf_color(0.45, 0.45, 0.45);
    pass.align = fail.align = skip.align = pdf_text_align::center;
    pr.register_style("verdict.pass", pass);
    pr.register_style("verdict.fail", fail);
    pr.register_style("verdict.skip", skip);
    pr.set_zebra(pdf_color::from_rgb255(244, 245, 248));

    pr.begin_document(doc_attributes());
    render_report_document(rep, pr, opts);
    pr.end_document();
    const std::string pdf = pr.to_bytes();
    std::ofstream("full.pdf", std::ios::binary).write(pdf.data(), pdf.size());

    // --- the same composition, other dialects -------------------------------
    for (document_format f : { document_format::html, document_format::text })
    {
        document_dialect d = make_document_dialect(f);
        d.renderer().begin_document(doc_attributes());
        render_report_document(rep, d.renderer(), opts);
        d.renderer().end_document();
        std::ofstream(std::string("full.") + format_name(f)) << d.str();
        std::cout << format_name(f) << ": " << d.str().size() << " bytes\n";
    }
    std::cout << "pdf: " << pdf.size() << " bytes\n";
    return 0;
}
