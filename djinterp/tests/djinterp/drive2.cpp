/*******************************************************************************
* djinterp [core]                                                     drive2.cpp
*
*
* path:      /tests/djinterp/drive2.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <iostream>
// djinterp
#include "djinterp/core/util/document/templates/document_dialect.hpp"

using namespace djinterp;

// a template exercising the blocks a table does not reach
struct sampler
{
    void render(document_renderer& r) const
    {
        doc_attributes h;  h.set(doc_attr_style, "verdict.pass");
                           h.set(doc_attr_align, "center");
        doc_attributes b;  b.set(doc_attr_bold, "true");
        doc_attributes i;  i.set(doc_attr_italic, "true");
        doc_attributes lk; lk.set(doc_attr_locator, "sec-1");

        r.heading(1, "Report & <Summary>", h);
        r.paragraph("Emphasis: strong", b);
        r.paragraph("Emphasis: slanted", i);
        r.key_value("Author", "teer", lk);
        r.rule(doc_attributes());
        r.vertical_space(18.0, doc_attributes());
        r.begin_list(false, doc_attributes());
          r.list_item("alpha", doc_attributes());
          r.begin_list(true, doc_attributes());
            r.list_item("one", doc_attributes());
            r.list_item("two | piped", doc_attributes());
          r.end_list();
        r.end_list();
        r.page_break();
        r.paragraph("after the break", doc_attributes());
    }
};

int main()
{
    sampler s;
    doc_attributes frame;
    frame.set("title", "DTest Run");

    const document_format fmts[] = { document_format::text,
                                     document_format::markdown,
                                     document_format::xml,
                                     document_format::html };

    for (document_format f : fmts)
    {
        std::cout << "########## " << format_name(f) << "\n"
                  << render_to_string(f, s, frame) << "\n";
    }
    return 0;
}
