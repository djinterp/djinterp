/*******************************************************************************
* djinterp [core]                                                    drive14.cpp
*
*
* path:      /tests/djinterp/drive14.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <fstream>
#include <iostream>
// djinterp
#include "djinterp/test/output/test_layout.hpp"
#include "djinterp/core/util/document/templates/document_dialect.hpp"
#include "djinterp/core/util/pdf/pdf_canvas_document_renderer.hpp"
using namespace djinterp;
using namespace djinterp::test;
int main()
{
    test_report rep;
    rep.title = "Nightly"; rep.subtitle = "build 4412"; rep.author = "teer";
    for (int mi = 0; mi < 2; ++mi) {
        report_module m; m.name = (mi ? "container" : "meta");
        for (int ui = 0; ui < 2; ++ui) {
            report_unit u; u.name = std::string(m.name) + "_u" + std::to_string(ui);
            u.checks.push_back({"a", test_status::passed, "x==y", "1", "1"});
            u.checks.push_back({"b", test_status::failed, "p==q", "2", "3"});
            m.units.push_back(u);
        }
        rep.modules.push_back(m);
    }

    for (document_format f : { document_format::html, document_format::markdown })
    {
        document_dialect d = make_document_dialect(f);
        d.renderer().begin_document(doc_attributes());
        render_test_report(rep, d.renderer());
        d.renderer().end_document();
        std::ofstream(std::string("layout.") + format_name(f)) << d.str();
        std::cout << format_name(f) << ": " << d.str().size() << " bytes\n";
    }

    pdf_canvas_document_renderer pr;
    pr.set_zebra(pdf_color::from_rgb255(244,245,248));
    pr.begin_document(doc_attributes());
    render_test_report(rep, pr);
    pr.end_document();
    const std::string b = pr.to_bytes();
    std::ofstream("layout.pdf", std::ios::binary).write(b.data(), b.size());
    std::cout << "pdf: " << b.size() << " bytes\n";
    return 0;
}
