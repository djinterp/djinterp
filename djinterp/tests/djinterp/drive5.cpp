/*******************************************************************************
* djinterp [core]                                                     drive5.cpp
*
*
* path:      /tests/djinterp/drive5.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <iostream>
// djinterp
#include "djinterp/test/output/test_report_content.hpp"
#include "djinterp/core/util/document/templates/document_dialect.hpp"

using namespace djinterp;
using namespace djinterp::test;

int main()
{
    test_report rep;
    rep.title = "Nightly"; rep.subtitle = "build 4412"; rep.author = "teer";
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

    for (document_format f : { document_format::text,
                               document_format::markdown,
                               document_format::html })
    {
        document_dialect d = make_document_dialect(f);
        d.renderer().begin_document(doc_attributes());
        emit_run_summary(rep, d.renderer());
        emit_module_summary(rep.modules[0], d.renderer());
        bool bound = resolve_report_content(rep, content_key_unit(0, 0),
                                            d.renderer());
        d.renderer().end_document();
        std::cout << "########## " << format_name(f)
                  << "  (unit key bound: " << (bound ? "yes":"no") << ")\n"
                  << d.str() << "\n";
    }
    std::cout << "unbound key probes false: "
              << (report_binds_content(rep, "unit.9.9") ? "NO" : "yes") << "\n";
    return 0;
}
