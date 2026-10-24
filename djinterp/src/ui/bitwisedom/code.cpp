/*******************************************************************************
* bitwisedom [ui]                                                       code.cpp
*
* The code view, and the registration of the application's elements.
*   The code view shows the generated header in the monospace font, lightly
* coloured: comments muted, directives in the accent, numbers warm, and
* every macro name in the colour of its row -- so the header reads with the
* grid's colours -- as far as that colour reads on the view: one too near
* the view's own is moved toward the text's, and a row with no fill has its
* names in the text's colour. Only the lines in view are laid out, through
* ImGui's list clipper; whether each line starts inside a block comment is
* worked out once per header, not per frame, and kept by the document's id.
*   The elements are registered with the uxoxo ImGui platform: the grid,
* the flow and code views, the add controls, and the dock space and its
* windows. Each renderer finds the document it draws by the "document" its
* element carries, as each frame is drawn -- several sheets are open at
* once -- and skips one closed since its interface was built.
*
* path:      /src/ui/bitwisedom/code.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.19
*******************************************************************************/
#include "../../../inc/ui/bitwisedom/elements.hpp"  // corresponding header

// std
#include <algorithm>  // std::max
#include <cctype>   // std::isalnum, std::isalpha, std::isdigit
#include <cfloat>   // FLT_MIN
#include <cstdlib>  // std::atoi
#include <map>      // std::map
#include <string>   // std::string
#include <utility>  // std::move
#include <vector>   // std::vector
// imgui
#include <imgui.h>  // ImGui
#include <imgui_internal.h>         // ImGuiWindow, ImFormatString, ImRect
#include <misc/cpp/imgui_stdlib.h>  // InputTextMultiline, std::string
// uxoxo
#include <uxoxo/realize.hpp>  // set_renderer
// bitwisedom
#include "../../../inc/ui/bitwisedom/theme.hpp"  // to_u32, mono_font, legible


