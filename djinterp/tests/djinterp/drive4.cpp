/*******************************************************************************
* djinterp [core]                                                     drive4.cpp
*
*
* path:      /tests/djinterp/drive4.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <iostream>
// djinterp
#include "djinterp/core/container/table/table_layout.hpp"        // REAL runtime_layout
#include "djinterp/core/container/table/table_metadata.hpp"   // REAL table_metadata
#include "djinterp/core/util/document/templates/table_model_dialect.hpp"

using namespace djinterp;

// a table_model_value-shaped model built on the REAL layout + metadata types
struct real_model
{
    std::size_t              m_rows, m_cols;
    std::vector<std::string> m_cells;
    runtime_layout           m_layout;
    table_metadata<>         m_meta;

    real_model(std::size_t r, std::size_t c)
        : m_rows(r), m_cols(c), m_cells(r * c), m_layout(), m_meta() {}

    std::size_t rows() const { return m_rows; }
    std::size_t cols() const { return m_cols; }
    std::size_t row_count()    const { return m_rows; }
    std::size_t column_count() const { return m_cols; }
    void set(std::size_t r, std::size_t c, std::string v)
    { m_cells[r * m_cols + c] = std::move(v); }
    void merge(std::size_t r, std::size_t c, std::size_t nr, std::size_t nc,
               std::string v)
    { m_layout.add_merge(r, c, nr, nc); set(r, c, std::move(v)); }
    const std::string& raw_at(std::size_t r, std::size_t c) const
    { return m_cells[r * m_cols + c]; }
    const runtime_layout&   layout()   const { return m_layout; }
    const table_metadata<>& metadata() const { return m_meta;   }
};

int main()
{
    real_model m(3, 4);
    m.set(0,0,"1"); m.set(0,1,"2"); m.set(0,2,"3"); m.set(0,3,"4");
    m.merge(1, 0, 2, 2, "2x2 merge");            // rows 1-2, cols 0-1
    m.set(1,2,"c"); m.set(1,3,"d");
    m.set(2,2,"g"); m.set(2,3,"h");

    // grouped column headers: "coordinate" over (x,y), "extra" over (p,q)
    header_stack<std::string> ch;
    header_level<std::string> outer;
    outer.push_back(header_cell<std::string>("coordinate", 2));
    outer.push_back(header_cell<std::string>("extra", 2));
    ch.push_back(outer);
    ch.push_back(make_header_level<std::string>({"x","y","p","q"}));
    m.m_meta.set_column_headers(ch);

    header_stack<std::string> rh;
    rh.push_back(make_header_level<std::string>({"r0","r1","r2"}));
    m.m_meta.set_row_headers(rh);

    std::cout << "layout valid: " << (m.layout().valid(3,4) ? "yes":"no")
              << " | headers conform: "
              << (m.metadata().conforms_to(m) ? "yes":"no") << "\n\n";

    for (document_format f : { document_format::text,
                               document_format::html })
    {
        std::cout << "########## " << format_name(f) << " (defaults)\n"
                  << render_table_model_to_string(m, f) << "\n";
    }

    // the `rows` header policy, monospace
    plain_document_renderer pr;
    table_render_options o = table_render_defaults(document_format::text);
    o.header_levels = header_level_policy::rows;
    render_table_model(m, pr, o);
    std::cout << "########## text (header_level_policy::rows)\n" << pr.str();
    return 0;
}
