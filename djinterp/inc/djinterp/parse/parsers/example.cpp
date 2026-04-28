/******************************************************************************
* djinterp [css]                                                   example.cpp
*
* End-to-end usage example for the css subsystem.
*
*   Demonstrates:
*     - parsing a stylesheet from a string literal
*     - building a synthetic widget tree (no html involved)
*     - constructing style_target_view records over that tree
*     - registering host-defined pseudo-states (here: :hover, :focus,
*       :disabled) so that selectors using them resolve correctly
*     - compiling the stylesheet into a css_engine for indexed lookup
*     - resolving the final cascade for each widget and printing the
*       result
*
*   This file is illustrative; it is not part of the header-only
* library and does not need to be compiled into a host project.  Build
* it with the rest of the framework's translation units to confirm
* that the css headers integrate cleanly:
*
*     g++ -std=c++17 -I<framework-root>/inc/cpp  \
*         example.cpp -o css_example
*
*
* path:      /inc/cpp/css/example.cpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.25
******************************************************************************/

#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

#include "./css_ast.hpp"
#include "./css_parser.hpp"
#include "./css_match.hpp"
#include "./css_engine.hpp"


// ================================================================
//  Host-application "widget" model
// ================================================================
//
//   The css subsystem makes no assumptions about how the host stores
// its own objects.  The host's only obligation is to populate a
// style_target_view per object before asking the engine to resolve
// properties for it.  Below is a deliberately unremarkable widget
// tree -- a fixed-size bag of pointers and a few strings -- to show
// how little is required.

namespace example_app
{

// widget_state_bits
//   enum: host-defined pseudo-state bit positions.  These names
// are registered into style_match_options below so that selectors
// like `:hover` and `:disabled` resolve against the bits set in
// each widget's state_flags.
enum widget_state_bits
{
    WidgetStateHoverBit     = 0,
    WidgetStateFocusBit     = 1,
    WidgetStateDisabledBit  = 2,
    WidgetStateActiveBit    = 3
};


// widget
//   struct: a stand-in for any host object the application wants
// to style.  The class list and attribute list are stored as
// vectors here for simplicity; a real host would likely use its
// own arena-backed storage.
struct widget
{
    std::string                                                 type_name;
    std::string                                                 id;
    std::vector<std::string>                                    classes;
    std::vector<std::pair<std::string, std::string> >           attributes;
    std::uint64_t                                               state_flags;

    widget*                                                     parent;
    std::vector<widget*>                                        children;

    widget()
        : type_name   ()
        , id          ()
        , classes     ()
        , attributes  ()
        , state_flags (0u)
        , parent      (nullptr)
        , children    ()
    {}
};


// add_child
//   function: appends _child to _parent's children and back-links
// the parent pointer.  Trivial helper used only to keep main()
// readable.
inline void
add_child(widget* _parent,
          widget* _child)
{
    _child->parent = _parent;
    _parent->children.push_back(_child);

    return;
}


// build_view
//   function: populates _out from _w plus the position-in-parent
// information that the caller has already computed.  The classes
// and attributes are projected into the C-string buffers supplied
// by the caller (which must be sized correctly).
inline void
build_view(const widget&                                         _w,
           const widget*                                         _prev,
           std::size_t                                           _index,
           std::size_t                                           _siblings,
           std::vector<const char*>&                             _class_buf,
           std::vector<djinterp::css::style_target_attribute>&   _attr_buf,
           djinterp::css::style_target_view&                     _out)
{
    std::size_t i;

    _class_buf.clear();
    _attr_buf.clear();

    for (i = 0u; i < _w.classes.size(); ++i)
    {
        _class_buf.push_back(_w.classes[i].c_str());
    }

    for (i = 0u; i < _w.attributes.size(); ++i)
    {
        _attr_buf.push_back(
            djinterp::css::style_target_attribute(
                _w.attributes[i].first.c_str(),
                _w.attributes[i].second.c_str()
            )
        );
    }

    _out.type_name           = _w.type_name.empty()
                                    ? nullptr
                                    : _w.type_name.c_str();
    _out.id                  = _w.id.empty()
                                    ? nullptr
                                    : _w.id.c_str();
    _out.classes             = _class_buf.empty()
                                    ? nullptr
                                    : _class_buf.data();
    _out.classes_count       = _class_buf.size();
    _out.attributes          = _attr_buf.empty()
                                    ? nullptr
                                    : _attr_buf.data();
    _out.attributes_count    = _attr_buf.size();
    _out.parent              = nullptr;       // patched up by caller
    _out.prev_sibling        = nullptr;       // patched up by caller
    _out.index_in_parent     = _index;
    _out.parent_child_count  = _siblings;
    _out.state_flags         = _w.state_flags;

    (void)_prev;

    return;
}

}  // namespace example_app


