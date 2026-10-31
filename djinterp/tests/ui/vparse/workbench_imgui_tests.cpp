/*******************************************************************************
* vparse [tests]                                       workbench_imgui_tests.cpp
*
* The workbench's face on Dear ImGui, with no window: drawn in frames on Dear
* ImGui's null backend, pointed at, and clicked, with the events the clicks
* post carried through update() to the next face.
*   Dear ImGui recovers from a misuse -- an unbalanced Begin and End, a stack
* left pushed -- and in a release build says nothing of it, so each frame's
* count of recovered errors is read, and must stay zero; so must the
* identifier of a conflict under the pointer. The suite's context turns the
* asserts on those errors off, so a debug build counts them too rather than
* stopping at the first.
*   Nothing is clicked at a coordinate written down here: the pointer is
* walked over the face until what is looked for posts, so the suite follows
* the layout as it changes.
*   Built where Dear ImGui has its null backend
* (backends/imgui_impl_null.cpp); tests/CMakeLists.txt says which.
*   LEVELS. uxoxo's floor is C++17, so this file's is: below it the file is
* empty rather than an error.
*
*
* path:      /tests/ui/vparse/workbench_imgui_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.06
*                                                            revised: 2026.10.26
*******************************************************************************/
#include "../../../inc/ui/vparse/view.hpp"  // the unit under test
#if D_ENV_LANG_IS_CPP17_OR_HIGHER  // the floor its header has

// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf
#include <string>   // std::string
#include <vector>   // std::vector
// imgui
#include "backends/imgui_impl_null.h"  // the null platform and renderer
#include "imgui.h"                     // frames, windows, the input queue
#include "imgui_internal.h"            // GImGui: the error and conflict counts
// uxoxo
#include "uxoxo/element.hpp"                  // event, make_element
#include "uxoxo/platform/imgui_platform.hpp"  // imgui_platform, imgui_frame
#include "uxoxo/realize.hpp"                  // realize
// vparse
#include "../../../inc/ui/vparse/elements.hpp"  // install_renderers, heat_type
#include "../../../inc/djinterp/tools/vparse/heat.hpp"  // heat_space
#include "../../../inc/djinterp/tools/vparse/run.hpp"   // part_names,
                                                           // part_title


namespace
{

namespace wb = ::vparse::workbench;

using ::uxoxo::event;
using ::uxoxo::option_set;
using ::uxoxo::platform::imgui_frame;
using ::uxoxo::platform::imgui_platform;

// NESTED
//   constant: a grammar that re-parses, so its heat map has something on it.
const char* const NESTED =
    "Expr = Sum ;\n"
    "Sum  = Prod '+' Sum | Prod '-' Sum | Prod ;\n"
    "Prod = Atom '*' Prod | Atom '/' Prod | Atom ;\n"
    "Atom = '(' Sum ')' | Num ;\n"
    "Num  = [0-9]+ ;\n";

// WARM_UP
//   constant: frames drawn before anything is asked of a face, so that every
// window and table in it has been laid out once.
const int WARM_UP = 3;

// AWAY
//   constant: a pointer position off the display, where nothing is hovered.
const float AWAY = -10000.0f;

// HOLD
//   constant: the frames the pointer rests at each point of a sweep. Dear
// ImGui finds an identifier claimed twice by counting, in one frame, the
// items that claim the identifier hovered in the frame before, and says so
// at the start of the next: so a conflict shows on the third frame at rest.
const int HOLD = 3;

// section
//   struct: one section of the suite -- its name, and the function that runs
// it.
struct section
{
    const char* name;
    bool      (*run)();
};

// g_model
//   the model the renderers read runs from: the suite's finder looks here.
const wb::model* g_model = nullptr;

// headless
//   class: one Dear ImGui context on the null backend, with the workbench's
// renderers and theme, for as long as a section needs it. It writes no
// settings file, and keeps the counts the suite judges frames by.
class headless
{
public:
    headless();
    ~headless();

    headless(
        const headless&
    ) = delete;
    headless& operator=(
        const headless&
    ) = delete;

    // frame
    //   modifier: one frame of _face with the pointer at (_x, _y) and the
    // left button down or up; returns the events the frame posted.
    std::vector<event> frame(
        const ::uxoxo::element_template& _face,
        float                            _x,
        float                            _y,
        bool                             _down
    );

    // show
    //   modifier: one frame of _model's face with the pointer away.
    std::vector<event> show(
        const wb::model& _model
    );

