/*******************************************************************************
* djinterp [cpp][text]                                    css_tests_detached.cpp
*
*   The groups for css/css_template.hpp's ten factories and its two adopt
* functions, a rule's and a stylesheet's, on the default backend, and the
* keyframe and comment accessors beside them. Each
* factory returns its rule in a handle (DCssDetached), which frees it unless
* a rule or a stylesheet adopts it; adoption takes only a handle. A handle
* left unadopted here is freed when it goes, which the suite's runs under
* AddressSanitizer and LeakSanitizer confirm.
*
*
* path:      /tests/djinterp/cpp/text/css_tests_detached.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.18
*                                                            revised: 2026.11.18
*******************************************************************************/
#include "css_tests.hpp"  // D_TESTS_TEXT_CHECK, the groups
// std
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <type_traits>  // std::false_type, std::true_type
    #include <utility>      // std::declval, std::move
#endif
// djinterp
#include "../../../../inc/djinterp/cpp/text/css/css_template.hpp"  // the tree


#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// css_backend, css_sheet, css_rule, css_new
//   the default backend, its stylesheet and rule facades, and the
// factories' handle.
typedef djinterp::css::DCssDefaultBackend          css_backend;
typedef djinterp::css::DCssStylesheet<css_backend> css_sheet;
typedef djinterp::css::DCssRule<css_backend>       css_rule;
typedef djinterp::css::DCssDetached<css_backend>   css_new;

/*
d_tests_css_adopts
  True when Parent's adopt takes a Child: a compile-time check of what
adoption refuses.
*/
template<typename Parent,
         typename Child,
         typename = void>
struct d_tests_css_adopts
:   public std::false_type
{};

template<typename Parent,
         typename Child>
struct d_tests_css_adopts<Parent,
                          Child,
                          decltype((void)std::declval<Parent&>().adopt(
                                       std::declval<Child>()))>
:   public std::true_type
{};


/*
d_tests_css_detached_factories
  Tests the following:
  - each of the ten factories returns a handle owning a parentless rule:
    a style rule with its selector; an at-rule with its keyword, prelude
    and kinds; @media, @supports, @keyframes, @font-face and @import
    through it; @charset's encoding quoted; a comment with its text; a
    keyframe with its selector
  - a comment's text reads back through comment_text, and is set through
    set_comment_text
  - a handle can be built under (a declaration) before it's placed
  - a handle nothing adopts frees its rule
*/
void
d_tests_css_detached_factories(void)
{
    using namespace djinterp::css;

    css_new style   = make_style_rule<css_backend>("p, a");
    css_new at      = make_at_rule<css_backend>("layer", "base");
    css_new media   = make_media_rule<css_backend>("screen");
    css_new support = make_supports_rule<css_backend>("(display: grid)");
    css_new frames  = make_keyframes_rule<css_backend>("spin");
    css_new face    = make_font_face_rule<css_backend>();
    css_new import  = make_import_rule<css_backend>("url(a.css)");
    css_new charset = make_charset_rule<css_backend>("UTF-8");
    css_new note    = make_comment<css_backend>("n");
    css_new frame   = make_keyframe<css_backend>("0%, 100%");

    D_TESTS_TEXT_CHECK(style.valid() &&
                       (style->rule_kind() == DCssRuleKind::style_rule) &&
                       (style->selector() == "p, a") &&
                       (style.backend_handle()->parent == nullptr));
    D_TESTS_TEXT_CHECK((at->at_keyword() == "layer") &&
                       (at->prelude() == "base") &&
                       (at->at_rule_kind() ==
                        at_rule_kind_from_name("layer")));
    D_TESTS_TEXT_CHECK((media->at_keyword() == "media") &&
                       (media->prelude() == "screen") &&
                       (media->at_rule_kind() == DCssAtRuleKind::media));
    D_TESTS_TEXT_CHECK((support->at_keyword() == "supports") &&
                       (frames->at_keyword() == "keyframes") &&
                       (frames->prelude() == "spin") &&
                       (face->at_keyword() == "font-face") &&
                       face->prelude().empty() &&
                       (import->prelude() == "url(a.css)"));
    D_TESTS_TEXT_CHECK((charset->at_keyword() == "charset") &&
                       (charset->prelude() == "\"UTF-8\""));
    D_TESTS_TEXT_CHECK((note->rule_kind() == DCssRuleKind::comment) &&
                       (note->comment_text() == "n"));
    note->set_comment_text("m");
    D_TESTS_TEXT_CHECK((note->comment_text() == "m") &&
                       css_rule().comment_text().empty());
    D_TESTS_TEXT_CHECK((frame->rule_kind() == DCssRuleKind::keyframe_rule) &&
                       (frame->selector() == "0%, 100%"));
    D_TESTS_TEXT_CHECK(style->add_declaration("color", "red").valid() &&
                       (style->declaration_count() == 1u));

    return;
}


