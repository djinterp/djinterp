/*******************************************************************************
* vparse [ui]                                                 elements_imgui.cpp
*
* The workbench's elements on Dear ImGui: their renderers, and the theme.
*   Listings are green-bar tables -- bands of three rows, as on fanfold
* printer paper -- drawn a screenful at a time with a list clipper, so a
* hundred-thousand-step trace costs what its visible rows cost. Two runs side
* by side are compared by a longest-common-subsequence over each row's key,
* computed once per pair of runs and part, and cached; runs never change after
* they finish, so the cache never goes stale.
*   The tape lays the text out as character cells, wrapping at the panel's
* width and at newlines, and draws the matched rules beneath as nested spans,
* one layer per depth. The cell under the machine's cursor at the current step
* is highlighted; for a failed run the frontier is marked and everything past it
* is greyed.
*
* path:      /src/ui/vparse/elements_imgui.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.08
*                                                            revised: 2026.10.26
*******************************************************************************/
#include "../../../inc/ui/vparse/elements.hpp"  // corresponding header
#if D_ENV_LANG_IS_CPP17_OR_HIGHER  // the floor the header has
// std
#include <algorithm>  // std::max, std::min
#include <cfloat>     // FLT_MAX
#include <climits>    // INT_MIN
#include <map>        // std::map
#include <string>     // std::string
#include <utility>    // std::move, std::pair
#include <vector>     // std::vector
// imgui
#include "imgui.h"  // widgets, tables, draw lists
// djinterp
#include "djinterp/tools/ui/frontends/imgui.hpp"  // ui::draw_field
// re_std
#include "re_std/cpp/cstdint/cstdint.hpp"  // re_std::uint16_t
// uxoxo
#include "uxoxo/platform/imgui_platform.hpp"  // imgui_platform, imgui_frame
#include "uxoxo/realize.hpp"                  // set_renderer
// vparse
#include "../../../inc/djinterp/tools/vparse/heat.hpp"  // heat_space,
                                                              // heat_scene,
                                                              // heat_overlay


