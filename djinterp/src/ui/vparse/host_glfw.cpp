/*******************************************************************************
* vparse [ui]                                                      host_glfw.cpp
*
* The workbench's window on GLFW and OpenGL 3: the host for Linux, macOS, and
* Windows where GLFW is found.
*   Dear ImGui's GLFW backend handles the window's input and its OpenGL 3
* backend draws. The context asked for is OpenGL 3.3 core, which the backend's
* "#version 330" shaders need; macOS asks for it forward-compatible, as it
* must. A capture reads the back buffer after the frame is drawn and before
* it is shown, and turns OpenGL's bottom-up rows the right way up.
*
* path:      /src/ui/vparse/host_glfw.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.08
*                                                            revised: 2026.10.26
*******************************************************************************/
#include "../../../inc/ui/vparse/host.hpp"  // corresponding header
#if D_ENV_LANG_IS_CPP17_OR_HIGHER  // the floor the header has

// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::fopen, std::fprintf, std::fwrite, std::fclose
#include <string>   // std::string
#include <vector>   // std::vector
// glfw
#include <GLFW/glfw3.h>  // windows, input, the OpenGL context and its gl*
// imgui
#include "imgui.h"               // the context, frames, draw data
#include "imgui_impl_glfw.h"     // the GLFW platform backend
#include "imgui_impl_opengl3.h"  // the OpenGL 3 renderer backend


namespace vparse
{
namespace workbench
{

namespace
{

// the window, while one is open
GLFWwindow* g_window = nullptr;
float       g_scale  = 1.0f;

// GLSL
//   constant: the shading language version the context asked for supports.
const char* const GLSL = "#version 330";

// fail
//   records why there is no window, and returns false for the caller to.
bool fail(
    std::string*       _error,
    const std::string& _why
)
{
    if (_error != nullptr)
    {
        *_error = _why;
    }

    return false;
}

// capture
//   the back buffer as a binary PPM, top row first.
bool capture(
    const std::string& _path,
    int                _width,
    int                _height
)
{
    const std::size_t row = static_cast<std::size_t>(_width) * 3u;

    std::vector<unsigned char> pixels(row * static_cast<std::size_t>(_height));

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, _width, _height, GL_RGB, GL_UNSIGNED_BYTE,
                 pixels.data());

    std::FILE* file = std::fopen(_path.c_str(), "wb");

    if (file == nullptr)
    {
        return false;
    }

    std::fprintf(file, "P6\n%d %d\n255\n", _width, _height);

    // OpenGL's rows run bottom to top
    for (int y = _height - 1; y >= 0; --y)
    {
        std::fwrite(pixels.data() + (static_cast<std::size_t>(y) * row),
                    1,
                    row,
                    file);
    }

    return (std::fclose(file) == 0);
}

}  // anonymous namespace


/*
host_name
  Fixed: the build compiles one host.
*/
const char* host_name()
{
    return "GLFW + OpenGL 3";
}

/*
host_scale
  GLFW reports the primary monitor's content scale; the window may later move
to another monitor, which this does not follow.
*/
float host_scale()
{
    return g_scale;
}

/*
host_open
  GLFW scales the window to the monitor itself where the system asks it to
(GLFW_SCALE_TO_MONITOR), so the size is passed as given.
*/
bool host_open(
    const window_options& _options,
    std::string*          _error
)
{
    if (g_window != nullptr)
    {
        return fail(_error, "a window is already open");
    }

    if (!glfwInit())
    {
        return fail(_error, "GLFW could not start: is there a display?");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif  // __APPLE__

    g_window = glfwCreateWindow(_options.width,
                                _options.height,
                                _options.title.c_str(),
                                nullptr,
                                nullptr);

    if (g_window == nullptr)
    {
        glfwTerminate();

        return fail(_error, "no OpenGL 3.3 window could be made");
    }

    glfwMakeContextCurrent(g_window);
    glfwSwapInterval(1);

    g_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(g_window, true);
    ImGui_ImplOpenGL3_Init(GLSL);

    return true;
}

/*
host_close
  The backends go before the context, and the context before the window, the
reverse of the order they were made in.
*/
void host_close()
{
    if (g_window == nullptr)
    {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(g_window);
    glfwTerminate();

    g_window = nullptr;
    g_scale  = 1.0f;

    return;
}

/*
host_begin_frame
  A minimized window has nothing to draw, so the host sleeps through it here,
still handling events, rather than drawing frames nobody can see.
*/
bool host_begin_frame()
{
    glfwPollEvents();

    while ( (!glfwWindowShouldClose(g_window)) &&
            (glfwGetWindowAttrib(g_window, GLFW_ICONIFIED) != 0) )
    {
        ImGui_ImplGlfw_Sleep(10);
        glfwPollEvents();
    }

    if (glfwWindowShouldClose(g_window))
    {
        return false;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    return true;
}

/*
host_end_frame
  The capture reads the back buffer, so it must come between drawing and the
swap that shows the frame.
*/
bool host_end_frame(
    float              _red,
    float              _green,
    float              _blue,
    const std::string& _capture,
    std::string*       _error
)
{
    ImGui::Render();

    int width  = 0;
    int height = 0;

    glfwGetFramebufferSize(g_window, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(_red, _green, _blue, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    bool captured = true;

    if ( (!_capture.empty()) &&
         (!capture(_capture, width, height)) )
    {
        captured = fail(_error, "could not write " + _capture);
    }

    glfwSwapBuffers(g_window);

    return captured;
}

}  // namespace workbench
}  // namespace vparse

#endif  // floor, for now
