/*******************************************************************************
* djinterp [core]                                                     drive2.cpp
*
*
* path:      /tests/djinterp/cpp/drive2.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <iostream>
// djinterp
#include "djinterp/cpp/util/document/templates/document_dialect.hpp"

using namespace djinterp;

// a template exercising the blocks a table does not reach
struct sampler
{
    void render(
        DDocumentRenderer& r
    ) const
    {
        DDocAttributes h;  h.set(doc_attr_style, "verdict.pass");
                           h.set(doc_attr_align, "center");
        DDocAttributes b;  b.set(doc_attr_bold, "true");
        DDocAttributes i;  i.set(doc_attr_italic, "true");
        DDocAttributes lk; lk.set(doc_attr_locator, "sec-1");

        r.heading(1, "Report & <Summary>", h);
        r.paragraph("Emphasis: strong", b);
        r.paragraph("Emphasis: slanted", i);
        r.key_value("Author", "teer", lk);
        r.rule(DDocAttributes());
        r.vertical_space(18.0, DDocAttributes());
        r.begin_list(false, DDocAttributes());
          r.list_item("alpha", DDocAttributes());
          r.begin_list(true, DDocAttributes());
            r.list_item("one", DDocAttributes());
            r.list_item("two | piped", DDocAttributes());
          r.end_list();
        r.end_list();
        r.page_break();
        r.paragraph("after the break", DDocAttributes());
    }
};

int main()
{
    sampler s;
    DDocAttributes frame;
    frame.set("title", "DTest Run");

    const DDocumentFormat fmts[] = { DDocumentFormat::text,
                                     DDocumentFormat::markdown,
                                     DDocumentFormat::xml,
                                     DDocumentFormat::html };

    for (DDocumentFormat f : fmts)
    {
        std::cout << "########## " << format_name(f) << "\n"
                  << render_to_string(f, s, frame) << "\n";
    }
    return 0;
}
