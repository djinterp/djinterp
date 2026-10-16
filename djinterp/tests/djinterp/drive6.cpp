/*******************************************************************************
* djinterp [core]                                                     drive6.cpp
*
*
* path:      /tests/djinterp/drive6.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <fstream>
#include <iostream>
// djinterp
#include "djinterp/core/util/pdf/pdf_canvas.hpp"

using namespace djinterp;

int main()
{
    pdf_canvas c;
    c.heading("DTest Report", 1);
    c.paragraph("A metric-accurate paragraph laid out by pdf_canvas, which "
                "wraps on real Adobe Core-14 advances rather than an average "
                "character cell, and justifies against measured widths.",
                canvas_style());
    c.rule(1.0, pdf_color::black());

    std::vector<std::vector<std::string>> rows = {
        { "RESULT", "#", "ASSERTION", "ACTUAL" },
        { "PASS", "1", "has_member<T>::value", "true"  },
        { "FAIL", "2", "has_member<U>::value", "true"  },
        { "SKIP", "3", "bare leaf",            ""      }
    };
    table_options to;
    to.has_header = true;
    c.table(rows, to);

    const std::string bytes = c.to_bytes();
    std::ofstream("out.pdf", std::ios::binary).write(bytes.data(), bytes.size());
    std::cout << "bytes: " << bytes.size()
              << " | header: " << bytes.substr(0, 8)
              << " | has EOF: " << (bytes.find("%%EOF") != std::string::npos)
              << "\n";
    return 0;
}