// ================================================================
//  Helpers for printing parse / resolve output
// ================================================================

// dump_value
//   function: writes a single css_value to stdout in a roughly
// canonical form.  Recursive for function-call values.
static void
dump_value(const djinterp::css::css_value& _v)
{
    using namespace djinterp::css;

    switch (_v.kind)
    {
        case DCssValueKindIdent:
            std::printf("%s", _v.text.c_str());
            break;

        case DCssValueKindNumber:
            if (_v.unit.empty())
            {
                std::printf("%g", _v.number);
            }
            else
            {
                std::printf("%g%s", _v.number, _v.unit.c_str());
            }
            break;

        case DCssValueKindPercentage:
            std::printf("%g%%", _v.number);
            break;

        case DCssValueKindString:
            std::printf("\"%s\"", _v.text.c_str());
            break;

        case DCssValueKindHash:
            std::printf("#%s", _v.text.c_str());
            break;

        case DCssValueKindFunction:
        {
            std::size_t i;

            std::printf("%s(", _v.text.c_str());

            for (i = 0u; i < _v.args.size(); ++i)
            {
                if (i > 0u)
                {
                    std::printf(" ");
                }

                dump_value(_v.args[i]);
            }

            std::printf(")");
        }
        break;

        case DCssValueKindDelim:
            std::printf("%s", _v.text.c_str());
            break;
    }

    return;
}


// dump_value_list
//   function: writes a value list separated by single spaces.
static void
dump_value_list(const djinterp::css::css_value_list& _vs)
{
    std::size_t i;

    for (i = 0u; i < _vs.size(); ++i)
    {
        if (i > 0u)
        {
            std::printf(" ");
        }

        dump_value(_vs[i]);
    }

    return;
}


// resolve_and_print
//   function: resolves the cascade for _w and writes the property
// map to stdout.  The widget tree must already have been built and
// _w must be reachable through _root.
static void
resolve_and_print(const example_app::widget&            _w,
                  const example_app::widget*            _prev,
                  std::size_t                           _index,
                  std::size_t                           _siblings,
                  const example_app::widget*            _parent_widget,
                  const djinterp::css::style_target_view*
                                                        _parent_view,
                  const djinterp::css::style_target_view*
                                                        _prev_view,
                  const djinterp::css::css_engine&      _engine,
                  const djinterp::css::style_match_options&
                                                        _opt)
{
    std::vector<const char*>                                   class_buf;
    std::vector<djinterp::css::style_target_attribute>         attr_buf;
    djinterp::css::style_target_view                           view;
    djinterp::css::resolved_property_map                       props;

    example_app::build_view(_w,
                            _prev,
                            _index,
                            _siblings,
                            class_buf,
                            attr_buf,
                            view);

    view.parent       = _parent_view;
    view.prev_sibling = _prev_view;

    _engine.resolve(view, _opt, props);

    std::printf("  %s",
                _w.type_name.empty()
                        ? "*"
                        : _w.type_name.c_str());

    if (!_w.id.empty())
    {
        std::printf("#%s", _w.id.c_str());
    }

    {
        std::size_t i;

        for (i = 0u; i < _w.classes.size(); ++i)
        {
            std::printf(".%s", _w.classes[i].c_str());
        }
    }

    std::printf("\n");

    if (props.empty())
    {
        std::printf("        (no styles applied)\n");
    }
    else
    {
        djinterp::css::resolved_property_map::const_iterator it;

        for (it = props.begin(); it != props.end(); ++it)
        {
            std::printf("        %-18s: ", it->first.c_str());
            dump_value_list(it->second);
            std::printf(";\n");
        }
    }

    (void)_parent_widget;

    return;
}


// ================================================================
//  Main
// ================================================================

