/*******************************************************************************
* djinterp [core]                                           test_table_model.cpp
*
*
* path:      /tests/djinterp/core/container/table/test_table_model.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <cassert>
#include <iostream>
// djinterp
#include "djinterp/core/container/table/table_model.hpp"
#include "djinterp/core/container/table/table_builder.hpp"
using namespace djinterp;
struct A{}; struct B{}; struct C{};

// ---------------------------------------------------------------------------
// I. the compile-time model, and the builder -> model arrow
// ---------------------------------------------------------------------------
using bld = table_builder< columns<int, int, int>,
                           row<A, merged_cell<1,2,B>>,
                           row<A, B, C> >;
using m = model_of_t<bld>;

static_assert(is_table_model<m>::value,             "is a model");
static_assert(m::width == 3 && m::height == 2,      "extents carried");
static_assert(m::rank == 2,                         "rank");
static_assert(m::has_merges && m::merge_count == 1, "cover carried");
static_assert(m::owner_of<0,2>::anchor_col == 1,    "(0,2) defers to (0,1)");
static_assert(m::is_homogeneous,                    "homogeneous");
static_assert(std::is_same<m::element_type, int>::value, "element");
static_assert(m::realization == realization_kind::static_homogeneous, "realization");
static_assert(std::is_same<m::shape_type, bld::shape_type>::value, "same shape");
static_assert(std::is_same<m::layout_type, bld::layout_type>::value, "same layout");
// the option packs ride along
static_assert(std::is_same<m::table_options_type, bld::table_options_type>::value, "opts");

// a model naming a heterogeneous record
using recm = model_of_t<table_builder<columns<int, std::string, double>, row<A,B,C>>>;
static_assert(!recm::is_homogeneous, "record");
static_assert(recm::realization == realization_kind::static_record, "-> record_table");
static_assert(std::is_same<recm::column_type_t<1>, std::string>::value, "col type");

int main()
{
    // -----------------------------------------------------------------------
    // II. the runtime model: layout-aware access
    // -----------------------------------------------------------------------
    table_model_value<> v(2, 3);
    v.set(0, 0, "a");
    v.merge(0, 1, 1, 2, "bc");     // one cell spanning columns 1-2
    v.set(1, 0, "d");
    v.set(1, 1, "e");
    v.set(1, 2, "f");

    assert(v.rows() == 2 && v.cols() == 3);
    assert(v.has_merges());
    // T[i] = T[anchor(cell(i))]: the covered position reports the merge's value
    assert(v.at(0, 1) == "bc");
    assert(v.at(0, 2) == "bc");
    // raw access reaches past the overlay
    assert(v.raw_at(0, 2) == "");
    assert(v.at(1, 2) == "f");
    assert(v.conforms());

    // -----------------------------------------------------------------------
    // III. render: the merged cell must land exactly on its span
    // -----------------------------------------------------------------------
    const std::string out = render(v);
    std::cout << "--- render (merge over columns 1-2) ---\n" << out;

    // every line is the same length -> the grid is square
    {
        std::size_t first = out.find('\n');
        std::string l0 = out.substr(0, first);
        std::string l1 = out.substr(first + 1, out.find('\n', first + 1) - first - 1);
        assert(l0.size() == l1.size());
        // the merged row has ONE interior delimiter; the plain row has TWO
        std::size_t p0 = 0, p1 = 0;
        for (char ch : l0) if (ch == '|') ++p0;
        for (char ch : l1) if (ch == '|') ++p1;
        assert(p0 == 3);   // | a | bc |
        assert(p1 == 4);   // | d | e | f |
        // the closing delimiters align -> the span is readable from position
        assert(l0.rfind('|') == l1.rfind('|'));
        std::cout << "  line widths equal: " << l0.size() << "\n";
        std::cout << "  delimiters: merged row " << p0 << ", plain row " << p1 << "\n";
    }

    // -----------------------------------------------------------------------
    // IV. an over-long merged cell widens its span's last column
    // -----------------------------------------------------------------------
    {
        table_model_value<> w(2, 3);
        w.set(0, 0, "x");
        w.merge(0, 1, 1, 2, "a very long merged label");
        w.set(1, 0, "y");
        w.set(1, 1, "p");
        w.set(1, 2, "q");
        const std::string o = render(w);
        std::cout << "--- render (over-long merge widens the span) ---\n" << o;
        std::size_t f = o.find('\n');
        std::string a0 = o.substr(0, f);
        std::string a1 = o.substr(f + 1, o.find('\n', f + 1) - f - 1);
        assert(a0.size() == a1.size());          // still square
        assert(a0.rfind('|') == a1.rfind('|'));  // still aligned
    }

    // -----------------------------------------------------------------------
    // V. headers: the sketch's grouped "coordinate over x,y,z" shape
    //    |     |      coordinate      |
    //    | id  |  x  |  y  |  z |
    // -----------------------------------------------------------------------
    {
        table_model_value<> h(1, 4);
        h.set(0, 0, "1"); h.set(0, 1, "9"); h.set(0, 2, "8"); h.set(0, 3, "7");

        header_level<std::string> outer;
        outer.push_back(header_cell<std::string>("", 1));
        outer.push_back(header_cell<std::string>("coordinate", 3));   // spans x,y,z
        header_level<std::string> inner =
            make_header_level<std::string>({"id", "x", "y", "z"});

        header_stack<std::string> stack;
        stack.push_back(outer);
        stack.push_back(inner);
        h.metadata().set_column_headers(stack);

        // table_metadata's own conformance: the finest level spans the columns
        assert(h.metadata().conforms_columns(4));
        assert(h.conforms());

        const std::string o = render(h);
        std::cout << "--- render (grouped headers + separator) ---\n" << o;

        // outer level: 2 cells; inner: 4; separator present
        std::vector<std::string> lines;
        std::size_t s = 0, e;
        while ((e = o.find('\n', s)) != std::string::npos) { lines.push_back(o.substr(s, e - s)); s = e + 1; }
        assert(lines.size() == 4);                    // outer, inner, separator, body
        assert(lines[2].find("---") != std::string::npos);  // the separator
        for (std::size_t i = 1; i < lines.size(); ++i)
            assert(lines[i].size() == lines[0].size());     // all square
        std::size_t pouter = 0; for (char ch : lines[0]) if (ch == '|') ++pouter;
        std::size_t pinner = 0; for (char ch : lines[1]) if (ch == '|') ++pinner;
        assert(pouter == 3);   // |  | coordinate |
        assert(pinner == 5);   // | id | x | y | z |
        std::cout << "  header delimiters: outer " << pouter << ", inner " << pinner << "\n";
    }

    // -----------------------------------------------------------------------
    // VI. detection + an ordinary (Gamma_0) table renders plainly
    // -----------------------------------------------------------------------
    static_assert(is_table_model_value<table_model_value<>>::value, "detect value model");
    static_assert(!is_table_model_value<int>::value, "not a model");
    {
        table_model_value<> p(1, 2);
        p.set(0,0,"a"); p.set(0,1,"b");
        assert(!p.has_merges());
        assert(render(p) == "| a | b |\n");
    }

    std::cout << "\nall runtime assertions passed\n";
    return 0;
}
