/*******************************************************************************
* vparse [ui]                                                       elements.cpp
*
* The workbench's own elements: their types, and the builders that make them.
*   Each is a leaf whose attributes name what to show -- a part of a run, a
* run's text, a run's heat -- and nothing else: the run itself is found when
* the element is drawn, through the finder the drawing platform was given. So
* a template stays small whatever the runs hold, and a platform that has not
* heard of these elements draws nothing for them rather than failing. Their
* drawing on Dear ImGui is in elements_imgui.cpp.
*
* path:      /src/ui/vparse/elements.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.22
*                                                            revised: 2026.10.26
*******************************************************************************/
#include "../../../inc/ui/vparse/elements.hpp"  // corresponding header
#if D_ENV_LANG_IS_CPP17_OR_HIGHER  // the floor the header has

// std
#include <string>  // std::string
// uxoxo
#include "uxoxo/element.hpp"  // element_type, make_element, option_set


namespace vparse
{
namespace workbench
{

using ::uxoxo::option_set;

/*
listing_type
  Leaf element; everything it shows is found through the run it names.
*/
const uxoxo::element_type* listing_type()
{
    static const uxoxo::element_type descriptor = uxoxo::element_type{
        "listing", "listing", option_set{}, 0, 0, option_set{},
        uxoxo::handler_fn{} };

    return &descriptor;
}

/*
listing_view
  Attributes only; the renderer reads the rest.
*/
uxoxo::element_template listing_view(
    const std::string& _part,
    int                _run,
    int                _other,
    long               _row,
    const std::string& _action
)
{
    option_set attrs;
    attrs.set("part", _part);
    attrs.set("run", static_cast<long>(_run));
    attrs.set("other", static_cast<long>(_other));
    attrs.set("row", _row);
    attrs.set("action", _action);

    return uxoxo::make_element(listing_type(), attrs, {});
}

/*
tape_type
  Leaf element, as listing_type.
*/
const uxoxo::element_type* tape_type()
{
    static const uxoxo::element_type descriptor = uxoxo::element_type{
        "tape", "tape", option_set{}, 0, 0, option_set{},
        uxoxo::handler_fn{} };

    return &descriptor;
}

/*
tape_view
  Attributes only; the renderer reads the rest.
*/
uxoxo::element_template tape_view(
    int                _run,
    const std::string& _which,
    long               _step,
    const std::string& _action
)
{
    option_set attrs;
    attrs.set("run", static_cast<long>(_run));
    attrs.set("which", _which);
    attrs.set("step", _step);
    attrs.set("action", _action);

    return uxoxo::make_element(tape_type(), attrs, {});
}

/*
heat_type
  Leaf element, as listing_type.
*/
const uxoxo::element_type* heat_type()
{
    static const uxoxo::element_type descriptor = uxoxo::element_type{
        "heat", "heat", option_set{}, 0, 0, option_set{},
        uxoxo::handler_fn{} };

    return &descriptor;
}

/*
heat_view
  Attributes only; the renderer reads the rest.
*/
uxoxo::element_template heat_view(
    int                _run,
    int                _other,
    long               _step,
    bool               _arrivals,
    const std::string& _action
)
{
    option_set attrs;
    attrs.set("run", static_cast<long>(_run));
    attrs.set("other", static_cast<long>(_other));
    attrs.set("step", _step);
    attrs.set("arrivals", _arrivals);
    attrs.set("action", _action);

    return uxoxo::make_element(heat_type(), attrs, {});
}

}  // namespace workbench
}  // namespace vparse

#endif  // floor, for now
