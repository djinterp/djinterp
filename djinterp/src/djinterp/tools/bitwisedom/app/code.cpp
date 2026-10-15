/*******************************************************************************
* bitwisedom [app]                                                      code.cpp
*
* The code view, and the registration of the application's elements.
*   The code view shows the generated header in the monospace font, lightly
* coloured: comments muted, directives in the accent, numbers warm, and
* every macro name in the colour of its row -- so the header reads with the
* grid's colours. Only the lines in view are laid out, through ImGui's list
* clipper; whether each line starts inside a block comment is worked out
* once per header, not per frame.
*
* path:      /src/djinterp/tools/bitwisedom/app/code.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.11
*******************************************************************************/
#include "./elements.hpp"  // corresponding header

// std
#include <cctype>   // std::isalnum, std::isalpha, std::isdigit
#include <map>      // std::map
#include <string>   // std::string
#include <vector>   // std::vector
// imgui
#include <imgui.h>  // ImGui
// uxoxo
#include <uxoxo/realize.hpp>  // set_renderer
// bitwisedom
#include "./theme.hpp"  // to_u32, mono_font


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
//   an application element's type: its archetype no backend knows.
const element_type*
descriptor(
    const char* _name
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
                                            0,
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
//   struct: the header's lines and names, for the version they came from.
struct code_cache
{
    int                         version = -1;
    std::vector<line_info>      lines;
    std::map<std::string, rgba> names;
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

    if (state.version == _model.version)
    {
        return;
    }

    state.version = _model.version;
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

    for (const resolved_row& each : _model.res.rows)
    {
        if (each.kind == row_kind::symbol)
        {
            state.names[each.full] = each.colour;
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
                piece(word, to_u32(found->second));
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

/*
register_renderers
  Each renderer captures the model and draws from it when the frame runs;
the blueprint it returns is rebuilt with the rest of the interface, but its
drawing always reads the model as it is now.
*/
void
register_renderers(
    model& _model
)
{
    model* state = &_model;

    uxoxo::set_renderer<imgui_platform>(
        descriptor("bitwisedom.bit_grid"),
        [state](const element_node<imgui_blueprint>& _node)
        {
            return imgui_blueprint{
                [state](imgui_frame& _frame)
                {
                    draw_grid(*state, _frame);
                },
                std::string(),
                _node.attrs.as_string("key", "grid") };
        });

    uxoxo::set_renderer<imgui_platform>(
        descriptor("bitwisedom.flow_view"),
        [state](const element_node<imgui_blueprint>& _node)
        {
            const float height =
                static_cast<float>(_node.attrs.as_double("height", 120.0));

            return imgui_blueprint{
                [state, height](imgui_frame& _frame)
                {
                    draw_flow(*state, _frame, height);
                },
                std::string(),
                _node.attrs.as_string("key", "flow") };
        });

    uxoxo::set_renderer<imgui_platform>(
        descriptor("bitwisedom.code_view"),
        [state](const element_node<imgui_blueprint>& _node)
        {
            return imgui_blueprint{
                [state](imgui_frame& _frame)
                {
                    draw_code(*state, _frame);
                },
                std::string(),
                _node.attrs.as_string("key", "code") };
        });

    return;
}

}  // namespace gui
}  // namespace bitwisedom