    // click
    //   modifier: presses and releases at (_x, _y), over two frames, and
    // returns what they posted.
    std::vector<event> click(
        const ::uxoxo::element_template& _face,
        float                            _x,
        float                            _y
    );

    int errors() const;
    int conflicts() const;
    int vertices() const;

private:
    int m_errors    = 0;
    int m_conflicts = 0;
    int m_vertices  = 0;
};

/*
headless::headless
  The settings and log files are switched off: a test that left an imgui.ini
behind it would have changed the directory it ran in.
*/
headless::headless()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();

    io.IniFilename                     = nullptr;
    io.LogFilename                     = nullptr;
    io.ConfigErrorRecoveryEnableAssert = false;

    ImGui_ImplNullPlatform_Init();
    ImGui_ImplNullRender_Init();

    wb::apply_theme();
    wb::install_renderers([](int _id)
                          {
                              return (g_model != nullptr)
                                         ? wb::find_run(*g_model, _id)
                                         : nullptr;
                          });
}

/*
headless::~headless
  Taken down in the reverse of the order it was put up.
*/
headless::~headless()
{
    ImGui_ImplNullRender_Shutdown();
    ImGui_ImplNullPlatform_Shutdown();
    ImGui::DestroyContext();
}

/*
headless::frame
  A frame as the program runs one: the backends' new frames, Dear ImGui's,
one window filling the display, the face drawn into it, and the frame
rendered, which is how the null renderer takes delivery of the font atlas.
The counts are read after Render(), which ends the frame and so has made
whatever recovery the frame needed.
*/
std::vector<event> headless::frame(
    const ::uxoxo::element_template& _face,
    float                            _x,
    float                            _y,
    bool                             _down
)
{
    ImGuiIO& io = ImGui::GetIO();

    io.AddMousePosEvent(_x, _y);
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, _down);

    ImGui_ImplNullPlatform_NewFrame();
    ImGui_ImplNullRender_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(io.DisplaySize);

    imgui_frame posted;

    if (ImGui::Begin("workbench",
                     nullptr,
                     (ImGuiWindowFlags_NoDecoration    |
                      ImGuiWindowFlags_NoMove          |
                      ImGuiWindowFlags_NoSavedSettings)))
    {
        const ::uxoxo::platform::imgui_blueprint face =
            ::uxoxo::realize<imgui_platform>(_face);

        if (face.draw)
        {
            face.draw(posted);
        }
    }

    ImGui::End();
    ImGui::Render();
    ImGui_ImplNullRender_RenderDrawData(ImGui::GetDrawData());

    m_errors    += GImGui->ErrorCountCurrentFrame;
    m_conflicts += (GImGui->DebugDrawIdConflictsId != 0u) ? 1 : 0;
    m_vertices   = ImGui::GetDrawData()->TotalVtxCount;

    return posted.events;
}

/*
headless::show
  The model's face, untouched.
*/
std::vector<event> headless::show(
    const wb::model& _model
)
{
    g_model = &_model;

    return frame(wb::view(_model), AWAY, AWAY, false);
}

/*
headless::click
  A press is a frame with the button down and one with it up, at the same
place: Dear ImGui reports a click on the release.
*/
std::vector<event> headless::click(
    const ::uxoxo::element_template& _face,
    float                            _x,
    float                            _y
)
{
    std::vector<event> posted = frame(_face, _x, _y, true);
    std::vector<event> later  = frame(_face, _x, _y, false);

    posted.insert(posted.end(), later.begin(), later.end());

    // and away, so the next press starts from nothing held
    (void)frame(_face, AWAY, AWAY, false);

    return posted;
}

int headless::errors() const
{
    return m_errors;
}

int headless::conflicts() const
{
    return m_conflicts;
}

int headless::vertices() const
{
    return m_vertices;
}

// carrying
//   function: an event of a kind, whose payload's "value" is the one given.
template<typename Value>
event carrying(
    const std::string& _kind,
    const Value&       _value
)
{
    option_set payload;

    payload.set("value", _value);

    return event{ _kind, payload };
}

// bare
//   function: an event of a kind, with no payload.
event bare(
    const std::string& _kind
)
{
    return event{ _kind, option_set{} };
}