int
main()
{
    using namespace djinterp::css;

    // ------------------------------------------------------------
    //  1.  parse a stylesheet
    // ------------------------------------------------------------
    //
    //   Note that none of these selectors mention "html", "div", or
    // any other HTML-specific type.  The host (example_app) has its
    // own type vocabulary -- "window", "panel", "button", etc. --
    // and the engine cheerfully matches against it.

    static const char k_source[] =
        "/* sample stylesheet */                                  \n"
        "* {                                                      \n"
        "    color:        black;                                 \n"
        "    font-size:    12px;                                  \n"
        "}                                                        \n"
        "                                                         \n"
        "window {                                                 \n"
        "    background:   #f0f0f0;                               \n"
        "    padding:      8px;                                   \n"
        "}                                                        \n"
        "                                                         \n"
        "button {                                                 \n"
        "    background:   #ddd;                                  \n"
        "    border:       1px solid #888;                        \n"
        "}                                                        \n"
        "                                                         \n"
        "button.primary {                                         \n"
        "    background:   #2266cc;                               \n"
        "    color:        white;                                 \n"
        "    font-weight:  bold;                                  \n"
        "}                                                        \n"
        "                                                         \n"
        "button:hover {                                           \n"
        "    background:   #3377dd;                               \n"
        "}                                                        \n"
        "                                                         \n"
        "button:disabled,                                         \n"
        "button[disabled] {                                       \n"
        "    color:        #999 !important;                       \n"
        "    background:   #eee;                                  \n"
        "}                                                        \n"
        "                                                         \n"
        "panel#content > label.title {                            \n"
        "    font-size:    18px;                                  \n"
        "    font-weight:  bold;                                  \n"
        "    color:        #333;                                  \n"
        "}                                                        \n"
        "                                                         \n"
        "input[type=\"number\"] {                                 \n"
        "    text-align:   right;                                 \n"
        "    font-family:  monospace;                             \n"
        "}                                                        \n"
        "                                                         \n"
        "label.title + input {                                    \n"
        "    margin-top:   2px;                                   \n"
        "}                                                        \n";

    parse::parse_result<css_stylesheet> parsed =
        parse_stylesheet(k_source, sizeof(k_source) - 1u);

    if (!parsed.ok())
    {
        std::fprintf(stderr,
                     "parse error at offset %zu: %s\n",
                     parsed.error().offset(),
                     parsed.error().message() != nullptr
                            ? parsed.error().message()
                            : "(no message)");
        return 1;
    }

    const css_stylesheet& sheet = parsed.value();

    std::printf("parsed stylesheet: %zu rules, %zu at-rules\n\n",
                sheet.rules.size(),
                sheet.at_rules.size());

    // ------------------------------------------------------------
    //  2.  compile into the engine
    // ------------------------------------------------------------

    css_engine engine;

    engine.compile(sheet);

    // ------------------------------------------------------------
    //  3.  build a synthetic widget tree
    // ------------------------------------------------------------
    //
    //   window#main
    //   |- header.toolbar
    //   |    |- button.primary           (hovered)
    //   |    \- button.secondary[disabled]
    //   \- panel#content
    //        |- label.title
    //        \- input[type="number"][value="42"]

    example_app::widget root;
    root.type_name = "window";
    root.id        = "main";

    example_app::widget header;
    header.type_name = "header";
    header.classes.push_back("toolbar");
    example_app::add_child(&root, &header);

    example_app::widget btn_primary;
    btn_primary.type_name = "button";
    btn_primary.classes.push_back("primary");
    btn_primary.state_flags = (1ull << example_app::WidgetStateHoverBit);
    example_app::add_child(&header, &btn_primary);

    example_app::widget btn_secondary;
    btn_secondary.type_name = "button";
    btn_secondary.classes.push_back("secondary");
    btn_secondary.attributes.push_back(std::make_pair(
        std::string("disabled"),
        std::string("")
    ));
    btn_secondary.state_flags =
        (1ull << example_app::WidgetStateDisabledBit);
    example_app::add_child(&header, &btn_secondary);

    example_app::widget content;
    content.type_name = "panel";
    content.id        = "content";
    example_app::add_child(&root, &content);

    example_app::widget title;
    title.type_name = "label";
    title.classes.push_back("title");
    example_app::add_child(&content, &title);

    example_app::widget number;
    number.type_name = "input";
    number.attributes.push_back(std::make_pair(
        std::string("type"),
        std::string("number")
    ));
    number.attributes.push_back(std::make_pair(
        std::string("value"),
        std::string("42")
    ));
    example_app::add_child(&content, &number);

    // ------------------------------------------------------------
    //  4.  register host-defined pseudo-states
    // ------------------------------------------------------------

    style_match_options opt;
    opt.register_state("hover",    example_app::WidgetStateHoverBit);
    opt.register_state("focus",    example_app::WidgetStateFocusBit);
    opt.register_state("disabled", example_app::WidgetStateDisabledBit);
    opt.register_state("active",   example_app::WidgetStateActiveBit);

    // ------------------------------------------------------------
    //  5.  resolve & print, walking the tree top-down
    // ------------------------------------------------------------
    //
    //   For each widget we (a) build a style_target_view, (b) link
    // it to the views already produced for its parent / previous
    // sibling, and (c) ask the engine to resolve the cascade.

    std::printf("--- resolved styles ---\n\n");

    // root
    std::vector<const char*>                            root_class;
    std::vector<style_target_attribute>                 root_attr;
    style_target_view                                   root_view;

    example_app::build_view(root,
                            nullptr,
                            0u,
                            1u,
                            root_class,
                            root_attr,
                            root_view);

    {
        resolved_property_map props;

        engine.resolve(root_view, opt, props);

        std::printf("  window#main\n");

        resolved_property_map::const_iterator it;

        for (it = props.begin(); it != props.end(); ++it)
        {
            std::printf("        %-18s: ", it->first.c_str());
            dump_value_list(it->second);
            std::printf(";\n");
        }
    }

    // header (child 0 of root)
    std::vector<const char*>                            header_class;
    std::vector<style_target_attribute>                 header_attr;
    style_target_view                                   header_view;

    example_app::build_view(header,
                            nullptr,
                            0u,
                            root.children.size(),
                            header_class,
                            header_attr,
                            header_view);

    header_view.parent = &root_view;

    {
        resolved_property_map props;

        engine.resolve(header_view, opt, props);

        std::printf("  header.toolbar\n");

        resolved_property_map::const_iterator it;

        for (it = props.begin(); it != props.end(); ++it)
        {
            std::printf("        %-18s: ", it->first.c_str());
            dump_value_list(it->second);
            std::printf(";\n");
        }
    }

    // btn_primary (child 0 of header)
    resolve_and_print(btn_primary,
                      nullptr,
                      0u,
                      header.children.size(),
                      &header,
                      &header_view,
                      nullptr,
                      engine,
                      opt);

    // btn_secondary (child 1 of header) -- with prev_sibling link
    std::vector<const char*>                            primary_class;
    std::vector<style_target_attribute>                 primary_attr;
    style_target_view                                   primary_view;

    example_app::build_view(btn_primary,
                            nullptr,
                            0u,
                            header.children.size(),
                            primary_class,
                            primary_attr,
                            primary_view);
    primary_view.parent = &header_view;

    resolve_and_print(btn_secondary,
                      &btn_primary,
                      1u,
                      header.children.size(),
                      &header,
                      &header_view,
                      &primary_view,
                      engine,
                      opt);

    // panel#content (child 1 of root)
    std::vector<const char*>                            content_class;
    std::vector<style_target_attribute>                 content_attr;
    style_target_view                                   content_view;

    example_app::build_view(content,
                            nullptr,
                            1u,
                            root.children.size(),
                            content_class,
                            content_attr,
                            content_view);

    content_view.parent       = &root_view;
    content_view.prev_sibling = &header_view;

    {
        resolved_property_map props;

        engine.resolve(content_view, opt, props);

        std::printf("  panel#content\n");

        resolved_property_map::const_iterator it;

        for (it = props.begin(); it != props.end(); ++it)
        {
            std::printf("        %-18s: ", it->first.c_str());
            dump_value_list(it->second);
            std::printf(";\n");
        }
    }

    // label.title (child 0 of content) -- this is where the
    // panel#content > label.title rule fires.
    resolve_and_print(title,
                      nullptr,
                      0u,
                      content.children.size(),
                      &content,
                      &content_view,
                      nullptr,
                      engine,
                      opt);

    // input[type="number"] (child 1 of content) -- prev sibling is
    // label.title, exercising the `label.title + input` rule.
    std::vector<const char*>                            title_class;
    std::vector<style_target_attribute>                 title_attr;
    style_target_view                                   title_view;

    example_app::build_view(title,
                            nullptr,
                            0u,
                            content.children.size(),
                            title_class,
                            title_attr,
                            title_view);
    title_view.parent = &content_view;

    resolve_and_print(number,
                      &title,
                      1u,
                      content.children.size(),
                      &content,
                      &content_view,
                      &title_view,
                      engine,
                      opt);

    std::printf("\n");
    std::printf("done.\n");

    return 0;
}
