/*******************************************************************************
* bitwisedom [ui]                                                  host_glfw.cpp
*
* The window's host on GLFW and OpenGL 3: Linux, macOS, and Windows where GLFW
* is installed.
*   The host owns the window, the GL context and ImGui's context and
* backends; each frame it runs the app between NewFrame and Render. It reads
* no command line: `bitwisedom gui` reads that (plugin.cpp), and gives it the
* events to post before the first frame, the window's size, the display's
* scale, and a screenshot -- for building and testing rather than for users:
* the frames asked for drawn, the last written as a binary PPM, whole or not
* at all as the kit writes a file, and the window closed. So a script can
* open an example, select a row, bring a tab to the front and photograph the
* result.
*   The window keeps the user's settings file, at preferences_path() -- the
* layout of its windows, the tables' column widths and the user's own
* preferences last from one run to the next -- and its session, at
* session_path(): its workspaces and every sheet in them, unsaved changes
* too, so it opens where it was left. A screenshot neither reads nor writes
* either, so it shows the window as built. A file dropped on the window is
* opened as File > Open opens one.
*   What GLFW says is said through the module's program: why a window could
* not be opened, as an error that ends the run in trouble; anything after,
* as a warning.
*
* path:      /src/ui/bitwisedom/host_glfw.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.23
*******************************************************************************/
#include "../../../inc/ui/bitwisedom/host.hpp"  // corresponding header
// std
#include <cstddef>  // std::size_t
#include <string>   // std::string, std::to_string
#include <vector>   // std::vector
// imgui
#include <imgui.h>               // ImGui
#include <imgui_impl_glfw.h>     // the GLFW platform backend
#include <imgui_impl_opengl3.h>  // the OpenGL 3 renderer backend
// glfw
#include <GLFW/glfw3.h>  // glfw*, and the system's GL 1.1 header
// djinterp
#include <djinterp/parse/cli/cli.hpp>  // program, severity, exit statuses
// bitwisedom
#include "../../../inc/ui/bitwisedom/app.hpp"    // app, preferences_path
#include "../../../inc/ui/bitwisedom/theme.hpp"  // dpi_scale


namespace
{

namespace kit = ::djinterp::parse::cli;

using bitwisedom::gui::app;
using bitwisedom::gui::window_options;

// glfw_voice
//   struct: what GLFW said last -- the reason a window that could not be
// opened gives -- and, once the window is open, the program that says the
// rest as warnings.
struct glfw_voice
{
    const kit::program* program = nullptr;
    std::string         last;
};

glfw_voice&
voice()
{
    static glfw_voice held;

    return held;
}

/*
report_glfw_error
  GLFW's error callback: its words kept, for an error that keeps the window
from opening; said as a warning once the window is open.
*/
void
report_glfw_error(
    int         _code,
    const char* _message
)
{
    glfw_voice& said = voice();

    said.last = "GLFW error " + std::to_string(_code) + ": " +
                ((_message != nullptr) ? _message : "no words");

    if (said.program != nullptr)
    {
        said.program->report(kit::severity::warning,
                             said.last);
    }

    return;
}

/*
cannot_open
  The window could not be opened: said, with what GLFW said last; trouble.
*/
int
cannot_open(
    const kit::program& _program
)
{
    const std::string& last = voice().last;

    _program.report(kit::severity::error,
                    last.empty() ? std::string("cannot open the window")
                                 : "cannot open the window: " + last);

    return D_CLI_EXIT_TROUBLE;
}

/*
write_ppm
  The framebuffer as a binary PPM, top row first, written as the kit writes
a file -- whole or not at all, `-` standard output: 0, or the errno value
the write failed with.
*/
int
write_ppm(
    const std::string& _path,
    int                _width,
    int                _height
)
{
    const std::size_t          row = static_cast<std::size_t>(_width) * 3u;
    std::vector<unsigned char> pixels(row *
                                      static_cast<std::size_t>(_height));
    std::string                image = "P6\n" + std::to_string(_width) +
                                       " " + std::to_string(_height) +
                                       "\n255\n";

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, _width, _height, GL_RGB, GL_UNSIGNED_BYTE,
                 pixels.data());
    image.reserve(image.size() + pixels.size());

    // GL's rows run bottom to top; a PPM's top to bottom
    for (int y = _height - 1; y >= 0; --y)
    {
        const unsigned char* const first =
            pixels.data() + static_cast<std::size_t>(y) * row;

        image.append(reinterpret_cast<const char*>(first),
                     row);
    }

    return kit::write_file(_path,
                           image);
}

// dropped
//   the files dropped on the window since the last frame: GLFW gives their
// paths as UTF-8.
std::vector<std::string>&
dropped()
{
    static std::vector<std::string> held;

    return held;
}

void
take_drop(
    GLFWwindow*  _window,
    int          _count,
    const char** _paths
)
{
    (void)_window;

    for (int i = 0; i < _count; ++i)
    {
        dropped().emplace_back(_paths[i]);
    }

    return;
}