namespace vparse
{
namespace workbench
{

namespace
{

using uxoxo::option_set;
using uxoxo::platform::imgui_blueprint;
using uxoxo::platform::imgui_frame;
using uxoxo::platform::imgui_platform;

//------------------------------------------------------------------ tokens
// The palette. Paper and green bar are the listing; ink is the ribbon; the
// highlighter marks the current step; red marks what went wrong.

constexpr unsigned PAPER       = 0xF7F9F4;
constexpr unsigned GREEN_BAR   = 0xDDEEDA;
constexpr unsigned MARGIN      = 0xE8EDE4;
constexpr unsigned INK         = 0x1F2A36;
constexpr unsigned PENCIL      = 0x66756A;
constexpr unsigned RULE        = 0x9DB0A0;
constexpr unsigned HIGHLIGHTER = 0xF6D55C;
constexpr unsigned RIBBON_RED  = 0xB42318;
constexpr unsigned RED_WASH    = 0xF4D9D4;
constexpr unsigned ENTRY_BLUE  = 0x1D4E89;
constexpr unsigned GREEN_INK   = 0x2F6B3A;

// span colours by depth: a pale fill and a firmer edge, like coloured pencil
constexpr unsigned SPAN_FILL[] = { 0xCFDDEE, 0xD7E9D5, 0xF1DECB,
                                   0xE3DAEE, 0xF3DDE7, 0xE2E8C8 };
constexpr unsigned SPAN_EDGE[] = { 0x3B6EA8, 0x2E7D32, 0xB5603A,
                                   0x7B5EA7, 0xA34E75, 0x6B7B24 };

ImU32 col(
    unsigned _hex,
    unsigned _alpha = 255
)
{
    return IM_COL32((_hex >> 16) & 0xFF,
                    (_hex >> 8) & 0xFF,
                    _hex & 0xFF,
                    _alpha);
}

ImVec4 vec(
    unsigned _hex,
    float    _alpha = 1.0f
)
{
    return ImVec4(static_cast<float>((_hex >> 16) & 0xFF) / 255.0f,
                  static_cast<float>((_hex >> 8) & 0xFF) / 255.0f,
                  static_cast<float>(_hex & 0xFF) / 255.0f,
                  _alpha);
}

//------------------------------------------------------------------ state

run_finder& finder()
{
    static run_finder find;

    return find;
}

// diff_flags
//   per side, 1 for each row the other side lacks.
using diff_flags = std::pair<std::vector<char>, std::vector<char> >;

// diff_keys
//   longest-common-subsequence over row keys. The common prefix and suffix
// are matched first, which leaves the table small for the usual small edit;
// if what remains is still too large, rows are compared position by position.
diff_flags diff_keys(
    const std::vector<std::string>& _a,
    const std::vector<std::string>& _b
)
{
    const std::size_t n = _a.size();
    const std::size_t m = _b.size();
    diff_flags        out{ std::vector<char>(n, 1), std::vector<char>(m, 1) };

    std::size_t lo = 0;

    while ( (lo < n) && (lo < m) && (_a[lo] == _b[lo]) )
    {
        out.first[lo] = out.second[lo] = 0;
        ++lo;
    }

    std::size_t end_a = n;
    std::size_t end_b = m;

    while ( (end_a > lo) && (end_b > lo) &&
            (_a[end_a - 1] == _b[end_b - 1]) )
    {
        --end_a;
        --end_b;
        out.first[end_a] = out.second[end_b] = 0;
    }

    const std::size_t rows = end_a - lo;
    const std::size_t cols = end_b - lo;

    // too large for the table: compare the middle position by position
    if ((rows * cols) > 4000000)
    {
        for (std::size_t i = 0; (i < rows) && (i < cols); ++i)
        {
            if (_a[lo + i] == _b[lo + i])
            {
                out.first[lo + i] = out.second[lo + i] = 0;
            }
        }

        return out;
    }

    std::vector<re_std::uint16_t> lcs((rows + 1) * (cols + 1), 0);
    const auto at = [cols](std::size_t _i, std::size_t _j)
    {
        return (_i * (cols + 1)) + _j;
    };

    for (std::size_t i = rows; i-- > 0; )
    {
        for (std::size_t j = cols; j-- > 0; )
        {
            lcs[at(i, j)] =
                (_a[lo + i] == _b[lo + j])
                    ? static_cast<re_std::uint16_t>(lcs[at(i + 1, j + 1)] + 1)
                    : std::max(lcs[at(i + 1, j)], lcs[at(i, j + 1)]);
        }
    }

    std::size_t i = 0;
    std::size_t j = 0;

    while ( (i < rows) && (j < cols) )
    {
        if (_a[lo + i] == _b[lo + j])
        {
            out.first[lo + i] = out.second[lo + j] = 0;
            ++i;
            ++j;
        }
        else if (lcs[at(i + 1, j)] >= lcs[at(i, j + 1)])
        {
            ++i;
        }
        else
        {
            ++j;
        }
    }

    return out;
}

// cached_diff
//   the diff of one part between two runs, computed once.
const diff_flags& cached_diff(
    const run_record&  _a,
    const run_record&  _b,
    const std::string& _part
)
{
    static std::map<std::string, diff_flags> cache;

    const std::string key = std::to_string(_a.id) + "/" +
                            std::to_string(_b.id) + "/" + _part;
    const auto        hit = cache.find(key);

    if (hit != cache.end())
    {
        return hit->second;
    }

    const listing left  = part_listing(_a, _part);
    const listing right = part_listing(_b, _part);

    return cache[key] = diff_keys(left.keys, right.keys);
}

//------------------------------------------------------------------ listings

bool is_trace(
    const std::string& _part
)
{
    return ( (_part == "trace") ||
             (_part == "reading_trace") );
}

// draw_table
//   one listing as a scrolling green-bar table of _size.
void draw_table(
    const char*              _id,
    const run_record&        _run,
    const std::string&       _part,
    long                     _highlight,
    const std::vector<char>* _flags,
    const std::string&       _action,
    imgui_frame&             _frame,
    ImVec2                   _size
)
{
    const bool     trace = is_trace(_part);
    const auto     hit   = _run.parts.find(_part);
    const listing  none;
    const listing& shown = ( (trace) ||
                             (hit == _run.parts.end()) ) ? none
                                                         : hit->second;

    static const std::vector<std::string> trace_columns = {
        "step", "ip", "instruction", "at", "next", "calls", "back", "caps" };

    const std::vector<std::string>& columns =
        trace ? trace_columns : shown.columns;
    const std::size_t count = trace ? trace_rows(_run, _part)
                                    : shown.rows.size();

    // an empty listing explains itself instead of drawing a bare header
    if (count == 0)
    {
        const std::string note =
            trace ? part_listing(_run, _part).note : shown.note;

        ImGui::PushStyleColor(ImGuiCol_Text, vec(PENCIL));
        ImGui::TextWrapped("%s", note.empty() ? "Nothing here."
                                              : note.c_str());
        ImGui::PopStyleColor();

        return;
    }

    const ImGuiTableFlags flags = ImGuiTableFlags_ScrollX         |
                                  ImGuiTableFlags_ScrollY         |
                                  ImGuiTableFlags_BordersInnerV   |
                                  ImGuiTableFlags_BordersOuter    |
                                  ImGuiTableFlags_SizingFixedFit  |
                                  ImGuiTableFlags_Resizable       |
                                  ImGuiTableFlags_NoSavedSettings;

    if (!ImGui::BeginTable(_id, static_cast<int>(columns.size()), flags,
                           _size))
    {
        return;
    }

    bool titled = false;

    for (const std::string& title : columns)
    {
        ImGui::TableSetupColumn(title.empty() ? " " : title.c_str());
        titled = ( (titled) || (!title.empty()) );
    }

    // untitled columns (the summary's) need no header row
    if (titled)
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();
    }

    // bring a newly highlighted row into view, once per change
    const float    row_h   = ImGui::GetTextLineHeight() +
                             (ImGui::GetStyle().CellPadding.y * 2.0f);
    ImGuiStorage*  storage = ImGui::GetStateStorage();
    const ImGuiID  seen    = ImGui::GetID("##highlight");

    if ( (_highlight >= 0) &&
         (storage->GetInt(seen, INT_MIN) != static_cast<int>(_highlight)) )
    {
        const float y       = static_cast<float>(_highlight) * row_h;
        const float top     = ImGui::GetScrollY();
        const float visible = ImGui::GetWindowHeight() - (row_h * 2.0f);

        if ( (y < top) ||
             (y > (top + visible)) )
        {
            ImGui::SetScrollY(std::max(0.0f, y - (visible * 0.4f)));
        }
    }

    storage->SetInt(seen, static_cast<int>(_highlight));

    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(count));