namespace bitwisedom
{
namespace gui
{

namespace
{

using uxoxo::element_node;
using uxoxo::element_type;
using uxoxo::handler_fn;
using uxoxo::option_set;
using uxoxo::platform::imgui_blueprint;
using uxoxo::platform::imgui_frame;
using uxoxo::platform::imgui_platform;

// descriptor
//   an application element's type: its archetype no backend knows, and how
// many children it takes -- none, or any number (-1).
const element_type*
descriptor(
    const char* _name,
    int         _most = 0
)
{
    static std::map<std::string, element_type> table;

    auto found = table.find(_name);

    if (found == table.end())
    {
        found = table.emplace(_name,
                              element_type{ _name,
                                            _name,
                                            option_set{},
                                            0,
                                            _most,
                                            option_set{},
                                            handler_fn{} }).first;
    }

    return &found->second;
}

// line_info
//   struct: one line of the header, and whether it begins in a comment.
struct line_info
{
    std::size_t start   = 0;
    std::size_t length  = 0;
    bool        comment = false;
};

// code_cache
//   struct: the header's lines, and its names in the colours they are
// written in, for the document and the version they came from.
struct code_cache
{
    int                          document = -1;
    int                          version  = -1;
    std::vector<line_info>       lines;
    std::map<std::string, ImU32> names;
};

code_cache&
cache()
{
    static code_cache state;

    return state;
}

void
refresh(
    const model& _model
)
{
    code_cache& state = cache();

    if ( (state.document == _model.id) &&
         (state.version == _model.version) )
    {
        return;
    }

    state.document = _model.id;
    state.version  = _model.version;
    state.lines.clear();
    state.names.clear();

    const std::string& text    = _model.header.text;
    std::size_t        start   = 0;
    bool               comment = false;

    while (start < text.size())
    {
        std::size_t end = text.find('\n', start);

        if (end == std::string::npos)
        {
            end = text.size();
        }

        state.lines.push_back(line_info{ start, end - start, comment });

        // the comment state the next line begins in
        for (std::size_t i = start; i + 1 < end; ++i)
        {
            if ( (!comment) &&
                 (text[i] == '/') &&
                 (text[i + 1] == '/') )
            {
                break;
            }

            if ( (!comment) &&
                 (text[i] == '/') &&
                 (text[i + 1] == '*') )
            {
                comment = true;
                ++i;
            }
            else if ( (comment) &&
                      (text[i] == '*') &&
                      (text[i + 1] == '/') )
            {
                comment = false;
                ++i;
            }
        }

        start = end + 1;
    }

    // each name in its row's colour, as far as that reads on the view; a
    // row with no fill, in the text's. The view's ground is the frames',
    // and a theme or scheme changed is a version changed.
    const ImU32 ground = ImGui::GetColorU32(ImGuiCol_FrameBg);
    const ImU32 plain  = ImGui::GetColorU32(ImGuiCol_Text);

    for (const resolved_row& each : _model.res.rows)
    {
        if (each.kind == row_kind::symbol)
        {
            state.names[each.full] = legible(each.colour, ground, plain);
        }
    }

    return;
}

// piece
//   draws one run of a line in a colour, on the same line as the last.
void
piece(
    const std::string& _text,
    ImU32              _colour
)
{
    if (_text.empty())
    {
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Text, _colour);
    ImGui::TextUnformatted(_text.c_str(), _text.c_str() + _text.size());
    ImGui::PopStyleColor();
    ImGui::SameLine(0.0f, 0.0f);

    return;
}

// paint_line
//   one line, tokenized just enough to colour it.
void
paint_line(
    const model&       _model,
    const std::string& _line,
    bool               _comment
)
{
    const code_cache& state     = cache();
    const ImU32       plain     = ImGui::GetColorU32(ImGuiCol_Text);
    const ImU32       muted     = ImGui::GetColorU32(ImGuiCol_TextDisabled);
    const ImU32       directive = to_u32(_model.shown.colours.accent);
    const ImU32       number    = is_light(_model.shown.colours)
                                      ? IM_COL32(176, 96, 16, 255)
                                      : IM_COL32(242, 165, 65, 255);
    bool              inside    = _comment;
    std::size_t       at        = 0;
    std::string       run;

    // flush
    //   draws the plain text gathered so far.
    auto flush = [&run, plain]()
    {
        piece(run, plain);
        run.clear();
    };

    while (at < _line.size())
    {
        if (inside)
        {
            const std::size_t close = _line.find("*/", at);
            const std::size_t end   = (close == std::string::npos)
                                          ? _line.size()
                                          : close + 2;

            piece(_line.substr(at, end - at), muted);
            inside = (close == std::string::npos);
            at     = end;

            continue;
        }

        const char each = _line[at];

        if ( (each == '/') &&
             (at + 1 < _line.size()) &&
             (_line[at + 1] == '/') )
        {
            flush();
            piece(_line.substr(at), muted);

            break;
        }

        if ( (each == '/') &&
             (at + 1 < _line.size()) &&
             (_line[at + 1] == '*') )
        {
            flush();
            inside = true;

            continue;
        }

        if ( (each == '#') &&
             (run.find_first_not_of(' ') == std::string::npos) )
        {
            flush();

            std::size_t end = at + 1;

            while ( (end < _line.size()) &&
                    (std::isalpha(static_cast<unsigned char>(_line[end]))) )
            {
                ++end;
            }

            piece(_line.substr(at, end - at), directive);
            at = end;

            continue;
        }

        if ( (std::isalpha(static_cast<unsigned char>(each))) ||
             (each == '_') )
        {
            std::size_t end = at;

            while ( (end < _line.size()) &&
                    ( (std::isalnum(static_cast<unsigned char>(_line[end]))) ||
                      (_line[end] == '_') ) )
            {
                ++end;
            }

            const std::string word  = _line.substr(at, end - at);
            const auto        found = state.names.find(word);

            if (found != state.names.end())
            {
                flush();
                piece(word, found->second);
            }
            else
            {
                run += word;
            }

            at = end;

            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(each)))
        {
            std::size_t end = at;

            while ( (end < _line.size()) &&
                    (std::isalnum(static_cast<unsigned char>(_line[end]))) )
            {
                ++end;
            }

            flush();
            piece(_line.substr(at, end - at), number);
            at = end;

            continue;
        }

        run += each;
        ++at;
    }

    flush();
    ImGui::NewLine();

    return;
}

}  // anonymous namespace


void
draw_code(
    model&       _model,
    imgui_frame& /*_frame*/
)
{
    refresh(_model);

    const code_cache& state = cache();

    ImGui::PushStyleColor(ImGuiCol_ChildBg,
                          ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));

