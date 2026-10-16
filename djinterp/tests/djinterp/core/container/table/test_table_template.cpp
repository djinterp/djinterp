/*******************************************************************************
* djinterp [core]                                        test_table_template.cpp
*
*
* path:      /tests/djinterp/core/container/table/test_table_template.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <cassert>
#include <iostream>
// djinterp
#include "djinterp/core/container/table/table_template.hpp"
#include "djinterp/core/container/table/table_parser.hpp"
using namespace djinterp;

int main()
{
    // -----------------------------------------------------------------------
    // 1. THE SKETCH'S QUESTION, ANSWERED:
    //    "| headers... ||||| is a shorthand ... this is saying apply placeholder
    //     `headers` (which is multiple elements) to 5 cells.
    //     What happens if headers... doesnt fit 1:1 for all cells?
    //     what if headers is e.g. 4?"
    // -----------------------------------------------------------------------
    // a 5-column grid whose single cell is a run placeholder spanning all 5
    const std::string tpl = "| headers...       |\n"
                            "| a | b | c | d | e |\n";
    auto pr = parse_table_text<>(tpl);
    assert(pr.ok());
    const auto& t = pr.value();
    assert(t.cols() == 5);
    assert(t.at(0,0) == "headers...");
    assert(t.layout().owner_of(0,0).cols == 5);   // the placeholder spans 5 cells
    std::cout << "--- parsed: 'headers...' spans " << t.layout().owner_of(0,0).cols
              << " cells\n";

    // ... and `headers` is bound to only FOUR values (the sketch's exact case)
    multi_bindings b4;
    b4.set("headers", {"h1","h2","h3","h4"});

    // exact  -> an error, named
    {
        auto r = expand_multi_placeholders(t, b4, table_strictness::exact);
        assert(!r.ok());
        std::cout << "  exact    -> refused: \""
                  << r.error().message().substr(0, 52) << "...\"\n";
    }
    // pad    -> the shortfall is filled; the 5th cell is empty
    {
        auto r = expand_multi_placeholders(t, b4, table_strictness::pad);
        assert(r.ok());
        const auto& m = r.value();
        assert(m.at(0,0) == "h1" && m.at(0,3) == "h4");
        assert(m.at(0,4) == "");             // padded
        assert(!m.has_merges());             // the run resolved INTO the grid
        std::cout << "  pad      -> " << render(m).substr(0, render(m).find('\n')) << "\n";
    }
    // lenient -> the same, tolerated silently
    {
        auto r = expand_multi_placeholders(t, b4, table_strictness::lenient);
        assert(r.ok());
        assert(r.value().at(0,4) == "");
        std::cout << "  lenient  -> admitted\n";
    }
    // truncate -> a shortfall is NOT a surplus; still refused
    {
        auto r = expand_multi_placeholders(t, b4, table_strictness::truncate);
        assert(!r.ok());
        std::cout << "  truncate -> refused (a shortfall is not a surplus)\n";
    }

    // and the SURPLUS case: seven values into five cells
    {
        multi_bindings b7;
        b7.set("headers", {"1","2","3","4","5","6","7"});
        auto ex = expand_multi_placeholders(t, b7, table_strictness::exact);
        assert(!ex.ok());
        auto tr = expand_multi_placeholders(t, b7, table_strictness::truncate);
        assert(tr.ok());
        assert(tr.value().at(0,4) == "5");   // the surplus dropped
        std::cout << "  surplus(7->5) truncate -> "
                  << render(tr.value()).substr(0, render(tr.value()).find('\n')) << "\n";
    }
    // an EXACT fit under exact
    {
        multi_bindings b5;
        b5.set("headers", {"p","q","r","s","t"});
        auto r = expand_multi_placeholders(t, b5, table_strictness::exact);
        assert(r.ok());
        assert(r.value().at(0,0) == "p" && r.value().at(0,4) == "t");
        std::cout << "  exact fit(5->5)        -> "
                  << render(r.value()).substr(0, render(r.value()).find('\n')) << "\n";
    }

    // -----------------------------------------------------------------------
    // 2. AN UNBOUND placeholder is LEFT STANDING (partial expansion), matching
    //    interpolate.hpp's own miss semantics
    // -----------------------------------------------------------------------
    {
        multi_bindings none;
        auto r = expand_multi_placeholders(t, none, table_strictness::exact);
        assert(r.ok());                                  // not an error
        assert(r.value().at(0,0) == "headers...");       // untouched
        assert(r.value().layout().owner_of(0,0).cols == 5);  // span preserved
        std::cout << "--- unbound key -> left standing, span preserved\n";
    }

    // -----------------------------------------------------------------------
    // 3. CELL INTERPOLATION via the real interpolate.hpp
    // -----------------------------------------------------------------------
    {
        auto p = parse_table_text<>("| hello {who} | n={n}  |\n| plain       | {miss} |\n");
        assert(p.ok());
        auto m = interpolate_cells(p.value(), bindings<char>({{"who","world"},{"n","42"}}));
        assert(m.at(0,0) == "hello world");
        assert(m.at(0,1) == "n=42");
        assert(m.at(1,0) == "plain");
        assert(m.at(1,1) == "{miss}");        // a miss is left standing
        std::cout << "--- interpolate_cells:\n" << render(m);
    }
    // a merged cell is interpolated once, at its anchor; the cover survives
    {
        table_model_value<> v(2,3);
        v.set(0,0,"x");
        v.merge(0,1,1,2,"hi {who}");
        v.set(1,0,"a"); v.set(1,1,"b"); v.set(1,2,"c");
        auto m = interpolate_cells(v, bindings<char>({{"who","there"}}));
        assert(m.has_merges());
        assert(m.at(0,1) == "hi there");
        assert(m.at(0,2) == "hi there");     // the covered position defers
        assert(m.layout().owner_of(0,2).anchor_col() == 1);
        std::cout << "--- merged cell interpolated once, cover survives\n";
    }

    // -----------------------------------------------------------------------
    // 4. ITERATION sigil detection (the sketch writes both bar# and #gamma)
    // -----------------------------------------------------------------------
    assert(is_iterated("bar#")   && iteration_of("bar#")   == "bar");
    assert(is_iterated("#gamma") && iteration_of("#gamma") == "gamma");
    assert(!is_iterated("plain") && iteration_of("plain")  == "plain");
    assert(is_multi_placeholder("headers...") && multi_placeholder_key("headers...") == "headers");
    assert(!is_multi_placeholder("headers"));
    assert(!is_multi_placeholder("..."));    // a bare ellipsis names no key
    std::cout << "--- sigil/ellipsis detection: PASS\n";

    // -----------------------------------------------------------------------
    // 5. THE WHOLE PIPELINE: text -> parse -> expand -> interpolate -> render
    // -----------------------------------------------------------------------
    {
        const std::string src =
            "| cols...            |\n"
            "|--------------------|\n"
            "| {a} | {b} | {c}    |\n";
        auto p = parse_table_text<>(src);
        assert(p.ok());
        multi_bindings mb;
        mb.set("cols", {"X","Y","Z"});
        auto e = expand_multi_placeholders(p.value(), mb, table_strictness::exact);
        assert(e.ok());
        auto f = interpolate_cells(e.value(), bindings<char>({{"a","1"},{"b","2"},{"c","3"}}));
        std::cout << "--- full pipeline:\n" << render(f);
        assert(f.at(0,0) == "1" && f.at(0,2) == "3");
        // the header block survived both passes ...
        assert(f.metadata().has_column_headers());
        // ... AND its run placeholder expanded: `cols...` is now X | Y | Z
        const auto& hs = f.metadata().column_headers();
        assert(hs.size() == 1);
        assert(hs[0].size() == 3);
        assert(hs[0][0].label == "X" && hs[0][1].label == "Y" && hs[0][2].label == "Z");
        for (const auto& hc : hs[0]) assert(hc.span == 1);
        // the level's EXTENT is unchanged, so table_metadata still conforms
        assert(f.metadata().conforms_columns(3));
        assert(f.conforms());
    }

    std::cout << "\nall template assertions passed\n";
    return 0;
}
