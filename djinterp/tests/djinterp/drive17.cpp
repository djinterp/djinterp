/*******************************************************************************
* djinterp [core]                                                    drive17.cpp
*
*
* path:      /tests/djinterp/drive17.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <fstream>
#include <iostream>
// djinterp
#include "djinterp/core/util/document/document_template.hpp"
#include "djinterp/core/util/document/templates/document_dialect.hpp"
#include "djinterp/core/util/pdf/pdf_canvas_document_renderer.hpp"
using namespace djinterp;

int main()
{
    // --- one declarative skeleton -------------------------------------------
    document_template t;
    t.frame().set("title", "Nightly");
    t.add(tmpl_heading(1, "{title} — build {build}"));
    t.add(tmpl_key_value("Verdict", "{verdict}", "verdict.fail"));
    t.add(tmpl_rule());
    t.add(tmpl_repeat("modules", {
        tmpl_heading(2, "{modules.name}", "suite.band"),
        tmpl_paragraph("{modules.desc}"),
        tmpl_table("checks", {
            tmpl_column("RESULT", "{checks.status}", text_alignment::center),
            tmpl_column("ASSERTION", "{checks.expr}"),
            tmpl_column("ms", "{checks.ms}", text_alignment::right) })
    }));

    // --- bound to a live model ----------------------------------------------
    template_context ctx;
    ctx.values = map_bindings({{"title","Nightly"},{"build","4412"},
                               {"verdict","FAILED"}});
    const char* mods[][2] = {{"meta","type traits"},{"container","tables"}};
    ctx.sequences = [&](const std::string& n) -> sequence_env {
        if (n == "modules")
            return sequence_env([]{ return std::size_t(2); },
                [&](std::size_t i) { return map_bindings(
                    {{"name",mods[i][0]},{"desc",mods[i][1]}}); });
        if (n == "checks")
            return sequence_env([]{ return std::size_t(2); },
                [](std::size_t i) { return map_bindings(
                    {{"status", i?"FAIL":"PASS"},
                     {"expr", i?"p==q":"x==y"}, {"ms", i?"3":"12"}}); });
        return sequence_env();
    };

    for (document_format f : { document_format::text, document_format::html })
    {
        document_dialect d = make_document_dialect(f);
        render_template(t, ctx, d.renderer());
        std::ofstream(std::string("tmpl.") + format_name(f)) << d.str();
        std::cout << format_name(f) << ": " << d.str().size() << " bytes\n";
    }
    pdf_canvas_document_renderer pr;
    render_template(t, ctx, pr);
    const std::string b = pr.to_bytes();
    std::ofstream("tmpl.pdf", std::ios::binary).write(b.data(), b.size());
    std::cout << "pdf: " << b.size() << " bytes\n";
    return 0;
}