    if (ImGui::BeginChild("##code",
                          ImVec2(0.0f, 0.0f),
                          ImGuiChildFlags_Borders,
                          ImGuiWindowFlags_HorizontalScrollbar))
    {
        ImGui::PushFont(mono_font(), 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 1.0f));

        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(state.lines.size()));

        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
            {
                const line_info& each =
                    state.lines[static_cast<std::size_t>(i)];

                paint_line(_model,
                           _model.header.text.substr(each.start, each.length),
                           each.comment);
            }
        }

        ImGui::PopStyleVar();
        ImGui::PopFont();
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    return;
}

namespace
{

// line_note
//   struct: what the preprocessor said of one line of a definition file:
// that it could not read it -- the line does nothing -- or only assumed
// something of it.
struct line_note
{
    bool        broken = true;
    std::string text;
};

// line_notes
//   a definition file's notes, by the line each names -- "line N: ..." --
// counting from 1; a note that names no line is left out.
std::map<int, std::vector<line_note>>
line_notes(
    const model&      _model,
    std::size_t       _index
)
{
    std::map<int, std::vector<line_note>> out;

    if (_index >= _model.res.definition_notes.size())
    {
        return out;
    }

    for (const std::string& each : _model.res.definition_notes[_index])
    {
        if (each.compare(0, 5, "line ") != 0)
        {
            continue;
        }

        const int number = std::atoi(each.c_str() + 5);

        if (number <= 0)
        {
            continue;
        }

        const std::size_t colon = each.find(':');
        line_note         made;

        made.text   = (colon == std::string::npos)
                          ? each
                          : each.substr(colon + 1 +
                                        ( (colon + 1 < each.size()) &&
                                          (each[colon + 1] == ' ') ? 1 : 0));
        made.broken = (each.find("read as taken") == std::string::npos);
        out[number].push_back(std::move(made));
    }

    return out;
}

// squiggle
//   a wavy line under text, from _from to _to along _y: red where the line
// does nothing, amber where something was only assumed of it.
void
squiggle(
    ImDrawList* _list,
    float       _from,
    float       _to,
    float       _y,
    bool        _broken
)
{
    const ImU32 colour = _broken ? IM_COL32(240, 80, 72, 255)
                                 : IM_COL32(232, 170, 60, 220);
    const float step   = 3.0f * std::max(1.0f, ui_scale());
    const float rise   = 1.5f * std::max(1.0f, ui_scale());
    ImVector<ImVec2> points;

    for (float x = _from; x <= _to; x += step)
    {
        points.push_back(ImVec2(x, _y + ( ((points.Size % 2) == 0) ? rise
                                                                   : -rise)));
    }

    if (points.Size >= 2)
    {
        _list->AddPolyline(points.Data, points.Size, colour,
                           ImDrawFlags_None, 1.0f);
    }

    return;
}

// noted_tip
//   the notes of a line the mouse is on, as its tooltip.
void
noted_tip(
    const std::vector<line_note>& _notes
)
{
    std::string text;

    for (const line_note& each : _notes)
    {
        text += (text.empty() ? "" : "\n") +
                std::string(each.broken ? "not read: " : "assumed: ") +
                each.text;
    }

    ImGui::SetTooltip("%s", text.c_str());

    return;
}

}  // anonymous namespace

