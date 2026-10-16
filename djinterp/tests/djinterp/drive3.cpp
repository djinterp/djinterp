/*******************************************************************************
* djinterp [core]                                                     drive3.cpp
*
*
* path:      /tests/djinterp/drive3.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <iostream>
// djinterp
#include "djinterp/core/util/document/templates/table_model_dialect.hpp"

using namespace djinterp;

// --- a minimal model conforming to the bridge's duck type -------------------
struct hcell { std::string label; std::size_t span; };
struct region { std::size_t row0, col0, rows, cols;
                std::size_t anchor_row() const { return row0; }
                std::size_t anchor_col() const { return col0; } };

struct layout_stub
{
    std::vector<region> merges;
    region owner_of(std::size_t r, std::size_t c) const
    {
        for (const region& m : merges)
        {
            if ( (r >= m.row0) && (r < m.row0 + m.rows) &&
                 (c >= m.col0) && (c < m.col0 + m.cols) ) return m;
        }
        return region{r, c, 1, 1};
    }
};

struct meta_stub
{
    std::vector<std::vector<hcell>> ch, rh;
    bool has_column_headers() const { return !ch.empty(); }
    bool has_row_headers()    const { return !rh.empty(); }
    const std::vector<std::vector<hcell>>& column_headers() const { return ch; }
    const std::vector<std::vector<hcell>>& row_headers()    const { return rh; }
};

struct model_stub
{
    std::size_t m_rows, m_cols;
    std::vector<std::string> cells;
    layout_stub lay;
    meta_stub   meta;
    std::size_t rows() const { return m_rows; }
    std::size_t cols() const { return m_cols; }
    const std::string& raw_at(std::size_t r, std::size_t c) const
    { return cells[r * m_cols + c]; }
    const layout_stub& layout()   const { return lay;  }
    const meta_stub&   metadata() const { return meta; }
};

int main()
{
    model_stub m;
    m.m_rows = 3; m.m_cols = 3;
    m.cells = { "1","2","3",  "spanning","", "6",  "7","8","9" };
    m.lay.merges.push_back(region{1, 0, 1, 2});          // row 1 cols 0-1 merged

    // grouped column headers: "coordinate" over (x, y), then "n"
    m.meta.ch = { { {"coordinate",2}, {"n",1} },
                  { {"x",1}, {"y",1}, {"count",1} } };
    m.meta.rh = { { {"r0",1}, {"r1",1}, {"r2",1} } };

    const document_format fs[] = { document_format::text,
                                   document_format::markdown,
                                   document_format::html };
    for (document_format f : fs)
    {
        std::cout << "########## " << format_name(f) << "\n"
                  << render_table_model_to_string(m, f) << "\n";
    }
    return 0;
}
