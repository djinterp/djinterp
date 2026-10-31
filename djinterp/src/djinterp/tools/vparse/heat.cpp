/*******************************************************************************
* vparse [vparse]                                                       heat.cpp
*
* Building a run's heat space from its trace, and reading it back.
*   One pass over the trace does everything: each step is counted at its cell,
* an arrival is counted whenever the cell differs from the previous step's, and
* the call stack is replayed -- CALL pushes a frame, RETURN pops one, and a
* backtrack pops frames until the depth matches the next step's -- recording at
* every step the innermost frame open as it runs. The end-of-input gate belongs
* to no rule, so its steps are not counted and it ends any stay in a cell.
*
* path:      /src/djinterp/tools/vparse/heat.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.23
*                                                            revised: 2026.10.17
*******************************************************************************/
#include "../../../../inc/djinterp/tools/vparse/heat.hpp"  // corresponding header
#if D_ENV_LANG_IS_CPP17_OR_HIGHER  // the floor the header has
// std
#include <algorithm>  // std::max, std::min, std::reverse, std::sort
#include <cstdio>     // std::snprintf
#include <map>        // std::map
// djinterp
#include "djinterp/tools/ui/export/svg.hpp"      // field_svg, svg_options
#include "djinterp/tools/ui/lenses/heatmap.hpp"  // heatmap_lens
// re_std
#include "re_std/cpp/cstdint/cstdint.hpp"  // re_std::uint32_t


