/*******************************************************************************
* djinterp [core]                                                     drive9.cpp
*
*
* path:      /tests/djinterp/drive9.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <fstream>
#include <iostream>
// djinterp
#include "djinterp/core/util/pdf/pdf_canvas_document_renderer.hpp"
#include "djinterp/test/output/test_report_content.hpp"

using namespace djinterp;
using namespace djinterp::test;

int main()
{
    test_report rep;
    rep.title = "Nightly";
    report_module m; m.name = "meta";
    report_unit u; u.name = "trait_detect";
    const char* names[] = {"alpha","beta","gamma","delta","epsilon"};
    test_status st[] = { test_status::passed, test_status::failed,
                         test_status::passed, test_status::skipped,
                         test_status::error };
    for (int i = 0; i < 5; ++i)
        u.checks.push_back({names[i], st[i], std::string("check_") + names[i],
                            "true", (i==1?"false":"true")});
    m.units.push_back(u);
    rep.modules.push_back(m);

    pdf_canvas_document_renderer r;

    // the verdict vocabulary emit_unit_table already emits, given colour
    canvas_style pass;  pass.color  = pdf_color(0.0, 0.50, 0.15);
    canvas_style fail;  fail.color  = pdf_color(0.80, 0.05, 0.05);
    canvas_style skip;  skip.color  = pdf_color(0.45, 0.45, 0.45);
    canvas_style err;   err.color   = pdf_color(0.60, 0.30, 0.00);
    pass.align = fail.align = skip.align = err.align = pdf_text_align::center;
    r.register_style("verdict.pass",  pass);
    r.register_style("verdict.fail",  fail);
    r.register_style("verdict.skip",  skip);
    r.register_style("verdict.error", err);
    r.set_zebra(pdf_color::from_rgb255(244, 245, 248));

    r.begin_document(doc_attributes());
    r.heading(1, rep.title, doc_attributes());
    resolve_report_content(rep, content_key_unit(0, 0), r);
    r.end_document();

    const std::string b = r.to_bytes();
    std::ofstream("styled.pdf", std::ios::binary).write(b.data(), b.size());
    std::cout << "bytes: " << b.size() << "\n";
    return 0;
}