    while (clipper.Step())
    {
        for (int r = clipper.DisplayStart; r < clipper.DisplayEnd; ++r)
        {
            const std::size_t row = static_cast<std::size_t>(r);
            const std::vector<std::string> cells =
                trace ? trace_row(_run, _part, row) : shown.rows[row];
            const int mark = ( (!trace) && (row < shown.marks.size()) )
                                 ? shown.marks[row] : 0;

            ImGui::TableNextRow();
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,
                                   col(((r / 3) % 2) ? GREEN_BAR : PAPER));

            if ( (_flags) &&
                 (row < _flags->size()) &&
                 ((*_flags)[row]) )
            {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1,
                                       col(RED_WASH));
            }

            if (static_cast<long>(r) == _highlight)
            {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1,
                                       col(HIGHLIGHTER, 200));
            }

            const unsigned ink = (mark == 3) ? ENTRY_BLUE
                               : (mark == 2) ? RIBBON_RED
                               : (mark == 1) ? PENCIL
                                             : INK;

            ImGui::PushStyleColor(ImGuiCol_Text, vec(ink));

            for (std::size_t c = 0; c < columns.size(); ++c)
            {
                ImGui::TableSetColumnIndex(static_cast<int>(c));

                const std::string& cell = (c < cells.size()) ? cells[c]
                                                             : columns[c];
                const char* text = (c < cells.size()) ? cell.c_str() : "";

                // the first cell of a clickable row spans the whole row
                if ( (c == 0) &&
                     (!_action.empty()) )
                {
                    ImGui::PushID(r);

                    if (ImGui::Selectable(text, false,
                                          ImGuiSelectableFlags_SpanAllColumns |
                                          ImGuiSelectableFlags_AllowOverlap))
                    {
                        option_set payload;
                        payload.set("value", static_cast<long>(r));
                        _frame.post(_action, payload);
                    }

                    ImGui::PopID();
                }
                else
                {
                    ImGui::TextUnformatted(text);
                }
            }

            ImGui::PopStyleColor();
        }
    }

    ImGui::EndTable();

    return;
}

// keyed
//   the blueprint of one of the workbench's elements: _draw, with the text
// _text, keyed by the element's "key" attribute, and with its "target"
// attribute copied into the payload of every event the drawing posts -- the
// two attributes uxoxo's own archetypes give the same meaning everywhere.
template<typename Draw>
imgui_blueprint keyed(
    const option_set& _attrs,
    std::string       _text,
    Draw              _draw
)
{
    return imgui_blueprint{
        [_attrs, _draw](imgui_frame& _frame)
        {
            const std::size_t               before = _frame.events.size();
            const ::djinterp::option_value* target = _attrs.find("target");

            _draw(_frame);

            // what this element posted carries its target
            for (std::size_t i = before;
                 (target != nullptr) && (i < _frame.events.size());
                 ++i)
            {
                _frame.events[i].payload.set("target", *target);
            }

            return;
        },
        std::move(_text),
        _attrs.as_string("key", "") };
}

imgui_blueprint render_listing(
    const uxoxo::element_node<imgui_blueprint>& _node
)
{
    const option_set attrs = _node.attrs;

    const auto draw = [attrs](imgui_frame& _frame)
    {
        const std::string part   = attrs.as_string("part", "summary");
        const std::string action = attrs.as_string("action", "");
        const long        row    = attrs.as_long("row", -1);
        const run_record* run    = finder()(
            static_cast<int>(attrs.as_long("run", -1)));
        const run_record* other  = finder()(
            static_cast<int>(attrs.as_long("other", -1)));

        if (!run)
        {
            ImGui::TextDisabled("No run selected.");

            return;
        }

        const ImVec2 avail = ImGui::GetContentRegionAvail();

        // one run: the whole space
        if ( (!other) ||
             (other == run) )
        {
            draw_table("##listing", *run, part, row, nullptr, action,
                       _frame, avail);

            return;
        }

        // two runs: side by side, differing rows washed red
        const diff_flags* flags =
            is_trace(part) ? nullptr : &cached_diff(*run, *other, part);
        const float half =
            (avail.x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

        const auto side = [&](const run_record&        _side,
                              const std::vector<char>* _marks,
                              const char*              _id,
                              long                     _row,
                              const std::string&       _act)
        {
            std::size_t changed = 0;

            if (_marks)
            {
                for (char m : *_marks)
                {
                    changed += (m != 0) ? 1 : 0;
                }
            }

            ImGui::BeginGroup();
            ImGui::Text("Run %d", _side.id);

            if (_marks)
            {
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text,
                                      vec(changed ? RIBBON_RED : PENCIL));
                if (changed)
                {
                    ImGui::Text("%zu rows not in the other run", changed);
                }
                else
                {
                    ImGui::TextUnformatted("same rows");
                }
                ImGui::PopStyleColor();
            }

            draw_table(_id, _side, part, _row, _marks, _act, _frame,
                       ImVec2(half, ImGui::GetContentRegionAvail().y));
            ImGui::EndGroup();

            return;
        };

        side(*run, flags ? &flags->first : nullptr, "##left", row,
             action);
        ImGui::SameLine();
        side(*other, flags ? &flags->second : nullptr, "##right", -1,
             "");

        return;
    };

    return keyed(attrs, attrs.as_string("part", ""), draw);
}

//------------------------------------------------------------------ tape

