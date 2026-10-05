/*******************************************************************************
* djinterp [core]                                               html_printer.hpp
*
*   The HTML serialisation policy -- the format half of the printer retargeted
* from XML to HTML. The single behavioural difference that matters is the
* empty-element rule: HTML has VOID elements (`<br>`, `<img>`, `<input>`, ...)
* that take no closing tag and no trailing slash, while every other element is
* paired even when empty (`<div></div>`, never `<div/>`). Void-ness is decided
* by tag name through `html::is_void_element_name`, so the policy needs no
* table of its own.
*
*   COMPILE-TIME AND RUNTIME:
*     compile-time   using html_document = document<html_print_policy>;
*                    render(node, html_print_policy{});
*     runtime        render(node, html_policy());        // boxed
*                    document<boxed_print_policy> doc( html_policy() );
*   `html_print_policy` is an ordinary policy; `html_policy()` boxes it for
* runtime selection. Both plug into the same `document_printer` / `render` /
* `document` as the XML policy does.
*
*   v1 LIMITATION: `escape_text` escapes uniformly and has no element context,
* so it would (wrongly) escape the body of raw-text elements such as <script>
* and <style>. Those need context-aware handling and are out of scope here.
*
*   Requires C++17; self-suppresses below it.
*
*
* path:      /inc/djinterp/core/text/html/html_printer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.18
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    HTML PRINT POLICY
      -----------------
      a. html_print_policy
      b. html_policy
      c.    html_document
*/

#ifndef DJINTERP_TEXT_HTML_HTML_PRINTER_HPP
#define DJINTERP_TEXT_HTML_HTML_PRINTER_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <string>
// djinterp
#include "../../../djinterp.hpp"
#include "../../util/document/document.hpp"         // document<>, document_printer, element_form
#include "./html.hpp"             // html::is_void_element_name


#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                  I.   HTML PRINT POLICY                                 ///
///////////////////////////////////////////////////////////////////////////////

// html_print_policy
//   struct: HTML serialisation rules. Text/attribute escaping match XML's
// safe subset (`& < >`, plus `"` in attributes); the substance is `form_of`,
// which classifies void elements as `voidlike` (open tag only) and everything
// else as `paired` -- HTML never self-closes a non-void element.
struct html_print_policy
{
    // escape_text
    //   function: escape character data (`&`, `<`, `>`).
    static xml_string_t
    escape_text(
        const xml_string_t& _in
    )
    {
        xml_string_t _out;
        _out.reserve(_in.size());

        for (char _c : _in)
        {
            switch (_c)
            {
                case '&':  _out += "&amp;"; break;
                case '<':  _out += "&lt;";  break;
                case '>':  _out += "&gt;";  break;
                default:   _out += _c;      break;
            }
        }

        return _out;
    }

    // escape_attribute
    //   function: escape character data plus the double quote, for values in
    // double-quoted attributes.
    static xml_string_t
    escape_attribute(
        const xml_string_t& _in
    )
    {
        xml_string_t _out;
        _out.reserve(_in.size());

        for (char _c : _in)
        {
            switch (_c)
            {
                case '&':  _out += "&amp;";  break;
                case '<':  _out += "&lt;";   break;
                case '>':  _out += "&gt;";   break;
                case '"':  _out += "&quot;"; break;
                default:   _out += _c;       break;
            }
        }

        return _out;
    }

    // form_of
    //   function: void elements (by tag name) render as a lone open tag;
    // all other elements are paired, empty or not.
    static element_form
    form_of(
        const xml_string_t& _name,
        bool                /*_is_empty*/
    )
    {
        return html::is_void_element_name(_name.c_str())
                   ? element_form::voidlike
                   : element_form::paired;
    }
};


// html_policy
//   function: the HTML policy as a runtime value, for runtime format
// selection (e.g. `render(node, choose ? html_policy() : xml_policy())`).
D_NODISCARD inline boxed_print_policy
html_policy()
{
    return boxed_print_policy(html_print_policy{});
}


// html_document
//   type: the fluent façade fixed to HTML serialisation (compile-time policy).
using html_document = document<html_print_policy>;


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // floor, for now

#endif  // DJINTERP_TEXT_HTML_HTML_PRINTER_HPP
