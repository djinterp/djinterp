/*******************************************************************************
* djinterp [core]                                                    drive13.cpp
*
*
* path:      /tests/djinterp/cpp/drive13.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <iostream>
// djinterp
#include "djinterp/cpp/modules/test/output/test_layout.hpp"
#include "djinterp/cpp/util/document/templates/document_dialect.hpp"
using namespace djinterp;
using namespace djinterp::test;
int main()
{
    test_report rep;
    rep.title = "Nightly"; rep.subtitle = "build 4412"; rep.author = "teer";
    rep.description = "Sweep over core & util.";
    report_module m; m.name = "meta"; m.description = "type traits";
    report_unit u; u.name = "trait_detect"; u.description = "detection idiom";
    u.checks.push_back({"a", test_status::passed, "has_member<T>", "true", "true"});
    u.checks.push_back({"b", test_status::failed, "has_member<U>", "false", "true"});
    m.units.push_back(u);
    rep.modules.push_back(m);

    DPlainDocumentRenderer r;
    render_test_report(rep, r);
    std::cout << r.str();
    return 0;
}
