/*******************************************************************************
* djinterp [test]                                           less_tests_sheet.cpp
*
*   The groups for css/less/less.hpp and css/less/less_template.hpp, on the
* default backend: the vocabulary, a stylesheet built through the facades and
* read back through them, and its Less source. The freestanding factories
* (make_variable and the rest), and adopt, are less_tests_detached.cpp's.
*
*
* path:      /tests/djinterp/cpp/text/less_tests_sheet.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.18
*******************************************************************************/
#include "less_tests.hpp"  // D_TESTS_TEXT_CHECK, the groups
// std
#include <string>  // std::string
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include <type_traits>  // std::is_base_of
#endif
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"  // framework root
#include "../../../../inc/djinterp/cpp/text/css/less/less_template.hpp"
                                                      // the stylesheet


#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// less_sheet
//   the default backend's stylesheet, shortly.
typedef djinterp::less::DLessStylesheet<djinterp::less::DLessDefaultBackend>
    less_sheet;

static_assert(djinterp::less::is_less_mixin_selector(".m()") &&
                  !djinterp::less::is_less_mixin_selector(".plain"),
              "a mixin selector has a parameter list");


/*
d_tests_less_runtime
  A name that the compiler can't see through.
*/
static const char*
d_tests_less_runtime(
    const char* _name
)
{
    const char* volatile name = _name;

    return name;
}


/*
d_tests_less_vocabulary
  Tests the following:
  - categories: mixins, guards, imports, namespaces, grouping
  - the at-keywords both ways, in any case
  - a variable is @ and a name, a property variable $ and a name
  - a mixin selector starts with . or # and has a parameter list; a plain
    class selector is not one
  - the backend tags derive from DLessBackendTag
*/
void
d_tests_less_vocabulary(void)
{
    using namespace djinterp::less;

    D_TESTS_TEXT_CHECK(is_less_mixin_kind(DLessRuleKind::mixin_call) &&
                       is_less_mixin_kind(DLessRuleKind::guarded_mixin));
    D_TESTS_TEXT_CHECK(is_less_guarded_kind(DLessRuleKind::guarded_mixin) &&
                       !is_less_guarded_kind(DLessRuleKind::mixin_call));
    D_TESTS_TEXT_CHECK(is_less_import_kind(DLessRuleKind::plugin_import));
    D_TESTS_TEXT_CHECK(is_less_namespace_kind(
                           DLessRuleKind::namespace_access));
    D_TESTS_TEXT_CHECK(is_less_grouping_kind(
                           DLessRuleKind::detached_ruleset_declaration) &&
                       !is_less_grouping_kind(
                           DLessRuleKind::detached_ruleset_call));
    D_TESTS_TEXT_CHECK(less_category_for_rule_kind(DLessRuleKind::unknown) ==
                       lrc_none);

    D_TESTS_TEXT_CHECK(less_at_rule_kind_from_name(
                           d_tests_less_runtime("plugin")) ==
                       DLessAtRuleKind::plugin);
    D_TESTS_TEXT_CHECK(less_at_rule_kind_from_name(
                           d_tests_less_runtime("IMPORT")) ==
                       DLessAtRuleKind::import_);
    D_TESTS_TEXT_CHECK(less_at_rule_kind_from_name(
                           name_from_less_at_rule_kind(
                               DLessAtRuleKind::options)) ==
                       DLessAtRuleKind::options);
    D_TESTS_TEXT_CHECK(less_at_rule_kind_from_name(nullptr) ==
                       DLessAtRuleKind::unknown);

    D_TESTS_TEXT_CHECK(is_less_variable_name(d_tests_less_runtime("@x")) &&
                       !is_less_variable_name(d_tests_less_runtime("@")));
    D_TESTS_TEXT_CHECK(is_less_property_variable_name("$w") &&
                       !is_less_property_variable_name("$"));
    D_TESTS_TEXT_CHECK(is_less_mixin_selector(
                           d_tests_less_runtime("#ns.m(@a)")));
    D_TESTS_TEXT_CHECK(!is_less_mixin_selector(d_tests_less_runtime(".a")) &&
                       !is_less_mixin_selector(d_tests_less_runtime("m()")));
    D_TESTS_TEXT_CHECK((std::is_base_of<DLessBackendTag,
                                        DLessLessjsBackendTag>::value));

    return;
}


