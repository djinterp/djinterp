/*******************************************************************************
* djinterp [core]                                                    drive17.cpp
*
*
* path:      /tests/djinterp/cpp/drive17.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <fstream>
#include <iostream>
// djinterp
#include "djinterp/cpp/util/document/document_template.hpp"
#include "djinterp/cpp/util/document/templates/document_dialect.hpp"
#include "djinterp/cpp/util/pdf/pdf_canvas_document_renderer.hpp"
using namespace djinterp;

int main()
{
    // --- one declarative skeleton -------------------------------------------
    DDocumentTemplate t;
    t.frame().set("title", "Nightly");
    t.add(tmpl_heading(1, "{title} — build {build}"));
    t.add(tmpl_key_value("Verdict", "{verdict}", "verdict.fail"));
    t.add(tmpl_rule());
    t.add(tmpl_repeat("modules", {
        tmpl_heading(2, "{modules.name}", "suite.band"),
        tmpl_paragraph("{modules.desc}"),
        tmpl_table("checks", {
            tmpl_column("RESULT", "{checks.status}", DTextAlignment::center),
            tmpl_column("ASSERTION", "{checks.expr}"),
            tmpl_column("ms", "{checks.ms}", DTextAlignment::right) })
    }));

    // --- bound to a live model ----------------------------------------------
    DTemplateContext ctx;
    ctx.values = map_bindings({{"title","Nightly"},{"build","4412"},
                               {"verdict","FAILED"}});
    const char* mods[][2] = {{"meta","type traits"},{"container","tables"}};
    ctx.sequences = [&](const std::string& n) -> DSequenceEnv {
        if (n == "modules")
            return DSequenceEnv([]{ return std::size_t(2); },
                [&](std::size_t i) { return map_bindings(
                    {{"name",mods[i][0]},{"desc",mods[i][1]}}); });
        if (n == "checks")
            return DSequenceEnv([]{ return std::size_t(2); },
                [](std::size_t i) { return map_bindings(
                    {{"status", i?"FAIL":"PASS"},
                     {"expr", i?"p==q":"x==y"}, {"ms", i?"3":"12"}}); });
        return DSequenceEnv();
    };

    for (DDocumentFormat f : { DDocumentFormat::text, DDocumentFormat::html })
    {
        DDocumentDialect d = make_document_dialect(f);
        render_template(t, ctx, d.renderer());
        std::ofstream(std::string("tmpl.") + format_name(f)) << d.str();
        std::cout << format_name(f) << ": " << d.str().size() << " bytes\n";
    }
    DPdfCanvasDocumentRenderer pr;
    render_template(t, ctx, pr);
    const std::string b = pr.to_bytes();
    std::ofstream("tmpl.pdf", std::ios::binary).write(b.data(), b.size());
    std::cout << "pdf: " << b.size() << " bytes\n";
    return 0;
}