/*
draw_definitions
  An imported definition file: its directives in the text's own ink, and
the rest -- code, comments -- behind a small expander, faded when shown.
What the preprocessor could not read is underlined in red, and does
nothing; what it only assumed, in amber; the mouse on one says what. Edit
turns the view into an editor of the whole text, each change a step to
undo, the underlines kept over it; Done turns it back.
*/
void
draw_definitions(model& _model, imgui_frame& _frame, std::size_t _index)
{
    if (_index >= _model.doc.definitions.size()) return;
    const definition_source& source  = _model.doc.definitions[_index];
    const auto               notes   = line_notes(_model, _index);
    const bool               editing =
        (_model.ui.definition_editing == static_cast<int>(_index));
    int                      broken  = 0;

    for (const auto& each : notes)
    {
        for (const line_note& one : each.second)
        {
            broken += one.broken ? 1 : 0;
        }
    }

    ImGui::PushID(static_cast<int>(_index));
    ImGui::TextDisabled("Imported: %s", source.path.c_str());
    ImGui::SameLine();

    if (ImGui::SmallButton(editing ? "Done##edit" : "Edit##edit"))
    {
        option_set request;
        request.set("value", static_cast<long>(_index));
        _frame.post("definitions.edit", std::move(request));
    }

    ImGui::SameLine();

    if (broken > 0)
    {
        ImGui::TextColored(ImVec4(0.94f, 0.36f, 0.33f, 1.0f),
                           "%d line(s) the preprocessor cannot read, "
                           "underlined: they do nothing",
                           broken);
    }
    else
    {
        ImGui::TextDisabled("every directive read");
    }

    if (!editing)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("(non-preprocessor/comment code %s)",
                            source.show_other ? "visible" : "hidden");
    }

    ImGui::Separator();

    // the editor: the whole text, the underlines drawn over it where its
    // lines lie in the field's own scrolling
    if (editing)
    {
        std::string text = source.text;

        ImGui::PushFont(mono_font(), 0.0f);

        const bool changed = ImGui::InputTextMultiline(
            "##definition.text",
            &text,
            ImVec2(-FLT_MIN, -FLT_MIN),
            ImGuiInputTextFlags_AllowTabInput);
        const ImRect frame(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        const ImGuiID field = ImGui::GetItemID();

        if (changed)
        {
            option_set request;
            request.set("value", static_cast<long>(_index));
            request.set("text", text);
            _frame.post("definitions.text", std::move(request));
        }

        // the field's own window, scrolled as it is
        ImGuiWindow* parent = ImGui::GetCurrentWindow();
        char         named[512];

        ImFormatString(named, IM_ARRAYSIZE(named), "%s/%s_%08X",
                       parent->Name, "##definition.text", field);

        const ImGuiWindow* inside = ImGui::FindWindowByName(named);
        const ImVec2       scroll = (inside != nullptr) ? inside->Scroll
                                                        : ImVec2(0.0f, 0.0f);
        const ImGuiStyle&  style  = ImGui::GetStyle();
        const float        line   = ImGui::GetFontSize();
        ImDrawList*        list   = ImGui::GetWindowDrawList();
        const ImVec2       mouse  = ImGui::GetMousePos();
        std::size_t        at     = 0;
        int                number = 1;

        list->PushClipRect(frame.Min, frame.Max, true);

        while (at <= text.size())
        {
            std::size_t end = text.find('\n', at);

            if (end == std::string::npos)
            {
                end = text.size();
            }

            const auto found = notes.find(number);

            if (found != notes.end())
            {
                const std::string row   = text.substr(at, end - at);
                const std::size_t first = row.find_first_not_of(" \t\r");
                const float       left  = frame.Min.x + style.FramePadding.x -
                                          scroll.x;
                const float       top   = frame.Min.y + style.FramePadding.y +
                                          static_cast<float>(number - 1) *
                                              line -
                                          scroll.y;
                const float       from  = left +
                    ( (first == std::string::npos)
                          ? 0.0f
                          : ImGui::CalcTextSize(row.c_str(),
                                                row.c_str() + first).x );
                const float       to    = left +
                    std::max(ImGui::CalcTextSize(row.c_str()).x,
                             (from - left) + line);
                bool              red   = false;

                for (const line_note& one : found->second)
                {
                    red = ( (red) || (one.broken) );
                }

                squiggle(list, from, to,
                         top + line - 2.0f * std::max(1.0f, ui_scale()),
                         red);

                if ( (mouse.y >= top) &&
                     (mouse.y < top + line) &&
                     (frame.Contains(mouse)) &&
                     (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) )
                {
                    noted_tip(found->second);
                }
            }

            if (end == text.size())
            {
                break;
            }

            at = end + 1;
            ++number;
        }

        list->PopClipRect();
        ImGui::PopFont();
        ImGui::PopID();

        return;
    }

    if (ImGui::BeginChild("##imported_code", ImVec2(0, 0),
                          ImGuiChildFlags_None,
                          ImGuiWindowFlags_HorizontalScrollbar))
    {
        ImGui::PushFont(mono_font(), 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 1.0f));
        const std::string& text = source.text;
        std::size_t at = 0;
        std::size_t hidden = 0;
        int number = 0;
        bool continuation = false;
        bool block_comment = false;
        bool shown_expander = false;
        while (at < text.size())
        {
            std::size_t end = text.find('\n', at);
            if (end == std::string::npos) end = text.size();
            const std::string line = text.substr(at, end - at);
            at = (end == text.size()) ? end : end + 1;
            ++number;
            const std::size_t first = line.find_first_not_of(" \t\r");
            // A '#' inside a block comment is not a directive.
            const bool directive = !block_comment &&
                ((first != std::string::npos && line[first] == '#') ||
                 continuation);
            continuation = directive && line.find_last_not_of(" \t\r") !=
                           std::string::npos &&
                           line[line.find_last_not_of(" \t\r")] == '\\';
            const bool comment = block_comment ||
                (first != std::string::npos &&
                  (line.compare(first, 2, "//") == 0 ||
                   line.compare(first, 2, "/*") == 0 ||
                   line.compare(first, 1, "*") == 0));
            if (line.find("/*") != std::string::npos) block_comment = true;
            if (line.find("*/") != std::string::npos) block_comment = false;
            const auto noted = notes.find(number);

            if (!directive && !source.show_other && noted == notes.end())
            {
                ++hidden;
                if (!shown_expander)
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                                        ImVec2(1.0f, 0.0f));
                    ImGui::PushFont(nullptr,
                                    font_base() * 0.70f);
                    if (ImGui::SmallButton("+##other"))
                    {
                        option_set request;
                        request.set("value", static_cast<long>(_index));
                        _frame.post("definitions.toggle", std::move(request));
                    }
                    ImGui::PopFont();
                    ImGui::PopStyleVar();
                    ImGui::SameLine();
                    ImGui::TextDisabled("other code/comments (collapsed)");
                    shown_expander = true;
                }
                continue;
            }
            if (!directive && source.show_other && !shown_expander)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                                    ImVec2(1.0f, 0.0f));
                ImGui::PushFont(nullptr,
                                font_base() * 0.70f);
                if (ImGui::SmallButton("-##other"))
                {
                    option_set request;
                    request.set("value", static_cast<long>(_index));
                    _frame.post("definitions.toggle", std::move(request));
                }
                ImGui::PopFont();
                ImGui::PopStyleVar();
                ImGui::SameLine();
                ImGui::TextDisabled("other code/comments (expanded)");
                shown_expander = true;
            }
            const ImVec4 tone = (!directive || comment)
                ? ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled)
                : ImGui::GetStyleColorVec4(ImGuiCol_Text);
            ImGui::PushStyleColor(ImGuiCol_Text, tone);
            ImGui::TextUnformatted(line.c_str());
            ImGui::PopStyleColor();

            // what the preprocessor could not read, or assumed: underlined
            if (noted != notes.end())
            {
                const ImVec2 low  = ImGui::GetItemRectMin();
                const ImVec2 high = ImGui::GetItemRectMax();
                const float  from = low.x +
                    ( (first == std::string::npos)
                          ? 0.0f
                          : ImGui::CalcTextSize(line.c_str(),
                                                line.c_str() + first).x );
                bool         red  = false;

                for (const line_note& one : noted->second)
                {
                    red = ( (red) || (one.broken) );
                }

                squiggle(ImGui::GetWindowDrawList(), from,
                         std::max(high.x, from + ImGui::GetFontSize()),
                         high.y - 1.0f, red);

                if (ImGui::IsItemHovered())
                {
                    noted_tip(noted->second);
                }
            }
        }
        (void)hidden;
        ImGui::PopStyleVar();
        ImGui::PopFont();
    }
    ImGui::EndChild();
    ImGui::PopID();
}

