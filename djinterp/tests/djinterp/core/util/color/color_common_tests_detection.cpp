/*******************************************************************************
* djinterp [core]                               color_common_tests_detection.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_common_tests_detection.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_common_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_common_is_color_model_accepts_tagged
  Verifies a type exposing a nested model_tag is detected as a color model.
  Tests the following:
  - a minimal tagged fixture is accepted
  - acceptance does not depend on the type having channels or operations
*/
bool
tests_color_common_is_color_model_accepts_tagged()
{
    D_CC_CHECK(is_color_model<cc_tagged>::value);
    D_CC_CHECK(std::is_same<cc_tagged::model_tag, rgb_tag>::value);
    D_CC_CHECK(is_color_model<cc_tagged>::value == true);

    return true;
}

/*
tests_color_common_is_color_model_rejects_untagged
  Verifies a transport-shaped type with channels but no model_tag is not a
  color model - the shape rgba deliberately has, per the color README.
  Tests the following:
  - the untagged fixture is rejected
  - rejection is by absence of the nested name, not by field layout
*/
bool
tests_color_common_is_color_model_rejects_untagged()
{
    D_CC_CHECK(!(is_color_model<cc_untagged>::value));
    D_CC_CHECK(is_color_model<cc_untagged>::value == false);

    // the fixture really does carry channels; only the tag is missing
    cc_untagged c;
    c.r = channel_t(1);
    c.g = channel_t(0);
    c.b = channel_t(0);
    c.a = channel_t(1);
    D_CC_CHECK(c.r == channel_t(1));

    return true;
}

/*
tests_color_common_is_color_model_rejects_scalars_and_void
  Verifies scalars, pointers, arrays, function types and void are rejected
  without a hard error - the probe must remain SFINAE-friendly.
  Tests the following:
  - fundamental, pointer, array, function and void types all read false
*/
bool
tests_color_common_is_color_model_rejects_scalars_and_void()
{
    D_CC_CHECK(!(is_color_model<int>::value));
    D_CC_CHECK(!(is_color_model<float>::value));
    D_CC_CHECK(!(is_color_model<channel_t>::value));
    D_CC_CHECK(!(is_color_model<bool>::value));
    D_CC_CHECK(!(is_color_model<void>::value));
    D_CC_CHECK(!(is_color_model<void*>::value));
    D_CC_CHECK(!(is_color_model<cc_tagged*>::value));
    D_CC_CHECK(!(is_color_model<int[3]>::value));
    D_CC_CHECK(!(is_color_model<channel_t[4]>::value));
    D_CC_CHECK(!(is_color_model<void()>::value));
    D_CC_CHECK(!(is_color_model<std::nullptr_t>::value));

    return true;
}

/*
tests_color_common_is_color_model_strips_cv_and_reference
  Verifies const, volatile and reference qualifiers are stripped before
  detection, so a model stays a model however it is passed.
  Tests the following:
  - const / volatile / const volatile qualified models are accepted
  - lvalue and rvalue references to models are accepted
  - qualified NON-models stay rejected (stripping does not create tags)
*/
bool
tests_color_common_is_color_model_strips_cv_and_reference()
{
    D_CC_CHECK(is_color_model<const cc_tagged>::value);
    D_CC_CHECK(is_color_model<volatile cc_tagged>::value);
    D_CC_CHECK(is_color_model<const volatile cc_tagged>::value);
    D_CC_CHECK(is_color_model<cc_tagged&>::value);
    D_CC_CHECK(is_color_model<const cc_tagged&>::value);
    D_CC_CHECK(is_color_model<cc_tagged&&>::value);
    D_CC_CHECK(is_color_model<const volatile cc_tagged&>::value);

    D_CC_CHECK(!(is_color_model<const cc_untagged>::value));
    D_CC_CHECK(!(is_color_model<cc_untagged&>::value));
    D_CC_CHECK(!(is_color_model<const int&>::value));

    return true;
}

/*
tests_color_common_is_color_model_is_structural_not_semantic
  Verifies detection probes only for the nested name: a model_tag of unrelated
  type still qualifies. This pins the trait's contract - membership of the
  conversion graph is advertised, not proven.
  Tests the following:
  - a type whose model_tag is `int` is still detected as a model
  - the trait makes no claim that the tag derives from color_model_tag
*/
bool
tests_color_common_is_color_model_is_structural_not_semantic()
{
    D_CC_CHECK(is_color_model<cc_foreign_tag>::value);
    D_CC_CHECK(std::is_same<cc_foreign_tag::model_tag, int>::value);
    D_CC_CHECK(!(std::is_base_of<color_model_tag,
                                 cc_foreign_tag::model_tag>::value));

    // the stricter question - does the tag belong to the hierarchy? - is a
    // separate predicate the foundation deliberately does not fold in
    D_CC_CHECK(std::is_base_of<color_model_tag, cc_tagged::model_tag>::value);

    return true;
}

/*
tests_color_common_is_color_model_respects_access_control
  Verifies a private model_tag fails detection by SFINAE rather than producing
  an access error - access checking is part of deduction since C++11.
  Tests the following:
  - the private-tag fixture reads false
  - the public-tag fixture reads true, isolating access as the difference
*/
bool
tests_color_common_is_color_model_respects_access_control()
{
    D_CC_CHECK(!(is_color_model<cc_private_tag>::value));
    D_CC_CHECK(is_color_model<cc_tagged>::value);
    D_CC_CHECK(!(is_color_model<const cc_private_tag&>::value));

    return true;
}

