/*******************************************************************************
* vparse [ui]                                                           host.hpp
*
* A window for the workbench, behind six calls.
*   The face is drawn by uxoxo's Dear ImGui platform and knows nothing of
* windows; plugin.cpp knows nothing of graphics. Between them is this file: it
* opens a window with a graphics device and a Dear ImGui context set up for
* both, begins and ends frames, captures what a frame drew, and closes. One
* source implements it for each pair of Dear ImGui backends, and the build
* compiles exactly one:
*
*     host_win32.cpp     Win32 and Direct3D 11 -- on Windows, with
*                             nothing to find
*     host_glfw.cpp   GLFW and OpenGL 3 -- wherever GLFW is found
*     host_none.cpp           no window: host_open() says why; a run is
*                             printed by vparse, the command line, which
*                             needs no window
*
*   The Dear ImGui context belongs to the host. host_open() creates it and
* host_close() destroys it, so whatever styles the face, and loads its fonts,
* goes between the two.
*   LEVELS. The workbench's floor is uxoxo's, C++17; below it this file is
* empty rather than an error.
*
*
* path:      /inc/ui/vparse/host.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.08
*                                                            revised: 2026.10.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE WINDOW ASKED FOR
    --------------------
    1.  window_options
2.  THE HOST
    --------
    1.  Identity
    2.  Lifetime
    3.  Frames
*/

#ifndef VPARSE_UI_HOST_HPP
#define VPARSE_UI_HOST_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error, as
// the rest of the workbench is: its floor is uxoxo's.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <string>  // std::string
// djinterp
#include "djinterp/cpp/djinterp.hpp"  // framework root


namespace vparse
{
namespace workbench
{


//==============================================================================
// 1.  THE WINDOW ASKED FOR
//==============================================================================


// 1.1    window_options
//------------------------------------------------------------------------------
// 1.1.1
// window_options
//   struct: the window asked for. Its size is in logical pixels, which a host
// multiplies by host_scale() where the system does not.
struct window_options
{
    std::string title  = "vparse workbench";
    int         width  = 1440;
    int         height = 900;
};


//==============================================================================
// 2.  THE HOST
//==============================================================================


// 2.1    Identity
//------------------------------------------------------------------------------
// 2.1.1
// what this build's host is -- "Win32 + Direct3D 11", "GLFW + OpenGL 3", or
// "none" -- and the monitor's content scale, 1 at 96 dots per inch; the scale
// is 1 until a window is open
const char* host_name();
float host_scale();


// 2.2    Lifetime
//------------------------------------------------------------------------------
/**
 * @brief Opens the window, its graphics device, and a Dear ImGui context set
 *        up for both.
 *
 * @param[in]  _options  the window asked for.
 * @param[out] _error    why there is no window, when there is none; may be
 *                       null.
 * @pre    No window is open.
 * @post   On success the current Dear ImGui context is the host's, with both
 *         backends initialized, and the window is shown. On failure nothing
 *         is left open.
 * @return whether the window opened.
 */
D_NODISCARD bool host_open(
    const window_options& _options,
    std::string*          _error
);

/**
 * @brief Shuts Dear ImGui's backends down, destroys its context, and closes
 *        the window.
 *
 * @post   No window is open, and no Dear ImGui context is current. Closing
 *         with no window open does nothing.
 */
void host_close();


// 2.3    Frames
//------------------------------------------------------------------------------
/**
 * @brief Begins a frame: handles the window's events, waits while the window
 *        is minimized, and starts a Dear ImGui frame.
 *
 * @pre    A window is open.
 * @return false when the window was asked to close, and then no frame is
 *         begun; true when a Dear ImGui frame has begun.
 */
D_NODISCARD bool host_begin_frame();

/**
 * @brief Ends the frame begun last: renders Dear ImGui's draw data over a
 *        cleared background, and shows it.
 *
 * @param[in]  _red      the background's red, in [0, 1].
 * @param[in]  _green    the background's green, in [0, 1].
 * @param[in]  _blue     the background's blue, in [0, 1].
 * @param[in]  _capture  a path to write the frame to, as a binary PPM,
 *                       before it is shown; empty for none.
 * @param[out] _error    why the capture failed, when it did; may be null.
 * @pre    host_begin_frame() began a frame that has not been ended.
 * @return whether the capture asked for was written; true when none was.
 */
bool host_end_frame(
    float              _red,
    float              _green,
    float              _blue,
    const std::string& _capture,
    std::string*       _error
);

}  // namespace workbench
}  // namespace vparse

#endif  // floor, for now

#endif  // VPARSE_UI_HOST_HPP