/*
d_tests_css_detached_adopt
  Tests the following:
  - a stylesheet's adopt and a rule's append, set the parent, empty the
    handle and return the rule's facade; a rule built under a handle goes
    with it
  - each refuses, with an empty facade and the handle keeping its rule: an
    empty rule, an empty handle, the handle's own rule, and a rule under it
  - a @keyframes rule adopts a keyframe a factory made, and add_keyframe
    makes one in place, after it, with its selector
  - a facade, or a handle not moved, doesn't compile as adopt's argument
*/
void
d_tests_css_detached_adopt(void)
{
    using namespace djinterp::css;

    css_sheet sheet;
    css_new   media = make_media_rule<css_backend>("print");
    css_rule  held  = media.get();
    css_rule  inner = media->adopt(make_style_rule<css_backend>("h1"));
    css_rule  placed;

    D_TESTS_TEXT_CHECK(inner.valid() && (inner.selector() == "h1") &&
                       (inner.backend_handle()->parent ==
                        held.backend_handle()) &&
                       (held.rule_count() == 1u));
    D_TESTS_TEXT_CHECK(!css_rule().adopt(std::move(media)).valid() &&
                       media.valid());
    D_TESTS_TEXT_CHECK(!sheet.adopt(css_new()).valid() &&
                       !held.adopt(css_new()).valid());
    D_TESTS_TEXT_CHECK(!held.adopt(std::move(media)).valid() &&
                       !inner.adopt(std::move(media)).valid() &&
                       media.valid());

    placed = sheet.adopt(std::move(media));
    D_TESTS_TEXT_CHECK(!media.valid() &&
                       (placed.backend_handle() == held.backend_handle()) &&
                       (sheet.rule_count() == 1u) &&
                       (sheet.rule_at(0).rule_count() == 1u));
    D_TESTS_TEXT_CHECK(sheet.adopt(make_comment<css_backend>("c")).valid() &&
                       (sheet.rule_count() == 2u));

    css_rule frames = sheet.adopt(make_keyframes_rule<css_backend>("k"));
    css_rule from   = frames.adopt(make_keyframe<css_backend>("from"));
    css_rule to     = frames.add_keyframe("to");

    D_TESTS_TEXT_CHECK(from.valid() && to.valid() &&
                       (frames.rule_count() == 2u) &&
                       (frames.rule_at(1).backend_handle() ==
                        to.backend_handle()) &&
                       (to.rule_kind() == DCssRuleKind::keyframe_rule) &&
                       (to.selector() == "to"));
    D_TESTS_TEXT_CHECK(!css_rule().add_keyframe("x").valid());

    const bool refused =
        (d_tests_css_adopts<css_sheet, css_new>::value) &&
        (d_tests_css_adopts<css_rule, css_new>::value) &&
        (!d_tests_css_adopts<css_sheet, css_rule>::value) &&
        (!d_tests_css_adopts<css_rule, css_rule>::value) &&
        (!d_tests_css_adopts<css_sheet, css_new&>::value) &&
        (!d_tests_css_adopts<css_rule, css_new&>::value);

    D_TESTS_TEXT_CHECK(refused);

    return;
}

#endif  // C++17