namespace vparse
{
namespace workbench
{

namespace
{

// the widest map drawn a column per character; longer inputs share columns
constexpr std::size_t max_columns = 160;

// rgba_of
//   a 0xRRGGBB colour at opacity _alpha.
ui::rgba rgba_of(
    unsigned _hex,
    float    _alpha = 1.0f
)
{
    return ui::rgba(static_cast<float>((_hex >> 16) & 0xFF) / 255.0f,
                    static_cast<float>((_hex >> 8) & 0xFF) / 255.0f,
                    static_cast<float>(_hex & 0xFF) / 255.0f,
                    _alpha);
}

// owner_rows
//   per program address, the row of the rule whose code it is; -1 in the
// end-of-input gate.
std::vector<int> owner_rows(
    const run_record& _run
)
{
    const int                  rules = static_cast<int>(_run.rule_names.size());
    std::map<std::string, int> index;

    for (int i = 0; i < rules; ++i)
    {
        index[_run.rule_names[static_cast<std::size_t>(i)]] = i;
    }

    std::vector<int> owner(_run.program.size(), -1);
    int              row = -1;

    for (std::size_t at = 0; at < _run.program.size(); ++at)
    {
        const auto hit = (at < _run.entry.size()) ? index.find(_run.entry[at])
                                                  : index.end();

        // a rule's first address starts its rows; the start rule is on top
        if (hit != index.end())
        {
            row = rules - 1 - hit->second;
        }

        owner[at] = row;
    }

    return owner;
}

// where
//   a cell's column as text: the character there and its line:column, or
// the range of characters a shared column covers.
std::string where(
    const heat_space& _space,
    const run_record& _run,
    int               _col
)
{
    const std::string& input = _run.request.input;
    const std::size_t  from  = static_cast<std::size_t>(_col) * _space.bucket;

    if (_space.bucket == 1)
    {
        const std::pair<int, int> at = line_col(input, static_cast<int>(from));

        return std::to_string(at.first) + ":" + std::to_string(at.second);
    }

    const std::size_t to = std::min(from + _space.bucket, input.size() + 1) - 1;

    return "chars " + std::to_string(from) + "-" + std::to_string(to);
}

// text_at
//   the input a cell's column covers, escaped and kept short.
std::string text_at(
    const heat_space& _space,
    const run_record& _run,
    int               _col
)
{
    const std::string& input = _run.request.input;
    const std::size_t  from  = static_cast<std::size_t>(_col) * _space.bucket;

    if (from >= input.size())
    {
        return "end";
    }

    const std::string text = escape(input.substr(from, _space.bucket));

    return (text.size() > 16) ? text.substr(0, 15) + "~" : text;
}

// add_frontier
//   marks, in _overlay, where a failed run got furthest: the innermost rule
// at the furthest position.
void add_frontier(
    ui::field_overlay& _overlay,
    const heat_space&  _space,
    const run_record&  _run
)
{
    const parse_pass& rec = _run.recognition;

    // only a run that failed has a frontier worth marking
    if ( (!rec.ran) ||
         (rec.matched) ||
         (rec.far.offset < 0) ||
         (rec.far.stack.empty()) )
    {
        return;
    }

    const std::string& rule = rec.far.stack.back();

    for (std::size_t r = 0; r < _space.rows.size(); ++r)
    {
        if (_space.rows[r] == rule)
        {
            ui::field_mark mark;
            mark.cell.col  = static_cast<re_std::uint32_t>(
                static_cast<std::size_t>(rec.far.offset) / _space.bucket);
            mark.cell.row  = static_cast<re_std::uint32_t>(r);
            mark.color     = rgba_of(0xB42318);
            mark.thickness = 2.0;
            _overlay.marks.push_back(mark);

            return;
        }
    }

    return;
}

}  // anonymous namespace


/*
build_heat
  One replay of the trace. The stack is kept as frames with parent links, and
each step records the innermost frame open as it runs, so any step's stack
is recovered by walking parents.
*/
heat_space build_heat(
    const run_record& _run
)
{
    const parse_pass&  rec       = _run.recognition;
    const std::string& input     = _run.request.input;
    const std::size_t  positions = input.size() + 1;
    const std::size_t  bucket    = (positions + max_columns - 1) / max_columns;
    const std::size_t  cols      = (positions + bucket - 1) / bucket;

    ui::axis x;
    x.name = "input position";
    x.kind = ui::axis_kind::discrete;

    for (std::size_t c = 0; c < cols; ++c)
    {
        const std::size_t from = c * bucket;
        const std::size_t to   = std::min(from + bucket, positions) - 1;

        // one character per column reads as the character itself
        if (bucket == 1)
        {
            x.labels.push_back((from < input.size())
                                   ? escape(input.substr(from, 1))
                                   : std::string("end"));
        }
        else
        {
            x.labels.push_back(std::to_string(from) + "-" +
                               std::to_string(to));
        }
    }

    ui::axis y;
    y.name   = "rule";
    y.kind   = ui::axis_kind::discrete;
    y.labels = std::vector<std::string>(_run.rule_names.rbegin(),
                                        _run.rule_names.rend());

    heat_space space({ x, y }, rec.trace.size());
    space.bucket  = bucket;
    space.columns = x.labels;
    space.rows    = y.labels;

    // nothing to count without a trace or rules
    if ( (rec.trace.empty()) ||
         (y.labels.empty()) )
    {
        return space;
    }

    const std::vector<int> owner = owner_rows(_run);
    const std::size_t      n     = rec.trace.size();
    const auto             row_of = [&owner](int _ip)
    {
        return ( (_ip >= 0) &&
                 (static_cast<std::size_t>(_ip) < owner.size()) )
                   ? owner[static_cast<std::size_t>(_ip)] : -1;
    };

    space.step_col.assign(n, -1);
    space.step_row.assign(n, -1);
    space.step_top.assign(n, -1);

    int top      = -1;
    int prev_col = -1;
    int prev_row = -1;

    for (std::size_t k = 0; k < n; ++k)
    {
        const trace_step& t   = rec.trace[k];
        const int         row = row_of(t.ip);
        const int         col = static_cast<int>(
            std::min(static_cast<std::size_t>(t.off), positions - 1) /
            bucket);

        space.step_top[k] = top;

        // rule code at a position: a step there, and an arrival if it is new
        if (row >= 0)
        {
            const std::vector<std::size_t> cell = {
                static_cast<std::size_t>(col),
                static_cast<std::size_t>(row) };

            space.step_col[k] = col;
            space.step_row[k] = row;
            space.steps.visit(cell, k);

            if ( (col != prev_col) ||
                 (row != prev_row) )
            {
                space.entries.visit(cell, k);
            }
        }

        prev_col = (row >= 0) ? col : -1;
        prev_row = row;

        // the stack after this instruction: a CALL opens a frame, and so
        // does a MEMO the memo did not answer, which the next step shows a
        // call deeper
        const vp::peg::DInstr& ins   =
            _run.program[static_cast<std::size_t>(t.ip)];
        const bool            opens =
            ( (ins.op == vp::peg::CALL) ||
              ( (ins.op == vp::peg::MEMO) &&
                ((k + 1) < n) &&
                (rec.trace[k + 1].calls > t.calls) ) );

        // a call opens a frame; a return closes one
        if (opens)
        {
            const int depth = (top >= 0)
                ? (space.frames[static_cast<std::size_t>(top)].depth + 1)
                : 1;

            space.frames.push_back(
                heat_space::frame{ row_of(ins.arg), col, top, depth });
            top = static_cast<int>(space.frames.size()) - 1;
        }
        else if ( (ins.op == vp::peg::RETURN) &&
                  (top >= 0) )
        {
            top = space.frames[static_cast<std::size_t>(top)].parent;
        }

        // a backtrack drops the calls made since its choice point
        if ((k + 1) < n)
        {
            while ( (top >= 0) &&
                    (space.frames[static_cast<std::size_t>(top)].depth >
                     rec.trace[k + 1].calls) )
            {
                top = space.frames[static_cast<std::size_t>(top)].parent;
            }
        }
    }

    // totals over the whole run
    for (std::size_t c = 0; c < cols; ++c)
    {
        for (std::size_t r = 0; r < space.rows.size(); ++r)
        {
            const std::size_t s = space.steps.total({ c, r });
            const std::size_t e = space.entries.total({ c, r });

            if (s == 0)
            {
                continue;
            }

            ++space.touched;
            space.revisits  += (e > 0) ? (e - 1) : 0;
            space.max_steps  = std::max(space.max_steps, s);

            if (e > space.max_entries)
            {
                space.max_entries = e;
                space.hot_col     = static_cast<int>(c);
                space.hot_row     = static_cast<int>(r);
            }
        }
    }

    return space;
}

/*
heat_colors
  Pale enough at the bottom that a cell visited once still reads as paper.
*/
ui::colormap heat_colors()
{
    ui::colormap out;
    out.stops = { { 0.00, rgba_of(0xEEF3E8).to_rgb() },
                  { 0.30, rgba_of(0xF6D55C).to_rgb() },
                  { 0.65, rgba_of(0xE8833A).to_rgb() },
                  { 1.00, rgba_of(0xB42318).to_rgb() } };

    return out;
}

/*
heat_scene
  The lens samples every cell as of the step; the scale is left to the
caller, which fixes it over the whole run so the colours hold still while
the step moves.
*/
ui::scene heat_scene(
    const heat_space& _space,
    bool              _entries,
    std::size_t       _step
)
{
    const ui::visit_source& source = _entries ? _space.entries : _space.steps;

    // an empty space draws nothing
    if (source.frame_count() == 0)
    {
        return ui::scene();
    }

    ui::config cfg;
    cfg.lens.mode = ui::lens_mode::heatmap;
    cfg.lens.axes = { 0, 1 };
    cfg.colors    = heat_colors();

    const auto built = ui::heatmap_lens().build(
        source, cfg, std::min(_step, source.frame_count() - 1));

    return built.is_ok() ? built.value() : ui::scene();
}

/*
heat_overlay
  The trail climbs from the start rule, through each open call at the column
it began, to the machine's cell; the frontier is marked where the innermost
rule at the furthest position stood.
*/
ui::field_overlay heat_overlay(
    const heat_space& _space,
    const run_record& _run,
    std::size_t       _step
)
{
    ui::field_overlay out;
    out.trail_color     = rgba_of(0x1D4E89);
    out.trail_thickness = 2.0;

    if (_space.step_top.empty())
    {
        return out;
    }

    const std::size_t k = std::min(_step, _space.step_top.size() - 1);

    // the open calls, innermost first, then reversed
    for (int f = _space.step_top[k]; f >= 0; )
    {
        const heat_space::frame& fr =
            _space.frames[static_cast<std::size_t>(f)];

        if (fr.row >= 0)
        {
            out.trail.push_back(ui::field_cell{
                static_cast<re_std::uint32_t>(fr.col),
                static_cast<re_std::uint32_t>(fr.row) });
        }

        f = fr.parent;
    }

    std::reverse(out.trail.begin(), out.trail.end());

    // the machine's own cell ends the trail and is outlined
    if (_space.step_row[k] >= 0)
    {
        const ui::field_cell here{
            static_cast<re_std::uint32_t>(_space.step_col[k]),
            static_cast<re_std::uint32_t>(_space.step_row[k]) };

        out.trail.push_back(here);

        ui::field_mark mark;
        mark.cell      = here;
        mark.color     = rgba_of(0x1F2A36);
        mark.thickness = 2.5;
        out.marks.push_back(mark);
    }

    // a failed run: where the parser got furthest
    add_frontier(out, _space, _run);

    return out;
}

/*
heat_svg
  Sized for the map, not the screen: columns get between 8 and 28 pixels,
rows a line and a half, so a picture of a short run is not mostly margin. A
picture of the whole run has no machine position to show -- where the last
step left the stack is not a summary of anything -- so only the frontier is
marked.
*/
std::string heat_svg(
    const heat_space& _space,
    const run_record& _run
)
{
    const std::size_t last = _space.step_top.empty()
                                 ? 0 : (_space.step_top.size() - 1);
    const double      cols = static_cast<double>(
        std::max<std::size_t>(1, _space.columns.size()));
    const double      cell = std::min(28.0, std::max(8.0, 1100.0 / cols));

    ui::field_scale scale;
    scale.colors = heat_colors();
    scale.low    = 1.0;
    scale.high   = std::max(1.0, static_cast<double>(_space.max_entries));

    ui::field_style style;
    style.backdrop = rgba_of(0xFFFFFF);
    style.text     = rgba_of(0x1F2A36);
    style.grid     = rgba_of(0xC9D1C4, 0.35f);

    ui::svg_options options;
    options.background = rgba_of(0xF7F9F4);
    options.title      = "Run " + std::to_string(_run.id) +
                         ": arrivals at each input position and rule";

    const ui::vec<2, double> size(
        (cols * cell) + 140.0,
        (static_cast<double>(_space.rows.size()) * 22.0) + 80.0);

    ui::field_overlay overlay;
    add_frontier(overlay, _space, _run);

    return ui::field_svg(heat_scene(_space, true, last), size, scale, style,
                         overlay, options);
}

/*
heat_listing
  Keys carry the counts, so a comparison marks cells whose work changed, not
only cells that appeared or vanished.
*/
listing heat_listing(
    const heat_space& _space,
    const run_record& _run
)
{
    struct cell
    {
        int         col;
        int         row;
        std::size_t steps;
        std::size_t entries;
    };

    std::vector<cell> cells;

    for (std::size_t c = 0; c < _space.columns.size(); ++c)
    {
        for (std::size_t r = 0; r < _space.rows.size(); ++r)
        {
            const std::size_t s = _space.steps.total({ c, r });

            if (s > 0)
            {
                cells.push_back(cell{ static_cast<int>(c),
                                      static_cast<int>(r),
                                      s,
                                      _space.entries.total({ c, r }) });
            }
        }
    }

    std::sort(cells.begin(), cells.end(),
              [](const cell& _a, const cell& _b)
              {
                  return (_a.entries != _b.entries) ? (_a.entries > _b.entries)
                       : (_a.steps != _b.steps)     ? (_a.steps > _b.steps)
                       : (_a.col != _b.col)         ? (_a.col < _b.col)
                                                    : (_a.row < _b.row);
              });

    listing out;
    out.columns = { "rule", "at", "text", "arrivals", "steps" };
    out.note    = _run.request.trace
                      ? "The program did not run, so it did no work to map."
                      : "The heat map comes from the trace, which was not "
                        "recorded for this run.";

    for (const cell& c : cells)
    {
        const std::string rule = _space.rows[static_cast<std::size_t>(c.row)];
        const std::string at   = where(_space, _run, c.col);
        const std::string text = text_at(_space, _run, c.col);

        out.rows.push_back({ rule, at, text, std::to_string(c.entries),
                             std::to_string(c.steps) });
        out.keys.push_back(rule + " " + at + " " + std::to_string(c.entries) +
                           "/" + std::to_string(c.steps));
        out.marks.push_back((c.entries > 1) ? 2 : 0);
    }

    return out;
}

/*
heat_summary
  Revisits are arrivals beyond the first at each cell: the work a memo table
would have saved.
*/
std::string heat_summary(
    const heat_space& _space
)
{
    if (_space.touched == 0)
    {
        return "-";
    }

    std::string out = std::to_string(_space.revisits) + " over " +
                      std::to_string(_space.touched) + " cells";

    if (_space.max_entries > 1)
    {
        out += "; most at " +
               _space.rows[static_cast<std::size_t>(_space.hot_row)] +
               " on column '" +
               _space.columns[static_cast<std::size_t>(_space.hot_col)] +
               "', arrived at " + std::to_string(_space.max_entries) +
               " times";
    }

    return out;
}

}  // namespace workbench
}  // namespace vparse

#endif  // floor, for now
