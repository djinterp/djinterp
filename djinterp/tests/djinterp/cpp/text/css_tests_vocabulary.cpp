/*******************************************************************************
* djinterp [cpp][text]                                  css_tests_vocabulary.cpp
*
*   The groups for css/css.hpp: the rule kinds' category masks and the
* predicates over them, every at-rule's keyword both ways - in any case, as
* CSS compares keywords - and its rule kind, custom property names, the name
* constants, and backend detection. The keyword lookups run at compile time
* and at run time: a recursive comparison forced inline once made GCC refuse
* every run-time call outside testing.
*
*
* path:      /tests/djinterp/cpp/text/css_tests_vocabulary.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/
#include "css_tests.hpp"  // D_TESTS_TEXT_CHECK, the groups
// std
#include <cstring>  // std::strcmp
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include <type_traits>  // std::is_base_of
#endif
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"  // framework root
#include "../../../../inc/djinterp/cpp/text/css/css.hpp"  // the vocabulary


#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// css_rule, css_at
//   the kinds, shortly.
typedef djinterp::css::DCssRuleKind   css_rule;
typedef djinterp::css::DCssAtRuleKind css_at;

static_assert(djinterp::css::at_rule_kind_from_name("FONT-FACE") ==
                  css_at::font_face,
              "keywords are compared ignoring case");
static_assert(!djinterp::css::is_custom_property_name("--"),
              "-- alone is reserved");


/*
d_tests_css_runtime
  A keyword that the compiler can't see through.
*/
static const char*
d_tests_css_runtime(
    const char* _name
)
{
    const char* volatile name = _name;

    return name;
}


/*
d_tests_css_rules
  Tests the following:
  - at-rules are at-rules, style rules and keyframes are not
  - conditional: @media, @supports, @container, @scope, @document and
    @starting-style; not @keyframes
  - grouping, descriptor, nestable and keyframes, each on its kinds
  - a declaration block is only that; unknown has no category
*/
void
d_tests_css_rules(void)
{
    using namespace djinterp::css;

    D_TESTS_TEXT_CHECK(is_at_rule_kind(css_rule::media_rule) &&
                       is_at_rule_kind(css_rule::at_rule));
    D_TESTS_TEXT_CHECK(!is_at_rule_kind(css_rule::style_rule) &&
                       !is_at_rule_kind(css_rule::keyframe_rule));
    D_TESTS_TEXT_CHECK(is_conditional_rule_kind(css_rule::supports_rule) &&
                       is_conditional_rule_kind(css_rule::container_rule) &&
                       is_conditional_rule_kind(
                           css_rule::starting_style_rule));
    D_TESTS_TEXT_CHECK(!is_conditional_rule_kind(css_rule::keyframes_rule));
    D_TESTS_TEXT_CHECK(is_grouping_rule_kind(css_rule::stylesheet) &&
                       is_grouping_rule_kind(css_rule::layer_rule) &&
                       !is_grouping_rule_kind(css_rule::font_face_rule));
    D_TESTS_TEXT_CHECK(is_descriptor_rule_kind(css_rule::font_face_rule) &&
                       is_descriptor_rule_kind(css_rule::keyframe_rule) &&
                       is_descriptor_rule_kind(css_rule::property_rule));
    D_TESTS_TEXT_CHECK(is_nestable_rule_kind(css_rule::style_rule) &&
                       is_nestable_rule_kind(css_rule::media_rule) &&
                       !is_nestable_rule_kind(css_rule::keyframes_rule));
    D_TESTS_TEXT_CHECK(is_keyframes_rule_kind(css_rule::keyframes_rule) &&
                       is_keyframes_rule_kind(css_rule::keyframe_rule));
    D_TESTS_TEXT_CHECK(css_category_for_rule_kind(
                           css_rule::declaration_block) == crc_declaration);
    D_TESTS_TEXT_CHECK(css_category_for_rule_kind(css_rule::unknown) ==
                       crc_none);

    return;
}


