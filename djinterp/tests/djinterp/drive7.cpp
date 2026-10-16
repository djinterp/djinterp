/*******************************************************************************
* djinterp [core]                                                     drive7.cpp
*
*
* path:      /tests/djinterp/drive7.cpp
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
    rep.title = "Nightly"; rep.author = "teer";
    rep.description = "Regression sweep over <core> & util.";
    report_module m; m.name = "meta"; m.description = "type traits";
    report_unit u; u.name = "trait_detect"; u.description = "detection idiom";
    u.checks.push_back({"detects member", test_status::passed,
                        "has_member<T>::value", "true", "true"});
    u.checks.push_back({"rejects absent", test_status::failed,
                        "has_member<U>::value", "false", "true"});
    u.checks.push_back({"bare leaf", test_status::skipped, "", "", ""});
    m.units.push_back(u);
    rep.modules.push_back(m);

    pdf_canvas_document_renderer r;

    canvas_style pass_style; pass_style.color = pdf_color(0.0, 0.55, 0.2);
    canvas_style fail_style; fail_style.color = pdf_color(0.75, 0.1, 0.1);
    r.register_style("verdict.pass", pass_style);
    r.register_style("verdict.fail", fail_style);

    r.begin_document(doc_attributes());
    r.heading(1, rep.title, doc_attributes());
    emit_run_summary(rep, r);
    r.rule(doc_attributes());
    emit_module_summary(rep.modules[0], r);
    resolve_report_content(rep, content_key_unit(0, 0), r);
    r.end_document();

    const std::string bytes = r.to_bytes();
    std::ofstream("report.pdf", std::ios::binary).write(bytes.data(), bytes.size());
    std::cout << "bytes: " << bytes.size()
              << " | " << bytes.substr(0, 8)
              << " | EOF: " << (bytes.find("%%EOF") != std::string::npos) << "\n";
    return 0;
}
