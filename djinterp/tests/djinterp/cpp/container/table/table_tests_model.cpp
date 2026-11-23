/*******************************************************************************
* djinterp [cpp][container]                                table_tests_model.cpp
*
* table_model.hpp: the compile-time model, the builder-to-model arrow, and
* the runtime value model.
*
*   Converted on 2026.11.16 from test_table_model.cpp, an assert-based
* program no leaf built: each assertion is one check, in its order.
*
*
* path:      /tests/djinterp/cpp/container/table/table_tests_model.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/
// std
#include <type_traits>  // std::is_same
// djinterp
#include "table_tests.hpp"  // D_TBL_CHECK, the declarations
#include <djinterp/cpp/container/table/table_model.hpp>  // table_model.hpp
#include <djinterp/cpp/container/table/table_builder.hpp>  // table_builder.hpp


NS_DJINTERP
NS_TESTING

// The fixtures of the original program, in its order.  Unnamed, so a
// fixture name reused by another section never breaks the one-definition
// rule.
namespace
{

// std
// djinterp
struct A{}; struct B{}; struct C{};

using bld = DTableBuilder< DColumns<int, int, int>,
                           DRow<A, DMergedCell<1,2,B>>,
                           DRow<A, B, C> >;
using m = model_of_t<bld>;

// the option packs ride along

// a model naming a heterogeneous record
using recm = model_of_t<DTableBuilder<DColumns<int, std::string, double>, DRow<A,B,C>>>;

}  // namespace


/*
tests_table_model_compile_time
  the compile-time model, and the builder-to-model arrow
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_model_compile_time()
{
    D_TBL_CHECK(is_table_model<m>::value);  // is a model
    D_TBL_CHECK(m::width == 3 && m::height == 2);  // extents carried
    D_TBL_CHECK(m::rank == 2);  // rank
    D_TBL_CHECK(m::has_merges && m::merge_count == 1);  // cover carried
    D_TBL_CHECK(m::owner_of<0,2>::anchor_col == 1);  // (0,2) defers to (0,1)
    D_TBL_CHECK(m::is_homogeneous);  // homogeneous
    D_TBL_CHECK(std::is_same<m::element_type, int>::value);  // element
    // realization
    D_TBL_CHECK(m::realization == DRealizationKind::static_homogeneous);
    // same shape
    D_TBL_CHECK(std::is_same<m::shape_type, bld::shape_type>::value);
    // same layout
    D_TBL_CHECK(std::is_same<m::layout_type, bld::layout_type>::value);
    // opts
    D_TBL_CHECK(std::is_same<m::table_options_type,
                bld::table_options_type>::value);
    D_TBL_CHECK(!recm::is_homogeneous);  // record
    // -> record_table
    D_TBL_CHECK(recm::realization == DRealizationKind::static_record);
    // col type
    D_TBL_CHECK(std::is_same<recm::column_type_t<1>, std::string>::value);

    return true;
}

/*
tests_table_model_runtime
  the runtime value model
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_model_runtime()
{
    // -----------------------------------------------------------------------
    // II. the runtime model: layout-aware access
    // -----------------------------------------------------------------------
    DTableModelValue<> v(2, 3);
    v.set(0, 0, "a");
    v.merge(0, 1, 1, 2, "bc");     // one cell spanning columns 1-2
    v.set(1, 0, "d");
    v.set(1, 1, "e");
    v.set(1, 2, "f");

    D_TBL_CHECK(v.rows() == 2 && v.cols() == 3);
    D_TBL_CHECK(v.has_merges());
    // T[i] = T[anchor(cell(i))]: the covered position reports the merge's value
    D_TBL_CHECK(v.at(0, 1) == "bc");
    D_TBL_CHECK(v.at(0, 2) == "bc");
    // raw access reaches past the overlay
    D_TBL_CHECK(v.raw_at(0, 2) == "");
    D_TBL_CHECK(v.at(1, 2) == "f");
    D_TBL_CHECK(v.conforms());

    // -----------------------------------------------------------------------
    // III. render: the merged cell must land exactly on its span
    // -----------------------------------------------------------------------
    const std::string out = render(v);

    // every line is the same length -> the grid is square
    {
        std::size_t first = out.find('\n');
        std::string l0 = out.substr(0, first);
        std::string l1 = out.substr(first + 1, out.find('\n',
                                    first + 1) - first - 1);
        D_TBL_CHECK(l0.size() == l1.size());
        // the merged row has ONE interior delimiter; the plain row has TWO
        std::size_t p0 = 0, p1 = 0;
        for (char ch : l0) if (ch == '|') ++p0;
        for (char ch : l1) if (ch == '|') ++p1;
        D_TBL_CHECK(p0 == 3);   // | a | bc |
        D_TBL_CHECK(p1 == 4);   // | d | e | f |
        // the closing delimiters align -> the span is readable from position
        D_TBL_CHECK(l0.rfind('|') == l1.rfind('|'));
    }

    // -----------------------------------------------------------------------
    // IV. an over-long merged cell widens its span's last column
    // -----------------------------------------------------------------------
    {
        DTableModelValue<> w(2, 3);
        w.set(0, 0, "x");
        w.merge(0, 1, 1, 2, "a very long merged label");
        w.set(1, 0, "y");
        w.set(1, 1, "p");
        w.set(1, 2, "q");
        const std::string o = render(w);
        std::size_t f = o.find('\n');
        std::string a0 = o.substr(0, f);
        std::string a1 = o.substr(f + 1, o.find('\n', f + 1) - f - 1);
        D_TBL_CHECK(a0.size() == a1.size());          // still square
        D_TBL_CHECK(a0.rfind('|') == a1.rfind('|'));  // still aligned
    }

    // -----------------------------------------------------------------------
    // V. headers: the sketch's grouped "coordinate over x,y,z" shape
    //    |     |      coordinate      |
    //    | id  |  x  |  y  |  z |
    // -----------------------------------------------------------------------
    {
        DTableModelValue<> h(1, 4);
        h.set(0, 0, "1"); h.set(0, 1, "9"); h.set(0, 2, "8"); h.set(0, 3, "7");

        DHeaderLevel<std::string> outer;
        outer.push_back(DHeaderCell<std::string>("", 1));
        // spans x,y,z
        outer.push_back(DHeaderCell<std::string>("coordinate", 3));
        DHeaderLevel<std::string> inner =
            make_header_level<std::string>({"id", "x", "y", "z"});

        DHeaderStack<std::string> stack;
        stack.push_back(outer);
        stack.push_back(inner);
        h.metadata().set_column_headers(stack);

        // table_metadata's own conformance: the finest level spans the columns
        D_TBL_CHECK(h.metadata().conforms_columns(4));
        D_TBL_CHECK(h.conforms());

        const std::string o = render(h);

        // outer level: 2 cells; inner: 4; separator present
        std::vector<std::string> lines;
        std::size_t s = 0, e;
        while ((e = o.find('\n', s))
               != std::string::npos) { lines.push_back(o.substr(s,
                                                       e - s)); s = e + 1; }
        // outer, inner, separator, body
        D_TBL_CHECK(lines.size() == 4);
        // the separator
        D_TBL_CHECK(lines[2].find("---") != std::string::npos);
        for (std::size_t i = 1; i < lines.size(); ++i)
            D_TBL_CHECK(lines[i].size() == lines[0].size());     // all square
        std::size_t pouter = 0; for (char ch : lines[0]) if (ch
                                     == '|') ++pouter;
        std::size_t pinner = 0; for (char ch : lines[1]) if (ch
                                     == '|') ++pinner;
        D_TBL_CHECK(pouter == 3);   // |  | coordinate |
        D_TBL_CHECK(pinner == 5);   // | id | x | y | z |
    }

    // -----------------------------------------------------------------------
    // VI. detection + an ordinary (Gamma_0) table renders plainly
    // -----------------------------------------------------------------------
    static_assert(is_table_model_value<DTableModelValue<>>::value,
                  "detect value model");
    static_assert(!is_table_model_value<int>::value, "not a model");
    {
        DTableModelValue<> p(1, 2);
        p.set(0,0,"a"); p.set(0,1,"b");
        D_TBL_CHECK(!p.has_merges());
        D_TBL_CHECK(render(p) == "| a | b |\n");
    }

    return true;
}

NS_END  // testing
NS_END  // djinterp