// glyph
//   draws one character centred in _cell; blanks and controls as marks.
void glyph(
    ImDrawList* _draw,
    ImVec2      _min,
    ImVec2      _max,
    char        _c,
    ImU32       _ink,
    float       _size
)
{
    const ImVec2 mid((_min.x + _max.x) * 0.5f, (_min.y + _max.y) * 0.5f);
    const float  w = _max.x - _min.x;

    if (_c == ' ')
    {
        _draw->AddCircleFilled(mid, 1.5f, _ink);
    }
    else if (_c == '\n')
    {
        // a return arrow: down, then back to the left
        const float r = mid.x + (w * 0.25f);
        const float l = mid.x - (w * 0.25f);
        _draw->AddLine(ImVec2(r, mid.y - 4.0f), ImVec2(r, mid.y + 2.0f),
                       _ink);
        _draw->AddLine(ImVec2(r, mid.y + 2.0f), ImVec2(l, mid.y + 2.0f),
                       _ink);
        _draw->AddTriangleFilled(ImVec2(l - 3.0f, mid.y + 2.0f),
                                 ImVec2(l + 1.0f, mid.y - 1.0f),
                                 ImVec2(l + 1.0f, mid.y + 5.0f), _ink);
    }
    else if (_c == '\t')
    {
        _draw->AddLine(ImVec2(_min.x + 3.0f, mid.y),
                       ImVec2(_max.x - 3.0f, mid.y), _ink);
        _draw->AddTriangleFilled(ImVec2(_max.x - 2.0f, mid.y),
                                 ImVec2(_max.x - 6.0f, mid.y - 3.0f),
                                 ImVec2(_max.x - 6.0f, mid.y + 3.0f), _ink);
    }
    else if ( (static_cast<unsigned char>(_c) < 0x20) ||
              (static_cast<unsigned char>(_c) == 0x7F) )
    {
        _draw->AddRect(ImVec2(mid.x - 3.0f, mid.y - 3.0f),
                       ImVec2(mid.x + 3.0f, mid.y + 3.0f), _ink);
    }
    else
    {
        const char   text[2] = { _c, 0 };
        const ImVec2 size    = ImGui::GetFont()->CalcTextSizeA(
            _size, FLT_MAX, 0.0f, text);
        _draw->AddText(ImGui::GetFont(), _size,
                       ImVec2(mid.x - (size.x * 0.5f),
                              mid.y - (size.y * 0.5f)),
                       _ink, text);
    }

    return;
}

// scrubber
//   step controls over a trace: back, a slider, forward, and what the step
// is. Returns the step drawn (clamped), or -1 when there is no trace.
long scrubber(
    const run_record&       _run,
    const parse_pass&       _pass,
    const vp::peg::program& _program,
    const std::string&      _text,
    long                    _step,
    const std::string&      _action,
    bool                    _grammar,
    imgui_frame&            _frame
)
{
    const long count = static_cast<long>(_pass.trace.size());

    if (count == 0)
    {
        const char* why =
            !_run.request.trace
                ? "The trace was not recorded for this run, so there is no "
                  "cursor to move."
            : ( (_grammar) &&
                (_run.request.frontend == "ebnf") )
                ? "EBNF is read by a hand-written parser rather than the "
                  "machine, so its reading has no trace."
                : "The program did not run.";

        ImGui::PushStyleColor(ImGuiCol_Text, vec(PENCIL));
        ImGui::TextWrapped("%s", why);
        ImGui::PopStyleColor();

        return -1;
    }

    const long step = std::max(0L, std::min(_step, count - 1));
    const auto move = [&_frame, &_action, count](long _to)
    {
        option_set payload;
        payload.set("value", std::max(0L, std::min(_to, count - 1)));
        _frame.post(_action, payload);

        return;
    };

    if (ImGui::ArrowButton("##back", ImGuiDir_Left))
    {
        move(step - 1);
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(std::max(160.0f,
                                     ImGui::GetContentRegionAvail().x * 0.4f));

    int value = static_cast<int>(step);

    if (ImGui::SliderInt("##step", &value, 0, static_cast<int>(count - 1),
                         "step %d"))
    {
        move(value);
    }

    ImGui::SameLine();

    if (ImGui::ArrowButton("##forward", ImGuiDir_Right))
    {
        move(step + 1);
    }

    const std::size_t         index = static_cast<std::size_t>(step);
    const trace_step&         at    = _pass.trace[index];
    const std::pair<int, int> where = line_col(_text, at.off);
    const std::string         instruction =
        ( (at.ip >= 0) &&
          (static_cast<std::size_t>(at.ip) < _program.size()) )
            ? disassemble(_program[static_cast<std::size_t>(at.ip)])
            : "?";

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, vec(PENCIL));
    ImGui::Text("of %ld", count - 1);
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::Text("ip %d  %s  at %d:%d", at.ip, instruction.c_str(),
                where.first, where.second);

    return step;
}

