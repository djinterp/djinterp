/*******************************************************************************
* djinterp [cpp][container]                             table_tests_template.cpp
*
* table_template.hpp: table templates and their interpolation.
*
*   Converted on 2026.11.16 from test_table_template.cpp, an assert-based
* program no leaf built: each assertion is one check, in its order.
*
*
* path:      /tests/djinterp/cpp/container/table/table_tests_template.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/
// std
#include <type_traits>  // std::is_same
// djinterp
#include "table_tests.hpp"  // D_TBL_CHECK, the declarations
#include <djinterp/cpp/container/table/table_template.hpp>  // table_template.hpp
#include <djinterp/cpp/container/table/table_parser.hpp>  // table_parser.hpp


NS_DJINTERP
NS_TESTING

// The fixtures of the original program, in its order.  Unnamed, so a
// fixture name reused by another section never breaks the one-definition
// rule.
namespace
{

// std
// djinterp

}  // namespace


/*
tests_table_template
  table templates and their interpolation
  Tests the following:
  - each assertion of the original block, as one check, in its order
*/
bool tests_table_template()
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
    D_TBL_CHECK(pr.ok());
    const auto& t = pr.value();
    D_TBL_CHECK(t.cols() == 5);
    D_TBL_CHECK(t.at(0,0) == "headers...");
    // the placeholder spans 5 cells
    D_TBL_CHECK(t.layout().owner_of(0,0).cols == 5);

    // ... and `headers` is bound to only FOUR values (the sketch's exact case)
    DMultiBindings b4;
    b4.set("headers", {"h1","h2","h3","h4"});

    // exact  -> an error, named
    {
        auto r = expand_multi_placeholders(t, b4, DTableStrictness::exact);
        D_TBL_CHECK(!r.ok());
    }
    // pad    -> the shortfall is filled; the 5th cell is empty
    {
        auto r = expand_multi_placeholders(t, b4, DTableStrictness::pad);
        D_TBL_CHECK(r.ok());
        const auto& m = r.value();
        D_TBL_CHECK(m.at(0,0) == "h1" && m.at(0,3) == "h4");
        D_TBL_CHECK(m.at(0,4) == "");             // padded
        // the run resolved INTO the grid
        D_TBL_CHECK(!m.has_merges());
    }
    // lenient -> the same, tolerated silently
    {
        auto r = expand_multi_placeholders(t, b4, DTableStrictness::lenient);
        D_TBL_CHECK(r.ok());
        D_TBL_CHECK(r.value().at(0,4) == "");
    }
    // truncate -> a shortfall is NOT a surplus; still refused
    {
        auto r = expand_multi_placeholders(t, b4, DTableStrictness::truncate);
        D_TBL_CHECK(!r.ok());
    }

    // and the SURPLUS case: seven values into five cells
    {
        DMultiBindings b7;
        b7.set("headers", {"1","2","3","4","5","6","7"});
        auto ex = expand_multi_placeholders(t, b7, DTableStrictness::exact);
        D_TBL_CHECK(!ex.ok());
        auto tr = expand_multi_placeholders(t, b7, DTableStrictness::truncate);
        D_TBL_CHECK(tr.ok());
        D_TBL_CHECK(tr.value().at(0,4) == "5");   // the surplus dropped
    }
    // an EXACT fit under exact
    {
        DMultiBindings b5;
        b5.set("headers", {"p","q","r","s","t"});
        auto r = expand_multi_placeholders(t, b5, DTableStrictness::exact);
        D_TBL_CHECK(r.ok());
        D_TBL_CHECK(r.value().at(0,0) == "p" && r.value().at(0,4) == "t");
    }

    // -----------------------------------------------------------------------
    // 2. AN UNBOUND placeholder is LEFT STANDING (partial expansion), matching
    //    interpolate.hpp's own miss semantics
    // -----------------------------------------------------------------------
    {
        DMultiBindings none;
        auto r = expand_multi_placeholders(t, none, DTableStrictness::exact);
        D_TBL_CHECK(r.ok());                                  // not an error
        D_TBL_CHECK(r.value().at(0,0) == "headers...");       // untouched
        // span preserved
        D_TBL_CHECK(r.value().layout().owner_of(0,0).cols == 5);
    }

    // -----------------------------------------------------------------------
    // 3. CELL INTERPOLATION via the real interpolate.hpp
    // -----------------------------------------------------------------------
    {
        auto p = parse_table_text<>("| hello {who} | n={n}  |\n| plain       | {miss} |\n");
        D_TBL_CHECK(p.ok());
        auto m = interpolate_cells(p.value(),
                                   bindings<char>({{"who","world"},{"n","42"}}));
        D_TBL_CHECK(m.at(0,0) == "hello world");
        D_TBL_CHECK(m.at(0,1) == "n=42");
        D_TBL_CHECK(m.at(1,0) == "plain");
        D_TBL_CHECK(m.at(1,1) == "{miss}");        // a miss is left standing
    }
    // a merged cell is interpolated once, at its anchor; the cover survives
    {
        DTableModelValue<> v(2,3);
        v.set(0,0,"x");
        v.merge(0,1,1,2,"hi {who}");
        v.set(1,0,"a"); v.set(1,1,"b"); v.set(1,2,"c");
        auto m = interpolate_cells(v, bindings<char>({{"who","there"}}));
        D_TBL_CHECK(m.has_merges());
        D_TBL_CHECK(m.at(0,1) == "hi there");
        D_TBL_CHECK(m.at(0,2) == "hi there");     // the covered position defers
        D_TBL_CHECK(m.layout().owner_of(0,2).anchor_col() == 1);
    }

    // -----------------------------------------------------------------------
    // 4. ITERATION sigil detection (the sketch writes both bar# and #gamma)
    // -----------------------------------------------------------------------
    D_TBL_CHECK(is_iterated("bar#")   && iteration_of("bar#")   == "bar");
    D_TBL_CHECK(is_iterated("#gamma") && iteration_of("#gamma") == "gamma");
    D_TBL_CHECK(!is_iterated("plain") && iteration_of("plain")  == "plain");
    D_TBL_CHECK(is_multi_placeholder("headers...")
                && multi_placeholder_key("headers...") == "headers");
    D_TBL_CHECK(!is_multi_placeholder("headers"));
    // a bare ellipsis names no key
    D_TBL_CHECK(!is_multi_placeholder("..."));

    // -----------------------------------------------------------------------
    // 5. THE WHOLE PIPELINE: text -> parse -> expand -> interpolate -> render
    // -----------------------------------------------------------------------
    {
        const std::string src =
            "| cols...            |\n"
            "|--------------------|\n"
            "| {a} | {b} | {c}    |\n";
        auto p = parse_table_text<>(src);
        D_TBL_CHECK(p.ok());
        DMultiBindings mb;
        mb.set("cols", {"X","Y","Z"});
        auto e = expand_multi_placeholders(p.value(), mb,
                                           DTableStrictness::exact);
        D_TBL_CHECK(e.ok());
        auto f = interpolate_cells(e.value(),
                                   bindings<char>({{"a","1"},{"b","2"},{"c","3"}}));
        D_TBL_CHECK(f.at(0,0) == "1" && f.at(0,2) == "3");
        // the header block survived both passes ...
        D_TBL_CHECK(f.metadata().has_column_headers());
        // ... AND its run placeholder expanded: `cols...` is now X | Y | Z
        const auto& hs = f.metadata().column_headers();
        D_TBL_CHECK(hs.size() == 1);
        D_TBL_CHECK(hs[0].size() == 3);
        D_TBL_CHECK(hs[0][0].label == "X" && hs[0][1].label == "Y"
                    && hs[0][2].label == "Z");
        for (const auto& hc : hs[0]) D_TBL_CHECK(hc.span == 1);
        // the level's EXTENT is unchanged, so table_metadata still conforms
        D_TBL_CHECK(f.metadata().conforms_columns(3));
        D_TBL_CHECK(f.conforms());
    }

    return true;
}

NS_END  // testing
NS_END  // djinterp