uxoxo::element_template
definition_view(
    option_set _attrs
)
{
    return uxoxo::make_element(descriptor("bitwisedom.definition_view"),
                               _attrs,
                               {});
}

uxoxo::element_template
bit_grid(
    option_set _attrs
)
{
    return uxoxo::make_element(descriptor("bitwisedom.bit_grid"), _attrs, {});
}

uxoxo::element_template
flow_view(
    option_set _attrs
)
{
    return uxoxo::make_element(descriptor("bitwisedom.flow_view"), _attrs, {});
}

uxoxo::element_template
code_view(
    option_set _attrs
)
{
    return uxoxo::make_element(descriptor("bitwisedom.code_view"), _attrs, {});
}

uxoxo::element_template
add_new_button(
    option_set _attrs
)
{
    return uxoxo::make_element(descriptor("bitwisedom.add_new"), _attrs, {});
}

uxoxo::element_template
add_child_field(
    long       _parent,
    option_set _attrs
)
{
    _attrs.set("value", _parent);

    return uxoxo::make_element(descriptor("bitwisedom.add_child"),
                               _attrs,
                               {});
}

uxoxo::element_template
action_list(
    option_set _attrs
)
{
    return uxoxo::make_element(descriptor("bitwisedom.action_list"),
                               _attrs,
                               {});
}

