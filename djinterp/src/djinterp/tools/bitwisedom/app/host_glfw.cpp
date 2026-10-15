/*******************************************************************************
* bitwisedom [app]                                                 host_glfw.cpp
*
* bitwisedom's main on GLFW and OpenGL 3: Linux, macOS, and Windows where GLFW
* is installed.
*   The host owns the window, the GL context and ImGui's context and
* backends; each frame it runs the app between NewFrame and Render. Two
* options exist for building and testing rather than for users:
*
*     --do KIND[:VALUE[:TARGET]]   post an event before the first frame,
*                                  as an element would: VALUE and TARGET
*                                  are numbers where they parse as one,
*                                  true or false, or text
*     --screenshot FILE.ppm        draw --frames frames (default 6), write
*                                  the last as a binary PPM, and exit
*     --size WxH                   the window's size (default 1600x960)
*     --scale S                    the display scale (default: GLFW's)
*
*   So a script can open an example, select a row, bring a tab to the front
* and photograph the result: the build's screenshot test does exactly that.
*   The window keeps the user's settings file, at preferences_path(): the
* tables' column widths and the panels' sizes last from one run to the
* next. A screenshot neither reads nor writes it, so it shows the window as
* built. A file dropped on the window is opened as File > Open opens one.
*
* path:      /src/djinterp/tools/bitwisedom/app/host_glfw.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.09
*******************************************************************************/
// std
#include <cstdio>   // std::fopen, std::fprintf, std::fwrite
#include <cstdlib>  // std::atof, std::atoi
#include <string>   // std::string
#include <vector>   // std::vector
// imgui
#include <imgui.h>                // ImGui
#include <imgui_impl_glfw.h>      // the GLFW platform backend
#include <imgui_impl_opengl3.h>   // the OpenGL 3 renderer backend
// glfw
#include <GLFW/glfw3.h>  // glfw*, and the system's GL 1.1 header
// bitwisedom
#include "./app.hpp"    // app
#include "./theme.hpp"  // ui_scale


namespace
{

using bitwisedom::gui::app;

// options
//   struct: the command line.
struct options
{
    int                       width      = 1600;
    int                       height     = 960;
    float                     scale      = 0.0f;
    std::string               screenshot;
    int                       frames     = 6;
    std::vector<uxoxo::event> events;
};

// value_of
//   a --do field as the payload value it means.
::djinterp::option_value
value_of(
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

// event_of
//   a --do argument as the event it posts.
uxoxo::event
event_of(
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
        made.payload.set("target", value_of(_text.substr(second + 1)));
    }

    return made;
}

bool
parse(
    int      _argc,
    char**   _argv,
    options& _options
)
{
    for (int i = 1; i < _argc; ++i)
    {
        const std::string each = _argv[i];
        const bool        more = (i + 1 < _argc);

        if ( (each == "--do") && more )
        {
            _options.events.push_back(event_of(_argv[++i]));
        }
        else if ( (each == "--screenshot") && more )
        {
            _options.screenshot = _argv[++i];
        }
        else if ( (each == "--frames") && more )
        {
            _options.frames = std::atoi(_argv[++i]);
        }
        else if ( (each == "--scale") && more )
        {
            _options.scale = static_cast<float>(std::atof(_argv[++i]));
        }
        else if ( (each == "--size") && more )
        {
            const std::string size = _argv[++i];
            const std::size_t x    = size.find('x');

            if (x != std::string::npos)
            {
                _options.width  = std::atoi(size.substr(0, x).c_str());
                _options.height = std::atoi(size.substr(x + 1).c_str());
            }
        }
        else
        {
            std::fprintf(stderr,
                         "bitwisedom: unknown argument '%s'\n"
                         "usage: bitwisedom [--do KIND[:VALUE[:TARGET]]]... "
                         "[--screenshot FILE.ppm [--frames N]] "
                         "[--size WxH] [--scale S]\n",
                         each.c_str());

            return false;
        }
    }

    return true;
}

// write_ppm
//   the framebuffer as a binary PPM, top row first.
bool
write_ppm(
    const std::string& _path,
    int                _width,
    int                _height
)
{
    std::vector<unsigned char> pixels(
        static_cast<std::size_t>(_width) * static_cast<std::size_t>(_height) *
        3u);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, _width, _height, GL_RGB, GL_UNSIGNED_BYTE,
                 pixels.data());