/*
d_tests_css_at_rules
  Tests the following:
  - each of the 18 named at-rules' keyword reads back as its kind, at run
    time, and its kind gives the dedicated rule kind - but @apply, which
    has none
  - a keyword in another case is the same at-rule
  - an unknown keyword and null are unknown, and unknown and custom have
    no keyword
*/
void
d_tests_css_at_rules(void)
{
    using namespace djinterp::css;

    bool round_trip = true;
    bool own_rule   = true;

    for (int i = static_cast<int>(css_at::media);
         i <= static_cast<int>(css_at::apply);
         ++i)
    {
        const css_at      kind = static_cast<css_at>(i);
        const char* const name = name_from_at_rule_kind(kind);

        round_trip = round_trip && (name[0] != '\0') &&
                     (at_rule_kind_from_name(d_tests_css_runtime(name)) ==
                      kind);
        own_rule   = own_rule &&
                     ( (kind == css_at::apply) ==
                       (rule_kind_from_at_rule_kind(kind) ==
                        css_rule::at_rule) );
    }

    D_TESTS_TEXT_CHECK(round_trip);
    D_TESTS_TEXT_CHECK(own_rule);
    D_TESTS_TEXT_CHECK(rule_kind_from_at_rule_kind(css_at::media) ==
                       css_rule::media_rule);
    D_TESTS_TEXT_CHECK(at_rule_kind_from_name(d_tests_css_runtime("MEDIA")) ==
                       css_at::media);
    D_TESTS_TEXT_CHECK(at_rule_kind_from_name(
                           d_tests_css_runtime("Starting-Style")) ==
                       css_at::starting_style);
    D_TESTS_TEXT_CHECK(at_rule_kind_from_name(
                           d_tests_css_runtime("-webkit-keyframes")) ==
                       css_at::unknown);
    D_TESTS_TEXT_CHECK(at_rule_kind_from_name(nullptr) == css_at::unknown);
    D_TESTS_TEXT_CHECK((std::strcmp(name_from_at_rule_kind(css_at::custom),
                                    "") == 0) &&
                       (std::strcmp(name_from_at_rule_kind(css_at::unknown),
                                    "") == 0));
    D_TESTS_TEXT_CHECK(rule_kind_from_at_rule_kind(css_at::custom) ==
                       css_rule::at_rule);

    return;
}


// DTestsCssBackend
//   probe: a type with a CSS backend tag.
struct DTestsCssBackend
{
    typedef djinterp::css::DCssLibcssBackendTag css_backend_tag;
};


/*
d_tests_css_names
  Tests the following:
  - a custom property starts with two dashes and has more after them
  - the name constants spell CSS's names
  - is_css_backend finds a nested css_backend_tag; the tags derive from
    DCssBackendTag
*/
void
d_tests_css_names(void)
{
    using namespace djinterp::css;

    D_TESTS_TEXT_CHECK(is_custom_property_name(d_tests_css_runtime("--x")));
    D_TESTS_TEXT_CHECK(!is_custom_property_name(d_tests_css_runtime("--")));
    D_TESTS_TEXT_CHECK(!is_custom_property_name(d_tests_css_runtime("-x")));
    D_TESTS_TEXT_CHECK(!is_custom_property_name(nullptr));
    D_TESTS_TEXT_CHECK(std::strcmp(props::background_color,
                                   "background-color") == 0);
    D_TESTS_TEXT_CHECK(std::strcmp(at_rules::font_face, "font-face") == 0);
    D_TESTS_TEXT_CHECK(is_css_backend<DTestsCssBackend>::value &&
                       is_css_backend<const DTestsCssBackend&>::value);
    D_TESTS_TEXT_CHECK(!is_css_backend<int>::value);
    D_TESTS_TEXT_CHECK((std::is_base_of<DCssBackendTag,
                                        DCssStyloBackendTag>::value));

    return;
}

#endif  // C++17