// draw_tape
//   the text as cells, spans beneath, cursor and frontier marked.
void draw_tape(
    const run_record&  _run,
    bool               _grammar,
    long               _step,
    const std::string& _action,
    imgui_frame&       _frame
)
{
    static const char* const tags[] = { "", "NAME", "SETBODY", "REF", "STAR",
                                        "PLUS", "LITCH", "BAR" };

    const std::string&      text    = _grammar ? _run.request.grammar
                                               : _run.request.input;
    const parse_pass&       pass    = _grammar ? _run.reading
                                               : _run.recognition;
    const vp::peg::program& program = _grammar ? _run.reader : _run.program;

    ImGui::PushID(_grammar ? "grammar_tape" : "input_tape");

    const long step = scrubber(_run, pass, program, text, _step, _action,
                               _grammar, _frame);

    // the grammar shows what each piece was read as, coloured by kind; the
    // input shows the rules, coloured by depth
    std::vector<span> spans;
    std::vector<int>  hues;

    if (_grammar)
    {
        for (const vp::peg::capture& c : pass.captures)
        {
            spans.push_back(span{ ( (c.tag >= 1) && (c.tag <= 7) )
                                      ? tags[c.tag] : "#",
                                  c.start, c.end, 0 });
            hues.push_back(c.tag);
        }
    }
    else
    {
        spans = pass.matches;
        int deepest = -1;

        for (const span& s : spans)
        {
            deepest = std::max(deepest, s.depth);
        }

        for (const vp::peg::capture& c : pass.captures)
        {
            spans.push_back(span{ "#" + std::to_string(c.tag),
                                  c.start, c.end, deepest + 1 });
        }
    }

    int layers = 0;

    for (const span& s : spans)
    {
        layers = std::max(layers, s.depth + 1);
    }

    // geometry: a cell per character plus one for the end of the text. A
    // short text is drawn larger, so the tape fills its panel's width.
    std::size_t widest  = 0;
    std::size_t current = 0;

    for (const char c : text)
    {
        current = (c == '\n') ? 0 : (current + 1);
        widest  = std::max(widest, current);
    }

    ImDrawList*       draw    = ImGui::GetWindowDrawList();
    const float       base    = ImGui::GetFontSize();
    const float       base_w  = ImGui::CalcTextSize("M").x + 5.0f;
    const float       lines   = static_cast<float>(
        std::count(text.begin(), text.end(), '\n') + 1);
    const float       fit_w   = (ImGui::GetContentRegionAvail().x - 8.0f) /
                                (static_cast<float>(widest + 2) * base_w);
    const float       fit_h   = (ImGui::GetContentRegionAvail().y - 16.0f) /
                                (lines * (base + 18.0f) +
                                 (static_cast<float>(layers) * lines *
                                  (base * 0.9f + 5.0f)));
    const float       zoom    = std::max(1.0f,
                                         std::min(3.0f,
                                                  std::min(fit_w, fit_h)));
    const float       font    = base * zoom;
    const float       cell_w  = base_w * zoom;
    const float       cell_h  = font + (8.0f * zoom);
    const float       label   = base * 0.9f;
    const float       layer_h = label + 5.0f;
    const float       row_h   = cell_h + (static_cast<float>(layers) *
                                          layer_h) + 10.0f;
    const ImVec2      origin  = ImVec2(ImGui::GetCursorScreenPos().x,
                                       ImGui::GetCursorScreenPos().y + 6.0f);
    const int         per_row = std::max(8, static_cast<int>(
                                    (ImGui::GetContentRegionAvail().x - 4.0f) /
                                    cell_w));
    const std::size_t length  = text.size();

    std::vector<std::pair<int, int> > place(length + 1);
    int column = 0;
    int line   = 0;

    for (std::size_t i = 0; i <= length; ++i)
    {
        place[i] = { column, line };

        if ( (i < length) &&
             (text[i] == '\n') )
        {
            column = 0;
            ++line;
        }
        else if (++column >= per_row)
        {
            column = 0;
            ++line;
        }
    }

    const auto cell_min = [&](std::size_t _i)
    {
        return ImVec2(origin.x + (static_cast<float>(place[_i].first) *
                                  cell_w),
                      origin.y + (static_cast<float>(place[_i].second) *
                                  row_h));
    };

    const float total_h = static_cast<float>(line + 1) * row_h + 6.0f;

    ImGui::InvisibleButton("##tape",
                           ImVec2(std::max(1.0f,
                                           ImGui::GetContentRegionAvail().x),
                                  total_h));

    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = ImGui::IsItemClicked();

    const int far    = ( (pass.ran) && (!pass.matched) ) ? pass.far.offset
                                                         : -1;
    const int cursor = (step >= 0)
                           ? pass.trace[static_cast<std::size_t>(step)].off
                           : -1;
    int       under  = -1;

    for (std::size_t i = 0; i <= length; ++i)
    {
        const ImVec2 lo = cell_min(i);
        const ImVec2 hi(lo.x + cell_w, lo.y + cell_h);

        if (!ImGui::IsRectVisible(lo, hi))
        {
            continue;
        }

        const int  at     = static_cast<int>(i);
        const bool beyond = ( (far >= 0) && (at > far) );

        if ( (hovered) &&
             (ImGui::IsMouseHoveringRect(lo, hi)) )
        {
            under = at;
        }

        draw->AddRectFilled(lo, hi,
                            (at == cursor) ? col(HIGHLIGHTER)
                            : beyond       ? col(MARGIN)
                                           : col(0xFFFFFF));
        draw->AddRect(lo, hi, col(RULE, 140));

        if (i < length)
        {
            glyph(draw, lo, hi, text[i],
                  beyond ? col(PENCIL, 150) : col(INK), font);
        }
        else
        {
            // the end of the text: a closing bar
            draw->AddRectFilled(ImVec2(lo.x + 3.0f, lo.y + 4.0f),
                                ImVec2(lo.x + 5.0f, hi.y - 4.0f),
                                col(PENCIL));
        }

        // the frontier: a red frame and a caret beneath
        if (at == far)
        {
            draw->AddRect(lo, hi, col(RIBBON_RED), 0.0f, 0, 2.0f);
            draw->AddTriangleFilled(
                ImVec2(lo.x + (cell_w * 0.5f), hi.y + 1.0f),
                ImVec2(lo.x + (cell_w * 0.5f) - 5.0f, hi.y + 8.0f),
                ImVec2(lo.x + (cell_w * 0.5f) + 5.0f, hi.y + 8.0f),
                col(RIBBON_RED));
        }
    }

    // spans: one layer per depth, split where the text wraps
    const float label_size = label;

    for (std::size_t n = 0; n < spans.size(); ++n)
    {
        const span&       s    = spans[n];
        const std::size_t hue  = static_cast<std::size_t>(
            (n < hues.size()) ? hues[n] : s.depth) % 6;
        const unsigned    fill = SPAN_FILL[hue];
        const unsigned    edge = SPAN_EDGE[hue];
        const float    dy   = cell_h + 3.0f +
                              (static_cast<float>(s.depth) * layer_h);

        // an empty match is a tick where it happened
        if (s.end <= s.start)
        {
            const ImVec2 lo = cell_min(static_cast<std::size_t>(
                std::min<int>(s.start, static_cast<int>(length))));
            draw->AddLine(ImVec2(lo.x, lo.y + dy),
                          ImVec2(lo.x, lo.y + dy + layer_h - 3.0f),
                          col(edge), 2.0f);
            continue;
        }

        int first = s.start;

        while (first < s.end)
        {
            int last = first;

            // extend the segment while it stays on one row
            while ( ((last + 1) < s.end) &&
                    (place[static_cast<std::size_t>(last + 1)].second ==
                     place[static_cast<std::size_t>(first)].second) )
            {
                ++last;
            }

            const ImVec2 a  = cell_min(static_cast<std::size_t>(first));
            const ImVec2 b  = cell_min(static_cast<std::size_t>(last));
            const ImVec2 lo(a.x + 1.0f, a.y + dy);
            const ImVec2 hi(b.x + cell_w - 1.0f, a.y + dy + layer_h - 3.0f);

            if (ImGui::IsRectVisible(lo, hi))
            {
                draw->AddRectFilled(lo, hi, col(fill));
                draw->AddRect(lo, hi, col(edge));

                const ImVec4 clip(lo.x + 2.0f, lo.y, hi.x - 2.0f, hi.y);
                draw->AddText(ImGui::GetFont(), label_size,
                              ImVec2(lo.x + 4.0f, lo.y + 1.0f), col(INK),
                              s.rule.c_str(), nullptr, 0.0f, &clip);
            }

            first = last + 1;
        }
    }

    // hovering names the character and the innermost rule over it; a click
    // moves the cursor to the first step that reached it
    if (under >= 0)
    {
        const std::pair<int, int> where = line_col(text, under);
        std::string inner;

        for (const span& s : spans)
        {
            if ( (s.start <= under) &&
                 (under < s.end) )
            {
                inner = s.rule;
            }
        }

        ImGui::SetTooltip("character %d, line %d column %d%s%s", under,
                          where.first, where.second,
                          inner.empty() ? "" : "\ninside ", inner.c_str());

        if (clicked)
        {
            for (std::size_t k = 0; k < pass.trace.size(); ++k)
            {
                if (pass.trace[k].off >= under)
                {
                    option_set payload;
                    payload.set("value", static_cast<long>(k));
                    _frame.post(_action, payload);
                    break;
                }
            }
        }
    }

    // arrow keys step while the pointer rests on the tape
    if ( (hovered) &&
         (step >= 0) &&
         (!ImGui::GetIO().WantTextInput) )
    {
        long to = step;

        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))  { to = step - 1; }
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) { to = step + 1; }

        if (to != step)
        {
            option_set payload;
            payload.set("value", std::max(0L, std::min(
                to, static_cast<long>(pass.trace.size()) - 1)));
            _frame.post(_action, payload);
        }
    }

    ImGui::PopID();

    return;
}

