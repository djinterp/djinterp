/*******************************************************************************
* vparse [ui]                                                         plugin.cpp
*
* The window's module: what `vparse gui` loads (GRAMMAR.md, section 7) --
* vparse's workbench, in a window.
*   The module is a program of its own, made from the host that runs it --
* its name, version, verbosity and colours vparse's -- so that `vparse gui
* --help` is the window's help, and what the window says, it says as vparse
* does. Its operands are vparse's: a GRAMMAR and an INPUT, read as the
* commands read them, and the options it shares with the commands are spelled
* as they spell them -- `-f, --frontend`, `--ebnf`, `--root` (GRAMMAR.md,
* section 11). Its own say what the window starts with and how it looks; and,
* for building and testing, how many frames it draws and where the last is
* written. Its `--version --verbose` says, after how the module was built,
* the host linked and the Dear ImGui it draws with.
*   A grammar or an input that cannot be read is refused before the window
* opens, as bitwisedom's window refuses a sheet, and the run is trouble.
*   The frame loop is the whole of the window: the host begins a frame, the
* model's face (view.hpp) is realized on uxoxo's Dear ImGui platform and
* drawn, the events the drawing posted are applied to the model, and the host
* shows the frame. Which window that is -- Win32 with Direct3D 11, or GLFW
* with OpenGL 3 -- is the build's choice (host.hpp).
*   Nothing escapes the run function: the program's work is guarded by the
* kit, and what is made before it -- the options, the request they fill --
* here.
*   LEVELS. The workbench's floor is uxoxo's, C++17; below it this file is
* empty rather than an error.
*
* path:      /src/ui/vparse/plugin.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.22
*                                                            revised: 2026.10.26
*******************************************************************************/
#include "../../../inc/ui/vparse/view.hpp"  // model, make_model, view,
                                            // update, find_run
#if D_ENV_LANG_IS_CPP17_OR_HIGHER  // the floor the view has

// std
#include <exception>     // std::exception
#include <filesystem>    // std::filesystem::exists
#include <memory>        // std::make_shared, std::shared_ptr
#include <new>           // std::bad_alloc
#include <string>        // std::string, std::to_string
#include <system_error>  // std::error_code
#include <vector>        // std::vector
// djinterp
#include <djinterp/tools/cli/cli.hpp>  // program, command, D_CLI_PLUGIN
// imgui
#include <imgui.h>  // the IO, fonts, style, the workbench's window
// uxoxo
#include <uxoxo/element.hpp>                  // event, option_set
#include <uxoxo/platform/imgui_platform.hpp>  // imgui_platform, imgui_frame
#include <uxoxo/realize.hpp>                  // realize
// vparse
#include "../../../inc/ui/vparse/elements.hpp"  // install_renderers,
                                                // apply_theme
#include "../../../inc/ui/vparse/host.hpp"      // host_open, host_name,
                                                // host_begin_frame
#include "../../../inc/djinterp/tools/vparse/run.hpp"    // the frontend a
                                                         // file names
#include "../../../inc/djinterp/tools/vparse/store.hpp"  // list_saved,
                                                         // files by name


// VPARSE_VERSION
//   macro: the build's version, "<major>.<minor>.<patch>", which the build
// defines as vparse's own: a module belongs to its program at one version,
// and is refused by any other. A unit built without it says so rather than
// claim a number.
#ifndef VPARSE_VERSION
    #define VPARSE_VERSION "(unversioned)"
#endif  // VPARSE_VERSION

