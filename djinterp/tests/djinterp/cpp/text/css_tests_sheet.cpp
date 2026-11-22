/*******************************************************************************
* djinterp [cpp][text]                                       css_tests_sheet.cpp
*
*   The groups for css/css_template.hpp, on its default backend: building a
* stylesheet through the facades, and its three renderings -- formatted,
* minified and SCSS -- over style rules, nesting, comments and every form of
* at-rule. Each expected text is written as CSS (and CSS Nesting) has it.
*   The facades' add_style_rule, add_at_rule and add_rule make rules in
* place. The freestanding factories (make_style_rule and the rest), and
* adopt, are css_tests_detached.cpp's.
*
*
* path:      /tests/djinterp/cpp/text/css_tests_sheet.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.18
*******************************************************************************/
#include "css_tests.hpp"  // D_TESTS_TEXT_CHECK, the groups
// std
#include <string>  // std::string
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"  // framework root
#include "../../../../inc/djinterp/cpp/text/css/css_template.hpp"
                                                      // the stylesheet


#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// css_backend, css_sheet, css_rule_facade
//   the default backend's stylesheet, shortly.
typedef djinterp::css::DCssDefaultBackend           css_backend;
typedef djinterp::css::DCssStylesheet<css_backend>  css_sheet;
typedef djinterp::css::DCssRule<css_backend>        css_rule_facade;


/*
d_tests_css_child
  A rule of a kind, in place under a parent, with a selector.
*/
static css_rule_facade
d_tests_css_child(
    css_rule_facade                 _parent,
    djinterp::css::DCssRuleKind     _kind,
    const std::string&              _selector
)
{
    css_rule_facade rule = _parent.add_rule(_kind);

    rule.set_selector(_selector);

    return rule;
}


/*
d_tests_css_sheet_build
  Tests the following:
  - a stylesheet starts empty, in CSS 3's level and plain CSS
  - add_style_rule and add_at_rule append rules, the latter knowing its
    kind from its keyword
  - add_declaration appends; set_property replaces a value, and its
    importance, or appends; find_declaration and remove_declaration
  - a nested rule counts as a rule, not a declaration
*/
void
d_tests_css_sheet_build(void)
{
    using namespace djinterp::css;

    css_sheet sheet;

    D_TESTS_TEXT_CHECK((sheet.rule_count() == 0u) &&
                       (sheet.level() == DCssLevel::css_3) &&
                       (sheet.syntax_mode() == DCssSyntaxMode::css));

    css_rule_facade rule  = sheet.add_style_rule(".a");
    css_rule_facade media = sheet.add_at_rule("media", "print");

    rule.add_declaration("color", "red");
    rule.set_property("color", "blue");
    rule.set_property("margin", "0", true);
    rule.add_declaration("--x", "1");
    (void)d_tests_css_child(rule, DCssRuleKind::style_rule, "&:hover");

    D_TESTS_TEXT_CHECK(sheet.rule_count() == 2u);
    D_TESTS_TEXT_CHECK((media.rule_kind() == DCssRuleKind::media_rule) &&
                       (media.at_rule_kind() == DCssAtRuleKind::media) &&
                       (media.prelude() == "print"));
    D_TESTS_TEXT_CHECK((rule.declaration_count() == 3u) &&
                       (rule.rule_count() == 1u));
    D_TESTS_TEXT_CHECK(rule.find_declaration("color").value() == "blue");
    D_TESTS_TEXT_CHECK(rule.find_declaration("margin").is_important());
    D_TESTS_TEXT_CHECK(rule.declaration_at(2).is_custom_property());
    D_TESTS_TEXT_CHECK(rule.rule_at(0).selector() == "&:hover");
    D_TESTS_TEXT_CHECK(rule.remove_declaration("color") &&
                       !rule.remove_declaration("color"));
    D_TESTS_TEXT_CHECK(!rule.find_declaration("color").valid());
    D_TESTS_TEXT_CHECK(!sheet.rule_at(2).valid());

    return;
}