/*
tests_color_common_is_color_model_ignores_data_members
  Verifies a model_tag DATA member does not satisfy the trait: the probe
  requires a nested TYPE, so a same-named field must not be mistaken for one.
  Tests the following:
  - the data-member fixture reads false
  - the member itself is nonetheless present and usable
*/
bool
tests_color_common_is_color_model_ignores_data_members()
{
    D_CC_CHECK(!(is_color_model<cc_tag_member>::value));

    cc_tag_member m;
    m.model_tag = 7;
    D_CC_CHECK(m.model_tag == 7);

    return true;
}

/*
tests_color_common_is_color_model_trait_shape
  Verifies is_color_model presents the standard bool-trait shape, so it
  composes with the rest of the type-trait vocabulary.
  Tests the following:
  - it derives from integral_constant<bool, V>
  - value_type is bool and type is the matching true_type / false_type
  - the implicit conversion and the call operator agree with ::value
*/
bool
tests_color_common_is_color_model_trait_shape()
{
    using yes = is_color_model<cc_tagged>;
    using no  = is_color_model<cc_untagged>;

    D_CC_CHECK(std::is_base_of<std::integral_constant<bool, true>,
                               yes>::value);
    D_CC_CHECK(std::is_base_of<std::integral_constant<bool, false>,
                               no>::value);

    D_CC_CHECK(std::is_same<yes::value_type, bool>::value);
    D_CC_CHECK(std::is_same<no::value_type, bool>::value);
    D_CC_CHECK(std::is_same<yes::type, std::true_type>::value);
    D_CC_CHECK(std::is_same<no::type, std::false_type>::value);

    // implicit conversion and call operator
    D_CC_CHECK(static_cast<bool>(yes()) == true);
    D_CC_CHECK(static_cast<bool>(no()) == false);
    D_CC_CHECK(yes()() == true);
    D_CC_CHECK(no()() == false);

    // value is a usable constant expression
    static_assert(yes::value, "tagged fixture must be a color model");
    static_assert(!no::value, "untagged fixture must not be a color model");

    return true;
}

/*
tests_color_common_is_color_model_v_matches_trait
  Verifies the C++14 variable-template shorthand agrees with the class trait
  for every probe. The shorthand is gated on the language level, so the body
  degrades to a no-op assertion below C++14.
  Tests the following:
  - _v equals ::value for tagged, untagged, qualified and scalar probes
*/
bool
tests_color_common_is_color_model_v_matches_trait()
{
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    D_CC_CHECK(is_color_model_v<cc_tagged> == is_color_model<cc_tagged>::value);
    D_CC_CHECK(is_color_model_v<cc_untagged>
               == is_color_model<cc_untagged>::value);
    D_CC_CHECK(is_color_model_v<const cc_tagged&>
               == is_color_model<const cc_tagged&>::value);
    D_CC_CHECK(is_color_model_v<int> == is_color_model<int>::value);
    D_CC_CHECK(is_color_model_v<cc_tagged>);
    D_CC_CHECK(!is_color_model_v<cc_untagged>);

    static_assert(is_color_model_v<cc_tagged>,
                  "is_color_model_v must be a constant expression");
#else
    D_CC_CHECK(is_color_model<cc_tagged>::value);
#endif

    return true;
}

/*
tests_color_common_has_model_tag_helper_direct
  Verifies the internal has_model_tag_helper selects its specialization
  exactly when a nested model_tag exists, driving is_color_model from below.
  Tests the following:
  - the helper reads true for tagged types and false for untagged ones
  - the helper does NOT strip cv-ref (that is is_color_model's job)
*/
bool
tests_color_common_has_model_tag_helper_direct()
{
    D_CC_CHECK(internal::has_model_tag_helper<cc_tagged>::value);
    D_CC_CHECK(internal::has_model_tag_helper<cc_foreign_tag>::value);
    D_CC_CHECK(!(internal::has_model_tag_helper<cc_untagged>::value));
    D_CC_CHECK(!(internal::has_model_tag_helper<cc_private_tag>::value));
    D_CC_CHECK(!(internal::has_model_tag_helper<cc_tag_member>::value));
    D_CC_CHECK(!(internal::has_model_tag_helper<int>::value));

    // the helper is the raw probe: a reference has no members of its own, so
    // it falls to the primary.  is_color_model is what normalises first.
    D_CC_CHECK(!(internal::has_model_tag_helper<cc_tagged&>::value));
    D_CC_CHECK(is_color_model<cc_tagged&>::value);

    return true;
}

/*
tests_color_common_detail_void_maps_to_void
  Verifies internal::color_detail_void maps any well-formed type to void - the
  C++11-clean void_t the detection partial specialization matches against.
  Tests the following:
  - ::type is void for class, scalar, reference and cv-qualified arguments
  - the alias agrees with the framework's own void_t
*/
bool
tests_color_common_detail_void_maps_to_void()
{
    D_CC_CHECK(std::is_same<internal::color_detail_void<int>::type,
                            void>::value);
    D_CC_CHECK(std::is_same<internal::color_detail_void<cc_tagged>::type,
                            void>::value);
    D_CC_CHECK(std::is_same<internal::color_detail_void<const cc_tagged&>::type,
                            void>::value);
    D_CC_CHECK(std::is_same<internal::color_detail_void<void>::type,
                            void>::value);
    D_CC_CHECK(std::is_same<internal::color_detail_void<rgb_tag>::type,
                            void>::value);

    // this is exactly the sink the probe uses on a real nested tag
    D_CC_CHECK(std::is_same<
                   internal::color_detail_void<cc_tagged::model_tag>::type,
                   void>::value);

    return true;
}

NS_END
NS_END