// states
//   function: one model in each state the face distinguishes -- no runs; a
// match; a failure; an unread grammar; an EBNF run; two runs compared; and
// the heat map measuring steps at a step part way through.
std::vector<wb::model> states()
{
    std::vector<wb::model> out(7u, wb::make_model());

    wb::update(out[1], bare("run"));

    wb::update(out[2], carrying("edit_input", std::string("12+3-*45")));
    wb::update(out[2], bare("run"));

    wb::update(out[3], carrying("edit_grammar", std::string("Sum = Term")));
    wb::update(out[3], bare("run"));

    wb::update(out[4], carrying("frontend", 1L));
    wb::update(out[4], carrying("edit_grammar",
                                std::string("S = [0-9]+ (\"+\" [0-9]+)* ;")));
    wb::update(out[4], carrying("edit_input", std::string("1+22")));
    wb::update(out[4], bare("run"));

    out[5] = out[1];
    wb::update(out[5], carrying("edit_input", std::string("9-8")));
    wb::update(out[5], bare("run"));
    wb::update(out[5], carrying("compare_run", 0L));

    wb::update(out[6], carrying("edit_grammar", std::string(NESTED)));
    wb::update(out[6], carrying("edit_input", std::string("((1+2)*(3-4))/5")));
    wb::update(out[6], bare("run"));
    wb::update(out[6], carrying("heat_measure", 1L));
    wb::update(out[6], carrying("step", 2000L));

    return out;
}

// tab_of
//   function: the model with the tab of _part brought to the front.
wb::model tab_of(
    wb::model          _model,
    const std::string& _part
)
{
    _model.force_tab = wb::part_title(_part);

    return _model;
}


/*
tests_workbench_imgui_frames
  Every state of the model, on every tab, draws, and drawing alone asks for
nothing.
  Tests the following:
  - each of seven states, with each part's tab brought forward, draws frames
    with vertices in them
  - no frame needed Dear ImGui to recover from a misuse
  - with the pointer away, no frame posts an event
*/
bool tests_workbench_imgui_frames()
{
    headless                     ui;
    const std::vector<wb::model> models = states();

    for (const wb::model& each : models)
    {
        for (const std::string& part : wb::part_names())
        {
            const wb::model shown = tab_of(each, part);

            for (int i = 0; i < WARM_UP; ++i)
            {
                if (!ui.show(shown).empty())
                {
                    return false;
                }
            }

            if (ui.vertices() <= 0)
            {
                return false;
            }
        }
    }

    return (ui.errors() == 0);
}

/*
tests_workbench_imgui_sweep
  The pointer is walked over the whole face, on the tabs with the most to
point at, and nothing it passes over misbehaves.
  Tests the following:
  - the heat map (both runs of a comparison), the trace, the program and the
    matches, each hovered at every point of a grid over the display
  - no hover posts an event: hovering shows, it does not do
  - no frame recovered from a misuse, and no identifier under the pointer
    was claimed twice, the pointer resting long enough at each point for
    Dear ImGui to tell
*/
bool tests_workbench_imgui_sweep()
{
    headless                     ui;
    const std::vector<wb::model> models = states();
    const ImVec2                 size   = ImVec2(1920.0f, 1080.0f);
    const float                  grid   = 60.0f;

    // the heat map's run, compared with the example's
    wb::model heated = models[6];

    heated.runs.push_back(models[1].runs[0]);
    heated.runs.back().id = heated.next_id++;
    wb::update(heated, carrying("compare_run", 1L));

    const std::string tabs[] = { "heat", "trace", "program", "matches" };

    for (const std::string& part : tabs)
    {
        const wb::model shown = tab_of(heated, part);

        g_model = &shown;

        for (int i = 0; i < WARM_UP; ++i)
        {
            (void)ui.frame(wb::view(shown), AWAY, AWAY, false);
        }

        for (float y = 1.0f; y < size.y; y += grid)
        {
            for (float x = 1.0f; x < size.x; x += grid)
            {
                for (int i = 0; i < HOLD; ++i)
                {
                    if (!ui.frame(wb::view(shown), x, y, false).empty())
                    {
                        return false;
                    }
                }
            }
        }
    }

    return ( (ui.errors() == 0)     &&
             (ui.conflicts() == 0) );
}

