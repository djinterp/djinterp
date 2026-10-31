/*******************************************************************************
* vparse [ui]                                                      host_none.cpp
*
* The host of a build with no window: what the workbench is compiled with
* where neither of the others can be -- not on Windows, and no GLFW found.
*   host_open() refuses with the reason and the remedy; nothing else is ever
* reached, since every other call needs an open window. `vparse gui` still
* reads its command line and answers --help; a run is printed by vparse's
* commands, which need no window at all.
*
* path:      /src/ui/vparse/host_none.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.08
*                                                            revised: 2026.10.26
*******************************************************************************/
#include "../../../inc/ui/vparse/host.hpp"  // corresponding header
#if D_ENV_LANG_IS_CPP17_OR_HIGHER  // the floor the header has

// std
#include <string>  // std::string


namespace vparse
{
namespace workbench
{

/*
host_name
  Fixed: the build compiles one host.
*/
const char* host_name()
{
    return "none";
}

/*
host_scale
  There is never a monitor to read.
*/
float host_scale()
{
    return 1.0f;
}

/*
host_open
  The remedy is named here, where the user meets the problem, rather than
left to the configure log.
*/
bool host_open(
    const window_options& _options,
    std::string*          _error
)
{
    (void)_options;

    if (_error != nullptr)
    {
        *_error = "this build has no window: it was configured without a "
                  "Dear ImGui host. Install GLFW (on Debian and Ubuntu, "
                  "libglfw3-dev) and configure again. vparse, the command "
                  "line, needs no window: vparse run GRAMMAR INPUT";
    }

    return false;
}

/*
host_close
  Nothing was opened.
*/
void host_close()
{
    return;
}

/*
host_begin_frame
  No window, so it is always closing.
*/
bool host_begin_frame()
{
    return false;
}

/*
host_end_frame
  Never reached: no frame can begin.
*/
bool host_end_frame(
    float              _red,
    float              _green,
    float              _blue,
    const std::string& _capture,
    std::string*       _error
)
{
    (void)_red;
    (void)_green;
    (void)_blue;
    (void)_error;

    return _capture.empty();
}

}  // namespace workbench
}  // namespace vparse

#endif  // floor, for now