uxoxo::element_template
status_bar(
    option_set _attrs
)
{
    return uxoxo::make_element(descriptor("bitwisedom.status_bar"),
                               _attrs,
                               {});
}

uxoxo::element_template
add_control(
    option_set _attrs
)
{
    return uxoxo::make_element(descriptor("bitwisedom.add_control"),
                               _attrs,
                               {});
}

uxoxo::element_template
dock_space(
    option_set _attrs
)
{
    return uxoxo::make_element(descriptor("bitwisedom.dock_space"),
                               _attrs,
                               {});
}

uxoxo::element_template
dock_window(
    const std::string& _key,
    const std::string& _title,
    element_list       _items,
    option_set         _attrs
)
{
    _attrs.set("key", _key);
    _attrs.set("title", _title);

    return uxoxo::make_element(descriptor("bitwisedom.dock_window", -1),
                               _attrs,
                               std::move(_items));
}

uxoxo::element_template
menu_bar_rest(
    element_list _leading,
    element_list _trailing,
    option_set   _attrs
)
{
    _attrs.set("trailing", static_cast<long>(_trailing.size()));

    for (uxoxo::element_template& each : _trailing)
    {
        _leading.push_back(std::move(each));
    }

    return uxoxo::make_element(descriptor("bitwisedom.menu_bar_rest", -1),
                               _attrs,
                               std::move(_leading));
}