/*
d_tests_less_sheet_build
  Tests the following:
  - a variable gains its @, and knows its stylesheet as its parent
  - a mixin's selector, parameters -- up to the parenthesis that matches,
    not into its guard -- and guard; a mixin without parameters gets ()
  - an import's options and URL, and a plugin's URL, read back with their
    quotes and backslashes unescaped
*/
void
d_tests_less_sheet_build(void)
{
    using namespace djinterp::less;

    less_sheet sheet;
    auto       var   = sheet.add_variable("primary", "#333");
    auto       mixin = sheet.add_mixin(".bordered", "@c: rgb(0, 0, 0)",
                                       "iscolor(@c)");
    auto       plain = sheet.add_mixin(".plain");

    (void)sheet.add_import("it's.less", lio_reference | lio_optional);
    (void)sheet.add_plugin("my\\plugin");

    D_TESTS_TEXT_CHECK(var.variable_name() == "@primary");
    D_TESTS_TEXT_CHECK(var.backend_handle()->parent ==
                       sheet.root_rule().backend_handle());
    D_TESTS_TEXT_CHECK(mixin.mixin_selector() == ".bordered");
    D_TESTS_TEXT_CHECK(mixin.parameters() == "@c: rgb(0, 0, 0)");
    D_TESTS_TEXT_CHECK((mixin.guard_clause() == "iscolor(@c)") &&
                       mixin.is_guarded());
    D_TESTS_TEXT_CHECK((plain.selector() == ".plain()") &&
                       plain.parameters().empty() && !plain.is_guarded());
    D_TESTS_TEXT_CHECK(sheet.less_rule_at(3).import_options() ==
                       (lio_reference | lio_optional));
    D_TESTS_TEXT_CHECK(sheet.less_rule_at(3).import_url() == "it's.less");
    D_TESTS_TEXT_CHECK(sheet.less_rule_at(4).plugin_url() == "my\\plugin");

    return;
}


/*
d_tests_less_sheet_render
  Tests the following:
  - variables, mixins with their guards, and declarations with
    !important
  - an import's and a plugin's URL quoted, with its quote and backslash
    escaped
  - a comment that can't end early; a keyframe written as its selector
    and block; an empty @layer as a statement; nested rules indented
*/
void
d_tests_less_sheet_render(void)
{
    using namespace djinterp::less;
    using djinterp::css::DCssRuleKind;

    less_sheet sheet;

    (void)sheet.add_variable("@w", "1px");
    sheet.add_mixin(".m", "@a", "@a > 0").add_declaration("width", "@a",
                                                           true);
    (void)sheet.add_import("it's.less", lio_css);
    (void)sheet.add_plugin("a\\b");

    auto comment = sheet.root_rule().add_rule(DCssRuleKind::comment);
    auto frames  = sheet.add_at_rule("keyframes", "k");
    auto frame   = frames.add_keyframe("to");
    auto outer   = sheet.add_style_rule(".a");

    comment.set_comment_text("x */ y");
    frame.add_declaration("opacity", "1");
    (void)sheet.add_at_rule("layer", "base");
    outer.add_rule(DCssRuleKind::style_rule).set_selector("&:hover");

    D_TESTS_TEXT_CHECK(d_tests_text_same(
        sheet.render_to_less_source().c_str(),
        "@w: 1px;\n"
        ".m(@a) when (@a > 0) {\n  width: @a !important;\n}\n"
        "@import (css) 'it\\'s.less';\n"
        "@plugin 'a\\\\b';\n"
        "/*x * / y*/\n"
        "@keyframes k {\n  to {\n    opacity: 1;\n  }\n}\n"
        ".a {\n  &:hover {\n  }\n}\n"
        "@layer base;\n"));

    return;
}

#endif  // C++17
