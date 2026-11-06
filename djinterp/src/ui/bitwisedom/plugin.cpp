/*******************************************************************************
* bitwisedom [ui]                                                     plugin.cpp
*
* The window's module: what `bitwisedom gui` loads (GRAMMAR.md, section 7).
*   The module is a program of its own, made from the host that runs it --
* its name, version, verbosity and colours bitwisedom's -- so that
* `bitwisedom gui --help` is the window's help, and what the window says, it
* says as bitwisedom does. Its operands are SHEETs, every one handed to the
* window, what it will not open said on standard error as it starts and the
* run ending in trouble when it closes (window_sheet_opens); or, by --sheets
* or the build (D_CFG_BITWISEDOM_GUI_SHEETS), refused by the commands' own
* rules before any window opens (window_sheet_of) -- ruling cli-taken 25.
* Its options are the window's size and the
* display's scale, and, for building and testing, the events posted before
* the first frame and what the host linked can do besides: a screenshot on
* GLFW's, WARP on Win32's (window_features). Its `--version --verbose` says,
* after how the module was built, the host linked (window_host) and the Dear
* ImGui it draws with.
*   Each sheet named is posted to the window as the Examples menu or File >
* Open posts one, then each --do in order, before the first frame; and the
* window opens where it was left, its session kept, unless it is taking a
* screenshot.
*   On Windows, a console made for this run alone -- one it was started in
* from a shortcut, not a terminal it shares -- is let go before the window
* opens, so that no empty console stands behind it.
*   Nothing escapes the run function: the program's work is guarded by the
* kit, and what is made before it -- the options, the request they fill --
* here.
*
* path:      /src/ui/bitwisedom/plugin.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.11.04
*******************************************************************************/
// std
#include <cstddef>    // std::size_t
#include <cstdio>     // std::snprintf
#include <cstdlib>    // std::strtol, std::strtod
#include <exception>  // std::exception
#include <memory>     // std::make_shared
#include <new>        // std::bad_alloc
#include <string>     // std::string
#include <vector>     // std::vector
// windows
#if defined(_WIN32)
    #ifndef NOMINMAX
        #define NOMINMAX 1
    #endif  // NOMINMAX
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN 1
    #endif  // WIN32_LEAN_AND_MEAN
    #include <windows.h>  // GetConsoleProcessList, FreeConsole
#endif  // _WIN32
// djinterp
#include <djinterp/tools/cli/cli.hpp>  // program, command, D_CLI_PLUGIN
#include <djinterp/config/tools/bitwisedom/cfg_bitwisedom.h>  // D_INTERNAL_
                                       // BITWISEDOM_GUI_SHEETS
// bitwisedom
#include <djinterp/tools/bitwisedom/cli.hpp>      // window_sheet_of,
                                                  // window_sheet_opens
#include <djinterp/tools/bitwisedom/version.hpp>  // BITWISEDOM_PROGRAM,
                                                  // BITWISEDOM_VERSION
#include "../../../inc/ui/bitwisedom/host.hpp"    // run_window,
                                                  // window_options,
                                                  // window_host
// imgui
#include <imgui.h>  // IMGUI_VERSION, IMGUI_HAS_DOCK