imgui_blueprint render_tape(
    const uxoxo::element_node<imgui_blueprint>& _node
)
{
    const option_set attrs = _node.attrs;

    const auto draw = [attrs](imgui_frame& _frame)
    {
        const run_record* run = finder()(
            static_cast<int>(attrs.as_long("run", -1)));

        if (!run)
        {
            return;
        }

        draw_tape(*run,
                  (attrs.as_string("which", "input") == "grammar"),
                  attrs.as_long("step", 0),
                  attrs.as_string("action", ""),
                  _frame);

        return;
    };

    return keyed(attrs, "", draw);
}


//------------------------------------------------------------------ heat

// ui_rgba
//   a design token as a djinterp::ui colour.
ui::rgba ui_rgba(
    unsigned _hex,
    float    _alpha = 1.0f
)
{
    const ImVec4 v = vec(_hex, _alpha);

    return ui::rgba(v.x, v.y, v.z, v.w);
}

// heat_click
//   struct: a click on a cell of the map: the first step that reached the
// cell, or -1 when nothing did, and which cell it was.
struct heat_click
{
    long        step   = -1;
    long        column = -1;
    long        row    = -1;
    std::string rule;
};

// draw_heat
//   one run's heat map as of _step, filling the space left, on a colour
// scale topping out at _high. Returns the cell clicked, if one was.
heat_click draw_heat(
    const run_record& _run,
    long              _step,
    bool              _arrivals,
    double            _high
)
{
    const heat_space& space = *_run.heat;
    const std::size_t count = _run.recognition.trace.size();
    const std::size_t step  = std::min<std::size_t>(
        static_cast<std::size_t>(std::max(0L, _step)),
        (count > 0) ? (count - 1) : 0);

    ui::field_scale scale;
    scale.colors = heat_colors();
    scale.low    = 1.0;
    scale.high   = std::max(1.0, _high);

    ui::field_style style;
    style.backdrop    = ui_rgba(0xFFFFFF);
    style.text        = ui_rgba(INK);
    style.grid        = ui_rgba(RULE, 0.35f);
    style.highlight   = ui_rgba(PENCIL);
    style.show_range  = false;
    style.show_titles = false;
    style.tooltip     = false;

    const ui::field_hit hit = ui::draw_field(heat_scene(space, _arrivals,
                                                        step),
                                             scale,
                                             style,
                                             heat_overlay(space, _run, step));

    // nothing under the mouse, nothing to say
    if (!hit.hovered.has_value())
    {
        return heat_click();
    }

    const ui::field_cell           cell = hit.hovered.value();
    const std::vector<std::size_t> at   = { cell.col, cell.row };
    const std::size_t from = static_cast<std::size_t>(cell.col) * space.bucket;
    std::string       place;

    // one character per column: where it is; otherwise the range
    if (space.bucket == 1)
    {
        const std::pair<int, int> lc =
            line_col(_run.request.input, static_cast<int>(from));

        place = "'" + space.columns[cell.col] + "' at " +
                std::to_string(lc.first) + ":" + std::to_string(lc.second);
    }
    else
    {
        place = "characters " + space.columns[cell.col];
    }

    const std::size_t arrived = space.entries.total(at);

    ImGui::BeginTooltip();
    ImGui::Text("%s, %s", space.rows[cell.row].c_str(), place.c_str());
    ImGui::Text("arrivals  %zu by step %zu, %zu in the whole run",
                space.entries.count(at, step), step, arrived);
    ImGui::Text("steps     %zu by step %zu, %zu in the whole run",
                space.steps.count(at, step), step, space.steps.total(at));
    ImGui::PushStyleColor(ImGuiCol_Text, vec(PENCIL));
    ImGui::TextUnformatted((arrived > 0) ? "click to go to the first step here"
                                         : "the parser never came here");
    ImGui::PopStyleColor();
    ImGui::EndTooltip();

    heat_click click;

    if (hit.clicked)
    {
        const auto first = space.steps.first_visit(at);

        click.step   = first.has_value() ? static_cast<long>(first.value())
                                         : -1L;
        click.column = static_cast<long>(cell.col);
        click.row    = static_cast<long>(cell.row);
        click.rule   = space.rows[cell.row];
    }

    return click;
}

