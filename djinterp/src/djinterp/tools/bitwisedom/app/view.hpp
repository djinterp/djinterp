/*******************************************************************************
* bitwisedom [app]                                                      view.hpp
*
* The interface, as a uxoxo template built from the model.
*
* path:      /src/djinterp/tools/bitwisedom/app/view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/

#ifndef BITWISEDOM_APP_VIEW_HPP
#define BITWISEDOM_APP_VIEW_HPP 1

// uxoxo
#include <uxoxo/element.hpp>  // element_template
// bitwisedom
#include "./model.hpp"  // model


namespace bitwisedom
{
namespace gui
{

/**
 * @brief The whole window, as a template.
 *
 * @param[in] _model  the model; read, never written.
 * @return the template the app realizes on the ImGui platform.
 */
uxoxo::element_template build_ui(const model& _model);

}  // namespace gui
}  // namespace bitwisedom


#endif  // BITWISEDOM_APP_VIEW_HPP
