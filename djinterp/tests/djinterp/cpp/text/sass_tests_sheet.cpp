/*******************************************************************************
* djinterp [test]                                           sass_tests_sheet.cpp
*
*   The groups for css/sass/sass.hpp, css/sass/sass_template.hpp and
* css/sass/sass_printer.hpp, on the default backend: the vocabulary, a
* stylesheet built through the facades and read back through them, its SCSS
* and indented sources, and the printer. The freestanding factories
* (make_variable and the rest) are not called: each allocates a rule that
* nothing can adopt, so each leaks, until that is settled.
*
*
* path:      /tests/djinterp/cpp/text/sass_tests_sheet.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/
#include "sass_tests.hpp"  // D_TESTS_TEXT_CHECK, the groups
// std
#include <cstddef>  // std::size_t
#include <sstream>  // std::ostringstream
#include <string>   // std::string
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"  // framework root
#include "../../../../inc/djinterp/cpp/text/css/sass/sass_template.hpp"
                                                      // the stylesheet
#include "../../../../inc/djinterp/cpp/text/css/sass/sass_printer.hpp"
                                                      // the printer


#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// sass_sheet
//   the default backend's stylesheet, shortly.
typedef djinterp::sass::DSassStylesheet<djinterp::sass::DSassDefaultBackend>
    sass_sheet;


/*
d_tests_sass_sample
  A stylesheet with something of everything the sources write.
*/
static void
d_tests_sass_sample(
    sass_sheet& _sheet
)
{
    using djinterp::css::DCssRuleKind;

    (void)_sheet.add_variable("w", "1px", true);
    (void)_sheet.add_use("my 'lib'", "lib");
    (void)_sheet.add_mixin("empty");
    (void)_sheet.add_include("m", "red");

    auto comment = _sheet.root_rule().add_rule(DCssRuleKind::comment);
    auto rule    = _sheet.add_style_rule(".a");
    auto frame   = _sheet.add_at_rule("keyframes", "k")
                         .add_rule(DCssRuleKind::keyframe_rule);

    comment.backend_handle()->comment_text = "a */ b\nsecond";
    rule.add_declaration("color", "red", true);
    frame.set_selector("to");
    frame.add_declaration("opacity", "1");

    return;
}


/*
d_tests_sass_vocabulary
  Tests the following:
  - categories: control flow, modules, diagnostics, grouping
  - every Sass at-keyword reads back as its kind, which gives its rule kind
  - a variable is $ and a name, a placeholder % and a name
*/
void
d_tests_sass_vocabulary(void)
{
    using namespace djinterp::sass;

    bool round_trip = true;

    for (int i = static_cast<int>(DSassAtRuleKind::mixin);
         i <= static_cast<int>(DSassAtRuleKind::import_legacy);
         ++i)
    {
        const DSassAtRuleKind kind = static_cast<DSassAtRuleKind>(i);

        round_trip = round_trip &&
                     (sass_at_rule_kind_from_name(
                          name_from_sass_at_rule_kind(kind)) == kind) &&
                     (sass_rule_kind_from_at_rule_kind(kind) !=
                      DSassRuleKind::unknown);
    }

    D_TESTS_TEXT_CHECK(round_trip);
    D_TESTS_TEXT_CHECK(is_sass_control_flow_kind(DSassRuleKind::each_statement)
                       && !is_sass_control_flow_kind(
                              DSassRuleKind::include_statement));
    D_TESTS_TEXT_CHECK(is_sass_module_kind(DSassRuleKind::forward_rule));
    D_TESTS_TEXT_CHECK(is_sass_diagnostic_kind(DSassRuleKind::warn_statement));
    D_TESTS_TEXT_CHECK(sass_at_rule_kind_from_name("nope") ==
                       DSassAtRuleKind::unknown);
    D_TESTS_TEXT_CHECK(is_sass_variable_name("$x") &&
                       !is_sass_variable_name("$") &&
                       !is_sass_variable_name("x"));
    D_TESTS_TEXT_CHECK(is_sass_placeholder_name("%p") &&
                       !is_sass_placeholder_name("%"));

    return;
}