namespace
{

namespace kit = ::djinterp::cli;
namespace wb  = ::vparse::workbench;

// PROGRAM
//   constant: the program the module belongs to.
const char* const PROGRAM = "vparse";

// DESCRIPTION
//   constant: what the module does, as its help begins.
const char* const DESCRIPTION =
    "open vparse's workbench on a grammar and an input, or on its example: "
    "write a grammar and an input, run them, and look at every stage of "
    "the run";

// TESTING
//   constant: the heading the options for building and testing are listed
// under.
const char* const TESTING = "Testing";

// FRONTENDS
//   constant: the frontends --frontend takes, as the commands take them.
const char* const FRONTENDS[] = { "notation", "ebnf" };

// SIZE_MOST
//   constant: the most pixels --size takes for either side of the window.
const long SIZE_MOST = 32767L;

// PAPER
//   constant: the background behind the workbench's window, the theme's page.
const float PAPER[3] = { 0.969f, 0.976f, 0.957f };

// window_request
//   struct: what `vparse gui` was given, as the options bind it: the
// grammar and the input to start with, how the grammar is read, where runs
// are saved, a saved run to load, whether to run; the font, its size and the
// window's size as typed; and, for testing, the frames to draw and where the
// last goes.
struct window_request
{
    std::string grammar;
    std::string input;
    std::string frontend;
    bool        ebnf       = false;
    std::string root       = "vparse_runs";
    std::string load;
    bool        no_run     = false;
    std::string font;
    double      font_size  = 15.0;
    std::string size       = "1440x900";
    long        frames     = 0L;
    std::string screenshot;
};

//------------------------------------------------------------------------------
// the command line
//------------------------------------------------------------------------------

/*
side_of
  One side of WIDTHxHEIGHT: digits alone, from 1 to SIZE_MOST; -1
otherwise. `_end` is where it stopped.
*/
long side_of(
    const char*  _text,
    const char*& _end
)
{
    long side = 0L;

    _end = _text;

    // digits, and no more of them than a side can need
    while ( (*_end >= '0')  &&
            (*_end <= '9')  &&
            (side <= SIZE_MOST) )
    {
        side = (side * 10L) + static_cast<long>(*_end - '0');
        ++_end;
    }

    return ( (_end != _text)  &&
             (side >= 1L)     &&
             (side <= SIZE_MOST) ) ? side
                                   : -1L;
}

/*
size_of
  WIDTHxHEIGHT as two sides, `x` between them and nothing after: false, the
sizes untouched, for anything else.
*/
bool size_of(
    const std::string& _text,
    int&               _width,
    int&               _height
)
{
    const char* end   = nullptr;
    const long  width = side_of(_text.c_str(),
                                end);

    // the width, and the x after it
    if ( (width < 0L) ||
         ( (*end != 'x') &&
           (*end != 'X') ) )
    {
        return false;
    }

    const long height = side_of(end + 1,
                                end);

    // the height, and nothing after it
    if ( (height < 0L) ||
         (*end != '\0') )
    {
        return false;
    }

    _width  = static_cast<int>(width);
    _height = static_cast<int>(height);

    return true;
}

/*
size_problem
  --size's check: nothing for a size, else what one is.
*/
std::string size_problem(
    const std::string& _text
)
{
    int width  = 0;
    int height = 0;

    return size_of(_text,
                   width,
                   height) ? std::string()
                           : "'" + _text + "' is not WIDTHxHEIGHT: two "
                             "whole numbers of pixels, from 1 to " +
                             std::to_string(SIZE_MOST) + ", as 1440x900";
}

/*
add_start
  The operands, and the options that say what the window starts with --
spelled as the commands spell those they share with it.
*/
void add_start(
    kit::command    _root,
    window_request& _request
)
{
    _root.add_option("GRAMMAR",
                     _request.grammar,
                     "the grammar to start with: a file; by default the "
                     "workbench's example");
    _root.add_option("INPUT",
                     _request.input,
                     "the input to start with: a file");
    _root.add_option("-f,--frontend",
                     _request.frontend,
                     "how the grammar is read: notation, vparse's own, or "
                     "ebnf; by default ebnf for a file whose name ends in "
                     ".ebnf, and notation otherwise")
         .type_name("FRONTEND")
         .check(kit::is_member(FRONTENDS));
    _root.add_flag("--ebnf",
                   _request.ebnf,
                   "read the grammar as EBNF: the same as --frontend ebnf")
         .excludes("--frontend");
    _root.add_option("--root",
                     _request.root,
                     "the folder runs are saved in, a session folder each "
                     "time")
         .type_name("DIR")
         .capture_default_str();
    _root.add_option("--load",
                     _request.load,
                     "load a saved run's folder at the start, and run it "
                     "again")
         .type_name("DIR");
    _root.add_flag("--no-run",
                   _request.no_run,
                   "do not run the grammar the window starts with");

    return;
}

/*
add_look
  The options that say how the window looks; then, under their heading,
how long it stays and what it leaves, for building and testing.
*/
void add_look(
    kit::command    _root,
    window_request& _request
)
{
    _root.add_option("--font",
                     _request.font,
                     "a TrueType font; by default a system monospace font")
         .type_name("FILE");
    _root.add_option("--font-size",
                     _request.font_size,
                     "the font's size, in pixels")
         .type_name("PX")
         .check(kit::range(4.0, 256.0))
         .capture_default_str();
    _root.add_option("--size",
                     _request.size,
                     "the window's size, in pixels")
         .type_name("WIDTHxHEIGHT")
         .check(size_problem)
         .capture_default_str();
    _root.add_option("--frames",
                     _request.frames,
                     "draw this many frames, and close")
         .type_name("N")
         .check(kit::range(1L, 1000000000L))
         .group(TESTING);
    _root.add_option("--screenshot",
                     _request.screenshot,
                     "write the last frame --frames draws to FILE, as a "
                     "binary PPM image")
         .type_name("FILE")
         .needs("--frames")
         .group(TESTING);

    return;
}

//------------------------------------------------------------------------------
// the window
//------------------------------------------------------------------------------

/*
read_into
  A file the command line named, read as the commands read one: whole, in
binary, its name in UTF-8; one that cannot be read is said as they say it.
*/
bool read_into(
    const kit::program& _program,
    const std::string&  _name,
    std::string&        _editor
)
{
    std::string reason;

    // nothing named, nothing to read
    if (_name.empty())
    {
        return true;
    }

    // said as vparse says it, and the window does not open
    if (!wb::read_whole_file(wb::path_from_utf8(_name),
                             _editor,
                             &reason))
    {
        _program.report(kit::severity::error,
                        "cannot read '" + _name + "': " + reason);

        return false;
    }

    return true;
}

/*
starting_model
  The frontend is the one asked for; else, for a grammar file named, the
one its name calls for; else the example's.
*/
bool starting_model(
    const kit::program&   _program,
    const window_request& _request,
    wb::model&            _bench
)
{
    _bench      = wb::make_model();
    _bench.root = _request.root;

    // the grammar and the input, each read whole, or refused
    if ( (!read_into(_program,
                     _request.grammar,
                     _bench.grammar)) ||
         (!read_into(_program,
                     _request.input,
                     _bench.input)) )
    {
        return false;
    }

    // the frontend its name calls for, failing anything said
    if (!_request.grammar.empty())
    {
        _bench.frontend = wb::frontend_for_file(_request.grammar);
    }

    // said outright
    if (_request.ebnf)
    {
        _bench.frontend = "ebnf";
    }
    else if (!_request.frontend.empty())
    {
        _bench.frontend = _request.frontend;
    }

    _bench.saved = wb::list_saved(wb::path_from_utf8(_bench.root));

    return true;
}

/*
start
  The first run: a saved folder loaded, or the starting grammar run.
*/
void start(
    wb::model&            _bench,
    const window_request& _request
)
{
    // a saved run, run again
    if (!_request.load.empty())
    {
        uxoxo::option_set folder;

        folder.set("value",
                   _request.load);
        wb::update(_bench,
                   uxoxo::event{ "load_folder", folder });
    }
    else if (!_request.no_run)
    {
        wb::update(_bench,
                   uxoxo::event{ "run", uxoxo::option_set{} });
    }

    return;
}

/*
load_font
  The font asked for, else the first system monospace font found, else
Dear ImGui's own.
*/
void load_font(
    const window_request& _request
)
{
    ImGuiIO&                 io = ImGui::GetIO();
    std::vector<std::string> candidates;

    // the one asked for comes first
    if (!_request.font.empty())
    {
        candidates.push_back(_request.font);
    }

    candidates.insert(candidates.end(), {
        "C:/Windows/Fonts/consola.ttf",
        "/System/Library/Fonts/SFNSMono.ttf",
        "/System/Library/Fonts/Menlo.ttc",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
        "/usr/share/fonts/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf" });

    // the first font that exists and loads wins
    for (const std::string& path : candidates)
    {
        std::error_code missing;

        if ( (std::filesystem::exists(wb::path_from_utf8(path),
                                      missing)) &&
             (io.Fonts->AddFontFromFileTTF(
                  path.c_str(),
                  static_cast<float>(_request.font_size)) != nullptr) )
        {
            return;
        }
    }

    io.Fonts->AddFontDefault();

    return;
}

/*
open_window
  The window, its theme and font at the display's scale, the renderers
reading runs from the model, and the first run.
*/
bool open_window(
    const kit::program&   _program,
    wb::model&            _bench,
    const window_request& _request
)
{
    wb::window_options window;
    std::string        error;

    // checked as it was parsed
    (void)size_of(_request.size,
                  window.width,
                  window.height);

    // no window, no workbench
    if (!wb::host_open(window,
                       &error))
    {
        _program.report(kit::severity::error,
                        error);

        return false;
    }

    // the layout is the template's, not a file's
    ImGui::GetIO().IniFilename = nullptr;

    wb::apply_theme();
    ImGui::GetStyle().ScaleAllSizes(wb::host_scale());
    ImGui::GetStyle().FontScaleDpi = wb::host_scale();
    load_font(_request);

    // the renderers read runs from the model the loop updates
    wb::install_renderers([&_bench](int _id)
                          {
                              return wb::find_run(_bench,
                                                  _id);
                          });

    start(_bench,
          _request);

    return true;
}

/*
draw_frame
  Model, template, blueprint, pixels: the face realized and drawn, then
the events the frame raised applied, in order.
*/
void draw_frame(
    wb::model& _bench
)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::Begin("##workbench",
                 nullptr,
                 ImGuiWindowFlags_NoDecoration          |
                 ImGuiWindowFlags_NoMove                |
                 ImGuiWindowFlags_NoSavedSettings       |
                 ImGuiWindowFlags_NoBringToFrontOnFocus);

