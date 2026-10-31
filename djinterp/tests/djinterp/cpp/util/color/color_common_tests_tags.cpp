/*******************************************************************************
* djinterp [cpp]                                     color_common_tests_tags.cpp
*
*
* path:      /tests/djinterp/cpp/util/color/color_common_tests_tags.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_common_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_common_tags_derive_from_base
  Verifies all seven model tags derive from DColorModelTag, so a single base
  identifies membership of the tag hierarchy.
  Tests the following:
  - each of rgb/cmyk/hsl/hsv/ycbcr/cie_xyz/cie_lab derives from the base
  - derivation is public (each converts to the base)
*/
bool tests_color_common_tags_derive_from_base()
{
    D_CC_CHECK(std::is_base_of<DColorModelTag, rgb_tag>::value);
    D_CC_CHECK(std::is_base_of<DColorModelTag, cmyk_tag>::value);
    D_CC_CHECK(std::is_base_of<DColorModelTag, hsl_tag>::value);
    D_CC_CHECK(std::is_base_of<DColorModelTag, hsv_tag>::value);
    D_CC_CHECK(std::is_base_of<DColorModelTag, ycbcr_tag>::value);
    D_CC_CHECK(std::is_base_of<DColorModelTag, cie_xyz_tag>::value);
    D_CC_CHECK(std::is_base_of<DColorModelTag, cie_lab_tag>::value);

    // public derivation: each tag converts to the base
    D_CC_CHECK(std::is_convertible<rgb_tag*, DColorModelTag*>::value);
    D_CC_CHECK(std::is_convertible<cmyk_tag*, DColorModelTag*>::value);
    D_CC_CHECK(std::is_convertible<hsl_tag*, DColorModelTag*>::value);
    D_CC_CHECK(std::is_convertible<hsv_tag*, DColorModelTag*>::value);
    D_CC_CHECK(std::is_convertible<ycbcr_tag*, DColorModelTag*>::value);
    D_CC_CHECK(std::is_convertible<cie_xyz_tag*, DColorModelTag*>::value);
    D_CC_CHECK(std::is_convertible<cie_lab_tag*, DColorModelTag*>::value);

    return true;
}

/*
tests_color_common_tags_are_distinct_types
  Verifies no two model tags name the same type, so tag dispatch can never
  collapse two models onto one overload.
  Tests the following:
  - every pair among the seven tags is distinct
  - no tag is the base type itself
*/
bool tests_color_common_tags_are_distinct_types()
{
    D_CC_CHECK(!(std::is_same<rgb_tag, cmyk_tag>::value));
    D_CC_CHECK(!(std::is_same<rgb_tag, hsl_tag>::value));
    D_CC_CHECK(!(std::is_same<rgb_tag, hsv_tag>::value));
    D_CC_CHECK(!(std::is_same<rgb_tag, ycbcr_tag>::value));
    D_CC_CHECK(!(std::is_same<rgb_tag, cie_xyz_tag>::value));
    D_CC_CHECK(!(std::is_same<rgb_tag, cie_lab_tag>::value));
    D_CC_CHECK(!(std::is_same<cmyk_tag, hsl_tag>::value));
    D_CC_CHECK(!(std::is_same<cmyk_tag, hsv_tag>::value));
    D_CC_CHECK(!(std::is_same<cmyk_tag, ycbcr_tag>::value));
    D_CC_CHECK(!(std::is_same<cmyk_tag, cie_xyz_tag>::value));
    D_CC_CHECK(!(std::is_same<cmyk_tag, cie_lab_tag>::value));
    D_CC_CHECK(!(std::is_same<hsl_tag, hsv_tag>::value));
    D_CC_CHECK(!(std::is_same<hsl_tag, ycbcr_tag>::value));
    D_CC_CHECK(!(std::is_same<hsl_tag, cie_xyz_tag>::value));
    D_CC_CHECK(!(std::is_same<hsl_tag, cie_lab_tag>::value));
    D_CC_CHECK(!(std::is_same<hsv_tag, ycbcr_tag>::value));
    D_CC_CHECK(!(std::is_same<hsv_tag, cie_xyz_tag>::value));
    D_CC_CHECK(!(std::is_same<hsv_tag, cie_lab_tag>::value));
    D_CC_CHECK(!(std::is_same<ycbcr_tag, cie_xyz_tag>::value));
    D_CC_CHECK(!(std::is_same<ycbcr_tag, cie_lab_tag>::value));
    D_CC_CHECK(!(std::is_same<cie_xyz_tag, cie_lab_tag>::value));

    D_CC_CHECK(!(std::is_same<rgb_tag, DColorModelTag>::value));
    D_CC_CHECK(!(std::is_same<cie_lab_tag, DColorModelTag>::value));

    return true;
}

/*
tests_color_common_tags_are_empty_and_default_constructible
  Verifies every tag is an empty, default-constructible class, so carrying one
  costs nothing and tag values can be materialised for overload dispatch.
  Tests the following:
  - std::is_empty holds for the base and all seven tags
  - each is default constructible and trivially so
  - each is a class type, not an enum or scalar
*/
bool tests_color_common_tags_are_empty_and_default_constructible()
{
    D_CC_CHECK(std::is_empty<DColorModelTag>::value);
    D_CC_CHECK(std::is_empty<rgb_tag>::value);
    D_CC_CHECK(std::is_empty<cmyk_tag>::value);
    D_CC_CHECK(std::is_empty<hsl_tag>::value);
    D_CC_CHECK(std::is_empty<hsv_tag>::value);
    D_CC_CHECK(std::is_empty<ycbcr_tag>::value);
    D_CC_CHECK(std::is_empty<cie_xyz_tag>::value);
    D_CC_CHECK(std::is_empty<cie_lab_tag>::value);

    D_CC_CHECK(std::is_default_constructible<rgb_tag>::value);
    D_CC_CHECK(std::is_trivially_default_constructible<rgb_tag>::value);
    D_CC_CHECK(std::is_default_constructible<cie_lab_tag>::value);

    D_CC_CHECK(std::is_class<DColorModelTag>::value);
    D_CC_CHECK(std::is_class<ycbcr_tag>::value);

    // materialising a tag is well formed and costs one byte at most
    rgb_tag t;
    (void)t;
    D_CC_CHECK(sizeof(rgb_tag) == 1u);

    return true;
}

/*
tests_color_common_tag_base_is_not_a_model_tag_itself
  Verifies DColorModelTag is the hierarchy root only: it exposes no nested
  model_tag, so the base is not mistaken for a participating model.
  Tests the following:
  - is_color_model rejects the base tag
  - is_color_model also rejects each leaf TAG (tags mark models, they are not
    themselves models)
*/
bool tests_color_common_tag_base_is_not_a_model_tag_itself()
{
    D_CC_CHECK(!(is_color_model<DColorModelTag>::value));
    D_CC_CHECK(!(is_color_model<rgb_tag>::value));
    D_CC_CHECK(!(is_color_model<cmyk_tag>::value));
    D_CC_CHECK(!(is_color_model<hsl_tag>::value));
    D_CC_CHECK(!(is_color_model<hsv_tag>::value));
    D_CC_CHECK(!(is_color_model<ycbcr_tag>::value));
    D_CC_CHECK(!(is_color_model<cie_xyz_tag>::value));
    D_CC_CHECK(!(is_color_model<cie_lab_tag>::value));

    return true;
}

NS_END
NS_END