/*
register_renderers
  Each renderer finds the document it draws when the frame runs, by the id
its element carries -- the one in front, for none -- so the blueprint it
returns, rebuilt with the rest of the interface, always draws a document
as it is now, and nothing for one closed since.
*/
void
register_renderers(
    document_source _documents,
    actions_source  _actions
)
{
    // which
    //   the document an element's attributes name.
    auto which = [](const option_set& _attrs)
    {
        return static_cast<int>(_attrs.as_long("document", 0));
    };

    uxoxo::set_renderer<imgui_platform>(
        descriptor("bitwisedom.bit_grid"),
        [_documents, which](const element_node<imgui_blueprint>& _node)
        {
            const int         id     = which(_node.attrs);
            const std::string window = _node.attrs.as_string(
                                           "preview_window", "");
            const bool        front  = _node.attrs.as_bool("front", true);

            return imgui_blueprint{
                [_documents, id, window, front](imgui_frame& _frame)
                {
                    model* state = _documents(id);

                    if (state == nullptr)
                    {
                        return;
                    }

                    // the operation's preview shows while its window is
                    // in view; a row is dropped only on the sheet in front
                    state->ui.preview_shown = ( (!window.empty()) &&
                                                (window_seen(window)) );
                    state->ui.in_front      = front;
                    draw_grid(*state, _frame);
                },
                std::string(),
                _node.attrs.as_string("key", "grid") };
        });

    uxoxo::set_renderer<imgui_platform>(
        descriptor("bitwisedom.flow_view"),
        [_documents, which](const element_node<imgui_blueprint>& _node)
        {
            const int   id     = which(_node.attrs);
            const float height =
                static_cast<float>(_node.attrs.as_double("height", 120.0));

            return imgui_blueprint{
                [_documents, id, height](imgui_frame& _frame)
                {
                    if (model* state = _documents(id))
                    {
                        draw_flow(*state, _frame, height);
                    }
                },
                std::string(),
                _node.attrs.as_string("key", "flow") };
        });

    uxoxo::set_renderer<imgui_platform>(
        descriptor("bitwisedom.code_view"),
        [_documents, which](const element_node<imgui_blueprint>& _node)
        {
            const int id = which(_node.attrs);

            return imgui_blueprint{
                [_documents, id](imgui_frame& _frame)
                {
                    if (model* state = _documents(id))
                    {
                        draw_code(*state, _frame);
                    }
                },
                std::string(),
                _node.attrs.as_string("key", "code") };
        });

    uxoxo::set_renderer<imgui_platform>(
        descriptor("bitwisedom.definition_view"),
        [_documents, which](const element_node<imgui_blueprint>& _node)
        {
            const int         id    = which(_node.attrs);
            const std::size_t index = static_cast<std::size_t>(
                _node.attrs.as_long("value", 0));

            return imgui_blueprint{
                [_documents, id, index](imgui_frame& _frame)
                {
                    if (model* state = _documents(id))
                    {
                        draw_definitions(*state, _frame, index);
                    }
                },
                std::string(),
                _node.attrs.as_string("key", "definition") };
        });

    // the docking draws from its attributes, and a window from its
    // children too
    uxoxo::set_renderer<imgui_platform>(
        descriptor("bitwisedom.dock_space"),
        [](const element_node<imgui_blueprint>& _node)
        {
            const option_set attrs = _node.attrs;

            return imgui_blueprint{
                [attrs](imgui_frame& _frame)
                {
                    draw_dock_space(attrs, _frame);
                },
                std::string(),
                attrs.as_string("key", "") };
        });

    uxoxo::set_renderer<imgui_platform>(
        descriptor("bitwisedom.dock_window", -1),
        [](const element_node<imgui_blueprint>& _node)
        {
            const option_set     attrs    = _node.attrs;
            const blueprint_list children = _node.children;

            return imgui_blueprint{
                [attrs, children](imgui_frame& _frame)
                {
                    draw_dock_window(attrs, children, _frame);
                },
                attrs.as_string("title", ""),
                attrs.as_string("key", "") };
        });

    // the controls draw from their attributes alone, and the menu bar's
    // rest from its children too
    uxoxo::set_renderer<imgui_platform>(
        descriptor("bitwisedom.add_new"),
        [](const element_node<imgui_blueprint>& _node)
        {
            const option_set attrs = _node.attrs;

            return imgui_blueprint{
                [attrs](imgui_frame& _frame)
                {
                    draw_add_new(attrs, _frame);
                },
                std::string(ADD_NEW_LABEL),
                attrs.as_string("key", "") };
        });

    uxoxo::set_renderer<imgui_platform>(
        descriptor("bitwisedom.add_control"),
        [](const element_node<imgui_blueprint>& _node)
        {
            const option_set attrs = _node.attrs;

            return imgui_blueprint{
                [attrs](imgui_frame& _frame)
                {
                    draw_add_control(attrs, _frame);
                },
                std::string("Add"),
                attrs.as_string("key", "") };
        });

    uxoxo::set_renderer<imgui_platform>(
        descriptor("bitwisedom.action_list"),
        [_actions](const element_node<imgui_blueprint>& _node)
        {
            return imgui_blueprint{
                [_actions](imgui_frame&)
                {
                    const std::vector<action_entry>* actions =
                        _actions ? _actions() : nullptr;

                    if (actions != nullptr)
                    {
                        draw_action_list(*actions);
                    }
                },
                std::string(),
                _node.attrs.as_string("key", "") };
        });

    uxoxo::set_renderer<imgui_platform>(
        descriptor("bitwisedom.status_bar"),
        [](const element_node<imgui_blueprint>& _node)
        {
            const option_set attrs = _node.attrs;

            return imgui_blueprint{
                [attrs](imgui_frame& _frame)
                {
                    draw_status_bar(attrs, _frame);
                },
                std::string(),
                attrs.as_string("key", "") };
        });

    uxoxo::set_renderer<imgui_platform>(
        descriptor("bitwisedom.add_child"),
        [](const element_node<imgui_blueprint>& _node)
        {
            const option_set attrs = _node.attrs;

            return imgui_blueprint{
                [attrs](imgui_frame& _frame)
                {
                    draw_add_child(attrs, _frame);
                },
                std::string(),
                attrs.as_string("key", "") };
        });

    uxoxo::set_renderer<imgui_platform>(
        descriptor("bitwisedom.menu_bar_rest", -1),
        [](const element_node<imgui_blueprint>& _node)
        {
            const option_set     attrs    = _node.attrs;
            const blueprint_list children = _node.children;

            return imgui_blueprint{
                [attrs, children](imgui_frame& _frame)
                {
                    draw_menu_bar_rest(attrs, children, _frame);
                },
                std::string(),
                attrs.as_string("key", "") };
        });

    return;
}

}  // namespace gui
}  // namespace bitwisedom