    const uxoxo::platform::imgui_blueprint blueprint =
        uxoxo::realize<uxoxo::platform::imgui_platform>(wb::view(_bench));
    uxoxo::platform::imgui_frame frame;

    blueprint.draw(frame);
    ImGui::End();

    _bench.force_tab.clear();

    // Ctrl+Enter runs from anywhere
    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Enter))
    {
        frame.post("run");
    }

    // the events, in the order the frame raised them
    for (const uxoxo::event& event : frame.events)
    {
        wb::update(_bench,
                   event);
    }

    return;
}

/*
show
  A frame at a time, until the window is closed or has drawn --frames; a
frame that could not be shown or written is said, and is trouble.
*/
int show(
    const kit::program&   _program,
    wb::model&            _bench,
    const window_request& _request
)
{
    std::string error;
    int         status = D_CLI_EXIT_OK;
    long        drawn  = 0L;

    // a frame at a time, until the window closes or --frames are drawn
    while (wb::host_begin_frame())
    {
        draw_frame(_bench);
        ++drawn;

        const bool last = ( (_request.frames > 0L) &&
                            (drawn >= _request.frames) );

        // a frame that could not be shown or written
        if (!wb::host_end_frame(PAPER[0],
                                PAPER[1],
                                PAPER[2],
                                last ? _request.screenshot : std::string(),
                                &error))
        {
            _program.report(kit::severity::error,
                            error);
            status = D_CLI_EXIT_TROUBLE;
        }

        // --frames drawn
        if (last)
        {
            break;
        }
    }

    wb::host_close();

    return status;
}