/*
d_tests_sass_sheet_build
  Tests the following:
  - a variable gains its $, knows its stylesheet as its parent, and is a
    variable declaration
  - a mixin's name and parameters, up to the parenthesis that matches
  - an include's target and arguments; an extend's target and !optional
  - a module's URL read back whole, with its quotes unescaped
  - control flow's condition and loop variable
*/
void
d_tests_sass_sheet_build(void)
{
    sass_sheet sheet;
    auto       var   = sheet.add_variable("w", "1px");
    auto       mixin = sheet.add_mixin("m", "$c: rgb(0, 0, 0)");
    auto       inc   = sheet.add_include("m", "red");
    auto       ext   = sheet.add_extend("%p", true);
    auto       use   = sheet.add_use("my 'lib'", "lib");
    auto       each  = sheet.add_each("$x", "a b c");

    D_TESTS_TEXT_CHECK((var.variable_name() == "$w") &&
                       var.is_variable_declaration());
    D_TESTS_TEXT_CHECK(var.backend_handle()->parent ==
                       sheet.root_rule().backend_handle());
    D_TESTS_TEXT_CHECK((mixin.mixin_name() == "m") &&
                       mixin.is_mixin_declaration());
    D_TESTS_TEXT_CHECK(mixin.parameters() == "$c: rgb(0, 0, 0)");
    D_TESTS_TEXT_CHECK((inc.include_target() == "m") &&
                       (inc.arguments() == "red"));
    D_TESTS_TEXT_CHECK((ext.extend_target() == "%p") &&
                       ext.extend_optional());
    D_TESTS_TEXT_CHECK(use.is_use_rule() &&
                       (use.module_url() == "my 'lib'"));
    D_TESTS_TEXT_CHECK(each.is_each_statement() &&
                       (each.loop_variable() == "$x"));

    return;
}


/*
d_tests_sass_sheet_scss
  Tests the following:
  - a variable's !default, a property's !important
  - a module's URL quoted, its quotes escaped
  - an empty @mixin keeps its block, which Sass requires; an @include is
    a statement
  - a comment that can't end early; a keyframe as its selector and block
*/
void
d_tests_sass_sheet_scss(void)
{
    sass_sheet sheet;

    d_tests_sass_sample(sheet);

    D_TESTS_TEXT_CHECK(d_tests_text_same(
        sheet.render_to_scss_source().c_str(),
        "$w: 1px !default;\n"
        "@use 'my \\'lib\\'' as lib;\n"
        "@mixin empty {\n}\n"
        "@include m(red);\n"
        "/*a * / b\nsecond*/\n"
        ".a {\n  color: red !important;\n}\n"
        "@keyframes k {\n  to {\n    opacity: 1;\n  }\n}\n"));

    return;
}


/*
d_tests_sass_sheet_indented
  Tests the following:
  - no braces and no semicolons; nesting by indentation
  - a comment's later lines a level deeper, so they stay in it
  - a keyframe as a nested rule
*/
void
d_tests_sass_sheet_indented(void)
{
    sass_sheet sheet;

    d_tests_sass_sample(sheet);

    D_TESTS_TEXT_CHECK(d_tests_text_same(
        sheet.render_to_sass_source().c_str(),
        "$w: 1px !default\n"
        "@use 'my \\'lib\\'' as lib\n"
        "@mixin empty\n"
        "@include m(red)\n"
        "// a */ b\n  second\n"
        ".a\n  color: red !important\n"
        "@keyframes k\n  to\n    opacity: 1\n"));

    return;
}


/*
d_tests_sass_printer
  Tests the following:
  - a printer's sink receives the source in the printer's mode
  - sass_to_string and sass_to_stream give the same text as the
    stylesheet, in each mode
*/
void
d_tests_sass_printer(void)
{
    using namespace djinterp::sass;

    sass_sheet         sheet;
    std::string        sunk;
    std::ostringstream stream;
    std::ostringstream compiled;

    d_tests_sass_sample(sheet);

    auto printer = make_sass_printer(
        [&sunk](const char* _data, std::size_t _size)
        {
            sunk.append(_data, _size);
        },
        DSassRenderMode::indented);

    printer.print(sheet);
    D_TESTS_TEXT_CHECK(sunk == sheet.render_to_sass_source());
    D_TESTS_TEXT_CHECK(sass_to_string(sheet) ==
                       sheet.render_to_scss_source());
    sheet.compile_to_css(compiled);
    D_TESTS_TEXT_CHECK(sass_to_string(sheet, DSassRenderMode::compiled_css) ==
                       compiled.str());
    sass_to_stream(stream, sheet, DSassRenderMode::indented);
    D_TESTS_TEXT_CHECK(stream.str() == sheet.render_to_sass_source());

    return;
}

#endif  // C++17
