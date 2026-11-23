/*******************************************************************************
* djinterp [cpp][container]                               table_tests_parser.cpp
*
* table_parser.hpp: parsing text tables into the runtime model.
*
*   Converted on 2026.11.16 from test_table_parser.cpp, an assert-based
* program no leaf built: each assertion is one check, in its order.
*
*
* path:      /tests/djinterp/cpp/container/table/table_tests_parser.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/
// std
#include <type_traits>  // std::is_same
// djinterp
#include "table_tests.hpp"  // D_TBL_CHECK, the declarations
#include <djinterp/cpp/container/table/table_parser.hpp>  // table_parser.hpp
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
struct tagA{}; struct tagB{}; struct tagD{}; struct tagE{}; struct tagF{};

}  // namespace


/*
tests_table_parser
  text tables parse into the runtime model
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_parser()
{
    // -----------------------------------------------------------------------
    // 1. THE SKETCH'S OWN GRID: merged header cells, a separator, a body.
    //    "something" spans bar+alpha; "something else" spans beta+gamma.
    // -----------------------------------------------------------------------
    const std::string sketch =
        "|     |   something   |  something else |\n"
        "| foo | bar  | alpha  | beta  | gamma   |\n"
        "|---------------------------------------|\n"
        "| 1   | 2    | 3      | 4     | 5       |\n";

    auto r = parse_table_text<>(sketch);
    D_TBL_CHECK(r.ok());
    const auto& m = r.value();

    D_TBL_CHECK(m.cols() == 5);      // the finest line resolves 5 columns
    D_TBL_CHECK(m.rows() == 1);      // one body row (below the separator)
    D_TBL_CHECK(m.at(0,0) == "1" && m.at(0,4) == "5");

    // the header block: two levels, outermost first
    D_TBL_CHECK(m.metadata().has_column_headers());
    const auto& stack = m.metadata().column_headers();
    D_TBL_CHECK(stack.size() == 2);

    // THE KEY ASSERTION: the outer level's spans were read off the delimiters
    const auto& outer = stack[0];
    D_TBL_CHECK(outer.size() == 3);
    // the empty leading cell
    D_TBL_CHECK(outer[0].span == 1);
    // bar+alpha
    D_TBL_CHECK(outer[1].label == "something"      && outer[1].span == 2);
    // beta+gamma
    D_TBL_CHECK(outer[2].label == "something else" && outer[2].span == 2);

    // the finer level is 5 unit-span labels
    const auto& inner = stack[1];
    D_TBL_CHECK(inner.size() == 5);
    for (const auto& hc : inner) D_TBL_CHECK(hc.span == 1);
    D_TBL_CHECK(inner[0].label == "foo" && inner[4].label == "gamma");

    // table_metadata's own conformance: the finest level spans the columns
    D_TBL_CHECK(m.metadata().conforms_columns(5));
    D_TBL_CHECK(m.conforms());

    // -----------------------------------------------------------------------
    // 2. A MERGED BODY CELL becomes a merge in the cover, anchored where it starts
    // -----------------------------------------------------------------------
    const std::string body_merge =
        "| a | bc     |\n"
        "| d | e | f  |\n";
    auto r2 = parse_table_text<>(body_merge);
    D_TBL_CHECK(r2.ok());
    const auto& m2 = r2.value();
    D_TBL_CHECK(m2.cols() == 3 && m2.rows() == 2);
    D_TBL_CHECK(m2.has_merges());
    D_TBL_CHECK(m2.layout().merge_count() == 1);
    // T[i] = T[anchor(cell(i))] : the covered position reports the merge's value
    D_TBL_CHECK(m2.at(0,1) == "bc");
    D_TBL_CHECK(m2.at(0,2) == "bc");
    D_TBL_CHECK(m2.layout().owner_of(0,2).anchor_col() == 1);
    D_TBL_CHECK(m2.at(1,2) == "f");

    // -----------------------------------------------------------------------
    // 3. THE ROUND-TRIP LAW:  parse . render = id
    //    A model built by hand renders to text that parses back to the same model.
    // -----------------------------------------------------------------------
    {
        DTableModelValue<> v(2, 3);
        v.set(0,0,"a");
        v.merge(0,1,1,2,"bc");
        v.set(1,0,"d"); v.set(1,1,"e"); v.set(1,2,"f");
        D_TBL_CHECK(survives_round_trip(v));
    }
    {
        DTableModelValue<> p(3, 4);
        for (std::size_t i = 0; i < 3; ++i)
            for (std::size_t j = 0; j < 4; ++j)
                p.set(i, j, std::string(1, char('a' + i*4 + j)));
        D_TBL_CHECK(!p.has_merges());
        D_TBL_CHECK(survives_round_trip(p));   // Gamma_0 round-trips too
    }
    {
        // an over-long merge (render widens the span's last column)
        DTableModelValue<> w(2, 3);
        w.set(0,0,"x");
        w.merge(0,1,1,2,"a very long merged label");
        w.set(1,0,"y"); w.set(1,1,"p"); w.set(1,2,"q");
        D_TBL_CHECK(survives_round_trip(w));
    }

    // -----------------------------------------------------------------------
    // 4. THE OTHER LEG:  render . parse = id  on canonical text
    // -----------------------------------------------------------------------
    {
        const std::string canonical = "| a | bc    |\n| d | e | f |\n";
        D_TBL_CHECK(renders_to_same_text(canonical));
        // a NON-canonical but equivalent grid parses to the same model and
        // renders to the canonical spelling -- which is what a canonical form is for
        const std::string sloppy = "|a|bc|\n|d|e|f|\n";
        auto ra = parse_table_text<>(canonical);
        auto rb = parse_table_text<>(sloppy);
        D_TBL_CHECK(ra.ok() && rb.ok());
        D_TBL_CHECK(models_equal(ra.value(), rb.value()));
    }

    // -----------------------------------------------------------------------
    // 5. THE BUILDER AND THE PARSER MEET AT THE MODEL
    //    the same table, declared both ways, agrees on shape and cover
    // -----------------------------------------------------------------------
    {
        // text: | a | bc | with a merge over columns 1-2
        auto rt = parse_table_text<>("| a | bc    |\n| d | e | f |\n");
        D_TBL_CHECK(rt.ok());
        const auto& tm = rt.value();
        // types: the same table
        using bld = DTableBuilder< DColumns<int,int,int>,
                                   DRow<tagA, DMergedCell<1,2,tagB>>,
                                   DRow<tagD, tagE, tagF> >;
        using bm = model_of_t<bld>;
        // the two front ends agree on the domain ...
        D_TBL_CHECK(tm.cols() == bm::width);
        D_TBL_CHECK(tm.rows() == bm::height);
        // ... and on the cover: the merge sits at (0,1) spanning 2, in both
        static_assert(bm::owner_of<0,2>::anchor_col == 1, "type side");
        // text side
        D_TBL_CHECK(tm.layout().owner_of(0,2).anchor_col() == 1);
        D_TBL_CHECK((tm.layout().owner_of(0,2).cols
                    == bm::owner_of<0,2>::cols));
    }

    // -----------------------------------------------------------------------
    // 6. STRICTNESS + errors
    // -----------------------------------------------------------------------
    {
        // no delimited line at all
        auto bad = parse_table_text<>("hello\n");
        D_TBL_CHECK(!bad.ok());
        D_TBL_CHECK(bad.error().status() == parse::DParseStatusMalformed);
    }
    {
        // a lenient dialect tolerates a short row; the exact one is the default
        DParseGridOptions lenient;
        lenient.cell_count = DTableStrictness::lenient;
        auto ok = parse_table_text<>("| a | b | c |\n| d |\n", lenient);
        D_TBL_CHECK(ok.ok());
    }

    return true;
}

NS_END  // testing
NS_END  // djinterp