/*
open_workbench
  The module's work: the model the request starts with, refused before any
window where a file it names cannot be read; then the window, run.
*/
int open_workbench(
    kit::program&         _program,
    const window_request& _request
)
{
    wb::model bench;

    // a file named that cannot be read
    if (!starting_model(_program,
                        _request,
                        bench))
    {
        return D_CLI_EXIT_TROUBLE;
    }

    // no window
    if (!open_window(_program,
                     bench,
                     _request))
    {
        return D_CLI_EXIT_TROUBLE;
    }

    return show(_program,
                bench,
                _request);
}

/*
add_window_build
  The host as it names itself, and Dear ImGui -- its docking branch, where
it is that -- each a line; a want of memory is kept in the program, which
its run says.
*/
void add_window_build(
    kit::program& _window
)
{
#if defined(IMGUI_HAS_DOCK)
    const char* const branch = ", docking";
#else
    const char* const branch = "";
#endif  // IMGUI_HAS_DOCK

    (void)_window.add_build("window",
                            wb::host_name());
    (void)_window.add_build("imgui",
                            std::string(IMGUI_VERSION) + branch);

    return;
}

/*
run_gui
  The module's run function: its program, made from the host; the operands
and options, the window's lines in its version, and the work, the window.
What making them throws is said, and the run is trouble: nothing crosses
into the program that loaded it.
*/
int run_gui(
    const dj_cli_plugin_host* _host,
    int                       _argc,
    const char* const*        _argv
)
{
    kit::program window(*_host,
                        DESCRIPTION);

    try
    {
        const std::shared_ptr<window_request> request =
            std::make_shared<window_request>();
        kit::command root(window);

        add_start(root,
                  *request);
        add_look(root,
                 *request);
        add_window_build(window);
        window.set_main(
            [request](kit::program& _program, kit::command)
            {
                return open_workbench(_program,
                                      *request);
            });

        return window.run(_argc,
                          _argv);
    }
    catch (const std::bad_alloc&)
    {
        window.report(kit::severity::error,
                      "out of memory");
    }
    catch (const std::exception& _exception)
    {
        window.report(kit::severity::error,
                      _exception.what());
    }

    return D_CLI_EXIT_TROUBLE;
}

}  // anonymous namespace


D_CLI_PLUGIN(PROGRAM,
             VPARSE_VERSION,
             "gui",
             "open the workbench",
             run_gui)

#endif  // floor, for now