namespace
{

namespace kit = ::djinterp::cli;

using bitwisedom::gui::window_options;

// DESCRIPTION
//   constant: what the module does, as its help begins.
const char* const DESCRIPTION =
    "open bitwisedom's window on the sheets named, or where it was left";

// TESTING
//   constant: the heading the options for building and testing are listed
// under.
const char* const TESTING = "Testing";

// SIZE_LARGEST
//   constant: the most points either side of the window may be asked for.
const long SIZE_LARGEST = 32767L;

// SCALE_LEAST / SCALE_MOST
//   constants: the display scales the window may be asked to draw at --
// beyond them its fonts are too small to make or too large to hold.
const double SCALE_LEAST = 0.25;
const double SCALE_MOST  = 8.0;

// SHEETS_WORDS
//   constant: the words of --sheets, in the order of
// D_CFG_BITWISEDOM_GUI_SHEETS_*: refuse a SHEET the window cannot open
// before it opens, or report it and open the rest.
const char* const SHEETS_WORDS[] = { "refuse", "report" };

// window_request
//   struct: what `bitwisedom gui` was given: the sheets, the --do events,
// the size as typed and what to do with a sheet the window cannot open, and
// the rest of the window's options as they bind.
struct window_request
{
    std::vector<std::string> sheets;
    std::vector<std::string> events;
    std::string              size;
    std::string              unopened =
        SHEETS_WORDS[D_INTERNAL_BITWISEDOM_GUI_SHEETS - 1];
    window_options           options;
};

/*
value_of
  A --do field as the payload value it means: true or false; a whole
number; a real one; else the text.
*/
::djinterp::option_value value_of(
    const std::string& _text
)
{
    if (_text == "true")
    {
        return ::djinterp::option_value(true);
    }

    if (_text == "false")
    {
        return ::djinterp::option_value(false);
    }

    char*      end   = nullptr;
    const long whole = std::strtol(_text.c_str(), &end, 10);

    if ( (!_text.empty()) &&
         (*end == '\0') )
    {
        return ::djinterp::option_value(whole);
    }

    const double real = std::strtod(_text.c_str(), &end);

    if ( (!_text.empty()) &&
         (*end == '\0') )
    {
        return ::djinterp::option_value(real);
    }

    return ::djinterp::option_value(_text);
}

/*
event_of
  A --do argument as the event it posts: KIND, then VALUE and TARGET where
the colons give them.
*/
uxoxo::event event_of(
    const std::string& _text
)
{
    uxoxo::event      made;
    const std::size_t first  = _text.find(':');
    const std::size_t second = (first == std::string::npos)
                                   ? std::string::npos
                                   : _text.find(':', first + 1);

    made.kind = _text.substr(0, first);

    if (first != std::string::npos)
    {
        made.payload.set("value",
                         value_of(_text.substr(first + 1,
                                               second - first - 1)));
    }

    if (second != std::string::npos)
    {
        made.payload.set("target",
                         value_of(_text.substr(second + 1)));
    }

    return made;
}

/*
side_of
  One side of WxH: digits alone, from 1 to SIZE_LARGEST; -1 otherwise.
`_end` is where it stopped.
*/
long side_of(
    const char*  _text,
    const char*& _end
)
{
    long side = 0L;

    _end = _text;

    while ( (*_end >= '0') &&
            (*_end <= '9') &&
            (side <= SIZE_LARGEST) )
    {
        side = side * 10L + static_cast<long>(*_end - '0');
        ++_end;
    }

    return ( (_end != _text)      &&
             (side >= 1L)         &&
             (side <= SIZE_LARGEST) )
           ? side
           : -1L;
}

/*
size_of
  WxH as two sides, `x` between them and nothing after: false, the sizes
untouched, for anything else.
*/
bool size_of(
    const std::string& _text,
    int&               _width,
    int&               _height
)
{
    const char* end    = nullptr;
    const long  width  = side_of(_text.c_str(), end);

    if ( (width < 0L) ||
         (*end != 'x') )
    {
        return false;
    }

    const long height = side_of(end + 1, end);

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

    return size_of(_text, width, height)
               ? std::string()
               : "'" + _text + "' is not WxH: two whole numbers of points, "
                 "from 1 to " + std::to_string(SIZE_LARGEST) +
                 ", as 1600x960";
}

/*
scale_problem
  --scale's check: nothing for a number from SCALE_LEAST to SCALE_MOST,
else what a scale is.
*/
std::string scale_problem(
    const std::string& _text
)
{
    char*        end   = nullptr;
    const double scale = std::strtod(_text.c_str(), &end);
    char         what[64];

    if ( (!_text.empty())       &&
         (*end == '\0')         &&
         (scale >= SCALE_LEAST) &&
         (scale <= SCALE_MOST) )
    {
        return std::string();
    }

    (void)std::snprintf(what,
                        sizeof(what),
                        "a number from %g to %g, as 1.5",
                        SCALE_LEAST,
                        SCALE_MOST);

    return "'" + _text + "' is not a scale: " + what;
}

/*
sheets_problem
  --sheets' check: nothing for one of its words, else what they are.
*/
std::string sheets_problem(
    const std::string& _text
)
{
    return ( (_text == SHEETS_WORDS[0]) ||
             (_text == SHEETS_WORDS[1]) )
               ? std::string()
               : "'" + _text + "' is not " + SHEETS_WORDS[0] + " or " +
                 SHEETS_WORDS[1];
}

/*
event_problem
  --do's check: nothing for an event with a kind, else that it needs one.
*/
std::string event_problem(
    const std::string& _text
)
{
    return ( (_text.empty()) ||
             (_text[0] == ':') )
               ? std::string("an event begins with its KIND")
               : std::string();
}

/*
add_options
  The operands and the options every host acts on; then the testing options
the host linked acts on, under their heading.
*/
void add_options(
    kit::command    _root,
    window_request& _request
)
{
    const int features = bitwisedom::gui::window_features();

    _root.add_option("SHEET",
                     _request.sheets,
                     "a sheet to open: a sheet file, a bitfield, an export or "
                     "a header to import; example:NAME for a built-in "
                     "example");
    _root.add_option("--sheets",
                     _request.unopened,
                     "what to do with a SHEET the window cannot open: report "
                     "it on standard error, open the rest and end in "
                     "trouble; or refuse it before the window opens")
         .type_name("HOW")
         .default_str(_request.unopened)
         .check(sheets_problem);
    _root.add_option("--size",
                     _request.size,
                     "the window's size, in points (default 1600x960)")
         .type_name("WxH")
         .check(size_problem);
    _root.add_option("--scale",
                     _request.options.scale,
                     "the display's scale, in place of the one it reports")
         .type_name("S")
         .check(scale_problem);
    _root.add_option("--do",
                     _request.events,
                     "post an event before the first frame, as an element "
                     "would: VALUE and TARGET are numbers where they read as "
                     "one, true or false, or text")
         .type_name("KIND[:VALUE[:TARGET]]")
         .check(event_problem)
         .group(TESTING);

    if ((features & bitwisedom::gui::window_feature_screenshot) != 0)
    {
        _root.add_option("--screenshot",
                         _request.options.screenshot,
                         "draw the frames --frames says, write the last to "
                         "FILE as a binary PPM, - for standard output, and "
                         "close; nothing read from or kept in the settings "
                         "or the session")
             .type_name("FILE")
             .group(TESTING);
        _root.add_option("--frames",
                         _request.options.frames,
                         "the frames a screenshot draws (default 6)")
             .type_name("N")
             .check(kit::range(1, 100000))
             .needs("--screenshot")
             .group(TESTING);
    }

    if ((features & bitwisedom::gui::window_feature_warp) != 0)
    {
        _root.add_flag("--warp",
                       _request.options.warp,
                       "draw with Direct3D's software rasterizer, WARP, "
                       "whatever the GPU, to tell the GPU's faults from the "
                       "drawing's")
             .group(TESTING);
    }

    return;
}

/*
post_sheets
  Each SHEET posted as the window opens it: an example as the Examples menu
posts one, by its place; a file as File > Open posts one, by its path.
Reporting, every one is posted, and what the window will not open is said
as it is met, `_unopened` set; refusing, by the commands' rules, false,
having said why, at the first the window could not open.
*/
bool post_sheets(
    kit::program&                   _program,
    const std::vector<std::string>& _sheets,
    bool                            _report,
    window_options&                 _options,
    bool&                           _unopened
)
{
    _unopened = false;

    for (const std::string& each : _sheets)
    {
        bitwisedom::window_sheet sheet;
        uxoxo::event             made;

        if (_report)
        {
            _unopened = (!bitwisedom::window_sheet_opens(_program, each,
                                                         sheet)) ||
                        _unopened;
        }
        else if (!bitwisedom::window_sheet_of(_program, each, sheet))
        {
            return false;
        }

        made.kind = (sheet.example >= 0) ? "file.example" : "file.open";
        made.payload.set("value",
                         (sheet.example >= 0)
                             ? ::djinterp::option_value(
                                   static_cast<long>(sheet.example))
                             : ::djinterp::option_value(sheet.path));
        _options.events.push_back(made);
    }

    return true;
}

/*
leave_console
  On Windows, a console this run alone holds -- made for it, as a shortcut
starts a console program -- let go, so that no empty console stands behind
the window; one shared with the terminal that started the run is kept, to
be heard. Nothing elsewhere.
*/
void leave_console()
{
#if defined(_WIN32)
    DWORD holders[2] = { 0, 0 };

    if (::GetConsoleProcessList(holders, 2) == 1)
    {
        (void)::FreeConsole();
    }
#endif  // _WIN32

    return;
}

/*
open_window
  The module's work: the sheets named, then each --do, as the events the
window posts first; the size asked; and the window, run -- its status, or
trouble where a sheet it was handed could not be opened.
*/
int open_window(
    kit::program&   _program,
    window_request& _request
)
{
    window_options& options  = _request.options;
    const bool      report   = (_request.unopened == SHEETS_WORDS[1]);
    bool            unopened = false;

    if (!post_sheets(_program, _request.sheets, report, options, unopened))
    {
        return D_CLI_EXIT_TROUBLE;
    }

    for (const std::string& each : _request.events)
    {
        options.events.push_back(event_of(each));
    }

    // checked as it was parsed
    if (!_request.size.empty())
    {
        (void)size_of(_request.size,
                      options.width,
                      options.height);
    }

    leave_console();

    const int status = bitwisedom::gui::run_window(_program,
                                                   options);

    return (unopened) ? D_CLI_EXIT_TROUBLE : status;
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
#endif

    (void)_window.add_build("window",
                            bitwisedom::gui::window_host());
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

        add_options(root,
                    *request);
        add_window_build(window);
        window.set_main(
            [request](kit::program& _program, kit::command)
            {
                return open_window(_program,
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
    catch (...)
    {
        window.report(kit::severity::error,
                      "the window's options could not be made");
    }

    return D_CLI_EXIT_TROUBLE;
}

}  // anonymous namespace


D_CLI_PLUGIN(BITWISEDOM_PROGRAM,
             BITWISEDOM_VERSION,
             "gui",
             "open the window",
             run_gui)
