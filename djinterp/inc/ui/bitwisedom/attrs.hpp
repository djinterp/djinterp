/*******************************************************************************
* bitwisedom [ui]                                                      attrs.hpp
*
* Attribute records in one expression: attrs({ { "width", 0.5 }, ... }).
*   An option_value is a variant of bool, long, double and string, and a
* string literal handed to it would, at some library levels, choose bool.
* attr_value takes each kind of argument explicitly, so a literal is text, an
* int is a long and a float a double, whatever the library.
*
* path:      /inc/ui/bitwisedom/attrs.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/

#ifndef UI_BITWISEDOM_ATTRS_HPP
#define UI_BITWISEDOM_ATTRS_HPP 1

// std
#include <initializer_list>  // std::initializer_list
#include <string>            // std::string
#include <utility>           // std::pair
// uxoxo
#include <uxoxo/element.hpp>  // option_set


namespace bitwisedom
{
namespace gui
{

// attr_value
//   struct: one attribute's value, converted the way it was meant.
struct attr_value
{
    ::djinterp::option_value value;

    attr_value(bool _value)        : value(_value) {}
    attr_value(int _value)         : value(static_cast<long>(_value)) {}
    attr_value(long _value)        : value(_value) {}
    attr_value(double _value)      : value(_value) {}
    attr_value(float _value)       : value(static_cast<double>(_value)) {}
    attr_value(const char* _value) : value(std::string(_value)) {}
    attr_value(const std::string& _value)
        : value(_value)
    {}
};

// attrs
//   an attribute record from a list of names and values.
inline uxoxo::option_set
attrs(
    std::initializer_list<std::pair<const char*, attr_value>> _entries
)
{
    uxoxo::option_set made;

    for (const std::pair<const char*, attr_value>& each : _entries)
    {
        made.set(each.first, each.second.value);
    }

    return made;
}

}  // namespace gui
}  // namespace bitwisedom


#endif  // UI_BITWISEDOM_ATTRS_HPP