/*
open_window
  GLFW, then the window and its GL context: GL 3.2 core where the platform
insists (macOS), 3.0 elsewhere, `_glsl` the shaders' version to match;
hidden for a screenshot. NULL, GLFW let go, when either fails.
*/
GLFWwindow*
open_window(
    const window_options& _options,
    const char*&          _glsl
)
{
    if (!glfwInit())
    {
        return nullptr;
    }

#if defined(__APPLE__)
    _glsl = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    _glsl = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    if (!_options.screenshot.empty())
    {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }

    GLFWwindow* window = glfwCreateWindow(_options.width,
                                          _options.height,
                                          "bitwis(e)dom",
                                          nullptr,
                                          nullptr);

    if (window == nullptr)
    {
        glfwTerminate();

        return nullptr;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    return window;
}

/*
start_imgui
  The display's scale -- the one asked for, else the window's content scale
-- then ImGui's context and backends, and the fonts at that scale. The
window keeps its own settings file, so ImGui keeps none.
*/
void
start_imgui(
    GLFWwindow*           _window,
    const window_options& _options,
    const char*           _glsl
)
{
    float scale = _options.scale;

    if (scale <= 0.0f)
    {
        float x = 1.0f;
        float y = 1.0f;

        glfwGetWindowContentScale(_window, &x, &y);
        scale = (x > 0.0f) ? x : 1.0f;
    }

    bitwisedom::gui::dpi_scale() = scale;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui::GetIO().IniFilename = nullptr;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForOpenGL(_window, true);
    ImGui_ImplOpenGL3_Init(_glsl);
    glfwSetDropCallback(_window, take_drop);
    app::load_fonts(16.0f * scale);

    return;
}

/*
draw_frame
  One frame: the files dropped since the last posted to the app, then the
app between NewFrame and Render, drawn over the theme's background. The
framebuffer's size, for a screenshot.
*/
void
draw_frame(
    GLFWwindow* _window,
    app&        _app,
    int&        _width,
    int&        _height
)
{
    glfwPollEvents();

    for (const std::string& each : dropped())
    {
        uxoxo::event made;

        made.kind = "file.drop";
        made.payload.set("value", each);
        _app.post(made);
    }

    dropped().clear();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    _app.frame();
    ImGui::Render();

    glfwGetFramebufferSize(_window, &_width, &_height);
    glViewport(0, 0, _width, _height);

    const ImVec4 clear = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);

    glClearColor(clear.x, clear.y, clear.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return;
}

/*
photograph
  The screenshot, taken before the buffers swap: written, and said with -v;
or not, and said as the kit says a file it could not write.
*/
int
photograph(
    const kit::program&   _program,
    const window_options& _options,
    int                   _width,
    int                   _height
)
{
    const int failed = write_ppm(_options.screenshot,
                                 _width,
                                 _height);

    if (failed != 0)
    {
        return _program.file_error("write",
                                   _options.screenshot,
                                   failed);
    }

    _program.note(1,
                  "wrote " + _options.screenshot + " (" +
                  std::to_string(_width) + "x" + std::to_string(_height) +
                  ")");

    return D_CLI_EXIT_OK;
}

/*
run_frames
  The app, its events posted, and its frames until the window is closed --
by the user, by the app quitting, or by the screenshot, the last frame. The
app's session and settings are kept unless a screenshot is taken.
*/
int
run_frames(
    const kit::program&   _program,
    GLFWwindow*           _window,
    const window_options& _options
)
{
    const bool kept   = _options.screenshot.empty();
    int        status = D_CLI_EXIT_OK;
    int        frame  = 0;
    app        bitwisedom(kept ? bitwisedom::gui::preferences_path()
                               : std::string(),
                          kept ? bitwisedom::gui::session_path()
                               : std::string());

    for (const uxoxo::event& each : _options.events)
    {
        bitwisedom.post(each);
    }

    while (!glfwWindowShouldClose(_window))
    {
        int width  = 0;
        int height = 0;

        draw_frame(_window,
                   bitwisedom,
                   width,
                   height);

        if ( (!kept) &&
             (++frame >= _options.frames) )
        {
            status = photograph(_program,
                                _options,
                                width,
                                height);
            glfwSetWindowShouldClose(_window, GLFW_TRUE);
        }

        glfwSwapBuffers(_window);

        if (bitwisedom.quitting())
        {
            glfwSetWindowShouldClose(_window, GLFW_TRUE);
        }
    }

    return status;
}

}  // anonymous namespace


/*
window_features
  A screenshot reads GL's framebuffer; WARP is Direct3D's.
*/
int
bitwisedom::gui::window_features()
{
    return static_cast<int>(window_feature_screenshot);
}

/*
run_window
  GLFW's errors said through the program, the window opened, the frames
run, and everything let go in the order it was made.
*/
int
bitwisedom::gui::run_window(
    kit::program&         _program,
    const window_options& _options
)
{
    const char* glsl = nullptr;

    voice() = glfw_voice();
    glfwSetErrorCallback(report_glfw_error);

    GLFWwindow* window = open_window(_options,
                                     glsl);

    if (window == nullptr)
    {
        return cannot_open(_program);
    }

    voice().program = &_program;
    start_imgui(window,
                _options,
                glsl);

    const int status = run_frames(_program,
                                  window,
                                  _options);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    voice() = glfw_voice();
    glfwSetErrorCallback(nullptr);

    return status;
}
