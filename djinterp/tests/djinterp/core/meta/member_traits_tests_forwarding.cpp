#include "member_traits_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_forwarding_has_member_type_detects
  The detector emitted by the old D_DEFINE_HAS_MEMBER_TYPE spelling works.
  Tests the following:
  - has_flavor_type is true for a type exposing ::flavor_type
  - it is false for a type that does not
  - the result is a bool trait value (exactly true / false)
  - it distinguishes the two probe types
*/
bool
tests_forwarding_has_member_type_detects()
{
    D_DT_CHECK(has_flavor_type<has_flavor>::value);
    D_DT_CHECK(!has_flavor_type<no_flavor>::value);
    D_DT_CHECK(has_flavor_type<has_flavor>::value == true);
    D_DT_CHECK(has_flavor_type<has_flavor>::value !=
               has_flavor_type<no_flavor>::value);

    return true;
}

/*
tests_forwarding_has_member_type_name_is_derived
  The old macro's has_<NAME> name derivation is preserved by the shim.
  Tests the following:
  - the emitted trait is spelled exactly has_flavor_type (naming it compiles)
  - that identifier is a usable unary trait
  - it derives from the expected integral_constant on a hit
  - and on a miss
*/
bool
tests_forwarding_has_member_type_name_is_derived()
{
    // Naming has_flavor_type at all is the proof the name was derived correctly;
    // if the shim had produced a differently-spelled trait this would not
    // compile.
    using hit  = has_flavor_type<has_flavor>;
    using miss = has_flavor_type<no_flavor>;

    D_DT_CHECK(std::is_base_of<std::true_type, hit>::value);
    D_DT_CHECK(std::is_base_of<std::false_type, miss>::value);
    D_DT_CHECK(std::is_same<hit::value_type, bool>::value);

    return true;
}

/*
tests_forwarding_has_member_type_matches_canonical
  The shim-built detector agrees pointwise with the canonical one.
  Tests the following:
  - the two agree on a type that has the member
  - the two agree on a type that does not
  - both strip cv-ref identically (the canonical clean_t behaviour)
  - both reject a value member of the same name identically
*/
bool
tests_forwarding_has_member_type_matches_canonical()
{
    // present / absent
    D_DT_CHECK(has_flavor_type<has_flavor>::value ==
               canon_has_flavor_type<has_flavor>::value);
    D_DT_CHECK(has_flavor_type<no_flavor>::value ==
               canon_has_flavor_type<no_flavor>::value);

    // cv-ref stripping matches
    D_DT_CHECK(has_flavor_type<const has_flavor&>::value ==
               canon_has_flavor_type<const has_flavor&>::value);
    D_DT_CHECK(has_flavor_type<const has_flavor&>::value);   // and is true

    // value-vs-typedef rejection matches
    D_DT_CHECK(has_flavor_type<flavor_value_shaped>::value ==
               canon_has_flavor_type<flavor_value_shaped>::value);
    D_DT_CHECK(!has_flavor_type<flavor_value_shaped>::value);   // and is false

    return true;
}

/*
tests_forwarding_member_type_or_extracts
  The extractor emitted by the old D_DEFINE_MEMBER_TYPE_OR spelling works.
  Tests the following:
  - shim_extract_flavor<has_flavor>::type is the member type (long)
  - the _t alias names the same type
  - it is not the fallback in the present case
  - extraction strips cv-ref (clean_t) so a reference extracts the same type
*/
bool
tests_forwarding_member_type_or_extracts()
{
    D_DT_CHECK((std::is_same<shim_extract_flavor<has_flavor>::type, long>::value));
    D_DT_CHECK((std::is_same<shim_extract_flavor_t<has_flavor>, long>::value));
    D_DT_CHECK(!(std::is_same<shim_extract_flavor<has_flavor>::type, void>::value));

    // clean_t means a reference extracts the same member type
    D_DT_CHECK((std::is_same<shim_extract_flavor<const has_flavor&>::type, long>::value));

    return true;
}

/*
tests_forwarding_member_type_or_falls_back
  The extractor yields the fallback when the member is absent.
  Tests the following:
  - shim_extract_flavor<no_flavor>::type is the fallback (void)
  - the _t alias yields void as well
  - a value member of the same name is not extracted (falls back)
  - the present case still extracts (contrast)
*/
bool
tests_forwarding_member_type_or_falls_back()
{
    D_DT_CHECK((std::is_same<shim_extract_flavor<no_flavor>::type, void>::value));
    D_DT_CHECK((std::is_same<shim_extract_flavor_t<no_flavor>, void>::value));

    // a value named flavor_type is not a typedef, so extraction falls back
    D_DT_CHECK((std::is_same<shim_extract_flavor<flavor_value_shaped>::type, void>::value));

    // contrast: the present case extracts
    D_DT_CHECK((std::is_same<shim_extract_flavor<has_flavor>::type, long>::value));

    return true;
}

/*
tests_forwarding_member_type_or_matches_canonical
  The shim-built extractor agrees with the canonical one.
  Tests the following:
  - the two extract the same type in the present case
  - the two yield the same fallback in the absent case
  - both handle the cv-ref case identically
  - the _t aliases agree as well
*/
bool
tests_forwarding_member_type_or_matches_canonical()
{
    // present
    D_DT_CHECK((std::is_same<shim_extract_flavor<has_flavor>::type,
                             canon_extract_flavor<has_flavor>::type>::value));

    // absent
    D_DT_CHECK((std::is_same<shim_extract_flavor<no_flavor>::type,
                             canon_extract_flavor<no_flavor>::type>::value));

    // cv-ref
    D_DT_CHECK((std::is_same<shim_extract_flavor<const has_flavor&>::type,
                             canon_extract_flavor<const has_flavor&>::type>::value));

    // _t aliases
    D_DT_CHECK((std::is_same<shim_extract_flavor_t<has_flavor>,
                             canon_extract_flavor_t<has_flavor> >::value));

    return true;
}

/*
tests_forwarding_shim_reexports_new_detectors
  Including the shim brings the new headers' surface into scope.
  Tests the following:
  - has_value_type (from member_types.hpp) is visible and works
  - it reports absence correctly too
  - internal::pick_member_type (also from member_types.hpp) is visible and works
  - the shim is a drop-in for code that used to rely on these being present
*/
bool
tests_forwarding_shim_reexports_new_detectors()
{
    // has_value_type came transitively via member_types.hpp
    D_DT_CHECK(has_value_type<has_flavor>::value == false);   // has_flavor has flavor_type, not value_type

    struct with_value { using value_type = int; };
    D_DT_CHECK(has_value_type<with_value>::value);

    // pick_member_type is visible too (fully qualified: the extractor-macro
    // invocations in the suite header opened a djinterp::testing::internal, so
    // an unqualified `internal::` would resolve there instead)
    D_DT_CHECK((std::is_same<
                    ::djinterp::internal::pick_member_type<true, int, void>::type,
                    int>::value));
    D_DT_CHECK((std::is_same<
                    ::djinterp::internal::pick_member_type<false, int, char>::type,
                    char>::value));

    return true;
}

NS_END  // testing
NS_END  // djinterp
