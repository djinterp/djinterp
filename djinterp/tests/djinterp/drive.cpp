/*******************************************************************************
* djinterp [core]                                                      drive.cpp
*
*
* path:      /tests/djinterp/drive.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <iostream>
// djinterp
#include "djinterp/core/util/document/templates/document_dialect.hpp"
#include "djinterp/core/util/document/templates/document_table.hpp"

int main()
{
    using namespace djinterp;

    document_table t;
    t.set_caption("Results");
    t.add_column("Test",   text_alignment::left);
    t.add_column("Result", text_alignment::center);
    t.add_column("ms",     text_alignment::right);
    t.add_row({"parses <a|b>", "PASS", "12"});
    t.add_row({"handles \"quotes\" & *stars*", "FAIL", "3"});

    const document_format fmts[] = { document_format::text,
                                     document_format::markdown,
                                     document_format::xml,
                                     document_format::html };

    for (document_format f : fmts)
    {
        std::cout << "########## " << format_name(f)
                  << " (" << format_extension(f) << ")\n"
                  << render_to_string(f, t) << "\n";
    }

    std::cout << "pdf dialect valid? "
              << (make_document_dialect(document_format::pdf).valid() ? "yes"
                                                                     : "no")
              << "\n";
    return 0;
}