imgui_blueprint render_heat(
    const uxoxo::element_node<imgui_blueprint>& _node
)
{
    const option_set attrs = _node.attrs;

    const auto draw = [attrs](imgui_frame& _frame)
    {
        const run_record* run   = finder()(
            static_cast<int>(attrs.as_long("run", -1)));
        const run_record* other = finder()(
            static_cast<int>(attrs.as_long("other", -1)));
        const bool        arrivals = attrs.as_bool("arrivals", true);
        const long        step     = attrs.as_long("step", 0);
        const std::string action   = attrs.as_string("action", "");

        if (!run)
        {
            ImGui::TextDisabled("No run selected.");

            return;
        }

        // no trace, no map
        if (!run->heat)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, vec(PENCIL));
            ImGui::TextWrapped("%s",
                               part_listing(*run, "heat").note.c_str());
            ImGui::PopStyleColor();

            return;
        }

        // the step as shown: the model's, within this run's trace
        const long last  = static_cast<long>(
            run->recognition.trace.size()) - 1;
        const long shown = std::max(0L, std::min(step, last));

        const auto high = [arrivals](const run_record& _r)
        {
            return !_r.heat ? 1.0
                 : static_cast<double>(arrivals ? _r.heat->max_entries
                                                : _r.heat->max_steps);
        };

        // a click on a cell that something reached moves the step there
        const auto go = [&_frame, &action](const heat_click& _click)
        {
            if ( (_click.step >= 0) &&
                 (!action.empty()) )
            {
                option_set payload;
                payload.set("value", _click.step);
                payload.set("column", _click.column);
                payload.set("row", _click.row);
                payload.set("rule", _click.rule);
                _frame.post(action, payload);
            }

            return;
        };

        // one run: the map fills the space under a caption
        if ( (!other) ||
             (other == run) ||
             (!other->heat) )
        {
            ImGui::PushStyleColor(ImGuiCol_Text, vec(PENCIL));
            ImGui::TextWrapped(
                "%s at each input position (across) and rule (down), up "
                "to step %ld. The blue trail is the call stack; the dark "
                "box is where the machine is. Revisits: %s.",
                arrivals ? "Arrivals" : "Steps spent", shown,
                heat_summary(*run->heat).c_str());
            ImGui::PopStyleColor();
            go(draw_heat(*run, shown, arrivals, high(*run)));

            return;
        }

        // two runs side by side, on one scale
        const double shared = std::max(high(*run), high(*other));
        const float  half   = (ImGui::GetContentRegionAvail().x -
                               ImGui::GetStyle().ItemSpacing.x) * 0.5f;
        const long   end    = static_cast<long>(
            other->recognition.trace.size()) - 1;

        ImGui::BeginChild("##heat_left", ImVec2(half, 0.0f));
        ImGui::Text("Run %d, step %ld", run->id, shown);
        go(draw_heat(*run, shown, arrivals, shared));
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("##heat_right", ImVec2(0.0f, 0.0f));
        ImGui::Text("Run %d, whole run", other->id);
        (void)draw_heat(*other, end, arrivals, shared);
        ImGui::EndChild();

        return;
    };

    return keyed(attrs, "", draw);
}

}  // anonymous namespace


