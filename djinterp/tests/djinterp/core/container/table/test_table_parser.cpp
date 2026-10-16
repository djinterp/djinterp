/*******************************************************************************
* djinterp [core]                                          test_table_parser.cpp
*
*
* path:      /tests/djinterp/core/container/table/test_table_parser.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <cassert>
#include <iostream>
// djinterp
#include "djinterp/core/container/table/table_parser.hpp"
#include "djinterp/core/container/table/table_builder.hpp"
using namespace djinterp;
struct tagA{}; struct tagB{}; struct tagD{}; struct tagE{}; struct tagF{};

int main()
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
    assert(r.ok());
    const auto& m = r.value();

    std::cout << "--- parsed the sketch's grid ---\n";
    std::cout << "  columns: " << m.cols() << ", body rows: " << m.rows() << "\n";
    assert(m.cols() == 5);      // the finest line resolves 5 columns
    assert(m.rows() == 1);      // one body row (below the separator)
    assert(m.at(0,0) == "1" && m.at(0,4) == "5");

    // the header block: two levels, outermost first
    assert(m.metadata().has_column_headers());
    const auto& stack = m.metadata().column_headers();
    assert(stack.size() == 2);

    // THE KEY ASSERTION: the outer level's spans were read off the delimiters
    const auto& outer = stack[0];
    assert(outer.size() == 3);
    assert(outer[0].span == 1);                       // the empty leading cell
    assert(outer[1].label == "something"      && outer[1].span == 2);  // bar+alpha
    assert(outer[2].label == "something else" && outer[2].span == 2);  // beta+gamma
    std::cout << "  outer header: '" << outer[1].label << "' spans " << outer[1].span
              << ", '" << outer[2].label << "' spans " << outer[2].span << "\n";

    // the finer level is 5 unit-span labels
    const auto& inner = stack[1];
    assert(inner.size() == 5);
    for (const auto& hc : inner) assert(hc.span == 1);
    assert(inner[0].label == "foo" && inner[4].label == "gamma");

    // table_metadata's own conformance: the finest level spans the columns
    assert(m.metadata().conforms_columns(5));
    assert(m.conforms());

    // -----------------------------------------------------------------------
    // 2. A MERGED BODY CELL becomes a merge in the cover, anchored where it starts
    // -----------------------------------------------------------------------
    const std::string body_merge =
        "| a | bc     |\n"
        "| d | e | f  |\n";
    auto r2 = parse_table_text<>(body_merge);
    assert(r2.ok());
    const auto& m2 = r2.value();
    assert(m2.cols() == 3 && m2.rows() == 2);
    assert(m2.has_merges());
    assert(m2.layout().merge_count() == 1);
    // T[i] = T[anchor(cell(i))] : the covered position reports the merge's value
    assert(m2.at(0,1) == "bc");
    assert(m2.at(0,2) == "bc");
    assert(m2.layout().owner_of(0,2).anchor_col() == 1);
    assert(m2.at(1,2) == "f");
    std::cout << "--- body merge: (0,2) defers to anchor col "
              << m2.layout().owner_of(0,2).anchor_col() << "\n";

    // -----------------------------------------------------------------------
    // 3. THE ROUND-TRIP LAW:  parse . render = id
    //    A model built by hand renders to text that parses back to the same model.
    // -----------------------------------------------------------------------
    {
        table_model_value<> v(2, 3);
        v.set(0,0,"a");
        v.merge(0,1,1,2,"bc");
        v.set(1,0,"d"); v.set(1,1,"e"); v.set(1,2,"f");
        assert(survives_round_trip(v));
        std::cout << "--- parse . render = id  (merged model) : PASS\n";
    }
    {
        table_model_value<> p(3, 4);
        for (std::size_t i = 0; i < 3; ++i)
            for (std::size_t j = 0; j < 4; ++j)
                p.set(i, j, std::string(1, char('a' + i*4 + j)));
        assert(!p.has_merges());
        assert(survives_round_trip(p));   // Gamma_0 round-trips too
        std::cout << "--- parse . render = id  (plain model)  : PASS\n";
    }
    {
        // an over-long merge (render widens the span's last column)
        table_model_value<> w(2, 3);
        w.set(0,0,"x");
        w.merge(0,1,1,2,"a very long merged label");
        w.set(1,0,"y"); w.set(1,1,"p"); w.set(1,2,"q");
        assert(survives_round_trip(w));
        std::cout << "--- parse . render = id  (wide merge)   : PASS\n";
    }

    // -----------------------------------------------------------------------
    // 4. THE OTHER LEG:  render . parse = id  on canonical text
    // -----------------------------------------------------------------------
    {
        const std::string canonical = "| a | bc    |\n| d | e | f |\n";
        assert(renders_to_same_text(canonical));
        std::cout << "--- render . parse = id (canonical text): PASS\n";
        // a NON-canonical but equivalent grid parses to the same model and
        // renders to the canonical spelling -- which is what a canonical form is for
        const std::string sloppy = "|a|bc|\n|d|e|f|\n";
        auto ra = parse_table_text<>(canonical);
        auto rb = parse_table_text<>(sloppy);
        assert(ra.ok() && rb.ok());
        assert(models_equal(ra.value(), rb.value()));
        std::cout << "--- equivalent spellings -> same model  : PASS\n";
    }

    // -----------------------------------------------------------------------
    // 5. THE BUILDER AND THE PARSER MEET AT THE MODEL
    //    the same table, declared both ways, agrees on shape and cover
    // -----------------------------------------------------------------------
    {
        // text: | a | bc | with a merge over columns 1-2
        auto rt = parse_table_text<>("| a | bc    |\n| d | e | f |\n");
        assert(rt.ok());
        const auto& tm = rt.value();
        // types: the same table
        using bld = table_builder< columns<int,int,int>,
                                   row<tagA, merged_cell<1,2,tagB>>,
                                   row<tagD, tagE, tagF> >;
        using bm = model_of_t<bld>;
        // the two front ends agree on the domain ...
        assert(tm.cols() == bm::width);
        assert(tm.rows() == bm::height);
        // ... and on the cover: the merge sits at (0,1) spanning 2, in both
        static_assert(bm::owner_of<0,2>::anchor_col == 1, "type side");
        assert(tm.layout().owner_of(0,2).anchor_col() == 1);          // text side
        assert((tm.layout().owner_of(0,2).cols == bm::owner_of<0,2>::cols));
        std::cout << "--- builder and parser agree on the model: PASS\n";
    }

    // -----------------------------------------------------------------------
    // 6. STRICTNESS + errors
    // -----------------------------------------------------------------------
    {
        // no delimited line at all
        auto bad = parse_table_text<>("hello\n");
        assert(!bad.ok());
        assert(bad.error().status() == parse::DParseStatusMalformed);
        std::cout << "--- malformed input rejected: \""
                  << bad.error().message().substr(0, 46) << "...\"\n";
    }
    {
        // a lenient dialect tolerates a short row; the exact one is the default
        parse_grid_options lenient;
        lenient.cell_count = table_strictness::lenient;
        auto ok = parse_table_text<>("| a | b | c |\n| d |\n", lenient);
        assert(ok.ok());
        std::cout << "--- lenient dialect admits a short row: PASS\n";
    }

    std::cout << "\nall parser assertions passed\n";
    return 0;
}