/*
d_tests_css_sheet_render
  Tests the following:
  - formatted: two spaces a level, a space before a block and after a
    colon, " !important"
  - minified: no whitespace, no comments, "!important"
  - SCSS: as formatted, nesting kept
  - a star and a slash in a comment are written with a space between,
    so the comment can't end early
*/
void
d_tests_css_sheet_render(void)
{
    using namespace djinterp::css;

    css_sheet       sheet;
    css_rule_facade comment = sheet.root_rule().add_rule(DCssRuleKind::comment);
    css_rule_facade rule    = sheet.add_style_rule(".a");

    comment.set_comment_text("a */ b");
    rule.add_declaration("color", "red");
    rule.add_declaration("margin", "0", true);
    d_tests_css_child(rule, DCssRuleKind::style_rule, "&:hover")
        .add_declaration("color", "blue");

    static const char* const formatted =
        "/*a * / b*/\n"
        ".a {\n"
        "  color: red;\n"
        "  margin: 0 !important;\n"
        "  &:hover {\n"
        "    color: blue;\n"
        "  }\n"
        "}\n";

    D_TESTS_TEXT_CHECK(d_tests_text_same(sheet.render_to_css().c_str(),
                                         formatted));
    D_TESTS_TEXT_CHECK(d_tests_text_same(
        sheet.render_to_minified_css().c_str(),
        ".a{color:red;margin:0!important;&:hover{color:blue;}}"));
    D_TESTS_TEXT_CHECK(d_tests_text_same(sheet.render_to_scss().c_str(),
                                         formatted));

    return;
}


/*
d_tests_css_sheet_at_rules
  Tests the following:
  - @import and @charset are statements; an empty @layer is too
  - @media holds rules; @font-face holds descriptors
  - @keyframes holds its keyframes, each written as its selector and block
  - an unknown at-rule keeps its block
*/
void
d_tests_css_sheet_at_rules(void)
{
    using namespace djinterp::css;

    css_sheet       sheet;
    css_rule_facade media;
    css_rule_facade frames;

    (void)sheet.add_at_rule("charset", "\"UTF-8\"");
    (void)sheet.add_at_rule("import", "url(\"a.css\")");
    (void)sheet.add_at_rule("layer", "base, theme");
    media = sheet.add_at_rule("media", "print");
    d_tests_css_child(media, DCssRuleKind::style_rule, "p")
        .add_declaration("color", "black");
    sheet.add_at_rule("font-face").add_declaration("font-family", "X");
    frames = sheet.add_at_rule("keyframes", "spin");
    frames.add_keyframe("from").add_declaration("opacity", "0");
    frames.add_keyframe("to").add_declaration("opacity", "1");
    sheet.add_at_rule("-webkit-keyframes", "x").add_keyframe("50%")
        .add_declaration("opacity", "0.5");

    D_TESTS_TEXT_CHECK(d_tests_text_same(
        sheet.render_to_css().c_str(),
        "@charset \"UTF-8\";\n"
        "@import url(\"a.css\");\n"
        "@layer base, theme;\n"
        "@media print {\n  p {\n    color: black;\n  }\n}\n"
        "@font-face {\n  font-family: X;\n}\n"
        "@keyframes spin {\n"
        "  from {\n    opacity: 0;\n  }\n"
        "  to {\n    opacity: 1;\n  }\n"
        "}\n"
        "@-webkit-keyframes x {\n  50% {\n    opacity: 0.5;\n  }\n}\n"));
    D_TESTS_TEXT_CHECK(d_tests_text_same(
        sheet.render_to_minified_css().c_str(),
        "@charset \"UTF-8\";@import url(\"a.css\");@layer base, theme;"
        "@media print{p{color:black;}}@font-face{font-family:X;}"
        "@keyframes spin{from{opacity:0;}to{opacity:1;}}"
        "@-webkit-keyframes x{50%{opacity:0.5;}}"));

    return;
}

#endif  // C++17