    std::FILE* out = std::fopen(_path.c_str(), "wb");

    if (out == nullptr)
    {
        return false;
    }

    std::fprintf(out, "P6\n%d %d\n255\n", _width, _height);

    // GL's rows run bottom to top; a PPM's top to bottom
    for (int y = _height - 1; y >= 0; --y)
    {
        std::fwrite(pixels.data() + static_cast<std::size_t>(y) *
                                        static_cast<std::size_t>(_width) * 3u,
                    1,
                    static_cast<std::size_t>(_width) * 3u,
                    out);
    }

    std::fclose(out);

    return true;
}

void
report_glfw_error(
    int         _code,
    const char* _message
)
{
    std::fprintf(stderr, "bitwisedom: GLFW error %d: %s\n", _code, _message);

    return;
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

}  // anonymous namespace


int
main(
    int    _argc,
    char** _argv
)
{
    options chosen;

    if (!parse(_argc, _argv, chosen))
    {
        return 2;
    }

    glfwSetErrorCallback(report_glfw_error);

    if (!glfwInit())
    {
        return 1;
    }

    // GL 3.2 core where the platform insists (macOS), 3.0 elsewhere
#if defined(__APPLE__)
    const char* glsl = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    const char* glsl = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    if (!chosen.screenshot.empty())
    {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }

    GLFWwindow* window = glfwCreateWindow(chosen.width,
                                          chosen.height,
                                          "bitwis(e)dom",
                                          nullptr,
                                          nullptr);

    if (window == nullptr)
    {
        glfwTerminate();

        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    float scale = chosen.scale;

    // the display's scale, unless the command line set one
    if (scale <= 0.0f)
    {
        float x = 1.0f;
        float y = 1.0f;

        glfwGetWindowContentScale(window, &x, &y);
        scale = (x > 0.0f) ? x : 1.0f;
    }

    bitwisedom::gui::ui_scale() = scale;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // the window keeps its own settings file; a screenshot keeps none
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl);
    glfwSetDropCallback(window, take_drop);
    app::load_fonts(16.0f * scale);

    {
        app bitwisedom(chosen.screenshot.empty()
                           ? bitwisedom::gui::preferences_path()
                           : std::string());

        for (const uxoxo::event& each : chosen.events)
        {
            bitwisedom.post(each);
        }

        int frame = 0;

        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();

            // a file dropped on the window, opened
            for (const std::string& each : dropped())
            {
                uxoxo::event made;
                made.kind = "file.drop";
                made.payload.set("value", each);
                bitwisedom.post(made);
            }

            dropped().clear();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            bitwisedom.frame();
            ImGui::Render();

            int width  = 0;
            int height = 0;

            glfwGetFramebufferSize(window, &width, &height);
            glViewport(0, 0, width, height);

            const ImVec4 clear = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);

            glClearColor(clear.x, clear.y, clear.z, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            // the photograph is taken before the buffers swap
            if ( (!chosen.screenshot.empty()) &&
                 (++frame >= chosen.frames) )
            {
                const bool written = write_ppm(chosen.screenshot,
                                               width,
                                               height);

                std::fprintf(stderr, "bitwisedom: %s %s (%dx%d)\n",
                             written ? "wrote" : "could not write",
                             chosen.screenshot.c_str(), width, height);
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }

            glfwSwapBuffers(window);

            if (bitwisedom.quitting())
            {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
        }
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