/*
tests_workbench_imgui_click
  What a click posts is what update() takes: the Run button runs, a cell of
the heat map moves the step to the first visit there, and the workbench's
elements carry their "target" as uxoxo's own do.
  Tests the following:
  - pressing down the top of the face finds the Run button, whose event
    adds a run when applied
  - pressing over the heat map finds a cell, whose event names the cell
    and the first step that reached it, and moves the model's step there
  - a heat element given a "target" attribute copies it into what it posts
  - none of it needed Dear ImGui to recover from a misuse
*/
bool tests_workbench_imgui_click()
{
    headless  ui;
    wb::model bench = states()[6];

    // the Run button: the first control of the face, at its top left
    g_model = &bench;

    for (int i = 0; i < WARM_UP; ++i)
    {
        (void)ui.show(bench);
    }

    bool ran = false;

    for (float y = 2.0f; (y < 60.0f) && (!ran); y += 3.0f)
    {
        for (const event& each : ui.click(wb::view(bench), 14.0f, y))
        {
            if (each.kind == "run")
            {
                const std::size_t before = bench.runs.size();

                wb::update(bench, each);
                ran = (bench.runs.size() == before + 1u);
            }
        }
    }

    if (!ran)
    {
        return false;
    }

    // a cell of the heat map, which is the lower right of the Heat tab
    wb::model heat = tab_of(states()[6], "heat");

    g_model = &heat;

    for (int i = 0; i < WARM_UP; ++i)
    {
        (void)ui.show(heat);
    }

    // the cells a click names, checked against the map's own counts
    const wb::heat_space& space   = *heat.runs.back().heat;
    bool                  stepped = false;

    for (float y = 600.0f; (y < 1060.0f) && (!stepped); y += 40.0f)
    {
        for (float x = 700.0f; (x < 1900.0f) && (!stepped); x += 40.0f)
        {
            for (const event& each : ui.click(wb::view(heat), x, y))
            {
                const long to     = each.payload.as_long("value", -1L);
                const long column = each.payload.as_long("column", -1L);
                const long row    = each.payload.as_long("row", -1L);

                if ( (each.kind != "step")                                ||
                     (column < 0)                                         ||
                     (row < 0)                                            ||
                     (static_cast<std::size_t>(row) >= space.rows.size()) )
                {
                    continue;
                }

                const auto first = space.steps.first_visit(
                    { static_cast<std::size_t>(column),
                      static_cast<std::size_t>(row) });

                // the step named is the first that reached the cell named
                if ( (!first.has_value())                                  ||
                     (static_cast<long>(first.value()) != to)              ||
                     (each.payload.as_string("rule", "") !=
                      space.rows[static_cast<std::size_t>(row)]) )
                {
                    return false;
                }

                wb::update(heat, each);
                stepped = (heat.step == to);
            }
        }
    }

    if (!stepped)
    {
        return false;
    }

    // a heat element of the suite's own, with a target to carry
    option_set attrs;

    attrs.set("run", static_cast<long>(heat.runs.back().id));
    attrs.set("other", -1L);
    attrs.set("step", 0L);
    attrs.set("arrivals", true);
    attrs.set("action", std::string("step"));
    attrs.set("target", std::string("left"));

    const ::uxoxo::element_template own =
        ::uxoxo::make_element(wb::heat_type(), attrs, {});

    bool carried = false;

    for (int i = 0; i < WARM_UP; ++i)
    {
        (void)ui.frame(own, AWAY, AWAY, false);
    }

    for (float y = 100.0f; (y < 1060.0f) && (!carried); y += 40.0f)
    {
        for (float x = 100.0f; (x < 1900.0f) && (!carried); x += 40.0f)
        {
            for (const event& each : ui.click(own, x, y))
            {
                carried = ( (each.kind == "step") &&
                            (each.payload.as_string("target", "") ==
                             "left") );
            }
        }
    }

    return ( (carried)           &&
             (ui.errors() == 0) );
}


// SECTIONS
//   constant: the suite, in the order it runs.
const section SECTIONS[] =
{
    { "tests_workbench_imgui_frames", &tests_workbench_imgui_frames },
    { "tests_workbench_imgui_sweep",  &tests_workbench_imgui_sweep  },
    { "tests_workbench_imgui_click",  &tests_workbench_imgui_click  }
};

}  // anonymous namespace


/*
main
  Runs every section, then the summary line the build's test matches.
*/
int main()
{
    int passed = 0;
    int failed = 0;

    std::printf("vparse workbench ImGui tests\n");

    for (const section& each : SECTIONS)
    {
        const bool ok = each.run();

        std::printf("  [%s] %s\n", ok ? "PASS" : "FAIL", each.name);

        // tally for the summary line
        if (ok)
        {
            passed++;
        }
        else
        {
            failed++;
        }
    }

    std::printf("passed: %d   failed: %d\n", passed, failed);

    return (failed == 0) ? 0 : 1;
}

#endif  // floor, for now
