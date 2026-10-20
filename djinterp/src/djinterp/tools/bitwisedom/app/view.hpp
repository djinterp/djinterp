/*******************************************************************************
* bitwisedom [app]                                                      view.hpp
*
* The interface, as a uxoxo template built from the window's state: the
* workspace in front, its documents, and the document in front of it.
*
* path:      /src/djinterp/tools/bitwisedom/app/view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.18
*******************************************************************************/

#ifndef BITWISEDOM_APP_VIEW_HPP
#define BITWISEDOM_APP_VIEW_HPP 1

// std
#include <vector>  // std::vector
// uxoxo
#include <uxoxo/element.hpp>  // element_template
// bitwisedom
#include "./model.hpp"  // model
#include "./shell.hpp"  // shell_state


namespace bitwisedom
{
namespace gui
{

// shell_view
//   struct: what the interface is built from: the window around the
// documents; the document in front; the documents of the workspace in
// front, in the order they were opened; and every document open, in every
// workspace.
struct shell_view
{
    const shell_state&        shell;
    const model&              front;
    std::vector<const model*> here;
    std::vector<const model*> every;
};

/**
 * @brief The whole window, as a template.
 *
 * @param[in] _view  the window's state; read, never written.
 * @return the template the app realizes on the ImGui platform.
 */
uxoxo::element_template build_ui(const shell_view& _view);

}  // namespace gui
}  // namespace bitwisedom


#endif  // BITWISEDOM_APP_VIEW_HPP