/*
install_renderers
  The finder is kept for the renderers; the two registrations are this
module's whole footprint on the platform.
*/
void install_renderers(
    run_finder _finder
)
{
    finder() = std::move(_finder);

    uxoxo::set_renderer<imgui_platform>(listing_type(), &render_listing);
    uxoxo::set_renderer<imgui_platform>(tape_type(), &render_tape);
    uxoxo::set_renderer<imgui_platform>(heat_type(), &render_heat);

    return;
}

/*
apply_theme
  Square corners and thin rules: the look of a printed listing, not of an app.
*/
void apply_theme()
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding    = 0.0f;
    style.ChildRounding     = 0.0f;
    style.FrameRounding     = 0.0f;
    style.PopupRounding     = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding      = 0.0f;
    style.TabRounding       = 0.0f;
    style.WindowBorderSize  = 0.0f;
    style.ChildBorderSize   = 1.0f;
    style.FrameBorderSize   = 1.0f;
    style.TabBorderSize     = 1.0f;
    style.WindowPadding     = ImVec2(12.0f, 10.0f);
    style.FramePadding      = ImVec2(7.0f, 4.0f);
    style.ItemSpacing       = ImVec2(8.0f, 6.0f);
    style.CellPadding       = ImVec2(6.0f, 2.0f);
    style.ScrollbarSize     = 12.0f;
    style.GrabMinSize       = 12.0f;

    ImVec4* c = style.Colors;

    c[ImGuiCol_Text]                 = vec(INK);
    c[ImGuiCol_TextDisabled]         = vec(PENCIL);
    c[ImGuiCol_WindowBg]             = vec(PAPER);
    c[ImGuiCol_ChildBg]              = vec(PAPER);
    c[ImGuiCol_PopupBg]              = vec(0xFFFFFF);
    c[ImGuiCol_Border]               = vec(RULE);
    c[ImGuiCol_BorderShadow]         = vec(PAPER, 0.0f);
    c[ImGuiCol_FrameBg]              = vec(0xFFFFFF);
    c[ImGuiCol_FrameBgHovered]       = vec(0xF1F6EF);
    c[ImGuiCol_FrameBgActive]        = vec(0xFFFFFF);
    c[ImGuiCol_TitleBg]              = vec(MARGIN);
    c[ImGuiCol_TitleBgActive]        = vec(MARGIN);
    c[ImGuiCol_ScrollbarBg]          = vec(MARGIN);
    c[ImGuiCol_ScrollbarGrab]        = vec(0xB9C8BC);
    c[ImGuiCol_ScrollbarGrabHovered] = vec(0xA2B4A6);
    c[ImGuiCol_ScrollbarGrabActive]  = vec(RULE);
    c[ImGuiCol_CheckMark]            = vec(INK);
    c[ImGuiCol_SliderGrab]           = vec(HIGHLIGHTER);   // the current step
    c[ImGuiCol_SliderGrabActive]     = vec(0xE0B930);
    c[ImGuiCol_Button]               = vec(0xE3ECE0);
    c[ImGuiCol_ButtonHovered]        = vec(0xD3E3CF);
    c[ImGuiCol_ButtonActive]         = vec(0xC3D8BE);
    c[ImGuiCol_Header]               = vec(HIGHLIGHTER, 0.85f);
    c[ImGuiCol_HeaderHovered]        = vec(0xE9EFDF);
    c[ImGuiCol_HeaderActive]         = vec(HIGHLIGHTER);
    c[ImGuiCol_Separator]            = vec(RULE);
    c[ImGuiCol_SeparatorHovered]     = vec(INK);
    c[ImGuiCol_SeparatorActive]      = vec(INK);
    c[ImGuiCol_ResizeGrip]           = vec(RULE, 0.4f);
    c[ImGuiCol_ResizeGripHovered]    = vec(INK, 0.6f);
    c[ImGuiCol_ResizeGripActive]     = vec(INK);
    c[ImGuiCol_Tab]                  = vec(0xE3ECE0);
    c[ImGuiCol_TabHovered]           = vec(0xF3E7B0);
    c[ImGuiCol_TabSelected]          = vec(PAPER);
    c[ImGuiCol_TabSelectedOverline]  = vec(INK);
    c[ImGuiCol_TabDimmed]            = vec(0xE3ECE0);
    c[ImGuiCol_TabDimmedSelected]    = vec(PAPER);
    c[ImGuiCol_TableHeaderBg]        = vec(MARGIN);
    c[ImGuiCol_TableBorderStrong]    = vec(RULE);
    c[ImGuiCol_TableBorderLight]     = vec(RULE, 0.5f);
    c[ImGuiCol_TableRowBg]           = vec(PAPER);
    c[ImGuiCol_TableRowBgAlt]        = vec(GREEN_BAR);
    c[ImGuiCol_TextSelectedBg]       = vec(HIGHLIGHTER, 0.6f);
    c[ImGuiCol_NavCursor]            = vec(INK);

    uxoxo::platform::imgui_tones& tones = imgui_platform::tones();

    const auto set = [](float (&_out)[4], unsigned _hex)
    {
        const ImVec4 v = vec(_hex);
        _out[0] = v.x;
        _out[1] = v.y;
        _out[2] = v.z;
        _out[3] = v.w;

        return;
    };

    set(tones.muted, PENCIL);
    set(tones.accent, INK);
    set(tones.error, RIBBON_RED);
    set(tones.ok, GREEN_INK);
    tones.heading = 1.2f;

    return;
}

}  // namespace workbench
}  // namespace vparse

#endif  // floor, for now
