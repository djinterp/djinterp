/*******************************************************************************
* djinterp [core]                                                      drive.cpp
*
*
* path:      /tests/djinterp/cpp/drive.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <iostream>
// djinterp
#include "djinterp/cpp/util/document/templates/document_dialect.hpp"
#include "djinterp/cpp/util/document/templates/document_table.hpp"

int main()
{
    using namespace djinterp;

    DDocumentTable t;
    t.set_caption("Results");
    t.add_column("Test",   DTextAlignment::left);
    t.add_column("Result", DTextAlignment::center);
    t.add_column("ms",     DTextAlignment::right);
    t.add_row({"parses <a|b>", "PASS", "12"});
    t.add_row({"handles \"quotes\" & *stars*", "FAIL", "3"});

    const DDocumentFormat fmts[] = { DDocumentFormat::text,
                                     DDocumentFormat::markdown,
                                     DDocumentFormat::xml,
                                     DDocumentFormat::html };

    for (DDocumentFormat f : fmts)
    {
        std::cout << "########## " << format_name(f)
                  << " (" << format_extension(f) << ")\n"
                  << render_to_string(f, t) << "\n";
    }

    std::cout << "pdf dialect valid? "
              << (make_document_dialect(DDocumentFormat::pdf).valid() ? "yes"
                                                                     : "no")
              << "\n";
    return 0;
}
