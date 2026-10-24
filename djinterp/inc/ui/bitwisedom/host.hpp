/*******************************************************************************
* bitwisedom [ui]                                                       host.hpp
*
* The window's host: what opens the window, runs its frames and closes it.
*   Each platform's host defines run_window -- Win32 and Direct3D 11 on
* Windows (host_win32.cpp), GLFW and OpenGL 3 elsewhere (host_glfw.cpp) --
* and the build links one. The module's run function (plugin.cpp) reads
* `bitwisedom gui`'s arguments into window_options and calls it, so a host
* reads no command line of its own; and it says what it has to say -- a
* window that could not be opened, a screenshot written -- through the
* program the module made, so it says it as every djinterp program does.
*
*
* path:      /inc/ui/bitwisedom/host.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE HOST
    --------
    1.  What the window is asked
         1.  window_options
         2.  window_feature
         3.  window_features
    2.  Running it
         1.  run_window
*/

#ifndef UI_BITWISEDOM_HOST_HPP
#define UI_BITWISEDOM_HOST_HPP 1

// std
#include <string>  // std::string
#include <vector>  // std::vector
// uxoxo
#include <uxoxo/element.hpp>  // event


namespace djinterp
{
namespace parse
{
namespace cli
{

class program;

}  // namespace cli
}  // namespace parse
}  // namespace djinterp


namespace bitwisedom
{
namespace gui
{


//==============================================================================
// 1.  THE HOST
//==============================================================================


// 1.1    What the window is asked
//------------------------------------------------------------------------------
// 1.1.1
// window_options
//   struct: what `bitwisedom gui` asked of the window: the events to post
// before the first frame -- the sheets named, then each --do -- and the
// testing options, each left as it is here unless asked for.
struct window_options
{
    // the events posted before the first frame, in order
    std::vector<uxoxo::event> events;

    // a screenshot: the file a binary PPM of the last of `frames` frames is
    // written to, after which the window closes; empty for none. A
    // screenshot reads and writes neither the settings nor the session
    std::string screenshot;
    int         frames = 6;

    // the window's size, in the display's points, and the display's scale:
    // 0 for the one the display reports
    int         width  = 1600;
    int         height = 960;
    float       scale  = 0.0f;

    // Direct3D's software rasterizer, WARP, whatever the GPU
    bool        warp   = false;
};

// 1.1.2
// window_feature
//   enum: the window_options a host may or may not act on, each a bit of
// what window_features() returns, so that the command line offers a host's
// testing options only where they do something.
enum window_feature
{
    window_feature_screenshot = 1 << 0,  // screenshot, frames
    window_feature_warp       = 1 << 1   // warp
};

// 1.1.3
// window_features
/**
 * @brief The window_options the linked host acts on, beside the events,
 *        the size and the scale, which every host does.
 *
 * @return window_feature values, or'd.
 */
int  window_features();

// 1.2    Running it
//------------------------------------------------------------------------------
// 1.2.1
// run_window
/**
 * @brief Opens the window, runs its frames until it is closed or the
 *        screenshot is taken, and closes it.
 *
 * @param[in] _program  the module's program, which says what the host has
 *                      to say: an error when the window cannot be opened,
 *                      or the screenshot not written; with `-v`, a note
 *                      that it was.
 * @param[in] _options  what the window is asked.
 * @return the status the command exits with (GRAMMAR.md, section 8): 0 when
 *         the window closed, or the screenshot was written; 2 when the
 *         window could not be opened or the screenshot not written.
 */
int  run_window(::djinterp::parse::cli::program& _program,
                const window_options&            _options);


}  // namespace gui
}  // namespace bitwisedom


#endif  // UI_BITWISEDOM_HOST_HPP
